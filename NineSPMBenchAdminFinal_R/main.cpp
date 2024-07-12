#include "widget.h"
#include <QStandardPaths>
#include <QSettings>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "spmbenchfulltest.h"
#include "configuration.h"



QFile logFile;

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();

    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "Debug: %s \n", localMsg.constData());
        logFile.write("Debug");
        break;
    case QtInfoMsg:
        fprintf(stderr, "Info: %s \n", localMsg.constData());
        logFile.write("Info: ");
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s \n", localMsg.constData());
        logFile.write("Warning: ");
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s \n", localMsg.constData());
        logFile.write("Critical: ");
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s \n", localMsg.constData());
        logFile.write("Fatal: ");
        logFile.write(localMsg);
        logFile.write("\r\n");
        logFile.flush();
        abort();
    }

    logFile.write(localMsg);
    logFile.write("\r\n");
}

int main(int argc, char *argv[])
{

    qInstallMessageHandler(myMessageOutput); // Install the handler
    QApplication a(argc, argv);
    a.setOrganizationName("Kimbal");
    a.setApplicationName("SPMBench");

    QQmlApplicationEngine engine;
    qmlRegisterType<SPMBenchFullTest>("SPMAdmin", 1, 0, "SPMBenchFullTest");

    const QUrl url(QStringLiteral("qrc:/main.qml"));
    // Register SPMBenchFullTest class with QML engine
//    qmlRegisterType<SPMBenchFullTest>("SPMAdmin", 1, 0, "SPMBenchFullTest");

        Configuration config;
        config.show();

        QObject::connect(&config, &Configuration::submitClicked, [&]() {
              // Close the widget
              config.close();
              engine.load(url);
          });


    QString logPath = QStandardPaths::standardLocations(QStandardPaths::DataLocation).at(0);
    QDir().mkpath(logPath);
    logPath += "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    logFile.setFileName(logPath + ".log");
    logFile.open(QIODevice::WriteOnly | QIODevice::Text);

    qDebug() << logPath;
//    Widget w;
//    w.show();
    return a.exec();
}
