#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "globalvars.h"
#include "backendclass.h"
#include "class_logfile.h"
#include "metamodethread.h"




QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void tx_StartRepairing(int idx,  bool startStop, TOOL_TYPE tool);
    void tx_StartRepairing_metaMode(int idx,  bool startStop, TOOL_TYPE tool);
    void tx_updateSerialPorts();
    void tx_updateADBdevices();

public slots:
    void on_timer_singleShot_Elapsed();
    void rx_ProcessCompleted(int idx, bool complt);
    void rx_TextBoxOutput(TOOL_TYPE tool, int idx, QString s, bool isBold, bool newline = false, QColor color = QColor::fromRgb(255, 255, 255));
    void rx_newSerialPortDetected(QList<QSerialPortInfo> sInfo);
    void rx_miscOperations(TOOL_TYPE tool,int idx, int value, QString str);

    void rx_ADB_ProcessData(signalStructure sig);

    void rx_miscOperations_metaMode(TOOL_TYPE tool,int idx, int value, QString str);
    void rx_TextBoxOutput_metaMode(TOOL_TYPE tool,int idx, QString s, bool isBold, bool newline = false, QColor color = QColor::fromRgb(255, 255, 255));

private slots:
    void on_pb_Login_clicked();
    void on_pb_Close_clicked();

    void on_pb_CheckADB_Devices_clicked();
    void on_pb_StartRepair_SPD_clicked();
    void on_pb_unLock_SPD_clicked();
    void on_pb_Stop_SPD_clicked();
    void on_pb_CopytoClipBoard_SPD_clicked();
    void on_cmb_DeviceModel_SPD_currentIndexChanged(int index);
    void on_chk_AssignPort_stateChanged(int arg1);
    void on_chk_Dual_IMEI_SPD_stateChanged(int arg1);


    void on_pb_CheckSerialPorts_D1_clicked();
    void on_pb_CopytoClipBoard_D1_clicked();
    void on_chk_Dual_IMEI_D1_stateChanged(int arg1);
    void on_pb_Stop_D1_clicked();
    void on_pb_StartRepairing_D1_clicked();
    void on_chk_Reboot_D1_stateChanged(int arg1);
    void on_cmb_PortNumber_D1_currentIndexChanged(int index);
    void on_cmb_DeviceModel_D1_currentIndexChanged(int index);

    void on_pb_CheckSerialPorts_D2_clicked();
    void on_chk_Dual_IMEI_D2_stateChanged(int arg1);
    void on_pb_StartRepairing_D2_clicked();
    void on_pb_Stop_D2_clicked();
    void on_pb_CopytoClipBoard_D2_clicked();
    void on_chk_Reboot_D2_stateChanged(int arg1);
    void on_cmb_PortNumber_D2_currentIndexChanged(int index);
    void on_cmb_DeviceModel_D2_currentIndexChanged(int index);

    void on_pb_CheckSerialPorts_D3_clicked();
    void on_chk_Dual_IMEI_D3_stateChanged(int arg1);
    void on_chk_Reboot_D3_stateChanged(int arg1);
    void on_pb_StartRepairing_D3_clicked();
    void on_pb_Stop_D3_clicked();
    void on_pb_CopytoClipBoard_D3_clicked();
    void on_cmb_PortNumber_D3_currentIndexChanged(int index);
    void on_cmb_DeviceModel_D3_currentIndexChanged(int index);

    void on_pb_CheckSerialPorts_D4_clicked();
    void on_chk_Dual_IMEI_D4_stateChanged(int arg1);
    void on_chk_Reboot_D4_stateChanged(int arg1);
    void on_pb_StartRepairing_D4_clicked();
    void on_pb_Stop_D4_clicked();
    void on_pb_CopytoClipBoard_D4_clicked();
    void on_cmb_PortNumber_D4_currentIndexChanged(int index);
    void on_cmb_DeviceModel_D4_currentIndexChanged(int index);

    void on_pb_CheckSerialPorts_D5_clicked();
    void on_chk_Dual_IMEI_D5_stateChanged(int arg1);
    void on_chk_Reboot_D5_stateChanged(int arg1);
    void on_pb_StartRepairing_D5_clicked();
    void on_pb_Stop_D5_clicked();
    void on_pb_CopytoClipBoard_D5_clicked();
    void on_cmb_PortNumber_D5_currentIndexChanged(int index);
    void on_cmb_DeviceModel_D5_currentIndexChanged(int index);

    void on_pb_CheckSerialPorts_D6_clicked();
    void on_chk_Dual_IMEI_D6_stateChanged(int arg1);
    void on_chk_Reboot_D6_stateChanged(int arg1);
    void on_pb_StartRepairing_D6_clicked();
    void on_pb_Stop_D6_clicked();
    void on_pb_CopytoClipBoard_D6_clicked();
    void on_cmb_PortNumber_D6_currentIndexChanged(int index);
    void on_cmb_DeviceModel_D6_currentIndexChanged(int index);

    void on_pb_CheckSerialPorts_D7_clicked();
    void on_chk_Dual_IMEI_D7_stateChanged(int arg1);
    void on_chk_Reboot_D7_stateChanged(int arg1);
    void on_pb_StartRepairing_D7_clicked();
    void on_pb_Stop_D7_clicked();
    void on_pb_CopytoClipBoard_D7_clicked();
    void on_cmb_PortNumber_D7_currentIndexChanged(int index);
    void on_cmb_DeviceModel_D7_currentIndexChanged(int index);

    void on_pb_CheckSerialPorts_D8_clicked();
    void on_chk_Dual_IMEI_D8_stateChanged(int arg1);
    void on_chk_Reboot_D8_stateChanged(int arg1);
    void on_pb_StartRepairing_D8_clicked();
    void on_pb_Stop_D8_clicked();
    void on_pb_CopytoClipBoard_D8_clicked();    
    void on_cmb_PortNumber_D8_currentIndexChanged(int index);
    void on_cmb_DeviceModel_D8_currentIndexChanged(int index);


    void on_tabWidgetMeta_currentChanged(int index);

    void on_pb_unLock_D1_clicked();
    void on_pb_unLock_D2_clicked();
    void on_pb_unLock_D3_clicked();
    void on_pb_unLock_D4_clicked();
    void on_pb_unLock_D5_clicked();
    void on_pb_unLock_D6_clicked();
    void on_pb_unLock_D7_clicked();
    void on_pb_unLock_D8_clicked();



    void on_chk_unLock_D1_stateChanged(int arg1);
    void on_chk_unLock_D2_stateChanged(int arg1);
    void on_chk_unLock_D4_stateChanged(int arg1);
    void on_chk_unLock_D3_stateChanged(int arg1);
    void on_chk_unLock_D5_stateChanged(int arg1);
    void on_chk_unLock_D6_stateChanged(int arg1);
    void on_chk_unLock_D7_stateChanged(int arg1);
    void on_chk_unLock_D8_stateChanged(int arg1);

    void on_chk_Manual_IMEI_D8_stateChanged(int arg1);
    void on_chk_manual_IMEI_D1_stateChanged(int arg1);
    void on_chk_manual_IMEI_D2_stateChanged(int arg1);
    void on_chk_manual_IMEI_D3_stateChanged(int arg1);
    void on_chk_manual_IMEI_D4_stateChanged(int arg1);
    void on_chk_manual_IMEI_D5_stateChanged(int arg1);
    void on_chk_manual_IMEI_D6_stateChanged(int arg1);
    void on_chk_manual_IMEI_D7_stateChanged(int arg1);
    void on_chk_Manual_IMEI_SPD_stateChanged(int arg1);

    void on_pb_FRP_SPD_clicked();
    void on_pb_carrierFix_D1_clicked();
    void on_chk_mdm_D1_stateChanged(int arg1);
    void on_chk_mdm_D2_stateChanged(int arg1);
    void on_chk_mdm_D3_stateChanged(int arg1);
    void on_chk_mdm_D4_stateChanged(int arg1);
    void on_chk_mdm_D5_stateChanged(int arg1);
    void on_chk_mdm_D6_stateChanged(int arg1);
    void on_chk_mdm_D7_stateChanged(int arg1);
    void on_chk_mdm_D8_stateChanged(int arg1);

    void on_pb_Minimize_clicked();
    void on_pb_CopyUniqueID_clicked();
    void on_pb_Close_2_clicked();

private:
    Ui::MainWindow *ui;
    void initializeObjectsAndStartThread();
    bool checkAllOk4spdTool();
    bool checkAllOk4MetaMode(int device);
    BackEndClass *backEndObj;
    QThread *thrd;
    Class_LogFile logger;

    MetaModeThread *metaModeClass[8];
    QThread *metaThread[8];
    QTimer *timer_singleShot;
    int singleShotIndex = 0;

    QLabel *heading;
    QList<QSerialPortInfo> localSerialList;

    bool eventFilter(QObject *obj, QEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    int m_nMouseClick_X_Coordinate;
    int m_nMouseClick_Y_Coordinate;

    bool buttonIsPressed = false;
    int metaTabUniversal = 0;

    void seeSettingsFile();
    void saveNewSettings();
    bool settingsReady = false;

    bool needUpdatedSoftware = false;


};
#endif // MAINWINDOW_H
