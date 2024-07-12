#ifndef SPMBENCHFULLTEST_H
#define SPMBENCHFULLTEST_H

#include "powersourcemte.h"
#include <QObject>
#include "benchmetertest.h"
#include <QColor>
class SPMBenchFullTest: public QObject
{
    Q_OBJECT
public:
    explicit SPMBenchFullTest(QObject *parent = nullptr);


    Q_INVOKABLE QColor getMeterColor(int meterIndex) const;
    Q_INVOKABLE QString getMeterValue(int meterIndex);
    Q_INVOKABLE QString getFailStage(int meterIndex);
    Q_INVOKABLE int getmeterSerialNo(int meterIndex);
public slots:
    void meter1Result(TEST_RESULT testStatus, QString errorDetails);
    void meter2Result(TEST_RESULT testStatus, QString errorDetails);
    void meter3Result(TEST_RESULT testStatus, QString errorDetails);
    void meter4Result(TEST_RESULT testStatus, QString errorDetails);
    void meter5Result(TEST_RESULT testStatus, QString errorDetails);
    void meter6Result(TEST_RESULT testStatus, QString errorDetails);
    void meter7Result(TEST_RESULT testStatus, QString errorDetails);
    void meter8Result(TEST_RESULT testStatus, QString errorDetails);
    void meter9Result(TEST_RESULT testStatus, QString errorDetails);
    void setAllTestResult(TEST_RESULT result);
    void processTestResult();
    void startTest();
    void powerStablised(quint8 channels);
    void setupPowerSource();
    void finishTest();



signals:
    void meterColorChanged(int index);

private:
    int x=0;
    QString tag;
    bool setSerialNo = false;
    bool isPowerStablised;
    PowerSourceMTE* powerSource;
    BenchMeterTest* benchMeter[9];
    TEST_RESULT meterTestStatus[9];
    TEST_START testToStart;
    QDateTime startTestTime;
    QDateTime totalTestTime;
    bool autoTestOn = false;
    QString errorDetail[9];
    QString failStage[9];
    QString currentStage;
};

#endif // SPMBENCHFULLTEST_H
