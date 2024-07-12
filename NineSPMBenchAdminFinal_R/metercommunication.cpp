#include "metercommunication.h"
#include <QDebug>
#include <QDataStream>
#include <QDateTime>
#include <QTimer>
#include <QCoreApplication>

#include "aes/qaesencryption.h"

void MeterCommunication::setupSerial()
{
    serial = new QSerialPort(portName, this);
    serial->setBaudRate(9600);
    serial->setParity(QSerialPort::NoParity);
    serial->setFlowControl(QSerialPort::NoFlowControl);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setDataBits(QSerialPort::Data8);

    encrypt = new QAESEncryption(QAESEncryption::AES_128, QAESEncryption::ECB);
    timer = new QTimer(this);
//    timer->start(1000);

    connect(timer, &QTimer::timeout, this, & MeterCommunication::readData);
    if(!mBlocking)
        connect(serial, &QSerialPort::readyRead, this, &MeterCommunication::readData);

    qDebug() << tag << "setupSerial portOpen" << serial->open(QIODevice::ReadWrite) << portName;
//    serial->setDataTerminalReady(true);
}

MeterCommunication::MeterCommunication(QString portName, QString tag, QObject *parent, bool blocking)
    : QObject{parent}
{
    mBlocking = blocking;
    this->tag = tag;
    this->portName = portName;
    receiverStream = new QDataStream(&receiverBuffer, QIODevice::WriteOnly);

    mStateMachine = new PVStateMachine(this);
    connect(mStateMachine, SIGNAL(stateChanged()), this, SLOT(printState()));

    setupSerial();

    receiverState = RECEIVER_IDEAL;
    isAuthenticating = false;
}

MeterCommunication::~MeterCommunication()
{
    closeSerialPort();
}

void MeterCommunication::readData()
{
    while(serial->bytesAvailable()){
        auto tmp = serial->readAll();
        totalData.append(tmp);
        receiverBuffer.append(tmp);
//        qInfo() << "bufferInfo" << receiverBuffer;
//        qInfo() << "totalData" << totalData;

        if(receiverState == RECEIVER_IDEAL && receiverBuffer.size() > 2){
            dataToBeReceived = receiverBuffer.at(2);
            receiverState = RECEIVER_HEADER_RECEIVED;
        }

        if(receiverState == RECEIVER_HEADER_RECEIVED && receiverBuffer.size() >= dataToBeReceived + 3){
            QByteArray data = receiverBuffer.mid(0, dataToBeReceived + 3);
            receiverBuffer.remove(0, dataToBeReceived + 3);
            qDebug() << tag << "packetReceived" << data.toHex();

            bool isValid = isPacketValid(data);
            qDebug() << tag << "packetReceived" << data.toHex() << " isValid "<<isValid << " receiverBuffer "<<receiverBuffer;
            if(!isValid){
                receiverState = RECEIVER_IDEAL;
                return;
            }

            if(mStateMachine->state() == "authenticating"){

            }

            if(isAuthenticating){
                if(data.length() > 6 && data.at(5) == 1)
                    emit authenticationPass();
                else
                    emit authenticationFail();
                isAuthenticating = false;
            }
            else{
                emit packetReceived(data);
            }
            receiverState = RECEIVER_IDEAL;
        }
    }
}

bool MeterCommunication::readPacket(QByteArray &data, int timeOut)
{
    QDateTime startTime = QDateTime::currentDateTime();

    while(startTime.msecsTo(QDateTime::currentDateTime()) < timeOut){
        serial->waitForBytesWritten(100);
        serial->waitForReadyRead(100);
//        qDebug() << tag << serial->bytesAvailable();
        if(receiverState == RECEIVER_IDEAL && serial->bytesAvailable() > 2){
            auto tmp = serial->read(3);
            dataToBeReceived = tmp.at(2);
            receiverState = RECEIVER_HEADER_RECEIVED;
        }
        if(receiverState == RECEIVER_HEADER_RECEIVED && serial->bytesAvailable() >= dataToBeReceived){
            data = serial->read(dataToBeReceived + 3);

            if(isAuthenticating){
                if(data.at(5) == 1)
                    emit authenticationPass();
                else
                    emit authenticationFail();
                isAuthenticating = false;
            }
            else
                emit packetReceived(data);
            receiverState = RECEIVER_IDEAL;

            return true;
        }
        QCoreApplication::processEvents();
    }
    return false;
}

void MeterCommunication::printState()
{
    qDebug() << tag << "evt" << QDateTime::currentDateTime() << mStartTime.msecsTo(QDateTime::currentDateTime()) << "state" << mStateMachine->lastState() << "->" << mStateMachine->state() ;
    mStartTime = QDateTime::currentDateTime();
}

void MeterCommunication::processAuthentication(QByteArray &packet)
{

}

QByteArray MeterCommunication::createPacket(MEM_SELECT memory, QByteArray data){
    QByteArray streamData, crcData;
    QDataStream stream (&streamData, QIODevice::WriteOnly);

    stream << (quint8)0x02; // preframe
    stream << (quint8)0x01; // command code
    stream << (quint8)(data.length() + 4); // data size
    stream << (quint8) memory; //match .net code
    for(int i=0; i< data.length(); i++)
        stream << (quint8) data.at(i);

    crcData = streamData;
    crcData.remove(0, 1);

    stream << (quint16) qChecksum(crcData.constData(), crcData.length()); //getCrc(crcData); // crc
    stream << (quint8)0x03; // frame end
    return streamData;
}

QByteArray MeterCommunication::createPacket(QByteArray data, quint8 adjustPacketSize){
    QByteArray streamData, crcData;
    QDataStream stream (&streamData, QIODevice::WriteOnly);

    stream << (quint8)0x02; // preframe
    stream << (quint8)0x01; // command code
    stream << (quint8)(data.length() + adjustPacketSize); // data size

    for(int i=0; i< data.length(); i++)
        stream << (quint8) data.at(i);

    crcData = streamData;
    crcData.remove(0, 1);

    stream << (quint16) qChecksum(crcData.constData(), crcData.length()); //getCrc(crcData); // crc
    stream << (quint8)0x03; // frame end
    return streamData;
}

void MeterCommunication::delay(int msec)
{
    QDateTime startTime = QDateTime::currentDateTime();
    while(startTime.msecsTo(QDateTime::currentDateTime()) < msec){
        QCoreApplication::processEvents();
    }
}


bool MeterCommunication::authenticate()
{
    QByteArray hexText, outputHex;

    QByteArray key = QString("k_meter_umd_auth").toLocal8Bit();
    QByteArray inText = QString("abcdefgh.12345678.a1@$kimbal.com").toLocal8Bit();

    QByteArray encodedData = encrypt->encode(inText, key);

    //Workaround only open port when authenticate retry, retry will happen quickly within 6 sec.
    static QDateTime lastDatetime;
    if(lastDatetime.secsTo(QDateTime::currentDateTime()) < 6){
        if(serial->isOpen())
            closeSerialPort();
        auto lastSerial = serial;
        receiverState = RECEIVER_IDEAL;
        receiverBuffer.clear();
        setupSerial();
        lastSerial->deleteLater();
    }

    lastDatetime = QDateTime::currentDateTime();

    isAuthenticating = true;
    auto data = createPacket(MeterCommunication::MEM_SELECT_NONE, encodedData);
    sendPacket(data);

    return false;
}

bool MeterCommunication::isPacketValid(QByteArray packet)
{
    if(packet.size() < 8)
        return false;

    QDataStream readStream (&packet, QIODevice::ReadOnly);
    readStream.setByteOrder(QDataStream::BigEndian);
    quint16 preframe, crc;
    quint8 end;

    readStream >> preframe;

    readStream.skipRawData(packet.size() - 5);

    readStream >> crc;
    readStream >> end;

    QByteArray crcData = packet.mid(1, packet.size()-4);
    quint16 receiveCRC = (quint16) qChecksum(crcData.constData(), crcData.length());
    if(preframe == 0x0203 && end == 0x03 && crc == receiveCRC){
        return true;
    }

    return false;
}

bool MeterCommunication::calibrate()
{
    if(mStateMachine->isRunning())
        return false;

    mStateMachine->removeAllStates();

    QState* authenticating = new QState();
    QState* calibrating = new QState();
//    calibrating = new QState();
    QFinalState* failed = new QFinalState();
    QFinalState* calibrated = new QFinalState();

    connect(mStateMachine, SIGNAL(stateChanged()), this, SLOT(printState()));

    connect(mStateMachine, SIGNAL(started()), this, SLOT(authenticate()));
    authenticating->addTransition(this, SIGNAL(authenticationPass()), calibrating);
    authenticating->addTransition(this, SIGNAL(authenticationFail()), failed);

    calibrating->addTransition(this, SIGNAL(calibrationSuccess()), calibrated);
    calibrating->addTransition(this, SIGNAL(calibrationFail()), failed);
    connect(calibrating, SIGNAL(entered()), this, SLOT(meterSendCalibrate()));

    mStateMachine->addState(authenticating, "authenticating");
    mStateMachine->addState(calibrating, "calibrating");
//    calibrateMachine->addState(calibrating);
    mStateMachine->addState(failed);
    mStateMachine->addState(calibrated);
    mStateMachine->setInitialState(authenticating);

    mStateMachine->start();

    return true;
}

void MeterCommunication::processCalibrate(QByteArray &packet)
{

}

void MeterCommunication::sendPacket(QByteArray &packet)
{
    this->packet = packet;
    QTimer::singleShot(100, this, SLOT(sendPacket()));
}

void MeterCommunication::sendPacket()
{
    qDebug() << tag << "sendingPacket" << packet.toHex();
    serial->write(packet);
}

bool MeterCommunication::sendPacketWaitReply(QByteArray &packet)
{
    return true;
}

void MeterCommunication::openSerialPort()
{
    qDebug() << tag << "portOpen" << serial->open(QIODevice::ReadWrite);
}

void MeterCommunication::closeSerialPort()
{
    if(serial->isOpen()){
        serial->close();
        qDebug() << tag << "serialPortClosed";
    }
    else
        qDebug() << tag << "serialPortAlreadyClosed";
}
