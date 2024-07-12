#ifndef METERKEYS_H
#define METERKEYS_H

#include <QObject>

enum EnCustomer
{
    CUST_APDCL,
    CUST_INTELLI,
    CUST_JAMMU,
    CUST_KASHMIR,
    CUST_PANASONIC,
    CUST_APARVA,
    CUST_MONTE,
    CUST_DEMO,
    CUST_OTHER,
    CUST_MAHARASTRA,
    CUST_MAHARASTRA_PKG09,
    CUST_DGVCL,
    CUST_DGVCL_6,
    CUST_DGVCL_12,
    CUST_TATA_POWER,
    CUST_PURBANCHAL,
    CUST_INTELLISMART,
    CUST_NOT_FOUND,
};

enum EnNetworkCard
{
    MODULE_RF,
    MODULE_4G,
    MODULE_NOT_FOUND
};

class MeterKeys
{
public:
    static constexpr int METER_NUMBER_LENGTH = 9;
private:
       EnCustomer customer = EnCustomer::CUST_JAMMU ;

public:
     void SetCustomer(EnCustomer newCustomer)
    {
        customer = newCustomer;
    }

     EnCustomer GetCustomer()
    {
        return customer;
    }

     QList<QString> GetHLSKeyList={"ASSOCIATION_UTST", "HLSKEY_INTELLI01", "HLSKEY_JANKASPDC", "HLSKEY_JANKASPDC", "HLSKEY_PANASONIC", "AparvaHIGHCcurHL", "MCarloHIGHAsocHL", "aaaaaaaaaaaaaaaa", "aaaaaaaaaaaaaaaa", "NCCMSEDCLcryp_HL", "NCCMSEDCLcryp_HL", "HLSKEY_GPDGVSPDC", "HLSKEY_GPDGVSPDC","HLSKEY_GPDGVSPDC","tatapwHIGHCcurHL", "purvanchalaiibHL","IntelliPk7AsocHL",""};
     QList<QString> GetAuthenticationKeyList = {"ENCRYPTION_UNICT", "AUTHENKEY_INTELI", "AUTHENKEY_JANKA1", "AUTHENKEY_JANKA1", "ENCRYP_PANASONIC", "AparvNcrypCcurEN", "MCarloncrypcurEN", "AAAAAAAAAAAAAAAA", "AAAAAAAAAAAAAAAA", "NCCMSEDCLcryp_EN", "NCCMSEDCLcryp_EN", "DghlkNcrypCcurEN","DghlkNcrypCcurEN", "DghlkNcrypCcurEN","TaTaPNcrypCcurEN","purvanchalaiibEN","InteliPk7APDCLEN", ""};
     QString GetHLSKey()
     {
         return GetHLSKeyList[GetCustomer()];
     }

     QList<QString> GetEncryptionKeyList={"ENCRYPTION_UNICT","AUTHENKEY_INTELI","AUTHENKEY_JANKA1","AUTHENKEY_JANKA1","ENCRYP_PANASONIC","AparvNcrypCcurEN", "MCarloncrypcurEN", "AAAAAAAAAAAAAAAA","AAAAAAAAAAAAAAAA","NCCMSEDCLcryp_EN", "NCCMSEDCLcryp_EN", "DghlkNcrypCcurEN","DghlkNcrypCcurEN", "DghlkNcrypCcurEN","TaTaPNcrypCcurEN", "purvanchalaiibEN","InteliPk7APDCLEN",""};
     QString GetEncryptionKey()
     {
         return GetEncryptionKeyList[GetCustomer()];
     }
     QString GetAuthenticationKey()
     {
         return GetAuthenticationKeyList[GetCustomer()];
     }
     QList<int> GetNWChannelList = {0x06, 0x03, 0x4, 0x4, 0x05,  0x02, 0x01, 0x01,0x06, 0x02, 0x05, 0x05, 0x05,0x05, 0,0x05, 0x02,0 };
     int GetNWChannel()
     {
        return GetNWChannelList[GetCustomer()];
     }
     QList<qint32> GetNWAddressList = {0X504F57, 0x302D35, 0x403C92, 0x403C92, 0x302D30, 0x252A25, 0x505F00, 0x223344, 0X504F57, 0x505F06, 0x505F08, 0x854936,0x854936,0x854936, 0,0x282D30,0x505F02, 0 }; //505F08

     qint32 GetNWAddress()
     {
        return GetNWAddressList[GetCustomer()];
     }
     QList<QString> GetRFSecurityKeyList = {"ENCRYPTION_RFNET",  "RFENCRYPT_INTELI",  "RFENCRYPT_JNKNET", "RFENCRYPT_JNKNET",
                                            "RFENCRYPT_PANASC","RFENCRYPT_APARVA","ENCRY_MONTECARLO","RFENCRYPT_DEMO12", "ENCRYPTION_RFNET", "ENCRY_NCC_MSEDCL", "ENPK9_NCC_MSEDCL", "ENCRYPTRF_GDGVCL","ENCRYPTRF_GDGVCL","ENCRYPTRF_GDGVCL", "","RFPURV_SCHNEIDER", "ENCRY_INTELLIPK7","" };
     QString GetRFSecurityKey()
     {
         return GetRFSecurityKeyList[GetCustomer()];
     }

     QList<QString> GetFirmwareInternalVersionList = {"",  "",  "JK000.01.03", "", "", "AR000.01.03", "MC000.01.03", "", "", "NCMH0.01.08", "NCPK9.01.11", "DGV10.01.00", "DGV10.01.06","DGV10.01.12","SK0300.00", "AIBP0.01.17","IPK70.01.18","" };
     QString GetFirmwareInternalVersion()
     {
         return GetFirmwareInternalVersionList[GetCustomer()];
     }

     QList<QString> GetFirmwareInternalNameplateList = {"",  "",  "TEST 1.22", "", "", "TEST 1.22", "TEST 1.22", "", "", "TEST 1.22", "TEST 1.22", "TEST 1.22","TEST 1.22", "TEST 1.22","KU1 01.24", "KU1 01.24","KU1 01.24","" };
     QString GetFirmwareVersionNameplate()
     {
         return GetFirmwareInternalNameplateList[GetCustomer()];
     }

     QList<QString> GetRFVersionList = {"",  "",  "11.11.2.3:13", "", "", "11.11.2.5:13", "", "", "", "16.2.2.14:23", "16.2.4.15:23", "16.3.17.11:23", "16.3.17.11:23","16.3.15.11:23", "10.0.0.8", "11.3.32.10:13","16.3.41.13:23","" };
     QString GetRFVersion()
     {
         return GetRFVersionList[GetCustomer()];
     }

     QList<QString> GetBarcodeFormatList = {"",  "",  "", "", "", "", "", "", "", "", "", "", "","","SU 1P %1 %2", "" };
     QString GetBarcodeFormat()
     {
         return GetBarcodeFormatList[GetCustomer()];
     }
};

#endif // METERKEYS_H
