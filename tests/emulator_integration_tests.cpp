#include "diagnosticswindow.h"

#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QMap>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QSignalSpy>
#include <QTableWidget>
#include <QtTest>

// This executable is deliberately outside ordinary CTest. The opt-in runner
// starts two disposable AVDs, runs the three initial slots, restarts A, then
// invokes freshScanAfterRestart in a new process. It never targets a phone.
class EmulatorIntegrationTests : public QObject
{
    Q_OBJECT

    QString m_adb;
    QString m_toolHash;
    QString m_serialA;
    QString m_serialB;
    QString m_avdA;
    QString m_avdB;
    QString m_evidence;
    QString m_error;
    bool m_killArmed = false;

    bool require(bool condition, const char *message)
    {
        if (!condition) m_error = QString::fromLatin1(message);
        return condition;
    }

    static QMap<QString, QString> propertyKeys()
    {
        return {{QStringLiteral("manufacturer"), QStringLiteral("ro.product.manufacturer")},
                {QStringLiteral("model"), QStringLiteral("ro.product.model")},
                {QStringLiteral("androidVersion"), QStringLiteral("ro.build.version.release")},
                {QStringLiteral("buildId"), QStringLiteral("ro.build.display.id")},
                {QStringLiteral("securityPatch"), QStringLiteral("ro.build.version.security_patch")}};
    }

    static QPushButton *button(DiagnosticsWindow &window, const char *name)
    {
        return window.findChild<QPushButton *>(QString::fromLatin1(name));
    }

    static Diagnostics::Service *service(DiagnosticsWindow &window)
    {
        return window.findChild<Diagnostics::Service *>();
    }

    bool sameTool()
    {
        Diagnostics::Service checker;
        return require(checker.setAdbPath(m_adb)
                           && checker.tool().path == m_adb
                           && checker.tool().sha256 == m_toolHash,
                       "The selected native ADB file changed or is no longer accessible.");
    }

    bool allowedReferenceCommand(const QStringList &args)
    {
        if (args == QStringList{QStringLiteral("devices"), QStringLiteral("-l")}) return true;
        if (args.size() < 4 || args.at(0) != QStringLiteral("-s")
            || (args.at(1) != m_serialA && args.at(1) != m_serialB)) return false;
        if (args.size() == 5 && args.at(2) == QStringLiteral("shell")
            && args.at(3) == QStringLiteral("getprop")) {
            return propertyKeys().values().contains(args.at(4))
                || args.at(4) == QStringLiteral("ro.kernel.qemu")
                || args.at(4) == QStringLiteral("ro.boot.qemu");
        }
        if (args.size() == 5 && args.mid(2) == QStringList{
                QStringLiteral("emu"), QStringLiteral("avd"), QStringLiteral("name")}) return true;
        if (args.size() == 4 && args.at(1) == m_serialA
            && args.mid(2) == QStringList{QStringLiteral("emu"), QStringLiteral("kill")}
            && m_killArmed) {
            m_killArmed = false; // One command, following a fresh identity check.
            return true;
        }
        return false;
    }

    // Independent, bounded reference reads. No shell, PATH lookup, remote ADB
    // server, raw transcript persistence, or arbitrary command entry point.
    bool reference(const QStringList &args, QByteArray *output, int timeoutMs = 10000)
    {
        if (!require(timeoutMs > 0, "The reference command deadline has already elapsed.")
            || !require(allowedReferenceCommand(args), "Reference command is outside the test allowlist.")
            || !sameTool()) return false;
        QProcess process;
        auto environment = QProcessEnvironment::systemEnvironment();
        for (const QString &key : {QStringLiteral("ANDROID_SERIAL"), QStringLiteral("ADB_SERVER_SOCKET"),
                 QStringLiteral("ANDROID_ADB_SERVER_PORT"), QStringLiteral("ADB_SERVER_PORT"),
                 QStringLiteral("ADB_TRACE")}) environment.remove(key);
        environment.insert(QStringLiteral("ADB_MDNS_AUTO_CONNECT"), QStringLiteral("0"));
        process.setProcessEnvironment(environment);
        process.setProcessChannelMode(QProcess::SeparateChannels);
        process.setProgram(m_adb);
        process.setArguments(QStringList{QStringLiteral("-H"), QStringLiteral("127.0.0.1"),
                                 QStringLiteral("-P"), QStringLiteral("5037")} + args);
        QElapsedTimer elapsed;
        elapsed.start();
        process.start(QIODevice::ReadOnly);
        if (!require(process.waitForStarted(qMin(timeoutMs, 3000)), "Reference ADB failed to start.")) {
            if (process.state() != QProcess::NotRunning) {
                process.kill();
                process.waitForFinished(2000);
            }
            return false;
        }
        QByteArray bytes;
        qint64 total = 0;
        bool bounded = true;
        auto drain = [&] {
            process.setReadChannel(QProcess::StandardOutput);
            while (process.bytesAvailable() > 0) {
                const QByteArray chunk = process.read(4096);
                total += chunk.size();
                if (total > 65536) { bounded = false; return; }
                bytes += chunk;
            }
            process.setReadChannel(QProcess::StandardError);
            while (process.bytesAvailable() > 0) {
                const QByteArray chunk = process.read(4096);
                total += chunk.size();
                if (total > 65536) { bounded = false; return; }
            }
            process.setReadChannel(QProcess::StandardOutput);
        };
        while (process.state() != QProcess::NotRunning && elapsed.elapsed() < timeoutMs) {
            const int remaining = timeoutMs - int(elapsed.elapsed());
            if (remaining <= 0) break;
            process.waitForReadyRead(qMin(50, remaining));
            drain();
            if (!bounded) break;
        }
        drain();
        if (!bounded || process.state() != QProcess::NotRunning) {
            process.kill(); // Only this client process; never kill the ADB server.
            process.waitForFinished(2000);
            return require(false, "Reference ADB exceeded its time or output limit.");
        }
        if (!require(process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0,
                     "Reference ADB did not complete successfully.") || !sameTool()) return false;
        if (output) *output = bytes;
        return true;
    }

    bool inventory(QMap<QString, QString> *devices, int timeoutMs = 10000)
    {
        QByteArray bytes;
        if (!reference({QStringLiteral("devices"), QStringLiteral("-l")}, &bytes, timeoutMs)) return false;
        QMap<QString, QString> parsed;
        bool header = false;
        const auto lines = QString::fromUtf8(bytes).split(QLatin1Char('\n'));
        for (QString line : lines) {
            line = line.trimmed();
            if (line.isEmpty()) continue;
            if (line == QStringLiteral("List of devices attached")) {
                if (!require(!header, "Reference inventory repeated its header.")) return false;
                header = true;
                continue;
            }
            if (!header && (line.startsWith(QStringLiteral("* daemon not running;"))
                           || line == QStringLiteral("* daemon started successfully"))) continue;
            const auto fields = line.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
            if (!require(header && fields.size() >= 2 && !parsed.contains(fields.at(0)),
                         "Reference inventory was malformed or ambiguous.")) return false;
            parsed.insert(fields.at(0), fields.at(1));
        }
        if (!require(header, "Reference inventory did not contain its expected header.")) return false;
        *devices = parsed;
        return true;
    }

    bool exactPairConnected()
    {
        QMap<QString, QString> devices;
        return inventory(&devices)
            && require(devices.size() == 2 && devices.value(m_serialA) == QStringLiteral("device")
                           && devices.value(m_serialB) == QStringLiteral("device"),
                       "The local ADB server must contain exactly the two authorized disposable emulators.");
    }

    bool emulatorIdentity(const QString &serial, const QString &expectedAvd)
    {
        QByteArray kernel, boot, avd;
        if (!reference({QStringLiteral("-s"), serial, QStringLiteral("shell"),
                QStringLiteral("getprop"), QStringLiteral("ro.kernel.qemu")}, &kernel)
            || !reference({QStringLiteral("-s"), serial, QStringLiteral("shell"),
                QStringLiteral("getprop"), QStringLiteral("ro.boot.qemu")}, &boot)
            || !require(kernel.trimmed() == "1" || boot.trimmed() == "1",
                        "The selected target does not identify itself as an emulator.")
            || !reference({QStringLiteral("-s"), serial, QStringLiteral("emu"),
                QStringLiteral("avd"), QStringLiteral("name")}, &avd)) return false;
        QStringList names;
        for (const auto &line : QString::fromUtf8(avd).split(QLatin1Char('\n'))) {
            if (!line.trimmed().isEmpty()) names.append(line.trimmed());
        }
        if (names.size() == 2 && names.last() == QStringLiteral("OK")) names.removeLast();
        return require(names.size() == 1 && names.first() == expectedAvd,
                       "The emulator AVD name did not match the explicitly authorized disposable AVD.");
    }

    bool waitResult(QSignalSpy &finished, DiagnosticsWindow &window, const QString &operation,
                    Diagnostics::Status expected, Diagnostics::Result *result = nullptr)
    {
        QElapsedTimer elapsed;
        elapsed.start();
        while (finished.isEmpty() && elapsed.elapsed() < 55000) finished.wait(250);
        if (!require(finished.size() == 1, "The UI operation did not produce exactly one terminal result."))
            return false;
        const auto value = qvariant_cast<Diagnostics::Result>(finished.first().first());
        if (!require(value.operation == operation, "The UI returned a result for an unexpected operation.")) return false;
        if (value.status != expected) {
            m_error = QStringLiteral("The %1 UI operation ended with status %2; expected %3.")
                .arg(operation).arg(int(value.status)).arg(int(expected));
            return false;
        }
        if (!require(!service(window)->isBusy(), "The service was still busy at completion.")) return false;
        if (result) *result = value;
        return true;
    }

    bool clickAndWait(DiagnosticsWindow &window, const char *name, const QString &operation,
                      Diagnostics::Status expected = Diagnostics::Status::Success,
                      Diagnostics::Result *result = nullptr)
    {
        auto *control = button(window, name);
        if (!require(control && control->isEnabled(), "The expected UI action was not available.")) return false;
        QSignalSpy finished(service(window), &Diagnostics::Service::completed);
        control->click();
        return waitResult(finished, window, operation, expected, result);
    }

    bool noReport(DiagnosticsWindow &window)
    {
        return require(!button(window, "exportReport")->isEnabled()
                           && window.findChild<QPlainTextEdit *>(QStringLiteral("reportPreview"))->toPlainText().isEmpty()
                           && window.findChild<QTableWidget *>(QStringLiteral("properties"))->rowCount() == 0,
                       "An incomplete or stale operation left an exportable report.");
    }

    bool scan(DiagnosticsWindow &window, const QSet<QString> &expected)
    {
        if (!clickAndWait(window, "refreshDevices", QStringLiteral("scan"))) return false;
        auto *table = window.findChild<QTableWidget *>(QStringLiteral("devices"));
        QSet<QString> actual;
        for (int row = 0; row < table->rowCount(); ++row) {
            if (!require(table->item(row, 0) && table->item(row, 1)
                             && table->item(row, 1)->data(Qt::UserRole).toString() == QStringLiteral("device"),
                         "The UI inventory contained an unexpected or unauthorized entry.")) return false;
            actual.insert(table->item(row, 0)->data(Qt::UserRole).toString());
        }
        return require(actual == expected && table->rowCount() == expected.size(),
                       "The UI inventory did not match the explicitly supplied emulator set.")
            && require(table->selectedItems().isEmpty() && table->currentRow() == -1
                           && !button(window, "primary")->isEnabled(),
                       "A refreshed device was selected automatically.") && noReport(window);
    }

    bool prepare(DiagnosticsWindow &window)
    {
        window.show();
        if (!require(service(window) && !service(window)->isBusy()
                         && !button(window, "refreshDevices")->isEnabled()
                         && !button(window, "primary")->isEnabled(),
                     "The diagnostics window did not start idle with explicit tool selection.")) return false;
        window.findChild<QLineEdit *>(QStringLiteral("adbPath"))->setText(m_adb);
        Diagnostics::Result verified;
        return clickAndWait(window, "verifyTool", QStringLiteral("verify"), Diagnostics::Status::Success, &verified)
            && require(verified.tool.path == m_adb && verified.tool.sha256 == m_toolHash
                           && !verified.tool.version.isEmpty(), "The UI tool fingerprint or version was not verified.")
            && scan(window, {m_serialA, m_serialB});
    }

    bool select(DiagnosticsWindow &window, const QString &serial)
    {
        auto *table = window.findChild<QTableWidget *>(QStringLiteral("devices"));
        int found = -1;
        for (int row = 0; row < table->rowCount(); ++row) {
            if (table->item(row, 0)->data(Qt::UserRole).toString() == serial) {
                if (!require(found == -1, "The requested UI target was duplicated.")) return false;
                found = row;
            }
        }
        if (!require(found >= 0, "The explicitly requested target was absent from the UI inventory.")) return false;
        table->setCurrentCell(found, 0);
        table->selectRow(found);
        return require(button(window, "primary")->isEnabled()
                           && table->item(table->currentRow(), 0)->data(Qt::UserRole).toString() == serial,
                       "The UI did not select the explicitly requested emulator.") && noReport(window);
    }

    bool privateStringsAbsent(const QJsonValue &value)
    {
        if (value.isObject()) {
            const auto object = value.toObject();
            for (auto it = object.constBegin(); it != object.constEnd(); ++it)
                if (!privateStringsAbsent(it.value())) return false;
        } else if (value.isString()) {
            const QString text = value.toString();
            for (const QString &forbidden : {m_serialA, m_serialB, m_adb, m_avdA, m_avdB, m_evidence}) {
                if (text.contains(forbidden)) return false;
            }
        }
        return !value.isArray(); // The report schema has no arrays.
    }

    bool checkAndSaveReport(DiagnosticsWindow &window, const QString &serial,
                            const QString &filename, QString *pseudonym = nullptr)
    {
        auto *preview = window.findChild<QPlainTextEdit *>(QStringLiteral("reportPreview"));
        const QByteArray bytes = preview->toPlainText().toUtf8();
        QJsonParseError parse;
        const QJsonDocument document = QJsonDocument::fromJson(bytes, &parse);
        if (!require(button(window, "exportReport")->isEnabled() && parse.error == QJsonParseError::NoError
                         && document.isObject(), "A successful inspection did not expose a valid report preview.")) return false;
        const QJsonObject report = document.object();
        QStringList expectedKeys{QStringLiteral("schemaVersion"), QStringLiteral("application"),
            QStringLiteral("applicationVersion"), QStringLiteral("generatedAt"), QStringLiteral("deviceId"),
            QStringLiteral("transport"), QStringLiteral("tool"), QStringLiteral("device")};
        expectedKeys.sort();
        const auto properties = report.value(QStringLiteral("device")).toObject();
        const auto tool = report.value(QStringLiteral("tool")).toObject();
        const QString id = report.value(QStringLiteral("deviceId")).toString();
        const QString generated = report.value(QStringLiteral("generatedAt")).toString();
        if (!require(report.keys() == expectedKeys && report.value(QStringLiteral("schemaVersion")).toInt() == 1
                         && report.value(QStringLiteral("application")).toString() == QStringLiteral("Legatto Device Studio")
                         && report.value(QStringLiteral("applicationVersion")).toString() == QStringLiteral("0.1.0-dev")
                         && report.value(QStringLiteral("transport")).toString() == QStringLiteral("adb")
                         && QRegularExpression(QStringLiteral("^device-[a-f0-9]{24}$")).match(id).hasMatch()
                         && generated.endsWith(QLatin1Char('Z'))
                         && QDateTime::fromString(generated, Qt::ISODateWithMs).isValid()
                         && properties.keys() == propertyKeys().keys()
                         && tool.keys() == QStringList{QStringLiteral("sha256"), QStringLiteral("version")}
                         && tool.value(QStringLiteral("sha256")).toString() == m_toolHash
                         && tool.value(QStringLiteral("version")).toString() == service(window)->tool().version,
                     "The report schema, metadata, or fingerprint did not match the export contract.")
            || !require(privateStringsAbsent(report), "The report included a private test identity or local path.")) return false;
        const auto keys = propertyKeys();
        for (auto it = keys.constBegin(); it != keys.constEnd(); ++it) {
            QByteArray referenceValue;
            if (!reference({QStringLiteral("-s"), serial, QStringLiteral("shell"),
                    QStringLiteral("getprop"), it.value()}, &referenceValue)) return false;
            const QString expected = QString::fromUtf8(referenceValue).trimmed();
            const auto actual = properties.value(it.key());
            if (!require(expected.isEmpty() ? actual.isNull() : actual.isString() && actual.toString() == expected,
                         "A reported property did not match an independent query of the explicitly selected emulator.")) return false;
        }
        const QString destination = QDir(m_evidence).filePath(QStringLiteral("report-") + filename);
        if (!require(!QFileInfo(destination).isSymLink() && window.saveReport(destination),
                     "The reviewed report could not be saved to the evidence directory.")) return false;
        QFile saved(destination);
        if (!require(saved.open(QIODevice::ReadOnly) && saved.readAll() == bytes,
                     "The exported bytes did not exactly match the report preview.")) return false;
        if (pseudonym) *pseudonym = id;
        return true;
    }

    bool inspectAndSave(DiagnosticsWindow &window, const QString &serial, const QString &filename,
                        QString *pseudonym = nullptr)
    {
        return select(window, serial)
            && clickAndWait(window, "primary", QStringLiteral("inspect"))
            && checkAndSaveReport(window, serial, filename, pseudonym);
    }

private slots:
    void initTestCase()
    {
        QVERIFY2(qEnvironmentVariable("LEGATTO_EMULATOR_ALLOW_DISPOSABLE") == QStringLiteral("1"),
                 "This suite requires explicit opt-in for two disposable emulators; it never skips missing prerequisites.");
        m_serialA = qEnvironmentVariable("LEGATTO_EMULATOR_SERIAL_A");
        m_serialB = qEnvironmentVariable("LEGATTO_EMULATOR_SERIAL_B");
        m_avdA = qEnvironmentVariable("LEGATTO_EMULATOR_AVD_A");
        m_avdB = qEnvironmentVariable("LEGATTO_EMULATOR_AVD_B");
        m_evidence = qEnvironmentVariable("LEGATTO_EMULATOR_EVIDENCE_DIR");
        const QRegularExpression serialPattern(QStringLiteral("^emulator-([0-9]{4,5})$"));
        for (const QString &serial : {m_serialA, m_serialB}) {
            const auto match = serialPattern.match(serial);
            const int port = match.captured(1).toInt();
            QVERIFY2(match.hasMatch() && port >= 1024 && port <= 65534 && port % 2 == 0,
                     "Supply explicit standard emulator serials with distinct even console ports.");
        }
        QVERIFY2(m_serialA != m_serialB, "The two disposable emulator serials must be distinct.");
        const QRegularExpression namePattern(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9_.-]{2,79}$"));
        QVERIFY2(namePattern.match(m_avdA).hasMatch() && namePattern.match(m_avdB).hasMatch() && m_avdA != m_avdB,
                 "Supply two distinct explicit disposable AVD names.");
        const QFileInfo evidence(m_evidence);
        QVERIFY2(evidence.isAbsolute() && evidence.isDir() && !evidence.isSymLink() && evidence.isWritable(),
                 "Supply an existing writable absolute evidence directory without a symlink destination.");
        m_evidence = evidence.canonicalFilePath();
        QVERIFY2(!m_evidence.isEmpty(), "The evidence directory is no longer accessible.");
        Diagnostics::Service checker;
        QVERIFY2(checker.setAdbPath(qEnvironmentVariable("LEGATTO_EMULATOR_ADB")),
                 "Supply the absolute native executable from the intended official Android Platform-Tools installation.");
        m_adb = checker.tool().path;
        m_toolHash = checker.tool().sha256;
        QVERIFY2(exactPairConnected(), qPrintable(m_error));
        QVERIFY2(emulatorIdentity(m_serialA, m_avdA), qPrintable(m_error));
        QVERIFY2(emulatorIdentity(m_serialB, m_avdB), qPrintable(m_error));
    }

    void realToolDiscoveryAndReports()
    {
        QVERIFY2(exactPairConnected(), qPrintable(m_error));
        DiagnosticsWindow window;
        QVERIFY2(prepare(window), qPrintable(m_error));
        QString firstA, firstB, repeatedA;
        QVERIFY2(inspectAndSave(window, m_serialA, QStringLiteral("discovery-a.json"), &firstA), qPrintable(m_error));
        QVERIFY2(inspectAndSave(window, m_serialB, QStringLiteral("discovery-b.json"), &firstB), qPrintable(m_error));
        QVERIFY2(inspectAndSave(window, m_serialA, QStringLiteral("discovery-a-repeat.json"), &repeatedA), qPrintable(m_error));
        QVERIFY2(firstA != firstB && firstA == repeatedA,
                 "Device pseudonyms must differ across targets and remain stable within one service session.");
    }

    void cancellationAndRecovery()
    {
        QVERIFY2(exactPairConnected(), qPrintable(m_error));
        DiagnosticsWindow window;
        QVERIFY2(prepare(window), qPrintable(m_error));
        QVERIFY2(select(window, m_serialA), qPrintable(m_error));
        auto *runner = service(window);
        QSignalSpy finished(runner, &Diagnostics::Service::completed);
        int cancellations = 0;
        // Cancel at the first progress notification, before dispatch. This tests
        // the real UI/service lifecycle without depending on emulator speed.
        const auto connection = connect(runner, &Diagnostics::Service::progress, &window, [&] {
            if (cancellations == 0 && runner->isBusy()) {
                ++cancellations;
                button(window, "cancelOperation")->click();
            }
        });
        button(window, "primary")->click();
        QVERIFY2(waitResult(finished, window, QStringLiteral("inspect"), Diagnostics::Status::Cancelled), qPrintable(m_error));
        disconnect(connection);
        QVERIFY2(cancellations == 1, "The cancel action must run exactly once.");
        QVERIFY2(noReport(window), qPrintable(m_error));
        const QString forbiddenExport = QDir(m_evidence).filePath(QStringLiteral("cancelled-must-not-exist.json"));
        QVERIFY2(!QFile::exists(forbiddenExport) && !window.saveReport(forbiddenExport) && !QFile::exists(forbiddenExport),
                 "A cancelled operation must not create an export file.");
        QTest::qWait(100);
        QVERIFY2(finished.size() == 1, "Cancellation delivered more than one terminal result.");
        QVERIFY2(scan(window, {m_serialA, m_serialB}), qPrintable(m_error));
        QVERIFY2(inspectAndSave(window, m_serialA, QStringLiteral("cancellation-recovery-a.json")), qPrintable(m_error));
        QVERIFY2(finished.size() == 3, "Cancellation or recovery delivered an unexpected extra terminal result.");
    }

    void disconnectRejectsStaleSelection()
    {
        QVERIFY2(exactPairConnected(), qPrintable(m_error));
        DiagnosticsWindow window;
        QVERIFY2(prepare(window), qPrintable(m_error));
        QVERIFY2(inspectAndSave(window, m_serialA, QStringLiteral("before-disconnect-a.json")), qPrintable(m_error));
        // Only the named, newly reverified disposable AVD may be stopped. A
        // physical device or unrelated emulator can never reach this command.
        QVERIFY2(exactPairConnected(), qPrintable(m_error));
        QVERIFY2(emulatorIdentity(m_serialA, m_avdA), qPrintable(m_error));
        m_killArmed = true;
        QVERIFY2(reference({QStringLiteral("-s"), m_serialA, QStringLiteral("emu"), QStringLiteral("kill")}, nullptr),
                 qPrintable(m_error));
        bool absent = false;
        QElapsedTimer elapsed;
        elapsed.start();
        while (elapsed.elapsed() < 30000) {
            QMap<QString, QString> devices;
            const int remaining = 30000 - int(elapsed.elapsed());
            QVERIFY2(inventory(&devices, qMin(5000, remaining)), qPrintable(m_error));
            QVERIFY2(devices.value(m_serialB) == QStringLiteral("device") && devices.size() <= 2
                         && (devices.size() == 1 || devices.contains(m_serialA)),
                     "Unexpected devices appeared while awaiting the disposable emulator disconnect.");
            if (!devices.contains(m_serialA)) { absent = true; break; }
            QTest::qWait(100);
        }
        QVERIFY2(absent, "The disposable emulator did not disappear within the disconnect deadline.");
        // The existing table still selects A. The service must revalidate it,
        // reject it and clear the report instead of silently querying B.
        QVERIFY2(clickAndWait(window, "primary", QStringLiteral("inspect"), Diagnostics::Status::InvalidRequest),
                 qPrintable(m_error));
        QVERIFY2(noReport(window), qPrintable(m_error));
        const QString forbiddenExport = QDir(m_evidence).filePath(QStringLiteral("stale-selection-must-not-exist.json"));
        QVERIFY2(!QFile::exists(forbiddenExport) && !window.saveReport(forbiddenExport) && !QFile::exists(forbiddenExport),
                 "A disconnected selection must not create an export file.");
        QVERIFY2(window.findChild<QTableWidget *>(QStringLiteral("devices"))->rowCount() == 0
                     && !button(window, "primary")->isEnabled(),
                 "A stale selection must require a fresh device scan.");
        QVERIFY2(scan(window, {m_serialB}), qPrintable(m_error));
        QVERIFY2(inspectAndSave(window, m_serialB, QStringLiteral("disconnect-recovery-b.json")), qPrintable(m_error));
    }

    void freshScanAfterRestart()
    {
        // The runner restarted A before this new Qt process was launched. This
        // verifies fresh discovery and does not claim persistence of stale UI
        // state, connection identity or pseudonyms across processes.
        QVERIFY2(exactPairConnected(), qPrintable(m_error));
        QVERIFY2(emulatorIdentity(m_serialA, m_avdA), qPrintable(m_error));
        DiagnosticsWindow window;
        QVERIFY2(prepare(window), qPrintable(m_error));
        QString idA, idB;
        QVERIFY2(inspectAndSave(window, m_serialA, QStringLiteral("restart-a.json"), &idA), qPrintable(m_error));
        QVERIFY2(inspectAndSave(window, m_serialB, QStringLiteral("restart-b.json"), &idB), qPrintable(m_error));
        QVERIFY2(idA != idB, "The restarted targets must have distinct session pseudonyms.");
    }
};

QTEST_MAIN(EmulatorIntegrationTests)
#include "emulator_integration_tests.moc"
