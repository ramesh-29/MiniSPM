#include "benchmetertest.h"
#include <QDebug>
#include "datamanager.h"

BenchMeterTest::BenchMeterTest(QString port, QString meterNo, bool setSerialNo, EnCustomer customer, float highCurrent, float lowCurrent, float startingCurrent, QString networkModule, QObject *parent)
    : MeterInfo{true, parent}
{
    tag = meterNo;
    setSerial = setSerialNo;
    functionalTest = new FunctionTest(port,customer,meterNo);
    calibratePhase = new MeterCalibration(port, MeterCalibration::CALIBRATE_PHASE, this, "CaLPhase" + meterNo);
    calibrateNeutral = new MeterCalibration(port, MeterCalibration::CALIBRATE_NEUTRAL, this, "CaLNeutral" + meterNo);
    lowCurrentErrorTest = new MeterErrorTest(240, lowCurrent, 0.5, 60, port, this, "E1L "+ meterNo);
    highCurrentErrorTest = new MeterErrorTest(240, highCurrent, 0.5, 60, port, this, "E2H "+ meterNo);
    startingCurrentTest = new MeterErrorTest(240,startingCurrent,50,0,port,this,"SC "+meterNo);
    creepTest = new NicSyncTest(port,customer,meterNo,networkModule,setSerialNo);

    connect(functionalTest, SIGNAL(functionTestResult(FunctionTestResult*)), this, SLOT(fetchFunctionTestResult(FunctionTestResult*)));
    connect(calibratePhase, SIGNAL(calibrationTestResult(CalibrationResult*)), this, SLOT(fetchCalibrationPhaseTestResult(CalibrationResult*)));
    connect(calibrateNeutral, SIGNAL(calibrationTestResult(CalibrationResult*)), this, SLOT(fetchCalibrationNeutralTestResult(CalibrationResult*)));
    connect(lowCurrentErrorTest, SIGNAL(errorTestResult(ErrorTestResult*)), this, SLOT(fetchLowCurrentTestResult(ErrorTestResult*)));
    connect(highCurrentErrorTest, SIGNAL(errorTestResult(ErrorTestResult*)), this, SLOT(fetchHighCurrentTestResult(ErrorTestResult*)));
    connect(startingCurrentTest, SIGNAL(errorTestResult(ErrorTestResult*)), this, SLOT(fetchStartingCurrentTestResult(ErrorTestResult*)));
    connect(creepTest, SIGNAL(nicSycnTestResult(NicSyncTestResult*)), this, SLOT(fetchNicSycnTestResult(NicSyncTestResult*)));

}



void BenchMeterTest::startTest(TEST_START testToStart)
{
    qDebug() << tag << "startTest";
    if(testToStart == TEST_FUNCTIONAL){
        functionalTest->startTest();
    }else if(testToStart == TEST_CALIBRATION_PHASE){
        calibratePhase->calibrate();
    }else if(testToStart == TEST_CALIBRATION_NEUTRAL){
            calibrateNeutral->calibrate();
    }else if(testToStart == TEST_lOW_ERROR){
        lowCurrentErrorTest->startErrorTest();
    }
    else if(testToStart == TEST_HIGH_ERROR){
        highCurrentErrorTest->startErrorTest();
    }else if(testToStart == TEST_STARTING_CURRENT){
        startingCurrentTest->startErrorTest();
    }else{
        creepTest->startTest();
    }
}



void BenchMeterTest::fetchCalibrationPhaseTestResult(CalibrationResult *result)
{

    calibrationStatus = result->isTestPass ? TEST_STATUS_PASS : TEST_STATUS_FAIL;

    qDebug() << tag << "PhaseCalibrationTestResult" << result->isTestPass;
    setPhaseCalibrationTestResult(*result);
    emit testFinish(calibrationStatus,result->errorDetails);
}

void BenchMeterTest::fetchCalibrationNeutralTestResult(CalibrationResult *result)
{

    calibrationStatus = result->isTestPass ? TEST_STATUS_PASS : TEST_STATUS_FAIL;

    qDebug() << tag << "NeutralCalibrationTestResult" << result->isTestPass;
    setNeutralCalibrationTestResult(*result);
    emit testFinish(calibrationStatus,result->errorDetails);
}


void BenchMeterTest::fetchLowCurrentTestResult(ErrorTestResult *result)
{

    lowCurrentErrorTestStatus = result->isTestPass ? TEST_STATUS_PASS : TEST_STATUS_FAIL;
    qDebug() << tag << "lowCurrentTestResult" << result->isTestPass;

    setLowCurrentTestResult(*result);
    emit testFinish(lowCurrentErrorTestStatus,result->errorDetails);
}

void BenchMeterTest::fetchHighCurrentTestResult(ErrorTestResult *result)
{

    highCurrentErrorTestStatus = result->isTestPass ? TEST_STATUS_PASS : TEST_STATUS_FAIL;

    qDebug()<< tag << "highCurrentTestResult" << result->isTestPass;

    setHighCurrentTestResult(*result);
    emit testFinish(highCurrentErrorTestStatus,result->errorDetails);
}

void BenchMeterTest::fetchStartingCurrentTestResult(ErrorTestResult *result)
{
    startingCurrentTestStatus = result->isTestPass ? TEST_STATUS_PASS : TEST_STATUS_FAIL;

    qDebug()<< tag << "startingTestResult" << result->isTestPass;

    setStartingCurrentTestResult(*result);
    emit testFinish(startingCurrentTestStatus,result->errorDetails);
}

void BenchMeterTest::fetchFunctionTestResult(FunctionTestResult *result)
{
    functionalTestStatus = result->isTestPass ? TEST_STATUS_PASS : TEST_STATUS_FAIL;

//    this->meterFinalSerialNumber = result->meterSerial.toInt();
//    qDebug()<< tag << "functionalTestResult" << result->isTestPass << " meter serial "<<result->meterSerial;

    setFunctionTestResult(*result);
    emit testFinish(functionalTestStatus,result->errorDetails);
}

void BenchMeterTest::fetchNicSycnTestResult(NicSyncTestResult *result)
{
    nicSyncTestStatus = result->isTestPass ? TEST_STATUS_PASS : TEST_STATUS_FAIL;

    if(setSerial){
        this->meterFinalSerialNumber = result->meterSerialNo.toInt();
        qDebug()<< tag << "nicSyncTestResult" << result->isTestPass<< " meter serial "<<result->meterSerialNo;
    }
    setNicSyncTestResult(*result);
    emit testFinish(nicSyncTestStatus,result->errorDetails);
}



