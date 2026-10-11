#include "backendclass.h"
#include "securityhelpers.h"

BackEndClass::BackEndClass(QObject *parent) : QObject(parent)
{
    signal.currentTool = Tool_Idle;
    signal.toolIndex = 1;
    signal.outputString= "";
    signal.outputBold=false;
    signal.outputNewLine=false;
    signal.needtoInsertOutputBox=false;
    signal.outputColor=GlobalVars::txtOutPutColor;
    signal.progressBarValue=0;
    signal.isProcessCompleted=false;
    signal.upload_LogHistory = false;

    timer_SPDTool = new QTimer(this);
    timer_SPDTool->setSingleShot(true);
    connect(timer_SPDTool, SIGNAL(timeout()), this, SLOT(rx_timer_SPDTool_elapsed()));
    singleShotIndex = 0;


    timer_SingleShot = new QTimer(this);
    timer_SingleShot->setSingleShot(true);
    connect(timer_SingleShot, SIGNAL(timeout()), this, SLOT(rx_timer_singleShot_elapsed()));


    QDir chckRequiredFolder(m3FolderPath);
    if(!chckRequiredFolder.exists()){
        QMessageBox::critical(nullptr, "M3 Error", "Please Install the Mototool Software.", QMessageBox::Ok);
        qDebug()<<" Folder not exist: "<<m3FolderPath;
        qDebug()<<" Folder not exist: "<<adbFolderPath;
        qDebug()<<" Folder not exist: "<<ChinoeIMEICMToolFolderPath_Data1;
    }
    chckRequiredFolder.setPath(adbFolderPath);
    if(!chckRequiredFolder.exists()){
        QMessageBox::critical(nullptr, "ADB Error", "Please Install the ADB Software.", QMessageBox::Ok);
    }
    chckRequiredFolder.setPath(ChinoeIMEICMToolFolderPath_Data1);
    if(!chckRequiredFolder.exists()){
        QMessageBox::critical(nullptr, "ADB Chinoe Error", "Please Install the Chineo Software.", QMessageBox::Ok);
    }
    chckRequiredFolder.setPath(fastBootFolderPath);
    if(!chckRequiredFolder.exists()){
        chckRequiredFolder.mkdir(fastBootFolderPath);
    }

    //qDebug()<<" M3: "<<m3FolderPath;
    //qDebug()<<" ADB: "<<adbFolderPath;



    adbProcess = new QProcess(this);
    adbProcess->setProgram(cmdShellPath);
    adbProcess->setWorkingDirectory(adbFolderPath);

    connect(adbProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(rx_ADB_OutPutReceived()));
    //connect(adbProcess, SIGNAL(finished(int)), this, SLOT(rx_ADB_OutPutReceived()));
    connect(adbProcess, SIGNAL(readyReadStandardError()), this, SLOT(rx_ADB_ErrorReceived()));
    connect(adbProcess, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(rx_ADB_ErrorReceived()));
}

void BackEndClass::rx_StartRepairing(int idx, bool startStop, TOOL_TYPE tool)
{
    // Backend guard: disabling a button is not a security boundary.
    if (startStop && tool == Tool_SPD_FRP_FastBoot) {
        signal.currentTool = tool;
        getServerFastBoot_File();
        return;
    }
    qDebug()<<" Signal received:  rx_StartRepairing "<<startStop;
    processStartStop = startStop;
    showThisinDebug = false;

    if(startStop){
        signal.currentTool = tool;  // Seting Current Tool , => MTK or SPD

        qDebug()<<" SPD Tool -> Sending ADB Commands";
        nowForSecondIMEI = false;
        adb_InfoOnly = false;
        actualADBcommands = false;
        singleShotIndex = 1;

        if(signal.currentTool != Tool_SPD_FRP_FastBoot) if(!getServerIMEInumber_updateGlobal()) return;



        /*
            1. "E6I" powershell
            2. "E7I" powelshell
            3. "E6s XT2053" cmd
            4. "G8 Power Lite XT2055"		 cmd
            5. "E7 Play XT2095" cmd
            6. "G20" cmd
            */


        if(GlobalVars::spd_modelStr.contains("E6I")) { adbProcess->setProgram(powerShellPath); }
        else if(GlobalVars::spd_modelStr.contains("E7I")||GlobalVars::spd_modelStr.contains("E20 XT2155")||GlobalVars::spd_modelStr.contains("E40 XT2159")) { adbProcess->setProgram(powerShellPath); }
        else if(GlobalVars::spd_modelStr.contains("E6s XT2053")) { adbProcess->setProgram(cmdShellPath); }
        else if(GlobalVars::spd_modelStr.contains("G8 Power Lite XT2055")) { adbProcess->setProgram(cmdShellPath); }
        else if(GlobalVars::spd_modelStr.contains("E7 Play XT2095")) { adbProcess->setProgram(cmdShellPath); }
        else if(GlobalVars::spd_modelStr.contains("G20")) { adbProcess->setProgram(cmdShellPath); }



        qDebug()<<" Selected Model: "<<GlobalVars::spd_modelStr<<" Program: "<<adbProcess->program();

        signal.isProcessCompleted = false;
        signal.progressBarValue = 5;
        signal.toolIndex = 1;
        signal.outputColor = GlobalVars::txtOutPutColor;

        if(signal.currentTool == Tool_SPD)
            signal.outputString =  "Start process Write ADB ImeI: OK";
        else if(signal.currentTool == Tool_UnLock)
            signal.outputString =  "Start process UnLOCK: OK";
        else if(signal.currentTool == Tool_SPD_FRP_FastBoot)
            signal.outputString = "Start process FastBoot: OK";
        signal.needtoInsertOutputBox = true;
        emit tx_ADB_ProcessData(signal);

        adbProcess->setWorkingDirectory(adbFolderPath);
        if(signal.currentTool ==  Tool_SPD_FRP_FastBoot)
        {
            spd_commandStr = "fastboot devices";
            timer_SingleShot->start(3000);
        } else {
            spd_commandStr = "adb devices";
        }

        shellArguments.clear();
        shellArguments.append("/C");
        shellArguments.append(spd_commandStr);
        adbProcess->close();
        adbProcess->waitForReadyRead(1000);
        adbProcess->setArguments(shellArguments);
        qDebug()<<" Arg::"<<shellArguments;
        qDebug()<<" Working Path::"<<adbProcess->workingDirectory();
        currentADB = ADB_ConnectedDevice;
        adbProcess->start();
        //qDebug()<<"\n\n \t Process Opening: "<<adbProcess->open(QIODevice::ReadWrite);

        if (adbProcess->waitForStarted(1000) == false)
            qDebug() << "Error starting external program";
        else
            qDebug() << "external program running";

        //adbProcess->setTextModeEnabled(true);
        //adbProcess->waitForReadyRead(1000);
        adbProcess->waitForFinished(1000);

        /*}*/
    }

    if(processStartStop ==  false){
        try {
            currentADB = ADB_Idle;
            adbProcess->close();
            singleShotIndex = 0;
            //metaProcess[idx]->close();
            /*
            switch (idx) {
            case 0:{
                if(timer_Device_1->isActive()) timer_Device_1->stop();
                timer_Device_1->start(500);
                break;
            }
            case 1:{
                if(timer_Device_2->isActive()) timer_Device_2->stop();
                timer_Device_2->start(500);
                break;
            }
            case 2:{
                if(timer_Device_3->isActive()) timer_Device_3->stop();
                timer_Device_3->start(500);
                break;
            }
            case 3:{
                if(timer_Device_4->isActive()) timer_Device_4->stop();
                timer_Device_4->start(500);
                break;
            }
            case 4:{
                if(timer_Device_5->isActive()) timer_Device_5->stop();
                timer_Device_5->start(500);
                break;
            }
            case 5:{
                if(timer_Device_6->isActive()) timer_Device_6->stop();
                timer_Device_6->start(500);
                break;
            }
            case 6:{
                if(timer_Device_7->isActive()) timer_Device_7->stop();
                timer_Device_7->start(500);
                break;
            }
            case 7:{
                if(timer_Device_8->isActive()) timer_Device_8->stop();
                timer_Device_8->start(500);
                break;
            }
            }
*/
        } catch (const std::bad_alloc &) { }
        return;
    }

}



// ---------------- QTimer Slots -----------------------
void BackEndClass::rx_timer_SPDTool_elapsed()
{
    if (signal.currentTool == Tool_SPD_FRP_FastBoot) {
        getServerFastBoot_File();
        return;
    }
    qDebug()<<" single Shot Timer_SPDTool :: "<<currentADB;

    switch (currentADB) {
    case ADB_Idle: break;
    case ADB_ConnectedDevice: break;
    case ADB_Model: {
        if(signal.currentTool == Tool_SPD_FRP_FastBoot){
            switch (frpInfoIndex) {
            case 0: spd_commandStr = "fastboot getvar has-slot:frp"; break;
            case 1: spd_commandStr = "fastboot getvar ro.boot.hardware.sku"; break;
            case 2: spd_commandStr = "fastboot getvar version-bootloader"; break;
            case 3: spd_commandStr = "fastboot getvar product_name"; break;
            case 4: spd_commandStr = "fastboot getvar unlock_raw_data"; break;
            }

        } else {
            spd_commandStr = "adb shell getprop ro.product.vendor.model";
        }
        shellArguments.clear();
        shellArguments.append("/C");
        shellArguments.append(spd_commandStr);
        adbProcess->close();
        adbProcess->waitForReadyRead(1000);
        adbProcess->setArguments(shellArguments);
        //adbProcess->start();
        //qDebug()<<" Model : command: "<<spd_commandStr;
        adbProcess->open(QIODevice::ReadWrite);
        if (adbProcess->waitForStarted(2000) == false)
            qDebug() << "Error starting external program";
        else
            qDebug()<< "external program running";
        adbProcess->waitForFinished(3000);
        break;
    }
    case ADB_AndroidVersion:{
        spd_commandStr = "adb shell getprop ro.product.build.version.release";
        shellArguments.clear();
        shellArguments.append("/C");
        shellArguments.append(spd_commandStr);
        adbProcess->close();
        adbProcess->waitForReadyRead(1000);
        adbProcess->setArguments(shellArguments);
        //qDebug()<<" Android Version : command: "<<spd_commandStr;
        adbProcess->open(QIODevice::ReadWrite);
        if (adbProcess->waitForStarted(1000) == false)
            qDebug() << "Error starting external program";
        else
            qDebug() << "external program running";
        adbProcess->waitForFinished(1000);
        break;
    }
    case ADB_BuildDate:{
        spd_commandStr = "adb shell getprop ro.product.build.date";
        shellArguments.clear();
        shellArguments.append("/C");
        shellArguments.append(spd_commandStr);
        adbProcess->close();
        adbProcess->waitForReadyRead(1000);
        adbProcess->setArguments(shellArguments);
        adbProcess->start();
        if (adbProcess->waitForStarted(1000) == false)
            qDebug() << "Error starting external program";
        else
            qDebug() << "external program running";
        adbProcess->waitForFinished(1000);
        break;
    }
    case ADB_SerialNumber:{
        spd_commandStr = "adb shell getprop ro.boot.serialno";
        shellArguments.clear();
        shellArguments.append("/C");
        shellArguments.append(spd_commandStr);
        adbProcess->close();
        adbProcess->waitForReadyRead(1000);
        adbProcess->setArguments(shellArguments);
        adbProcess->start();
        if (adbProcess->waitForStarted(1000) == false)
            qDebug() << "Error starting external program";
        else
            qDebug()<<"\n";// << "external program running";
        adbProcess->waitForFinished(1000);
        break;
    }
    case ADB_AssignPortCmd: {
        adbProcess->setWorkingDirectory(ChinoeIMEICMToolFolderPath_Data1);
        spd_commandStr = "ChinoeIMEICMTool.exe -plat SP -catch";
        //commandStr = "ChinoeIMEICMTool.exe help";
        shellArguments.clear();
        shellArguments.append("/C");
        shellArguments.append(spd_commandStr);
        adbProcess->close();
        adbProcess->waitForReadyRead(1000);
        adbProcess->setArguments(shellArguments);
        adbProcess->start();
        if (adbProcess->waitForStarted(1000) == false)
            qDebug() << "Error starting external program";
        else
            qDebug() << " Assigning Port external program running";
        adbProcess->waitForFinished(1000);
        break;
    }
    case ADB_Original_imei:{
        spd_commandStr = "adb shell \"service call iphonesubinfo 1 | toybox cut -d \"'\" -f2 | toybox grep -Eo '[0-9]' | toybox xargs | toybox sed 's/\ //g'\"";
        //qDebug()<<" \n\n Origional IMEI Command: "<<commandStr<<"\n\n";
        shellArguments.clear();
        shellArguments.append("/C");
        shellArguments.append(spd_commandStr);
        adbProcess->close();
        adbProcess->waitForReadyRead(1000);
        adbProcess->setArguments(shellArguments);
        adbProcess->start();
        if (adbProcess->waitForStarted(1000) == false)
            qDebug() << "Error starting external program";
        else
            qDebug()<< " Origional IMEI external program running";
        adbProcess->waitForFinished(1000);
        break;
    }
    case ADB_Reboot:{
        spd_commandStr = "adb shell reboot -p";
        shellArguments.clear();
        shellArguments.append("/C");
        shellArguments.append(spd_commandStr);
        adbProcess->close();
        adbProcess->waitForReadyRead(1000);
        adbProcess->setArguments(shellArguments);
        adbProcess->start();
        if (adbProcess->waitForStarted(1000) == false)
            qDebug() << "Error starting external program";
        else
            qDebug()<<"Rebooting Mobile external program running";
        adbProcess->waitForFinished(1000);

        if(adb_InfoOnly == false) {
            if(signal.currentTool == Tool_UnLock) {
                currentADB = ADB_UnLockSIM;
                timer_SPDTool->start(5000);
            } else if(signal.currentTool == Tool_SPD){
                currentADB = ADB_ChinoeIMEICMTool;
                actualADBcommands = true;
                timer_SPDTool->start(5000);
            }
        } else {
            if(GlobalVars::spd_modelStr.contains("E6I")||GlobalVars::spd_modelStr.contains("E7I")||GlobalVars::spd_modelStr.contains("E20 XT2155")||GlobalVars::spd_modelStr.contains("E40 XT2159")) {
                if(GlobalVars::spd_AssignPort_bool) {
                    currentADB = ADB_AssignPortCmd;
                    timer_SPDTool->start(3500);
                }
            }
        }


        break;
    }
    case ADB_ChinoeIMEICMTool:{
        //qDebug()<<" Current Selected Model : "<<GlobalVars::modelStr;

        if(GlobalVars::spd_modelStr.contains("E6I")||GlobalVars::spd_modelStr.contains("E7I")||GlobalVars::spd_modelStr.contains("E20 XT2155")||GlobalVars::spd_modelStr.contains("E40 XT2159")) {
            adbProcess->setWorkingDirectory(ChinoeIMEICMToolFolderPath_Data1);
            if(!nowForSecondIMEI){
                spd_commandStr = QString("./ChinoeIMEICMTool.exe -plat SP -auth lixinsheng mf2Rqrw% -wi 0 "+GlobalVars::spd_imei_1+" lixinsheng mf2Rqrw%");
            }else if(nowForSecondIMEI) {
                spd_commandStr = QString("./ChinoeIMEICMTool.exe -plat SP -auth lixinsheng mf2Rqrw% -wi 1 "+GlobalVars::spd_imei_2+" lixinsheng mf2Rqrw%");
            }
        }
        else if(GlobalVars::spd_modelStr.contains("E6s XT2053")||GlobalVars::spd_modelStr.contains("G8 Power Lite XT2055")||GlobalVars::spd_modelStr.contains("E7 Play XT2095")) {
            adbProcess->setWorkingDirectory(ChinoeIMEICMToolFolderPath_Data2);
            if(!nowForSecondIMEI){
                spd_commandStr = QString("Chinoeimeicmtool -i 0 "+GlobalVars::spd_imei_1+" lixinsheng mf2Rqrw%");
            }else if(nowForSecondIMEI) {
                spd_commandStr = QString("Chinoeimeicmtool -i 1 "+GlobalVars::spd_imei_2+" lixinsheng mf2Rqrw%");
            }
        }
        else if(GlobalVars::spd_modelStr.contains("G20")) {
            adbProcess->setWorkingDirectory(ChinoeIMEICMToolFolderPath_Data3);

            nowForSecondIMEI = true;
            if(GlobalVars::spd_dual_imei_bool){
                spd_commandStr = QString("WriteIMEIToolV1.1.exe -cTSDCLogin dfs,Dfs@2021dfs -cConnect -cWriteIMEI1 "+GlobalVars::spd_imei_1+" -cWriteIMEI2 "+GlobalVars::spd_imei_2+" -cReadTrackId -cReadIMEI1 -cReadIMEI2  -cLog -E");
            }else{

                spd_commandStr = QString("WriteIMEIToolV1.1.exe -cTSDCLogin dfs,Dfs@2021dfs -cConnect -cReadTrackId -cReadIMEI1 -cWriteIMEI1 "+GlobalVars::spd_imei_1+"  -cLog -E");
                //spd_commandStr = QString("WriteIMEIToolV1.1.exe -cTSDCLogin dfs,Dfs@2021dfs -cConnect -cWriteIMEI1 "+GlobalVars::spd_imei_1+" -cWriteIMEI2 "+GlobalVars::spd_imei_2+" -cReadTrackId -cReadIMEI1 -cReadIMEI2  -cLog -E");
            }
        }


        qDebug()<<" \n\n Repair Working Dir: after reboot::"<<adbProcess->workingDirectory()<<"\n comd: "<<spd_commandStr<<"\n\n";

        shellArguments.clear();
        shellArguments.append("/C");
        shellArguments.append(spd_commandStr);
        adbProcess->close();
        adbProcess->waitForReadyRead(1000);
        adbProcess->setArguments(shellArguments);
        //adbProcess->start();
        adbProcess->open(QIODevice::ReadWrite);

        if (adbProcess->waitForStarted(1000) == false)
            qDebug() << "Error starting external program";
        else
            qDebug() << "external program running";
        //adbProcess->waitForFinished(1000);
        adbProcess->waitForReadyRead(15000);
        break;
    }
    case ADB_UnLockSIM: {
        if(GlobalVars::spd_modelStr.contains("E6I")) {
            adbProcess->setWorkingDirectory(ChinoeIMEICMToolFolderPath_Data1);
            //spd_commandStr = QString("./ChinoeIMEICMTool.exe -plat SP -auth lixinsheng mf2Rqrw% -ws "+simUnLock_E6I+" lixinsheng mf2Rqrw%");

            spd_commandStr = QString("./ChinoeIMEICMTool.exe -plat SP -auth lixinsheng mf2Rqrw% -ws "+simUnLock_E6I+" lixinsheng mf2Rqrw%");
        }
        else if(GlobalVars::spd_modelStr.contains("E7I")) {
            adbProcess->setWorkingDirectory(ChinoeIMEICMToolFolderPath_Data1);
            //spd_commandStr = QString("./Chinoeimeicmtool -s "+simUnLock_E7I+" lixinsheng mf2Rqrw%");
            spd_commandStr = QString("./ChinoeIMEICMTool.exe -plat SP -auth lixinsheng mf2Rqrw% -ws "+simUnLock_E7I+" lixinsheng mf2Rqrw%");
        }
        else if(GlobalVars::spd_modelStr.contains("E20 XT2155")){
            adbProcess->setWorkingDirectory(ChinoeIMEICMToolFolderPath_Data1);
            spd_commandStr = QString("./ChinoeIMEICMTool.exe -plat SP -auth lixinsheng mf2Rqrw% -ws "+simUnLock_E20xt2155+" lixinsheng mf2Rqrw%");
        }
        else if(GlobalVars::spd_modelStr.contains("E40 XT2159")){
            adbProcess->setWorkingDirectory(ChinoeIMEICMToolFolderPath_Data1);
            spd_commandStr = QString("./ChinoeIMEICMTool.exe -plat SP -auth lixinsheng mf2Rqrw% -ws "+simUnLock_E40xt2159+" lixinsheng mf2Rqrw%");
        }

        else if(GlobalVars::spd_modelStr.contains("E6s XT2053")) {
            adbProcess->setWorkingDirectory(ChinoeIMEICMToolFolderPath_Data2);
            spd_commandStr = QString("Chinoeimeicmtool -s "+simUnLock_E6sXT2053+" lixinsheng mf2Rqrw%");
        }
        else if(GlobalVars::spd_modelStr.contains("G8 Power Lite XT2055")) {
            adbProcess->setWorkingDirectory(ChinoeIMEICMToolFolderPath_Data2);
            spd_commandStr = QString("Chinoeimeicmtool -s "+simUnLock_G8PowerLite+" lixinsheng mf2Rqrw%");
        }
        else if(GlobalVars::spd_modelStr.contains("E7 Play XT2095")) {
            adbProcess->setWorkingDirectory(ChinoeIMEICMToolFolderPath_Data2);
            spd_commandStr = QString("Chinoeimeicmtool -s "+simUnLock_E7PlayXT2095+" lixinsheng mf2Rqrw%");
        }
        else if(GlobalVars::spd_modelStr.contains("G20")) {
            adbProcess->setWorkingDirectory(ChinoeIMEICMToolFolderPath_Data3);
            spd_commandStr = QString("WriteIMEIToolV1.1.exe -cTSDCLogin dfs,Dfs@2021dfs -cConnect -cSIMLOCK "+simUnLock_G20+" -cLog -E");
        }

        qDebug() << "\n\n SIM UNLOCK :"<<spd_commandStr<<"\n\n";

        shellArguments.clear();
        shellArguments.append("/C");
        shellArguments.append(spd_commandStr);
        adbProcess->close();
        adbProcess->waitForReadyRead(1000);
        adbProcess->setArguments(shellArguments);
        //adbProcess->start();
        adbProcess->open(QIODevice::ReadWrite);

        if (adbProcess->waitForStarted(1000) == false)
            qDebug() << "Error starting external program";
        else
            qDebug() << "SIM UNLOCK external program running";
        //adbProcess->waitForFinished(1000);
        adbProcess->waitForReadyRead(15000);
        break;
    }
    case ADB_verify: break;
    case ADB_FRP_GetServerFile:
    case ADB_FRP_PushFile:
    case ADB_FRP_FastBootErase:
    case ADB_FRP_Fast_Reboot:
        getServerFastBoot_File();
        break;
    }

}

void BackEndClass::rx_timer_singleShot_elapsed()
{
    signal.progressBarValue = 100;
    signal.outputBold = true;
    signal.outputString = "No Device Found. ";
    signal.outputColor = Qt::red; //GlobalVars::txtOutPutColor;
    signal.needtoInsertOutputBox = true;
    signal.isProcessCompleted = true;
    signal.upload_LogHistory = false;
    emit tx_ADB_ProcessData(signal);
}




// ---------------- ADB + SPD Slots and Methods -----------------------
void BackEndClass::rx_ADB_OutPutReceived()
{
    if(processStartStop ==  false){
        return;
    }
    singleShotIndex =1;
    spd_commandlineOutPut = adbProcess->readAllStandardOutput();
    if(actualADBcommands){
        spd_commandlineOutPut = spd_commandlineOutPut.replace("\r\n","\n");
    }else {
        spd_commandlineOutPut = spd_commandlineOutPut.left(spd_commandlineOutPut.length()-2);
    }
    qDebug()<<" rx_ADB_OutPutReceived: "<<currentADB<<" : "<<spd_commandlineOutPut;


    switch (currentADB) {
    case ADB_Idle: break;
    case ADB_ConnectedDevice: getConnectedDevices_andSendThemGUI(spd_commandlineOutPut.toUtf8()); break;
    case ADB_Model: getADB_Model(spd_commandlineOutPut.toUtf8()); break;
    case ADB_AndroidVersion: getADB_AndroidVersion(spd_commandlineOutPut.toUtf8());  break;
    case ADB_BuildDate: getADB_BuildDate(spd_commandlineOutPut.toUtf8());  break;
    case ADB_SerialNumber: getADB_SerialNumber(spd_commandlineOutPut.toUtf8());  break;
    case ADB_AssignPortCmd: break;
    case ADB_Original_imei: getADB_Original_imei(spd_commandlineOutPut.toUtf8());  break;
    case ADB_Reboot: getADB_rebootCommand(spd_commandlineOutPut.toUtf8()); break;
    case ADB_ChinoeIMEICMTool: getADB_ChinoeIMEICMTool(spd_commandlineOutPut.toUtf8()); break;
    case ADB_UnLockSIM: getADB_UnLockSIMTool(spd_commandlineOutPut.toUtf8());  break;
    case ADB_verify: break;

    case ADB_FRP_GetServerFile: get_FRP_GetServerFile(spd_commandlineOutPut.toUtf8()); break;
    case ADB_FRP_PushFile: get_FRP_PushFile(spd_commandlineOutPut.toUtf8());  break;
    case ADB_FRP_FastBootErase: get_FRP_FastBootErase(spd_commandlineOutPut.toUtf8());  break;
    case ADB_FRP_Fast_Reboot: get_FRP_Fast_Reboot(spd_commandlineOutPut.toUtf8());  break;

    }

}

void BackEndClass::getConnectedDevices_andSendThemGUI(QByteArray ba)
{
    //qDebug()<<"getConnectedDevices_andSendThemGUI::  "<<ba;
    int endlineIndx = 0;
    int tabIndx = 0;
    QString deviceID ="";
    QString authorized ="";
    adbDeviceDetectedandAuthorized = false;

    if(signal.currentTool == Tool_SPD_FRP_FastBoot){
        frp_infoCompleted = false;
        frpInfoIndex = 0;
        timer_SingleShot->stop();
        if(ba.contains("fastboot")){
            endlineIndx = ba.indexOf(" ");
            frp_DeviceID = ba.left(endlineIndx);
            ba = ba.right(ba.length()-(endlineIndx+1));
            //qDebug()<<" New bytes: "<<ba;
            endlineIndx = ba.indexOf("\r\n");
            authorized = ba.left(endlineIndx);
            if(authorized == "fastboot"){
                signal.outputString = "Start process Read Info : OK";
                signal.needtoInsertOutputBox= true;
                signal.outputColor = GlobalVars::txtOutPutColor;
                emit tx_ADB_ProcessData(signal);

                signal.outputString = "Processing";
                emit tx_ADB_ProcessData(signal);

                adbDeviceDetectedandAuthorized = true;
            }
            else if(authorized =="unauthorized")
            {
                adbDevicesRaw.clear();
                signal.outputString = "Device is not authorized. Please enable the usb debuging mode.";
                signal.needtoInsertOutputBox = true;
                signal.isProcessCompleted = true;
                signal.upload_LogHistory = false;
                signal.progressBarValue = 100;
                emit tx_ADB_ProcessData(signal);

                adbProcess->reset();
            }
            currentADB = ADB_Model;
            singleShotIndex=1;
            timer_SPDTool->start(500);

            signal.needtoInsertOutputBox = false;
            signal.progressBarValue = 15;
            emit tx_ADB_ProcessData(signal);

        } else {
            signal.outputString = "Error in Finding Devices, Please re-Plug the device \n ---------------------------------";
            signal.outputColor = Qt::red;
            signal.needtoInsertOutputBox = true;
            signal.progressBarValue = 100;
            emit tx_ADB_ProcessData(signal);
        }
    } else {
        if(ba.contains("List of devices attached")){
            adbDevicesRaw.clear();
            endlineIndx = ba.indexOf("\n");
            ba = ba.right(ba.length()-(endlineIndx+1));
            //qDebug()<<" New bytes: "<<ba;
            tabIndx = ba.indexOf("\t");
            deviceID = ba.left(tabIndx);

            ba.remove(0, tabIndx+1);
            endlineIndx = ba.indexOf("\r\n");
            authorized = ba.left(endlineIndx);
            if(authorized == "device"){
                adbDevicesRaw.append(deviceID);
                signal.outputString = "Start process Read Info : OK";
                signal.outputColor = GlobalVars::txtOutPutColor;
                signal.outputBold = false;
                signal.needtoInsertOutputBox = true;
                signal.progressBarValue = 11;
                emit tx_ADB_ProcessData(signal);
                signal.outputString = "Processing";
                emit tx_ADB_ProcessData(signal);
                signal.outputString = "Getting Authorization From Server: OK";
                emit tx_ADB_ProcessData(signal);

                adbDeviceDetectedandAuthorized = true;
            }
            else if(authorized =="unauthorized")
            {
                adbDevicesRaw.clear();
                signal.outputString = "Device is not authorized. Please enable the usb debuging mode.";
                signal.outputColor = GlobalVars::txtOutPutColor;
                signal.outputBold = false;
                signal.needtoInsertOutputBox = true;
                signal.progressBarValue = 100;
                signal.isProcessCompleted = true;
                emit tx_ADB_ProcessData(signal);

                adbProcess->reset();
            }


            currentADB = ADB_Model;
            singleShotIndex=1;
            timer_SPDTool->start(500);

            signal.needtoInsertOutputBox = false;
            signal.progressBarValue = 50;
            emit tx_ADB_ProcessData(signal);
        }
        else
        {
            signal.outputString = "Error in Finding Devices, Please re-Plug the device \n ---------------------------------";
            signal.outputColor = Qt::red; // GlobalVars::txtOutPutColor;
            signal.outputBold = true;
            signal.needtoInsertOutputBox = true;
            signal.progressBarValue = 100;
            signal.isProcessCompleted = true;
            emit tx_ADB_ProcessData(signal);
        }
    }
}
void BackEndClass::getADB_Model(QByteArray ba)
{
    //qDebug()<<"getADB_Model::  "<<ba;
    if(signal.currentTool == Tool_SPD_FRP_FastBoot){
        getFastBoot_info(ba);
        if(frp_infoCompleted){
            currentADB =  ADB_FRP_GetServerFile;
            timer_SPDTool->start(500);
            signal.outputColor = GlobalVars::txtOutPutColor;
            signal.needtoInsertOutputBox = false;
            signal.progressBarValue = 25;
            signal.isProcessCompleted = false;
            emit tx_ADB_ProcessData(signal);
        }

    } else {
        signal.outputString = QString("Model: "+ba);
        signal.outputColor = GlobalVars::txtOutPutColor;
        signal.outputBold = false;
        signal.needtoInsertOutputBox = true;
        signal.progressBarValue = 20;
        signal.isProcessCompleted = false;
        emit tx_ADB_ProcessData(signal);

        currentADB = ADB_AndroidVersion;
        timer_SPDTool->start(500);
    }
}
void BackEndClass::getADB_AndroidVersion(QByteArray ba)
{
    //qDebug()<<"getADB_AndroidVersion::  "<<ba;

    signal.outputString = QString("Android Version: "+ba);
    signal.outputColor = GlobalVars::txtOutPutColor;
    signal.outputBold = false;
    signal.needtoInsertOutputBox = true;
    signal.progressBarValue = 30;
    signal.isProcessCompleted = false;
    emit tx_ADB_ProcessData(signal);

    if(adb_InfoOnly == false){
        currentADB = sendOperationDetailsToServerToReduceCredit();
    }
    else {
        currentADB = ADB_SerialNumber;
    }
    qDebug()<<"\n\n\t ********** getADB_AndroidVersion Timer_SPDTool :: "<<currentADB;
    timer_SPDTool->start(500);
}
void BackEndClass::getADB_BuildDate(QByteArray ba)
{
    //qDebug()<<"getADB_BuildDate::  "<<ba;
    signal.outputString = QString("BuildDate: "+ba);
    signal.outputColor = GlobalVars::txtOutPutColor;
    signal.outputBold = false;
    signal.needtoInsertOutputBox = true;
    signal.progressBarValue = 10;
    emit tx_ADB_ProcessData(signal);

    currentADB = ADB_SerialNumber;
    timer_SPDTool->start(500);

}
void BackEndClass::getADB_SerialNumber(QByteArray ba)
{
    //qDebug()<<"getADB_SerialNumber::  "<<ba;

    signal.outputString = QString("Serial: "+ba);
    signal.outputColor = GlobalVars::txtOutPutColor;
    signal.outputBold = false;
    signal.needtoInsertOutputBox = true;
    signal.isProcessCompleted = false;
    emit tx_ADB_ProcessData(signal);

    if(adb_InfoOnly == false){
        if(signal.currentTool == Tool_SPD)
        {
            if(GlobalVars::spd_dual_imei_bool)
            {
                signal.outputString = QString("IMEI-1: " + GlobalVars::spd_imei_1);
                emit tx_ADB_ProcessData(signal);
                signal.outputString = QString("IMEI-2: "+GlobalVars::spd_imei_2);
                emit tx_ADB_ProcessData(signal);
            } else {
                signal.outputString = QString("IMEI-1: "+ GlobalVars::spd_imei_1);
                emit tx_ADB_ProcessData(signal);
            }
        }

        currentADB = ADB_Reboot;
        timer_SPDTool->start(500);
        signal.needtoInsertOutputBox = false;
        signal.progressBarValue = 40;
        emit tx_ADB_ProcessData(signal);
    }
    else{
        //qDebug()<<"\n\n\t assignPort_bool:"<<GlobalVars::spd_AssignPort_bool;
        if(GlobalVars::spd_modelStr.contains("E6I")||GlobalVars::spd_modelStr.contains("E7I")||GlobalVars::spd_modelStr.contains("E20 XT2155")||GlobalVars::spd_modelStr.contains("E40 XT2159")) {
            if(GlobalVars::spd_AssignPort_bool)
            {
                currentADB = ADB_Reboot;
                //currentADB = ADB_AssignPortCmd;
                timer_SPDTool->start(500);
                signal.progressBarValue = 70;
                emit tx_ADB_ProcessData(signal);
            }
            else
            {
                signal.outputString = "--------------------------------------";
                signal.outputColor = Qt::white;
                signal.outputBold = false;
                signal.needtoInsertOutputBox = true;
                signal.progressBarValue = 100;
                signal.isProcessCompleted = true;
                emit tx_ADB_ProcessData(signal);
            }
        }
        else if(GlobalVars::spd_modelStr.contains("E6s XT2053")||GlobalVars::spd_modelStr.contains("G8 Power Lite XT2055")||GlobalVars::spd_modelStr.contains("E7 Play XT2095")) {
            signal.outputString = "------------------------------------------";
            signal.outputColor = Qt::white; // GlobalVars::txtOutPutColor;
            signal.outputBold = false;
            signal.needtoInsertOutputBox = true;
            signal.progressBarValue = 100;
            signal.isProcessCompleted = true;
            emit tx_ADB_ProcessData(signal);
        }
        else if(GlobalVars::spd_modelStr.contains("G20")) {

            signal.outputString = "------------------------------";
            signal.outputColor = Qt::white; // GlobalVars::txtOutPutColor;
            signal.outputBold = false;
            signal.needtoInsertOutputBox = true;
            signal.progressBarValue = 100;
            signal.isProcessCompleted = true;
            emit tx_ADB_ProcessData(signal);
        }
    }
}
void BackEndClass::getADB_AssignPortCmd(QByteArray ba)
{
    qDebug()<<"getADB_AssignPortCmd::  "<<ba;

    signal.outputString = ba;
    signal.outputColor = GlobalVars::txtOutPutColor;
    signal.outputBold = false;
    signal.needtoInsertOutputBox = true;
    emit tx_ADB_ProcessData(signal);

    if(adb_InfoOnly == false){
        currentADB = ADB_ChinoeIMEICMTool;
        timer_SPDTool->start(1500);
        signal.needtoInsertOutputBox = false;
        signal.progressBarValue = 80;
        emit tx_ADB_ProcessData(signal);
    }
    else {
        signal.needtoInsertOutputBox = false;
        signal.progressBarValue = 100;
        signal.isProcessCompleted = true;
        emit tx_ADB_ProcessData(signal);
    }
}
void BackEndClass::getADB_Original_imei(QByteArray ba)
{
    //qDebug()<<"getADB_Original_imei::  "<<ba;

    signal.outputString = QString("Original IMEI: "+ba);
    emit tx_ADB_ProcessData(signal);

    currentADB = ADB_Reboot;
    timer_SPDTool->start(500);

}
void BackEndClass::getADB_rebootCommand(QByteArray ba)
{
    qDebug()<<"getADB_rebootCommand::  "<<ba;
    ba.replace("\n", "");
    ba.replace("\r", "");


    signal.outputString = QString("Reboot: "+ba);
    emit tx_ADB_ProcessData(signal);

    //qDebug()<<" TootType indx : "<<signal.currentTool;
    if(timer_SPDTool->isActive()) timer_SPDTool->stop();
    if(adb_InfoOnly == false) {
        if(signal.currentTool == Tool_UnLock)
        {
            currentADB = ADB_UnLockSIM;
            timer_SPDTool->start(2000);

            signal.progressBarValue = 50;
            emit tx_ADB_ProcessData(signal);
        }
        else if(signal.currentTool == Tool_SPD){
            currentADB = ADB_ChinoeIMEICMTool;
            actualADBcommands = true;
            timer_SPDTool->start(2000);

            signal.progressBarValue = 50;
            emit tx_ADB_ProcessData(signal);
        }
    } else {
        if(GlobalVars::spd_modelStr.contains("E6I")||GlobalVars::spd_modelStr.contains("E7I")||GlobalVars::spd_modelStr.contains("E20 XT2155")||GlobalVars::spd_modelStr.contains("E40 XT2159")) {
            if(GlobalVars::spd_AssignPort_bool)
            {
                currentADB = ADB_AssignPortCmd;
                timer_SPDTool->start(1500);

                signal.progressBarValue = 70;
                emit tx_ADB_ProcessData(signal);
            }
            else
            {
                signal.outputString = "-------------------------------------";
                signal.outputColor = Qt::white; // GlobalVars::txtOutPutColor;
                signal.outputBold = false;
                signal.needtoInsertOutputBox = true;
                signal.isProcessCompleted = true;
                signal.progressBarValue = 100;
                emit tx_ADB_ProcessData(signal);
            }
        }
    }
}

void BackEndClass::getADB_ChinoeIMEICMTool(QByteArray ba)
{
    qDebug()<<"getADB_ChinoeIMEICMTool::  "<<ba;

    if(ba.contains("Connect device fail")||ba.contains("Connect To cellphone = Fail"))
    {
        signal.outputString = "Error: Device not connected";
        signal.outputColor = Qt::red; // GlobalVars::txtOutPutColor;
        signal.outputBold = false;
        signal.needtoInsertOutputBox = true;
        signal.isProcessCompleted = true;
        signal.progressBarValue = 100;
        emit tx_ADB_ProcessData(signal);
        return;
    }

    if(GlobalVars::spd_modelStr.contains("E6I")||GlobalVars::spd_modelStr.contains("E7I")||GlobalVars::spd_modelStr.contains("E20 XT2155")||GlobalVars::spd_modelStr.contains("E40 XT2159")) {
        get_ChinoeDirectory1(ba);
    }
    else if(GlobalVars::spd_modelStr.contains("E6s XT2053")||GlobalVars::spd_modelStr.contains("G8 Power Lite XT2055")||GlobalVars::spd_modelStr.contains("E7 Play XT2095")) {
        get_ChinoeDirectory2(ba);
    }
    else if(GlobalVars::spd_modelStr.contains("G20")) {
        get_ChinoeDirectory3(ba);
    }


}
void BackEndClass::get_ChinoeDirectory1(QByteArray ba)
{
    qDebug()<<"\n\n \t actual Bytes Dir-1: "<<ba;
    int indx = ba.indexOf("Write IMEI");
    ba = ba.right(ba.length() - indx);
    indx=0;
    indx = ba.indexOf("Backup ");
    if(indx < 3) indx = ba.indexOf("\r\nWork don");
    QString sendToGUI = ba.left(indx);
    sendToGUI.replace("\r", "");
    sendToGUI = sendToGUI.left(sendToGUI.length()-2);

    indx = ba.indexOf("\r\nWork don");
    sendToGUI = sendToGUI.left(sendToGUI.length()-indx);
    qDebug()<<"\n\n \t sendToGUI: "<<sendToGUI;

    signal.outputString = QString("Tool: "+sendToGUI);
    signal.outputColor = GlobalVars::txtOutPutColor;
    signal.outputBold = false;
    signal.needtoInsertOutputBox = true;
    if(!sendToGUI.contains("Please connect device"))
         emit tx_ADB_ProcessData(signal);

    qDebug()<<" Left over info: "<<ba;
    if(ba.contains("Work don")){

        if(!nowForSecondIMEI && GlobalVars::spd_dual_imei_bool){
            nowForSecondIMEI = true; timer_SPDTool->start(1000);
        }
        if(GlobalVars::spd_dual_imei_bool == false) nowForSecondIMEI = true;

        if(nowForSecondIMEI && !timer_SPDTool->isActive()){
            signal.outputString = "Task successfully Completed";
            signal.outputColor = GlobalVars::txtOutPutColor;
            signal.outputBold = false;
            signal.needtoInsertOutputBox = true;
            signal.isProcessCompleted = true;
            signal.progressBarValue = 100;
            signal.upload_LogHistory = true;
            emit tx_ADB_ProcessData(signal);

        }
        else
        {
            signal.outputString = "Task IMEI-1 Completed.";
            signal.outputColor = GlobalVars::txtOutPutColor;
            signal.outputBold = false;
            signal.needtoInsertOutputBox = true;
            signal.progressBarValue = 75;
            emit tx_ADB_ProcessData(signal);


            signal.outputString = "Writing IMEI-2: wait.";
            signal.outputColor = GlobalVars::txtOutPutColor;
            signal.outputBold = true;
            emit tx_ADB_ProcessData(signal);
        }
    }

}
void BackEndClass::get_ChinoeDirectory2(QByteArray ba)
{
    qDebug()<<"\n\n \t actual Bytes Dir-2: "<<ba;

    int indx = ba.indexOf("Write IMEI1");
    ba = ba.right(ba.length() - indx);
    indx=0;
    indx = ba.indexOf("Backup ");
    if(indx <3) indx = ba.indexOf("\r\nWork don");
    QString sendToGUI = ba.left(indx);
    sendToGUI.replace("\r", "");
    sendToGUI = sendToGUI.left(sendToGUI.length()-2);
    qDebug()<<"\n\n \t sendToGUI: "<<sendToGUI;

    signal.outputString = QString("Tool: "+sendToGUI);
    signal.outputColor = GlobalVars::txtOutPutColor;
    signal.outputBold = false;
    signal.needtoInsertOutputBox = true;
    emit tx_ADB_ProcessData(signal);

    qDebug()<<" Left over info: "<<ba;
    if(ba.contains("Work don")){
        if(!nowForSecondIMEI && GlobalVars::spd_dual_imei_bool){
            nowForSecondIMEI = true; timer_SPDTool->start(1000);
        }
        if(GlobalVars::spd_dual_imei_bool == false) nowForSecondIMEI = true;

        //if(timer_singleShot->isActive()) qDebug()<<" QTimer is started Again: "<<nowForSecondIMEI<<"  Dual IMEI: "<<GlobalVars::dual_imei_bool;
        //else qDebug()<<" QTimer is not Active : "<<nowForSecondIMEI<<"  Dual IMEI: "<<GlobalVars::dual_imei_bool;

        if(nowForSecondIMEI){
            signal.outputString = "Task successfully Completed.";
            signal.outputColor =  GlobalVars::txtOutPutColor;
            signal.outputBold = true;
            signal.needtoInsertOutputBox = true;
            signal.isProcessCompleted = true;
            signal.progressBarValue = 100;
            signal.upload_LogHistory = true;
            emit tx_ADB_ProcessData(signal);
        }
        else
        {
            signal.outputString = "Task IMEI-1 Completed.";
            signal.outputColor = GlobalVars::txtOutPutColor;
            signal.outputBold = false;
            signal.needtoInsertOutputBox = true;
            signal.isProcessCompleted = false;
            signal.progressBarValue = 75;
            emit tx_ADB_ProcessData(signal);

            signal.outputString = "Writing IMEI-2: Wait.";
            emit tx_ADB_ProcessData(signal);
        }
    }

}
void BackEndClass::get_ChinoeDirectory3(QByteArray ba)
{
    qDebug()<<"***************************************\n\n \t G20 Dir-3: "<<ba<<"\n***************************************";
    int indx = 0;
    QString required_data = "";
    if(ba.contains("Read TrackId ="))
    {
        // removing all text from start to this string
        indx = ba.indexOf("Read TrackId =");
        ba = ba.right(ba.length() - indx);
        // removing all with =
        indx = ba.indexOf("=");
        ba = ba.right(ba.length() - (indx+1));
        // finding , after Track id
        indx = ba.indexOf(",");
        required_data = ba.left(indx);

        signal.outputString = QString("Tracking ID: "+required_data);
        signal.outputColor = GlobalVars::txtOutPutColor;
        signal.outputBold = false;
        signal.needtoInsertOutputBox = true;
        emit tx_ADB_ProcessData(signal);
    }
    if(ba.contains("Write Simlock = "))
    {
        // removing all text from start to this string
        indx = ba.indexOf("Read TrackId =");
        ba = ba.right(ba.length() - indx);
        // removing all with =
        indx = ba.indexOf("=");
        ba = ba.right(ba.length() - (indx+1));
        // finding , after Track id
        indx = ba.indexOf(",");
        required_data = ba.left(indx);
        signal.outputString = QString("Write SimLock: " + required_data);
        signal.outputColor = GlobalVars::txtOutPutColor;
        signal.needtoInsertOutputBox = true;
        emit tx_ADB_ProcessData(signal);
    }
    if(ba.contains("Total Test Result = PASS"))
    {

        signal.outputString = "Task Successfully Completed";
        signal.outputColor = GlobalVars::txtOutPutColor;
        signal.outputBold = true;
        signal.needtoInsertOutputBox = true;
        signal.progressBarValue = 100;
        signal.isProcessCompleted = true;
        signal.upload_LogHistory = true;
        emit tx_ADB_ProcessData(signal);
    }
    else if(ba.contains("Total Test Result = FAIL"))
    {
        signal.outputString = "Test Result Failed";
        signal.outputColor = Qt::red; // GlobalVars::txtOutPutColor;
        signal.outputBold = false;
        signal.needtoInsertOutputBox = true;
        signal.progressBarValue = 100;
        signal.isProcessCompleted = true;
        emit tx_ADB_ProcessData(signal);
    }
    else if(ba.contains("Connect To cellphone = Fail"))
    {
        signal.outputString = "Coonect to the CellPhone = Fail";
        signal.outputColor = Qt::red; // GlobalVars::txtOutPutColor;
        signal.outputBold = false;
        signal.needtoInsertOutputBox = true;
        signal.progressBarValue = 100;
        signal.isProcessCompleted = true;
        emit tx_ADB_ProcessData(signal);
    }

}


void BackEndClass::getFastBoot_info(QByteArray ba)
{
    qDebug()<<"\n getFastBoot_info "<<ba;
    //frp_ProdName = "";
    //frp_uniqID = "";
    int idx = 0;
    QString requiredData = "";
    bool needToincremenrTheIndex = false;
    // ---------- FRP ------------
    if(ba.contains("has-slot:frp:")){
        needToincremenrTheIndex = true;
        idx = ba.indexOf("has-slot:frp:");
        ba = ba.right(ba.length() - idx);
        idx = ba.indexOf("frp:");
        ba = ba.right(ba.length() - idx);
        idx = ba.indexOf(":");
        ba = ba.right(ba.length() - (idx+1));
        qDebug()<<" aa :: "<<ba;
        idx = ba.indexOf("\r\n");
        requiredData = ba.left(idx);

        signal.outputString = QString("FRP = "+requiredData);
        signal.outputColor = GlobalVars::txtOutPutColor;
        signal.outputBold = false;
        signal.needtoInsertOutputBox = true;
        emit tx_ADB_ProcessData(signal);
    }
    // ---------- Model ------------
    if(ba.contains("ro.boot.hardware.sku:")){
        needToincremenrTheIndex = true;
        idx = ba.indexOf("ro.boot.hardware.sku:");
        ba = ba.right(ba.length() - idx);
        idx = ba.indexOf(":");
        ba = ba.right(ba.length() - (idx+1));
        idx = ba.indexOf("\r\n");
        requiredData = ba.left(idx);

        signal.outputString = QString("Model = "+requiredData);
        emit tx_ADB_ProcessData(signal);
    }

    // ---------- Android Version ------------
    if(ba.contains("version-bootloader:")){
        needToincremenrTheIndex = true;
        idx = ba.indexOf("version-bootloader:");
        ba = ba.right(ba.length() - idx);
        idx = ba.indexOf(":");
        ba = ba.right(ba.length() - (idx+1));
        idx = ba.indexOf("\r\n");
        requiredData = ba.left(idx);

        signal.outputString = QString("Android Version: "+requiredData);
        emit tx_ADB_ProcessData(signal);
    }

    // ---------- Device product Name ------------
    if(ba.contains("product_name:")){
        needToincremenrTheIndex = true;
        idx = ba.indexOf("product_name:");
        ba = ba.right(ba.length() - idx);
        idx = ba.indexOf(":");
        ba = ba.right(ba.length() - (idx+2));
        idx = ba.indexOf("\r\n");
        requiredData = ba.left(idx);
        frp_ProdName = requiredData;

        signal.outputString = QString("Product Name = "+frp_ProdName);
        emit tx_ADB_ProcessData(signal);
    }

    // ---------- Device Unique ID ------------
    if(ba.contains("unlock_raw_data:")){
        needToincremenrTheIndex = true;
        idx = ba.indexOf("unlock_raw_data:");
        ba = ba.right(ba.length() - idx);
        idx = ba.indexOf(":");
        ba = ba.right(ba.length() - (idx+2));
        idx = ba.indexOf("\r\n");
        requiredData = ba.left(idx);
        frp_uniqID = requiredData;
        frp_infoCompleted = true;

        signal.outputString = QString("Device Unique ID:"+requiredData);
        emit tx_ADB_ProcessData(signal);
    }

    if(needToincremenrTheIndex) {
        qDebug()<<" FRP index:"<<frpInfoIndex;
        frpInfoIndex++;
        timer_SPDTool->start(300);
    }

}

void BackEndClass::get_FRP_GetServerFile(QByteArray ba)
{
    qDebug()<<"get_FRP_GetServerFile::  "<<ba;

}
void BackEndClass::get_FRP_PushFile(QByteArray ba)
{
    qDebug()<<"get_FRP_PushFile::  "<<ba;

    //emit tx_TextBoxOutput(signal.currentTool, 1, QString("PushFile : " + ba), false, false, GlobalVars::txtOutPutColor);

    fastbootEraseDone = false;
    if(ba.toLower().contains("erase partition for assign") && !fastBootPushCommandDone){
        fastBootPushCommandDone = true;
        signal.outputString = "Device FRP Execution : OK";
        signal.outputColor = GlobalVars::txtOutPutColor;
        signal.outputBold = false;
        signal.needtoInsertOutputBox = true;
        signal.progressBarValue = 60;
        emit tx_ADB_ProcessData(signal);

        currentADB = ADB_FRP_FastBootErase;
        timer_SPDTool->start(500);
    }
}
void BackEndClass::get_FRP_FastBootErase(QByteArray ba)
{
    qDebug()<<"get_FRP_FastBootErase::  "<<ba;
    if(ba.contains("Erasing 'frp'") && !fastbootEraseDone){
        fastbootEraseDone = true;
        int idx = ba.indexOf("Erasing 'frp'");
        ba = ba.right(ba.length() - idx);

        signal.outputString = QString("Removing Device FRP : OK");
        signal.needtoInsertOutputBox = true;
        signal.progressBarValue = 75;
        emit tx_ADB_ProcessData(signal);

        fastbootRebootingDone = false;
        currentADB = ADB_FRP_Fast_Reboot;
        timer_SPDTool->start(500);
    }

}
void BackEndClass::get_FRP_Fast_Reboot(QByteArray ba)
{
    qDebug()<<"get_FRP_Fast_Reboot::  "<<ba;
    if(ba.contains("Rebooting ") && !fastbootRebootingDone){
        fastbootRebootingDone = true;
        signal.outputString = "Rebooting : OK";
        signal.outputColor = GlobalVars::txtOutPutColor;
        signal.needtoInsertOutputBox = true;
        signal.progressBarValue = 90;
        emit tx_ADB_ProcessData(signal);

        signal.outputString = "FRP Process: Successfull.";
        signal.outputColor = GlobalVars::txtOutPutColor;
        signal.outputBold = true;
        signal.needtoInsertOutputBox = true;
        signal.isProcessCompleted = true;
        signal.progressBarValue = 100;
        signal.upload_LogHistory = true;
        emit tx_ADB_ProcessData(signal);


    }

}

void BackEndClass::getADB_UnLockSIMTool(QByteArray ba)
{
    qDebug()<<"getADB_UnLockSIMTool::  "<<ba;
    if(ba.contains("device fail"))
    {
        signal.outputString = "Error: Device Fail to connect.";
        signal.outputColor = Qt::red; // GlobalVars::txtOutPutColor;
        signal.outputBold = false;
        signal.needtoInsertOutputBox = true;
        signal.isProcessCompleted = true;
        signal.progressBarValue = 100;
        signal.upload_LogHistory = true;
        emit tx_ADB_ProcessData(signal);
        return;
    }
    //int idx = ba.indexOf("UnlockCode generated:");
    int idx = ba.indexOf("Unlock");
    ba = ba.right(ba.length()-idx);
    idx = ba.indexOf(":");
    idx += 1;
    ba = ba.right(ba.length()- idx);
    qDebug()<<" right: "<<ba;
    idx = 0;
    idx = ba.indexOf("Backup NVRAM");
    qDebug()<<"idx::"<<idx;
    if(idx <2) idx = ba.indexOf("\r\nWork");
    qDebug()<<"left IDX:"<<idx<<" bytes:"<<ba;
    QString unLockCode = ba.left(idx);
    unLockCode.replace("\r","");
    unLockCode.replace("\n","");

    signal.outputString = QString("unLock Code: "+ unLockCode);
    signal.outputColor = Qt::white; // GlobalVars::txtOutPutColor;
    signal.outputBold = true;
    signal.needtoInsertOutputBox = true;
    emit tx_ADB_ProcessData(signal);

    if(ba.contains("Work do"))
    {
        signal.outputString = "Task Successfully Completed.";
        signal.outputColor = GlobalVars::txtOutPutColor;
        signal.outputBold = true;
        signal.needtoInsertOutputBox = true;
        signal.isProcessCompleted = true;
        signal.progressBarValue = 100;
        signal.upload_LogHistory = true;
        emit tx_ADB_ProcessData(signal);
    }
    else
    {
        signal.outputString = "Task Failed.";
        signal.outputColor = Qt::red; // GlobalVars::txtOutPutColor;
        signal.outputBold = false;
        signal.needtoInsertOutputBox = true;
        signal.isProcessCompleted = true;
        signal.progressBarValue = 100;
        signal.upload_LogHistory = true;
        emit tx_ADB_ProcessData(signal);
    }

}

void BackEndClass::rx_ADB_ErrorReceived()
{
    spd_commandError = adbProcess->readAllStandardError();
    qDebug()<<" rx_ADB_ErrorReceived Error:: "<<spd_commandError;
    if(currentADB == ADB_Reboot && spd_commandError.length()>2){
        getADB_rebootCommand(spd_commandError.toUtf8());
    }



    if(spd_commandError.contains("adb.exe: no devices/emulators found")){
        signal.outputString = "No Devices/Emulators Found.";
        signal.outputColor = Qt::red; // GlobalVars::txtOutPutColor;
        signal.outputBold = false;
        signal.needtoInsertOutputBox = true;
        signal.isProcessCompleted = true;
        signal.progressBarValue = 100;
        emit tx_ADB_ProcessData(signal);
    }

    if(signal.currentTool == Tool_SPD_FRP_FastBoot)
    {
        switch (currentADB) {
        case ADB_Model: getADB_Model(spd_commandError.toUtf8()); break;

        case ADB_FRP_GetServerFile: get_FRP_GetServerFile(spd_commandError.toUtf8()); break;
        case ADB_FRP_PushFile: get_FRP_PushFile(spd_commandError.toUtf8());  break;
        case ADB_FRP_FastBootErase: get_FRP_FastBootErase(spd_commandError.toUtf8());  break;
        case ADB_FRP_Fast_Reboot: get_FRP_Fast_Reboot(spd_commandError.toUtf8());  break;

        }
    }
}
void BackEndClass::rx_updateADBdevices()
{
    //qDebug()<<"rx_updateADBdevices:: reading ADB Devices ";
    processStartStop = true;
    adb_InfoOnly = true;
    nowForSecondIMEI = false;
    actualADBcommands = false;
    signal.currentTool = Tool_SPD;
    adbProcess->setWorkingDirectory(adbFolderPath);

/*
    QProcess *process = new QProcess();
    QStringList params;
    QString command = "powershell";
    params << "-c" << "Start-Process powershell.exe -Verb runAs " ;
    process->start(command, params);
    process->waitForFinished();
    process->kill();
    if (process->waitForStarted(1000) == false)
        qDebug() << "Error process starting external program";
    else
        qDebug() << "external process  program running";

    qDebug()<<" killed ";

*/

    QString command = "powershell";

    currentADB = ADB_ConnectedDevice;
    spd_commandStr = "adb devices";
    //qDebug()<<" write:"<<adbProcess->write(commandStr.toUtf8());
    //commandStr = "adb shell getprop ro.product.vendor.model";
    shellArguments.clear();
    //shellArguments.append("/C");

    shellArguments.append("-c");
    shellArguments.append(QString("Start-Process "+powerShellPath+" -Verb runAs"));

    shellArguments.append(spd_commandStr);

    adbProcess->close();
    adbProcess->waitForReadyRead(1000);
    adbProcess->setArguments(shellArguments);
    qDebug()<<" 15179 Arg::"<<shellArguments;
    //adbProcess->start();
    adbProcess->start(command, shellArguments);

    //qDebug()<<"\n\n \t Process Opening: "<<adbProcess->open(QIODevice::ReadWrite);

    if (adbProcess->waitForStarted(1000) == false)
        qDebug() << "Error starting external program";
    else
        qDebug() << "external program running";

    //adbProcess->setTextModeEnabled(true);
    //adbProcess->waitForReadyRead(1000);
    adbProcess->waitForFinished(1000);
}







bool BackEndClass::getServerFastBoot_File()
{
    // Intentionally fail closed. No charge, network request, downloaded file,
    // or shell execution is permitted through this retired workflow.
    timer_SPDTool->stop();
    timer_SingleShot->stop();
    currentADB = ADB_Idle;
    processStartStop = false;
    signal.outputString = "FRP is disabled pending a secure, validated replacement.";
    signal.outputColor = Qt::red;
    signal.outputBold = true;
    signal.needtoInsertOutputBox = true;
    signal.progressBarValue = 100;
    signal.isProcessCompleted = true;
    signal.upload_LogHistory = false;
    emit tx_ADB_ProcessData(signal);
    return false;
}

bool BackEndClass::getServerIMEInumber_updateGlobal()
{
    if(!GlobalVars::spd_manual_imei_bool)
    {
        QByteArray postdata;// = data.toJson();
        postdata.append(QString("model="+GlobalVars::spd_projectName+"&").toUtf8());
        postdata.append(QString("simCode="+QString(GlobalVars::spd_dual_imei_bool? "ds":"ss")).toUtf8());

        QJsonObject obj;
        obj["data"] =  QString(QCryptographicHash::hash(postdata, QCryptographicHash::Sha3_256).toUpper().toHex());
        obj["encrypt"] = "2";
        QJsonDocument doc(obj);
        QByteArray data = doc.toJson();
        qDebug()<<" \n\n IMEI ByteArray:"<<data <<" \n Base64: "<<data.toBase64();

        if(processStartStop ==  false){
            return false;
        }
        QEventLoop loop;
        QNetworkAccessManager nam;
        QNetworkRequest req;
        req.setUrl(QUrl(GlobalVars::api_MTKimeiGenerator));
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        req.setRawHeader(QByteArray("Authorization"), GlobalVars::authorizedToken.toUtf8());


        QNetworkReply *reply = nam.post(req, data.toBase64());
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        if(processStartStop ==  false){ return false; }

        signal.outputString = "Getting IMEI from Server: OK";
        signal.outputColor = GlobalVars::txtOutPutColor;
        signal.outputBold = false;
        signal.needtoInsertOutputBox = true;
        signal.progressBarValue = 40;
        emit tx_ADB_ProcessData(signal);

        QByteArray buffer = reply->readAll();
        //qDebug()<<" reply:: "<<buffer;
        reply->deleteLater();
        if(processStartStop ==  false){ return false; }
        buffer = QByteArray::fromBase64(buffer);
        qDebug()<<"\n\n\n IMEI response_data::"<<buffer;
        //qDebug()<<"\n\n";


        if(buffer.contains("Tunnel") && buffer.contains("ERR_NGROK_3200")){
            qDebug()<<" yes returning and emitting signal ";
            signal.outputString = "Server OFFLine.";
            signal.outputColor = Qt::red; // GlobalVars::txtOutPutColor;
            signal.outputBold = false;
            signal.needtoInsertOutputBox = true;
            signal.isProcessCompleted = true;
            signal.progressBarValue = 100;
            emit tx_ADB_ProcessData(signal);
            return false;
        }


        QJsonDocument document = QJsonDocument::fromJson(buffer);
        QJsonObject jsonBuffer = document.object();

        qDebug()<<" reply:: "<<jsonBuffer;
        qDebug()<<" isError: "<<jsonBuffer.value("isError"); //.value("AccessToken");
        qDebug()<<"\n\n ";

        if (jsonBuffer.value("isError").toBool() == true)
        {
            signal.outputString = "Failed to get IMEI-1 from Server";
            signal.outputColor = Qt::red; // GlobalVars::txtOutPutColor;
            signal.outputBold = false;
            signal.needtoInsertOutputBox = true;
            signal.isProcessCompleted = true;
            signal.progressBarValue = 100;
            emit tx_ADB_ProcessData(signal);

            return false;
        }





        if (GlobalVars::spd_dual_imei_bool) //(buffer.toLower().contains("&"))
        {
            QJsonValue buffer1 = jsonBuffer.value("Model");

            GlobalVars::spd_imei_1 = buffer1.toString();

            reply = nam.post(req, data.toBase64());
            connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();
            if(processStartStop ==  false){ return false; }

            signal.outputString = "Getting IMEI-2 from Server: OK";
            signal.outputColor = GlobalVars::txtOutPutColor;
            signal.outputBold = false;
            signal.needtoInsertOutputBox = true;
            signal.progressBarValue = 50;
            emit tx_ADB_ProcessData(signal);

            QByteArray buffer = reply->readAll();
            reply->deleteLater();
            if(processStartStop ==  false){ return false; }
            buffer = QByteArray::fromBase64(buffer);
            qDebug()<<"\n\n\n IMEI-2 response_data::"<<buffer;
            //qDebug()<<"\n\n";

            document = QJsonDocument::fromJson(buffer);
            jsonBuffer = document.object();

            qDebug()<<" reply 2:: "<<jsonBuffer;
            qDebug()<<" isError: "<<jsonBuffer.value("isError"); //.value("AccessToken");
            qDebug()<<"\n\n ";

            if (jsonBuffer.value("isError").toBool() == true) {
                signal.outputString = "Failed to get IMEI-2 from Server.";
                signal.outputColor = Qt::red; // GlobalVars::txtOutPutColor;
                signal.outputBold = false;
                signal.needtoInsertOutputBox = true;
                signal.isProcessCompleted = true;
                signal.progressBarValue = 100;
                emit tx_ADB_ProcessData(signal);

                return false;
            }

            QJsonValue imei2 = jsonBuffer.value("Model");
            GlobalVars::spd_imei_2 = imei2.toString();
        }
        else
        {
            QJsonValue buffer1 = jsonBuffer.value("Model");
            GlobalVars::spd_imei_1 = buffer1.toString();
        }
        qDebug()<<"******ADB IMEI-1: "<<GlobalVars::spd_imei_1<<" IMEI-2:"<<GlobalVars::spd_imei_2;
    }

    return true;
}

ADB_Commands BackEndClass::sendOperationDetailsToServerToReduceCredit()
{
    const auto failed = [this]() -> ADB_Commands {
        signal.outputString = "Operation authorization failed or returned an invalid response. No device operation was started by this step.";
        signal.outputColor = Qt::red;
        signal.outputBold = true;
        signal.needtoInsertOutputBox = true;
        signal.progressBarValue = 100;
        signal.isProcessCompleted = true;
        signal.upload_LogHistory = false;
        emit tx_ADB_ProcessData(signal);
        return ADB_Idle;
    };
    if (!processStartStop) return ADB_Idle;
    if (GlobalVars::authorizedToken.isEmpty()) return failed();

    QJsonObject object;
    if (signal.currentTool == Tool_SPD) object["OperationName"] = "ADBrepair";
    else if (signal.currentTool == Tool_UnLock) object["OperationName"] = "ADBunlock";
    else return failed();
    object["DeviceName"] = GlobalVars::spd_modelStr;

    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl(GlobalVars::api_MTKcharges));
    if (request.url().scheme() != "https") return failed();
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("Authorization", GlobalVars::authorizedToken.toUtf8());
    QNetworkReply *reply = manager.post(request, QJsonDocument(object).toJson().toBase64());
    QEventLoop loop;
    QTimer deadline;
    deadline.setSingleShot(true);
    bool timedOut = false;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&deadline, &QTimer::timeout, &loop, [&]() {
        timedOut = true;
        reply->abort();
        loop.quit();
    });
    deadline.start(15000);
    if (!reply->isFinished()) loop.exec();
    deadline.stop();
    if (!processStartStop) return ADB_Idle;
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (timedOut || reply->error() != QNetworkReply::NoError || status < 200 || status >= 300)
        return failed();

    Security::ChargeResult result;
    if (!Security::parseChargeReply(QByteArray::fromBase64(reply->readAll()), &result))
        return failed();
    GlobalVars::operationID = result.operationId;
    GlobalVars::userInfo_creditDetails = result.credit;
    signal.outputString = "OperationID:" + QString::number(result.operationId, 'f', 0);
    signal.outputColor = GlobalVars::txtOutPutColor;
    signal.outputBold = false;
    signal.needtoInsertOutputBox = true;
    emit tx_ADB_ProcessData(signal);
    return ADB_SerialNumber;
}












// ---------------- Serial + MTK Slots and Methods -----------------------

void BackEndClass::rx_updateSerialPorts()
{
    //qDebug()<<" Signal received:  rx_updateSerialPorts ";

    QSerialPortInfo info;
    QList<QSerialPortInfo> ll = info.availablePorts();
    /*qDebug()<<" Total Serial Port: "<<ll.length();
    for(int i=0; i<ll.length(); i++)
    {
        qDebug()<<" Available Serial Port: "<<i<<" Name: "<<ll.at(i).portName()<<" manufact:"<<ll.at(i).manufacturer()<<" descript:"<<ll.at(i).description();
    }
    qDebug()<<"\n\n";
*/
    emit tx_newSerialPortDetected(ll);
}
