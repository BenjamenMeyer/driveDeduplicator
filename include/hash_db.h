#ifndef HASH_DB_H__
#define HASH_DB_H__

#include <stdint.h>

#include <QtCore/QObject>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

class HashDb : public QObject
	{
	Q_OBJECT
	public:
		HashDb(QObject* _parent=NULL);
		~HashDb();

        bool hasPath(QString _path);
        bool hasPathHash(QString _hash);
        bool addPath(QString _path);
        bool addPathHash(QString _hash);
        uint32_t getPathId(QString _path);
        uint32_t getPathHashId(QString _hash);
        bool associatePathHash(QString _path, QString _hash);

        bool hasFile(QString _path, QString _name);
        bool hasFileHash(QString _hash);
        bool addFile(QString _path, QString _name);
        bool addFileHash(QString _hash);
        uint32_t getFileId(QString _path, QString _name);
        uint32_t getFileHashId(QString _hash);
        bool associateFileHash(QString _path, QString _name, QString _hash);

	public Q_SLOTS:
		//void generateMissingObjects();
		//void generateNewObjects();
		//void copyMissingObjects(QString _source_path, QString _destination_path);

		void resetDatabase();

	Q_SIGNALS:
		void message(QString);
		//void message_missing(QString _message);
		//void message_new(QString _new);
		//void copyFile(int _modulo, QString _source_path, QString _destination_path);

	protected:
		QSqlDatabase db;                            // The in-memory database

        QSqlQuery SQL_INSERT_PATH;                  // Add a specific directory to the path table
        QSqlQuery SQL_INSERT_PATH_HASH;             // Add the hash of a specific directory to the path's hash table
        QSqlQuery SQL_INSERT_PATH_MAP;              // Map a directory to its hash
        QSqlQuery SQL_HAS_PATH;                     // Check if a directory has already been added
        QSqlQuery SQL_HAS_PATH_HASH;                // Check if a directory hash has already been added
        QSqlQuery SQL_GET_DIRECTORIES_BY_HASH;      // Access a directory by its hash
        QSqlQuery SQL_GET_HASH_FOR_PATH;            // Get all directories that have the *same* hash
        QSqlQuery SQL_GET_UNVERIFIED_DIRECTORIES;   // Get any directory that has not been verified (it has no hash)
        QSqlQuery SQL_GET_VERIFIED_DIRECTORIES;     // Get any directory that *has* been verified (it has a hash)

		QSqlQuery SQL_INSERT_FILE;                  // Add a specific file to the file table
        QSqlQuery SQL_INSERT_FILE_HASH;             // Add the hash of a specific file to the file's hash table
        QSqlQuery SQL_INSERT_FILE_MAP;              // Map a file to its hash
        QSqlQuery SQL_HAS_FILE;                     // Check if a file has already been added
        QSqlQuery SQL_HAS_FILE_HASH;                // Check if a file hash hash already been added
        QSqlQuery SQL_GET_FILES_BY_HASH;            // Access a file by its hash
        QSqlQuery SQL_GET_HASH_FOR_FILE;            // Get all files that have the *same* hash
        QSqlQuery SQL_GET_UNVERIFIED_FILES;         // Get any file that has not been verified (it has no hash)
        QSqlQuery SQL_GET_VERIFIED_FILES;           // Get any file that *has* been verified (it has a hash)

		void init_database();
        void log_query_error(const QSqlQuery& _query, const QString& _message);

	protected Q_SLOTS:
	private:
	private Q_SLOTS:
	};

#endif //HASH_DB_H__
