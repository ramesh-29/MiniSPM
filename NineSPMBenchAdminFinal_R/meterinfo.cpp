#include "meterinfo.h"
#include <QDebug>
#include <QJsonDocument>

MeterInfo::MeterInfo(bool isValid, QObject* parent) : QObject(parent)
{
    this->isValid = isValid;
    failedAt = QString();
}

void MeterInfo::setFunctionTestResult(FunctionTestResult &functionTestResult)
{
//    functionTestResult.isTestPass=true;
    this->functionTestResult = functionTestResult;
    //force pass ft to check power stablisation at ATMA MTE
//    functionTestResult.isTestPass = true;

    if(!functionTestResult.isTestPass){
        failedAt += "Function Test";
        errorDetails = functionTestResult.errorDetails;
    }
}

void MeterInfo::setMeggarTestResult(TestResult &testResult)
{
    this->megarTestResult = testResult;

    if(!testResult.isTestPass)
        failedAt += "Meggar";
}

void MeterInfo::setHVTestResult(TestResult &testResult)
{
    this->hvTestResult = testResult;

    if(!testResult.isTestPass)
        failedAt += "HV";
}

void MeterInfo::setCalibrationTestResult(CalibrationResult &calibration)
{
    this->combinedCalibrationResult = calibration;

    if(!calibration.isTestPass){
        failedAt += "Calibration";
        errorDetails = calibration.errorDetails;
    }
}

void MeterInfo::setPhaseCalibrationTestResult(CalibrationResult &calibration)
{
//    calibration.isTestPass=true;
    this->phaseCalibrationResult = calibration;

    if(!calibration.isTestPass){
        failedAt += "Phase Calibration";
        errorDetails = calibration.errorDetails;
    }
}

void MeterInfo::setNeutralCalibrationTestResult(CalibrationResult &calibration)
{
//    calibration.isTestPass=true;
    this->neutralCalibrationResult = calibration;

    if(!calibration.isTestPass){
        failedAt += "Neutral Calibration";
        errorDetails = calibration.errorDetails;
    }
}

void MeterInfo::setStartingCurrentTestResult(ErrorTestResult errorTest)
{
    errorTest.isTestPass = true;
    this->startingCurrentTestResult = errorTest;
    if(!errorTest.isTestPass){
        failedAt += "Starting Current";
        errorDetails = errorTest.errorDetails;
    }
}

void MeterInfo::setLowCurrentTestResult(ErrorTestResult errorTest)
{
//    errorTest.isTestPass = true;
    this->lowCurrentTestResult = errorTest;
    //force pass

    if(!errorTest.isTestPass){
        failedAt += "Low Current";
        errorDetails = errorTest.errorDetails;
    }
}

void MeterInfo::setHighCurrentTestResult(ErrorTestResult errorTest)
{
//    errorTest.isTestPass = true;
    this->highCurrentTestResult = errorTest;
    //force pass

    if(!errorTest.isTestPass){
        failedAt += "High Current";
        errorDetails = errorTest.errorDetails;
    }
}

void MeterInfo::setLaserEngraveResult(LaserEngraveResult testResult)
{
    testResult.isTestPass=true;
    this->laserEngraveResult = testResult;

    qDebug() << "LA" << "R" << " 2 testResult->isTestPass" << testResult.isTestPass << laserEngraveResult.isTestPass;
    if(!laserEngraveResult.isTestPass)
        failedAt += "Laser";
}

void MeterInfo::setNicSyncTestResult(NicSyncTestResult nicSyncTest)
{
//    nicSyncTest.isTestPass=true;
    this->nicSyncTestResult = nicSyncTest;

    qDebug() << "NS" << "R" << " 2 setNicSyncTestResult->isTestPass" << nicSyncTestResult.isTestPass;
    if(!nicSyncTestResult.isTestPass){
        failedAt += "Nic Sync";
        errorDetails = nicSyncTest.errorDetails;
    }
}

void MeterInfo::setDLMSTestResult(NicSyncTestResult nicSyncTest)
{

}

QVariantMap MeterInfo::saveLogs()
{
    QVariantMap testResults;
//    testResults["meterTmpSerialNumber"] = meterTmpSerialNumber;
    testResults["meterFinalSerialNumber"] = meterFinalSerialNumber;
    testResults["failedAt"] = failedAt;
    testResults["functionTestResult"] = functionTestResult.getResult();
//    testResults["hvTestResult"] = hvTestResult.getResult();
//    testResults["megarTestResult"] = megarTestResult.getResult();
    testResults["combinedCalibration"] = combinedCalibrationResult.getResult();
//    testResults["phaseCalibration"] = phaseCalibrationResult.getResult();
//    testResults["neutralCalibration"] = neutralCalibrationResult.getResult();
    testResults["startingCurrentTestResult"] = startingCurrentTestResult.getResult();
    testResults["lowCurrentTestResult"] = lowCurrentTestResult.getResult();
    testResults["highCurrentTestResult"] = highCurrentTestResult.getResult();
//    testResults["laserEngraveResult"] = laserEngraveResult.getResult();
    testResults["nicSyncTestResult"] = nicSyncTestResult.getResult();

    QJsonDocument jsonDocument = QJsonDocument::fromVariant(testResults);
    QString jsonString = jsonDocument.toJson(QJsonDocument::Compact);

    qDebug() << jsonString.toStdString().c_str();
//    qDebug() << QString::fromQJsonDocument::fromVariant(testResults).toJson();
    return testResults;
}

TestResult::TestResult()
{
    resetValue();
}

void TestResult::resetValue()
{
    isTestPass = false;
    testDuration = 0;
    lastState.clear();
    log.clear();
    errorDetails.clear();
}

QVariantMap TestResult::getResult()
{
    QVariantMap variantMap;

    variantMap["isTestPass"] = isTestPass;
    variantMap["lastState"] = lastState;
    variantMap["errorDetails"] = errorDetails;
    variantMap["startTestTime"] = startTestTime;
    variantMap["endTestTime"] = endTestTime;
    variantMap["testDuration"] = testDuration;
    return variantMap;
}

ErrorTestResult::ErrorTestResult()
{
    resetValue();
}

void ErrorTestResult::resetValue()
{
    TestResult::resetValue();

    meterEnergyWH = 0;
    startingCumlativeEnergy = 0;
    phaseEnergyVAH=0;
    neutralEnergyVAH=0;
    phaseReactiveEnergy = 0;
    neutralReactiveEnergy = 0;
    phaseWattHour = 0;
    neutralWattHour = 0;
    cumulativeEnergy = 0;
    testVoltage = 0;
    testCurrent = 0;
    timeMS = 0;
    theoryEnergy = 0;
    powerWH = 0;
}

QVariantMap ErrorTestResult::getResult()
{
    QVariantMap variantMap;

    variantMap["meterEnergyWH"] = meterEnergyWH;
    variantMap["startingCumlativeEnergy"] = startingCumlativeEnergy;
    variantMap["phaseEnergy"] = phaseEnergyVAH;
    variantMap["neutralEnergy"] = neutralEnergyVAH;
    variantMap["phaseReactiveEnergy"] = phaseReactiveEnergy;
    variantMap["neutralReactiveEnergy"] = neutralReactiveEnergy;
    variantMap["phaseWattHour"] = phaseWattHour;
    variantMap["neutralWattHour"] = neutralWattHour;
    variantMap["cumulativeEnergy"] = cumulativeEnergy;
    variantMap["testVoltage"] = testVoltage;
    variantMap["testCurrent"] = testCurrent;
    variantMap["timeMS"] = timeMS;
    variantMap["theoryEnergy"] = theoryEnergy;
    variantMap["powerWH"] = powerWH;
    variantMap["common"] = TestResult::getResult();

    return variantMap;
}

LaserEngraveResult::LaserEngraveResult() : TestResult()
{

}

void LaserEngraveResult::resetValue()
{
    serialNumber.clear();
    loaNumber.clear();
    loaNumber2.clear();
    mfgDate.clear();
}

QVariantMap LaserEngraveResult::getResult()
{
    QVariantMap map;

    map["serialNumber"] = serialNumber;
    map["loaNumber"] = loaNumber;
    map["loaNumber2"] = loaNumber2;
    map["mfgDate"] = mfgDate;
    map["common"] = TestResult::getResult();
    return map;
}

void FunctionTestResult::resetValue()
{
    TestResult::resetValue();

    memset(&result, 0 , sizeof(FunctionTestInfo));
    pcbNumber = 0;
    voltage = 0;
    phaseCurrent = 0;
    neutralCurrent = 0;
    rtcDriftSecond = 0;
}

QVariantMap FunctionTestResult::getResult()
{
    QVariantMap map;
    QVariantMap subTests;

    map["coverClosed"] = result.coverClosed ? "False" : "True";
    map["pushButton"] = result.pushButton ? "False" : "True";
    map["magnetStat1"] = result.magnetStat1 ? "False" : "True";
    map["magnetStat2"] = result.magnetStat2 ? "False" : "True";
    map["phaseCurrentOff"] = result.phaseCurrentOff ? "False" : "True";
    map["neutralCurrentOff"] = result.neutralCurrentOff ? "False" : "True";
    map["phaseCurrentOn"] = result.phaseCurrentOn ? "False" : "True";
    map["neutralCurrentOn"] = result.neutralCurrentOn ? "False" : "True";

    subTests["functionTest"] = map;

    map.clear();
    map["pcbNumber"] = pcbNumber;
    subTests["pcbNumber"] = map;

    map.clear();
    map["voltage"] = voltage;
    map["phaseCurrent"] = phaseCurrent;
    map["neutralCurrent"] = neutralCurrent;
    subTests["vi"] = map;

    map.clear();
    map["rtcDriftSecond"] = rtcDriftSecond;
    subTests["rtc"] = map;
    subTests["common"] = TestResult::getResult();

    return subTests;
}

QVariantMap CalibrationResult::getResult()
{
    QVariantMap map;

    //
    return TestResult::getResult();

//    return map;
}

NicSyncTestResult::NicSyncTestResult()
{
    resetValue();
}

QVariantMap NicSyncTestResult::getResult()
{
    QVariantMap map;

    map["meterMemoryNumber"] = meterMemoryNumber;
    map["meterNumberPrint"] = meterNumberPrint;
    map["netowrkAddress"] = networkAddress;
    map["channelAddrss"] = networkChannel;
    map["rfKey"] = rfKey;

    map["noLoadPhaseCurrent"] = noLoadPhaseCurrent;
    map["noLoadNeutralCurrent"] = noLoadNeutralCurrent;
    map["common"] = TestResult::getResult();

    return map;
}

void NicSyncTestResult::resetValue()
{
    meterMemoryNumber.clear();
    meterNumberPrint.clear();
    meterQRNumber.clear();

    rfKey.clear();

    networkAddress = 0;
    networkChannel = 0;
    noLoadPhaseCurrent = 0;
    noLoadNeutralCurrent = 0;

    TestResult::resetValue();
}
