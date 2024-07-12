#include "functiontest.h"
#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QTimer>
#include <QState>
#include "meterkeys.h"

#define MIN_VOLTAGE_FT  (160)
#define MIN_CURRENT_FT  (0.8)
#define MAX_RTC_DRIFT   (5)

//#define FW_VER  ("NCMH0.01.08")

FunctionTest::FunctionTest(QString port, const EnCustomer customer, QString meterNo, QObject *parent)
    : MeterCommunication{port, "FT"+ meterNo, parent}
{
    connect(this, &MeterCommunication::packetReceived, this, & FunctionTest::packetReceived);

    meter_keys = new MeterKeys();
    meter_keys->SetCustomer(customer);

    stateMachine = new PVStateMachine(this);

    authenticating = new PVState();
    stFetchSerial = new PVState();
    fetchingFirmwareVer = new PVState();
    fetchingFirmwareVerInternal = new PVState();
    fetchingFirmwareVerNameplate = new PVState();
    fetchingFirmwareVerNIC = new PVState();
    fetchingPCBNumber = new PVState();
    fetchingRTCDrift = new PVState();
    updateRTC = new PVState();
    doingFunctionTest = new PVState();
    failed = new QFinalState();
    functionTestFinish = new QFinalState();

    connect(stateMachine, SIGNAL(stateChanged()), this, SLOT(printState()));
    connect(stateMachine, SIGNAL(finished()), this, SLOT(processStateMachineStoped()));

    connect(authenticating, SIGNAL(entered()), this, SLOT(authenticate()));
    authenticating->addTransition(this, SIGNAL(authenticationPass()), fetchingFirmwareVerInternal);
    authenticating->addFailTransition(this, SIGNAL(authenticationFail()), failed, 2000, 5);

    connect(stFetchSerial, SIGNAL(entered()), this, SLOT(fetchSerial()));
    stFetchSerial->addTransition(this, SIGNAL(fetchSerialPass()), fetchingFirmwareVer);
    stFetchSerial->addFailTransition(this, SIGNAL(fetchSerialFail()), failed);

    connect(fetchingFirmwareVer, SIGNAL(entered()), this, SLOT(fetchFWVersion()));
    fetchingFirmwareVer->addTransition(this, SIGNAL(fetchFirmwareVersionPass()), fetchingFirmwareVerInternal);
    fetchingFirmwareVer->addFailTransition(this, SIGNAL(fetchFirmwareVersionFail()), failed, 2000, 2);

    connect(fetchingFirmwareVerInternal, SIGNAL(entered()), this, SLOT(fetchFWVersionInternal()));
    fetchingFirmwareVerInternal->addTransition(this, SIGNAL(fetchFirmwareVersionInternalPass()), fetchingPCBNumber);
    fetchingFirmwareVerInternal->addFailTransition(this, SIGNAL(fetchFirmwareVersionFail()), failed, 2000, 5);

    connect(doingFunctionTest, SIGNAL(entered()), this, SLOT(startFunctionalTest()));
    doingFunctionTest->addTransition(this, SIGNAL(functionTestPass()), fetchingPCBNumber);
    doingFunctionTest->addFailTransition(this, SIGNAL(functionTestFail()), failed, 6000, 2);

    connect(fetchingFirmwareVerNIC, SIGNAL(entered()), this, SLOT(fetchFirmwareVersionNIC()));
    fetchingFirmwareVerNIC->addTransition(this, SIGNAL(fetchFirmwareVersionNICPass()), fetchingRTCDrift);
    fetchingFirmwareVerNIC->addFailTransition(this, SIGNAL(fetchFirmwareVersionNICFail()), failed, 2000, 5);

    connect(fetchingPCBNumber, SIGNAL(entered()), this, SLOT(fetchPCBNumber()));
    fetchingPCBNumber->addTransition(this, SIGNAL(fetchPCBNumberPass()), fetchingRTCDrift);
    fetchingPCBNumber->addFailTransition(this, SIGNAL(fetchPCBNumberFail()), failed);

    connect(fetchingRTCDrift, SIGNAL(entered()), this, SLOT(fetchRTCDrift()));
    fetchingRTCDrift->addTransition(this, SIGNAL(fetchRTCDriftPass()), fetchingFirmwareVerNameplate);
    fetchingRTCDrift->addTransition(this, SIGNAL(fetchRTCUpdateRTC()), updateRTC);
    fetchingRTCDrift->addFailTransition(this, SIGNAL(fetchRTCDriftFail()), failed);

    connect(updateRTC, SIGNAL(entered()), this, SLOT(setRTC()));
    updateRTC->addTransition(this, SIGNAL(setRTCPass()), fetchingFirmwareVerNameplate);
    updateRTC->addFailTransition(this, SIGNAL(setRTCFail()), failed);

    connect(fetchingFirmwareVerNameplate, SIGNAL(entered()), this, SLOT(fetchFWVersionNameplate()));
    fetchingFirmwareVerNameplate->addTransition(this, SIGNAL(fetchFirmwareVersionNameplatePass()), functionTestFinish);
    fetchingFirmwareVerNameplate->addFailTransition(this, SIGNAL(fetchFirmwareVersionNameplateFail()), failed, 2000, 2);


    stateMachine->addState(authenticating, "Authenticating");
    stateMachine->addState(stFetchSerial, "stFetchSerial");
    stateMachine->addState(fetchingFirmwareVer, "fetchingFirmwareVersion");
    stateMachine->addState(fetchingFirmwareVerNameplate, "fetchingFirmwareVerNameplate");
    stateMachine->addState(fetchingFirmwareVerInternal, "fetchingFirmwareVersionInternal");
    stateMachine->addState(fetchingFirmwareVerNIC, "fetchingFirmwareVersionNIC");
    stateMachine->addState(fetchingPCBNumber, "fetchingPCBNumber");
    stateMachine->addState(fetchingRTCDrift, "fetchingRTCDrift");
    stateMachine->addState(updateRTC, "updateRTC");
    stateMachine->addState(doingFunctionTest, "doingFunctionTest");
    stateMachine->addState(failed);
    stateMachine->addState(functionTestFinish);

    stateMachine->setInitialState(authenticating);
}

void FunctionTest::startTest()
{
    qDebug() << tag << "startTest";
    retryCounter = 1;
    testResult.resetValue();
    testResult.startTestTime = QDateTime::currentDateTime();
    stateMachine->start();
}

void FunctionTest::fetchSerial()
{
    QByteArray streamData, crcData;
    QDataStream stream (&streamData, QIODevice::WriteOnly);
    stream << (quint8) 0x05;//data type
    stream << (quint8) 0x01;//read
    stream << (quint8) 0x05;//memory
    stream << (quint32) 0x0D;//address;
    stream << (quint8)  0x21;;//size of dummy data
    for(int i=0;i< 0x21;i++)
        stream << (quint8)0x00;
    auto finalPacket = createPacket(streamData,3);
    sendPacket(finalPacket);
}

void FunctionTest::fetchPCBNumber()
{
    QByteArray streamData, crcData;
    QDataStream stream (&streamData, QIODevice::WriteOnly);

    stream << (quint8) 0x05;//data type
    stream << (quint8) 0x01;//read
    stream << (quint8) 0x05;//memory
    stream << (quint32)0x0B;//address;
    stream << (quint8) 0x04;//size of dummy data
    for(int i=0;i<0x04;i++)
        stream << (quint8)0x00;

    streamData = createPacket(streamData, 3);
    sendPacket(streamData);
    qDebug() << tag << streamData.toHex();
}

void FunctionTest::printState()
{
    qDebug() << tag << "evt" << QDateTime::currentDateTime() << time.msecsTo(QDateTime::currentDateTime()) << "state" << stateMachine->lastState() << "->" << stateMachine->state() ;
    time = QDateTime::currentDateTime();
}

void FunctionTest::fetchRTCDrift()
{
    QByteArray streamData, crcData;
    QDataStream stream (&streamData, QIODevice::WriteOnly);
    stream << (quint8) 0x05;//data type
    stream << (quint8) 0x01;//read
    stream << (quint8) 0x05;//memory
    stream << (quint32) 0x03;//address;
    stream << (quint8) 0x09;//size of dummy data
    for(int i=0;i<9;i++)
        stream << (quint8)0x00;

    streamData = createPacket(streamData, 3);
    sendPacket(streamData);
    qDebug() << tag << streamData.toHex();
}

void FunctionTest::setRTC()
{
    QByteArray streamData;
    QDataStream stream (&streamData, QIODevice::WriteOnly);
    stream << (quint8) 0x05;//data type
    stream << (quint8) 0x00;//write
    stream << (quint8) 0x05;//memory
    stream << (quint32) 0x03;//address;
    stream << (quint8) 0x08;//size of data

    QDateTime dateTime = QDateTime::currentDateTime();
    QString dateTimeString = dateTime.toString("ssmmHHddMMyyyy");
    qDebug() << tag << "setRTC" << dateTimeString << dateTime.toString();
    for (int i = 0; i < dateTimeString.length(); i += 2) {
        if(i==8)stream <<(quint8) dateTime.date().dayOfWeek();
        quint8 x = ((dateTimeString[i].toLatin1() - '0') << 4 | (dateTimeString[i + 1].toLatin1() - '0'));
        stream << x;
    }

    streamData = createPacket(streamData, 3);
    qDebug()  << tag << "setRTC" << streamData.toHex();
    sendPacket(streamData);
}

void FunctionTest::startFunctionalTest()
{
    QByteArray streamData, crcData;
    QDataStream stream (&streamData, QIODevice::WriteOnly);

    stream << (quint8) 0x05;//data type
    stream << (quint8) 0x01;//read
    stream << (quint8) 0x05;//memory
    stream << (quint32)0x15;//address;
    stream << (quint8) 0x01;//
    stream << (quint8) 0x00;//

    auto finalPacket = createPacket(streamData, 3);
    sendPacket(finalPacket);
    qDebug() << finalPacket.toHex();
}


void FunctionTest::fetchFWVersion()
{
    QByteArray streamData;
    QDataStream stream (&streamData, QIODevice::WriteOnly);

    stream << (quint8) 0x05;//data type
    stream << (quint8) 0x01;//read
    stream << (quint8) 0x05;//memory
    stream << (quint8) 0x00;
    stream << (quint8) 0x00;
    stream << (quint8) 0x00;
    stream << (quint8) 0x04;
    stream << (quint8) 0x0a;
    for(int i = 0; i < 0x0a; i++)
        stream << (quint8)0x00;

    streamData = createPacket(streamData, 3);
    sendPacket(streamData);
    qDebug() << streamData.toHex();
}

void FunctionTest::fetchFWVersionInternal()
{
    QByteArray streamData;
    QDataStream stream (&streamData, QIODevice::WriteOnly);

    stream << (quint8) 0x05;//data type
    stream << (quint8) 0x01;//read
    stream << (quint8) 0x05;//memory
    stream << (quint8) 0x00;
    stream << (quint8) 0x00;
    stream << (quint8) 0x00;
    stream << (quint8) 0x26;
    stream << (quint8) 0x0f;
    for(int i = 0; i < 0x0f; i++)
        stream << (quint8)0x00;

    streamData = createPacket(streamData, 3);
    sendPacket(streamData);
    qDebug() << streamData.toHex();
}

void FunctionTest::fetchFWVersionNameplate()
{
    QByteArray streamData;
    QDataStream stream (&streamData, QIODevice::WriteOnly);

    stream << (quint8) 0x05;//data type
    stream << (quint8) 0x01;//read
    stream << (quint8) 0x05;//memory
    stream << (quint8) 0x00;
    stream << (quint8) 0x00;
    stream << (quint8) 0x00;
    stream << (quint8) 0x04;
    stream << (quint8) 0x0a;
    for(int i = 0; i < 0x0a; i++)
        stream << (quint8)0x00;

    streamData = createPacket(streamData, 3);
    sendPacket(streamData);
    qDebug() << streamData.toHex();
}

void FunctionTest::fetchFirmwareVersionNIC()
{
    QByteArray streamData;
    QDataStream stream (&streamData, QIODevice::WriteOnly);

    stream << (quint8) 0x05;//data type
    stream << (quint8) 0x01;//read
    stream << (quint8) 0x05;//memory
    stream << (quint8) 0x00;
    stream << (quint8) 0x00;
    stream << (quint8) 0x00;
    stream << (quint8) 0x09;
    stream << (quint8) 0x10;
    for(int i = 0; i < 0x10; i++)
        stream << (quint8)0x00;

    streamData = createPacket(streamData, 3);
    sendPacket(streamData);
    qDebug() << streamData.toHex();
}

void FunctionTest::processSerial(QByteArray packet)
{
//    QDataStream stream (&packet, QIODevice::ReadOnly);
//    stream.setByteOrder(QDataStream::LittleEndian);
//    stream.skipRawData(7);

//    char meterSerialNumber[13];
//    quint8 packetLength;

//    quint8 *ptr = (quint8*)meterSerialNumber;
//    stream >> packetLength;

//    for(int i=0; i<10; i++)
//        stream >> *ptr++;

//    meterSerialNumber[packetLength] = 0;

//    testResult.meterSerial = QString(meterSerialNumber);
//    qDebug() << tag <<  "procesFetchSerial" << meterSerialNumber;
//    if(!testResult.meterSerial.isEmpty())
//        emit fetchSerialPass();
//    else
//        emit fetchSerialFail();
}

bool FunctionTest::processPCBNumber(QByteArray packet)
{
    QDataStream stream (&packet, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.skipRawData(6); // Skip the first 6 bytes
    quint32 result;
    stream >> result;
    testResult.pcbNumber = result;
    qDebug() << tag <<"PCB number: " <<result;
    emit fetchPCBNumberPass();
    return true;
}


bool FunctionTest::processRTCDrift(QByteArray packet)
{

    if(packet.size()<17){
        emit fetchRTCDriftFail();
        return false;
    }

    quint8 result = packet[packet.length()-4];
    if(result){
        testResult.errorDetails += functionTestID[0] + " fail ";
        qDebug () << tag << " result "<< result << " cover closed " << functionTestID[0] + " fail ";
         emit fetchRTCDriftFail();
        return false;
    }

    QString dtStr = QString("%1:%2:%3 %4/%5/%6%7")
            .arg(static_cast<quint8>(packet.at(8)), 2, 16, QLatin1Char('0'))
            .arg(static_cast<quint8>(packet.at(7)), 2, 16, QLatin1Char('0'))
            .arg(static_cast<quint8>(packet.at(6)), 2, 16, QLatin1Char('0'))
            .arg(static_cast<quint8>(packet.at(9)), 2, 16, QLatin1Char('0'))
            .arg(static_cast<quint8>(packet.at(11)), 2, 16, QLatin1Char('0'))
            .arg(static_cast<quint8>(packet.at(12)), 2, 16, QLatin1Char('0'))
            .arg(static_cast<quint8>(packet.at(13)), 2, 16, QLatin1Char('0'));

    //packetReceived "02030f050001262418260209202300534703" "fetchingRTCDrift"
    //                02030f050001212618260209202300b1ca03
    QString dateFormat = "HH:mm:ss dd/MM/yyyy";
    QDateTime meterRTC = QDateTime::fromString(dtStr, dateFormat);
    qint64 deviationSec = QDateTime::currentSecsSinceEpoch() - meterRTC.toSecsSinceEpoch();

    testResult.rtcDriftSecond = deviationSec;
    qDebug() << tag << "processRTC" << meterRTC << dtStr << deviationSec;
    if (1/*qAbs(deviationSec) <= MAX_RTC_DRIFT*/)
    {
        if (qAbs(deviationSec) > 2)
        {
            emit fetchRTCUpdateRTC();
        }
        else{
            emit fetchRTCDriftPass();
        }
        return true;
    }
    else
    {
        qDebug() << tag << "Error : RTC_DRIFTED, Deviation in sec is " << qAbs(deviationSec) << ", Total deviation allowed = " << MAX_RTC_DRIFT;
    }

    emit fetchRTCDriftFail();

    return false;
}

bool FunctionTest::processSetRTC(QByteArray packet)
{
    if(packet.at(5) == 1)
        emit setRTCPass();
    else
        emit setRTCFail();

    return true;
}

bool FunctionTest::processFunctionTest(QByteArray packet)
{
    quint8 result = packet[6];
    qDebug() << tag << "processFunctionTest" << functionTestID << result;

    quint8* ptr = (quint8*)&testResult.result;
    *ptr = result;
    bool allPass = true;
    for (int i = 0; i < 1; i++)
    {
        bool isFail = (packet[6] & (1 << i));
        qDebug() << tag << functionTestID[i] <<  (packet[6] & (1 << i)) << isFail;
        //        if(i== 2 || i==3)
        //            continue;

        if(isFail){
            testResult.errorDetails += functionTestID[i] + " fail ";
            allPass = false;
        }
    }

    if(!allPass){
        emit functionTestFail();
        return false;
    }
    emit functionTestPass();

    return true;
}


bool FunctionTest::processFirmwareVersion(QByteArray packet)
{
    uint8_t length = packet.at(2);
    testResult.firmwareVersion = packet.mid(6, length);

    qDebug() << tag <<" Firmware Nameplate "<<testResult.firmwareVersion;
    emit fetchFirmwareVersionPass();
    return true;
}

bool FunctionTest::processFirmwareVersionNameplate(QByteArray packet)
{
    uint8_t length = packet.at(2);
    testResult.firmwareVersionNameplate = packet.mid(6, length-3);

    QString firmwareVersionNameplate = meter_keys->GetFirmwareVersionNameplate();
    qDebug() << tag <<" Firmware Nameplate "<<testResult.firmwareVersionNameplate << " version "<<firmwareVersionNameplate;

    if(testResult.firmwareVersionNameplate == firmwareVersionNameplate)
        emit fetchFirmwareVersionNameplatePass();
    else
        emit fetchFirmwareVersionNameplateFail();
    return true;
}

bool FunctionTest::processFirmwareVersionInternal(QByteArray packet)
{
    uint8_t length = packet.at(2);
    testResult.firmwareVersion = packet.mid(6, length);

    QString firmwareVersion = meter_keys->GetFirmwareInternalVersion();
    qDebug() << tag <<" Firmware Internal "<<testResult.firmwareVersion << " version "<<firmwareVersion;

    if(testResult.firmwareVersion == firmwareVersion)
        emit fetchFirmwareVersionInternalPass();
    else
        emit fetchFirmwareVersionInternalFail();
    return true;
}

bool FunctionTest::processFirmwareVersionNIC(QByteArray packet)
{
    //       RESP: 02 03 16 05 00 01 33 2E 30 2E 30 2E 30 3A 32 33 00 00 00 00 00 00 10 AA 03
    //    ACTRESP: 02 03 06 05 00 02 83 8e 03

    uint8_t length = packet.at(2);
    QString firmware = packet.mid(6, length);

    qDebug() << tag << " Firmware NIC "<<testResult.firmwareVersionNIC;
    emit fetchFirmwareVersionNICPass();
    return true;
}

void FunctionTest::processStateMachineStoped()
{
    testResult.lastState = stateMachine->lastState();
    testResult.endTestTime = QDateTime::currentDateTime();
    testResult.testDuration = testResult.startTestTime.secsTo(testResult.endTestTime);

    if(stateMachine->configuration().contains(functionTestFinish)){
        testResult.isTestPass = true;
    }
    else if(retryCounter-- > 0){
        testResult.resetValue();
        stateMachine->start();
        return;
    }
    else{
        testResult.errorDetails += "Failed in " + stateMachine->lastState();
    }
    qDebug() << tag << testResult.getResult();
    qDebug() << tag << "### duration" << testResult.testDuration;
    closeSerialPort();
    emit functionTestResult(&testResult);
}

void FunctionTest::packetReceived(QByteArray packet)
{
    qDebug() << tag << "packetReceived" << packet.toHex() << stateMachine->state();
    if(stateMachine->configuration().contains(stFetchSerial)){
        processSerial(packet);
    }
    else if(stateMachine->configuration().contains(fetchingPCBNumber)){
        processPCBNumber(packet);
    }
    else if(stateMachine->configuration().contains(fetchingRTCDrift)){
        processRTCDrift(packet);
    }
    else if(stateMachine->configuration().contains(updateRTC)){
        processSetRTC(packet);
    }
    else if(stateMachine->configuration().contains(doingFunctionTest)){
        processFunctionTest(packet);
    }
    else if(stateMachine->configuration().contains(fetchingFirmwareVer)){
        processFirmwareVersion(packet);
    }
    else if(stateMachine->configuration().contains(fetchingFirmwareVerNameplate)){
        processFirmwareVersionNameplate(packet);
    }
    else if(stateMachine->configuration().contains(fetchingFirmwareVerInternal)){
        processFirmwareVersionInternal(packet);
    }
    else if(stateMachine->configuration().contains(fetchingFirmwareVerNIC)){
        processFirmwareVersionNIC(packet);
    }
}
