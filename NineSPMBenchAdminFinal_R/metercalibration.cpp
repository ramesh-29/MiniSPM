#include "metercalibration.h"
#include <QSerialPort>
#include <QFinalState>
#include <QDataStream>
#include <QDateTime>
#include <QTimer>
#include <QDebug>

MeterCalibration::MeterCalibration(QString portName, CALIBRATION calibrate, QObject *parent, QString tag)
    : MeterCommunication(portName, tag, parent)
{
    connect(this, &MeterCommunication::packetReceived, this, & MeterCalibration::packetReceived);
    calibrateWhat = calibrate;

    timeout = new QTimer(this);
    timeout->setInterval(15000);
    timeout->setSingleShot(true);
    tmrStopPhaseNeutral = new QTimer(this);
    tmrStopPhaseNeutral->setInterval(1000 * 4);
    tmrStopPhaseNeutral->setSingleShot(true);

    connect(timeout, SIGNAL(timeout()), this, SLOT(processCalibrationFail()));

    calibrateMachine = new PVStateMachine(this);
    authenticating = new PVState();
    phaseCalibration = new PVState();
    tmrPhaseToNeutral = new PVState();
    neutralCalibrating = new PVState();
    neutralCalibrating1 = new PVState();
    calibrationTimeout = new PVState();
    failed = new QFinalState();
    calibrated = new QFinalState();

    connect(calibrateMachine, SIGNAL(stateChanged()), this, SLOT(printState()));
    connect(calibrateMachine, SIGNAL(finished()), this, SLOT(processStateMachineStoped()));

    connect(authenticating, SIGNAL(entered()), this, SLOT(authenticate()));
    //    connect(calibrateMachine, SIGNAL(started()), timeout, SLOT(start()));
    if(calibrateWhat == CALIBRATE_PHASE)
        authenticating->addTransition(this, SIGNAL(authenticationPass()), phaseCalibration);
    else
        authenticating->addTransition(this, SIGNAL(authenticationPass()), neutralCalibrating);

    authenticating->addFailTransition(this, SIGNAL(authenticationFail()), failed);

    phaseCalibration->addTransition(this, SIGNAL(calibrationSuccess()), calibrated);
    phaseCalibration->addFailTransition(this, SIGNAL(calibrationFail()), failed, 18000, 2);
    connect(phaseCalibration, SIGNAL(entered()), this, SLOT(meterSendCalibrate()));
    connect(phaseCalibration, SIGNAL(entered()), timeout, SLOT(start()));

    connect(tmrPhaseToNeutral, SIGNAL(entered()), tmrStopPhaseNeutral, SLOT(start()));
    tmrPhaseToNeutral->addTransition(tmrStopPhaseNeutral, SIGNAL(timeout()), neutralCalibrating);

    neutralCalibrating->addTransition(this, SIGNAL(calibrationSuccess()), calibrated);
    neutralCalibrating->addFailTransition(this, SIGNAL(calibrationFail()), failed, 18000, 2);
    connect(neutralCalibrating, SIGNAL(entered()), this, SLOT(meterSendCalibrate()));
    connect(neutralCalibrating, SIGNAL(entered()), timeout, SLOT(start()));


    calibrationTimeout->addTransition(calibrationTimeout, SIGNAL(entered()), failed);

    calibrateMachine->addState(authenticating, "authenticating");
    calibrateMachine->addState(phaseCalibration, "phaseCalibration");
    calibrateMachine->addState(tmrPhaseToNeutral,"tmrPhaseToNeutral");
    calibrateMachine->addState(neutralCalibrating, "neutralCalibration");
    calibrateMachine->addState(neutralCalibrating1, "neutralCalibration1");
    calibrateMachine->addState(calibrationTimeout, "calibrationTimeout");
    calibrateMachine->addState(failed);
    calibrateMachine->addState(calibrated);
    calibrateMachine->setInitialState(authenticating);
}

//quint8 cycle, float voltage, float current, float maxCurrent, quint8 isPhase
void MeterCalibration::calibrate()
{
    qDebug() << tag << "Calibration Call";
    receivedPacketLength = 0;
    calibratePacketData.clear();
    testResult.resetValue();
    testResult.startTestTime = QDateTime::currentDateTime();
    calibrateMachine->start();
    qDebug() << tag << "Calibration Call End " << QDateTime::currentDateTime();
}

void MeterCalibration::meterSendCalibrate()
{
    QByteArray streamData, crcData;
    QDataStream stream (&streamData, QIODevice::WriteOnly);

    stream << (quint8)50; // cycle
    stream << (quint8)50; // cycle

    QVector<float> values;
    values.append(QString("60").toFloat());   // max current
    values.append(QString("240").toFloat());  // voltage
    values.append(QString("10").toFloat());       // current

    for (const float& value : values) {
        QByteArray temp_arr(reinterpret_cast<const char*>(&value), sizeof(value));
        std::reverse(temp_arr.begin(), temp_arr.end());
        for(auto i:temp_arr) {
            stream <<(quint8) i;
        }
    }

//    if(calibrateMachine->configuration().contains(phaseCalibration))
//        calibrateWhat = CALIBRATE_PHASE;
//    else
//        calibrateWhat = CALIBRATE_NEUTRAL;
     stream << (quint8) calibrateWhat;

    auto finalPacket = createPacket(MeterCommunication::MEM_SELECT_ONCHIP, streamData);

    sendPacket(finalPacket);

    qDebug() << tag << finalPacket.size() <<  finalPacket.toHex();
}


void MeterCalibration::printState()
{
    static QDateTime time;
    qDebug() << tag << "evt" << QDateTime::currentDateTime() << time.msecsTo(QDateTime::currentDateTime()) << "state" << calibrateMachine->lastState() << "->" << calibrateMachine->state() ;
    time = QDateTime::currentDateTime();
}

void MeterCalibration::packetReceived(QByteArray packet)
{
    receivedPacketLength += packet.length();
    calibratePacketData += packet;
    qDebug() << tag << calibratePacketData.toHex();
    //    calibrateMachine->configuration();

    qDebug() << tag << "MeterCalibration received:" << packet.size() <<  packet.toHex() << receivedPacketLength / 42;

    //ToDo: Act based on packet type
    if(calibrateWhat == CALIBRATE_BOTH){
        if(receivedPacketLength >= (84 * 4)){
            if(calibratePacketData.at(131) == 0 && calibratePacketData.at(299) == 0 )
                processCalibrationPass();
            else
                processCalibrationFail();
        }
    }
    else{
        if(receivedPacketLength >= (84 * 2)){
            if(calibratePacketData.at(131) == 0){
                processCalibrationPass();
                calibratePacketData.clear();
                receivedPacketLength = 0;
            }
            else{
                testResult.errorDetails = QString(" 131_BYTE_IS_NOT_0_BUT_%1").arg(calibratePacketData.at(131));
                qDebug() << tag << "R" << "calibratePacketData.at(131) is not zero" << calibratePacketData.at(131);
                processCalibrationFail();
            }
        }
    }
}

void MeterCalibration::processCalibrationPass()
{
    receivedPacketLength = 0;
    timeout->stop();
    emit calibrationSuccess();
}

void MeterCalibration::processCalibrationFail()
{
    timeout->stop();
    receivedPacketLength = 0;
    emit calibrationFail();
}

void MeterCalibration::processStateMachineStoped()
{
    testResult.lastState = calibrateMachine->lastState();
    testResult.endTestTime = QDateTime::currentDateTime();
    testResult.testDuration = testResult.startTestTime.secsTo(testResult.endTestTime);

    if(calibrateMachine->configuration().contains(calibrated)){
        testResult.isTestPass = true;
        qDebug() << tag << "Calibration Pass";
    }
    else{
        testResult.errorDetails += " Failed At" + calibrateMachine->lastState();
        qDebug() << tag << "Calibration Fail";
    }
    qDebug() << tag << "### duration" << testResult.testDuration;
    closeSerialPort();
//    openSerialPort();
    emit calibrationTestResult(&testResult);
}

