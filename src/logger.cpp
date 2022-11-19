#include <logger.h>

#include <QtCore/QFile>
#include <QtCore/QIODevice>
#include <QtCore/QString>
#include <QtCore/QStringList>

Logger::Logger(QObject* _parent): QObject(_parent)
	{
	setLogFile(QString(".application-logger.log"));
	}
Logger::~Logger()
	{
	if (logFile.isOpen())
		{
		logFile.close();
		}
	}

void Logger::setLogFile(QString _filename)
	{
	QStringList logFileOptions;

	logFileOptions << logFile.fileName() << _filename;

	if (logFile.isOpen())
		{
		logFile.close();
		}

	for (QStringList::iterator iter = logFileOptions.begin(); iter != logFileOptions.end(); ++iter)
		{
		if (!(*iter).isEmpty())
			{
			logFile.setFileName((*iter));
			if (logFile.open(QIODevice::Text|QIODevice::Append|QIODevice::WriteOnly|QIODevice::Unbuffered))
				{
				break;
				}
			}
		}
	
	if (logFile.isOpen())
		{
		Q_EMIT send_message(QString("Recording log to file %1").arg(logFile.fileName()));
		}
	else
		{
		Q_EMIT send_message("No log file to record in use");
		}
	}

QString Logger::getLogFile() const
	{
	return logFile.fileName();
	}

void Logger::message(QString _message)
	{
	if (logFile.isOpen())
		{
		logFile.write(_message.toLatin1());
		logFile.write("\n");
		}
	}
