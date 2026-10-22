#ifndef DIAGNOSTICSWINDOW_H
#define DIAGNOSTICSWINDOW_H

#include "diagnosticscore.h"
#include <QJsonObject>
#include <QMainWindow>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QTableWidget;
class QCloseEvent;
class QResizeEvent;
class QSplitter;

// Presentation only: every device command belongs to Diagnostics::Service.
class DiagnosticsWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit DiagnosticsWindow(QWidget *parent = nullptr);
    bool saveReport(const QString &path, QString *error = nullptr) const;

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void browseForTool();
    void verifyTool();
    void scanDevices();
    void inspectSelectedDevice();
    void exportReport();
    void receiveResult(const Diagnostics::Result &result);
    void updateControls();
    void clearReport();
    void showStatus(const QString &message, bool error = false);
    QString selectedSerial() const;
    bool selectedDeviceAuthorized() const;

    Diagnostics::Service *service;
    QLineEdit *toolPath;
    QLabel *toolDetails;
    QLabel *status;
    QLabel *deviceHint;
    QLabel *reportHint;
    QPushButton *browseButton;
    QPushButton *verifyButton;
    QPushButton *refreshButton;
    QPushButton *inspectButton;
    QPushButton *exportButton;
    QPushButton *cancelButton;
    QProgressBar *progress;
    QTableWidget *devices;
    QTableWidget *properties;
    QPlainTextEdit *reportPreview;
    QSplitter *panels = nullptr;
    QJsonObject report;
    bool verified = false;
    bool closing = false;
};

#endif
