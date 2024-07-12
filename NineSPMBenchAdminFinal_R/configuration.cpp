#include "configuration.h"
#include "ui_configuration.h"

#include <QTimer>
#include <QtDebug>
#include <QDateTime>
#include <QMessageBox>
#include <QSettings>
#include <QFile>
#include "meterkeys.h"
#include "datamanager.h"

Configuration::Configuration(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Configuration)
{
    ui->setupUi(this);
    QSettings setting;
    ui->leCurrentSerial->setText(setting.value("currentSerialNo").toString());
    ui->leSerialTo->setText(setting.value("serialTo").toString());
    ui->leSerialFrom->setText(setting.value("serialFrom").toString());
    ui->lePrefix->setText(setting.value("prefix").toString());


    connect(ui->cbCustomer, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Configuration::on_cbCustomer_currentIndexChanged);
    connect(ui->cbHighCurrent, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Configuration::on_cbHighCurrent_currentIndexChanged);
    setWindowTitle("User Configurations");

    fillValues();
    refresh();
}

Configuration::~Configuration()
{
    delete ui;
}
QString enCustomerToString(EnCustomer customer)
{
    switch (customer)
    {
    case CUST_APDCL:
        return "CUST_APDCL";
    case CUST_INTELLI:
        return "CUST_INTELLI";
    case CUST_JAMMU:
        return "CUST_JAMMU";
    case CUST_KASHMIR:
        return "CUST_KASHMIR";
    case CUST_PANASONIC:
        return "CUST_PANASONIC";
    case CUST_APARVA:
        return "CUST_APARVA";
    case CUST_MONTE:
        return "CUST_MONTE";
    case CUST_DEMO:
        return "CUST_DEMO";
    case CUST_OTHER:
        return "CUST_OTHER";
    case CUST_MAHARASTRA:
        return "CUST_MAHARASTRA";
    case CUST_MAHARASTRA_PKG09:
        return "CUST_MAHARASTRA_PKG09";
    case CUST_DGVCL:
        return "CUST_DGVCL";
    case CUST_DGVCL_6:
        return "CUST_DGVCL_6";
    case CUST_DGVCL_12:
        return "CUST_DGVCL_12";
    case CUST_TATA_POWER:
        return "CUST_TATA_POWER";
    case CUST_PURBANCHAL:
        return "CUST_PURBANCHAL";
    case CUST_INTELLISMART:
        return "CUST_INTELLISMART";
    case CUST_NOT_FOUND:
        return "CUST_NOT_FOUND";
    default:
        return ""; // Default case should not happen, but return an empty string to handle it
    }
}
void Configuration::fillValues()
{
    for (int i = 0; i <= 17; ++i)
        {
            EnCustomer customer = static_cast<EnCustomer>(i);
            ui->cbCustomer->addItem(enCustomerToString(customer));
        }
    //high current
    ui->cbHighCurrent->addItem("30");
    ui->cbHighCurrent->addItem("60");

    //low current    return true;
    ui->cbLowCurrent->addItem("1");
    ui->cbLowCurrent->addItem("5");


    //low current    return true;
    ui->cbNetworkModule->addItem("RF");
    ui->cbNetworkModule->addItem("4G");

    ui->cbSetSerial->addItem("false");
    ui->cbSetSerial->addItem("true");


    //cali
//    ui->cbCalibration->addItem("Single");
//    ui->cbCalibration->addItem("Double");
}

bool Configuration::validate()
{
    QSettings setting;
    setting.setValue("currentSerialNo",ui->leCurrentSerial->text());
    setting.setValue("serialTo",ui->leSerialTo->text());
    setting.setValue("serialFrom",ui->leSerialFrom->text());
    setting.setValue("prefix",ui->lePrefix->text());
    //user config
    QString nameplate = ui->leNameplate->text();
    QString internalFirmwareVersion = ui->leFirmwareInternal->text();
    QString rfVersion = ui->leRFVersion->text();
    QString prefix = ui->lePrefix->text();
    float currentSerial = ui->leCurrentSerial->text().toFloat();
    float serialTo = ui->leSerialTo->text().toFloat();
    float serialFrom = ui->leSerialFrom->text().toFloat();
    QString netModule = ui->cbNetworkModule->currentText();
    QString setSerialValid = ui->cbSetSerial->currentText();
    float highCurrent = ui->cbHighCurrent->currentText().toFloat();
    float lowCurrent = ui->cbLowCurrent->currentText().toFloat();
    float startingCurrent = ui->leStartingCurrent->text().toFloat();
    int selectedIndex = ui->cbCustomer->currentIndex();

    DataManager& dataManager = DataManager::instance();
    dataManager.setCustomer(selectedIndex);
    dataManager.setHighCurrent(highCurrent);
    dataManager.setLowCurrent(lowCurrent);
    dataManager.setStartingCurrent(startingCurrent);
    dataManager.setNetworkModule(netModule);
    dataManager.setPrefix(prefix);
    dataManager.setCurrentSerial(currentSerial);
    dataManager.setSerialFrom(serialFrom);
    dataManager.setSerialTo(serialTo);
    qDebug()<<startingCurrent<<" "<<highCurrent<<" "<<lowCurrent;
    if(setSerialValid == "true"){
      dataManager.setSetSerial(true);
    }else{
        dataManager.setSetSerial(false);
    }

    //spm config
//    QString serialFrom = ui->leSerialFrom->text();
//    QString serialTo = ui->leSerialTo->text();
//    QString loa = ui->leLOA->text();
//    QString loa2 = ui->leLOA2->text();

//    bool validated = !(serialFrom.isEmpty() || serialTo.isEmpty() || loa.isEmpty() || loa2.isEmpty());
//    validated &= !(nameplate.isEmpty() || internalFirmwareVersion.isEmpty() || rfVersion.isEmpty() || highCurrent <= 0);

    bool validated = !(nameplate.isEmpty() || internalFirmwareVersion.isEmpty() || rfVersion.isEmpty() || highCurrent <= 0);
    return validated;
}

void Configuration::on_cbCustomer_currentIndexChanged(int index)
{
    qDebug() << "Index changed to:" << index;
    EnCustomer customer;
    customer = static_cast<EnCustomer>(index);
    ui->leNameplate->setText(MeterKeys().GetFirmwareInternalNameplateList[customer]);
    ui->leFirmwareInternal->setText(MeterKeys().GetFirmwareInternalVersionList[customer]);
    ui->leRFVersion->setText(MeterKeys().GetRFVersionList[customer]);
}

void Configuration::on_cbHighCurrent_currentIndexChanged(int index)
{
    if(index == 1){
        ui->leStartingCurrent->setText("0.04");
    }else{
       ui->leStartingCurrent->setText("0.02");
    }
}

void Configuration::on_btnSubmit_clicked()
{
    if(!validate())
    {
        qDebug () << " please check field ";
        QMessageBox::critical(this, "Error", "missing values");
        return;
    }

    //save spm setting from UI

    this->refresh();
    QMessageBox::information(this, "Success", "Key Changed");
    emit submitClicked();
}

void Configuration::refresh()
{
    //load spm settings in UI
}

void Configuration::on_cbSetSerial_currentIndexChanged(int index)
{
    if(index == 0){
        ui->lePrefix->setEnabled(false);
        ui->leCurrentSerial->setEnabled(false);
        ui->leSerialTo->setEnabled(false);
        ui->leSerialFrom->setEnabled(false);
    }else{
        ui->lePrefix->setEnabled(true);
        ui->leCurrentSerial->setEnabled(true);
        ui->leSerialTo->setEnabled(true);
        ui->leSerialFrom->setEnabled(true);
    }
}
