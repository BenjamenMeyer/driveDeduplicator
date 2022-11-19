#include <hash_db.h>

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtSql/QSqlRecord>

/*
    The below tables outline how to efficiently store the information.
    Need to rethink how the comparisons happen though.
    Perhaps we just do a scan of each path, and then the report is on duplicates.

	There are two methods of looking at the data:
		1. Drive -> Directory -> File -> Hash
		2. Hash -> (File, Directory, Drive)

    Table: FileHashes
        - Key: file hash
        - Contains: file hash, file id

        hash - TEXT, NOT NULL PRIMARY KEY
        id - INT, NOT NULL PRIMARY KEY, AUTOINCREMENT

    Table: DirectoryHashes
        - Key: Composite of all file hashes in a given directory
            using the following psuedo-code:
                hasher = new hashObject()
                for file_hash in directory.orderBy(fileid)
                    hasher.update(file_hash)
                for subdir_hash in directory.orderBy(subdirid)
                    hasher.update(subdir_hash)

                directory hash = hasher.hexdigest()
        - Contains: directory hash, directory id

        hash - TEXT, NOT NULL, PRIMARY KEY
        id - INT, NOT NULL PRIMARY KEY, AUTOINCREMENT

    Table: FileMap
        - Keys: hashid, fileid
        hashid - INT, NOT NULL, PRIMARY KEY, FOREIGN KEY(FileHashes: id)
        fileid - INT, NOT NULL, PRIMARY KEY, FOREIGN KEY(Files: id)

    Table: DirectoryMap
        - Keys: hashid, directoryid
        hashid: INT, NOT NULL, PRIMARY KEY, FOREIGN KEY(DirectoryHashes: id)
        directoryid: INT, NOT NULL, PRIMARY KEY, FOREIGN KEY(Directories: id)

    Table: Directories
        - List of directory paths
        path: TEXT, NOT NULL
        id: INT, NOT NULL, PRIMARY KEY, AUTOINCREMENT

        Note: This might need another optional field that is self-referential
            for tracking parent paths.

    Table: Files
        - List of file names
        directoryid: INT, NOT NULL, FOREIGN KEY(Directories: id)
        name: TEXT, NOT NULL
        id: INT, NOT NULL, PRIMARY KEY, AUTOINCREMENT

    ----------------------------------------
	Table: Drives
		Index - Int, Auto-Increment --> rowid
		Drive - Text, NOT NULL

	Table: Directory
		Index - Int, Auto-Increment --> rowid
		Drive ID - Int, Foreign Key (Drives:Index)
		Directory - Text, NOT NULL

	Table: Files
		Index - Int, Auto-Increment --> rowid
		Directory ID - Int, Foreign Key (Directory: Index)
		File - Text, NOT NULL
		Hash - Text, NOT NULL
 */

QStringList schemas = (
	QStringList() <<
        // Hash Listings
        QString("CREATE TABLE IF NOT EXISTS fileHashes (hash TEXT NOT NULL)") <<
        QString("CREATE TABLE IF NOT EXISTS pathHashes (hash TEXT NOT NULL)") <<

        // Path Listings
        // NOTE: `rowid` is an internally generated for in Sqlite
        QString("CREATE TABLE IF NOT EXISTS paths (path TEXT NOT NULL)") <<
        QString("CREATE TABLE IF NOT EXISTS files (name TEXT NOT NULL, pathid INTEGER, FOREIGN KEY (pathid) REFERENCES paths(rowid))") <<

        // Cross Referencing
        // NOTE: `rowid` is an internally generated for in Sqlite
        QString("CREATE TABLE IF NOT EXISTS fileMap (hashid INTEGER, fileid INTEGER, FOREIGN KEY (hashid) REFERENCES fileHashes(rowid), FOREIGN KEY (fileid) REFERENCES files(rowid))") <<
        QString("CREATE TABLE IF NOT EXISTS pathMap (hashid INTEGER, pathid INTEGER, FOREIGN KEY (hashid) REFERENCES pathHashes(rowid), FOREIGN KEY (pathid) REFERENCES paths(rowid))")
);

QStringList resetSchemas = (
	QStringList() <<
        QString("DROP TABLE IF EXISTS paths") <<
        QString("DROP TABLE IF EXISTS pathHashes") <<
        QString("DROP TABLE IF EXISTS pathMap") <<
        QString("DROP TABLE IF EXISTS files") <<
        QString("DROP TABLE IF EXISTS fileHashes") <<
        QString("DROP TABLE IF EXISTS fileMap")
);

static void PrepareSqlStatement(QSqlQuery& name, QSqlDatabase& the_db, const char* the_statement)
    {
	name = QSqlQuery(the_db);
    if (!name.prepare(the_statement))
		{
		qDebug() << "Failed to prepare " << the_statement;
		qDebug() << "Error: " << name.lastError();
		}
    }

HashDb::HashDb(QObject* _parent) : QObject(_parent)
	{
	db = QSqlDatabase::addDatabase("QSQLITE");
	db.setDatabaseName(":memory:");
	db.open();
    init_database();
    // REMINDER: `rowid` is an internally generated for in Sqlite
    PrepareSqlStatement(SQL_INSERT_PATH, db, "INSERT INTO paths(path) VALUES(:path);");
    PrepareSqlStatement(SQL_INSERT_PATH_HASH, db, "INSERT INTO pathHashes(hash) VALUES(:hash);");
    PrepareSqlStatement(SQL_INSERT_PATH_MAP, db, "INSERT INTO pathMap(hashid, pathid) VALUES(:hashid, :pathid);");
    PrepareSqlStatement(SQL_HAS_PATH, db, "SELECT COUNT(*) FROM paths WHERE path = :path");
    PrepareSqlStatement(SQL_HAS_PATH_HASH, db, "SELECT COUNT(*) FROM pathHashes WHERE hash = :hash");
    PrepareSqlStatement(SQL_GET_DIRECTORIES_BY_HASH, db, "SELECT path FROM paths WHERE rowid IN (SELECT pathid FROM pathMap WHERE hashid = (SELECT rowid FROM pathHashes WHERE hash = :hash));");
    PrepareSqlStatement(SQL_GET_HASH_FOR_PATH, db, "SELECT hash FROM pathHashes WHERE rowid IN (SELECT hashid FROM pathMap WHERE pathid IN (SELECT rowid FROM paths WHERE path = :path))");
    PrepareSqlStatement(SQL_GET_UNVERIFIED_DIRECTORIES, db, "SELECT path FROM paths WHERE rowid NOT IN (SELECT pathid FROM pathmap)");
    PrepareSqlStatement(SQL_GET_VERIFIED_DIRECTORIES, db, "SELECT path FROM paths WHERE rowid IN (SELECT pathid FROM pathmap)");

    // REMINDER: `rowid` is an internally generated for in Sqlite
    PrepareSqlStatement(SQL_INSERT_FILE, db, "INSERT INTO files(name, pathid) VALUES(:name, :pathid);");
    PrepareSqlStatement(SQL_INSERT_FILE_HASH, db, "INSERT INTO fileHashes(hash) VALUES(:hash);");
    PrepareSqlStatement(SQL_INSERT_FILE_MAP, db, "INSERT INTO fileMap(hashid, fileid) VALUES(:hashid, :fileid);");
    PrepareSqlStatement(SQL_HAS_FILE, db, "SELECT COUNT(*) FROM files WHERE name = :name AND pathid = (SELECT rowid FROM paths WHERE path = :path)");
    PrepareSqlStatement(SQL_HAS_FILE_HASH, db, "SELECT COUNT(*) FROM fileHashes WHERE hash = :hash");
    PrepareSqlStatement(SQL_GET_FILES_BY_HASH, db, "SELECT name FROM files WHERE rowid IN (SELECT fileid FROM fileMap WHERE hashid = (SELECT rowid FROM fileHashes WHERE hash = :hash));");
    PrepareSqlStatement(SQL_GET_HASH_FOR_FILE, db, "SELECT hash FROM fileHashes WHERE rowid IN (SELECT hashid FROM fileMap WHERE fileid IN (SELECT rowid FROM files WHERE name = :name AND pathid = (SELECT rowid FROM paths WHERE path = :path)))");
    PrepareSqlStatement(SQL_GET_UNVERIFIED_FILES, db, "SELECT name, pathid FROM files WHERE rowid NOT IN (SELECT fileid FROM filemap)");
    PrepareSqlStatement(SQL_GET_VERIFIED_FILES, db, "SELECT name, pathid FROM files WHERE rowid IN (SELECT fileid FROM filemap)");
	}

HashDb::~HashDb()
	{
	db.close();
	}

void HashDb::init_database()
	{
	if (db.isOpen())
		{
        // clear any existing schema
		for (QStringList::iterator iter = resetSchemas.begin();
			 iter != resetSchemas.end();
			 ++iter)
			 {
			 QSqlQuery setup(db);
			 setup.prepare((*iter));
			 if (!setup.exec())
			 	{
				Q_EMIT message("Failed to drop table...");
				Q_EMIT message(QString("Query: %1").arg(setup.executedQuery()));
				Q_EMIT message(QString("Error: %1").arg(setup.lastError().text()));

				/*
				qDebug() << "Failed to drop table...";
				qDebug() << "Query: " << setup.executedQuery();
				qDebug() << "Error: " << setup.lastError();
				*/
				}
	  		 setup.clear();
			 db.commit();
			 }
        // build out the schema
		for (QStringList::iterator iter = schemas.begin();
			 iter != schemas.end();
			 ++iter)
			 {
			 QSqlQuery setup(db);
			 setup.prepare((*iter));
			 if (!setup.exec())
			 	{
				Q_EMIT message("Failed to create table...");
				Q_EMIT message(QString("Query: %1").arg(setup.executedQuery()));
				Q_EMIT message(QString("Error: %1").arg(setup.lastError().text()));

				qDebug() << "Failed to create table...";
				qDebug() << "Query: " << setup.executedQuery();
				qDebug() << "Error: " << setup.lastError();
				}
			 setup.clear();
			 db.commit();
			 }
		 }
	}
void HashDb::resetDatabase()
	{
	Q_EMIT message("Resetting database...");
	init_database();
	}

void HashDb::log_query_error(const QSqlQuery& _query, const QString& _message)
    {
    Q_EMIT message(_message);
    Q_EMIT message(QString("Query: %1").arg(_query.executedQuery()));
    QMapIterator<QString, QVariant> bv = insertion.boundValues();
    while(bv.hasNext())
        {
        bv.next();
        Q_EMIT message(QString("Mapped - Key: %1, Value: %2").arg(bv.key(), bv.value().toString()));
        }
    Q_EMIT message(QString("Error: %1").arg(_query.lastError().text()));
    }
/********************
 *** Path Support ***
 ********************/
bool HashDb::hasPath(QString _path)
    {
    bool result = false;
    if (db.isOpen())
        {
        QSqlQuery verifyPath = SQL_HAS_PATH;
        verifyPath.bindValue(":path", _path);
        
        if (verifyPath.exec())
            {
            if (verifyPath.next())
                {
                int count = verifyPath.value(1).toInt();
                result = (count > 0);
                }
            else
                {
                log_query_error(verifyPath, QString("Failed to search for path"));
                }
            }
		else
			{
            log_query_error(verifyPath, QString("Failed to search for path"));
			}
        }
    else
        {
        Q_EMIT message("Database closed");
        }
    return result;
    }
bool HashDb::hasPathHash(QString _hash)
    {
    bool result = false;
    if (db.isOpen())
        {
        QSqlQuery verifyPathHash = SQL_HAS_PATH_HASH;
        verifyPathHash.bindValue(":hash", _hash);
        
        if (verifyPathHash.exec())
            {
            if (verifyPathHash.next())
                {
                int count = verifyPathHash.value(1).toInt();
                result = (count > 0);
                }
            else
                {
                log_query_error(verifyPathHash, QString("Failed to search for path hash"));
                }
            }
		else
			{
            log_query_error(verifyPathHash, QString("Failed to search for path hash"));
			}
        }
    else
        {
        Q_EMIT message("Database closed");
        }
    return result;
    }
bool HashDb::addPath(QString _path)
    {
    bool result = false;
    if (db.isOpen())
        {
        if (!hasPath(_path))
            {
            db.transaction();
            Q_EMIT message(QString("Adding Directory %1 to the database").arg(_path));
            QSqlQuery insertPath = SQL_INSERT_PATH;
            insertPath.bindValue(":path", _path);
            if (insertPath.exec())
                {
                db.commit()
                result = true;
                }
            else
                {
                log_query_error(insertPath, QString("Failed to insert path %1").arg(_path));
                db.rollback();
                }
            }
        else
            {
            // it's already there - just treat it like success
            result = true;
            }
        }
    else
        {
        Q_EMIT message("Database closed");
        }
    return result;
    }
bool HashDb::addPathHash(QString _hash)
    {
    bool result = false;
    if (db.isOpen())
        {
        if (!hasPathHash(_hash))
            {
            db.transaction();
            Q_EMIT message(QString("Adding Directory Hash %1 to the database").arg(_hash));
            QSqlQuery insertPathHash = SQL_INSERT_PATH_HASH;
            insertPathHash.bindValue(":hash", _hash);
            if (insertPathHash.exec())
                {
                db.commit()
                result = true;
                }
            else
                {
                log_query_error(insertPathHash, QString("Failed to insert hash %1 for path %2").arg(_hash).arg(_path));
                db.rollback();
                }
            }
        else
            {
            // it's already there - just treat it like success
            result = true;
            }
        }
    else
        {
        Q_EMIT message("Database closed");
        }
    return result;
    }
bool HashDb::associatePathHash(QString _path, QString _hash)
    {
    bool result = false;
    if (db.isOpen())
        {
        bool skip = false;
        if (!hasPathHash(_hash))
            {
            if (!addPathHash(_hash))
                {
                result = false;
                skip = true;
                }
            }

        if (!hasPath(_path))
            {
            if (!addPath(_path))
                {
                result = false;
                skip = true;
                }
            }

        if (!skip)
            {
            db.transaction();
            Q_EMIT message(QString("Associating Directory Path %1 to hash %2").arg(_path).arg(_hash));

            QSqlQuery insertPathHashAssociation = SQL_INSERT_PATH_MAP;
            insertPathHashAssociation.bindValue(":hash", _hash);
            insertPathHashAssociation.bindValue(":path", _path);
            if (insertPathHashAssociation.exec())
                {
                db.commit();
                result = true;
                }
            else
                {
                db.rollback();
                }
            }
        }
    else
        {
        Q_EMIT message("Database closed");
        }
    return result;
    }
/********************
 *** File Support ***
 ********************/
bool HashDb::hasFile(QString _path, QString _name)
    {
    bool result = false;
    if (db.isOpen())
        {
        QSqlQuery verifyFile = SQL_HAS_FILE;
        verifyFile.bindValue(":name", _name);
        verifyFile.bindValue(":path", _path);
        
        if (verifyFile.exec())
            {
            if (verifyFile.next())
                {
                int count = verifyFile.value(1).toInt();
                result = (count > 0);
                }
            else
                {
                log_query_error(verifyFile, QString("Failed to search for file"));
                }
            }
		else
			{
            log_query_error(verifyFile, QString("Failed to search for file"));
			}
        }
    else
        {
        Q_EMIT message("Database closed");
        }
    return result;
    }
bool HashDb::hasFileHash(QString _hash)
    {
    bool result = false;
    if (db.isOpen())
        {
        QSqlQuery verifyFileHash = SQL_HAS_FILE_HASH;
        verifyFileHash.bindValue(":hash", _hash);
        
        if (verifyFileHash.exec())
            {
            if (verifyFileHash.next())
                {
                int count = verifyFileHash.value(1).toInt();
                result = (count > 0);
                }
            else
                {
                log_query_error(verifyFileHash, QString("Failed to search for file hash"));
                }
            }
		else
			{
            log_query_error(verifyFileHash, QString("Failed to search for file hash"));
			}
        }
    else
        {
        Q_EMIT message("Database closed");
        }
    return result;
    }
bool HashDb::addFile(QString _path, QString _name)
    {
    bool result = false;
    if (db.isOpen())
        {
        // each is only half the check so it's safe to do these irrespective of order
        if (!(hasPath(_path) && hasFile(_name)))
            {
            int pathid = getPathId(_path);

            db.transaction();
            Q_EMIT message(QString("Adding File %1 to the database").arg(_name));
            QSqlQuery insertFile = SQL_INSERT_FILE;
            insertFile.bindValue(":pathid", pathid);
            insertFile.bindValue(":name", _name);

            if (insertPath.exec())
                {
                db.commit()
                result = true;
                }
            else
                {
                log_query_error(insertFile, QString("Failed to insert file %1").arg(_name));
                db.rollback();
                }
            }
        else
            {
            // it's already there - just treat it like success
            result = true;
            }
        }
    else
        {
        Q_EMIT message("Database closed");
        }
    return result;
    }
bool HashDb::addFileHash(QString _hash)
    {
    bool result = false;
    if (db.isOpen())
        {
        if (!hasPathHash(_hash))
            {
            db.transaction();
            Q_EMIT message(QString("Adding Directory Hash %1 to the database").arg(_hash));
            QSqlQuery insertPathHash = SQL_INSERT_PATH_HASH;
            insertPathHash.bindValue(":hash", _hash);
            if (insertPathHash.exec())
                {
                db.commit()
                result = true;
                }
            else
                {
                log_query_error(insertPathHash, QString("Failed to insert hash %1 for path %2").arg(_hash).arg(_path));
                db.rollback();
                }
            }
        else
            {
            // it's already there - just treat it like success
            result = true;
            }
        }
    else
        {
        Q_EMIT message("Database closed");
        }
    return result;
    }
bool HashDb::associateFileHash(QString _path, QString _name, QString _hash)
    {
    bool result = false;
    if (db.isOpen())
        {
        bool skip = false;
        if (!hasPathHash(_hash))
            {
            if (!addPathHash(_hash))
                {
                result = false;
                skip = true;
                }
            }

        if (!hasPath(_path))
            {
            if (!addPath(_path))
                {
                result = false;
                skip = true;
                }
            }

        if (!skip)
            {
            db.transaction();
            Q_EMIT message(QString("Associating Directory Path %1 to hash %2").arg(_path).arg(_hash));

            QSqlQuery insertPathHashAssociation = SQL_INSERT_PATH_MAP;
            insertPathHashAssociation.bindValue(":hash", _hash);
            insertPathHashAssociation.bindValue(":path", _path);
            if (insertPathHashAssociation.exec())
                {
                db.commit();
                result = true;
                }
            else
                {
                db.rollback();
                }
            }
        }
    else
        {
        Q_EMIT message("Database closed");
        }
    return result;
    }
