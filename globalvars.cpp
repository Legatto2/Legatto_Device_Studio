#include "globalvars.h"

QString GlobalVars::legacySettingsFilePath="";

QString GlobalVars::meta_imei_1[8] = {"", "", "", "", "", "", "", ""};
QString GlobalVars::meta_imei_2[8] = {"", "", "", "", "", "", "", ""};
QString GlobalVars::meta_serialPortName[8] = {"", "", "", "", "", "", "", ""};
int GlobalVars::meta_serialIndex[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int GlobalVars::meta_modelIndex[8] = {0, 0, 0, 0, 0, 0, 0, 0};
QString GlobalVars::meta_modelStr[8] = {"", "", "", "", "", "", "", ""};
QString GlobalVars::meta_projectName[8] = {"", "", "", "", "", "", "", ""};
QString GlobalVars::meta_projectDataType[8] = {"", "", "", "", "", "", "", ""};
QString GlobalVars::meta_projectUnlockType[8] = {"", "", "", "", "", "", "", ""};
bool GlobalVars::meta_dual_imei_bool[8] = {false, false, false, false, false, false, false, false};
bool GlobalVars::meta_manual_imei_bool[8] = {false, false, false, false, false, false, false, false};
bool GlobalVars::meta_unLock_bool[8] = {false, false, false, false, false, false, false, false};
bool GlobalVars::meta_MDM_bool[8] = {false, false, false, false, false, false, false, false};
bool GlobalVars::meta_reboot_bool[8] = {false, false, false, false, false, false, false, false};


QString GlobalVars::spd_imei_1 = "";
QString GlobalVars::spd_imei_2 = "";
int GlobalVars::spd_modelIndex = 0;
QString GlobalVars::spd_modelStr = "";
bool GlobalVars::spd_dual_imei_bool = false;
bool GlobalVars::spd_manual_imei_bool = false;
bool GlobalVars::spd_reboot_bool = false;
bool GlobalVars::spd_AssignPort_bool = false;
QString GlobalVars::spd_projectName = "";
QString GlobalVars::spd_projectDataType = "";

QList<QSerialPortInfo> GlobalVars::availablePortsList;

QColor GlobalVars::txtOutPutColor = QColor::fromRgb(255, 153, 0);


QString GlobalVars::authorizedToken = "";
float GlobalVars::userInfo_creditDetails=0.0;
QString GlobalVars::userInfo_HardwareKey = "";
QString GlobalVars::userInfo_UserName = "";
float GlobalVars::operationID = 0.0;




QString GlobalVars::api_MTKcharges = QString("https://portal.m-key.org/api/MTK/charge");
QString GlobalVars::api_MTKimeiGenerator = QString("https://portal.m-key.org/api/MTK/imei");
QString GlobalVars::api_MTKserverAuthenticity = QString("https://portal.m-key.org/api/MTK/auth");
QString GlobalVars::api_logURLQString = QString("https://portal.m-key.org/api/login");
QString GlobalVars::api_serverStatusQString = QString("https://portal.m-key.org/api/status");
QString GlobalVars::api_VersionQString = QString("https://portal.m-key.org/api/version");
QString GlobalVars::api_LogQString = QString("https://portal.m-key.org/api/log");
QString GlobalVars::api_PerformQString = QString("https://portal.m-key.org/api/perform");

QString GlobalVars::serverSoftwareVersion = "";



GlobalVars::GlobalVars()
{

}

void GlobalVars::initalizeToZero()
{

    GlobalVars::spd_imei_1 = "";
    GlobalVars::spd_imei_2 = "";
    GlobalVars::spd_modelIndex = 0;
    GlobalVars::spd_modelStr = "";
    GlobalVars::spd_projectName = "";
    GlobalVars::spd_projectDataType = "";
    GlobalVars::spd_dual_imei_bool = false;
    GlobalVars::spd_reboot_bool = false;
    GlobalVars::spd_AssignPort_bool = false;
    GlobalVars::availablePortsList.clear();

    GlobalVars::txtOutPutColor = QColor::fromRgb(255, 153, 0);

    GlobalVars::legacySettingsFilePath=QString(QDir::homePath()+"/legatto-device-studio-legacy-settings");
}
