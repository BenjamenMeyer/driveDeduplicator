#ifndef FILE_COPIER_H__
#define FILE_COPIER_H__

#include <QtCore/QObject>

class FileCopier : public QObject
	{
	Q_OBJECT
	public:

		FileCopier(QObject* _parent=NULL);
		virtual ~FileCopier();

		int getModulo() const;
		void setModulo(int _m);

	public Q_SLOTS:
		void copyFile(int _modulo, QString _source, QString _destination);

	Q_SIGNALS:
		void send_message(QString _message);

	protected:
		int modulo;
	protected Q_SLOTS:
	private:
	private Q_SLOTS:
	};

#endif //FILE_COPIER_H__

