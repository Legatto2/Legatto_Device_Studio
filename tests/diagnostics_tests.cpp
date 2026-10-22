#include <QtTest>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include "diagnostics/diagnosticscore.h"

using namespace Diagnostics;

class DiagnosticsTests : public QObject {
    Q_OBJECT
private:
    QString m_fixture;
    QByteArray m_oldMode;
    QByteArray m_oldTrace;
    QByteArray m_oldSerial;
    QByteArray m_oldSocket;
    QByteArray m_oldAdbTrace;

    static Result result(const QSignalSpy &spy, int index = 0) {
        return qvariant_cast<Result>(spy.at(index).at(0));
    }
    static QByteArray read(const QString &path) {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    }
    static QString copyFixture(const QString &source, const QTemporaryDir &directory) {
#ifdef Q_OS_WIN
        const QString target = directory.filePath(QStringLiteral("adb.exe"));
#else
        const QString target = directory.filePath(QStringLiteral("adb"));
#endif
        if (!QFile::copy(source, target)) return {};
        QFile::setPermissions(target, QFile::permissions(target) | QFileDevice::ExeOwner | QFileDevice::ExeUser);
        return target;
    }
    static void restore(const char *key, const QByteArray &value) {
        if (value.isNull()) qunsetenv(key); else qputenv(key, value);
    }

private slots:
    void initTestCase() {
        m_fixture = qEnvironmentVariable("MOTOTOOL_TEST_ADB");
#ifdef TEST_FAKE_ADB_PATH
        if (m_fixture.isEmpty()) m_fixture = QString::fromUtf8(TEST_FAKE_ADB_PATH);
#endif
        QVERIFY2(!m_fixture.isEmpty() && QFileInfo::exists(m_fixture), "Set MOTOTOOL_TEST_ADB to the compiled fake_adb target (named adb or adb.exe).");
        m_oldMode = qgetenv("MOTOTOOL_FAKE_MODE");
        m_oldTrace = qgetenv("MOTOTOOL_FAKE_TRACE");
        m_oldSerial = qgetenv("ANDROID_SERIAL");
        m_oldSocket = qgetenv("ADB_SERVER_SOCKET");
        m_oldAdbTrace = qgetenv("ADB_TRACE");
    }
    void init() {
        qputenv("MOTOTOOL_FAKE_MODE", "normal");
        qunsetenv("MOTOTOOL_FAKE_TRACE");
        qunsetenv("ANDROID_SERIAL");
        qunsetenv("ADB_SERVER_SOCKET");
        qunsetenv("ADB_TRACE");
    }
    void cleanupTestCase() {
        restore("MOTOTOOL_FAKE_MODE", m_oldMode);
        restore("MOTOTOOL_FAKE_TRACE", m_oldTrace);
        restore("ANDROID_SERIAL", m_oldSerial);
        restore("ADB_SERVER_SOCKET", m_oldSocket);
        restore("ADB_TRACE", m_oldAdbTrace);
    }

    void noImplicitExecutableSearch() {
        Service service;
        QString error;
        QVERIFY(!service.verifyTool());
        QVERIFY(!service.scanDevices());
        QVERIFY(!service.setAdbPath(QStringLiteral("adb"), &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(service.tool().path.isEmpty());
    }
    void scriptCannotMasqueradeAsNativeAdb() {
        QTemporaryDir directory;
#ifdef Q_OS_WIN
        const QString path = directory.filePath(QStringLiteral("adb.exe"));
#else
        const QString path = directory.filePath(QStringLiteral("adb"));
#endif
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("#!/bin/sh\necho unsafe\n");
        file.close();
        QVERIFY(QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));
        Service service;
        QVERIFY(!service.setAdbPath(path));
    }
    void versionFingerprintAndNoRawPath() {
        Service service;
        QSignalSpy spy(&service, &Service::completed);
        QVERIFY(service.setAdbPath(m_fixture));
        QCOMPARE(service.tool().sha256.size(), 64);
        QVERIFY(service.verifyTool());
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 5000);
        const Result verified = result(spy);
        QCOMPARE(verified.status, Status::Success);
        QCOMPARE(verified.operation, QStringLiteral("verify"));
        QCOMPARE(verified.tool.version, QStringLiteral("1.0.41 / 36.0.0-test"));
        QVERIFY(!verified.message.contains(QStringLiteral("private")));
        QVERIFY(!service.isBusy());
    }
    void scanAutoVerifiesAndListsAllDevices() {
        Service service;
        QSignalSpy spy(&service, &Service::completed);
        QVERIFY(service.setAdbPath(m_fixture));
        QVERIFY(service.scanDevices());
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 5000);
        const Result scanned = result(spy);
        QCOMPARE(scanned.status, Status::Success);
        QCOMPARE(scanned.devices.size(), 2);
        QVERIFY(!scanned.tool.version.isEmpty());
        QCOMPARE(scanned.devices.at(0).serial, QStringLiteral("FAKE-DEVICE-A"));
        QCOMPARE(scanned.devices.at(1).serial, QStringLiteral("FAKE-DEVICE-B"));
    }
    void deviceParser_data() {
        QTest::addColumn<QByteArray>("mode");
        QTest::addColumn<Status>("status");
        QTest::addColumn<QString>("state");
        QTest::newRow("fragmented-and-daemon") << QByteArray("partial") << Status::Success << QString("device");
        QTest::newRow("unauthorized") << QByteArray("unauthorized") << Status::Success << QString("unauthorized");
        QTest::newRow("offline") << QByteArray("offline") << Status::Success << QString("offline");
        QTest::newRow("no-permissions") << QByteArray("no_permissions") << Status::Success << QString("no permissions");
        QTest::newRow("empty") << QByteArray("empty") << Status::Success << QString();
        QTest::newRow("duplicate") << QByteArray("duplicate") << Status::InvalidOutput << QString();
        QTest::newRow("malformed") << QByteArray("malformed") << Status::InvalidOutput << QString();
        QTest::newRow("unknown-attribute") << QByteArray("unknown_attribute") << Status::InvalidOutput << QString();
    }
    void deviceParser() {
        QFETCH(QByteArray, mode);
        QFETCH(Status, status);
        QFETCH(QString, state);
        qputenv("MOTOTOOL_FAKE_MODE", mode);
        Service service;
        QSignalSpy spy(&service, &Service::completed);
        QVERIFY(service.setAdbPath(m_fixture));
        QVERIFY(service.scanDevices());
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 5000);
        const Result scanned = result(spy);
        QCOMPARE(scanned.status, status);
        if (!state.isEmpty()) QCOMPARE(scanned.devices.first().state, state);
        else QVERIFY(scanned.devices.isEmpty());
    }
    void explicitSelectionAndAllowlistedCommands() {
        QTemporaryDir directory;
        const QString tracePath = directory.filePath(QStringLiteral("commands.jsonl"));
        qputenv("MOTOTOOL_FAKE_TRACE", tracePath.toUtf8());
        qputenv("ANDROID_SERIAL", "WRONG-DEVICE");
        qputenv("ADB_SERVER_SOCKET", "tcp:untrusted.invalid:5037");
        qputenv("ADB_TRACE", "all");
        Service service;
        QSignalSpy spy(&service, &Service::completed);
        QVERIFY(service.setAdbPath(m_fixture));
        QVERIFY(service.scanDevices());
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 5000);
        QVERIFY(service.inspectDevice(QStringLiteral("FAKE-DEVICE-B")));
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 2, 15000);
        const Result inspected = result(spy, 1);
        QCOMPARE(inspected.status, Status::Success);
        QCOMPARE(inspected.report.value(QStringLiteral("schemaVersion")).toInt(), 1);
        QCOMPARE(inspected.report.value(QStringLiteral("device")).toObject().value(QStringLiteral("model")).toString(), QStringLiteral("Moto Test"));
        int propertyReads = 0;
        for (const QByteArray &line : read(tracePath).split('\n')) {
            if (line.isEmpty()) continue;
            const QJsonObject record = QJsonDocument::fromJson(line).object();
            const QJsonArray args = record.value(QStringLiteral("arguments")).toArray();
            QCOMPARE(args.at(0).toString(), QStringLiteral("-H"));
            QCOMPARE(args.at(1).toString(), QStringLiteral("127.0.0.1"));
            QCOMPARE(args.at(2).toString(), QStringLiteral("-P"));
            QCOMPARE(args.at(3).toString(), QStringLiteral("5037"));
            QVERIFY(record.value(QStringLiteral("ANDROID_SERIAL")).toString().isEmpty());
            QVERIFY(record.value(QStringLiteral("ADB_SERVER_SOCKET")).toString().isEmpty());
            QVERIFY(record.value(QStringLiteral("ADB_TRACE")).toString().isEmpty());
            if (args.at(4).toString() == QStringLiteral("-s")) {
                ++propertyReads;
                QCOMPARE(args.at(5).toString(), QStringLiteral("FAKE-DEVICE-B"));
                QCOMPARE(args.at(6).toString(), QStringLiteral("shell"));
                QCOMPARE(args.at(7).toString(), QStringLiteral("getprop"));
                QCOMPARE(args.size(), 9);
            } else {
                QVERIFY(args.at(4).toString() == QStringLiteral("version") || args.at(4).toString() == QStringLiteral("devices"));
            }
        }
        QCOMPARE(propertyReads, 5);
        const QByteArray json = QJsonDocument(inspected.report).toJson();
        for (const QByteArray &forbidden : {QByteArray("FAKE-DEVICE"), m_fixture.toUtf8(), QByteArray("private"), QByteArray("DUMMY_TOKEN"), QByteArray("transport_id")})
            QVERIFY(!json.contains(forbidden));
    }
    void selectorNeverFallsBackToFirstDevice_data() {
        QTest::addColumn<QString>("selector");
        QTest::newRow("empty") << QString();
        QTest::newRow("unknown") << QString("UNKNOWN");
        QTest::newRow("option") << QString("-d");
        QTest::newRow("shell-characters") << QString("FAKE-DEVICE-A;echo injected");
    }
    void selectorNeverFallsBackToFirstDevice() {
        QFETCH(QString, selector);
        Service service;
        QSignalSpy spy(&service, &Service::completed);
        QVERIFY(service.setAdbPath(m_fixture));
        QVERIFY(service.scanDevices());
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 5000);
        QVERIFY(service.inspectDevice(selector));
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 2, 5000);
        QCOMPARE(result(spy, 1).status, Status::InvalidRequest);
        QVERIFY(result(spy, 1).report.isEmpty());
    }
    void inspectRejectsChangedIdentity_data() {
        QTest::addColumn<QByteArray>("mode");
        QTest::newRow("disconnected") << QByteArray("disconnected");
        QTest::newRow("reconnected-transport") << QByteArray("transport_changed");
        QTest::newRow("unauthorized") << QByteArray("unauthorized");
        QTest::newRow("offline") << QByteArray("offline");
    }
    void inspectRejectsChangedIdentity() {
        QFETCH(QByteArray, mode);
        Service service;
        QSignalSpy spy(&service, &Service::completed);
        QVERIFY(service.setAdbPath(m_fixture));
        QVERIFY(service.scanDevices());
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 5000);
        qputenv("MOTOTOOL_FAKE_MODE", mode);
        QVERIFY(service.inspectDevice(QStringLiteral("FAKE-DEVICE-A")));
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 2, 5000);
        QCOMPARE(result(spy, 1).status, Status::InvalidRequest);
        QVERIFY(result(spy, 1).report.isEmpty());
    }
    void processFailures_data() {
        QTest::addColumn<QByteArray>("mode");
        QTest::addColumn<Status>("status");
        QTest::newRow("nonzero") << QByteArray("fail") << Status::ProcessFailed;
        QTest::newRow("crash") << QByteArray("crash") << Status::Crashed;
        QTest::newRow("timeout") << QByteArray("timeout") << Status::TimedOut;
        QTest::newRow("stdout-limit") << QByteArray("flood") << Status::OutputLimit;
        QTest::newRow("stderr-limit") << QByteArray("stderr_flood") << Status::OutputLimit;
        QTest::newRow("invalid-version") << QByteArray("invalid_version") << Status::InvalidOutput;
        QTest::newRow("stderr-not-fatal") << QByteArray("stderr") << Status::Success;
    }
    void processFailures() {
        QFETCH(QByteArray, mode);
        QFETCH(Status, status);
        qputenv("MOTOTOOL_FAKE_MODE", mode);
        Limits limits;
        limits.commandTimeoutMs = 1000;
        limits.maxOutputBytes = 4096;
        Service service(limits);
        QSignalSpy spy(&service, &Service::completed);
        QVERIFY(service.setAdbPath(m_fixture));
        QVERIFY(service.verifyTool());
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 5000);
        QCOMPARE(result(spy).status, status);
        QVERIFY(!result(spy).message.contains(QStringLiteral("DUMMY_TOKEN")));
        QVERIFY(!result(spy).message.contains(QStringLiteral("private")));
        QVERIFY(!service.isBusy());
        QTest::qWait(30);
        QCOMPARE(spy.count(), 1);
        qputenv("MOTOTOOL_FAKE_MODE", "normal");
        QVERIFY(service.verifyTool());
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 2, 5000);
        QCOMPARE(result(spy, 1).status, Status::Success);
    }
    void busyCancellationAndReuse() {
        qputenv("MOTOTOOL_FAKE_MODE", "timeout");
        Service service;
        QSignalSpy spy(&service, &Service::completed);
        QSignalSpy progress(&service, &Service::progress);
        QVERIFY(service.setAdbPath(m_fixture));
        QVERIFY(service.verifyTool());
        QVERIFY(!service.scanDevices());
        QVERIFY(!service.setAdbPath(m_fixture));
        QTRY_VERIFY_WITH_TIMEOUT(progress.count() > 0, 5000);
        service.cancel();
        service.cancel();
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 5000);
        QCOMPARE(result(spy).status, Status::Cancelled);
        QVERIFY(!service.isBusy());
        qputenv("MOTOTOOL_FAKE_MODE", "normal");
        QVERIFY(service.scanDevices());
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 2, 5000);
        QCOMPARE(result(spy, 1).status, Status::Success);
    }
    void immediateCancelCannotRunStaleKickoff() {
        Service service;
        QSignalSpy spy(&service, &Service::completed);
        QVERIFY(service.setAdbPath(m_fixture));
        QVERIFY(service.verifyTool());
        service.cancel();
        QCOMPARE(spy.count(), 1);
        QCOMPARE(result(spy).status, Status::Cancelled);
        QVERIFY(service.scanDevices());
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 2, 5000);
        QCOMPARE(result(spy, 1).operation, QStringLiteral("scan"));
        QCOMPARE(result(spy, 1).status, Status::Success);
        QCOMPARE(result(spy, 1).devices.size(), 2);
        QTest::qWait(30);
        QCOMPARE(spy.count(), 2);
    }
    void changedExecutableRequiresNewSelection() {
        QTemporaryDir directory;
        const QString path = copyFixture(m_fixture, directory);
        QVERIFY(!path.isEmpty());
        Service service;
        QSignalSpy spy(&service, &Service::completed);
        QVERIFY(service.setAdbPath(path));
        QFile file(path);
        QVERIFY(file.open(QIODevice::Append));
        file.write("changed");
        file.close();
        QVERIFY(service.verifyTool());
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 5000);
        QCOMPARE(result(spy).status, Status::ToolChanged);
        QVERIFY(service.tool().version.isEmpty());
    }
    void failedStartAndStartingCancellation_data() {
        QTest::addColumn<bool>("cancelStarting");
        QTest::newRow("failed-start") << false;
        QTest::newRow("cancel-during-start") << true;
    }
    void failedStartAndStartingCancellation() {
        QFETCH(bool, cancelStarting);
        QTemporaryDir directory;
#ifdef Q_OS_WIN
        const QString path = directory.filePath(QStringLiteral("adb.exe"));
        const QByteArray magic("MZ\0\0", 4);
#elif defined(Q_OS_MACOS)
        const QString path = directory.filePath(QStringLiteral("adb"));
        const QByteArray magic = QByteArray::fromHex("cffaedfe");
#else
        const QString path = directory.filePath(QStringLiteral("adb"));
        const QByteArray magic("\x7f" "ELF", 4);
#endif
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(magic), qint64(4));
        file.close();
        QVERIFY(QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));
        Service service;
        QSignalSpy spy(&service, &Service::completed);
        QVERIFY(service.setAdbPath(path));
        bool cancellationScheduled = false;
        if (cancelStarting) {
            connect(&service, &Service::progress, &service, [&] {
                if (cancellationScheduled) return;
                cancellationScheduled = true;
                QTimer::singleShot(0, &service, &Service::cancel);
            });
        }
        QVERIFY(service.verifyTool());
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 5000);
        const auto status = result(spy).status;
        if (cancelStarting) QVERIFY(status == Status::Cancelled || status == Status::FailedToStart);
        else QCOMPARE(status, Status::FailedToStart);
        QVERIFY(!service.isBusy());
        QTest::qWait(30);
        QCOMPARE(spy.count(), 1);
        QVERIFY(service.setAdbPath(m_fixture));
        QVERIFY(service.verifyTool());
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 2, 5000);
        QCOMPARE(result(spy, 1).status, Status::Success);
    }
    void directSignalCancelAndRestart_data() {
        QTest::addColumn<bool>("duringBusyChanged");
        QTest::newRow("busy-notification") << true;
        QTest::newRow("progress-notification") << false;
    }
    void directSignalCancelAndRestart() {
        QFETCH(bool, duringBusyChanged);
        Service service;
        QSignalSpy spy(&service, &Service::completed);
        bool cancelledOnce = false;
        bool restarted = false;
        bool completedWhileBusy = false;
        const auto cancelOnce = [&] {
            if (!cancelledOnce) { cancelledOnce = true; service.cancel(); }
        };
        if (duringBusyChanged) {
            connect(&service, &Service::busyChanged, &service, [&](bool busy) { if (busy) cancelOnce(); });
        } else {
            connect(&service, &Service::progress, &service, [&](const QString &) { cancelOnce(); });
        }
        connect(&service, &Service::completed, &service, [&](const Result &terminal) {
            completedWhileBusy = completedWhileBusy || service.isBusy();
            if (terminal.operation == QStringLiteral("verify") && terminal.status == Status::Cancelled)
                restarted = service.scanDevices();
        });
        QVERIFY(service.setAdbPath(m_fixture));
        QVERIFY(service.verifyTool());
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 2, 5000);
        QVERIFY(restarted);
        QVERIFY(!completedWhileBusy);
        QCOMPARE(result(spy).status, Status::Cancelled);
        QCOMPARE(result(spy, 1).operation, QStringLiteral("scan"));
        QCOMPARE(result(spy, 1).status, Status::Success);
        QCOMPARE(result(spy, 1).devices.size(), 2);
        QVERIFY(!service.isBusy());
        QTest::qWait(30);
        QCOMPARE(spy.count(), 2);
    }
    void missingAndInvalidProperties_data() {
        QTest::addColumn<QByteArray>("mode");
        QTest::addColumn<Status>("status");
        QTest::newRow("missing") << QByteArray("missing_properties") << Status::Success;
        QTest::newRow("invalid") << QByteArray("bad_property") << Status::InvalidOutput;
        QTest::newRow("invalid-date") << QByteArray("invalid_date") << Status::InvalidOutput;
        QTest::newRow("identifier-redaction") << QByteArray("leaky_property") << Status::Success;
    }
    void missingAndInvalidProperties() {
        QFETCH(QByteArray, mode);
        QFETCH(Status, status);
        Service service;
        QSignalSpy spy(&service, &Service::completed);
        QVERIFY(service.setAdbPath(m_fixture));
        QVERIFY(service.scanDevices());
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 5000);
        qputenv("MOTOTOOL_FAKE_MODE", mode);
        QVERIFY(service.inspectDevice(QStringLiteral("FAKE-DEVICE-A")));
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 2, 15000);
        const Result inspected = result(spy, 1);
        QCOMPARE(inspected.status, status);
        const QByteArray json = QJsonDocument(inspected.report).toJson();
        QVERIFY(!json.contains("FAKE-DEVICE-A"));
        QVERIFY(!json.contains("private"));
        if (mode == "missing_properties") {
            const auto device = inspected.report.value(QStringLiteral("device")).toObject();
            QCOMPARE(device.size(), 5);
            for (const QJsonValue value : device) QVERIFY(value.isNull());
        } else if (mode == "leaky_property") {
            QCOMPARE(inspected.report.value(QStringLiteral("device")).toObject().value(QStringLiteral("manufacturer")).toString(), QStringLiteral("[redacted]"));
        } else {
            QVERIFY(inspected.report.isEmpty());
        }
    }
    void operationHasTotalDeadline() {
        Limits limits;
        limits.commandTimeoutMs = 1000;
        limits.operationTimeoutMs = 1500;
        Service service(limits);
        QSignalSpy spy(&service, &Service::completed);
        QVERIFY(service.setAdbPath(m_fixture));
        QVERIFY(service.scanDevices());
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 5000);
        qputenv("MOTOTOOL_FAKE_MODE", "slow");
        QVERIFY(service.inspectDevice(QStringLiteral("FAKE-DEVICE-A")));
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 2, 5000);
        QCOMPARE(result(spy, 1).status, Status::TimedOut);
        QVERIFY(result(spy, 1).report.isEmpty());
    }
    void pseudonymChangesBetweenSessions() {
        QStringList identifiers;
        for (int index = 0; index < 2; ++index) {
            Service service;
            QSignalSpy spy(&service, &Service::completed);
            QVERIFY(service.setAdbPath(m_fixture));
            QVERIFY(service.scanDevices());
            QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 5000);
            QVERIFY(service.inspectDevice(QStringLiteral("FAKE-DEVICE-A")));
            QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 2, 15000);
            QCOMPARE(result(spy, 1).status, Status::Success);
            identifiers << result(spy, 1).report.value(QStringLiteral("deviceId")).toString();
        }
        QVERIFY(!identifiers.at(0).isEmpty());
        QVERIFY(identifiers.at(0) != identifiers.at(1));
    }
};

QTEST_GUILESS_MAIN(DiagnosticsTests)
#include "diagnostics_tests.moc"
