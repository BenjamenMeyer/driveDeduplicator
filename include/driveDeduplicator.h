#ifndef DRIVE_DEDUPLICATOR_H__
#define DRIVE_DEDUPLICATOR_H__

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QThread>
#include <QtGui/QCheckBox>
#include <QtGui/QGroupBox>
#include <QtGui/QLabel>
#include <QtGui/QListWidget>
#include <QtGui/QPushButton>
#include <QtGui/QTextEdit>
#include <QtGui/QTreeView>
#include <QtGui/QWidget>

#include <fileModel.h>
#include <hash_coordinator.h>
#include <hash_db.h>
#include <logger.h>

class DriveDeduplicator: public QWidget
    {
    Q_OBJECT
    public:
        DriveDeduplicator(QWidget* _parent);
        virtual ~DriveDeduplicator();

    public Q_SLOTS:
        void logMessage(QtMsgType _type, QString _msg);

    Q_SIGNALS:
        void startHashing(QString _path, bool _generate);
        void resetHashing();

        //void getMissing();
        //void getNew();
        // void copyMissing(QString _source_path, QString _destination_path);

        void resetDatabase();
        void clearResultDisplay();

        void send_message(QString _message);

    protected:
        void createLayout();

        //void doCheckState();

    protected Q_SLOTS:
        void doAddPath();
        void doRemovePaths();
        void doReport();

        void displayLogMessage(QString _message);

        void checkEnableRemoved();

        // void doSelectDestination();
        // void doCompare();
        // void doGetResults();
        // void doCopy();
        // void receive_missing(QString _message);
        // void receive_new(QString _new);

    private:
        QGroupBox* boxPaths;
        QPushButton* buttonAddPath;
        QPushButton* buttonRemovePath;
        QListWidget* listPaths;
        QTreeView* treeReport;

        QPushButton* buttonSearch;
        QPushButton* buttonReport;
        QPushButton* buttonReset;

        QTextEdit* labelLog;
        
        HashDb db;
        FileDataModel fileDataModel;

        HashCoordinator hasher;
        QThread hashThread;

        Logger theLog;
        QThread logThread;
    private Q_SLOTS:
    };

#endif //DRIVE_DEDUPLICATOR_H__
