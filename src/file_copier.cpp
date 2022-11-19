#include <file_copier.h>

#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

FileCopier::FileCopier(QObject* _parent) : QObject(_parent), modulo(0)
	{
	}
FileCopier::~FileCopier()
	{
	}

int FileCopier::getModulo() const
	{
	return modulo;
	}
void FileCopier::setModulo(int _m)
	{
	modulo = _m;
	}

void FileCopier::copyFile(int _modulo, QString _source, QString _destination)
	{
	if (_modulo != modulo)
		{
		return;
		}

	QFileInfo destination_info(_destination);
	QString destination_path = destination_info.absolutePath();
	
	QDir dir;
	if (!dir.mkpath(destination_path))
		{
		Q_EMIT send_message(QString("Copier [%1] - Failed to create directory %2)").arg(modulo).arg(destination_path));
		Q_EMIT send_message(QString("Copier [%1] - Did not copy %2").arg(modulo).arg(_destination));
		return;
		}

	if (QFile::copy(_source, _destination))
		{
		Q_EMIT send_message(QString("Copier [%1] - Copied %2 to %3").arg(modulo).arg(_source).arg(_destination));
		}
	else
		{
		Q_EMIT send_message(QString("Copier [%1] - FAILED to copy %2 to %3").arg(modulo).arg(_source).arg(_destination));
		}
	}
