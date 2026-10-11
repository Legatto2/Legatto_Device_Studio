"""One-shot, hash-checked application of the reviewed containment edits.
The preparation workflow removes this script after tests pass. It never touches main.
"""
from pathlib import Path
import hashlib
import json
import re

ROOT = Path(__file__).resolve().parents[1]
manifest = json.loads((ROOT / 'maintenance/phase1-manifest.json').read_text())
for name, hashes in manifest.items():
    expected = hashes['before'] or hashes['after']
    actual = hashlib.sha256((ROOT / name).read_bytes()).hexdigest()
    if actual != expected:
        raise SystemExit('Input changed; refusing to overwrite: ' + name)

TOKEN = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]', re.MULTILINE)
def replace_function(text, signature, replacement):
    start = text.index(signature)
    begin = text.index('{', start)
    depth = 0
    for token in TOKEN.finditer(text, begin):
        if token.group() == '{':
            depth += 1
        elif token.group() == '}':
            depth -= 1
            if depth == 0:
                return text[:start] + replacement.rstrip() + text[token.end():]
    raise ValueError(signature)

def once(text, old, new):
    if text.count(old) != 1:
        raise ValueError('Expected one replacement anchor: ' + old[:80])
    return text.replace(old, new, 1)

p = ROOT / 'MotoTool_MTK.pro'
s = once(p.read_text(), 'CONFIG += c++11', '''CONFIG += c++11

# Temporary containment: legacy debug streams include authentication material.
# Apply to debug AND release builds until structured redacted logging is in place.
DEFINES += QT_NO_DEBUG_OUTPUT
SOURCES += securityhelpers.cpp portselection.cpp
HEADERS += securityhelpers.h portselection.h''')
p.write_text(s)

p = ROOT / 'mainwindow.h'
s = p.read_text().replace('    void processSettingsFileLine(QString linee);\n', '')
s = once(s, '    bool rememberMyCredentials = false;', '    bool settingsReady = false;')
p.write_text(s)
p = ROOT / 'mainwindow.cpp'
s = once(p.read_text(), '#include <QApplication>', '#include <QApplication>\n#include <QSignalBlocker>\n#include "securityhelpers.h"\n#include "portselection.h"')
s = once(s, '    ui->setupUi(this);', '''    ui->setupUi(this);
    ui->chck_LoginRememberMe->setText("Remember username only");
    ui->txt_Password->clear();
    ui->pb_FRP_SPD->setEnabled(false);
    ui->pb_FRP_SPD->setToolTip("Disabled pending a secure, validated replacement.");
    connect(ui->chck_LoginRememberMe, &QCheckBox::toggled, this, [this](bool) {
        if (settingsReady) saveNewSettings();
    });''')
s = once(s, '    //ui->processFrame->hide();\n    ui->loginFrame->hide();', '    ui->processFrame->hide();\n    ui->loginFrame->show();')
s = once(s, 'void MainWindow::on_pb_Login_clicked()\n{', '''void MainWindow::on_pb_Login_clicked()
{
    if (!settingsReady) {
        ui->lbl_LoginResults->setText("Settings are not ready. Resolve the settings error before login.");
        return;
    }''')
start = s.index('    QByteArray postdata;', s.index('void MainWindow::on_pb_Login_clicked'))
end = s.index('    ui->lbl_LoginResults->setText("Wait for Server Response");', start)
s = s[:start] + '''    const QByteArray postdata = Security::encodeLoginForm(
        ui->lbl_SystemID->text(), ui->txt_UsernameEmail->text(), ui->txt_Password->text());

''' + s[end:]
s = replace_function(s, 'void MainWindow::seeSettingsFile()', r'''void MainWindow::seeSettingsFile()
{
    settingsReady = false;
    ui->txt_Password->clear();
    Security::LoginPreferences preferences;
    QString error;
    if (!Security::loadLoginPreferences(GlobalVars::motoTool_SettingsFilePath, &preferences, &error)) {
        ui->lbl_LoginResults->setText(error + " Login disabled; resolve the settings file and restart.");
        ui->pb_Login->setEnabled(false);
        return;
    }
    const QSignalBlocker blocker(ui->chck_LoginRememberMe);
    ui->chck_LoginRememberMe->setChecked(preferences.rememberUsername);
    ui->txt_UsernameEmail->setText(preferences.username);
    settingsReady = true;
    ui->pb_Login->setEnabled(true);
}''')
s = replace_function(s, 'void MainWindow::processSettingsFileLine(QString linee)', '')
s = replace_function(s, 'void MainWindow::saveNewSettings()', r'''void MainWindow::saveNewSettings()
{
    Security::LoginPreferences preferences;
    preferences.rememberUsername = ui->chck_LoginRememberMe->isChecked();
    if (preferences.rememberUsername) preferences.username = ui->txt_UsernameEmail->text();
    QString error;
    if (!Security::saveLoginPreferences(GlobalVars::motoTool_SettingsFilePath, preferences, &error)) {
        settingsReady = false;
        ui->pb_Login->setEnabled(false);
        ui->lbl_LoginResults->setText(error + " Preferences were not saved.");
    }
}''')
s = replace_function(s, 'void MainWindow::on_pb_FRP_SPD_clicked()', r'''void MainWindow::on_pb_FRP_SPD_clicked()
{
    ui->pb_FRP_SPD->setEnabled(false);
    rx_TextBoxOutput(Tool_SPD, 1,
        "FRP is disabled pending a secure, validated replacement.", true, false, Qt::red);
}''')
s = s.replace('ui->pb_FRP_SPD->setEnabled(true);', 'ui->pb_FRP_SPD->setEnabled(false);')
s = replace_function(s, 'void MainWindow::rx_newSerialPortDetected(QList<QSerialPortInfo> sInfo)', r'''void MainWindow::rx_newSerialPortDetected(QList<QSerialPortInfo> sInfo)
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
}''')
s = once(s, 'bool MainWindow::checkAllOk4MetaMode(int device)\n{', '''bool MainWindow::checkAllOk4MetaMode(int device)
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
    }''')
p.write_text(s)

p = ROOT / 'backendclass.cpp'
s = once(p.read_text(), '#include "backendclass.h"', '#include "backendclass.h"\n#include "securityhelpers.h"')
s = once(s, 'void BackEndClass::rx_StartRepairing(int idx, bool startStop, TOOL_TYPE tool)\n{', '''void BackEndClass::rx_StartRepairing(int idx, bool startStop, TOOL_TYPE tool)
{
    // Backend guard: disabling a button is not a security boundary.
    if (startStop && tool == Tool_SPD_FRP_FastBoot) {
        signal.currentTool = tool;
        getServerFastBoot_File();
        return;
    }''')
s = once(s, 'void BackEndClass::rx_timer_SPDTool_elapsed()\n{', '''void BackEndClass::rx_timer_SPDTool_elapsed()
{
    if (signal.currentTool == Tool_SPD_FRP_FastBoot) {
        getServerFastBoot_File();
        return;
    }''')
start = s.index('    case ADB_FRP_GetServerFile:{', s.index('void BackEndClass::rx_timer_SPDTool_elapsed'))
end = s.index('\n    }\n\n}', start)
s = s[:start] + '''    case ADB_FRP_GetServerFile:
    case ADB_FRP_PushFile:
    case ADB_FRP_FastBootErase:
    case ADB_FRP_Fast_Reboot:
        getServerFastBoot_File();
        break;''' + s[end:]
s = replace_function(s, 'bool BackEndClass::getServerFastBoot_File()', r'''bool BackEndClass::getServerFastBoot_File()
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
}''')
s = replace_function(s, 'ADB_Commands BackEndClass::sendOperationDetailsToServerToReduceCredit()', r'''ADB_Commands BackEndClass::sendOperationDetailsToServerToReduceCredit()
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
}''')
p.write_text(s)

p = ROOT / 'class_logfile.cpp'
s = p.read_text()
start = s.index('    QByteArray postdata;', s.index('void Class_LogFile::writeToLog'))
s = s[:start] + '''    // Remote log upload is disabled until consent, redaction, retention,
    // and per-operation IDs are implemented. Existing local logs are retained.
}
'''
p.write_text(s)

p = ROOT / 'mainwindow.ui'
s = p.read_text()
for name in ['txt_Password', 'txt_UsernameEmail']:
    pattern = re.compile(r'(<widget class="QLineEdit" name="' + name + r'">)([\s\S]*?)(</widget>)')
    found = pattern.search(s)
    if not found:
        raise ValueError('Missing credential widget: ' + name)
    inner, count = re.subn(r'(<property name="text">\s*)<string>[^<]*</string>', r'\1<string/>', found.group(2))
    if count != 1:
        raise ValueError('Unexpected credential widget shape: ' + name)
    s = s[:found.start()] + found.group(1) + inner + found.group(3) + s[found.end():]
p.write_text(s)

for name, hashes in manifest.items():
    if hashlib.sha256((ROOT / name).read_bytes()).hexdigest() != hashes['after']:
        raise SystemExit('Output differs from locally verified patch: ' + name)
print('Verified all 16 expected outputs; no application or servicing binary was executed.')
