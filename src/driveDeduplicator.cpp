#include <driveDeduplicator.h>

#include <QtCore/QtCore>
#include <QtCore/QtGlobal>
#include <QtCore/QDir>
#include <QtCore/QString>

#include <QtGui/QFileDialog>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>
#include <QtGui/QTextEdit>
#include <QtGui/QVBoxLayout>

/* +--------------------------------------------------------------+
 * |+- Paths ---------------------+ +----------------------------+|
 * ||+----------+ +--------------+| | <report goes here>         ||
 * ||| Add Path | | Remove Path  || |                            ||
 * ||+----------+ +--------------+| |                            ||
 * ||+---------------------------+| |                            ||
 * |||<paths to search go here>  || |                            ||
 * |||                           || |                            ||
 * |||                           || |                            ||
 * |||                           || |                            ||
 * |||                           || |                            ||
 * |||                           || |                            ||
 * ||+---------------------------+| |                            ||
 * |+-----------------------------+ |                            ||
 * |+--------+ +--------+ +-------+ |                            ||
 * || Search | | Report | | Reset | |                            ||
 * |+--------+ +--------+ +-------+ |                            ||
 * |+-----------------------------+ |                            ||
 * ||<logs go here>               | |                            ||
 * ||                             | |                            ||
 * ||                             | |                            ||
 * |+-----------------------------+ +----------------------------+|
 * +--------------------------------------------------------------+
 *
 * 1. User selects (adds/removes) paths
 * 2. User clicks search
 * 2.1 Paths are searched for directories and files
 * 2.1.1 Each directory is recursed and searched per 2.1
 * 2.1.2 Each file is sent to be hashed
 * 2.2 Search ends, report button enabled
 * 3. User clicks report
 * 3.1 Duplicate files are listed together
 * 4. User selects files to remove
 * 4.1 User clicks "remove" to *delete* the files
 */

DriveDeduplicator* instance = NULL;

void messageCapture(QtMsgType _type, const char* _msg)
    {
    // qInstallMsgHandler(messageCapture);
    QString msg(_msg);
    if (instance != NULL)
        {
        instance->logMessage(_type, msg);
        }
    }

DriveDeduplicator::DriveDeduplicator(QWidget* _parent) : QWidget(_parent, 0),
        boxPaths(NULL), buttonAddPath(NULL), buttonRemovePath(NULL), listPaths(NULL), treeReport(NULL),
        buttonSearch(NULL), buttonReport(NULL), buttonReset(NULL),
        labelLog(NULL),
        fileDataModel(db, NULL)
    {
    instance = this;

    setWindowTitle("Drive Deduplicator");
    createLayout();

    /*
    connect(this, SIGNAL(startHashing(QString, bool)),
            &hasher, SLOT(startHashing(QString, bool)),
            Qt::QueuedConnection);
    connect(this, SIGNAL(resetHashing()),
            &hasher, SIGNAL(resetHashing()),
            Qt::QueuedConnection);

    connect(this, SIGNAL(getMissing()),
            &hasher, SIGNAL(getMissing()));
    connect(this, SIGNAL(getNew()),
            &hasher, SIGNAL(getNew()));
    connect(this, SIGNAL(copyMissing(QString, QString)),
            &hasher, SIGNAL(copyMissing(QString, QString)));
    connect(this, SIGNAL(resetDatabase()),
            &hasher, SIGNAL(resetDatabase()));

    connect(&hasher, SIGNAL(send_message(QString)),
            this, SLOT(displayLogMessage(QString)),
            Qt::QueuedConnection);
    connect(&hasher, SIGNAL(send_missing(QString)),
            this, SLOT(receive_missing(QString)),
            Qt::QueuedConnection);
    connect(&hasher, SIGNAL(send_new(QString)),
            this, SLOT(receive_new(QString)),
            Qt::QueuedConnection);

    connect(&hasher, SIGNAL(send_message(QString)),
            &theLog, SLOT(message(QString)),
            Qt::QueuedConnection);
    connect(&hasher, SIGNAL(send_missing(QString)),
            &theLog, SLOT(message(QString)),
            Qt::QueuedConnection);
    connect(&hasher, SIGNAL(send_new(QString)),
            &theLog, SLOT(message(QString)),
            Qt::QueuedConnection);

    hasher.moveToThread(&hashThread);
    hashThread.start();
    */

    connect(this, SIGNAL(send_message(QString)),
            &theLog, SLOT(message(QString)),
            Qt::QueuedConnection);
    connect(&theLog, SIGNAL(send_message(QString)),
            this, SLOT(displayLogMessage(QString)),
            Qt::QueuedConnection);

    theLog.setLogFile(QString(".drive-deduplicator.log"));

    theLog.moveToThread(&logThread);
    logThread.start();
    }

DriveDeduplicator::~DriveDeduplicator()
    {
    //hashThread.quit();
    //hashThread.wait();
    logThread.quit();
    logThread.wait();
    instance = NULL;
    }

void DriveDeduplicator::createLayout()
    {
    QHBoxLayout* masterLayout = new QHBoxLayout();
    if (masterLayout != NULL)
        {
        QVBoxLayout* mainLayout = new QVBoxLayout();
        if (mainLayout != NULL)
            {
            QHBoxLayout* controlLayout = new QHBoxLayout;
            if (controlLayout != NULL)
                {
                QVBoxLayout* pathLayout = new QVBoxLayout;
                if (pathLayout != NULL)
                    {
                    if (boxPaths == NULL)
                        {
                        boxPaths = new QGroupBox("Paths");
                        }
                    if (boxPaths != NULL)
                        {
                        QVBoxLayout* pathToolsLayout = new QVBoxLayout();
                        if (pathToolsLayout != NULL)
                            {
                            QHBoxLayout* pathCommandLayout = new QHBoxLayout();
                            if (pathCommandLayout != NULL)
                                {
                                if (buttonAddPath == NULL)
                                    {
                                    buttonAddPath = new QPushButton("Add Path");
                                    }
                                if (buttonAddPath != NULL)
                                    {
                                    connect(buttonAddPath, SIGNAL(clicked()),
                                            this, SLOT(doAddPath()));
                                    pathCommandLayout->addWidget(buttonAddPath);
                                    }

                                if (buttonRemovePath == NULL)
                                    {
                                    buttonRemovePath = new QPushButton("Remove Path");
                                    }
                                if (buttonRemovePath != NULL)
                                    {
                                    buttonRemovePath->setEnabled(false);
                                    connect(buttonRemovePath, SIGNAL(clicked()),
                                            this, SLOT(doRemovePaths()));
                                    pathCommandLayout->addWidget(buttonRemovePath);
                                    }

                                pathToolsLayout->addLayout(pathCommandLayout);
                                }

                            if (listPaths == NULL)
                                {
                                listPaths = new QListWidget();
                                }
                            if (listPaths != NULL)
                                {
                                pathToolsLayout->addWidget(listPaths);
                                connect(listPaths, SIGNAL(itemSelectionChanged()),
                                        this, SLOT(checkEnableRemoved()));
                                }

                            boxPaths->setLayout(pathToolsLayout);
                            }

                        pathLayout->addWidget(boxPaths);
                        }

                    controlLayout->addLayout(pathLayout);
                    }
                mainLayout->addLayout(controlLayout);
                }

            QHBoxLayout* commandLayout = new QHBoxLayout;
            if (commandLayout != NULL)
                {
                if (buttonSearch == NULL)
                    {
                    buttonSearch = new QPushButton("Search");
                    }
                if (buttonSearch != NULL)
                    {
                    commandLayout->addWidget(buttonSearch);
                    }

                if (buttonReport == NULL)
                    {
                    buttonReport = new QPushButton("Report");
                    }
                if (buttonReport != NULL)
                    {
                    commandLayout->addWidget(buttonReport);
                    }

                if (buttonReset == NULL)
                    {
                    buttonReset = new QPushButton("Reset");
                    }
                if (buttonReset != NULL)
                    {
                    commandLayout->addWidget(buttonReset);
                    }

                mainLayout->addLayout(commandLayout);
                }

            labelLog = new QTextEdit();
            if (labelLog != NULL)
                {
                labelLog->setReadOnly(true);
                mainLayout->addWidget(labelLog);
                }

            masterLayout->addLayout(mainLayout);
            }

        if (treeReport == NULL)
            {
            treeReport = new QTreeView();
            }
        if (treeReport != NULL)
            {
            masterLayout->addWidget(treeReport);
            }

        setLayout(masterLayout);
        }
    }

void DriveDeduplicator::logMessage(QtMsgType _type, QString _msg)
    {
    switch (_type)
        {
        case QtDebugMsg:    _msg.prepend("Debug   :");    break;
        case QtWarningMsg:    _msg.prepend("Warning :");    break;
        case QtCriticalMsg:    _msg.prepend("Critical:");    break;
        case QtFatalMsg:    _msg.prepend("Fatal   :");    break;
        default:            _msg.prepend("Unknown :");    break;
        };
    displayLogMessage(_msg);
    }
void DriveDeduplicator::displayLogMessage(QString _message)
    {
    if (labelLog != NULL && _message.isEmpty() == false)
        {
        labelLog->append(_message);
        }
    }

void DriveDeduplicator::doAddPath()
    {
    QString selectedDirectory = QFileDialog::getExistingDirectory(this, "Select Source directory", QDir::currentPath(), QFileDialog::ShowDirsOnly);
    if (listPaths != NULL)
        {
        listPaths->addItem(selectedDirectory);
        }
    //doCheckState();
    }
void DriveDeduplicator::doRemovePaths()
    {
    if (listPaths != NULL)
        {
        auto selectedItems = listPaths->selectedItems();
        for (auto item = selectedItems.begin(); item != selectedItems.end(); ++item)
            {
            auto removedItem = listPaths->takeItem(listPaths->row(*item));
            delete removedItem;
            }
        }
    }
void DriveDeduplicator::doReport()
    {
    //TODO
    }

void DriveDeduplicator::checkEnableRemoved()
    {
    bool removeEnabled = false;
    if (listPaths != NULL)
        {
        removeEnabled = (listPaths->selectedItems().count() > 0);
        }
    if (buttonRemovePath != NULL)
        {
        buttonRemovePath->setEnabled(removeEnabled);
        }
    }

/*
void DriveDeduplicator::doCheckState()
    {
    QString source;
    QString destination;
    if (labelSourcePath != NULL)
        {
        source = labelSourcePath->text();
        }
    if (labelDestinationPath != NULL)
        {
        destination = labelDestinationPath->text();
        }

    if (source.isEmpty() == false && destination.isEmpty() == false)
        {
        // enable compare button
        Q_EMIT resetDatabase();
        }
    else
        {
        // disable compare button
        }
    }
*/
/*
void DriveDeduplicator::doCompare()
    {
    QString source;
    QString destination;
    if (labelSourcePath != NULL)
        {
        source = labelSourcePath->text();
        }
    if (labelDestinationPath != NULL)
        {
        destination = labelDestinationPath->text();
        }

    Q_EMIT startHashing(source, true);
    Q_EMIT startHashing(destination, false);
    }
*/
/*
void DriveDeduplicator::doGetResults()
    {
    //Q_EMIT getMissing();
    //Q_EMIT getNew();
    }
*/
/*
void DriveDeduplicator::receive_missing(QString _missing)
    {
    if (editSource != NULL && _missing.isEmpty() == false)
        {
        editSource->append(_missing);
        }
    }
*/
/*
void DriveDeduplicator::receive_new(QString _new)
    {
    if (editDestination != NULL && _new.isEmpty() == false)
        {
        editDestination->append(_new);
        }
    }
*/
/*
void DriveDeduplicator::doCopy()
    {
    QString source;
    QString destination;
    if (labelSourcePath != NULL)
        {
        source = labelSourcePath->text();
        if (labelDestinationPath != NULL)
            {
            destination = labelDestinationPath->text();
            Q_EMIT copyMissing(source, destination);
            }
        }
    }
*/
