#include "spmbenchfulltest.h"
#include <QDebug>
#include <QCoreApplication>


SPMBenchFullTest::SPMBenchFullTest(QObject *parent)
    : QObject{parent}
{


    DataManager& dataManager = DataManager::instance();
    EnCustomer customer = static_cast<EnCustomer>(dataManager.getCustomer());
    setSerialNo = dataManager.getSetSerial();
    float highCurrent = dataManager.getHighCurrent();
    float lowCurrent = dataManager.getLowCurrent();
    float startingCurrent = dataManager.getStartingCurrent();
    QString networkModule = dataManager.getNetworkModule();
    qDebug()<<startingCurrent<<" "<<highCurrent<<" "<<lowCurrent<<" "<<customer << setSerialNo;

    powerSource = new PowerSourceMTE("/dev/ttyUSB11", "/dev/ttyUSB10", highCurrent, lowCurrent, startingCurrent, this);
    powerSource->disablePowerOutput();

    tag = "SPMBench";
    testToStart = TEST_START::TEST_FINISH;
    currentStage = "";
    connect(powerSource, SIGNAL(channelReady(quint8)), this, SLOT(powerStablised(quint8)));


    benchMeter[0] = new BenchMeterTest("/dev/ttyUSB0","Meter1",setSerialNo,customer, highCurrent,lowCurrent,startingCurrent,networkModule,this);
    benchMeter[1] = new BenchMeterTest("/dev/ttyUSB1","Meter2",setSerialNo,customer, highCurrent,lowCurrent,startingCurrent,networkModule, this);
    benchMeter[2] = new BenchMeterTest("/dev/ttyUSB2","Meter3",setSerialNo,customer, highCurrent,lowCurrent,startingCurrent, networkModule,this);
    benchMeter[3] = new BenchMeterTest("/dev/ttyUSB3","Meter4",setSerialNo,customer, highCurrent,lowCurrent,startingCurrent,networkModule,this);
    benchMeter[4] = new BenchMeterTest("/dev/ttyUSB4","Meter5",setSerialNo,customer, highCurrent,lowCurrent,startingCurrent,networkModule,this);
    benchMeter[5] = new BenchMeterTest("/dev/ttyUSB5", "Meter6",setSerialNo,customer, highCurrent,lowCurrent,startingCurrent,networkModule,this);
    benchMeter[6] = new BenchMeterTest("/dev/ttyUSB6", "Meter7",setSerialNo,customer, highCurrent,lowCurrent,startingCurrent,networkModule,this);
    benchMeter[7] = new BenchMeterTest("/dev/ttyUSB7","Meter8",setSerialNo, customer, highCurrent,lowCurrent,startingCurrent,networkModule,this);
    benchMeter[8] = new BenchMeterTest("/dev/ttyUSB8", "Meter9",setSerialNo,customer, highCurrent,lowCurrent,startingCurrent,networkModule,this);

    connect(benchMeter[0], SIGNAL(testFinish(TEST_RESULT,QString)), this, SLOT(meter1Result(TEST_RESULT,QString)));
    connect(benchMeter[1], SIGNAL(testFinish(TEST_RESULT,QString)), this, SLOT(meter2Result(TEST_RESULT,QString)));
    connect(benchMeter[2], SIGNAL(testFinish(TEST_RESULT,QString)), this, SLOT(meter3Result(TEST_RESULT,QString)));
    connect(benchMeter[3], SIGNAL(testFinish(TEST_RESULT,QString)), this, SLOT(meter4Result(TEST_RESULT,QString)));
    connect(benchMeter[4], SIGNAL(testFinish(TEST_RESULT,QString)), this, SLOT(meter5Result(TEST_RESULT,QString)));
    connect(benchMeter[5], SIGNAL(testFinish(TEST_RESULT,QString)), this, SLOT(meter6Result(TEST_RESULT,QString)));
    connect(benchMeter[6], SIGNAL(testFinish(TEST_RESULT,QString)), this, SLOT(meter7Result(TEST_RESULT,QString)));
    connect(benchMeter[7], SIGNAL(testFinish(TEST_RESULT,QString)), this, SLOT(meter8Result(TEST_RESULT,QString)));
    connect(benchMeter[8], SIGNAL(testFinish(TEST_RESULT,QString)), this, SLOT(meter9Result(TEST_RESULT,QString)));

    setAllTestResult(TEST_STATUS_IDEAL);
}

void SPMBenchFullTest::meter1Result(TEST_RESULT testStatus, QString errorDetails)
{
    if(testStatus == TEST_STATUS_PASS){
        meterTestStatus[0] = TEST_STATUS_IDEAL;
    }else{
        errorDetail[0]=errorDetails;
        failStage[0] = currentStage;
        meterTestStatus[0] = TEST_STATUS_FAIL;
    }
    processTestResult();
}

void SPMBenchFullTest::meter2Result(TEST_RESULT testStatus, QString errorDetails)
{
    if(testStatus == TEST_STATUS_PASS){
        meterTestStatus[1] = TEST_STATUS_IDEAL;
    }else{
        errorDetail[1]=errorDetails;
        failStage[1] = currentStage;
        meterTestStatus[1] = TEST_STATUS_FAIL;
    }
    processTestResult();
}

void SPMBenchFullTest::meter3Result(TEST_RESULT testStatus, QString errorDetails)
{
    if(testStatus == TEST_STATUS_PASS){
        meterTestStatus[2] = TEST_STATUS_IDEAL;
    }else{
        errorDetail[2]=errorDetails;
        failStage[2] = currentStage;
        meterTestStatus[2] = TEST_STATUS_FAIL;
    }
    processTestResult();
}

void SPMBenchFullTest::meter4Result(TEST_RESULT testStatus, QString errorDetails)
{
    if(testStatus == TEST_STATUS_PASS){
        meterTestStatus[3] = TEST_STATUS_IDEAL;
    }else{
        errorDetail[3]=errorDetails;
        failStage[3] = currentStage;
        meterTestStatus[3] = TEST_STATUS_FAIL;
    }
    processTestResult();
}

void SPMBenchFullTest::meter5Result(TEST_RESULT testStatus, QString errorDetails)
{
    if(testStatus == TEST_STATUS_PASS){
        meterTestStatus[4] = TEST_STATUS_IDEAL;
    }else{
        errorDetail[4]=errorDetails;
        failStage[4] = currentStage;
        meterTestStatus[4] = TEST_STATUS_FAIL;
    }
    processTestResult();
}

void SPMBenchFullTest::meter6Result(TEST_RESULT testStatus, QString errorDetails)
{
    if(testStatus == TEST_STATUS_PASS){
        meterTestStatus[5] = TEST_STATUS_IDEAL;
    }else{
        errorDetail[5]=errorDetails;
        failStage[5] = currentStage;
        meterTestStatus[5] = TEST_STATUS_FAIL;
    }
    processTestResult();
}

void SPMBenchFullTest::meter7Result(TEST_RESULT testStatus, QString errorDetails)
{
    if(testStatus == TEST_STATUS_PASS){
        meterTestStatus[6] = TEST_STATUS_IDEAL;
    }else{
        errorDetail[6]=errorDetails;
        failStage[6] = currentStage;
        meterTestStatus[6] = TEST_STATUS_FAIL;
    }
    processTestResult();
}

void SPMBenchFullTest::meter8Result(TEST_RESULT testStatus, QString errorDetails)
{
    if(testStatus == TEST_STATUS_PASS){
        meterTestStatus[7] = TEST_STATUS_IDEAL;
    }else{
        errorDetail[7]=errorDetails;
        failStage[7] = currentStage;
        meterTestStatus[7] = TEST_STATUS_FAIL;
    }
    processTestResult();
}

void SPMBenchFullTest::meter9Result(TEST_RESULT testStatus, QString errorDetails)
{
    if(testStatus == TEST_STATUS_PASS){
        meterTestStatus[8] = TEST_STATUS_IDEAL;
    }else{
        errorDetail[8]=errorDetails;
        failStage[8] = currentStage;
        meterTestStatus[8] = TEST_STATUS_FAIL;
    }
    processTestResult();
}

void SPMBenchFullTest::processTestResult()
{
    qDebug() << tag <<  meterTestStatus[0] << meterTestStatus[1] << meterTestStatus[2] << meterTestStatus[3] << meterTestStatus[4] << meterTestStatus[5] << meterTestStatus[6] << meterTestStatus[7] << meterTestStatus[8];
    for (int i = 0; i < 9; ++i) {
        if(meterTestStatus[i] == TEST_STATUS_RUNNING)
            return;
    }



    emit meterColorChanged(testToStart);

    for (int i = 0; i < 9; ++i) {
        if(meterTestStatus[i] != TEST_STATUS_FAIL){
            break;
        }
            if(i==8){
                qDebug()<<" test finsih ";
                qDebug() << tag << "###    duration  "<<currentStage<<" #########" << startTestTime.msecsTo(QDateTime::currentDateTime());
                qDebug() << tag << "###    duration total test duration   #########" << totalTestTime.msecsTo(QDateTime::currentDateTime());
                currentStage = "";
                testToStart = TEST_FINISH;
                finishTest();
                powerSource->sendCommand("R;");
                return;
            }
    }

    if(testToStart == TEST_FUNCTIONAL){
        currentStage = " phase Calibration";
        testToStart = TEST_CALIBRATION_PHASE;
        qDebug() << tag << "###    duration funcational  #########" << startTestTime.msecsTo(QDateTime::currentDateTime());
        startTestTime = QDateTime::currentDateTime();
//        QTimer::singleShot(4000, this, SLOT(startTest()));
        startTest(); // start calibration test.
        return;
    }



    powerSource->sendCommand("R;");
    if(testToStart == TEST_CALIBRATION_PHASE){
        currentStage = "Neutral calibration";
        testToStart = TEST_CALIBRATION_NEUTRAL;
        qDebug() << tag << "###    duration phase calibration  #########" << startTestTime.msecsTo(QDateTime::currentDateTime());
    }if(testToStart == TEST_CALIBRATION_NEUTRAL){
        currentStage = "Low Error";
        testToStart = TEST_lOW_ERROR;
        qDebug() << tag << "###    duration neutral calibration  #########" << startTestTime.msecsTo(QDateTime::currentDateTime());
    }else if(testToStart == TEST_lOW_ERROR){
        testToStart = TEST_HIGH_ERROR;
        currentStage = "High Error";
        qDebug() << tag << "###    duration  low error #########" << startTestTime.msecsTo(QDateTime::currentDateTime());
    }else if(testToStart == TEST_HIGH_ERROR){
        testToStart = TEST_STARTING_CURRENT;
        currentStage = "Starting Current";
        qDebug() << tag << "###    duration  high error #########" << startTestTime.msecsTo(QDateTime::currentDateTime());
    }else if(testToStart == TEST_STARTING_CURRENT){
        testToStart = TEST_NIC_SYNC;
        currentStage = "Nic Sync";
        qDebug() << tag << "###    duration  starting current #########" << startTestTime.msecsTo(QDateTime::currentDateTime());
    }else{
        testToStart = TEST_FINISH;
        currentStage = "";
        finishTest();
        qDebug() << tag << "###    duration  nic sync #########" << startTestTime.msecsTo(QDateTime::currentDateTime());
        qDebug()<<" test finsih ";
        qDebug() << tag << "###    duration total test duration   #########" << totalTestTime.msecsTo(QDateTime::currentDateTime());
        return;
    }
    QTimer::singleShot(6000, this, SLOT(setupPowerSource()));
}

void SPMBenchFullTest::finishTest(){
    for (int i = 0; i < 9; ++i) {
        if(meterTestStatus[i]==TEST_STATUS_IDEAL)
            meterTestStatus[i] = TEST_STATUS_PASS;
    }


    //savelog

    for (int i = 0; i < 9; ++i)
    {
        qDebug() << "--WatchMe--";
        auto meterInfo  = benchMeter[i];
        meterInfo->saveLogs();
    }

    emit meterColorChanged(6);

    if(autoTestOn){
        QTimer::singleShot(20000, this, SLOT(setupPowerSource()));
    }
}

void SPMBenchFullTest::setupPowerSource()
{
    if(testToStart == TEST_FINISH){
        testToStart = TEST_FUNCTIONAL;
        setAllTestResult(TEST_STATUS_IDEAL);
        currentStage = "Functional";
        totalTestTime = QDateTime::currentDateTime();
    }
    powerSource->setChannelState(7,true,testToStart);
}

void SPMBenchFullTest::powerStablised(quint8 channels)
{
    qDebug() << "SPMFullTest::powerStablised" << channels;
    if(testToStart == TEST_FUNCTIONAL){
        startTest();
        return;
    }
    if(testToStart == TEST_NIC_SYNC){
       QTimer::singleShot(6000, this, SLOT(startTest()));
    }else{
        QTimer::singleShot(4000, this, SLOT(startTest()));
    }
}

void SPMBenchFullTest::startTest(){
    startTestTime = QDateTime::currentDateTime();
    for (int i = 0; i < 9; ++i) {
        if (meterTestStatus[i] == TEST_STATUS_IDEAL) {
            benchMeter[i]->startTest(testToStart);
            meterTestStatus[i] = TEST_STATUS_RUNNING;
        }
    }
}

QColor SPMBenchFullTest::getMeterColor(int meterIndex) const
{
    switch (meterTestStatus[meterIndex]) {
    case TEST_STATUS_FAIL:
        return QColor("red");
    case TEST_STATUS_PASS:
        return QColor("green");
    case TEST_STATUS_IDEAL:
    default:
        return QColor("skyblue");
    }
}

QString SPMBenchFullTest::getMeterValue(int meterIndex)
{
    if(meterIndex==0){
        qDebug()<<errorDetail[0]<<" dfsfdsf";
    }
    return errorDetail[meterIndex];
}

QString SPMBenchFullTest::getFailStage(int meterIndex)
{
    return failStage[meterIndex];
}

int SPMBenchFullTest::getmeterSerialNo(int meterIndex)
{
    if(setSerialNo && testToStart == TEST_FINISH)
        return benchMeter[meterIndex]->meterFinalSerialNumber;
    else
        return -1;
}

void SPMBenchFullTest::setAllTestResult(TEST_RESULT result)
{
    for (int i = 0; i < 9; ++i) {
        meterTestStatus[i] = result;
        benchMeter[i]->functionTestResult.resetValue();
        benchMeter[i]->combinedCalibrationResult.resetValue();
        benchMeter[i]->lowCurrentTestResult.resetValue();
        benchMeter[i]->highCurrentTestResult.resetValue();
        benchMeter[i]->startingCurrentTestResult.resetValue();
        benchMeter[i]->nicSyncTestResult.resetValue();
        benchMeter[i]->meterFinalSerialNumber = 0;
        errorDetail[i]="";
        failStage[i] = "";
    }
    emit meterColorChanged(7);
}


