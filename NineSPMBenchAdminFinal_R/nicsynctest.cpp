#include "nicsynctest.h"

#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QTimer>
#include <QState>
#include <QStateMachine>

NicSyncTest::NicSyncTest(QString port, EnCustomer customer, QString meterNo, QString networkModule, bool setSerial, QObject *parent) : MeterCommunication(port, "NST" + meterNo, parent)
{
    meter_keys = new MeterKeys();
    meter_keys->SetCustomer(customer);
    isSetSerial = setSerial;


    connect(this, &MeterCommunication::packetReceived, this, & NicSyncTest::packetReceived);
    connect(&stateMachine, SIGNAL(stateChanged()), this, SLOT(printState()));
    connect(&stateMachine, SIGNAL(finished()), this, SLOT(processStateMachineStoped()));

    authenticating = new PVState();
    fetchingVI = new PVState();
    startAccumalation = new PVState();
    fetchingInput = new PVState();
    fetchingOutput = new PVState();
    fetchingFirmwareVerNIC = new PVState();
    doingFunctionTest = new PVState();
    fetchingInputSecond = new PVState();
    fetchingAuth = new PVState();
    fetchingHLS = new PVState();
    fetchingEncry = new PVState();
    verifyingQR = new PVState();
    stopEnergyAccumulation = new PVState();
    setSerialNo = new PVState();
    authenticating2 = new PVState();
    verifySingleWireOffSetTest = new PVState();
    stReadRFParameter = new PVState();
    meterSerialNo = new PVState();
    coverOpen = new PVState();
    failed = new QFinalState();
    nicsyncTestFinish = new QFinalState();

    connect(authenticating, SIGNAL(entered()), this, SLOT(authenticate()));
    authenticating->addTransition(this, SIGNAL(authenticationPass()), fetchingVI);
    authenticating->addFailTransition(this, SIGNAL(authenticationFail()), failed);

    connect(fetchingVI, SIGNAL(entered()), this, SLOT(fetchVI()));
    if(networkModule == "4G")
        fetchingVI->addTransition(this, SIGNAL(fetchVIPass()), verifySingleWireOffSetTest);
    else
        fetchingVI->addTransition(this, SIGNAL(fetchVIPass()), startAccumalation);
    fetchingVI->addFailTransition(this, SIGNAL(fetchVIFail()), failed);

    connect(startAccumalation, SIGNAL(entered()), this, SLOT(startAccumalationEnergy()));
    startAccumalation->addTransition(this, SIGNAL(startAccumalationPass()), fetchingInput);
    startAccumalation->addFailTransition(this, SIGNAL(startAccumalationFail()), failed);

    //input output
    connect(fetchingInput, SIGNAL(entered()), this, SLOT(fetchIN()));
    fetchingInput->addTransition(this, SIGNAL(fetchingInputPass()), fetchingOutput);
    fetchingInput->addFailTransition(this, SIGNAL(failedInOut()), failed, 4000, 4);

    connect(fetchingOutput, SIGNAL(entered()), this, SLOT(fetchOUT()));
    fetchingOutput->addTransition(this, SIGNAL(fetchingOutputPass()),  fetchingInputSecond);
    fetchingOutput->addFailTransition(this, SIGNAL(failedInOut()), failed);

    //input output
    connect(fetchingInputSecond, SIGNAL(entered()), this, SLOT(fetchIN()));
    fetchingInputSecond->addTransition(this, SIGNAL(fetchingInputPass()), fetchingAuth);
    fetchingInputSecond->addFailTransition(this, SIGNAL(failedInOut()), failed);

    //RF
    connect(fetchingAuth, SIGNAL(entered()), this, SLOT(fetchAuth()));
    fetchingAuth->addTransition(this, SIGNAL(fetchingAuthPass()), fetchingHLS);
    fetchingAuth->addFailTransition(this, SIGNAL(failedRF()), failed, 2000, 8);

    connect(fetchingHLS, SIGNAL(entered()), this, SLOT(fetchHLS()));
    fetchingHLS->addTransition(this, SIGNAL(fetchingHLSPass()), fetchingEncry);
    fetchingHLS->addFailTransition(this, SIGNAL(failedRF()), failed, 2000, 8);

    connect(fetchingEncry, SIGNAL(entered()), this, SLOT(fetchEncry()));
    fetchingEncry->addTransition(this, SIGNAL(fetchingEncryPass()), fetchingFirmwareVerNIC);
    fetchingEncry->addFailTransition(this, SIGNAL(failedRF()), failed, 2000, 8);

    connect(fetchingFirmwareVerNIC, SIGNAL(entered()), this, SLOT(fetchFirmwareVersionNIC()));
    fetchingFirmwareVerNIC->addTransition(this, SIGNAL(fetchFirmwareVersionNICPass()), stopEnergyAccumulation);
    fetchingFirmwareVerNIC->addFailTransition(this, SIGNAL(fetchFirmwareVersionNICFail()), failed, 2000, 5);

    connect(verifyingQR, SIGNAL(entered()), this, SLOT(verifyQR()));
    verifyingQR->addTransition(this, SIGNAL(verifyingQRPass()), stopEnergyAccumulation);
    verifyingQR->addFailTransition(this, SIGNAL(verifyingQRFail()), failed);

    connect(stopEnergyAccumulation, SIGNAL(entered()), this, SLOT(stopAccumalationEnergy()));
        if(networkModule == "4G")
            stopEnergyAccumulation->addTransition(this, SIGNAL(stopEnergyAccumulationPass()), verifySingleWireOffSetTest);
        else
            stopEnergyAccumulation->addTransition(this, SIGNAL(stopEnergyAccumulationPass()), stReadRFParameter);

    stopEnergyAccumulation->addFailTransition(this, SIGNAL(stopEnergyAccumulationFail()), failed);

    connect(doingFunctionTest, SIGNAL(entered()), this, SLOT(startFunctionalTest()));
    doingFunctionTest->addTransition(this, SIGNAL(functionTestPass()), nicsyncTestFinish);
    doingFunctionTest->addFailTransition(this, SIGNAL(functionTestFail()), failed, 6000, 2);


    connect(stReadRFParameter, SIGNAL(entered()), this, SLOT(readRFParameter()));
    stReadRFParameter->addTransition(this, SIGNAL(readRFParametersPass()), verifySingleWireOffSetTest);
    stReadRFParameter->addFailTransition(this, SIGNAL(readRFParametersFail()), failed);

    connect(verifySingleWireOffSetTest, SIGNAL(entered()), this, SLOT(singleWireOffSet()));
    if(setSerial)
        verifySingleWireOffSetTest->addTransition(this, SIGNAL(verifySingleWireOffSetTestPass()), setSerialNo);
    else
        verifySingleWireOffSetTest->addTransition(this, SIGNAL(verifySingleWireOffSetTestPass()), doingFunctionTest);
    verifySingleWireOffSetTest->addFailTransition(this, SIGNAL(verifySingleWireOffSetTestFail()), failed);

    connect(setSerialNo, SIGNAL(entered()), this, SLOT(setSerial()));
    setSerialNo->addTransition(this, SIGNAL(authenticationPass()), setSerialNo);
    setSerialNo->addFailTransition(this, SIGNAL(authenticationFail()), authenticating2, 2000, 1);
    setSerialNo->addTransition(this, SIGNAL(fetchsetSerialNoFail()), failed);

    connect(authenticating2, SIGNAL(entered()), this, SLOT(authenticate()));
    authenticating2->addTransition(this, SIGNAL(authenticationPass()), meterSerialNo);
    authenticating2->addFailTransition(this, SIGNAL(authenticationFail()), failed);

    connect(meterSerialNo, SIGNAL(entered()), this, SLOT(readRFParameter()));
    meterSerialNo->addTransition(this, SIGNAL(meterSerialNoPass()), coverOpen);
    meterSerialNo->addFailTransition(this, SIGNAL(meterSerialNoFail()), failed);

    connect(coverOpen, SIGNAL(entered()), this, SLOT(checkCoverOpen()));
    coverOpen->addTransition(this, SIGNAL(coverOpenPass()), nicsyncTestFinish);
    coverOpen->addFailTransition(this, SIGNAL(coverOpenFail()), failed);

    stateMachine.addState(authenticating, "authenticating");
    stateMachine.addState(fetchingVI, "fetchingVI");
    stateMachine.addState(startAccumalation, "startAccumalation");
    stateMachine.addState(fetchingInput, "fetchingInput");
    stateMachine.addState(fetchingOutput, "fetchingOutput");
    stateMachine.addState(fetchingInputSecond, "fetchingInputSecond");
    stateMachine.addState(fetchingAuth, "fetchingAuth");
    stateMachine.addState(fetchingHLS, "fetchingHLS");
    stateMachine.addState(fetchingEncry, "fetchingEncry");
    stateMachine.addState(verifyingQR, "verifyingQR");
    stateMachine.addState(stopEnergyAccumulation, "stopEnergyAccumulation");
    stateMachine.addState(setSerialNo, "setSerialNo");
    stateMachine.addState(authenticating2, "authenticating2");
    stateMachine.addState(stReadRFParameter, "stReadRFParameter");
    stateMachine.addState(verifySingleWireOffSetTest, "verifySingleWireOffSetTest");
    stateMachine.addState(fetchingFirmwareVerNIC, "fetchingFirmwareVersionNIC");
    stateMachine.addState(meterSerialNo, "meterSerialNo");
    stateMachine.addState(coverOpen, "coverclosed");
    stateMachine.addState(doingFunctionTest, "coverClosedCheck");

    stateMachine.addState(failed);
    stateMachine.addState(nicsyncTestFinish);

    stateMachine.setInitialState(authenticating);
}

void NicSyncTest::setSerialToVerify(QString serial)
{
    this->meterSerial = serial;
}

void NicSyncTest::startTest()
{
//    qDebug() << tag << "startTest" << meterSerial;
    testResult.resetValue();
    setSerialRetry = 0;
//    setSerialToVerify(meterSerial);
    nicSyncStartTime = QDateTime::currentDateTime();
    testResult.startTestTime = QDateTime::currentDateTime();
    stateMachine.start();
}

void NicSyncTest::setQRToVerify(QString qrCode, QString manufacturingDate)
{
    this->qrCode = qrCode;
//    this->serialPrefix = serialPrefix;
    this->manufacturingDate = manufacturingDate;
}

void NicSyncTest::fetchVI()
{
    QByteArray streamData;
    QDataStream stream (&streamData, QIODevice::WriteOnly);

    stream << (quint8) 0x05;//data type
    stream << (quint8) 0x01;//read
    stream << (quint8) 0x05;//memory
    stream << (quint32) 0x01;//address;
    stream << (quint8) 0x0C;//size of dummy data
    for(int i=0;i<12;i++)
        stream << (quint8)0x00;

    streamData = MeterCommunication::createPacket(streamData, 3);
    sendPacket(streamData);
}

void NicSyncTest::setSerial()
{
    DataManager& dataManager = DataManager::instance();
    QString prefix = dataManager.getPrefix();
    float meterNumber = dataManager.getCurrentSerial();
    if(meterNumber == -1){
        emit fetchsetSerialNoFail();
        return;
    }
    testResult.meterSerialNo = QString("%1").arg(meterNumber);
    meterSerial = QString("%1%2").arg(dataManager.getPrefix()).arg(testResult.meterSerialNo);
    qDebug()<<"CurrentSerialNo" << meterSerial;
    const char* serial = meterSerial.toStdString().c_str();


    QByteArray streamData, crcData;
    QDataStream stream (&streamData, QIODevice::WriteOnly);
    stream << (quint8)0x05;//data_type
    stream << (quint8)0x00;//write
    stream << (quint8)0x05;//memory instant
    stream << (quint32)0x00;//address
    stream << (quint8)0x10;//size of serial data
    for(int i=0; i<0x7; i++){
        stream<<(quint8)0x30;//pass serial no data
    }
    for(int i=0; i<0x9; i++){
        stream<<(quint8)*serial++;//pass serial no data
    }
    streamData = MeterCommunication::createPacket(streamData, 3);
    sendPacket(streamData);

    if(setSerialRetry++ < 3){
        QTimer::singleShot(1000, this, SLOT(authenticate()));
    }
    else
        emit setSerialVerifyFail();
}

void NicSyncTest::checkSerial()
{
    QByteArray streamData, crcData;
    QDataStream stream (&streamData, QIODevice::WriteOnly);
    stream << (quint8)0x05;//data_type
    stream << (quint8)0x01;//read
    stream << (quint8)0x05;//memory instant
    stream << (quint32)0x00;//address
    stream << (quint8)0x10;//size of dummy data
    for(int i=0;i<0x10;i++)
        stream<<(quint8)0x00;//pass serial no data
    auto finalPacket = createPacket(streamData,3);
    sendPacket(finalPacket);
}

void NicSyncTest::startAccumalationEnergy()
{
    QByteArray streamData, crcData;
    QDataStream stream (&streamData, QIODevice::WriteOnly);
    stream << (quint8) 0x05;//data type
    stream << (quint8) 0x01;//read
    stream << (quint8) 0x05;//memory
    stream << (quint32) 0x16;//address;
    stream << (quint8) 0x08;//size of dummy data
    for(int i=0;i<8;i++)
        stream << (quint8)0x00;
    auto finalPacket = createPacket(streamData,3);
    sendPacket(finalPacket);
}

void NicSyncTest::startFunctionalTest()
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
void NicSyncTest::stopAccumalationEnergy()
{
    QByteArray streamData, crcData;
    QDataStream stream (&streamData, QIODevice::WriteOnly);
    stream << (quint8) 0x05;//data type
    stream << (quint8) 0x01;//read
    stream << (quint8) 0x05;//memory
    stream << (quint32) 0x17;//address;
    stream << (quint8) 0x14;//size of dummy data
    for(int i=0;i<0x14;i++)
        stream << (quint8)0x00;
    auto finalPacket = createPacket(streamData,3);
    sendPacket(finalPacket);
}

void NicSyncTest::fetchIN()
{
    QByteArray streamData = QByteArray::fromHex("02010603080B10DE03");
    sendPacket(streamData);
}

void NicSyncTest::fetchOUT()
{
    QByteArray streamData = QByteArray::fromHex("02010603080C646103");
    sendPacket(streamData);
}

void NicSyncTest::fetchAuth(){
    QByteArray streamData, crcData;
    QDataStream stream (&streamData, QIODevice::WriteOnly);
    stream << (quint8) 0x05;//data type
    stream << (quint8) 0x01;//read
    stream << (quint8) 0x05;//memory instant
    stream << (quint32) 0x06;//address;
    stream << (quint8) 0x10;//size of dummy data
    for(int i=0;i<0x10;i++)
        stream << (quint8)0x00;
    auto finalPacket = createPacket(streamData,3);
    sendPacket(finalPacket);
}

void NicSyncTest::fetchHLS(){
    QByteArray streamData, crcData;
    QDataStream stream (&streamData, QIODevice::WriteOnly);
    stream << (quint8) 0x05;//data type
    stream << (quint8) 0x01;//read
    stream << (quint8) 0x05;//memory instant
    stream << (quint32) 0x07;//address;
    stream << (quint8) 0x10;//size of dummy data
    for(int i=0;i<0x10;i++)
        stream << (quint8)0x00;
    auto finalPacket = createPacket(streamData,3);
    sendPacket(finalPacket);
}

void NicSyncTest::verifyQR()
{
    //Get_QR_data \n1P2W SU KUSHALU1-CAA1 Sr.No. AS2342135 11-2023\r
    QString qrToVerify;
    qrCode = qrCode.trimmed();
    if(qrCode.startsWith("\n"))
        qrCode = qrCode.remove(0, 1);

    if(qrCode.endsWith("\r"))
        qrCode.chop(1);

    qrToVerify = QString("1P2W SU KUSHALU1-CAA1 Sr.No. %1 %2").arg(meterSerial).arg(manufacturingDate);
    qDebug() << tag << qrToVerify << qrCode;
    //    if(qrCode == qrToVerify){
    emit verifyingQRPass();
    //    }
    //    else
    //        emit verifyingQRFail();
}

void NicSyncTest::fetchFirmwareVersionNIC()
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

void NicSyncTest::checkCoverOpen()
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

bool NicSyncTest::processFirmwareVersionNIC(QByteArray packet)
{
    uint8_t length = packet.at(2);
    testResult.firmwareVersionNIC = packet.mid(6, length-3);

    QString firmwareVersionNIC = meter_keys->GetRFVersion();
    qDebug() << tag <<" Firmware NIC "<<testResult.firmwareVersionNIC << " version "<<firmwareVersionNIC;

    if(testResult.firmwareVersionNIC == firmwareVersionNIC)
        emit fetchFirmwareVersionNICPass();
    else
        emit fetchFirmwareVersionNICFail();
    return true;
}

void NicSyncTest::fetchEncry(){
    QByteArray streamData, crcData;
    QDataStream stream (&streamData, QIODevice::WriteOnly);
    stream << (quint8) 0x05;//data type
    stream << (quint8) 0x01;//read
    stream << (quint8) 0x05;//memory instant
    stream << (quint32) 0x05;//address;
    stream << (quint8) 0x10;//size of dummy data
    for(int i=0;i<0x10;i++)
        stream << (quint8)0x00;
    auto finalPacket = createPacket(streamData,3);
    sendPacket(finalPacket);
}

void NicSyncTest::readRFParameter()
{
    //02 01 2C 05 01 05 00 00 00 0D 21 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 93 A0 03
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

void NicSyncTest::processReadRFParameter(QByteArray packet){
    QDataStream stream (&packet, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.skipRawData(7);

    char meterSerialNumber[13];
    char radioEncryptionKey[17];

    quint8 packetLength;
    qint32 expectedNetwrokAddress = meter_keys->GetNWAddress();

    quint8 *ptr = (quint8*)meterSerialNumber;
    stream >> packetLength;

    for(int i=0; i<10; i++)
        stream >> *ptr++;

    meterSerialNumber[packetLength] = 0;

    stream >> testResult.networkAddress;
    stream >> testResult.networkChannel;

    ptr = (quint8*)radioEncryptionKey;
    for(int i=0; i<16; i++)
        stream >> *ptr++;
    *ptr = 0;

//    testResult.meterMemoryNumber = QString(meterSerialNumber);
    qDebug() << tag <<  "processReadRFParameter" << testResult.networkAddress << testResult.networkChannel << radioEncryptionKey << meterSerialNumber;

    if(meter_keys->GetNWChannel() != testResult.networkChannel){
        qDebug() << tag  << "NetworkChannel Mismatch" << meter_keys->GetNWChannel() << testResult.networkChannel;
    }
    else if(expectedNetwrokAddress != testResult.networkAddress){
        qDebug() << tag  << "NetworkAddress Mismatch" << meter_keys->GetNWAddress() << testResult.networkAddress;
    }
    else if(meter_keys->GetRFSecurityKey() != QString(radioEncryptionKey)){
        qDebug() << tag  << "EncryptionKey Mismatch" << meter_keys->GetEncryptionKey() << radioEncryptionKey;
    }
    else{
        emit readRFParametersPass();
        return;
    }
    emit readRFParametersFail();
}

void NicSyncTest::singleWireOffSet()
{
    emit verifySingleWireOffSetTestPass();
    return;

    QByteArray streamData, crcData;
    QDataStream stream (&streamData, QIODevice::WriteOnly);
    stream << (quint8) 0x05;//data type
    stream << (quint8) 0x01;//read
    stream << (quint8) 0x05;//memory
    stream << (quint32) 0x1F;//address;
    stream << (quint8)  0x02;;//size of dummy data
    for(int i=0;i< 0x02;i++)
        stream << (quint8)0x00;
    auto finalPacket = createPacket(streamData,3);
    sendPacket(finalPacket);
}

void NicSyncTest::processFetchingVI(QByteArray packet)
{
    QDataStream stream (&packet, QIODevice::ReadOnly);

    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.skipRawData(6);
    stream >> testResult.noLoadVoltage;
    stream >> testResult.noLoadPhaseCurrent;
    stream >> testResult.noLoadNeutralCurrent;
    qDebug() << tag << "Voltage" << testResult.noLoadVoltage;
    qDebug() << tag << "phase_current" << testResult.noLoadPhaseCurrent;
    qDebug() << tag << "neutral_current" << testResult.noLoadNeutralCurrent;

    if(testResult.noLoadVoltage > 160)
        emit fetchVIPass();
    else
        emit fetchVIFail();
}

bool NicSyncTest::processSetSerial(QByteArray packet)
{
    if (packet.at(5) == 1)
    {
        emit fetchsetSerialNoPass();
        return true;
    }
    else
    {
        qDebug() << tag << "Meter locked";
        emit fetchsetSerialNoFail();
        return false;
    }
}

void NicSyncTest::processStartAccumation(QByteArray packet)
{
    if(packet.length()<=5){
        emit startAccumalationFail();
        return;
    }else{
        if(packet.length()==8){
            authenticate();//authentication required
        }else{
            emit startAccumalationPass();
        }
        return;
    }
    emit startAccumalationFail();
}

bool NicSyncTest::processFunctionTest(QByteArray packet)
{
    quint8 result = packet[6];
    qDebug() << tag << "processFunctionTest" << functionTestID << result;

    //    quint8* ptr = (quint8*)&testResult.result;
    //    *ptr = result;
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

void NicSyncTest::processFetchInput(QByteArray packet)
{
    qDebug() << tag << "processFetchInput" << packet.toHex() << packet.at(5);
    if(packet.at(5) == 1)
        emit fetchingInputPass();
    else
        emit failedInOut();
}

void NicSyncTest::processFetchOutput(QByteArray packet)
{
    if(packet.at(5) == 1)
        emit fetchingOutputPass();
    else{
        emit failedInOut();
        qDebug() << "fetchOutput fail 6th bit not 1" << packet.at(6);
    }
}

void NicSyncTest::processCoverOpen(QByteArray packet)
{
    quint8 result = packet[packet.length()-4];

    if(!result){
        emit coverOpenPass();
    }
    else{
        testResult.errorDetails += "CoverOpen fail ";
        emit coverOpenFail();
    }
}

void NicSyncTest::processCheckSerialNo(QByteArray packet)
{
    if(packet.size() < 16){
        emit readRFParametersFail();
        return;
    }

    QDataStream stream (&packet, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.skipRawData(7);

    char meterSerialNumber[13];

    quint8 packetLength;

    quint8 *ptr = (quint8*)meterSerialNumber;
    stream >> packetLength;

    for(int i=0; i<10; i++)
        stream >> *ptr++;

    meterSerialNumber[packetLength] = 0;

    testResult.meterMemoryNumber = QString(meterSerialNumber);
   if(isSetSerial && meterSerial != testResult.meterMemoryNumber){
        qDebug() << tag  << "MeterSerial Mismatch" << meterSerial << QString(meterSerialNumber);
    }
    else{
        emit meterSerialNoPass();
        return;
    }
    emit meterSerialNoFail();
}

void NicSyncTest::processAuthKey(QByteArray packet)
{
    QString key = validateRFKeyValidate(packet);
    qDebug() << tag << "processAuthKey" << key << meter_keys->GetAuthenticationKey();
    if(meter_keys->GetAuthenticationKey()==key){
        emit fetchingAuthPass();
    }
    else{
        emit failedRF();
    }
}

void NicSyncTest::processHLSKey(QByteArray packet)
{
    QString key = validateRFKeyValidate(packet);
    qDebug() << tag << "processHLSKey" << key << meter_keys->GetHLSKey();

    if(meter_keys->GetHLSKey()==key){
        emit fetchingHLSPass();
    }
    else{
        emit failedRF();
    }
}

void NicSyncTest::processEncryptKey(QByteArray packet)
{
    QString key = validateRFKeyValidate(packet);
    qDebug() << tag << "processEncryptKey" << key << meter_keys->GetEncryptionKey();
    if(meter_keys->GetEncryptionKey()==key){
        emit fetchingEncryPass();
    }
    else{
        emit failedRF();
    }
}

QString NicSyncTest::validateRFKeyValidate(QByteArray packet){

    if(packet.size()<10){
        return QString();
    }
    int startIndex = 6;
    int count = packet.length() - 9;
    if (startIndex >= 0 && count >= 0 && startIndex + count <= packet.length()) {
        //           QByteArray subData = packet.left(count).mid(startIndex);
        QByteArray subData = packet.mid(startIndex, count);
        QString Key = QString::fromUtf8(subData.constData(), subData.length());
        qDebug() << tag << "validateRFKeyValidate" << subData;
        return Key;
    }
    return QString();
}

void NicSyncTest::processStateMachineStoped()
{
    closeSerialPort();
    testResult.lastState = stateMachine.lastState();
    testResult.endTestTime = QDateTime::currentDateTime();
    testResult.testDuration = testResult.startTestTime.secsTo(testResult.endTestTime);;
    if(stateMachine.configuration().contains(nicsyncTestFinish)){
        qDebug() << tag << "nicSyncTestPass : closeSerialPort";
        testResult.isTestPass = true;
        emit fetchNicSyncPass();
    }
    else{
        qDebug() << tag << "nicSyncTestFail : closeSerialPort";
        testResult.errorDetails = " FailedAt " + stateMachine.lastState();
        emit fetchNicSyncFail();
    }
    qDebug() << tag << "### duration" << testResult.testDuration;
    emit nicSycnTestResult(&testResult);
    qDebug() << tag << testResult.getResult();
    qDebug() << tag << "testDuration" << nicSyncStartTime.msecsTo(QDateTime::currentDateTime());
}

void NicSyncTest::processSingleWireOffSet(QByteArray packet)
{
    if (packet.at(6) == 0 && packet.at(7) == 0)
    {
        emit verifySingleWireOffSetTestPass();
    }
    else
    {
        emit verifySingleWireOffSetTestFail();
    }
}

void NicSyncTest::processStopEnergyAccumulation(QByteArray packet){
    QDataStream stream (&packet, QIODevice::ReadOnly);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.skipRawData(6);

    stream >> testResult.noLoadPhaseCurrent;
    stream >> testResult.noLoadNeutralCurrent;

    qDebug() << tag << "phase_current" << testResult.noLoadPhaseCurrent;
    qDebug() << tag << "neutral_current" <<testResult.noLoadNeutralCurrent;

    if(testResult.noLoadPhaseCurrent == 0 && testResult.noLoadNeutralCurrent == 0){
        emit stopEnergyAccumulationPass();
    }else{
        emit stopEnergyAccumulationFail();
    }
}

void NicSyncTest::printState()
{
    qDebug() << tag << "evt" << QDateTime::currentDateTime() << time.msecsTo(QDateTime::currentDateTime()) << "state" << stateMachine.lastState() << "->" << stateMachine.state() ;
    time = QDateTime::currentDateTime();
}

void NicSyncTest::packetReceived(QByteArray packet)
{
    if(stateMachine.configuration().contains(coverOpen)){
        processCoverOpen(packet);
    }
    else if(stateMachine.configuration().contains(meterSerialNo)){
        processCheckSerialNo(packet);
    }
    if(stateMachine.configuration().contains(fetchingVI)){
        processFetchingVI(packet);
    }
    else if(stateMachine.configuration().contains(setSerialNo)){
        processSetSerial(packet);
    }
    else if(stateMachine.configuration().contains(startAccumalation)){
        processStartAccumation(packet);
    }
    else if(stateMachine.configuration().contains(fetchingInput)){
        processFetchInput(packet);
    }
    else if(stateMachine.configuration().contains(fetchingOutput)){
        processFetchOutput(packet);
    }
    else if(stateMachine.configuration().contains(fetchingInputSecond)){
        processFetchInput(packet);
    }
    else if(stateMachine.configuration().contains(fetchingAuth)){
        processAuthKey(packet);
    }
    else if(stateMachine.configuration().contains(fetchingHLS)){
        processHLSKey(packet);
    }
    else if(stateMachine.configuration().contains(fetchingEncry)){
        processEncryptKey(packet);
    }
    else if(stateMachine.configuration().contains(verifySingleWireOffSetTest)){
        processSingleWireOffSet(packet);
    }
    else if(stateMachine.configuration().contains(stopEnergyAccumulation)){
        processStopEnergyAccumulation(packet);
    }
    else if(stateMachine.configuration().contains(stReadRFParameter)){
        processReadRFParameter(packet);
    }
    else if(stateMachine.configuration().contains(fetchingFirmwareVerNIC)){
        processFirmwareVersionNIC(packet);
    }
    else if(stateMachine.configuration().contains(doingFunctionTest)){
        processFunctionTest(packet);
    }
}
