#ifndef MTEPOWERSOURCE_H
#define MTEPOWERSOURCE_H

#include <QTimer>
#include <QObject>
#include <QVector>
#include <QDateTime>
#include "pvstatemachine.h"
#include "pvstate.h"
#include "datamanager.h"
//#include "cmricontrol.h"
//#include "powersource.h"

class QSerialPort;
class PowerChannel{
public:
    float voltage;
    float current;
    bool enable;
};

enum TEST_START{
    TEST_FUNCTIONAL,
    TEST_CALIBRATION_PHASE,
    TEST_CALIBRATION_NEUTRAL,
    TEST_lOW_ERROR,
    TEST_HIGH_ERROR,
    TEST_STARTING_CURRENT,
    TEST_NIC_SYNC,
    TEST_FINISH
};

class PowerSourceMTE : public QObject
{
    Q_OBJECT
public:
    explicit PowerSourceMTE(QString portName, QString refferencePortName,float highCurrent,float lowCurrent,float startingCurrent,QObject *parent = nullptr);

    enum COMM_STATE
    {
        COMM_STATE_IDEAL,
        COMM_STATE_VOLTAGE,
        COMM_STATE_CURRENT,
        COMM_STATE_TIMEOUT,
        COMM_STATE_SERIAL_ERROR
    };

    enum POWER_CHANNEL
    {
        POWER_CHANNEL_1 = (1<<0),
        POWER_CHANNEL_2 = (1<<1),
        POWER_CHANNEL_3 = (1<<2)
    };

public slots:
    int setChannelState(quint8 channelNumber, bool state, TEST_START testToStart);
    void printState();

public slots:
    void processStabliseTimeout();

protected:
    bool setState(COMM_STATE newState);
    void validateCurrent(QString current);

public slots: //suppose to be private
    void readData();
    int getSerial();
    int setVoltage(int channel, float voltage);
    int setCurrent(int channel, float current);
    void setPhaseAngle(int channel, float angle);
    int setFrequency(float frequency);
    int setVoltage(float channel1Voltage, float channel2Voltage, float channel3Voltage);
    int setCurrent(float channel1Current, float channel2Current, float channel3Current);
    void setPhaseAngle(float channel1Angle, float channel2Angle, float channel3Angle);
    void setDefaultVoltageCurrent();

    int enablePowerOutput();
    int disablePowerOutput();
    void restartEnablePowerOutput();
    void restartPowerOutput();
    int requestPowerStatus();
    int checkVoltage();

    int sendCommand(QString command);
    int sendRefferenceCommand(QString command);
    void processStateMachineStoped();

signals:
    void validateCurrentStart();
    void validateCurrentPass();
    void validateCurrentFail();
    void allChannelReady();
    void channelReady(quint8 channel);

    void powerSourceRestarted();
    void powerSourceError();
    void powerBuildFail();
    void neverTrigger();


    void currentReceived(QString current);
    void voltageReceived(QString voltage);
    void neverEmited();

private:
    QSerialPort* serial;
    QSerialPort* serialRefference;
    QString receiverBuffer;
    QString tag;
    COMM_STATE commState;
    PowerChannel powerSource[3];

    QTimer* tmrForceRead;
    QTimer* tmrForseStop;

    QDateTime startTime;
    TEST_START isTest;

    PVStateMachine* stabliseMachine;
    PVState *stateRestartPS;
    PVState *stateRequestPSStatus;
    PVState *stateStablising;
    PVState *statePowerBuildSuccess;
    PVState *statePowerBuildFail;
    PVState *statePowerBuildFailLastRead;
    QFinalState *stateMachineFinish;

    QString lastReply;
    quint8 activeChannel;

    int stablisationCounter;
    int restartPSCounter;

    float currentHigh,currentLow,currentStarting;
    float channel1P;
    float channel2P;
    float channel3P;
};
#endif // MTEPOWERSOURCE_H
