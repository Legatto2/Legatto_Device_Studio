#include "diagnosticswindow.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QResizeEvent>
#include <QSaveFile>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStyle>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

namespace {
QLabel *label(const QString &text, const char *style, QWidget *parent = nullptr)
{
    auto *result = new QLabel(text, parent);
    result->setObjectName(QString::fromLatin1(style));
    result->setTextFormat(Qt::PlainText);
    result->setWordWrap(true);
    return result;
}

void configureTable(QTableWidget *table)
{
    table->verticalHeader()->hide();
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setAlternatingRowColors(true);
    table->setShowGrid(false);
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setDefaultSectionSize(38);
}

QString prettyValue(const QJsonValue &value)
{
    if (value.isString()) return value.toString();
    if (value.isBool()) return value.toBool() ? QStringLiteral("Yes") : QStringLiteral("No");
    if (value.isDouble()) return QString::number(value.toDouble());
    return QStringLiteral("Not reported");
}

QString propertyLabel(const QString &key)
{
    if (key == QStringLiteral("manufacturer")) return QStringLiteral("Manufacturer");
    if (key == QStringLiteral("model")) return QStringLiteral("Model");
    if (key == QStringLiteral("androidVersion")) return QStringLiteral("Android version");
    if (key == QStringLiteral("buildId")) return QStringLiteral("Build ID");
    if (key == QStringLiteral("securityPatch")) return QStringLiteral("Security patch");
    return key;
}
}

DiagnosticsWindow::DiagnosticsWindow(QWidget *parent)
    : QMainWindow(parent), service(new Diagnostics::Service(this))
{
    service->setObjectName(QStringLiteral("diagnosticService"));
    setWindowTitle(QStringLiteral("MotoTool | Device diagnostics"));
    resize(1120, 820);
    setMinimumSize(760, 600);

    setStyleSheet(QStringLiteral(
        "QMainWindow, QScrollArea, QWidget#workspace { background: #101923; color: #e7eef5; }"
        "QWidget { font-size: 13px; }"
        "QLabel { color: #e7eef5; background: transparent; }"
        "QLabel#brand { color: #70e4cc; font-size: 18px; font-weight: 700; }"
        "QLabel#title { font-size: 30px; font-weight: 700; }"
        "QLabel#section { font-size: 17px; font-weight: 600; }"
        "QLabel#muted { color: #aabccc; }"
        "QLabel#status { padding: 12px; border-radius: 6px; background: #1b3040; }"
        "QLabel#status[error=\"true\"] { color: #ffd4cb; background: #442c31; }"
        "QFrame#card { background: #17232f; border: 1px solid #2b3b4b; border-radius: 8px; }"
        "QLineEdit, QPlainTextEdit { color: #e7eef5; background: #101923; border: 1px solid #405266; border-radius: 4px; padding: 9px; selection-background-color: #285f69; }"
        "QLineEdit:focus, QPlainTextEdit:focus { border-color: #70e4cc; }"
        "QPushButton { color: #e7eef5; background: #283d4f; border: 1px solid #486074; border-radius: 5px; padding: 9px 14px; font-weight: 600; }"
        "QPushButton:hover { background: #365168; }"
        "QPushButton:focus { border: 2px solid #70e4cc; }"
        "QPushButton:disabled { color: #8a98a6; background: #202b36; border-color: #344352; }"
        "QPushButton#primary { color: #0d2726; background: #70e4cc; border-color: #70e4cc; }"
        "QPushButton#primary:hover { background: #9aeedc; }"
        "QPushButton#primary:disabled { color: #8a98a6; background: #263e42; border-color: #344352; }"
        "QTableWidget { color: #e7eef5; background: #111c27; alternate-background-color: #192735; border: 1px solid #344352; selection-background-color: #245a62; selection-color: #ffffff; }"
        "QHeaderView::section { color: #b8cad8; background: #1b2c3a; border: 0; padding: 9px; font-weight: 600; }"
        "QTabWidget::pane { border: 1px solid #344352; }"
        "QTabBar::tab { color: #aabccc; background: #17232f; padding: 10px 15px; }"
        "QTabBar::tab:selected { color: #70e4cc; background: #223544; }"
        "QProgressBar { border: none; background: #263b4c; max-height: 4px; }"
        "QProgressBar::chunk { background: #70e4cc; }"
        "QSplitter::handle { background: #2b3b4b; }"));

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    setCentralWidget(scroll);
    auto *workspace = new QWidget;
    workspace->setObjectName(QStringLiteral("workspace"));
    scroll->setWidget(workspace);
    auto *layout = new QVBoxLayout(workspace);
    layout->setContentsMargins(26, 22, 26, 22);
    layout->setSpacing(16);

    auto *brandRow = new QHBoxLayout;
    brandRow->addWidget(label(QStringLiteral("MOTOTOOL"), "brand"));
    brandRow->addStretch();
    auto *previewBadge = label(QStringLiteral("DIAGNOSTICS  /  DEVELOPMENT PREVIEW"), "muted");
    previewBadge->setWordWrap(false);
    brandRow->addWidget(previewBadge);
    layout->addLayout(brandRow);
    layout->addWidget(label(QStringLiteral("Know the device. See the result."), "title"));
    layout->addWidget(label(QStringLiteral("Inspect an authorized Android device and export a concise report. Every device query is read-only."), "muted"));

    auto *toolCard = new QFrame;
    toolCard->setObjectName(QStringLiteral("card"));
    auto *toolLayout = new QVBoxLayout(toolCard);
    toolLayout->setContentsMargins(18, 15, 18, 15);
    toolLayout->addWidget(label(QStringLiteral("1. Set up Android tools"), "section"));
    toolLayout->addWidget(label(QStringLiteral("Choose adb from a trusted Android Platform-Tools download. Checking the tool runs the file you select."), "muted"));
    auto *pathRow = new QHBoxLayout;
    toolPath = new QLineEdit;
    toolPath->setObjectName(QStringLiteral("adbPath"));
    toolPath->setAccessibleName(QStringLiteral("Absolute path to adb executable"));
#ifdef Q_OS_WIN
    toolPath->setPlaceholderText(QStringLiteral("C:\\Android\\platform-tools\\adb.exe"));
#else
    toolPath->setPlaceholderText(QStringLiteral("/home/you/Android/platform-tools/adb"));
#endif
    browseButton = new QPushButton(QStringLiteral("&Browse…"));
    browseButton->setObjectName(QStringLiteral("browseTool"));
    verifyButton = new QPushButton(QStringLiteral("&Check tool"));
    verifyButton->setObjectName(QStringLiteral("verifyTool"));
    pathRow->addWidget(toolPath, 1);
    pathRow->addWidget(browseButton);
    pathRow->addWidget(verifyButton);
    toolLayout->addLayout(pathRow);
    toolDetails = label(QStringLiteral("No tool selected. Nothing runs automatically."), "muted");
    toolDetails->setTextInteractionFlags(Qt::TextSelectableByMouse);
    toolLayout->addWidget(toolDetails);
    auto *downloadButton = new QPushButton(QStringLiteral("Get official Platform-Tools ↗"));
    downloadButton->setAccessibleName(QStringLiteral("Open Android Developers Platform-Tools downloads in your browser"));
    auto *downloadRow = new QHBoxLayout;
    downloadRow->addWidget(downloadButton);
    downloadRow->addStretch();
    toolLayout->addLayout(downloadRow);
    layout->addWidget(toolCard);

    auto *splitter = new QSplitter(Qt::Horizontal);
    panels = splitter;
    splitter->setChildrenCollapsible(false);
    auto *devicePanel = new QWidget;
    auto *deviceLayout = new QVBoxLayout(devicePanel);
    deviceLayout->setContentsMargins(0, 0, 10, 0);
    auto *deviceHeader = new QHBoxLayout;
    deviceHeader->addWidget(label(QStringLiteral("2. Choose a device"), "section"));
    deviceHeader->addStretch();
    refreshButton = new QPushButton(QStringLiteral("&Refresh"));
    refreshButton->setObjectName(QStringLiteral("refreshDevices"));
    deviceHeader->addWidget(refreshButton);
    deviceLayout->addLayout(deviceHeader);
    devices = new QTableWidget(0, 3);
    devices->setObjectName(QStringLiteral("devices"));
    devices->setAccessibleName(QStringLiteral("Connected devices; select an authorized device to inspect"));
    devices->setHorizontalHeaderLabels({QStringLiteral("Serial"), QStringLiteral("State"), QStringLiteral("Model")});
    configureTable(devices);
    devices->setMinimumHeight(190);
    devices->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    deviceLayout->addWidget(devices, 1);
    deviceHint = label(QStringLiteral("Check the Android tool, then refresh. A device is never selected automatically."), "muted");
    deviceLayout->addWidget(deviceHint);
    splitter->addWidget(devicePanel);

    auto *reportPanel = new QWidget;
    auto *reportLayout = new QVBoxLayout(reportPanel);
    reportLayout->setContentsMargins(10, 0, 0, 0);
    reportLayout->addWidget(label(QStringLiteral("3. Inspect & export"), "section"));
    auto *reportActions = new QHBoxLayout;
    inspectButton = new QPushButton(QStringLiteral("&Inspect selected device"));
    inspectButton->setObjectName(QStringLiteral("primary"));
    inspectButton->setAccessibleName(QStringLiteral("Inspect selected device"));
    inspectButton->setProperty("role", QStringLiteral("inspectDevice"));
    exportButton = new QPushButton(QStringLiteral("&Export report…"));
    exportButton->setObjectName(QStringLiteral("exportReport"));
    reportActions->addWidget(inspectButton);
    reportActions->addWidget(exportButton);
    reportLayout->addLayout(reportActions);
    auto *tabs = new QTabWidget;
    properties = new QTableWidget(0, 2);
    properties->setObjectName(QStringLiteral("properties"));
    properties->setAccessibleName(QStringLiteral("Collected device properties"));
    properties->setHorizontalHeaderLabels({QStringLiteral("Property"), QStringLiteral("Value")});
    configureTable(properties);
    properties->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    reportPreview = new QPlainTextEdit;
    reportPreview->setObjectName(QStringLiteral("reportPreview"));
    reportPreview->setAccessibleName(QStringLiteral("Exact JSON report that will be exported"));
    reportPreview->setReadOnly(true);
    reportPreview->setPlaceholderText(QStringLiteral("Run an inspection to preview the report before saving it."));
    tabs->addTab(properties, QStringLiteral("Device details"));
    tabs->addTab(reportPreview, QStringLiteral("Export preview"));
    reportLayout->addWidget(tabs, 1);
    reportHint = label(QStringLiteral("Reports exclude raw serial, local path, and command-output fields. Review device-provided values before sharing."), "muted");
    reportLayout->addWidget(reportHint);
    splitter->addWidget(reportPanel);
    splitter->setStretchFactor(0, 4);
    splitter->setStretchFactor(1, 5);
    layout->addWidget(splitter, 1);

    auto *statusRow = new QHBoxLayout;
    status = label(QStringLiteral("Choose Android tools to begin."), "status");
    status->setAccessibleName(QStringLiteral("Operation status"));
    statusRow->addWidget(status, 1);
    cancelButton = new QPushButton(QStringLiteral("&Cancel"));
    cancelButton->setObjectName(QStringLiteral("cancelOperation"));
    statusRow->addWidget(cancelButton);
    layout->addLayout(statusRow);
    progress = new QProgressBar;
    progress->setRange(0, 0);
    progress->setTextVisible(false);
    progress->setAccessibleName(QStringLiteral("Operation in progress"));
    layout->addWidget(progress);
    layout->addWidget(label(QStringLiteral("Uses Android's authorized debugging connection. No repair, flash, reset, unlock, or account-service commands are exposed here."), "muted"));

    connect(browseButton, &QPushButton::clicked, this, &DiagnosticsWindow::browseForTool);
    connect(verifyButton, &QPushButton::clicked, this, &DiagnosticsWindow::verifyTool);
    connect(toolPath, &QLineEdit::returnPressed, this, &DiagnosticsWindow::verifyTool);
    connect(toolPath, &QLineEdit::textChanged, this, [this] {
        verified = false;
        devices->setRowCount(0);
        clearReport();
        toolDetails->setText(QStringLiteral("Check this tool before connecting to a device."));
        updateControls();
    });
    connect(downloadButton, &QPushButton::clicked, this, [] {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://developer.android.com/tools/releases/platform-tools")));
    });
    connect(refreshButton, &QPushButton::clicked, this, &DiagnosticsWindow::scanDevices);
    connect(inspectButton, &QPushButton::clicked, this, &DiagnosticsWindow::inspectSelectedDevice);
    connect(exportButton, &QPushButton::clicked, this, &DiagnosticsWindow::exportReport);
    connect(cancelButton, &QPushButton::clicked, service, &Diagnostics::Service::cancel);
    connect(devices, &QTableWidget::itemSelectionChanged, this, [this] {
        clearReport();
        if (!selectedSerial().isEmpty()) {
            const QString state = devices->item(devices->currentRow(), 1)->data(Qt::UserRole).toString();
            if (state == QStringLiteral("unauthorized"))
                deviceHint->setText(QStringLiteral("Unlock the phone and approve USB debugging on its screen, then refresh."));
            else if (state == QStringLiteral("offline"))
                deviceHint->setText(QStringLiteral("The device is offline. Reconnect its USB cable, then refresh."));
            else if (selectedDeviceAuthorized())
                deviceHint->setText(QStringLiteral("Selection confirmed. Inspect queries only this device; serials stay in this window."));
            else
                deviceHint->setText(QStringLiteral("This device state cannot be inspected. Check USB permissions and the device connection, then refresh."));
        }
        updateControls();
    });
    connect(service, &Diagnostics::Service::progress, this, [this](const QString &message) { showStatus(message); });
    connect(service, &Diagnostics::Service::busyChanged, this, [this](bool) { updateControls(); });
    connect(service, &Diagnostics::Service::completed, this, &DiagnosticsWindow::receiveResult);
    updateControls();
}

void DiagnosticsWindow::browseForTool()
{
#ifdef Q_OS_WIN
    const QString filter = QStringLiteral("Android Debug Bridge (adb.exe)");
#else
    const QString filter = QStringLiteral("Android Debug Bridge (adb)");
#endif
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Choose the official adb executable"), QString(), filter);
    if (!path.isEmpty()) toolPath->setText(path);
}

void DiagnosticsWindow::verifyTool()
{
    if (service->isBusy() || toolPath->text().trimmed().isEmpty()) return;
    verified = false;
    devices->setRowCount(0);
    clearReport();
    QString error;
    if (!service->setAdbPath(toolPath->text().trimmed(), &error)) {
        showStatus(error, true);
        updateControls();
        return;
    }
    showStatus(QStringLiteral("Checking the selected Android tool…"));
    if (!service->verifyTool()) {
        showStatus(QStringLiteral("The tool check could not start. Choose an accessible adb executable and try again."), true);
        updateControls();
    }
}

void DiagnosticsWindow::scanDevices()
{
    if (!verified || service->isBusy()) return;
    clearReport();
    devices->setRowCount(0);
    showStatus(QStringLiteral("Looking for connected devices…"));
    if (!service->scanDevices()) showStatus(QStringLiteral("Cannot scan right now. Check the selected tool again."), true);
    updateControls();
}

QString DiagnosticsWindow::selectedSerial() const
{
    const int row = devices->currentRow();
    if (row < 0 || devices->selectedItems().isEmpty() || !devices->item(row, 0)) return QString();
    return devices->item(row, 0)->data(Qt::UserRole).toString();
}

bool DiagnosticsWindow::selectedDeviceAuthorized() const
{
    const int row = devices->currentRow();
    return !selectedSerial().isEmpty() && devices->item(row, 1)
        && devices->item(row, 1)->data(Qt::UserRole).toString() == QStringLiteral("device");
}

void DiagnosticsWindow::inspectSelectedDevice()
{
    if (!verified || service->isBusy() || !selectedDeviceAuthorized()) return;
    const QString serial = selectedSerial();
    clearReport();
    showStatus(QStringLiteral("Checking the selected device and reading its properties…"));
    if (!service->inspectDevice(serial)) showStatus(QStringLiteral("Inspection could not start. Refresh the devices and select one again."), true);
    updateControls();
}

void DiagnosticsWindow::receiveResult(const Diagnostics::Result &result)
{
    if (closing) {
        QTimer::singleShot(0, this, &QWidget::close);
        return;
    }
    if (result.status != Diagnostics::Status::Success) {
        clearReport();
        if (result.operation == QStringLiteral("verify") || result.status == Diagnostics::Status::ToolChanged) {
            verified = false;
            devices->setRowCount(0);
            toolDetails->setText(QStringLiteral("The tool check is no longer valid. Select and check the executable again."));
        } else if (result.operation == QStringLiteral("inspect") && result.status == Diagnostics::Status::InvalidRequest) {
            devices->setRowCount(0);
            deviceHint->setText(QStringLiteral("The selected connection changed. Refresh the device list and select it again."));
        }
        showStatus(result.message, result.status != Diagnostics::Status::Cancelled);
        updateControls();
        return;
    }
    if (result.operation == QStringLiteral("verify")) {
        verified = true;
        toolDetails->setText(QStringLiteral("%1\nSHA-256: %2\nThis fingerprint identifies the file; it does not authenticate its publisher.")
            .arg(result.tool.version, result.tool.sha256));
        deviceHint->setText(QStringLiteral("Connect a phone with USB debugging authorized, then refresh and select it."));
    } else if (result.operation == QStringLiteral("scan")) {
        const QSignalBlocker blocker(devices);
        devices->setRowCount(result.devices.size());
        for (int row = 0; row < result.devices.size(); ++row) {
            const auto &device = result.devices.at(row);
            auto *serial = new QTableWidgetItem(device.serial);
            serial->setData(Qt::UserRole, device.serial);
            devices->setItem(row, 0, serial);
            auto *state = new QTableWidgetItem(device.isAuthorized() ? QStringLiteral("Authorized") : device.state);
            state->setData(Qt::UserRole, device.state);
            devices->setItem(row, 1, state);
            devices->setItem(row, 2, new QTableWidgetItem(device.model.isEmpty() ? QStringLiteral("—") : device.model));
        }
        devices->clearSelection();
        devices->setCurrentCell(-1, -1);
        deviceHint->setText(result.devices.isEmpty()
            ? QStringLiteral("No devices found. Check the cable, USB debugging, and Android USB driver, then refresh.")
            : QStringLiteral("Select the exact phone you want to inspect. Unauthorized and offline devices need attention first."));
    } else if (result.operation == QStringLiteral("inspect")) {
        report = result.report;
        reportPreview->setPlainText(QString::fromUtf8(QJsonDocument(report).toJson(QJsonDocument::Indented)));
        const QJsonObject values = report.value(QStringLiteral("device")).toObject();
        properties->setRowCount(values.size());
        int row = 0;
        for (auto it = values.constBegin(); it != values.constEnd(); ++it, ++row) {
            properties->setItem(row, 0, new QTableWidgetItem(propertyLabel(it.key())));
            properties->setItem(row, 1, new QTableWidgetItem(prettyValue(it.value())));
        }
        reportHint->setText(QStringLiteral("Inspection complete. Export preview shows exactly what will be saved. Review device-provided values before sharing."));
    }
    showStatus(result.message);
    updateControls();
}

void DiagnosticsWindow::clearReport()
{
    report = QJsonObject();
    properties->setRowCount(0);
    reportPreview->clear();
    reportHint->setText(QStringLiteral("Reports exclude raw serial, local path, and command-output fields. Review device-provided values before sharing."));
    exportButton->setEnabled(false);
}

void DiagnosticsWindow::updateControls()
{
    const bool busy = service->isBusy();
    toolPath->setEnabled(!busy && !closing);
    browseButton->setEnabled(!busy && !closing);
    verifyButton->setEnabled(!busy && !closing && !toolPath->text().trimmed().isEmpty());
    refreshButton->setEnabled(verified && !busy && !closing);
    devices->setEnabled(!busy && !closing);
    inspectButton->setEnabled(verified && !busy && !closing && selectedDeviceAuthorized());
    exportButton->setEnabled(!busy && !closing && !report.isEmpty());
    cancelButton->setEnabled(busy && !closing);
    progress->setVisible(busy);
}

void DiagnosticsWindow::showStatus(const QString &message, bool error)
{
    status->setText(message);
    status->setProperty("error", error);
    status->style()->unpolish(status);
    status->style()->polish(status);
}

bool DiagnosticsWindow::saveReport(const QString &path, QString *error) const
{
    if (error) error->clear();
    if (service->isBusy() || report.isEmpty()) {
        if (error) *error = QStringLiteral("Run a successful inspection before exporting a report.");
        return false;
    }
    if (path.isEmpty() || QFileInfo(path).isSymLink()) {
        if (error) *error = QStringLiteral("Choose a regular destination file for the report.");
        return false;
    }
    QSaveFile output(path);
    output.setDirectWriteFallback(false);
    const QByteArray bytes = QJsonDocument(report).toJson(QJsonDocument::Indented);
    if (!output.open(QIODevice::WriteOnly) || output.write(bytes) != bytes.size() || !output.commit()) {
        if (error) *error = QStringLiteral("The report could not be saved. Choose a writable folder and try again.");
        return false;
    }
    return true;
}

void DiagnosticsWindow::exportReport()
{
    if (service->isBusy() || report.isEmpty()) return;
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Save the reviewed diagnostics report"),
        QStringLiteral("mototool-diagnostics.json"), QStringLiteral("JSON report (*.json)"));
    if (path.isEmpty()) return;
    QString error;
    if (saveReport(path, &error)) showStatus(QStringLiteral("Report saved. Only the fields shown in Export preview were included."));
    else showStatus(error, true);
}

void DiagnosticsWindow::closeEvent(QCloseEvent *event)
{
    if (service->isBusy()) {
        closing = true;
        showStatus(QStringLiteral("Cancelling the current diagnostic operation before closing…"));
        updateControls();
        service->cancel();
        event->ignore();
        return;
    }
    event->accept();
}

void DiagnosticsWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (panels) panels->setOrientation(event->size().width() < 960 ? Qt::Vertical : Qt::Horizontal);
}
