#ifndef FUNCTIONTEST_H
#define FUNCTIONTEST_H

#include <QObject>
#include "metercommunication.h"
#include "meterinfo.h"
#include "pvstatemachine.h"
#include "pvstate.h"
#include "meterkeys.h"

class FunctionTest : public MeterCommunication
{
    Q_OBJECT
public:
    explicit FunctionTest(QString port, const EnCustomer customer,QString meterNo, QObject *parent = nullptr);

public slots:
    void startTest();


    void fetchSerial();
    void fetchPCBNumber();
    void fetchRTCDrift();
    void setRTC();
    void startFunctionalTest();
    void fetchFWVersion();
    void fetchFWVersionInternal();
    void fetchFWVersionNameplate();
    void fetchFirmwareVersionNIC();

    void processSerial(QByteArray packet);
    bool processPCBNumber(QByteArray packet);
    bool processRTCDrift(QByteArray packet);
    bool processSetRTC(QByteArray packet);
    bool processFunctionTest(QByteArray packet);
    bool processFirmwareVersion(QByteArray packet);
    bool processFirmwareVersionNameplate(QByteArray packet);
    bool processFirmwareVersionInternal(QByteArray packet);
    bool processFirmwareVersionNIC(QByteArray packet);

    void processStateMachineStoped();

protected slots:
    void packetReceived(QByteArray packet);
    void printState();

signals:
    void fetchSerialPass();
    void fetchSerialFail();
    void fetchPCBNumberPass();
    void fetchPCBNumberFail();
    void functionTestResult(FunctionTestResult* result);
    void fetchRTCDriftPass();
    void fetchRTCDriftFail();
    void fetchRTCUpdateRTC();
    void setRTCPass();
    void setRTCFail();
    void functionTestPass();
    void functionTestFail();
    void fetchFirmwareVersionPass();
    void fetchFirmwareVersionFail();
    void fetchFirmwareVersionInternalPass();
    void fetchFirmwareVersionInternalFail();
    void fetchFirmwareVersionNameplatePass();
    void fetchFirmwareVersionNameplateFail();
    void fetchFirmwareVersionNICPass();
    void fetchFirmwareVersionNICFail();

private:
    FunctionTestResult testResult;
    QDateTime time;
    MeterKeys* meter_keys;
    PVStateMachine* stateMachine;
    PVState *authenticating;
    PVState *authenticated;
    PVState *stFetchSerial;
    PVState *fetchingPCBNumber;
    PVState *doingFunctionTest;
    PVState *fetchingFirmwareVer;
    PVState *fetchingFirmwareVerInternal;
    PVState *fetchingFirmwareVerNameplate;
    PVState *fetchingFirmwareVerNIC;
    PVState *fetchingRTCDrift;
    PVState *updateRTC;
    QFinalState *failed;
    QFinalState *functionTestFinish;

    QStringList functionTestID = {".coverClosed", ".pushButton", ".magnetStat1", ".magnetStat2", ".phaseCurrentOFF", ".neutralCurrentOFF", ".phaseCurrentON", ".neutralCurrentON = true"};
    int retryCounter;
};

#endif // FUNCTIONTEST_H
