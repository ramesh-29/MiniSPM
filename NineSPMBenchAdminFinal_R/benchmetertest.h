#ifndef BENCHMETERTEST_H
#define BENCHMETERTEST_H

#include <QObject>
#include "metererrortest.h"
#include "meterinfo.h"
#include "metercalibration.h"
#include "functiontest.h"
#include "nicsynctest.h"
#include "powersourcemte.h"
#include "meterkeys.h"

enum TEST_RESULT{
    TEST_STATUS_IDEAL,
    TEST_STATUS_NOT_APPLICABLE,
    TEST_STATUS_RUNNING,
    TEST_STATUS_PASS,
    TEST_STATUS_FAIL
};

typedef QList<TEST_RESULT> TestResults;

class BenchMeterTest: public MeterInfo
{
    Q_OBJECT
public:
    explicit BenchMeterTest(QString port, QString meterNo,bool setSerialNo,EnCustomer customer, float highCurrent, float lowCurrent, float startingCurrent, QString networkModule,  QObject *parent = nullptr);


public slots:
    void startTest(TEST_START testToStart);
    void fetchCalibrationPhaseTestResult(CalibrationResult* result);
    void fetchCalibrationNeutralTestResult(CalibrationResult* result);
    void fetchLowCurrentTestResult(ErrorTestResult *result);
    void fetchHighCurrentTestResult(ErrorTestResult* result);
    void fetchStartingCurrentTestResult(ErrorTestResult* result);
    void fetchFunctionTestResult(FunctionTestResult* result);
    void fetchNicSycnTestResult(NicSyncTestResult* result);

signals:
    void testFinish(TEST_RESULT testStatus,QString errorDetail);
    void startMeterTest();


private:
    QString tag;
    bool isPowerStablised;
    PowerSourceMTE* powerSource;
    TEST_RESULT calibrationStatus;
    TEST_RESULT lowCurrentErrorTestStatus;
    TEST_RESULT highCurrentErrorTestStatus;
    TEST_RESULT functionalTestStatus;
    TEST_RESULT nicSyncTestStatus;
    TEST_RESULT startingCurrentTestStatus;
    QDateTime startTestTime;
    QDateTime time;
    FunctionTest* functionalTest;
    MeterErrorTest* lowCurrentErrorTest;
    MeterErrorTest* highCurrentErrorTest;
    MeterErrorTest* startingCurrentTest;
    MeterCalibration* calibratePhase;
    MeterCalibration* calibrateNeutral;
    NicSyncTest* creepTest;
    bool setSerial;


};

#endif // BENCHMETERTEST_H
