#include "powersourcemte.h"
#include <QSerialPort>
#include <QDebug>
#include <QTimer>
#include <QFinalState>
#include <QtMath>

#define CHANNEL1_TOLERANCE  (0.1)
#define CHANNEL2_TOLERANCE  (0.1)
#define CHANNEL3_TOLERANCE  (0.1)

PowerSourceMTE::PowerSourceMTE(QString portName, QString refferencePortName, float highCurrent, float lowCurrent, float startingCurrent, QObject *parent)
    : QObject{parent}
{
    serial = new QSerialPort(portName);
    serial->setBaudRate(19200);
    serial->setFlowControl(QSerialPort::NoFlowControl);

    serialRefference = new QSerialPort(refferencePortName);
    serialRefference->setBaudRate(4800);
    serialRefference->setFlowControl(QSerialPort::NoFlowControl);


    tag = "PSMTE";

    tmrForceRead = new QTimer(this);
    tmrForceRead->setInterval(200);
    tmrForceRead->start();
    connect(tmrForceRead, SIGNAL(timeout()), this, SLOT(readData()));

    tmrForseStop = new QTimer(this);
    tmrForseStop->setInterval(20000);
    tmrForseStop->setSingleShot(true);

    currentLow = lowCurrent;currentHigh = highCurrent;currentStarting = startingCurrent;

    connect(serialRefference, &QSerialPort::readyRead, this, &PowerSourceMTE::readData);
    connect(this, &PowerSourceMTE::currentReceived, this, &PowerSourceMTE::validateCurrent);

    if(serial->open(QSerialPort::ReadWrite)){
        commState = COMM_STATE_IDEAL;
        //        sendCommand("SKLI0,1;SET");
        setDefaultVoltageCurrent();
    }
    else{
        qDebug() << tag << "PorOpen Error " << portName << serial->errorString();
//                qFatal("PowerSourceMTE::portOpen Fail");
        commState = COMM_STATE_SERIAL_ERROR;
    }

    if(serialRefference->open(QSerialPort::ReadWrite)){
        sendRefferenceCommand("VER;DB1;YI0;U3,3,3;I7,5,8;MAN;SG;SET;");
    }
    else{
//                qFatal("ReferenceSourceMTE::portOpen Fail");
        commState = COMM_STATE_SERIAL_ERROR;
    }

    stabliseMachine = new PVStateMachine(this);
    auto stateStartPS = new PVState;
    stateRestartPS = new PVState();
    stateRequestPSStatus = new PVState();
    stateStablising = new PVState();
    statePowerBuildSuccess = new PVState();
    statePowerBuildFail = new PVState();
    statePowerBuildFailLastRead = new PVState();
    stateMachineFinish = new QFinalState();

    connect(stabliseMachine, SIGNAL(stateChanged()), this, SLOT(printState()));
    //    connect(stabliseMachine, SIGNAL(started()), tmrForseStop, SLOT(start()));

    connect(stateStartPS, SIGNAL(entered()), this, SLOT(enablePowerOutput()));
    stateStartPS->addTransition(stateStartPS, SIGNAL(entered()), stateRequestPSStatus);

    connect(stateRestartPS, SIGNAL(entered()), this, SLOT(restartPowerOutput()));
    stateRestartPS->addTransition(this, SIGNAL(powerSourceRestarted()), stateRequestPSStatus);
    stateRestartPS->addTransition(tmrForseStop, SIGNAL(timeout()), statePowerBuildFail);

    connect(stateRequestPSStatus, SIGNAL(entered()), this, SLOT(requestPowerStatus()));
    stateRequestPSStatus->addTransition(this, SIGNAL(validateCurrentFail()), stateRequestPSStatus);
    stateRequestPSStatus->addTransition(this, SIGNAL(validateCurrentPass()), statePowerBuildSuccess);
    stateRequestPSStatus->addTransition(this, SIGNAL(powerBuildFail()), statePowerBuildFail);
    stateRequestPSStatus->addTransition(this, SIGNAL(powerSourceError()), stateRestartPS);
    //    stateRequestPSStatus->addFailTransition(this, SIGNAL(neverEmited()), stateRequestPSStatus, 1000, 10);
    stateRequestPSStatus->addTransition(tmrForseStop, SIGNAL(timeout()), statePowerBuildFail);

    statePowerBuildSuccess->addTransition(statePowerBuildSuccess, SIGNAL(entered()), stateMachineFinish);

    statePowerBuildFail->addFailTransition(this, SIGNAL(neverTrigger()), statePowerBuildFailLastRead, 20000, 1);

    connect(statePowerBuildFailLastRead, SIGNAL(entered()), this, SLOT(requestPowerStatus()));
    statePowerBuildFailLastRead->addTransition(this, SIGNAL(powerSourceError()), stateMachineFinish);
    statePowerBuildFailLastRead->addTransition(this, SIGNAL(powerBuildFail()), stateMachineFinish);

    connect(stabliseMachine, SIGNAL(finished()), this, SLOT(processStateMachineStoped()));
    connect(stabliseMachine, SIGNAL(finished()), tmrForseStop, SLOT(stop()));
    stabliseMachine->addState(stateStartPS, "stateStartPS");
    stabliseMachine->addState(stateRestartPS, "stateRestartPS");
    stabliseMachine->addState(stateRequestPSStatus, "stateRequestPSStatus");
    stabliseMachine->addState(stateStablising, "stateStablising");
    stabliseMachine->addState(statePowerBuildSuccess, "statePowerBuildSuccess");
    stabliseMachine->addState(statePowerBuildFail, "statePowerBuildFail");
    stabliseMachine->addState(statePowerBuildFailLastRead, "statePowerBuildFailLastRead");
    stabliseMachine->addState(stateMachineFinish);
    stabliseMachine->setInitialState(stateStartPS);
}

int PowerSourceMTE::setChannelState(quint8 channelNumber, bool state,TEST_START testToStart)
{
    isTest = testToStart;
    if(testToStart == TEST_FUNCTIONAL){
        //        sendRefferenceCommand("VER;DB1;YI0;U1,1,3;I1,1,8;MAN;SG;SET;");
        sendRefferenceCommand("DB1;I1,1,8;SET;");
    }else if(testToStart == TEST_lOW_ERROR){
        //        sendRefferenceCommand("VER;DB1;YI0;U1,1,3;I1,1,7;MAN;SG;SET;");
        sendRefferenceCommand("I1,1,7;SET;");
    }else if(testToStart == TEST_HIGH_ERROR){
        //        sendRefferenceCommand("VER;DB0;YI0;U1,1,3;I1,1,8;MAN;SG;SET;");
        sendRefferenceCommand("DB0;I1,1,8;SET;");
    }else if(testToStart == TEST_STARTING_CURRENT){
        sendRefferenceCommand("DB1;I1,1,1;SET;");
    }

    QString channelStr, cmdStr;
    activeChannel = channelNumber;

    qDebug() << tag << "********** setChannelState" << QDateTime::currentDateTime() << channelNumber << "*********************";

    if(commState == COMM_STATE_SERIAL_ERROR){
        qDebug() << tag << "SerialPort Not Open";
        return -1;
    }
    if(channelNumber > 0x7 /*|| channelNumber < 1*/)
        return -2;

    if(channelNumber & POWER_CHANNEL_1)
        channelStr += "0,";
    if(channelNumber & POWER_CHANNEL_2)
        channelStr += "1,";
    if(channelNumber & POWER_CHANNEL_1)
        channelStr += "2,";

    channelStr.chop(1);

    if(state)
        cmdStr = "ON";// + channelStr;
    else
        cmdStr = "OFF";// + channelStr;

    //    cmriControl->cmriReset();
    stablisationCounter = 0;
    restartPSCounter = 0;

    sendCommand(cmdStr);
    startTime = QDateTime::currentDateTime();
    stabliseMachine->start();

    return 0;
}

void PowerSourceMTE::printState()
{
    qDebug() << tag << "state" << stabliseMachine->lastState() << "->" << stabliseMachine->state();
}

void PowerSourceMTE::processStabliseTimeout()
{
    serial->close();
    qDebug() << tag << " processStabliseTimeout porOpen " << serial->open(QIODevice::ReadWrite);
}

void PowerSourceMTE::readData()
{
    QString newData = QString::fromStdString( serialRefference->readAll().toStdString());
    receiverBuffer += newData;

    //    qDebug() << tag << "readData" << receiverBuffer;

    int pos = receiverBuffer.indexOf("\r");
    if(pos == -1)
        return;

    qDebug() << tag << "readData packet" << receiverBuffer;
    lastReply = receiverBuffer.mid(0, pos);
    receiverBuffer = receiverBuffer.mid(pos+1);

    if(stabliseMachine->configuration().contains(stateRequestPSStatus) || stabliseMachine->configuration().contains(statePowerBuildFailLastRead)){
        emit currentReceived(lastReply);
    }
}

bool PowerSourceMTE::setState(COMM_STATE newState)
{
    commState = newState;
    return true;
}

void PowerSourceMTE::validateCurrent(QString current)
{
    if(!lastReply.startsWith("EB"))
        return;

    auto seperateCurrent = current.trimmed().split(",");
//    float channel1 = seperateCurrent.at(1).toFloat();
//    float channel2 = seperateCurrent.at(2).toFloat();
    float channel3 = seperateCurrent.at(3).toFloat();
    //    float channelDiff1, channelDiff2;
    float channelDiff3;

    //    qDebug() << tag << "receivedValue" << current << channel1 << channel2 << channel3 << QDateTime::currentDateTime();
    qDebug() << tag << "receivedValue" << current << channel3 << QDateTime::currentDateTime();

    //    float theoryWattHourcCh1 = (powerSource[0].current * powerSource[0].voltage);

    //    float theoryWattHourcCh2 = (powerSource[1].current * powerSource[0].voltage);
    //    float theoryWattHourcCh3 = (powerSource[2].current * powerSource[0].voltage);
    //    float theoryWattHourcCh2 = (powerSource[1].current * powerSource[0].voltage * qCos(qDegreesToRadians(60.0)));
    if(isTest == TEST_NIC_SYNC){
        emit validateCurrentPass();
        return;
    }

    float theoryWattHourcCh3 = (powerSource[0].current * powerSource[0].voltage);
    if(isTest == TEST_lOW_ERROR || isTest == TEST_HIGH_ERROR){
        theoryWattHourcCh3 *= qCos(qDegreesToRadians(60.0));
    }


    //    channelDiff1 = std::abs(theoryWattHourcCh1 - channel1);
    //    channelDiff2 = std::abs(theoryWattHourcCh2 - channel2);
    channelDiff3 = std::abs(theoryWattHourcCh3 - channel3);

    //in %
    //    channel1P = channelDiff1 * 100 / theoryWattHourcCh1;
    //    channel2P = channelDiff2 * 100 / theoryWattHourcCh2;
    channel3P = channelDiff3 * 100 / theoryWattHourcCh3;

    //    qDebug() << tag << "diff %" << channel1P << channel2P << channel3P << activeChannel << " - " << channelDiff1 << channelDiff3 << channelDiff3 << activeChannel
    //             << (powerSource[0].current * powerSource[0].voltage) << (powerSource[1].current * powerSource[1].voltage) << (powerSource[2].current * powerSource[2].voltage);
    qDebug() << tag << "diff %" << channel3P << activeChannel << " - " << channelDiff3 << activeChannel
             << (powerSource[0].current * powerSource[0].voltage);

    if(stabliseMachine->configuration().contains(statePowerBuildFailLastRead)){
        qDebug() << "statePowerBuildFailLastRead";
        emit powerBuildFail();
        return;
    }
    //    if(((activeChannel & POWER_CHANNEL_1) && channel1P > 99.5) || ((activeChannel & POWER_CHANNEL_2) && channel2P > 99.5) || ((activeChannel & POWER_CHANNEL_3) && channel3P > 99.5)){
    //        qDebug() << tag << "powerStablisingFail count" << stablisationCounter << restartPSCounter;
    //        if(restartPSCounter < 3){
    //            if(stablisationCounter++ > 4)
    //                emit powerSourceError();
    //        }
    //        else
    //            emit powerBuildFail();
    //    }

    if(((activeChannel & POWER_CHANNEL_1) && channel3P > 99.5)){
        qDebug() << tag << "powerStablisingFail count" << stablisationCounter << restartPSCounter;
        if(restartPSCounter < 3){
            if(stablisationCounter++ > 4)
                emit powerSourceError();
        }
        else
            emit powerBuildFail();
    }

    if(isTest == TEST_STARTING_CURRENT){
        if((channel3P < 5 && activeChannel & POWER_CHANNEL_1)){
            qDebug() << tag << "*** duration starting current power stablished" << startTime.msecsTo(QDateTime::currentDateTime());
            emit validateCurrentPass();
            return;
        }
    }

    if((channel3P > CHANNEL1_TOLERANCE && activeChannel & POWER_CHANNEL_1)){
        QTimer::singleShot(1000, this, SIGNAL(validateCurrentFail()));
    }
    else{

        if(isTest == TEST_FUNCTIONAL){
            qDebug() << tag << "*** duration phase calibration power stablished" << startTime.msecsTo(QDateTime::currentDateTime());
        }if(isTest == TEST_CALIBRATION_NEUTRAL){
            qDebug() << tag << "*** duration neutral calibration power stablished" << startTime.msecsTo(QDateTime::currentDateTime());
        }else if(isTest == TEST_lOW_ERROR){
            qDebug() << tag << "*** duration low error power stablished" << startTime.msecsTo(QDateTime::currentDateTime());
        }else if(isTest == TEST_HIGH_ERROR){
            qDebug() << tag << "*** duration high error power stablished" << startTime.msecsTo(QDateTime::currentDateTime());
        }else if(isTest == TEST_STARTING_CURRENT){
            qDebug() << tag << "*** duration starting current power stablished" << startTime.msecsTo(QDateTime::currentDateTime());
        }else {
            qDebug() << tag << "*** duration nicsync power stablished" << startTime.msecsTo(QDateTime::currentDateTime());
        }
        //            qDebug() << tag << "***" << startTime.msecsTo(QDateTime::currentDateTime());
        emit validateCurrentPass();
    }

    //    if((channel1P > CHANNEL1_TOLERANCE && activeChannel & POWER_CHANNEL_1) || (channel2P > CHANNEL2_TOLERANCE && activeChannel & POWER_CHANNEL_2) || (channel3P > CHANNEL3_TOLERANCE && activeChannel & POWER_CHANNEL_3) ){
    //        QTimer::singleShot(1000, this, SIGNAL(validateCurrentFail()));
    //    }
    //    else{
    //        qDebug() << tag << "***" << startTime.msecsTo(QDateTime::currentDateTime());
    //        emit validateCurrentPass();
    //    }
}

int PowerSourceMTE::getSerial()
{
    qDebug() << tag << "bytesWrittend" << sendCommand("MEAS:CURR:AC?");
    return 0;
}

int PowerSourceMTE::setVoltage(float channel1Voltage, float channel2Voltage, float channel3Voltage)
{
    setVoltage(1, channel1Voltage);
    setVoltage(2, channel2Voltage);
    setVoltage(3, channel3Voltage);
    return 0;
}

int PowerSourceMTE::setCurrent(float channel1Current, float channel2Current, float channel3Current)
{
    setCurrent(1, channel1Current);
    setCurrent(2, channel2Current);
    setCurrent(3, channel3Current);
    return 0;
}

void PowerSourceMTE::setPhaseAngle(float channel1Angle, float channel2Angle, float channel3Angle)
{
    setPhaseAngle(1, channel1Angle);
    setPhaseAngle(2, channel2Angle);
    setPhaseAngle(3, channel3Angle);
}

int PowerSourceMTE::setVoltage(int channel, float voltage)
{
    quint8 outputChannel;

    if(channel < 1 || channel > 3)
        return -1;

    sendCommand(QString("U%1,%2").arg(channel).arg(voltage));
    sendCommand("SET");

    powerSource[channel-1].voltage = voltage;
    return 0;
}

int PowerSourceMTE::setCurrent(int channel, float current)
{
    quint8 outputChannel;

    if(channel < 1 || channel > 3)
        return -1;

    sendCommand(QString("I%1,%2").arg(channel).arg(current));
    sendCommand("SET");

    powerSource[channel-1].current = current;
    return 0;
}

void PowerSourceMTE::setPhaseAngle(int channel, float angle)
{
    int sourceChannel;
    if(channel < 1 || channel > 3)
        return;

    sendCommand(QString("W%1,%2").arg(channel).arg(angle));
    sendCommand("SET");
}

int PowerSourceMTE::setFrequency(float frequency)
{
    if(frequency < 40 || frequency > 70)
        return -1;

    //ToDo: Add frequency command
    //    sendCommand(QString("GEN:FREQ %1").arg(frequency));
    return 0;
}

void PowerSourceMTE::setDefaultVoltageCurrent()
{
    sendCommand("SKLI0,1;SET");

    //    setPhaseAngle(1, 0);
    //    setPhaseAngle(2, 60);
    //    setPhaseAngle(3, 60);

    //    setVoltage(1, 240);
    //    setVoltage(2, 240);
    //    setVoltage(3, 240);

    //    setCurrent(1, 10);
    //    setCurrent(2, 1);
    //    setCurrent(3, 30);

    if(isTest == TEST_FUNCTIONAL || isTest == TEST_CALIBRATION_NEUTRAL){
        setPhaseAngle(1,0);
        setVoltage(1,240);
        setCurrent(1,10);
    }else if(isTest == TEST_lOW_ERROR){
        setPhaseAngle(1,60);
//        setPhaseAngle(1,0);
        setVoltage(1,240);
        setCurrent(1,currentLow);
    }else if(isTest == TEST_HIGH_ERROR){
        setPhaseAngle(1,60);
//        setPhaseAngle(1,0);
        setVoltage(1,240);
        setCurrent(1,currentHigh);
    }else if(isTest == TEST_STARTING_CURRENT){
        setPhaseAngle(1,0);
        setVoltage(1,240);
        setCurrent(1,currentStarting);
    }else{
        setPhaseAngle(1,0);
        setVoltage(1,240);
        setCurrent(1,0);
    }

    //    setFrequency(50);
}

int PowerSourceMTE::enablePowerOutput()
{

    setDefaultVoltageCurrent();
    sendCommand("ON");
    return 0;
}

int PowerSourceMTE::disablePowerOutput()
{
    qDebug() << tag << "PS disablePowerOutput" ;
    sendCommand("OFF");
    return 0;
}

void PowerSourceMTE::restartEnablePowerOutput()
{
    startTime = QDateTime::currentDateTime();
    qDebug()<< startTime;

    setDefaultVoltageCurrent();
    sendCommand("ON");
    //    QTimer::singleShot(5000, this, SLOT(enablePowerOutput()));
    emit powerSourceRestarted();
}

void PowerSourceMTE::restartPowerOutput()
{
    stablisationCounter = 0;
    restartPSCounter++;
    qDebug() << tag << "##### restartingPowerSource";
    disablePowerOutput();
    QTimer::singleShot(6000, this, SLOT(restartEnablePowerOutput()));
}

int PowerSourceMTE::requestPowerStatus()
{
    sendRefferenceCommand("?3");
    sendRefferenceCommand("?1");
    sendRefferenceCommand("?2");
    setState(PowerSourceMTE::COMM_STATE_CURRENT);
    emit validateCurrentStart();
    return 0;
}

int PowerSourceMTE::checkVoltage()
{
    sendCommand("MEAS:VOLT:AC?");
    return 0;
}

int PowerSourceMTE::sendCommand(QString command)
{
    if(!command.endsWith("\r"))
        command += "\r";

    auto writeReturn = serial->write(command.toStdString().c_str());

    qDebug() << tag << "PS sendCommand" << command << writeReturn << serial->bytesToWrite() << QDateTime::currentDateTime();

    return writeReturn;
}

int PowerSourceMTE::sendRefferenceCommand(QString command)
{
    if(!command.endsWith("\r"))
        command += "\r";
    QString tag1 = "RFMTE";

    auto writeReturn = serialRefference->write(command.toStdString().c_str());

    qDebug() << tag1 << "PS sendCommand Refference " << command << writeReturn << serialRefference->bytesToWrite() << QDateTime::currentDateTime();

    return writeReturn;
}

void PowerSourceMTE::processStateMachineStoped()
{
    quint8 result = 0;
    if(channel1P <= CHANNEL1_TOLERANCE)
        result |= (1<<0);
    if(channel2P <= CHANNEL2_TOLERANCE)
        result |= (1<<1);
    if(channel3P <= CHANNEL3_TOLERANCE)
        result |= (1<<2);

    qDebug() << tag << "processStateMachineStoped" << stabliseMachine->lastState();
    emit allChannelReady();
    emit channelReady(result);
}
