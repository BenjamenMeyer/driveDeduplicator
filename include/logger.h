#ifndef MY_LOGGER_H__
#define MY_LOGGER_H__

#include <QtCore/QFile>
#include <QtCore/QObject>
#include <QtCore/QString>

class Logger: public QObject
	{
	Q_OBJECT
	public:
		Logger(QObject* _parent=NULL);
		virtual ~Logger();

		void setLogFile(QString _filename);
		QString getLogFile() const;

	public Q_SLOTS:
		void message(QString _message);

	Q_SIGNALS:
		void send_message(QString _message);

	protected:
		QFile logFile;

	protected Q_SLOTS:
	private:
	private Q_SLOTS:
	};

#endif //MY_LOGGER_H__

