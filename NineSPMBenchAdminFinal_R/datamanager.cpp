#include "datamanager.h"
#include <QSettings>

DataManager::DataManager()
{

}

DataManager::~DataManager()
{
}

DataManager& DataManager::instance()
{
    static DataManager instance;
    return instance;
}

int DataManager::getCustomer() const
{
    return m_customer;
}

void DataManager::setCustomer(int customer)
{
    m_customer = customer;
}

QString DataManager::getNetworkModule() const
{
    return m_networkModule;
}

void DataManager::setNetworkModule(QString networkModule)
{
    m_networkModule = networkModule;
}

bool DataManager::getSetSerial() const
{
    return m_setSerial;
}

void DataManager::setSetSerial(bool setSerial)
{
    m_setSerial = setSerial;
}



float DataManager::getHighCurrent() const
{
    return m_highCurrent;
}

void DataManager::setHighCurrent(float highCurrent)
{
    m_highCurrent = highCurrent;
}

float DataManager::getLowCurrent() const
{
    return m_lowCurrent;
}

void DataManager::setLowCurrent(float lowCurrent)
{
    m_lowCurrent = lowCurrent;
}

float DataManager::getStartingCurrent() const
{
    return m_startingCurrent;
}

void DataManager::setStartingCurrent(float startingCurrent)
{
    m_startingCurrent = startingCurrent;
}

float DataManager::getSerialFrom() const
{
    return m_serialFrom;
}

void DataManager::setSerialFrom(float serialFrom)
{
    m_serialFrom = serialFrom;
}

float DataManager::getSerialTo() const
{
    return m_serialTo;
}

void DataManager::setSerialTo(float serialTo)
{
    m_serialTo = serialTo;
}

QString DataManager::getPrefix() const
{
    return m_prefix;
}

void DataManager::setPrefix(QString prefix)
{
    m_prefix = prefix;
}

float DataManager::getCurrentSerial()
{
    if(m_currentSerial>m_serialTo)
        return -1;
    QSettings setting;
    int currentserialInt = static_cast<int>(m_currentSerial);
    setting.setValue("currentSerialNo",currentserialInt+1);
    return m_currentSerial++;
}

void DataManager::setCurrentSerial(float currentSerial)
{
    m_currentSerial = currentSerial;
}
