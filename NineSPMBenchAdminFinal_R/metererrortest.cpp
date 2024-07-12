#include "metererrortest.h"
#include <QFinalState>
#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QtMath>

MeterErrorTest::MeterErrorTest(float voltage, float current, float tolerance, float testPowerFactor, QString serialPort, QObject *parent, QString tag)
    : MeterCommunication{serialPort, tag, parent}
{
    connect(this, &MeterCommunication::packetReceived, this, & MeterErrorTest::packetReceived);
    this->tolerance = tolerance;
    testVoltage = voltage;
    testCurrent = current;
    this->testPowerFactor = testPowerFactor;

    tmrStopEnergyAccumulation = new QTimer(this);
    tmrStopEnergyAccumulation->setInterval(1000 * 12);
    tmrStopEnergyAccumulation->setSingleShot(true);

    stateMachine = new PVStateMachine(this);
    authenticating = new PVState();
    startEnergyAccu = new PVState();
    energyAccumulating = new PVState();
    stopEnergyAccu = new PVState();
    energyCalculating = new PVState();
    timeOut = new PVState();
    failed = new QFinalState();
    errorTestFinish = new QFinalState();

    connect(stateMachine, SIGNAL(stateChanged()), this, SLOT(printState()));
    connect(stateMachine, SIGNAL(finished()), this, SLOT(processStateMachineStoped()));

    connect(authenticating, SIGNAL(entered()), this, SLOT(authenticate()));
    authenticating->addTransition(this, SIGNAL(authenticationPass()), startEnergyAccu);
    authenticating->addFailTransition(this, SIGNAL(authenticationFail()), failed, 2000, 5);

    connect(startEnergyAccu, SIGNAL(entered()), this, SLOT(startEnergyAccumulation()));
    startEnergyAccu->addTransition(this, SIGNAL(startEnergyAccumulationSuccess()), energyAccumulating);
    startEnergyAccu->addFailTransition(this, SIGNAL(startEnergyAccumulationFail()), failed);

    connect(energyAccumulating, SIGNAL(entered()), tmrStopEnergyAccumulation, SLOT(start()));
    energyAccumulating->addTransition(tmrStopEnergyAccumulation, SIGNAL(timeout()), stopEnergyAccu);
    //No fail transition as it's only timer and will never fail

    connect(stopEnergyAccu, SIGNAL(entered()), this, SLOT(stopEnergyAccumulation()));
    stopEnergyAccu->addTransition(this, SIGNAL(stopEnergyAccumulationSuccess()), energyCalculating);
    stopEnergyAccu->addFailTransition(this, SIGNAL(stopEnergyAccumulationFail()), failed);

    energyCalculating->addTransition(this, SIGNAL(energyCalculatingPass()), errorTestFinish);
    energyCalculating->addTransition(this, SIGNAL(energyCalculatingFail()), failed);

    timeOut->addTransition(timeOut, SIGNAL(entered()), failed);

    stateMachine->addState(authenticating, "Authenticating");
    stateMachine->addState(startEnergyAccu, "startEnergyAccu");
    stateMachine->addState(energyAccumulating, "energyAccumulating");
    stateMachine->addState(stopEnergyAccu, "stopEnergyAccu");
    stateMachine->addState(energyCalculating, "energyCalculating");
    stateMachine->addState(timeOut, "timeOut");
    stateMachine->addState(failed);
    stateMachine->addState(errorTestFinish);

    stateMachine->setInitialState(authenticating);
}

void MeterErrorTest::startErrorTest()
{
    stopEnergyAccumulationRetry = 5;
    testResult.resetValue();
    testResult.startTestTime = QDateTime::currentDateTime();
    openSerialPort();
    stateMachine->start();
}

void MeterErrorTest::startEnergyAccumulation()
{
    QByteArray streamData;
    QDataStream stream (&streamData, QIODevice::WriteOnly);

    stream << (quint8) 0x05;//data type
    stream << (quint8) 0x00;//read
    stream << (quint8) 0x05;//memory
    stream << (quint32) 0x16;//address;
    stream << (quint8) 0x01;//size of dummy data
    stream << (quint8) 0x0A;

    auto finalPacket = createPacket(streamData, 3);
    sendPacket(finalPacket);

    qDebug() << tag << "sending startEnergyAccu" << finalPacket.toHex();
}

void MeterErrorTest::stopEnergyAccumulation()
{
    QByteArray streamData, crcData;
    QDataStream stream (&streamData, QIODevice::WriteOnly);

    stream << (quint8) 0x05;//data type
    stream << (quint8) 0x01;//read
    stream << (quint8) 0x05;//memory
    stream << (quint32) 0x17;//address;
    stream << (quint8) 0x20;//size of dummy data

    for(int i=0; i<0x20; i++)
        stream << (quint8)0x00;

    auto finalPacket = createPacket(streamData, 3);
    sendPacket(finalPacket);
}

void MeterErrorTest::printState()
{
    qDebug() << tag << "evt" << QDateTime::currentDateTime() << time.msecsTo(QDateTime::currentDateTime()) << "state" << stateMachine->lastState() << "->" << stateMachine->state() ;
    time = QDateTime::currentDateTime();
}

void MeterErrorTest::packetReceived(QByteArray packet)
{
    qDebug() << tag << "packetReceived" << packet.toHex() << stateMachine->configuration() << startEnergyAccu << energyAccumulating << stopEnergyAccu << energyCalculating;
    if(stateMachine->configuration().contains(startEnergyAccu))
        processStartEnergyAccumulation(packet);

    else if(stateMachine->configuration().contains(stopEnergyAccu))
        processStopEnergyAccumulation(packet);
}

void MeterErrorTest::processStateMachineStoped()
{
    testResult.lastState = stateMachine->lastState();
    testResult.endTestTime = QDateTime::currentDateTime();
    testResult.testDuration = testResult.startTestTime.secsTo(testResult.endTestTime);
    qDebug() << tag << testResult.getResult();
    if(stateMachine->configuration().contains(errorTestFinish)){
        testResult.isTestPass = true;
    }
    else{
        testResult.errorDetails += " Failed At" + stateMachine->lastState();
        qDebug() << tag << "R" << stateMachine->lastState();
    }
    qDebug() << tag << "### duration" << testResult.testDuration;
    closeSerialPort();
    emit errorTestResult(&testResult);
}

void MeterErrorTest::processStartEnergyAccumulation(QByteArray &packet)
{
    QDataStream stream (&packet, QIODevice::ReadOnly);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    stream.setByteOrder(QDataStream::LittleEndian);

    float test;
    stream.skipRawData(6);
    stream >> test;

    memcpy(&startingCumlativeEnergy, packet.constData() + 6, sizeof(startingCumlativeEnergy));
    testResult.startingCumlativeEnergy = startingCumlativeEnergy;

    emit startEnergyAccumulationSuccess();
    qDebug() << tag << "startingCumlativeEnergy:" <<startingCumlativeEnergy << test;
}

void MeterErrorTest::processStopEnergyAccumulation(QByteArray &packet)
{
    emit stopEnergyAccumulationSuccess();

    QDataStream stream (&packet, QIODevice::ReadOnly);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream.skipRawData(6);
    stream >> testResult.phaseWattHour;
    stream >> testResult.neutralWattHour;
    stream >> testResult.timeMS;
    stream >> testResult.cumulativeEnergy;
    stream >> testResult.phaseEnergyVAH;
    stream >> testResult.neutralEnergyVAH;
    stream >> testResult.phaseReactiveEnergy;
    stream >> testResult.neutralReactiveEnergy;

    qDebug() << tag << " *** Phase Energy: " << testResult.phaseWattHour << "Neutral Energy:" << testResult.neutralWattHour
             << "Time (ms):" << testResult.timeMS <<"Cumulative Energy:" << testResult.cumulativeEnergy - startingCumlativeEnergy;

    testResult.powerWH = testVoltage * testCurrent * qCos(qDegreesToRadians(testPowerFactor));// 60D
    testResult.theoryEnergy = (((float)testResult.timeMS) * testResult.powerWH) / (METER_ERROR_TEST_HOUR * METER_ERROR_TEST_MINUTE * 1000);
    float phaseDifference = abs(testResult.theoryEnergy - testResult.phaseWattHour);
    float neutralDifference = abs(testResult.theoryEnergy - testResult.neutralWattHour);
    float toleranceParcent = testResult.theoryEnergy * tolerance / 100;

    qDebug() << tag << "(theoryEnergy :" << testResult.theoryEnergy << ") (phaseDifference%: " << (phaseDifference*100/testResult.theoryEnergy) <<
                "), (neutralDifference%: " << (neutralDifference*100/testResult.theoryEnergy) << "), (Tolerance: " << toleranceParcent <<")";

    if(testResult.phaseWattHour == 0 || testResult.neutralWattHour == 0){ //StartingCurrentUserful, Must should accumulates some energey else calculation no use,
        qDebug() << tag << "phaseEnergy or neutralEnergy is zero" << testResult.phaseWattHour << testResult.neutralWattHour;
        if(testResult.phaseWattHour == 0)
            testResult.errorDetails += "PHASE_ENERGY_0";

        if(testResult.neutralWattHour == 0)
            testResult.errorDetails += "NEUTRAL_ENERGY_0";

        emit energyCalculatingFail();
        return;
    }

    if(phaseDifference <= toleranceParcent && neutralDifference <= toleranceParcent){
        qDebug() << tag << "Pass";
        emit energyCalculatingPass();
    }
    else{
        qDebug() << tag << "R" << "Out of Tolerance Fail";
        if(phaseDifference > toleranceParcent)
            testResult.errorDetails += "PHASE_OUT_TOLERANCE "+ QString::number(phaseDifference*100/testResult.theoryEnergy);

        if(neutralDifference > toleranceParcent)
            testResult.errorDetails += "NEUTRAL_OUT_TOLERANCE "+ QString::number(neutralDifference*100/testResult.theoryEnergy);
        emit energyCalculatingFail();
    }
}

float MeterErrorTest::round(float value, int roundFactor)
{
    int tmpValue = value * roundFactor;
    value = tmpValue / roundFactor;

    return value;
}
