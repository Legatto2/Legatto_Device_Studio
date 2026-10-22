#include "diagnosticswindow.h"

#include <QFile>
#include <QJsonDocument>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QtTest>

class DiagnosticsUiTests : public QObject
{
    Q_OBJECT

private:
    QString fakePath;

    static QPushButton *button(DiagnosticsWindow &window, const char *name)
    {
        return window.findChild<QPushButton *>(QString::fromLatin1(name));
    }

    bool checkTool(DiagnosticsWindow &window)
    {
        auto *service = window.findChild<Diagnostics::Service *>();
        QSignalSpy finished(service, &Diagnostics::Service::completed);
        window.findChild<QLineEdit *>(QStringLiteral("adbPath"))->setText(fakePath);
        button(window, "verifyTool")->click();
        if (finished.isEmpty() && !finished.wait(5000)) return false;
        return button(window, "refreshDevices")->isEnabled();
    }

    bool scan(DiagnosticsWindow &window)
    {
        auto *service = window.findChild<Diagnostics::Service *>();
        QSignalSpy finished(service, &Diagnostics::Service::completed);
        button(window, "refreshDevices")->click();
        return !finished.isEmpty() || finished.wait(5000);
    }

private slots:
    void initTestCase()
    {
        fakePath = qEnvironmentVariable("LEGATTO_TEST_ADB");
        QVERIFY2(!fakePath.isEmpty(), "CTest must provide LEGATTO_TEST_ADB to the synthetic fixture.");
        QVERIFY(QFile::exists(fakePath));
    }

    void init()
    {
        qunsetenv("LEGATTO_FAKE_MODE");
    }

    void cleanup()
    {
        qunsetenv("LEGATTO_FAKE_MODE");
    }

    void startupIsIdleAndRequiresExplicitTool()
    {
        DiagnosticsWindow window;
        QCOMPARE(window.windowTitle(), QStringLiteral("Legatto Device Studio | Android diagnostics"));
        QVERIFY(button(window, "aboutApplication"));
        QCOMPARE(button(window, "aboutApplication")->accessibleName(), QStringLiteral("About Legatto Device Studio"));
        auto *service = window.findChild<Diagnostics::Service *>();
        QSignalSpy finished(service, &Diagnostics::Service::completed);
        QVERIFY(!service->isBusy());
        QVERIFY(!button(window, "verifyTool")->isEnabled());
        QVERIFY(!button(window, "refreshDevices")->isEnabled());
        QVERIFY(!button(window, "primary")->isEnabled());
        QVERIFY(!button(window, "exportReport")->isEnabled());
        QTest::qWait(50);
        QCOMPARE(finished.size(), 0);
        QString error;
        QTemporaryDir directory;
        const QString path = directory.filePath(QStringLiteral("report.json"));
        QVERIFY(!window.saveReport(path, &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(!QFile::exists(path));
    }

    void invalidToolCannotEnableDeviceActions()
    {
        DiagnosticsWindow window;
        window.findChild<QLineEdit *>(QStringLiteral("adbPath"))->setText(QStringLiteral("adb"));
        button(window, "verifyTool")->click();
        QVERIFY(!button(window, "refreshDevices")->isEnabled());
        QVERIFY(!button(window, "primary")->isEnabled());
    }

    void explicitSelectionThenAtomicExportAndStaleReportInvalidation()
    {
        DiagnosticsWindow window;
        QVERIFY(checkTool(window));
        QVERIFY(scan(window));
        auto *table = window.findChild<QTableWidget *>(QStringLiteral("devices"));
        QVERIFY(table->rowCount() > 0);
        QCOMPARE(table->selectedItems().size(), 0);
        QVERIFY(!button(window, "primary")->isEnabled());
        int authorized = -1;
        for (int row = 0; row < table->rowCount(); ++row) {
            if (table->item(row, 1)->data(Qt::UserRole).toString() == QStringLiteral("device")) {
                authorized = row;
                break;
            }
        }
        QVERIFY(authorized >= 0);
        table->setCurrentCell(authorized, 0);
        table->selectRow(authorized);
        QVERIFY(button(window, "primary")->isEnabled());
        const QString serial = table->item(authorized, 0)->data(Qt::UserRole).toString();
        auto *service = window.findChild<Diagnostics::Service *>();
        QSignalSpy finished(service, &Diagnostics::Service::completed);
        button(window, "primary")->click();
        QVERIFY(!button(window, "exportReport")->isEnabled());
        if (finished.isEmpty()) QVERIFY(finished.wait(10000));
        QVERIFY(button(window, "exportReport")->isEnabled());
        auto *preview = window.findChild<QPlainTextEdit *>(QStringLiteral("reportPreview"));
        QVERIFY(!preview->toPlainText().isEmpty());
        QVERIFY(!preview->toPlainText().contains(serial));
        QVERIFY(!preview->toPlainText().contains(fakePath));
        QTemporaryDir directory;
        const QString path = directory.filePath(QStringLiteral("report.json"));
        QString error;
        QVERIFY2(window.saveReport(path, &error), qPrintable(error));
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), preview->toPlainText().toUtf8());
        file.close();
        QVERIFY(!QJsonDocument::fromJson(preview->toPlainText().toUtf8()).object().isEmpty());

        table->clearSelection();
        QVERIFY(!button(window, "exportReport")->isEnabled());
        QVERIFY(preview->toPlainText().isEmpty());
        QVERIFY(!window.saveReport(path, &error));

        window.findChild<QLineEdit *>(QStringLiteral("adbPath"))->setText(fakePath + QStringLiteral(".changed"));
        QVERIFY(!button(window, "refreshDevices")->isEnabled());
        QCOMPARE(table->rowCount(), 0);
    }

    void unauthorizedDevicesCannotBeInspected()
    {
        qputenv("LEGATTO_FAKE_MODE", "unauthorized");
        DiagnosticsWindow window;
        QVERIFY(checkTool(window));
        QVERIFY(scan(window));
        auto *table = window.findChild<QTableWidget *>(QStringLiteral("devices"));
        QVERIFY(table->rowCount() > 0);
        table->setCurrentCell(0, 0);
        table->selectRow(0);
        QVERIFY(!button(window, "primary")->isEnabled());
        QVERIFY(!button(window, "exportReport")->isEnabled());
    }

    void failedScanNeverSelectsOrExportsOldState()
    {
        DiagnosticsWindow window;
        QVERIFY(checkTool(window));
        qputenv("LEGATTO_FAKE_MODE", "fail");
        QVERIFY(scan(window));
        QCOMPARE(window.findChild<QTableWidget *>(QStringLiteral("devices"))->rowCount(), 0);
        QVERIFY(!button(window, "primary")->isEnabled());
        QVERIFY(!button(window, "exportReport")->isEnabled());
    }

    void cancelAndCloseDoNotLeaveBusyWork()
    {
        DiagnosticsWindow window;
        QVERIFY(checkTool(window));
        qputenv("LEGATTO_FAKE_MODE", "timeout");
        auto *service = window.findChild<Diagnostics::Service *>();
        QSignalSpy finished(service, &Diagnostics::Service::completed);
        button(window, "refreshDevices")->click();
        QVERIFY(service->isBusy());
        QVERIFY(!button(window, "verifyTool")->isEnabled());
        QVERIFY(button(window, "cancelOperation")->isEnabled());
        button(window, "cancelOperation")->click();
        if (finished.isEmpty()) QVERIFY(finished.wait(5000));
        QVERIFY(!service->isBusy());
        QVERIFY(button(window, "refreshDevices")->isEnabled());
        QVERIFY(!button(window, "exportReport")->isEnabled());

        window.show();
        finished.clear();
        button(window, "refreshDevices")->click();
        QVERIFY(service->isBusy());
        window.close();
        if (finished.isEmpty()) QVERIFY(finished.wait(5000));
        QTRY_VERIFY(!window.isVisible());
        QVERIFY(!service->isBusy());
    }
};

QTEST_MAIN(DiagnosticsUiTests)
#include "diagnostics_ui_tests.moc"
