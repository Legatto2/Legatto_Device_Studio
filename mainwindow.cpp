#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QApplication>
#include <QSignalBlocker>
#include "securityhelpers.h"
#include "portselection.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->chck_LoginRememberMe->setText("Remember username only");
    ui->txt_Password->clear();
    ui->pb_FRP_SPD->setEnabled(false);
    ui->pb_FRP_SPD->setToolTip("Disabled pending a secure, validated replacement.");
    connect(ui->chck_LoginRememberMe, &QCheckBox::toggled, this, [this](bool) {
        if (settingsReady) saveNewSettings();
    });

    this->setWindowFlags(Qt::FramelessWindowHint);
    //this->setFixedSize(550, 450);
    this->setFixedSize(800, 600);

    logger.rx_initialFileFolders();

    ui->txt_IMEI_D1_1->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"))); ui->txt_IMEI_D1_2->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"))); ui->txt_IMEI_D1_2->setEnabled(false);
    ui->txt_IMEI_D2_1->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"))); ui->txt_IMEI_D2_2->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"))); ui->txt_IMEI_D2_2->setEnabled(false);
    ui->txt_IMEI_D3_1->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"))); ui->txt_IMEI_D3_2->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"))); ui->txt_IMEI_D3_2->setEnabled(false);
    ui->txt_IMEI_D4_1->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"))); ui->txt_IMEI_D4_2->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"))); ui->txt_IMEI_D4_2->setEnabled(false);
    ui->txt_IMEI_D5_1->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"))); ui->txt_IMEI_D5_2->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"))); ui->txt_IMEI_D5_2->setEnabled(false);
    ui->txt_IMEI_D6_1->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"))); ui->txt_IMEI_D6_2->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"))); ui->txt_IMEI_D6_2->setEnabled(false);
    ui->txt_IMEI_D7_1->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"))); ui->txt_IMEI_D7_2->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"))); ui->txt_IMEI_D7_2->setEnabled(false);
    ui->txt_IMEI_D8_1->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"))); ui->txt_IMEI_D8_2->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"))); ui->txt_IMEI_D8_2->setEnabled(false);


    //ui->tabWidget->setTabEnabled(1, false);

    GlobalVars::initalizeToZero();
    initializeObjectsAndStartThread();
    ui->cmb_DeviceModel_D1->setCurrentIndex(2);

    ui->processFrame->hide();
    ui->loginFrame->show();

    heading = ui->lbl_ProgramHeading;
    heading->setMouseTracking(true);
    //qDebug()<<"ID: "<<QString(QSysInfo::machineUniqueId()).toUpper();

    QByteArray unique(QSysInfo::machineUniqueId());
    //qDebug()<<" ASCII Hex: "<<unique.toHex();

    QByteArray encryptID = QCryptographicHash::hash(unique.toHex(), QCryptographicHash::Sha1);
    //qDebug()<<" Encrypted : "<<encryptID;
    //qDebug()<<" Encrypted Hex: "<<encryptID.toHex().toUpper();

    ui->lbl_SystemID->setText(encryptID.toHex().toUpper());

    ui->lbl_ProgramHeading->installEventFilter(this);

    timer_singleShot = new QTimer(this);
    connect(timer_singleShot, SIGNAL(timeout()), this, SLOT(on_timer_singleShot_Elapsed()));
    timer_singleShot->setSingleShot(true);
    timer_singleShot->start(80);
    singleShotIndex = 1;

    ui->lbl_loginServerStatus->setText("--");
    ui->lbl_loginServerStatus->setStyleSheet("color:black;");


    metaTabUniversal = ui->tabWidgetMeta->currentIndex();

    ui->lbl_Credit->setStyleSheet("color:  rgb(255, 153, 0); font:700 14pt 'Microsoft Sans Serif';");
    ui->lbl_Version->setText(QString("Version: "+QApplication::applicationVersion()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_timer_singleShot_Elapsed()
{
    if(singleShotIndex == 0)
    {
        ui->loginFrame->hide();
        ui->processFrame->show();
        this->setFixedSize(860, 600);
    }
    else if(singleShotIndex == 1)
    {
        seeSettingsFile();
    }
}


// ------------ Button Slots ----------------
void MainWindow::on_pb_Login_clicked()
{
    if (!settingsReady) {
        ui->lbl_LoginResults->setText("Settings are not ready. Resolve the settings error before login.");
        return;
    }
    if(needUpdatedSoftware ){
        QDesktopServices::openUrl(QUrl("https://m-key.org/"));
        return;
    }

    /*
    ui->loginFrame->hide();
    ui->processFrame->show();
    this->setFixedSize(860, 600);
    return;
    */
    const QByteArray postdata = Security::encodeLoginForm(
        ui->lbl_SystemID->text(), ui->txt_UsernameEmail->text(), ui->txt_Password->text());

    ui->lbl_LoginResults->setText("Wait for Server Response");

    qDebug()<<" \n\n ByteArray:"<<postdata<<"\n\n";
    QEventLoop loop;
    QNetworkAccessManager nam;
    QNetworkRequest req;
    req.setUrl(QUrl(GlobalVars::api_logURLQString));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply *reply = nam.post(req, postdata);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    //QByteArray buffer = reply->readAll();


    QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
    QJsonObject buffer = document.object();

    qDebug()<<" reply:: "<<buffer;
    qDebug()<<"\n\n IsSuccessStatusCode:"<<buffer.value("IsSuccessStatusCode");
    qDebug()<<"Access Token: "<<buffer.value("Model"); //.value("AccessToken");
    qDebug()<<"\n\n ";

    if (buffer.value("IsSuccessStatusCode") == QJsonValue(200))
    {
        QJsonValue buffer1 = buffer.value("Model");
        qDebug()<<" QJsonValue Model: "<<buffer1;
        QJsonObject bufferModel = buffer1.toObject();
        qDebug()<<"\n\n bufferModel : "<<bufferModel;
        qDebug()<<"\n\n ";

        GlobalVars::authorizedToken = bufferModel.value("AccessToken").toString();
        buffer1 = bufferModel.value("UserInfo");
        qDebug()<<" QJsonValue UserInfo: "<<buffer1;
        bufferModel = buffer1.toObject();
        qDebug()<<"\n\n bufferModel 2 : "<<bufferModel;
        qDebug()<<"\n\n ";
        GlobalVars::userInfo_creditDetails = bufferModel.value("Credit").toDouble();
        GlobalVars::userInfo_HardwareKey = bufferModel.value("MTKHardwareKey").toString();
        GlobalVars::userInfo_UserName = bufferModel.value("UserName").toString();
        //ui->lbl_LoginResults->setText("Server:" + GlobalVars::authorizedToken);

        if(ui->lbl_SystemID->text() != GlobalVars::userInfo_HardwareKey){
            ui->lbl_LoginResults->setText("HardWare unique ID mismatched.");
            ui->lbl_LoginResults->setStyleSheet("color:red;");
            return;
        }

        ui->lbl_LoginResults->setText("Login Sucessfull, Credit:" + QString::number(GlobalVars::userInfo_creditDetails, 'f', 2));
        ui->lbl_LoginResults->setStyleSheet("color:lime;");

        // ------------- Server Status Check ---------------------
        //qDebug()<<" \n\n Testing Server is online:";
        req.setUrl(QUrl(GlobalVars::api_serverStatusQString)); //+"?bearer="+GlobalVars::authorizedToken));
        req.setRawHeader(QByteArray("Authorization"), GlobalVars::authorizedToken.toUtf8());
        reply = nam.get(req); // post(req, postdata);
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        document = QJsonDocument::fromJson(reply->readAll());
        buffer = document.object();

        //qDebug()<<"Server online check reply:: "<<buffer<<"length : "<<buffer.size();
        //qDebug()<<"\n\n";

        buffer1 = buffer.value("Model");
        ui->lbl_loginServerStatus->setText(buffer1.toString());
        if(buffer1.toString().toLower().contains("onl"))
            ui->lbl_loginServerStatus->setStyleSheet("color:lime;");
        else
            ui->lbl_loginServerStatus->setStyleSheet("color:red;");

        // ------------- Version Check ---------------------
        req.setUrl(QUrl(GlobalVars::api_VersionQString)); //+"?Authorization="+GlobalVars::authorizedToken));
        //req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        req.setRawHeader(QByteArray("Authorization"), GlobalVars::authorizedToken.toUtf8());
        //qDebug()<<" Version Check: "<<req.url();
        reply = nam.get(req); //post(req, postdata);
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        document = QJsonDocument::fromJson(reply->readAll());
        buffer = document.object();
        //qDebug()<<"Version check reply:: "<<buffer<<"length : "<<buffer.size();
        //qDebug()<<"\n\n";
        buffer1 = buffer.value("Model");
        //qDebug()<<" QJsonValue Version Model: "<<buffer1;
        bufferModel = buffer1.toObject();
        qDebug()<<"\n\n bufferModel 33 : "<<bufferModel;

        GlobalVars::serverSoftwareVersion = bufferModel.value("Version").toString();
        qDebug()<<" Server Version:: "<<GlobalVars::serverSoftwareVersion;

        if(GlobalVars::serverSoftwareVersion != QApplication::applicationVersion())
        {
            ui->pb_Login->setText("Open Link");
            ui->lbl_LoginResults->setText("Please update to " + GlobalVars::serverSoftwareVersion);

            qDebug()<<" \n\n\n No need to go further, update please. \n\n\n ";
            QMessageBox msgBox;
            msgBox.setText("You are using older version of Software. Please update it.");
            msgBox.exec();
            needUpdatedSoftware = true;
            QDesktopServices::openUrl(QUrl("https://m-key.org/"));

            return;
        }


        ui->lbl_LoginResults->setText("Login Sucessfull, Credit:" + QString::number(GlobalVars::userInfo_creditDetails, 'f', 2)+",  Version:"+bufferModel.value("Version").toString());
        ui->lbl_Credit->setText("Credit: "+QString::number(GlobalVars::userInfo_creditDetails,'f', 1 ));

        timer_singleShot->start(1000);
        singleShotIndex = 0;
    }
    else
    {

        ui->lbl_LoginResults->setText("Please Check your information.");
        ui->lbl_LoginResults->setStyleSheet("color:red;");
        GlobalVars::authorizedToken = "";
    }

    if (buffer.value("Error") == true)
    {
        ui->lbl_LoginResults->setText("Error: "+buffer.value("Error").toString());
        ui->lbl_LoginResults->setStyleSheet("color:red;");
    }
    else if(buffer.contains("404 Not Found"))
    {
        ui->lbl_LoginResults->setText("Authentication File Not Found.");
        ui->lbl_LoginResults->setStyleSheet("color:red;");
    }


    saveNewSettings();
    //timer_singleShot->start(3000);
    singleShotIndex = 0;

}
void MainWindow::on_pb_Close_2_clicked()
{
    on_pb_Close_clicked();
}
void MainWindow::on_pb_Close_clicked()
{

    backEndObj->thread()->quit();
    metaModeClass[0]->thread()->quit();
    metaModeClass[1]->thread()->quit();
    metaModeClass[2]->thread()->quit();
    metaModeClass[3]->thread()->quit();
    metaModeClass[4]->thread()->quit();
    metaModeClass[5]->thread()->quit();
    metaModeClass[6]->thread()->quit();
    metaModeClass[7]->thread()->quit();
    backEndObj->thread()->wait();
    metaModeClass[0]->thread()->wait();
    metaModeClass[1]->thread()->wait();
    metaModeClass[2]->thread()->wait();
    metaModeClass[3]->thread()->wait();
    metaModeClass[4]->thread()->wait();
    metaModeClass[5]->thread()->wait();
    metaModeClass[6]->thread()->wait();
    metaModeClass[7]->thread()->wait();

    backEndObj->thread()->deleteLater();
    metaModeClass[0]->thread()->deleteLater();
    metaModeClass[1]->thread()->deleteLater();
    metaModeClass[2]->thread()->deleteLater();
    metaModeClass[3]->thread()->deleteLater();
    metaModeClass[4]->thread()->deleteLater();
    metaModeClass[5]->thread()->deleteLater();
    metaModeClass[6]->thread()->deleteLater();
    metaModeClass[7]->thread()->deleteLater();
    /*
    backEndObj->deleteLater();
    metaModeClass[0]->deleteLater();
    metaModeClass[1]->deleteLater();
    metaModeClass[2]->deleteLater();
    metaModeClass[3]->deleteLater();
    metaModeClass[4]->deleteLater();
    metaModeClass[5]->deleteLater();
    metaModeClass[6]->deleteLater();
    metaModeClass[7]->deleteLater(); */

    QApplication::quit();
}
void MainWindow::on_pb_CopyUniqueID_clicked()
{
    QClipboard *clip = QApplication::clipboard();
    clip->setText(ui->lbl_SystemID->text(), QClipboard::Clipboard);
}


void MainWindow::seeSettingsFile()
{
    settingsReady = false;
    ui->txt_Password->clear();
    Security::LoginPreferences preferences;
    QString error;
    if (!Security::loadLoginPreferences(GlobalVars::legacySettingsFilePath, &preferences, &error)) {
        ui->lbl_LoginResults->setText(error + " Login disabled; resolve the settings file and restart.");
        ui->pb_Login->setEnabled(false);
        return;
    }
    const QSignalBlocker blocker(ui->chck_LoginRememberMe);
    ui->chck_LoginRememberMe->setChecked(preferences.rememberUsername);
    ui->txt_UsernameEmail->setText(preferences.username);
    settingsReady = true;
    ui->pb_Login->setEnabled(true);
}

void MainWindow::saveNewSettings()
{
    Security::LoginPreferences preferences;
    preferences.rememberUsername = ui->chck_LoginRememberMe->isChecked();
    if (preferences.rememberUsername) preferences.username = ui->txt_UsernameEmail->text();
    QString error;
    if (!Security::saveLoginPreferences(GlobalVars::legacySettingsFilePath, preferences, &error)) {
        settingsReady = false;
        ui->pb_Login->setEnabled(false);
        ui->lbl_LoginResults->setText(error + " Preferences were not saved.");
    }
}


// ------------------------- SPD Tool -----------------
void MainWindow::on_pb_CheckADB_Devices_clicked()
{
    ui->txt_outPut_SPD->clear();

    if(checkAllOk4spdTool()){

        emit tx_updateADBdevices();

        ui->pb_Stop_SPD->setEnabled(true);
        ui->pb_StartRepair_SPD->setEnabled(false);
        ui->pb_CopytoClipBoard_SPD->setEnabled(false);
        ui->pb_CheckADB_Devices->setEnabled(false);
        ui->pb_unLock_SPD->setEnabled(false);
        ui->pb_FRP_SPD->setEnabled(false);
    }

}
void MainWindow::on_pb_StartRepair_SPD_clicked()
{
    ui->txt_outPut_SPD->clear();

    if(checkAllOk4spdTool()){
        emit tx_StartRepairing(15179, true, Tool_SPD);

        ui->pb_Stop_SPD->setEnabled(true);
        ui->pb_StartRepair_SPD->setEnabled(false);
        ui->pb_CopytoClipBoard_SPD->setEnabled(false);
        ui->pb_CheckADB_Devices->setEnabled(false);
        ui->pb_unLock_SPD->setEnabled(false);
        ui->pb_FRP_SPD->setEnabled(false);
    }
    else
    {

    }
    qDebug()<<" Dual IMEI: "<<GlobalVars::spd_dual_imei_bool;
}
void MainWindow::on_pb_unLock_SPD_clicked()
{
    ui->txt_outPut_SPD->clear();

    if(checkAllOk4spdTool()){
        emit tx_StartRepairing(15179, true, Tool_UnLock);
        ui->pb_Stop_SPD->setEnabled(true);
        ui->pb_StartRepair_SPD->setEnabled(false);
        ui->pb_CopytoClipBoard_SPD->setEnabled(false);
        ui->pb_CheckADB_Devices->setEnabled(false);
        ui->pb_unLock_SPD->setEnabled(false);
        ui->pb_FRP_SPD->setEnabled(false);
    }
    else
    {

    }
}
void MainWindow::on_pb_Stop_SPD_clicked()
{
    rx_TextBoxOutput(Tool_SPD, 0, "Process Stopped", true, false, Qt::red);

    ui->pb_Stop_SPD->setEnabled(false);
    ui->pb_StartRepair_SPD->setEnabled(true);
    ui->pb_CopytoClipBoard_SPD->setEnabled(true);
    ui->pb_CheckADB_Devices->setEnabled(true);
    ui->pb_unLock_SPD->setEnabled(true);
    ui->pb_FRP_SPD->setEnabled(false);

    emit tx_StartRepairing(15179, false, Tool_SPD);

    logger.writeToLog(ui->txt_outPut_SPD->toPlainText().toUtf8());
}
void MainWindow::on_pb_CopytoClipBoard_SPD_clicked()
{
    QClipboard *clip = QApplication::clipboard();
    clip->setText(ui->txt_outPut_SPD->toPlainText(), QClipboard::Clipboard);
}
void MainWindow::on_cmb_DeviceModel_SPD_currentIndexChanged(int index)
{
    //qDebug()<<" Device Model index :"<<index<<" txt:"<<ui->cmb_DeviceModel->currentText();

    if (index == 1) {
        GlobalVars::spd_projectName = "fijisc";
        GlobalVars::spd_projectDataType = "1";
    } else if (index == 2) {
        GlobalVars::spd_projectName = "maltasc";
        GlobalVars::spd_projectDataType  = "1";
    }else if (index == 3) {
        GlobalVars::spd_projectName = "malta";
        GlobalVars::spd_projectDataType  = "1";
    } else if (index == 4) {
        GlobalVars::spd_projectName = "fiji";
        GlobalVars::spd_projectDataType  = "1";
    } else if (index == 5) {
        GlobalVars::spd_projectName = "bj";
        GlobalVars::spd_projectDataType  = "1";
    } else if (index == 6) {
        GlobalVars::spd_projectName = "malta";
        GlobalVars::spd_projectDataType  = "1";
    }
    else
    {
        GlobalVars::spd_projectName = "java";
        GlobalVars::spd_projectDataType  = "4";
    }

    qDebug()<<" Device Model index :"<<index<<" txt:"<<ui->cmb_DeviceModel_SPD->currentText()<<" ProjectName: "<<GlobalVars::spd_projectName<<" ProjectDataType: "<<GlobalVars::spd_projectDataType;

    if(ui->cmb_DeviceModel_SPD->currentIndex()==1 || ui->cmb_DeviceModel_SPD->currentIndex()==2 || ui->cmb_DeviceModel_SPD->currentIndex()==3)
    {
        ui->txt_IMEI_1_SPD->setText("351622116227949");
        ui->txt_IMEI_2_SPD->setText("351622116227949");
    }
    else
    {
        ui->txt_IMEI_1_SPD->setText("111111111111119");
        ui->txt_IMEI_2_SPD->setText("111111111111119");
    }
}
void MainWindow::on_chk_AssignPort_stateChanged(int arg1)
{
    GlobalVars::spd_AssignPort_bool = ui->chk_AssignPort->isChecked();
}
void MainWindow::on_chk_Dual_IMEI_SPD_stateChanged(int arg1)
{
    GlobalVars::spd_dual_imei_bool = arg1;
    if(GlobalVars::spd_manual_imei_bool){
        ui->txt_IMEI_2_SPD->setEnabled(GlobalVars::spd_dual_imei_bool);
        ui->txt_IMEI_1_SPD->setEnabled(GlobalVars::spd_manual_imei_bool);
    }
}
void MainWindow::on_chk_Manual_IMEI_SPD_stateChanged(int arg1)
{
    GlobalVars::spd_manual_imei_bool= arg1;
    if(GlobalVars::spd_dual_imei_bool){
        ui->txt_IMEI_1_SPD->setEnabled(GlobalVars::spd_manual_imei_bool);
        ui->txt_IMEI_2_SPD->setEnabled(GlobalVars::spd_manual_imei_bool);
    }
    else {
        ui->txt_IMEI_1_SPD->setEnabled(GlobalVars::spd_manual_imei_bool);
    }
}
void MainWindow::on_pb_FRP_SPD_clicked()
{
    ui->pb_FRP_SPD->setEnabled(false);
    rx_TextBoxOutput(Tool_SPD, 1,
        "FRP is disabled pending a secure, validated replacement.", true, false, Qt::red);
}

// ---------------------- Public Slots -----------------------
void MainWindow::rx_ProcessCompleted(int idx, bool complt)
{
    qDebug()<<"rx_ProcessCompleted idx:"<<idx<<" complt:"<<complt;
}
void MainWindow::rx_newSerialPortDetected(QList<QSerialPortInfo> sInfo)
{
    localSerialList = sInfo;
    QStringList names, labels;
    for (const QSerialPortInfo &info : sInfo) {
        names.append(info.portName());
        labels.append(info.portName() + " " + info.manufacturer());
    }
    const QList<QComboBox *> combos = {
        ui->cmb_PortNumber_D1, ui->cmb_PortNumber_D2,
        ui->cmb_PortNumber_D3, ui->cmb_PortNumber_D4,
        ui->cmb_PortNumber_D5, ui->cmb_PortNumber_D6,
        ui->cmb_PortNumber_D7, ui->cmb_PortNumber_D8
    };
    for (QComboBox *combo : combos) refreshPortSelection(combo, names, labels);
}

void MainWindow::rx_ADB_ProcessData(signalStructure sig)
{
    //qDebug()<<"rx_ADB_ProcessData:: needToInsert: "<<sig.needtoInsertOutputBox<<" str: "<<sig.outputString<<" Tool:"<<sig.currentTool;

    if(sig.needtoInsertOutputBox)
        rx_TextBoxOutput(sig.currentTool, sig.toolIndex, sig.outputString, sig.outputBold, sig.outputNewLine, sig.outputColor);

    ui->processProgress->setValue(sig.progressBarValue);
    if(sig.progressBarValue > 30 && sig.progressBarValue < 60) {
        ui->txt_IMEI_1_SPD->setText(GlobalVars::spd_imei_1);
        ui->txt_IMEI_2_SPD->setText(GlobalVars::spd_imei_2);
    }

    if(sig.isProcessCompleted){
        qDebug()<<" \n\n\t Operation Completed with value: 222, ";

        ui->pb_Stop_SPD->setEnabled(false);
        ui->pb_StartRepair_SPD->setEnabled(true);
        ui->pb_CopytoClipBoard_SPD->setEnabled(true);
        ui->pb_CheckADB_Devices->setEnabled(true);
        ui->pb_unLock_SPD->setEnabled(true);
        ui->pb_FRP_SPD->setEnabled(false);
        ui->lbl_Credit->setText("Credit: "+QString::number(GlobalVars::userInfo_creditDetails,'f', 1 ));
        if(sig.upload_LogHistory)
            logger.writeToLog(ui->txt_outPut_SPD->toPlainText().toUtf8());
    }

}

void MainWindow::rx_TextBoxOutput(TOOL_TYPE tool, int idx, QString s, bool isBold, bool newline, QColor color)
{
    //qDebug()<<"rx_TextBoxOutput Tool:"<<tool<<" idx:"<<idx<<" QString:"<<s<<" isBold:"<<isBold<<" NewLine:"<<newline<<" Color:"<<color;

    if(tool == Tool_MTK){
        switch (idx) {
        case 0:{
            if(newline) ui->txt_outPut_D1->append("\n");
            ui->txt_outPut_D1->setTextColor(color);
            if(isBold) ui->txt_outPut_D1->setFontWeight(QFont::Bold);
            else ui->txt_outPut_D1->setFontWeight(QFont::Light);
            ui->txt_outPut_D1->append(s);
            break;
        }
        case 1:{
            if(newline) ui->txt_outPut_D2->append("\n");
            ui->txt_outPut_D2->setTextColor(color);
            if(isBold) ui->txt_outPut_D2->setFontWeight(QFont::Bold);
            else ui->txt_outPut_D2->setFontWeight(QFont::Light);
            ui->txt_outPut_D2->append(s);
            break;
        }
        case 2:{
            if(newline) ui->txt_outPut_D3->append("\n");
            ui->txt_outPut_D3->setTextColor(color);
            if(isBold) ui->txt_outPut_D3->setFontWeight(QFont::Bold);
            else ui->txt_outPut_D3->setFontWeight(QFont::Light);
            ui->txt_outPut_D3->append(s);
            break;
        }
        case 3:{
            if(newline) ui->txt_outPut_D4->append("\n");
            ui->txt_outPut_D4->setTextColor(color);
            if(isBold) ui->txt_outPut_D4->setFontWeight(QFont::Bold);
            else ui->txt_outPut_D4->setFontWeight(QFont::Light);
            ui->txt_outPut_D4->append(s);
            break;
        }
        case 4:{
            if(newline) ui->txt_outPut_D5->append("\n");
            ui->txt_outPut_D5->setTextColor(color);
            if(isBold) ui->txt_outPut_D5->setFontWeight(QFont::Bold);
            else ui->txt_outPut_D5->setFontWeight(QFont::Light);
            ui->txt_outPut_D5->append(s);
            break;
        }
        case 5:{
            if(newline) ui->txt_outPut_D6->append("\n");
            ui->txt_outPut_D6->setTextColor(color);
            if(isBold) ui->txt_outPut_D6->setFontWeight(QFont::Bold);
            else ui->txt_outPut_D6->setFontWeight(QFont::Light);
            ui->txt_outPut_D6->append(s);
            break;
        }
        case 6:{
            if(newline) ui->txt_outPut_D7->append("\n");
            ui->txt_outPut_D7->setTextColor(color);
            if(isBold) ui->txt_outPut_D7->setFontWeight(QFont::Bold);
            else ui->txt_outPut_D7->setFontWeight(QFont::Light);
            ui->txt_outPut_D7->append(s);
            break;
        }
        case 7:{
            if(newline) ui->txt_outPut_D8->append("\n");
            ui->txt_outPut_D8->setTextColor(color);
            if(isBold) ui->txt_outPut_D8->setFontWeight(QFont::Bold);
            else ui->txt_outPut_D8->setFontWeight(QFont::Light);
            ui->txt_outPut_D8->append(s);
            break;
        }
        }
    }
    else if(tool==Tool_SPD || tool==Tool_UnLock || tool==Tool_SPD_FRP_FastBoot){
        //qDebug()<<"rx_TextBoxOutput Tool:"<<tool<<" idx:"<<idx<<" QString:"<<s;
        if(newline) ui->txt_outPut_SPD->append("\n");
        ui->txt_outPut_SPD->setTextColor(color);
        if(isBold) ui->txt_outPut_SPD->setFontWeight(QFont::Bold);
        else ui->txt_outPut_SPD->setFontWeight(QFont::Light);
        ui->txt_outPut_SPD->append(s);
    }

}
void MainWindow::rx_miscOperations(TOOL_TYPE tool, int idx, int value, QString str)
{
    if(idx == 1){
        ui->processProgress->setValue(value);
        if(value > 30 && value < 60) {
            ui->txt_IMEI_1_SPD->setText(GlobalVars::spd_imei_1);
            ui->txt_IMEI_2_SPD->setText(GlobalVars::spd_imei_2);
        }
        if(value >= 100) {

        }
    }

    /*
    if(tool == Tool_MTK || tool == Tool_MTK_UnLock){
        switch (idx) {
        case 0:{
            ui->progress_D1->setValue(value);
            if(value >= 100) {
                ui->pb_Stop_D1->setEnabled(false);
                ui->pb_StartRepairing_D1->setEnabled(true);
                ui->pb_CopytoClipBoard_D1->setEnabled(true);
                ui->pb_CheckSerialPorts_D1->setEnabled(true);
                ui->pb_unLock_D1->setEnabled(true);
                logger.writeToLog(ui->txt_outPut_D1->toPlainText().toUtf8());
            }
            break;
        }
        case 1:{
            ui->progress_D2->setValue(value);
            if(value >= 100) {
                ui->pb_Stop_D2->setEnabled(false);
                ui->pb_StartRepairing_D2->setEnabled(true);
                ui->pb_CopytoClipBoard_D2->setEnabled(true);
                ui->pb_CheckSerialPorts_D2->setEnabled(true);
                ui->pb_unLock_D2->setEnabled(true);
                logger.writeToLog(ui->txt_outPut_D2->toPlainText().toUtf8());
            }
            break;
        }
        case 2:{
            ui->progress_D3->setValue(value);
            if(value >= 100) {
                ui->pb_Stop_D3->setEnabled(false);
                ui->pb_StartRepairing_D3->setEnabled(true);
                ui->pb_CopytoClipBoard_D3->setEnabled(true);
                ui->pb_CheckSerialPorts_D3->setEnabled(true);
                logger.writeToLog(ui->txt_outPut_D3->toPlainText().toUtf8());
            }
            break;
        }
        case 3:{
            ui->progress_D4->setValue(value);
            if(value >= 100) {
                ui->pb_Stop_D4->setEnabled(false);
                ui->pb_StartRepairing_D4->setEnabled(true);
                ui->pb_CopytoClipBoard_D4->setEnabled(true);
                ui->pb_CheckSerialPorts_D4->setEnabled(true);
                logger.writeToLog(ui->txt_outPut_D4->toPlainText().toUtf8());
            }
            break;
        }
        case 4:{
            ui->progress_D5->setValue(value);
            if(value >= 100) {
                ui->pb_Stop_D5->setEnabled(false);
                ui->pb_StartRepairing_D5->setEnabled(true);
                ui->pb_CopytoClipBoard_D5->setEnabled(true);
                ui->pb_CheckSerialPorts_D5->setEnabled(true);
                logger.writeToLog(ui->txt_outPut_D5->toPlainText().toUtf8());
            }
            break;
        }
        case 5:{
            ui->progress_D6->setValue(value);
            if(value >= 100) {
                ui->pb_Stop_D6->setEnabled(false);
                ui->pb_StartRepairing_D6->setEnabled(true);
                ui->pb_CopytoClipBoard_D6->setEnabled(true);
                ui->pb_CheckSerialPorts_D6->setEnabled(true);
                logger.writeToLog(ui->txt_outPut_D6->toPlainText().toUtf8());
            }
            break;
        }
        case 6:{
            ui->progress_D7->setValue(value);
            if(value >= 100) {
                ui->pb_Stop_D7->setEnabled(false);
                ui->pb_StartRepairing_D7->setEnabled(true);
                ui->pb_CopytoClipBoard_D7->setEnabled(true);
                ui->pb_CheckSerialPorts_D7->setEnabled(true);
                logger.writeToLog(ui->txt_outPut_D7->toPlainText().toUtf8());
            }
            break;
        }
        case 7:{
            ui->progress_D8->setValue(value);
            if(value >= 100) {
                ui->pb_Stop_D8->setEnabled(false);
                ui->pb_StartRepairing_D8->setEnabled(true);
                ui->pb_CopytoClipBoard_D8->setEnabled(true);
                ui->pb_CheckSerialPorts_D8->setEnabled(true);
                logger.writeToLog(ui->txt_outPut_D8->toPlainText().toUtf8());
            }
            break;
        }
        } // end of Switch

    }else if(tool == Tool_SPD || tool == Tool_UnLock) {
        if(idx == 1){
            ui->processProgress->setValue(value);
            if(value >= 100) {
                ui->pb_Stop_SPD->setEnabled(false);
                ui->pb_StartRepair_SPD->setEnabled(true);
                ui->pb_CopytoClipBoard_SPD->setEnabled(true);
                ui->pb_CheckADB_Devices->setEnabled(true);
                ui->pb_unLock_SPD->setEnabled(true);

                logger.writeToLog(ui->txt_outPut_SPD->toPlainText().toUtf8());
            }
        }
        if(idx == 11){
            // ADB device
            //qDebug()<<"rx_miscOperations  device: "<<value<<" Name: "<<str;
        }
    }
    */

}

void MainWindow::rx_miscOperations_metaMode(TOOL_TYPE tool, int idx, int value, QString str)
{
    switch (idx) {
    case 0:{
        ui->progress_D1->setValue(value);
        if(value > 30 && value < 60) {
            ui->txt_IMEI_D1_1->setText(GlobalVars::meta_imei_1[idx]);
            ui->txt_IMEI_D1_2->setText(GlobalVars::meta_imei_2[idx]);
        }


        if(value >= 100) {
            ui->pb_Stop_D1->setEnabled(false);
            ui->pb_StartRepairing_D1->setEnabled(true);
            ui->pb_CopytoClipBoard_D1->setEnabled(true);
            ui->pb_CheckSerialPorts_D1->setEnabled(true);
            ui->pb_unLock_D1->setEnabled(true);
            ui->pb_carrierFix_D1->setEnabled(true);
            logger.writeToLog(ui->txt_outPut_D1->toPlainText().toUtf8());
        }
        break;
    }
    case 1:{
        ui->progress_D2->setValue(value);
        if(value > 30 && value < 60) {
            ui->txt_IMEI_D2_1->setText(GlobalVars::meta_imei_1[idx]);
            ui->txt_IMEI_D2_2->setText(GlobalVars::meta_imei_2[idx]);
        }
        if(value >= 100) {
            ui->pb_Stop_D2->setEnabled(false);
            ui->pb_StartRepairing_D2->setEnabled(true);
            ui->pb_CopytoClipBoard_D2->setEnabled(true);
            ui->pb_CheckSerialPorts_D2->setEnabled(true);
            ui->pb_unLock_D2->setEnabled(true);

            logger.writeToLog(ui->txt_outPut_D2->toPlainText().toUtf8());
        }
        break;
    }
    case 2:{
        ui->progress_D3->setValue(value);
        if(value > 30 && value < 60) {
            ui->txt_IMEI_D3_1->setText(GlobalVars::meta_imei_1[idx]);
            ui->txt_IMEI_D3_2->setText(GlobalVars::meta_imei_2[idx]);
        }
        if(value >= 100) {
            ui->pb_Stop_D3->setEnabled(false);
            ui->pb_StartRepairing_D3->setEnabled(true);
            ui->pb_CopytoClipBoard_D3->setEnabled(true);
            ui->pb_CheckSerialPorts_D3->setEnabled(true);
            ui->pb_unLock_D3->setEnabled(true);
            logger.writeToLog(ui->txt_outPut_D3->toPlainText().toUtf8());
        }
        break;
    }
    case 3:{
        ui->progress_D4->setValue(value);
        if(value > 30 && value < 60) {
            ui->txt_IMEI_D4_1->setText(GlobalVars::meta_imei_1[idx]);
            ui->txt_IMEI_D4_2->setText(GlobalVars::meta_imei_2[idx]);
        }
        if(value >= 100) {
            ui->pb_Stop_D4->setEnabled(false);
            ui->pb_StartRepairing_D4->setEnabled(true);
            ui->pb_CopytoClipBoard_D4->setEnabled(true);
            ui->pb_CheckSerialPorts_D4->setEnabled(true);
            ui->pb_unLock_D4->setEnabled(true);

            logger.writeToLog(ui->txt_outPut_D4->toPlainText().toUtf8());
        }
        break;
    }
    case 4:{
        ui->progress_D5->setValue(value);
        if(value > 30 && value < 60) {
            ui->txt_IMEI_D5_1->setText(GlobalVars::meta_imei_1[idx]);
            ui->txt_IMEI_D5_2->setText(GlobalVars::meta_imei_2[idx]);
        }
        if(value >= 100) {
            ui->pb_Stop_D5->setEnabled(false);
            ui->pb_StartRepairing_D5->setEnabled(true);
            ui->pb_CopytoClipBoard_D5->setEnabled(true);
            ui->pb_CheckSerialPorts_D5->setEnabled(true);
            ui->pb_unLock_D5->setEnabled(true);

            logger.writeToLog(ui->txt_outPut_D5->toPlainText().toUtf8());
        }
        break;
    }
    case 5:{
        ui->progress_D6->setValue(value);
        if(value > 30 && value < 60) {
            ui->txt_IMEI_D6_1->setText(GlobalVars::meta_imei_1[idx]);
            ui->txt_IMEI_D6_2->setText(GlobalVars::meta_imei_2[idx]);
        }
        if(value >= 100) {
            ui->pb_Stop_D6->setEnabled(false);
            ui->pb_StartRepairing_D6->setEnabled(true);
            ui->pb_CopytoClipBoard_D6->setEnabled(true);
            ui->pb_CheckSerialPorts_D6->setEnabled(true);
            ui->pb_unLock_D6->setEnabled(true);

            logger.writeToLog(ui->txt_outPut_D6->toPlainText().toUtf8());
        }
        break;
    }
    case 6:{
        ui->progress_D7->setValue(value);
        if(value > 30 && value < 60) {
            ui->txt_IMEI_D7_1->setText(GlobalVars::meta_imei_1[idx]);
            ui->txt_IMEI_D7_2->setText(GlobalVars::meta_imei_2[idx]);
        }
        if(value >= 100) {
            ui->pb_Stop_D7->setEnabled(false);
            ui->pb_StartRepairing_D7->setEnabled(true);
            ui->pb_CopytoClipBoard_D7->setEnabled(true);
            ui->pb_CheckSerialPorts_D7->setEnabled(true);
            ui->pb_unLock_D7->setEnabled(true);

            logger.writeToLog(ui->txt_outPut_D7->toPlainText().toUtf8());
        }
        break;
    }
    case 7:{
        ui->progress_D8->setValue(value);
        if(value > 30 && value < 60) {
            ui->txt_IMEI_D8_1->setText(GlobalVars::meta_imei_1[idx]);
            ui->txt_IMEI_D8_2->setText(GlobalVars::meta_imei_2[idx]);
        }
        if(value >= 100) {
            ui->pb_Stop_D8->setEnabled(false);
            ui->pb_StartRepairing_D8->setEnabled(true);
            ui->pb_CopytoClipBoard_D8->setEnabled(true);
            ui->pb_CheckSerialPorts_D8->setEnabled(true);
            ui->pb_unLock_D8->setEnabled(true);
            logger.writeToLog(ui->txt_outPut_D8->toPlainText().toUtf8());
        }
        break;
    }
    } // end of Switch

    if(tool==Tool_MTK && value>=100 && str=="Success" && GlobalVars::meta_unLock_bool[idx])
    {
        rx_TextBoxOutput_metaMode(Tool_MTK, idx, "\n", false, false);
        emit tx_StartRepairing_metaMode(idx, true, Tool_MTK_UnLock);
    }
    ui->lbl_Credit->setText("Credit: "+QString::number(GlobalVars::userInfo_creditDetails,'f', 1 ));
}
void MainWindow::rx_TextBoxOutput_metaMode(TOOL_TYPE tool, int idx, QString s, bool isBold, bool newline, QColor color)
{
    // qDebug()<<"rx_TextBoxOutput_metaMode idx:"<<idx<<" QString:"<<s<<" isBold:"<<isBold<<" NewLine:"<<newline<<" Color:"<<color;
    switch (idx) {
    case 0:{
        if(newline) ui->txt_outPut_D1->append("\n");
        ui->txt_outPut_D1->setTextColor(color);
        if(isBold) ui->txt_outPut_D1->setFontWeight(QFont::Bold);
        else ui->txt_outPut_D1->setFontWeight(QFont::Light);
        ui->txt_outPut_D1->append(s);
        break;
    }
    case 1:{
        if(newline) ui->txt_outPut_D2->append("\n");
        ui->txt_outPut_D2->setTextColor(color);
        if(isBold) ui->txt_outPut_D2->setFontWeight(QFont::Bold);
        else ui->txt_outPut_D2->setFontWeight(QFont::Light);
        ui->txt_outPut_D2->append(s);
        break;
    }
    case 2:{
        if(newline) ui->txt_outPut_D3->append("\n");
        ui->txt_outPut_D3->setTextColor(color);
        if(isBold) ui->txt_outPut_D3->setFontWeight(QFont::Bold);
        else ui->txt_outPut_D3->setFontWeight(QFont::Light);
        ui->txt_outPut_D3->append(s);
        break;
    }
    case 3:{
        if(newline) ui->txt_outPut_D4->append("\n");
        ui->txt_outPut_D4->setTextColor(color);
        if(isBold) ui->txt_outPut_D4->setFontWeight(QFont::Bold);
        else ui->txt_outPut_D4->setFontWeight(QFont::Light);
        ui->txt_outPut_D4->append(s);
        break;
    }
    case 4:{
        if(newline) ui->txt_outPut_D5->append("\n");
        ui->txt_outPut_D5->setTextColor(color);
        if(isBold) ui->txt_outPut_D5->setFontWeight(QFont::Bold);
        else ui->txt_outPut_D5->setFontWeight(QFont::Light);
        ui->txt_outPut_D5->append(s);
        break;
    }
    case 5:{
        if(newline) ui->txt_outPut_D6->append("\n");
        ui->txt_outPut_D6->setTextColor(color);
        if(isBold) ui->txt_outPut_D6->setFontWeight(QFont::Bold);
        else ui->txt_outPut_D6->setFontWeight(QFont::Light);
        ui->txt_outPut_D6->append(s);
        break;
    }
    case 6:{
        if(newline) ui->txt_outPut_D7->append("\n");
        ui->txt_outPut_D7->setTextColor(color);
        if(isBold) ui->txt_outPut_D7->setFontWeight(QFont::Bold);
        else ui->txt_outPut_D7->setFontWeight(QFont::Light);
        ui->txt_outPut_D7->append(s);
        break;
    }
    case 7:{
        if(newline) ui->txt_outPut_D8->append("\n");
        ui->txt_outPut_D8->setTextColor(color);
        if(isBold) ui->txt_outPut_D8->setFontWeight(QFont::Bold);
        else ui->txt_outPut_D8->setFontWeight(QFont::Light);
        ui->txt_outPut_D8->append(s);
        break;
    }
    }
}


// ---------- Misc methods -----------------
void MainWindow::initializeObjectsAndStartThread()
{
    backEndObj = new BackEndClass();
    thrd  = new QThread(backEndObj);
    backEndObj->moveToThread(thrd);
    //connect(backEndObj, SIGNAL(tx_TextBoxOutput(TOOL_TYPE, int,QString,bool,bool,QColor)), this, SLOT(rx_TextBoxOutput(TOOL_TYPE, int, QString,bool,bool,QColor)));
    //connect(backEndObj, SIGNAL(tx_miscOperations(TOOL_TYPE,int,int,QString)), this, SLOT(rx_miscOperations(TOOL_TYPE,int,int,QString)));

    connect(backEndObj, SIGNAL(tx_ADB_ProcessData(signalStructure)), this, SLOT(rx_ADB_ProcessData(signalStructure)));

    connect(backEndObj, SIGNAL(tx_newSerialPortDetected(QList<QSerialPortInfo>)), this, SLOT(rx_newSerialPortDetected(QList<QSerialPortInfo>)));
    connect(this, SIGNAL(tx_StartRepairing(int, bool,TOOL_TYPE)), backEndObj, SLOT(rx_StartRepairing(int, bool,TOOL_TYPE)));
    connect(this, SIGNAL(tx_updateSerialPorts()), backEndObj, SLOT(rx_updateSerialPorts()));
    connect(this, SIGNAL(tx_updateADBdevices()), backEndObj, SLOT(rx_updateADBdevices()));

    thrd->start(QThread::NormalPriority);

    int localIndx = 0;
    metaModeClass[localIndx] = new MetaModeThread(); metaThread[localIndx] = new QThread(metaModeClass[localIndx]); metaModeClass[localIndx]->moveToThread(metaThread[localIndx]); metaModeClass[localIndx]->setGeneralIndex_metaMode(localIndx);
    connect(metaModeClass[localIndx], SIGNAL(tx_TextBoxOutput_metaMode(TOOL_TYPE,int,QString,bool,bool,QColor)), this, SLOT(rx_TextBoxOutput_metaMode(TOOL_TYPE,int, QString,bool,bool,QColor)));
    connect(metaModeClass[localIndx], SIGNAL(tx_miscOperations_metaMode(TOOL_TYPE,int,int,QString)), this, SLOT(rx_miscOperations_metaMode(TOOL_TYPE,int,int,QString)));
    connect(this, SIGNAL(tx_StartRepairing_metaMode(int, bool,TOOL_TYPE)), metaModeClass[localIndx], SLOT(rx_StartRepairing_metaMode(int, bool,TOOL_TYPE)));
    metaThread[localIndx]->start(QThread::NormalPriority);

    localIndx = 1;
    metaModeClass[localIndx] = new MetaModeThread(); metaThread[localIndx] = new QThread(metaModeClass[localIndx]); metaModeClass[localIndx]->moveToThread(metaThread[localIndx]); metaModeClass[localIndx]->setGeneralIndex_metaMode(localIndx);
    connect(metaModeClass[localIndx], SIGNAL(tx_TextBoxOutput_metaMode(TOOL_TYPE,int,QString,bool,bool,QColor)), this, SLOT(rx_TextBoxOutput_metaMode(TOOL_TYPE,int, QString,bool,bool,QColor)));
    connect(metaModeClass[localIndx], SIGNAL(tx_miscOperations_metaMode(TOOL_TYPE,int,int,QString)), this, SLOT(rx_miscOperations_metaMode(TOOL_TYPE,int,int,QString)));
    connect(this, SIGNAL(tx_StartRepairing_metaMode(int, bool,TOOL_TYPE)), metaModeClass[localIndx], SLOT(rx_StartRepairing_metaMode(int, bool,TOOL_TYPE)));
    metaThread[localIndx]->start(QThread::NormalPriority);

    localIndx = 2;
    metaModeClass[localIndx] = new MetaModeThread(); metaThread[localIndx] = new QThread(metaModeClass[localIndx]); metaModeClass[localIndx]->moveToThread(metaThread[localIndx]); metaModeClass[localIndx]->setGeneralIndex_metaMode(localIndx);
    connect(metaModeClass[localIndx], SIGNAL(tx_TextBoxOutput_metaMode(TOOL_TYPE,int,QString,bool,bool,QColor)), this, SLOT(rx_TextBoxOutput_metaMode(TOOL_TYPE,int, QString,bool,bool,QColor)));
    connect(metaModeClass[localIndx], SIGNAL(tx_miscOperations_metaMode(TOOL_TYPE,int,int,QString)), this, SLOT(rx_miscOperations_metaMode(TOOL_TYPE,int,int,QString)));
    connect(this, SIGNAL(tx_StartRepairing_metaMode(int, bool,TOOL_TYPE)), metaModeClass[localIndx], SLOT(rx_StartRepairing_metaMode(int, bool,TOOL_TYPE)));
    metaThread[localIndx]->start(QThread::NormalPriority);

    localIndx = 3;
    metaModeClass[localIndx] = new MetaModeThread(); metaThread[localIndx] = new QThread(metaModeClass[localIndx]); metaModeClass[localIndx]->moveToThread(metaThread[localIndx]); metaModeClass[localIndx]->setGeneralIndex_metaMode(localIndx);
    connect(metaModeClass[localIndx], SIGNAL(tx_TextBoxOutput_metaMode(TOOL_TYPE,int,QString,bool,bool,QColor)), this, SLOT(rx_TextBoxOutput_metaMode(TOOL_TYPE,int, QString,bool,bool,QColor)));
    connect(metaModeClass[localIndx], SIGNAL(tx_miscOperations_metaMode(TOOL_TYPE,int,int,QString)), this, SLOT(rx_miscOperations_metaMode(TOOL_TYPE,int,int,QString)));
    connect(this, SIGNAL(tx_StartRepairing_metaMode(int, bool,TOOL_TYPE)), metaModeClass[localIndx], SLOT(rx_StartRepairing_metaMode(int, bool,TOOL_TYPE)));
    metaThread[localIndx]->start(QThread::NormalPriority);

    localIndx = 4;
    metaModeClass[localIndx] = new MetaModeThread(); metaThread[localIndx] = new QThread(metaModeClass[localIndx]); metaModeClass[localIndx]->moveToThread(metaThread[localIndx]); metaModeClass[localIndx]->setGeneralIndex_metaMode(localIndx);
    connect(metaModeClass[localIndx], SIGNAL(tx_TextBoxOutput_metaMode(TOOL_TYPE,int,QString,bool,bool,QColor)), this, SLOT(rx_TextBoxOutput_metaMode(TOOL_TYPE,int, QString,bool,bool,QColor)));
    connect(metaModeClass[localIndx], SIGNAL(tx_miscOperations_metaMode(TOOL_TYPE,int,int,QString)), this, SLOT(rx_miscOperations_metaMode(TOOL_TYPE,int,int,QString)));
    connect(this, SIGNAL(tx_StartRepairing_metaMode(int, bool,TOOL_TYPE)), metaModeClass[localIndx], SLOT(rx_StartRepairing_metaMode(int, bool,TOOL_TYPE)));
    metaThread[localIndx]->start(QThread::NormalPriority);

    localIndx = 5;
    metaModeClass[localIndx] = new MetaModeThread(); metaThread[localIndx] = new QThread(metaModeClass[localIndx]); metaModeClass[localIndx]->moveToThread(metaThread[localIndx]); metaModeClass[localIndx]->setGeneralIndex_metaMode(localIndx);
    connect(metaModeClass[localIndx], SIGNAL(tx_TextBoxOutput_metaMode(TOOL_TYPE,int,QString,bool,bool,QColor)), this, SLOT(rx_TextBoxOutput_metaMode(TOOL_TYPE,int, QString,bool,bool,QColor)));
    connect(metaModeClass[localIndx], SIGNAL(tx_miscOperations_metaMode(TOOL_TYPE,int,int,QString)), this, SLOT(rx_miscOperations_metaMode(TOOL_TYPE,int,int,QString)));
    connect(this, SIGNAL(tx_StartRepairing_metaMode(int, bool,TOOL_TYPE)), metaModeClass[localIndx], SLOT(rx_StartRepairing_metaMode(int, bool,TOOL_TYPE)));
    metaThread[localIndx]->start(QThread::NormalPriority);

    localIndx = 6;
    metaModeClass[localIndx] = new MetaModeThread(); metaThread[localIndx] = new QThread(metaModeClass[localIndx]); metaModeClass[localIndx]->moveToThread(metaThread[localIndx]); metaModeClass[localIndx]->setGeneralIndex_metaMode(localIndx);
    connect(metaModeClass[localIndx], SIGNAL(tx_TextBoxOutput_metaMode(TOOL_TYPE,int,QString,bool,bool,QColor)), this, SLOT(rx_TextBoxOutput_metaMode(TOOL_TYPE,int, QString,bool,bool,QColor)));
    connect(metaModeClass[localIndx], SIGNAL(tx_miscOperations_metaMode(TOOL_TYPE,int,int,QString)), this, SLOT(rx_miscOperations_metaMode(TOOL_TYPE,int,int,QString)));
    connect(this, SIGNAL(tx_StartRepairing_metaMode(int, bool,TOOL_TYPE)), metaModeClass[localIndx], SLOT(rx_StartRepairing_metaMode(int, bool,TOOL_TYPE)));
    metaThread[localIndx]->start(QThread::NormalPriority);

    localIndx = 7;
    metaModeClass[localIndx] = new MetaModeThread(); metaThread[localIndx] = new QThread(metaModeClass[localIndx]); metaModeClass[localIndx]->moveToThread(metaThread[localIndx]); metaModeClass[localIndx]->setGeneralIndex_metaMode(localIndx);
    connect(metaModeClass[localIndx], SIGNAL(tx_TextBoxOutput_metaMode(TOOL_TYPE,int,QString,bool,bool,QColor)), this, SLOT(rx_TextBoxOutput_metaMode(TOOL_TYPE,int, QString,bool,bool,QColor)));
    connect(metaModeClass[localIndx], SIGNAL(tx_miscOperations_metaMode(TOOL_TYPE,int,int,QString)), this, SLOT(rx_miscOperations_metaMode(TOOL_TYPE,int,int,QString)));
    connect(this, SIGNAL(tx_StartRepairing_metaMode(int, bool,TOOL_TYPE)), metaModeClass[localIndx], SLOT(rx_StartRepairing_metaMode(int, bool,TOOL_TYPE)));
    metaThread[localIndx]->start(QThread::NormalPriority);



}
bool MainWindow::checkAllOk4spdTool()
{
    if(ui->txt_IMEI_1_SPD->text().length() != 15)
    {
        QMessageBox::critical(this, "IMEI Error", "Please enter a valid 15 digit IMEI-1 code.", QMessageBox::Ok);
        return false;
    }
    if(GlobalVars::spd_dual_imei_bool){
        if(ui->txt_IMEI_2_SPD->text().length() != 15)
        {
            QMessageBox::critical(this, "IMEI Error", "Please enter a valid 15 digit IMEI-2 code.", QMessageBox::Ok);
            return false;
        }
    }
    if (ui->cmb_DeviceModel_SPD->currentIndex() <= 0)
    {
        rx_TextBoxOutput(Tool_SPD, 0, "Please Select Correct Model", true, false, Qt::yellow);
        return false;
    }

    GlobalVars::spd_dual_imei_bool =  ui->chk_Dual_IMEI_SPD->isChecked();
    GlobalVars::spd_AssignPort_bool = ui->chk_AssignPort->isChecked();
    GlobalVars::spd_imei_1 =  ui->txt_IMEI_1_SPD->text();
    GlobalVars::spd_imei_2 =  ui->txt_IMEI_2_SPD->text();

    GlobalVars::spd_modelStr = ui->cmb_DeviceModel_SPD->currentText();
    GlobalVars::spd_modelIndex = ui->cmb_DeviceModel_SPD->currentIndex();
    return true;
}
bool MainWindow::checkAllOk4MetaMode(int device)
{
    const QList<QComboBox *> combos = {
        ui->cmb_PortNumber_D1, ui->cmb_PortNumber_D2,
        ui->cmb_PortNumber_D3, ui->cmb_PortNumber_D4,
        ui->cmb_PortNumber_D5, ui->cmb_PortNumber_D6,
        ui->cmb_PortNumber_D7, ui->cmb_PortNumber_D8
    };
    if (device < 0 || device >= combos.size()) return false;
    const int selected = combos.at(device)->currentIndex();
    if (selected < 0 || selected >= localSerialList.size()
        || combos.at(device)->currentData().toString() != localSerialList.at(selected).portName()) {
        rx_TextBoxOutput(Tool_MTK, device, "Select a currently connected serial port.", true, false, Qt::yellow);
        return false;
    }
    switch (device) {
    case 0: {
        if(ui->txt_IMEI_D1_1->text().length() != 15) { QMessageBox::critical(this, "IMEI Error", "Please enter a valid 15 digit IMEI-1 code.", QMessageBox::Ok); return false; }
        if(GlobalVars::meta_dual_imei_bool[device]){
            if(ui->txt_IMEI_D1_2->text().length() != 15)
            { QMessageBox::critical(this, "IMEI Error", "Please enter a valid 15 digit IMEI-2 code.", QMessageBox::Ok); return false; }
        }
        if (ui->cmb_DeviceModel_D1->currentIndex() <= 0) { rx_TextBoxOutput(Tool_MTK, device, "Please Select Correct Model", true, false, Qt::yellow); return false; }
        if (ui->cmb_PortNumber_D1->count()<=0) { rx_TextBoxOutput(Tool_MTK, device, "Please Select Correct Serial Port", true, false, Qt::yellow); return false; }

        GlobalVars::meta_reboot_bool[device] =  ui->chk_Reboot_D1->isChecked();
        GlobalVars::meta_imei_1[device] =  ui->txt_IMEI_D1_1->text();
        GlobalVars::meta_imei_2[device] =  ui->txt_IMEI_D1_2->text();
        GlobalVars::meta_modelStr[device] = ui->cmb_DeviceModel_D1->currentText();
        GlobalVars::meta_modelIndex[device] = ui->cmb_DeviceModel_D1->currentIndex();
        GlobalVars::meta_serialPortName[device] = localSerialList.at(ui->cmb_PortNumber_D1->currentIndex()).portName().right(localSerialList.at(ui->cmb_PortNumber_D1->currentIndex()).portName().length() - 3); //ui->cmb_PortNumber->currentText();
        GlobalVars::meta_serialIndex[device] = ui->cmb_PortNumber_D1->currentIndex();

        ui->pb_Stop_D1->setEnabled(true);
        ui->pb_StartRepairing_D1->setEnabled(false);
        ui->pb_CopytoClipBoard_D1->setEnabled(false);
        ui->pb_CheckSerialPorts_D1->setEnabled(false);
        ui->pb_unLock_D1->setEnabled(false);
        ui->pb_carrierFix_D1->setEnabled(false);
        break;
    }
    case 1: {
        if(ui->txt_IMEI_D2_1->text().length() != 15) { QMessageBox::critical(this, "IMEI Error", "Please enter a valid 15 digit IMEI-1 code.", QMessageBox::Ok); return false; }
        if(GlobalVars::meta_dual_imei_bool[device]){
            if(ui->txt_IMEI_D2_2->text().length() != 15)
            { QMessageBox::critical(this, "IMEI Error", "Please enter a valid 15 digit IMEI-2 code.", QMessageBox::Ok); return false; }
        }
        if (ui->cmb_DeviceModel_D2->currentIndex() <= 0) { rx_TextBoxOutput(Tool_MTK, device, "Please Select Correct Model", true, false, Qt::yellow); return false; }
        if (ui->cmb_PortNumber_D2->count()<=0) { rx_TextBoxOutput(Tool_MTK, device, "Please Select Correct Serial Port", true, false, Qt::yellow); return false; }

        GlobalVars::meta_reboot_bool[device] =  ui->chk_Reboot_D2->isChecked();
        GlobalVars::meta_imei_1[device] =  ui->txt_IMEI_D2_1->text();
        GlobalVars::meta_imei_2[device] =  ui->txt_IMEI_D2_2->text();
        GlobalVars::meta_modelStr[device] = ui->cmb_DeviceModel_D2->currentText();
        GlobalVars::meta_modelIndex[device] = ui->cmb_DeviceModel_D2->currentIndex();
        GlobalVars::meta_serialPortName[device] = localSerialList.at(ui->cmb_PortNumber_D2->currentIndex()).portName().right(localSerialList.at(ui->cmb_PortNumber_D2->currentIndex()).portName().length() - 3); //ui->cmb_PortNumber->currentText();
        GlobalVars::meta_serialIndex[device] = ui->cmb_PortNumber_D2->currentIndex();
        qDebug()<<" All ok in checkAllOk4MetaMode:: "<<device;

        ui->pb_Stop_D2->setEnabled(true);
        ui->pb_StartRepairing_D2->setEnabled(false);
        ui->pb_CopytoClipBoard_D2->setEnabled(false);
        ui->pb_CheckSerialPorts_D2->setEnabled(false);
        ui->pb_unLock_D2->setEnabled(false);
        break;
    }
    case 2: {
        if(ui->txt_IMEI_D3_1->text().length() != 15) { QMessageBox::critical(this, "IMEI Error", "Please enter a valid 15 digit IMEI-1 code.", QMessageBox::Ok); return false; }
        if(GlobalVars::meta_dual_imei_bool[device]){
            if(ui->txt_IMEI_D3_2->text().length() != 15)
            { QMessageBox::critical(this, "IMEI Error", "Please enter a valid 15 digit IMEI-2 code.", QMessageBox::Ok); return false; }
        }
        if (ui->cmb_DeviceModel_D3->currentIndex() <= 0) { rx_TextBoxOutput(Tool_MTK, device, "Please Select Correct Model", true, false, Qt::yellow); return false; }
        if (ui->cmb_PortNumber_D3->count()<=0) { rx_TextBoxOutput(Tool_MTK, device, "Please Select Correct Serial Port", true, false, Qt::yellow); return false; }

        GlobalVars::meta_reboot_bool[device] =  ui->chk_Reboot_D3->isChecked();
        GlobalVars::meta_imei_1[device] =  ui->txt_IMEI_D3_1->text();
        GlobalVars::meta_imei_2[device] =  ui->txt_IMEI_D3_2->text();
        GlobalVars::meta_modelStr[device] = ui->cmb_DeviceModel_D3->currentText();
        GlobalVars::meta_modelIndex[device] = ui->cmb_DeviceModel_D3->currentIndex();
        GlobalVars::meta_serialPortName[device] = localSerialList.at(ui->cmb_PortNumber_D3->currentIndex()).portName().right(localSerialList.at(ui->cmb_PortNumber_D3->currentIndex()).portName().length() - 3);
        GlobalVars::meta_serialIndex[device] = ui->cmb_PortNumber_D3->currentIndex();

        ui->pb_Stop_D3->setEnabled(true);
        ui->pb_StartRepairing_D3->setEnabled(false);
        ui->pb_CopytoClipBoard_D3->setEnabled(false);
        ui->pb_CheckSerialPorts_D3->setEnabled(false);
        ui->pb_unLock_D3->setEnabled(false);
        break;
    }
    case 3: {
        if(ui->txt_IMEI_D4_1->text().length() != 15) { QMessageBox::critical(this, "IMEI Error", "Please enter a valid 15 digit IMEI-1 code.", QMessageBox::Ok); return false; }
        if(GlobalVars::meta_dual_imei_bool[device]){
            if(ui->txt_IMEI_D4_2->text().length() != 15) { QMessageBox::critical(this, "IMEI Error", "Please enter a valid 15 digit IMEI-2 code.", QMessageBox::Ok); return false; }
        }
        if (ui->cmb_DeviceModel_D4->currentIndex() <= 0) { rx_TextBoxOutput(Tool_MTK, device, "Please Select Correct Model", true, false, Qt::yellow); return false; }
        if (ui->cmb_PortNumber_D4->count()<=0) { rx_TextBoxOutput(Tool_MTK, device, "Please Select Correct Serial Port", true, false, Qt::yellow); return false; }

        GlobalVars::meta_reboot_bool[device] =  ui->chk_Reboot_D4->isChecked();
        GlobalVars::meta_imei_1[device] =  ui->txt_IMEI_D4_1->text();
        GlobalVars::meta_imei_2[device] =  ui->txt_IMEI_D4_2->text();
        GlobalVars::meta_modelStr[device] = ui->cmb_DeviceModel_D4->currentText();
        GlobalVars::meta_modelIndex[device] = ui->cmb_DeviceModel_D4->currentIndex();
        GlobalVars::meta_serialPortName[device] = localSerialList.at(ui->cmb_PortNumber_D4->currentIndex()).portName().right(localSerialList.at(ui->cmb_PortNumber_D4->currentIndex()).portName().length() - 3); //ui->cmb_PortNumber->currentText();
        GlobalVars::meta_serialIndex[device] = ui->cmb_PortNumber_D4->currentIndex();

        ui->pb_Stop_D4->setEnabled(true);
        ui->pb_StartRepairing_D4->setEnabled(false);
        ui->pb_CopytoClipBoard_D4->setEnabled(false);
        ui->pb_CheckSerialPorts_D4->setEnabled(false);
        ui->pb_unLock_D4->setEnabled(false);
        break;
    }
    case 4: {
        if(ui->txt_IMEI_D5_1->text().length() != 15) { QMessageBox::critical(this, "IMEI Error", "Please enter a valid 15 digit IMEI-1 code.", QMessageBox::Ok); return false; }
        if(GlobalVars::meta_dual_imei_bool[device]){
            if(ui->txt_IMEI_D5_2->text().length() != 15)
            { QMessageBox::critical(this, "IMEI Error", "Please enter a valid 15 digit IMEI-2 code.", QMessageBox::Ok); return false; }
        }
        if (ui->cmb_DeviceModel_D5->currentIndex() <= 0) { rx_TextBoxOutput(Tool_MTK, device, "Please Select Correct Model", true, false, Qt::yellow); return false; }
        if (ui->cmb_PortNumber_D5->count()<=0) { rx_TextBoxOutput(Tool_MTK, device, "Please Select Correct Serial Port", true, false, Qt::yellow); return false; }

        GlobalVars::meta_reboot_bool[device] =  ui->chk_Reboot_D5->isChecked();
        GlobalVars::meta_imei_1[device] =  ui->txt_IMEI_D5_1->text();
        GlobalVars::meta_imei_2[device] =  ui->txt_IMEI_D5_2->text();
        GlobalVars::meta_modelStr[device] = ui->cmb_DeviceModel_D5->currentText();
        GlobalVars::meta_modelIndex[device] = ui->cmb_DeviceModel_D5->currentIndex();
        GlobalVars::meta_serialPortName[device] = localSerialList.at(ui->cmb_PortNumber_D5->currentIndex()).portName().right(localSerialList.at(ui->cmb_PortNumber_D5->currentIndex()).portName().length() - 3); //ui->cmb_PortNumber->currentText();
        GlobalVars::meta_serialIndex[device] = ui->cmb_PortNumber_D5->currentIndex();

        ui->pb_Stop_D5->setEnabled(true);
        ui->pb_StartRepairing_D5->setEnabled(false);
        ui->pb_CopytoClipBoard_D5->setEnabled(false);
        ui->pb_CheckSerialPorts_D5->setEnabled(false);
        ui->pb_unLock_D5->setEnabled(false);
        break;
    }
    case 5: {
        if(ui->txt_IMEI_D6_1->text().length() != 15) { QMessageBox::critical(this, "IMEI Error", "Please enter a valid 15 digit IMEI-1 code.", QMessageBox::Ok); return false; }
        if(GlobalVars::meta_dual_imei_bool[device]){
            if(ui->txt_IMEI_D6_2->text().length() != 15)
            { QMessageBox::critical(this, "IMEI Error", "Please enter a valid 15 digit IMEI-2 code.", QMessageBox::Ok); return false; }
        }
        if (ui->cmb_DeviceModel_D6->currentIndex() <= 0) { rx_TextBoxOutput(Tool_MTK, device, "Please Select Correct Model", true, false, Qt::yellow); return false; }
        if (ui->cmb_PortNumber_D6->count()<=0) { rx_TextBoxOutput(Tool_MTK, device, "Please Select Correct Serial Port", true, false, Qt::yellow); return false; }

        GlobalVars::meta_reboot_bool[device] =  ui->chk_Reboot_D6->isChecked();
        GlobalVars::meta_imei_1[device] =  ui->txt_IMEI_D6_1->text();
        GlobalVars::meta_imei_2[device] =  ui->txt_IMEI_D6_2->text();
        GlobalVars::meta_modelStr[device] = ui->cmb_DeviceModel_D6->currentText();
        GlobalVars::meta_modelIndex[device] = ui->cmb_DeviceModel_D6->currentIndex();
        GlobalVars::meta_serialPortName[device] = localSerialList.at(ui->cmb_PortNumber_D6->currentIndex()).portName().right(localSerialList.at(ui->cmb_PortNumber_D6->currentIndex()).portName().length() - 3); //ui->cmb_PortNumber->currentText();
        GlobalVars::meta_serialIndex[device] = ui->cmb_PortNumber_D6->currentIndex();

        ui->pb_Stop_D6->setEnabled(true);
        ui->pb_StartRepairing_D6->setEnabled(false);
        ui->pb_CopytoClipBoard_D6->setEnabled(false);
        ui->pb_CheckSerialPorts_D6->setEnabled(false);
        ui->pb_unLock_D6->setEnabled(false);
        break;
    }
    case 6: {
        if(ui->txt_IMEI_D7_1->text().length() != 15) { QMessageBox::critical(this, "IMEI Error", "Please enter a valid 15 digit IMEI-1 code.", QMessageBox::Ok); return false; }
        if(GlobalVars::meta_dual_imei_bool[device]){
            if(ui->txt_IMEI_D7_2->text().length() != 15)
            { QMessageBox::critical(this, "IMEI Error", "Please enter a valid 15 digit IMEI-2 code.", QMessageBox::Ok); return false; }
        }
        if (ui->cmb_DeviceModel_D7->currentIndex() <= 0) { rx_TextBoxOutput(Tool_MTK, device, "Please Select Correct Model", true, false, Qt::yellow); return false; }
        if (ui->cmb_PortNumber_D7->count()<=0) { rx_TextBoxOutput(Tool_MTK, device, "Please Select Correct Serial Port", true, false, Qt::yellow); return false; }

        GlobalVars::meta_reboot_bool[device] =  ui->chk_Reboot_D7->isChecked();
        GlobalVars::meta_imei_1[device] =  ui->txt_IMEI_D7_1->text();
        GlobalVars::meta_imei_2[device] =  ui->txt_IMEI_D7_2->text();
        GlobalVars::meta_modelStr[device] = ui->cmb_DeviceModel_D7->currentText();
        GlobalVars::meta_modelIndex[device] = ui->cmb_DeviceModel_D7->currentIndex();
        GlobalVars::meta_serialPortName[device] = localSerialList.at(ui->cmb_PortNumber_D7->currentIndex()).portName().right(localSerialList.at(ui->cmb_PortNumber_D7->currentIndex()).portName().length() - 3); //ui->cmb_PortNumber->currentText();
        GlobalVars::meta_serialIndex[device] = ui->cmb_PortNumber_D7->currentIndex();

        ui->pb_Stop_D7->setEnabled(true);
        ui->pb_StartRepairing_D7->setEnabled(false);
        ui->pb_CopytoClipBoard_D7->setEnabled(false);
        ui->pb_CheckSerialPorts_D7->setEnabled(false);
        ui->pb_unLock_D7->setEnabled(false);
        break;
    }
    case 7: {
        if(ui->txt_IMEI_D8_1->text().length() != 15) { QMessageBox::critical(this, "IMEI Error", "Please enter a valid 15 digit IMEI-1 code.", QMessageBox::Ok); return false; }
        if(GlobalVars::meta_dual_imei_bool[device]){
            if(ui->txt_IMEI_D8_2->text().length() != 15)
            { QMessageBox::critical(this, "IMEI Error", "Please enter a valid 15 digit IMEI-2 code.", QMessageBox::Ok); return false; }
        }
        if (ui->cmb_DeviceModel_D8->currentIndex() <= 0) { rx_TextBoxOutput(Tool_MTK, device, "Please Select Correct Model", true, false, Qt::yellow); return false; }
        if (ui->cmb_PortNumber_D8->count()<=0) { rx_TextBoxOutput(Tool_MTK, device, "Please Select Correct Serial Port", true, false, Qt::yellow); return false; }

        GlobalVars::meta_reboot_bool[device] =  ui->chk_Reboot_D8->isChecked();
        GlobalVars::meta_imei_1[device] =  ui->txt_IMEI_D8_1->text();
        GlobalVars::meta_imei_2[device] =  ui->txt_IMEI_D8_2->text();
        GlobalVars::meta_modelStr[device] = ui->cmb_DeviceModel_D8->currentText();
        GlobalVars::meta_modelIndex[device] = ui->cmb_DeviceModel_D8->currentIndex();
        GlobalVars::meta_serialPortName[device] = localSerialList.at(ui->cmb_PortNumber_D8->currentIndex()).portName().right(localSerialList.at(ui->cmb_PortNumber_D8->currentIndex()).portName().length() - 3); //ui->cmb_PortNumber->currentText();
        GlobalVars::meta_serialIndex[device] = ui->cmb_PortNumber_D8->currentIndex();

        ui->pb_Stop_D8->setEnabled(true);
        ui->pb_StartRepairing_D8->setEnabled(false);
        ui->pb_CopytoClipBoard_D8->setEnabled(false);
        ui->pb_CheckSerialPorts_D8->setEnabled(false);
        ui->pb_unLock_D8->setEnabled(false);
        break;
    }
    }
    return true;
}

void MainWindow::on_tabWidgetMeta_currentChanged(int index)
{
    qDebug()<<" Tab Index changed: "<<index;
    metaTabUniversal = index;
}

// ------------------------ For Moveing MainWindow on when click and drag on main Heading ------------------
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == heading) {
        if (event->type() == QEvent::MouseButtonPress) { buttonIsPressed = true; }
        if( event->type() == QEvent::MouseButtonRelease) { buttonIsPressed = false; }
        return QMainWindow::eventFilter(obj, event);
    } else {
        // pass the event on to the parent class
        return QMainWindow::eventFilter(obj, event);
    }
}
void MainWindow::mousePressEvent(QMouseEvent *event) {
    m_nMouseClick_X_Coordinate = event->x();
    m_nMouseClick_Y_Coordinate = event->y();
}
void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    if(buttonIsPressed) move(event->globalX()-m_nMouseClick_X_Coordinate,event->globalY()-m_nMouseClick_Y_Coordinate);
}


// ----------------------------- Device 1 ------------------------
void MainWindow::on_pb_CheckSerialPorts_D1_clicked()
{
    emit tx_updateSerialPorts();
}
void MainWindow::on_pb_CopytoClipBoard_D1_clicked()
{
    QClipboard *clip = QApplication::clipboard();
    clip->setText(ui->txt_outPut_D1->toPlainText(), QClipboard::Clipboard);
}
void MainWindow::on_chk_Dual_IMEI_D1_stateChanged(int arg1)
{
    GlobalVars::meta_dual_imei_bool[metaTabUniversal] = ui->chk_Dual_IMEI_D1->isChecked();
    if(GlobalVars::meta_manual_imei_bool[metaTabUniversal]){
        ui->txt_IMEI_D1_2->setEnabled(GlobalVars::meta_dual_imei_bool[metaTabUniversal]);
        ui->txt_IMEI_D1_1->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
    }
}
void MainWindow::on_pb_Stop_D1_clicked()
{
    rx_TextBoxOutput(Tool_MTK, metaTabUniversal, "Process Stopped", true, false, Qt::red);
    ui->pb_Stop_D1->setEnabled(false);
    ui->pb_StartRepairing_D1->setEnabled(true);
    ui->pb_CopytoClipBoard_D1->setEnabled(true);
    ui->pb_CheckSerialPorts_D1->setEnabled(true);
    ui->pb_unLock_D1->setEnabled(true);
    ui->pb_carrierFix_D1->setEnabled(true);
    emit tx_StartRepairing_metaMode(metaTabUniversal, false, Tool_MTK);
    logger.writeToLog(ui->txt_outPut_D1->toPlainText().toUtf8());
}
void MainWindow::on_pb_StartRepairing_D1_clicked()
{
    ui->txt_outPut_D1->clear();
    if(checkAllOk4MetaMode(metaTabUniversal)){
        emit tx_StartRepairing_metaMode(metaTabUniversal, true, Tool_MTK);
    }
    //qDebug()<<" Dual IMEI: "<<GlobalVars::dual_imei_bool;
}
void MainWindow::on_chk_Reboot_D1_stateChanged(int arg1)
{
    GlobalVars::meta_reboot_bool[metaTabUniversal] = ui->chk_Reboot_D1->isChecked();
}
void MainWindow::on_cmb_PortNumber_D1_currentIndexChanged(int index)
{
    //GlobalVars::serialPortName[0] = localSerialList.at(index).portName();
}
void MainWindow::on_cmb_DeviceModel_D1_currentIndexChanged(int index)
{
    //qDebug()<<" META Device Model index :"<<index<<" txt:"<<ui->cmb_DeviceModel->currentText();
    if (index == 1) //
    {
        GlobalVars::meta_projectName[metaTabUniversal] = "fiji";
        GlobalVars::meta_projectDataType[metaTabUniversal] = "1";
        GlobalVars::meta_projectUnlockType[metaTabUniversal]  = "2";
    }
    else if (index == 2)
    {
        GlobalVars::meta_projectName[metaTabUniversal] = "bj";
        GlobalVars::meta_projectDataType[metaTabUniversal]  = "1";
        GlobalVars::meta_projectUnlockType[metaTabUniversal]  = "2";
    }
    else
    {
        GlobalVars::meta_projectName[metaTabUniversal] = "malta";
        GlobalVars::meta_projectDataType[metaTabUniversal]  = "4";
        GlobalVars::meta_projectUnlockType[metaTabUniversal]  = "5";
    }
    qDebug()<<" Device Model index :"<<index<<" txt:"<<ui->cmb_DeviceModel_D2->currentText()<<" ProjectName: "<<GlobalVars::meta_projectName[metaTabUniversal]<<" ProjectDataType: "<<GlobalVars::meta_projectDataType[metaTabUniversal]<<" SIM_Unlock:"<<GlobalVars::meta_projectUnlockType[metaTabUniversal];
}


// ----------------------------- Device 2 ------------------------
void MainWindow::on_pb_CheckSerialPorts_D2_clicked()
{    
    emit tx_updateSerialPorts();
}
void MainWindow::on_chk_Dual_IMEI_D2_stateChanged(int arg1)
{
    GlobalVars::meta_dual_imei_bool[metaTabUniversal] = ui->chk_Dual_IMEI_D2->isChecked();
    if(GlobalVars::meta_manual_imei_bool[metaTabUniversal]){
        ui->txt_IMEI_D2_2->setEnabled(GlobalVars::meta_dual_imei_bool[metaTabUniversal]);
        ui->txt_IMEI_D2_1->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
    }
}
void MainWindow::on_pb_StartRepairing_D2_clicked()
{
    ui->txt_outPut_D2->clear();
    if(checkAllOk4MetaMode(metaTabUniversal)){
        emit tx_StartRepairing_metaMode(metaTabUniversal, true, Tool_MTK);
    }
}
void MainWindow::on_pb_Stop_D2_clicked()
{
    rx_TextBoxOutput(Tool_MTK, metaTabUniversal, "Process Stopped", true, false, Qt::red);
    ui->pb_Stop_D2->setEnabled(false);
    ui->pb_StartRepairing_D2->setEnabled(true);
    ui->pb_CopytoClipBoard_D2->setEnabled(true);
    ui->pb_CheckSerialPorts_D2->setEnabled(true);
    ui->pb_unLock_D2->setEnabled(true);
    emit tx_StartRepairing_metaMode(metaTabUniversal, false, Tool_MTK);
    logger.writeToLog(ui->txt_outPut_D2->toPlainText().toUtf8());
}
void MainWindow::on_pb_CopytoClipBoard_D2_clicked()
{
    QClipboard *clip = QApplication::clipboard();
    clip->setText(ui->txt_outPut_D2->toPlainText(), QClipboard::Clipboard);
}
void MainWindow::on_chk_Reboot_D2_stateChanged(int arg1)
{

}
void MainWindow::on_cmb_PortNumber_D2_currentIndexChanged(int index)
{

}
void MainWindow::on_cmb_DeviceModel_D2_currentIndexChanged(int index)
{
    //qDebug()<<" META Device Model index :"<<index<<" txt:"<<ui->cmb_DeviceModel->currentText();
    if (index == 1) //
    {
        GlobalVars::meta_projectName[metaTabUniversal] = "fiji";
        GlobalVars::meta_projectDataType[metaTabUniversal] = "1";
        GlobalVars::meta_projectUnlockType[metaTabUniversal]  = "2";
    }
    else if (index == 2)
    {
        GlobalVars::meta_projectName[metaTabUniversal] = "bj";
        GlobalVars::meta_projectDataType[metaTabUniversal]  = "1";
        GlobalVars::meta_projectUnlockType[metaTabUniversal]  = "2";
    }
    else
    {
        GlobalVars::meta_projectName[metaTabUniversal] = "malta";
        GlobalVars::meta_projectDataType[metaTabUniversal]  = "4";
        GlobalVars::meta_projectUnlockType[metaTabUniversal]  = "5";
    }

    qDebug()<<" Device Model index :"<<index<<" txt:"<<ui->cmb_DeviceModel_D2->currentText()<<" ProjectName: "<<GlobalVars::meta_projectName[metaTabUniversal]<<" ProjectDataType: "<<GlobalVars::meta_projectDataType[metaTabUniversal]<<" SIM_Unlock:"<<GlobalVars::meta_projectUnlockType[metaTabUniversal];
}


// ----------------------------- Device 3 ------------------------
void MainWindow::on_pb_CheckSerialPorts_D3_clicked()
{
    emit tx_updateSerialPorts();
}
void MainWindow::on_chk_Dual_IMEI_D3_stateChanged(int arg1)
{
    GlobalVars::meta_dual_imei_bool[metaTabUniversal] = ui->chk_Dual_IMEI_D3->isChecked();
    if(GlobalVars::meta_manual_imei_bool[metaTabUniversal]){
        ui->txt_IMEI_D3_2->setEnabled(GlobalVars::meta_dual_imei_bool[metaTabUniversal]);
        ui->txt_IMEI_D3_1->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
    }
}
void MainWindow::on_chk_Reboot_D3_stateChanged(int arg1)
{

}
void MainWindow::on_pb_StartRepairing_D3_clicked()
{
    ui->txt_outPut_D3->clear();
    if(checkAllOk4MetaMode(metaTabUniversal)){
        emit tx_StartRepairing_metaMode(metaTabUniversal, true, Tool_MTK);
    }
}
void MainWindow::on_pb_Stop_D3_clicked()
{
    rx_TextBoxOutput(Tool_MTK, metaTabUniversal, "Process Stopped", true, false, Qt::red);
    ui->pb_Stop_D3->setEnabled(false);
    ui->pb_StartRepairing_D3->setEnabled(true);
    ui->pb_CopytoClipBoard_D3->setEnabled(true);
    ui->pb_CheckSerialPorts_D3->setEnabled(true);
    ui->pb_unLock_D3->setEnabled(true);
    emit tx_StartRepairing_metaMode(metaTabUniversal, false, Tool_MTK);
    logger.writeToLog(ui->txt_outPut_D3->toPlainText().toUtf8());
}
void MainWindow::on_pb_CopytoClipBoard_D3_clicked()
{
    QClipboard *clip = QApplication::clipboard();
    clip->setText(ui->txt_outPut_D3->toPlainText(), QClipboard::Clipboard);
}
void MainWindow::on_cmb_PortNumber_D3_currentIndexChanged(int index)
{

}
void MainWindow::on_cmb_DeviceModel_D3_currentIndexChanged(int index)
{
    //qDebug()<<" META Device Model index :"<<index<<" txt:"<<ui->cmb_DeviceModel->currentText();
    if (index == 1) //
    {
        GlobalVars::meta_projectName[metaTabUniversal] = "fiji";
        GlobalVars::meta_projectDataType[metaTabUniversal] = "1";
        GlobalVars::meta_projectUnlockType[metaTabUniversal]  = "2";
    }
    else if (index == 2)
    {
        GlobalVars::meta_projectName[metaTabUniversal] = "bj";
        GlobalVars::meta_projectDataType[metaTabUniversal]  = "1";
        GlobalVars::meta_projectUnlockType[metaTabUniversal]  = "2";
    }
    else
    {
        GlobalVars::meta_projectName[metaTabUniversal] = "malta";
        GlobalVars::meta_projectDataType[metaTabUniversal]  = "4";
        GlobalVars::meta_projectUnlockType[metaTabUniversal]  = "5";
    }
}

// ----------------------------- Device 4 ------------------------
void MainWindow::on_pb_CheckSerialPorts_D4_clicked()
{
    emit tx_updateSerialPorts();
}
void MainWindow::on_chk_Dual_IMEI_D4_stateChanged(int arg1)
{
    GlobalVars::meta_dual_imei_bool[metaTabUniversal] = ui->chk_Dual_IMEI_D4->isChecked();
    if(GlobalVars::meta_manual_imei_bool[metaTabUniversal]){
        ui->txt_IMEI_D4_2->setEnabled(GlobalVars::meta_dual_imei_bool[metaTabUniversal]);
        ui->txt_IMEI_D4_1->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
    }
}
void MainWindow::on_chk_Reboot_D4_stateChanged(int arg1)
{

}
void MainWindow::on_pb_StartRepairing_D4_clicked()
{
    ui->txt_outPut_D4->clear();
    if(checkAllOk4MetaMode(metaTabUniversal)){
        emit tx_StartRepairing_metaMode(metaTabUniversal, true, Tool_MTK);
    }
}
void MainWindow::on_pb_Stop_D4_clicked()
{
    rx_TextBoxOutput(Tool_MTK, metaTabUniversal, "Process Stopped", true, false, Qt::red);
    ui->pb_Stop_D4->setEnabled(false);
    ui->pb_StartRepairing_D4->setEnabled(true);
    ui->pb_CopytoClipBoard_D4->setEnabled(true);
    ui->pb_CheckSerialPorts_D4->setEnabled(true);
    ui->pb_unLock_D4->setEnabled(true);
    emit tx_StartRepairing_metaMode(metaTabUniversal, false, Tool_MTK);
    logger.writeToLog(ui->txt_outPut_D4->toPlainText().toUtf8());
}
void MainWindow::on_pb_CopytoClipBoard_D4_clicked()
{
    QClipboard *clip = QApplication::clipboard();
    clip->setText(ui->txt_outPut_D4->toPlainText(), QClipboard::Clipboard);
}
void MainWindow::on_cmb_PortNumber_D4_currentIndexChanged(int index)
{

}
void MainWindow::on_cmb_DeviceModel_D4_currentIndexChanged(int index)
{
    //qDebug()<<" META Device Model index :"<<index<<" txt:"<<ui->cmb_DeviceModel->currentText();
    if (index == 1) //
    {
        GlobalVars::meta_projectName[metaTabUniversal] = "fiji";
        GlobalVars::meta_projectDataType[metaTabUniversal] = "1";
        GlobalVars::meta_projectUnlockType[metaTabUniversal]  = "2";
    }
    else if (index == 2)
    {
        GlobalVars::meta_projectName[metaTabUniversal] = "bj";
        GlobalVars::meta_projectDataType[metaTabUniversal]  = "1";
        GlobalVars::meta_projectUnlockType[metaTabUniversal]  = "2";
    }
    else
    {
        GlobalVars::meta_projectName[metaTabUniversal] = "malta";
        GlobalVars::meta_projectDataType[metaTabUniversal]  = "4";
        GlobalVars::meta_projectUnlockType[metaTabUniversal]  = "5";
    }
}

// ----------------------------- Device 5 ------------------------
void MainWindow::on_pb_CheckSerialPorts_D5_clicked()
{
    emit tx_updateSerialPorts();
}
void MainWindow::on_chk_Dual_IMEI_D5_stateChanged(int arg1)
{
    GlobalVars::meta_dual_imei_bool[metaTabUniversal] = ui->chk_Dual_IMEI_D5->isChecked();
    if(GlobalVars::meta_manual_imei_bool[metaTabUniversal]){
        ui->txt_IMEI_D5_2->setEnabled(GlobalVars::meta_dual_imei_bool[metaTabUniversal]);
        ui->txt_IMEI_D5_1->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
    }
}
void MainWindow::on_chk_Reboot_D5_stateChanged(int arg1)
{

}
void MainWindow::on_pb_StartRepairing_D5_clicked()
{
    ui->txt_outPut_D5->clear();
    if(checkAllOk4MetaMode(metaTabUniversal)){
        emit tx_StartRepairing_metaMode(metaTabUniversal, true, Tool_MTK);
    }
}
void MainWindow::on_pb_Stop_D5_clicked()
{
    rx_TextBoxOutput(Tool_MTK, metaTabUniversal, "Process Stopped", true, false, Qt::red);
    ui->pb_Stop_D5->setEnabled(false);
    ui->pb_StartRepairing_D5->setEnabled(true);
    ui->pb_CopytoClipBoard_D5->setEnabled(true);
    ui->pb_CheckSerialPorts_D5->setEnabled(true);
    ui->pb_unLock_D5->setEnabled(true);
    emit tx_StartRepairing_metaMode(metaTabUniversal, false, Tool_MTK);
    logger.writeToLog(ui->txt_outPut_D5->toPlainText().toUtf8());
}
void MainWindow::on_pb_CopytoClipBoard_D5_clicked()
{
    QClipboard *clip = QApplication::clipboard();
    clip->setText(ui->txt_outPut_D5->toPlainText(), QClipboard::Clipboard);
}
void MainWindow::on_cmb_PortNumber_D5_currentIndexChanged(int index)
{

}
void MainWindow::on_cmb_DeviceModel_D5_currentIndexChanged(int index)
{
    //qDebug()<<" META Device Model index :"<<index<<" txt:"<<ui->cmb_DeviceModel->currentText();
    if (index == 1) //
    {
        GlobalVars::meta_projectName[metaTabUniversal] = "fiji";
        GlobalVars::meta_projectDataType[metaTabUniversal] = "1";
        GlobalVars::meta_projectUnlockType[metaTabUniversal]  = "2";
    }
    else if (index == 2)
    {
        GlobalVars::meta_projectName[metaTabUniversal] = "bj";
        GlobalVars::meta_projectDataType[metaTabUniversal]  = "1";
        GlobalVars::meta_projectUnlockType[metaTabUniversal]  = "2";
    }
    else
    {
        GlobalVars::meta_projectName[metaTabUniversal] = "malta";
        GlobalVars::meta_projectDataType[metaTabUniversal]  = "4";
        GlobalVars::meta_projectUnlockType[metaTabUniversal]  = "5";
    }
}


// ----------------------------- Device 6 ------------------------
void MainWindow::on_pb_CheckSerialPorts_D6_clicked()
{
    emit tx_updateSerialPorts();
}
void MainWindow::on_chk_Dual_IMEI_D6_stateChanged(int arg1)
{
    GlobalVars::meta_dual_imei_bool[metaTabUniversal] = ui->chk_Dual_IMEI_D6->isChecked();
    if(GlobalVars::meta_manual_imei_bool[metaTabUniversal]){
        ui->txt_IMEI_D6_2->setEnabled(GlobalVars::meta_dual_imei_bool[metaTabUniversal]);
        ui->txt_IMEI_D6_1->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
    }
}
void MainWindow::on_chk_Reboot_D6_stateChanged(int arg1)
{

}
void MainWindow::on_pb_StartRepairing_D6_clicked()
{
    ui->txt_outPut_D6->clear();
    if(checkAllOk4MetaMode(metaTabUniversal)){
        emit tx_StartRepairing_metaMode(metaTabUniversal, true, Tool_MTK);
    }
}
void MainWindow::on_pb_Stop_D6_clicked()
{
    rx_TextBoxOutput(Tool_MTK, metaTabUniversal, "Process Stopped", true, false, Qt::red);
    ui->pb_Stop_D5->setEnabled(false);
    ui->pb_StartRepairing_D6->setEnabled(true);
    ui->pb_CopytoClipBoard_D6->setEnabled(true);
    ui->pb_CheckSerialPorts_D6->setEnabled(true);
    ui->pb_unLock_D6->setEnabled(true);
    emit tx_StartRepairing_metaMode(metaTabUniversal, false, Tool_MTK);
    logger.writeToLog(ui->txt_outPut_D5->toPlainText().toUtf8());
}
void MainWindow::on_pb_CopytoClipBoard_D6_clicked()
{
    QClipboard *clip = QApplication::clipboard();
    clip->setText(ui->txt_outPut_D6->toPlainText(), QClipboard::Clipboard);
}
void MainWindow::on_cmb_PortNumber_D6_currentIndexChanged(int index)
{

}
void MainWindow::on_cmb_DeviceModel_D6_currentIndexChanged(int index)
{
    //qDebug()<<" META Device Model index :"<<index<<" txt:"<<ui->cmb_DeviceModel->currentText();
    if (index == 1) //
    {
        GlobalVars::meta_projectName[metaTabUniversal] = "fiji";
        GlobalVars::meta_projectDataType[metaTabUniversal] = "1";
        GlobalVars::meta_projectUnlockType[metaTabUniversal]  = "2";
    }
    else if (index == 2)
    {
        GlobalVars::meta_projectName[metaTabUniversal] = "bj";
        GlobalVars::meta_projectDataType[metaTabUniversal]  = "1";
        GlobalVars::meta_projectUnlockType[metaTabUniversal]  = "2";
    }
    else
    {
        GlobalVars::meta_projectName[metaTabUniversal] = "malta";
        GlobalVars::meta_projectDataType[metaTabUniversal]  = "4";
        GlobalVars::meta_projectUnlockType[metaTabUniversal]  = "5";
    }
}


// ----------------------------- Device 7 ------------------------
void MainWindow::on_pb_CheckSerialPorts_D7_clicked()
{
    emit tx_updateSerialPorts();
}
void MainWindow::on_chk_Dual_IMEI_D7_stateChanged(int arg1)
{
    GlobalVars::meta_dual_imei_bool[metaTabUniversal] = ui->chk_Dual_IMEI_D7->isChecked();
    if(GlobalVars::meta_manual_imei_bool[metaTabUniversal]){
        ui->txt_IMEI_D7_2->setEnabled(GlobalVars::meta_dual_imei_bool[metaTabUniversal]);
        ui->txt_IMEI_D7_1->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
    }
}
void MainWindow::on_chk_Reboot_D7_stateChanged(int arg1)
{

}
void MainWindow::on_pb_StartRepairing_D7_clicked()
{
    ui->txt_outPut_D7->clear();
    if(checkAllOk4MetaMode(metaTabUniversal)){
        emit tx_StartRepairing_metaMode(metaTabUniversal, true, Tool_MTK);
    }
}
void MainWindow::on_pb_Stop_D7_clicked()
{
    rx_TextBoxOutput(Tool_MTK, metaTabUniversal, "Process Stopped", true, false, Qt::red);
    ui->pb_Stop_D7->setEnabled(false);
    ui->pb_StartRepairing_D7->setEnabled(true);
    ui->pb_CopytoClipBoard_D7->setEnabled(true);
    ui->pb_CheckSerialPorts_D7->setEnabled(true);
    ui->pb_unLock_D7->setEnabled(true);
    emit tx_StartRepairing_metaMode(metaTabUniversal, false, Tool_MTK);
    logger.writeToLog(ui->txt_outPut_D7->toPlainText().toUtf8());
}
void MainWindow::on_pb_CopytoClipBoard_D7_clicked()
{
    QClipboard *clip = QApplication::clipboard();
    clip->setText(ui->txt_outPut_D7->toPlainText(), QClipboard::Clipboard);
}
void MainWindow::on_cmb_PortNumber_D7_currentIndexChanged(int index)
{

}
void MainWindow::on_cmb_DeviceModel_D7_currentIndexChanged(int index)
{
    //qDebug()<<" META Device Model index :"<<index<<" txt:"<<ui->cmb_DeviceModel->currentText();
    if (index == 1) //
    {
        GlobalVars::meta_projectName[metaTabUniversal] = "fiji";
        GlobalVars::meta_projectDataType[metaTabUniversal] = "1";
        GlobalVars::meta_projectUnlockType[metaTabUniversal]  = "2";
    }
    else if (index == 2)
    {
        GlobalVars::meta_projectName[metaTabUniversal] = "bj";
        GlobalVars::meta_projectDataType[metaTabUniversal]  = "1";
        GlobalVars::meta_projectUnlockType[metaTabUniversal]  = "2";
    }
    else
    {
        GlobalVars::meta_projectName[metaTabUniversal] = "malta";
        GlobalVars::meta_projectDataType[metaTabUniversal]  = "4";
        GlobalVars::meta_projectUnlockType[metaTabUniversal]  = "5";
    }
}


// ----------------------------- Device 8 ------------------------
void MainWindow::on_pb_CheckSerialPorts_D8_clicked()
{
    emit tx_updateSerialPorts();
}
void MainWindow::on_chk_Dual_IMEI_D8_stateChanged(int arg1)
{
    GlobalVars::meta_dual_imei_bool[metaTabUniversal] = ui->chk_Dual_IMEI_D8->isChecked();
    if(GlobalVars::meta_manual_imei_bool[metaTabUniversal]){
        ui->txt_IMEI_D8_2->setEnabled(GlobalVars::meta_dual_imei_bool[metaTabUniversal]);
        ui->txt_IMEI_D8_1->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
    }
}
void MainWindow::on_chk_Reboot_D8_stateChanged(int arg1)
{

}
void MainWindow::on_pb_StartRepairing_D8_clicked()
{
    ui->txt_outPut_D8->clear();
    if(checkAllOk4MetaMode(metaTabUniversal)){
        emit tx_StartRepairing_metaMode(metaTabUniversal, true, Tool_MTK);
    }
}
void MainWindow::on_pb_Stop_D8_clicked()
{
    rx_TextBoxOutput(Tool_MTK, metaTabUniversal, "Process Stopped", true, false, Qt::red);
    ui->pb_Stop_D8->setEnabled(false);
    ui->pb_StartRepairing_D8->setEnabled(true);
    ui->pb_CopytoClipBoard_D8->setEnabled(true);
    ui->pb_CheckSerialPorts_D8->setEnabled(true);
    ui->pb_unLock_D8->setEnabled(true);
    emit tx_StartRepairing_metaMode(metaTabUniversal, false, Tool_MTK);
    logger.writeToLog(ui->txt_outPut_D8->toPlainText().toUtf8());
}
void MainWindow::on_pb_CopytoClipBoard_D8_clicked()
{
    QClipboard *clip = QApplication::clipboard();
    clip->setText(ui->txt_outPut_D8->toPlainText(), QClipboard::Clipboard);
}
void MainWindow::on_cmb_PortNumber_D8_currentIndexChanged(int index)
{

}
void MainWindow::on_cmb_DeviceModel_D8_currentIndexChanged(int index)
{
    //qDebug()<<" META Device Model index :"<<index<<" txt:"<<ui->cmb_DeviceModel->currentText();
    if (index == 1) //
    {
        GlobalVars::meta_projectName[metaTabUniversal] = "fiji";
        GlobalVars::meta_projectDataType[metaTabUniversal] = "1";
        GlobalVars::meta_projectUnlockType[metaTabUniversal]  = "2";
    }
    else if (index == 2)
    {
        GlobalVars::meta_projectName[metaTabUniversal] = "bj";
        GlobalVars::meta_projectDataType[metaTabUniversal]  = "1";
        GlobalVars::meta_projectUnlockType[metaTabUniversal]  = "2";
    }
    else
    {
        GlobalVars::meta_projectName[metaTabUniversal] = "malta";
        GlobalVars::meta_projectDataType[metaTabUniversal]  = "4";
        GlobalVars::meta_projectUnlockType[metaTabUniversal]  = "5";
    }
}

// -------------------- UnLock Buttons ------------------------
void MainWindow::on_pb_unLock_D1_clicked()
{
    ui->txt_outPut_D1->clear();
    if(checkAllOk4MetaMode(metaTabUniversal)){
        qDebug()<<" Process : "<<metaTabUniversal;
        emit tx_StartRepairing_metaMode(metaTabUniversal, true, Tool_MTK_UnLock);
    }
}
void MainWindow::on_pb_unLock_D2_clicked()
{
    ui->txt_outPut_D2->clear();
    if(checkAllOk4MetaMode(metaTabUniversal)){
        qDebug()<<" Process : "<<metaTabUniversal;
        emit tx_StartRepairing_metaMode(metaTabUniversal, true, Tool_MTK_UnLock);
    }
}
void MainWindow::on_pb_unLock_D3_clicked()
{
    ui->txt_outPut_D3->clear();
    if(checkAllOk4MetaMode(metaTabUniversal)){
        qDebug()<<" Process : "<<metaTabUniversal;
        emit tx_StartRepairing_metaMode(metaTabUniversal, true, Tool_MTK_UnLock);
    }
}
void MainWindow::on_pb_unLock_D4_clicked()
{
    ui->txt_outPut_D4->clear();
    if(checkAllOk4MetaMode(metaTabUniversal)){
        qDebug()<<" Process : "<<metaTabUniversal;
        emit tx_StartRepairing_metaMode(metaTabUniversal, true, Tool_MTK_UnLock);
    }
}
void MainWindow::on_pb_unLock_D5_clicked()
{
    ui->txt_outPut_D5->clear();
    if(checkAllOk4MetaMode(metaTabUniversal)){
        emit tx_StartRepairing_metaMode(metaTabUniversal, true, Tool_MTK_UnLock);
    }
}
void MainWindow::on_pb_unLock_D6_clicked()
{
    ui->txt_outPut_D6->clear();
    if(checkAllOk4MetaMode(metaTabUniversal)){
        emit tx_StartRepairing_metaMode(metaTabUniversal, true, Tool_MTK_UnLock);
    }
}
void MainWindow::on_pb_unLock_D7_clicked()
{
    ui->txt_outPut_D7->clear();
    if(checkAllOk4MetaMode(metaTabUniversal)){
        emit tx_StartRepairing_metaMode(metaTabUniversal, true, Tool_MTK_UnLock);
    }
}
void MainWindow::on_pb_unLock_D8_clicked()
{
    ui->txt_outPut_D8->clear();
    if(checkAllOk4MetaMode(metaTabUniversal)){
        emit tx_StartRepairing_metaMode(metaTabUniversal, true, Tool_MTK_UnLock);
    }
}


// -------------------- UnLock Check Box ------------------------
void MainWindow::on_chk_unLock_D1_stateChanged(int arg1)
{
    GlobalVars::meta_unLock_bool[metaTabUniversal] = arg1;
    //qDebug()<<" Unlock button: "<<GlobalVars::meta_unLock_bool[metaTabUniversal]<<" param: "<<arg1;
}
void MainWindow::on_chk_unLock_D2_stateChanged(int arg1)
{
    GlobalVars::meta_unLock_bool[metaTabUniversal] = arg1;
}
void MainWindow::on_chk_unLock_D4_stateChanged(int arg1)
{
    GlobalVars::meta_unLock_bool[metaTabUniversal] = arg1;
}
void MainWindow::on_chk_unLock_D3_stateChanged(int arg1)
{
    GlobalVars::meta_unLock_bool[metaTabUniversal] = arg1;
}
void MainWindow::on_chk_unLock_D5_stateChanged(int arg1)
{
    GlobalVars::meta_unLock_bool[metaTabUniversal] = arg1;
}
void MainWindow::on_chk_unLock_D6_stateChanged(int arg1)
{
    GlobalVars::meta_unLock_bool[metaTabUniversal] = arg1;
}
void MainWindow::on_chk_unLock_D7_stateChanged(int arg1)
{
    GlobalVars::meta_unLock_bool[metaTabUniversal] = arg1;
}
void MainWindow::on_chk_unLock_D8_stateChanged(int arg1)
{
    GlobalVars::meta_unLock_bool[metaTabUniversal] = arg1;
}

// -------------------- Manual, Automatic IMEI Check Box ------------------------
void MainWindow::on_chk_manual_IMEI_D1_stateChanged(int arg1)
{
    GlobalVars::meta_manual_imei_bool[metaTabUniversal] = arg1;
    if(GlobalVars::meta_dual_imei_bool[metaTabUniversal]){
        ui->txt_IMEI_D1_2->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
        ui->txt_IMEI_D1_1->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
    }
    else {
        ui->txt_IMEI_D1_1->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
    }
}
void MainWindow::on_chk_manual_IMEI_D2_stateChanged(int arg1)
{
    GlobalVars::meta_manual_imei_bool[metaTabUniversal] = arg1;
    if(GlobalVars::meta_dual_imei_bool[metaTabUniversal]){
        ui->txt_IMEI_D2_2->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
        ui->txt_IMEI_D2_1->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
    }
    else {
        ui->txt_IMEI_D2_1->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
    }
}
void MainWindow::on_chk_manual_IMEI_D3_stateChanged(int arg1)
{
    GlobalVars::meta_manual_imei_bool[metaTabUniversal] = arg1;
    if(GlobalVars::meta_dual_imei_bool[metaTabUniversal]){
        ui->txt_IMEI_D3_2->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
        ui->txt_IMEI_D3_1->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
    }
    else {
        ui->txt_IMEI_D3_1->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
    }
}
void MainWindow::on_chk_manual_IMEI_D4_stateChanged(int arg1)
{
    GlobalVars::meta_manual_imei_bool[metaTabUniversal] = arg1;
    if(GlobalVars::meta_dual_imei_bool[metaTabUniversal]){
        ui->txt_IMEI_D4_2->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
        ui->txt_IMEI_D4_1->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
    }
    else {
        ui->txt_IMEI_D4_1->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
    }
}
void MainWindow::on_chk_manual_IMEI_D5_stateChanged(int arg1)
{
    GlobalVars::meta_manual_imei_bool[metaTabUniversal] = arg1;
    if(GlobalVars::meta_dual_imei_bool[metaTabUniversal]){
        ui->txt_IMEI_D5_2->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
        ui->txt_IMEI_D5_1->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
    }
    else {
        ui->txt_IMEI_D5_1->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
    }
}
void MainWindow::on_chk_manual_IMEI_D6_stateChanged(int arg1)
{
    GlobalVars::meta_manual_imei_bool[metaTabUniversal] = arg1;
    if(GlobalVars::meta_dual_imei_bool[metaTabUniversal]){
        ui->txt_IMEI_D6_2->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
        ui->txt_IMEI_D6_1->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
    }
    else {
        ui->txt_IMEI_D6_1->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
    }
}
void MainWindow::on_chk_manual_IMEI_D7_stateChanged(int arg1)
{
    GlobalVars::meta_manual_imei_bool[metaTabUniversal] = arg1;
    if(GlobalVars::meta_dual_imei_bool[metaTabUniversal]){
        ui->txt_IMEI_D7_2->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
        ui->txt_IMEI_D7_1->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
    }
    else {
        ui->txt_IMEI_D7_1->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
    }
}
void MainWindow::on_chk_Manual_IMEI_D8_stateChanged(int arg1)
{
    GlobalVars::meta_manual_imei_bool[metaTabUniversal] = arg1;
    if(GlobalVars::meta_dual_imei_bool[metaTabUniversal]){
        ui->txt_IMEI_D8_2->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
        ui->txt_IMEI_D8_1->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
    }
    else {
        ui->txt_IMEI_D8_1->setEnabled(GlobalVars::meta_manual_imei_bool[metaTabUniversal]);
    }
}

// -------------------- MDM Check Box ------------------------
void MainWindow::on_chk_mdm_D1_stateChanged(int arg1)
{
    GlobalVars::meta_MDM_bool[metaTabUniversal] = arg1;
}
void MainWindow::on_chk_mdm_D2_stateChanged(int arg1)
{
    GlobalVars::meta_MDM_bool[metaTabUniversal] = arg1;
}
void MainWindow::on_chk_mdm_D3_stateChanged(int arg1)
{
    GlobalVars::meta_MDM_bool[metaTabUniversal] = arg1;
}
void MainWindow::on_chk_mdm_D4_stateChanged(int arg1)
{
    GlobalVars::meta_MDM_bool[metaTabUniversal] = arg1;
}
void MainWindow::on_chk_mdm_D5_stateChanged(int arg1)
{
    GlobalVars::meta_MDM_bool[metaTabUniversal] = arg1;
}
void MainWindow::on_chk_mdm_D6_stateChanged(int arg1)
{
    GlobalVars::meta_MDM_bool[metaTabUniversal] = arg1;
}
void MainWindow::on_chk_mdm_D7_stateChanged(int arg1)
{
    GlobalVars::meta_MDM_bool[metaTabUniversal] = arg1;
}
void MainWindow::on_chk_mdm_D8_stateChanged(int arg1)
{
    GlobalVars::meta_MDM_bool[metaTabUniversal] = arg1;
}



// -------------------- META Mode Carrier Fix Button  ------------------------
void MainWindow::on_pb_carrierFix_D1_clicked()
{
    ui->txt_outPut_D1->clear();
    if(checkAllOk4MetaMode(metaTabUniversal)){
        emit tx_StartRepairing_metaMode(metaTabUniversal, true, Tool_META_CarrierFix);
    }
}


void MainWindow::on_pb_Minimize_clicked()
{
    this->setWindowState(Qt::WindowMinimized);
}





