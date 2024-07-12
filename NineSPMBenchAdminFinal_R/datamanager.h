#ifndef DATAMANAGER_H
#define DATAMANAGER_H
#include <QString>

class DataManager
{
public:
    static DataManager& instance();

    int getCustomer() const;
     void setCustomer(int customer);

     QString getNetworkModule() const;
      void setNetworkModule(QString networkModule);

      bool getSetSerial() const;
       void setSetSerial(bool setSerial);

     float getHighCurrent() const;
     void setHighCurrent(float highCurrent);

     float getLowCurrent() const;
     void setLowCurrent(float lowCurrent);

     float getStartingCurrent() const;
     void setStartingCurrent(float startingCurrent);

     float getSerialFrom() const;
     void setSerialFrom(float serialFrom);

     float getSerialTo() const;
     void setSerialTo(float serialTo);

     QString getPrefix() const;
      void setPrefix(QString prefix);

     float getCurrentSerial();
     void setCurrentSerial(float currentSerial);
private:
    DataManager();
    ~DataManager();

    float m_customer;
    float m_highCurrent;
    float m_lowCurrent;
    float m_startingCurrent;
    float m_serialFrom;
    float m_serialTo;
    QString m_networkModule;
    QString m_prefix;
    float m_currentSerial;
    bool m_setSerial;

};

#endif // DATAMANAGER_H
