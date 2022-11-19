#include <QApplication>

#include <driveDeduplicator.h>

int main(int argc, char* argv[])
	{
	QApplication theApplication(argc, argv);
	DriveDeduplicator main(NULL);
	main.show();
	return theApplication.exec();
	}
