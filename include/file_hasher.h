#ifndef FILE_HASHER_H__
#define FILE_HASHER_H__

#include <QtCore/QObject>

class FileHasher : public QObject
	{
	Q_OBJECT
	public:

		FileHasher(QObject* _parent=NULL);
		virtual ~FileHasher();

		int getModulo() const;
		void setModulo(int _m);

	public Q_SLOTS:
		void processFile(int _modulo, QString _path, bool _generate);
		void receive_cancelHashing();
		void receive_resetHashing();

	Q_SIGNALS:
		void fileData(QString _path, QByteArray _hash, bool _generate);
		void send_message(QString _message);

	protected:
		int modulo;
		bool cancelled;
	protected Q_SLOTS:
	private:
	private Q_SLOTS:
	};

#endif //FILE_HASHER_H__
