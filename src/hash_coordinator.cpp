#include <hash_coordinator.h>

#include <QtCore/QDir>
#include <QtCore/QTimer>
#include <QtCore/QString>

HashCoordinator::HashCoordinator(QObject* _parent) : cancelled(false)
	{
	connect(this, SIGNAL(processDirectory(QString, bool)),
	        this, SLOT(doHashDirectory(QString, bool)),
			Qt::QueuedConnection);
	connect(this, SIGNAL(cancelHashing()),
	        this, SLOT(receive_cancelHashing()));
	connect(this, SIGNAL(resetHashing()),
	        this, SLOT(receive_resetHashing()));
	connect(&db, SIGNAL(message(QString)),
	        this, SIGNAL(send_message(QString)),
			Qt::QueuedConnection);
	connect(&db, SIGNAL(message_missing(QString)),
	        this, SIGNAL(send_missing(QString)),
			Qt::QueuedConnection);
	connect(&db, SIGNAL(message_new(QString)),
	        this, SIGNAL(send_new(QString)),
			Qt::QueuedConnection);

	connect(this, SIGNAL(getMissing()),
	        &db, SLOT(generateMissingObjects()));
	connect(this, SIGNAL(getNew()),
	        &db, SLOT(generateNewObjects()));
	connect(this, SIGNAL(copyMissing(QString, QString)),
	        &db, SLOT(copyMissingObjects(QString, QString)));
	connect(&db, SIGNAL(copyFile(int, QString, QString)),
	        this, SLOT(doCopyFile(int, QString, QString)));
	connect(this, SIGNAL(resetDatabase()),
	        &db, SLOT(resetDatabase()));

	for(unsigned int i = 0; i < HASHER_THREAD_COUNT; ++i)
		{
		hashers[i].hasher.setModulo(i);
		connect(this, SIGNAL(cancelHashing()),
				&(hashers[i].hasher), SLOT(receive_cancelHashing()));
		connect(this, SIGNAL(resetHashing()),
				&(hashers[i].hasher), SLOT(receive_resetHashing()));
		connect(this, SIGNAL(processFile(int, QString, bool)),
		        &(hashers[i].hasher), SLOT(processFile(int, QString, bool)),
				Qt::QueuedConnection);
		connect(&(hashers[i].hasher), SIGNAL(fileData(QString, QByteArray, bool)),
		        this, SLOT(receive_hash(QString, QByteArray, bool)),
				Qt::QueuedConnection);

		hashers[i].hasher.moveToThread(&(hashers[i].thread));
		hashers[i].thread.start();
		}

	for (unsigned int i = 0; i < COPIER_THREAD_COUNT; ++i)
		{
		copiers[i].copier.setModulo(i);
		connect(this, SIGNAL(copyFile(int, QString, QString)),
		        &(copiers[i].copier), SLOT(copyFile(int, QString, QString)),
				Qt::QueuedConnection);
		connect(&(copiers[i].copier), SIGNAL(send_message(QString)),
		        this, SIGNAL(send_message(QString)));
		copiers[i].copier.moveToThread(&(copiers[i].thread));
		copiers[i].thread.start();
		}
	}
HashCoordinator::~HashCoordinator()
	{
	Q_EMIT send_message("Stopping Hashers");
	for(unsigned int i = 0; i < HASHER_THREAD_COUNT; ++i)
		{
		hashers[i].thread.quit();
		hashers[i].thread.wait();
		}
	Q_EMIT send_message("Stopping Copiers");
	for (unsigned int i = 0; i < COPIER_THREAD_COUNT; ++i)
		{
		copiers[i].thread.quit();
		copiers[i].thread.wait();
		}
	Q_EMIT send_message("All tasks stopped");
	}

void HashCoordinator::startHashing(QString _path, bool _generate)
	{
	Q_EMIT hashing_started();
	hash_directory(_path, _generate);
	Q_EMIT hashing_pending();
	}

void HashCoordinator::doHashDirectory(QString _path, bool _mode)
	{
	if (cancelled)
		{
		Q_EMIT send_message(QString("Cancelling on Path: %1").arg(_path));
		return;
		}
	hash_directory(_path, _mode);
	}

void HashCoordinator::doCopyFile(int _count, QString _source, QString _destination)
	{
	int copy_index = _count % COPIER_THREAD_COUNT;
	Q_EMIT copyFile(copy_index, _source, _destination);
	}

void HashCoordinator::hash_directory(QString _path, bool _generate)
	{
	Q_EMIT send_message(QString("Processing Path: %1").arg(_path));
	Q_EMIT send_message(QString("Generate: %1").arg(_generate));

	QDir directories(_path);
	QDir files(_path);

	directories.setFilter(QDir::Dirs|
							QDir::NoDotAndDotDot|
							QDir::Hidden|QDir::System);
	directories.setSorting(QDir::Name);
	QStringList dirList = directories.entryList();
	for (QStringList::iterator iter = dirList.begin(); iter != dirList.end(); ++iter)
		{
		QString full_path_to_check = _path + directories.separator() + (*iter);
		Q_EMIT send_message(QString("Found sub-directory: %1 -> %2").arg(*iter).arg(full_path_to_check));
		Q_EMIT processDirectory(full_path_to_check, _generate);
		}

	int hash_index = 0;
	files.setFilter(QDir::Files|
					QDir::Hidden|QDir::System|
					QDir::NoSymLinks);
	files.setSorting(QDir::Name);
	QStringList fileList = files.entryList();
	for (QStringList::iterator iter = fileList.begin(); iter != fileList.end(); ++iter)
		{
		QString full_file_to_check = _path + directories.separator() + (*iter);
		Q_EMIT send_message(QString("Found file: %1 -> %2").arg(*iter).arg(full_file_to_check));
		Q_EMIT send_message(QString("\tHashing on index: %1").arg(hash_index));
		Q_EMIT processFile(hash_index, full_file_to_check, _generate);

		hash_index = (hash_index + 1) % HASHER_THREAD_COUNT;
		}
	}

void HashCoordinator::receive_cancelHashing()
	{
	Q_EMIT send_message("Cancel Hashing");
	cancelled = true;
	}

void HashCoordinator::receive_resetHashing()
	{
	Q_EMIT send_message("Reset Hashing");
	cancelled = false;
	}

void HashCoordinator::receive_hash(QString _path, QByteArray _hash_value, bool _generate)
	{
	Q_EMIT send_message(QString("Received Hash of %1 on file %2 - generate: %3").arg(QString(_hash_value)).arg(_path).arg(_generate));
	db.addFile(0, _path, _hash_value, _generate);
	}
