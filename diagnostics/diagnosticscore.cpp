#include "diagnosticscore.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonValue>
#include <QProcessEnvironment>
#include <QPointer>
#include <QRegularExpression>
#include <QSet>
#include <QUuid>

namespace {

struct Property {
    const char *key;
    const char *reportName;
    const char *pattern;
};

// Only these five device properties are available through the diagnostics API.
const Property properties[] = {
    {"ro.product.manufacturer", "manufacturer", "^[\\p{L}\\p{N} ._()+-]{1,120}$"},
    {"ro.product.model", "model", "^[\\p{L}\\p{N} ._()+-]{1,120}$"},
    {"ro.build.version.release", "androidVersion", "^[A-Za-z0-9._+-]{1,32}$"},
    {"ro.build.display.id", "buildId", "^[A-Za-z0-9 ._+-]{1,128}$"},
    {"ro.build.version.security_patch", "securityPatch", "^[0-9]{4}-[0-9]{2}-[0-9]{2}$"}
};
const int propertyCount = int(sizeof(properties) / sizeof(properties[0]));

bool fail(QString *error, const QString &message)
{
    if (error) *error = message;
    return false;
}

bool nativeHeader(const QByteArray &bytes)
{
#ifdef Q_OS_WIN
    return bytes.startsWith("MZ");
#elif defined(Q_OS_MACOS)
    const QByteArray head = bytes.left(4).toHex();
    return head == "feedface" || head == "feedfacf" || head == "cefaedfe"
        || head == "cffaedfe" || head == "cafebabe" || head == "bebafeca"
        || head == "cafebabf" || head == "bfbafeca";
#else
    return bytes.startsWith(QByteArray("\x7f" "ELF", 4));
#endif
}

bool fingerprint(const QString &path, Diagnostics::Tool *tool, QString *error)
{
    if (error) error->clear();
    const QFileInfo info(path);
#ifdef Q_OS_WIN
    const bool validName = info.fileName().compare(QStringLiteral("adb.exe"), Qt::CaseInsensitive) == 0;
#else
    const bool validName = info.fileName() == QStringLiteral("adb");
#endif
    if (!info.isAbsolute() || !validName || !info.isFile() || info.isSymLink()
        || !info.isExecutable() || info.size() <= 0 || info.size() > 128 * 1024 * 1024)
        return fail(error, QStringLiteral("Choose the native adb executable from an extracted Android Platform-Tools folder using its full path."));

    const QString canonical = info.canonicalFilePath();
    QFile file(canonical);
    if (canonical.isEmpty() || !file.open(QIODevice::ReadOnly))
        return fail(error, QStringLiteral("The selected ADB executable cannot be read."));
    if (!nativeHeader(file.peek(4)))
        return fail(error, QStringLiteral("The selected file is not a native executable for this platform."));
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file))
        return fail(error, QStringLiteral("The selected ADB executable could not be fingerprinted."));
    const QFileInfo after(canonical);
    if (after.isSymLink() || !after.isFile() || after.size() != info.size()
        || after.lastModified() != info.lastModified()
        || after.canonicalFilePath() != canonical)
        return fail(error, QStringLiteral("The selected executable changed while it was being checked."));
    tool->path = canonical;
    tool->sha256 = QString::fromLatin1(hash.result().toHex());
    tool->version.clear();
    return true;
}

bool validSerial(const QString &serial)
{
    static const QRegularExpression expression(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9._:-]{0,127}$"));
    return expression.match(serial).hasMatch();
}

bool decode(const QByteArray &bytes, QString *text)
{
    *text = QString::fromUtf8(bytes);
    return text->toUtf8() == bytes && !text->contains(QChar(0));
}

bool parseVersion(const QByteArray &bytes, QString *version)
{
    QString text;
    if (!decode(bytes, &text)) return false;
    static const QRegularExpression adb(QStringLiteral("^Android Debug Bridge version ([0-9][A-Za-z0-9._+-]{0,63})$"));
    static const QRegularExpression platform(QStringLiteral("^Version ([0-9][A-Za-z0-9._+-]{0,63})$"));
    QString adbVersion, platformVersion;
    for (const QString &raw : text.split(QLatin1Char('\n'))) {
        const QString line = raw.trimmed();
        const auto adbMatch = adb.match(line);
        if (adbMatch.hasMatch()) {
            if (!adbVersion.isEmpty()) return false;
            adbVersion = adbMatch.captured(1);
        }
        const auto platformMatch = platform.match(line);
        if (platformMatch.hasMatch()) {
            if (!platformVersion.isEmpty()) return false;
            platformVersion = platformMatch.captured(1);
        }
    }
    if (adbVersion.isEmpty()) return false;
    // Deliberately omit "Installed as ..." and all other free-form lines.
    *version = adbVersion + (platformVersion.isEmpty() ? QString() : QStringLiteral(" / ") + platformVersion);
    return true;
}

bool parseDevices(const QByteArray &bytes, QList<Diagnostics::Device> *devices)
{
    devices->clear();
    QString text;
    if (!decode(bytes, &text)) return false;
    bool header = false;
    QSet<QString> serials;
    QSet<QString> transports;
    const QRegularExpression whitespace(QStringLiteral("\\s+"));
    const QRegularExpression transport(QStringLiteral("^[1-9][0-9]{0,18}$"));
    for (const QString &raw : text.split(QLatin1Char('\n'))) {
        const QString line = raw.trimmed();
        if (line.isEmpty()) continue;
        if (!header && (line.startsWith(QStringLiteral("* daemon not running;"))
                        || line == QStringLiteral("* daemon started successfully"))) continue;
        if (line == QStringLiteral("List of devices attached")) {
            if (header) return false;
            header = true;
            continue;
        }
        if (!header || devices->size() >= 256) return false;
        const QStringList fields = line.split(whitespace, Qt::SkipEmptyParts);
        if (fields.size() < 2 || !validSerial(fields.at(0)) || serials.contains(fields.at(0))) return false;
        Diagnostics::Device device;
        device.serial = fields.at(0);
        device.state = fields.at(1);
        int firstAttribute = 2;
        if (device.state == QStringLiteral("no") && fields.size() >= 3
            && fields.at(2) == QStringLiteral("permissions")) {
            device.state = QStringLiteral("no permissions");
            // Linux may append a human-readable USB permission explanation.
            firstAttribute = fields.size();
        } else if (device.state != QStringLiteral("device") && device.state != QStringLiteral("offline")
                   && device.state != QStringLiteral("unauthorized") && device.state != QStringLiteral("recovery")
                   && device.state != QStringLiteral("sideload") && device.state != QStringLiteral("bootloader")) {
            return false;
        }
        QSet<QString> attributes;
        for (int index = firstAttribute; index < fields.size(); ++index) {
            const QString field = fields.at(index);
            const int colon = field.indexOf(QLatin1Char(':'));
            if (colon <= 0 || colon == field.size() - 1) return false;
            const QString key = field.left(colon), value = field.mid(colon + 1);
            if (attributes.contains(key) || value.size() > 256) return false;
            attributes.insert(key);
            if (key == QStringLiteral("model")) device.model = value;
            else if (key == QStringLiteral("product")) device.product = value;
            else if (key == QStringLiteral("transport_id")) {
                if (!transport.match(value).hasMatch() || transports.contains(value)) return false;
                device.transportId = value;
                transports.insert(value);
            } else if (key != QStringLiteral("device") && key != QStringLiteral("usb")) {
                return false;
            }
        }
        serials.insert(device.serial);
        devices->append(device);
    }
    return header;
}

bool sameDevice(const Diagnostics::Device &before, const Diagnostics::Device &now)
{
    return before.serial == now.serial && before.transportId == now.transportId
        && before.product == now.product && before.model == now.model && now.isAuthorized();
}

} // namespace

namespace Diagnostics {

Service::Service(QObject *parent) : Service(Limits(), parent) {}

Service::Service(const Limits &limits, QObject *parent)
    : QObject(parent), m_limits(limits), m_sessionSalt(QUuid::createUuid().toRfc4122())
{
    qRegisterMetaType<Diagnostics::Result>();
    m_limits.commandTimeoutMs = qBound(1, m_limits.commandTimeoutMs, 60000);
    m_limits.operationTimeoutMs = qBound(1, m_limits.operationTimeoutMs, 180000);
    m_limits.maxOutputBytes = qBound<qint64>(qint64(1024), m_limits.maxOutputBytes, qint64(1024 * 1024));
    m_commandTimer.setSingleShot(true);
    m_operationTimer.setSingleShot(true);
    connect(&m_commandTimer, &QTimer::timeout, this, [this] {
        requestEnd(Status::TimedOut, QStringLiteral("ADB exceeded the command deadline. No diagnostic report was produced."));
    });
    connect(&m_operationTimer, &QTimer::timeout, this, [this] {
        requestEnd(Status::TimedOut, QStringLiteral("The diagnostic operation exceeded its total deadline."));
    });
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &Service::collectOutput);
    connect(&m_process, &QProcess::readyReadStandardError, this, &Service::collectOutput);
    connect(&m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, &Service::processFinished);
    connect(&m_process, &QProcess::started, this, [this] { if (m_ending) m_process.kill(); });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (!m_busy) return;
        if (error == QProcess::FailedToStart) {
            // FailedToStart has no matching finished signal. Cancellation may
            // already be pending if the executable failed while Starting.
            m_commandActive = false;
            if (m_ending) { finalize(); return; }
            requestEnd(Status::FailedToStart, QStringLiteral("ADB could not start. Check that Platform-Tools is complete and matches this computer."));
        } else if (m_ending) {
            return;
        } else if (error == QProcess::Crashed)
            requestEnd(Status::Crashed, QStringLiteral("ADB exited unexpectedly. The operation did not complete."));
        else
            requestEnd(Status::ProcessFailed, QStringLiteral("ADB encountered a process communication error."));
    });
}

Service::~Service()
{
    disconnect(&m_process, nullptr, this, nullptr);
    if (m_process.state() != QProcess::NotRunning) {
        m_process.kill();
        m_process.waitForFinished(1000);
    }
}

bool Service::setAdbPath(const QString &path, QString *error)
{
    if (m_busy) return fail(error, QStringLiteral("Wait for or cancel the active operation before changing ADB."));
    m_tool = Tool();
    m_lastDevices.clear();
    return fingerprint(path, &m_tool, error);
}

bool Service::begin(const QString &operation)
{
    if (m_busy || m_commandActive || m_process.state() != QProcess::NotRunning || m_tool.path.isEmpty()) return false;
    m_busy = true;
    m_ending = false;
    ++m_generation;
    m_operation = operation;
    m_resultDevices.clear();
    m_properties = QJsonObject();
    m_report = QJsonObject();
    m_propertyIndex = 0;
    m_stdout.clear();
    m_operationTimer.start(m_limits.operationTimeoutMs);
    return true;
}

bool Service::verifyTool()
{
    if (!begin(QStringLiteral("verify"))) return false;
    m_tool.version.clear();
    const quint64 generation = m_generation;
    const QPointer<Service> alive(this);
    emit busyChanged(true);
    if (!alive) return true;
    QTimer::singleShot(0, this, [this, generation] { if (m_busy && !m_ending && generation == m_generation) runCommand({QStringLiteral("version")}, Phase::Version); });
    return true;
}

bool Service::scanDevices()
{
    if (!begin(QStringLiteral("scan"))) return false;
    m_lastDevices.clear();
    const quint64 generation = m_generation;
    const QPointer<Service> alive(this);
    emit busyChanged(true);
    if (!alive) return true;
    QTimer::singleShot(0, this, [this, generation] { if (m_busy && !m_ending && generation == m_generation) runVersionOrDevices(); });
    return true;
}

bool Service::inspectDevice(const QString &serial)
{
    if (!begin(QStringLiteral("inspect"))) return false;
    bool found = false;
    if (validSerial(serial)) {
        for (const Device &device : m_lastDevices) {
            if (device.serial == serial && device.isAuthorized()) {
                m_selected = device;
                found = true;
                break;
            }
        }
    }
    const quint64 generation = m_generation;
    const QPointer<Service> alive(this);
    emit busyChanged(true);
    if (!alive) return true;
    QTimer::singleShot(0, this, [this, found, generation] {
        if (!m_busy || m_ending || generation != m_generation) return;
        if (!found) requestEnd(Status::InvalidRequest, QStringLiteral("Select an authorized device from a fresh scan before inspecting it."));
        else runVersionOrDevices();
    });
    return true;
}

void Service::runVersionOrDevices()
{
    if (m_tool.version.isEmpty()) runCommand({QStringLiteral("version")}, Phase::Version);
    else runCommand({QStringLiteral("devices"), QStringLiteral("-l")}, Phase::Devices);
}

bool Service::checkToolIdentity()
{
    Tool current;
    if (!fingerprint(m_tool.path, &current, nullptr) || current.path != m_tool.path || current.sha256 != m_tool.sha256) {
        m_tool.version.clear();
        m_lastDevices.clear();
        requestEnd(Status::ToolChanged, QStringLiteral("The selected ADB executable changed or became unavailable. Select and verify it again."));
        return false;
    }
    return true;
}

void Service::runCommand(const QStringList &arguments, Phase phase)
{
    if (!m_busy || m_ending || !checkToolIdentity()) return;
    const quint64 generation = m_generation;
    if (m_process.state() != QProcess::NotRunning) {
        requestEnd(Status::ProcessFailed, QStringLiteral("An earlier ADB command is still running."));
        return;
    }
    m_phase = phase;
    m_stdout.clear();
    m_outputBytes = 0;
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    // Do not inherit a device selector, remote server override, tracing switch,
    // or automatic wireless pairing preference from another development task.
    for (const QString &key : {QStringLiteral("ANDROID_SERIAL"), QStringLiteral("ADB_SERVER_SOCKET"),
                              QStringLiteral("ANDROID_ADB_SERVER_PORT"), QStringLiteral("ADB_SERVER_PORT"),
                              QStringLiteral("ADB_TRACE"), QStringLiteral("ADB_MDNS_AUTO_CONNECT")})
        environment.remove(key);
    environment.insert(QStringLiteral("ADB_MDNS_AUTO_CONNECT"), QStringLiteral("0"));
    m_process.setProcessEnvironment(environment);
    m_process.setWorkingDirectory(QFileInfo(m_tool.path).absolutePath());
    m_process.setProgram(m_tool.path);
    // The host and port are explicit: an inherited setting cannot redirect a
    // device inventory to a remote ADB server.
    m_process.setArguments(QStringList{QStringLiteral("-H"), QStringLiteral("127.0.0.1"),
                                       QStringLiteral("-P"), QStringLiteral("5037")} + arguments);
    const QPointer<Service> alive(this);
    emit progress(phase == Phase::Version ? QStringLiteral("Checking ADB version…")
                  : phase == Phase::Devices ? QStringLiteral("Checking connected device identities…")
                  : QStringLiteral("Reading device information…"));
    if (!alive || !m_busy || m_ending || generation != m_generation) return;
    m_commandTimer.start(m_limits.commandTimeoutMs);
    m_commandActive = true;
    m_process.start(QIODevice::ReadOnly);
}

void Service::collectOutput()
{
    if (!m_busy || m_ending) return;
    for (QProcess::ProcessChannel channel : {QProcess::StandardOutput, QProcess::StandardError}) {
        m_process.setReadChannel(channel);
        while (m_process.bytesAvailable() > 0) {
            // Never copy an unbounded process buffer into our own allocation.
            const qint64 allowance = m_limits.maxOutputBytes - m_outputBytes + 1;
            const QByteArray bytes = m_process.read(qMin<qint64>(qint64(4096), allowance));
            if (bytes.isEmpty()) break;
            m_outputBytes += bytes.size();
            if (m_outputBytes > m_limits.maxOutputBytes) {
                requestEnd(Status::OutputLimit, QStringLiteral("ADB produced more output than the diagnostics limit. The result was discarded."));
                return;
            }
            if (channel == QProcess::StandardOutput) m_stdout += bytes;
            // Stderr counts toward the same limit but is never retained/exported.
        }
    }
}

void Service::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (!m_busy) return;
    const quint64 generation = m_generation;
    const QPointer<Service> alive(this);
    m_commandActive = false;
    m_commandTimer.stop();
    collectOutput();
    if (!alive || !m_busy || generation != m_generation) return;
    if (m_ending) { finalize(); return; }
    if (exitStatus != QProcess::NormalExit) {
        requestEnd(Status::Crashed, QStringLiteral("ADB exited unexpectedly. The operation did not complete."));
        return;
    }
    if (exitCode != 0) {
        requestEnd(Status::ProcessFailed, QStringLiteral("ADB reported a failed command. Check the USB connection and device authorization, then scan again."));
        return;
    }
    if (!checkToolIdentity()) return;
    commandSucceeded();
}

void Service::commandSucceeded()
{
    if (m_phase == Phase::Version) {
        if (!parseVersion(m_stdout, &m_tool.version)) {
            requestEnd(Status::InvalidOutput, QStringLiteral("The selected executable did not return a recognized ADB version."));
            return;
        }
        if (m_operation == QStringLiteral("verify")) {
            requestEnd(Status::Success, QStringLiteral("ADB version checked and executable fingerprint recorded."));
        } else {
            runCommand({QStringLiteral("devices"), QStringLiteral("-l")}, Phase::Devices);
        }
    } else if (m_phase == Phase::Devices) {
        QList<Device> devices;
        if (!parseDevices(m_stdout, &devices)) {
            m_lastDevices.clear();
            requestEnd(Status::InvalidOutput, QStringLiteral("ADB returned an ambiguous or malformed device list. No device was selected."));
            return;
        }
        if (m_operation == QStringLiteral("scan")) {
            m_lastDevices = devices;
            m_resultDevices = devices;
            requestEnd(Status::Success, devices.isEmpty() ? QStringLiteral("No ADB devices are connected.")
                                                        : QStringLiteral("Device scan complete. Select a device to inspect."));
            return;
        }
        bool matched = false;
        for (const Device &device : devices) {
            if (sameDevice(m_selected, device)) { matched = true; break; }
        }
        if (!matched) {
            m_lastDevices.clear();
            requestEnd(Status::InvalidRequest, QStringLiteral("The selected device disconnected, changed identity, or lost authorization. Scan and select it again."));
            return;
        }
        if (m_propertyIndex == propertyCount) {
            m_report = makeReport();
            requestEnd(Status::Success, QStringLiteral("Read-only inspection complete. A report without the raw device serial is ready."));
        } else {
            runCommand({QStringLiteral("-s"), m_selected.serial, QStringLiteral("shell"), QStringLiteral("getprop"),
                        QString::fromLatin1(properties[m_propertyIndex].key)}, Phase::Property);
        }
    } else {
        QString value;
        if (!decode(m_stdout, &value)) {
            requestEnd(Status::InvalidOutput, QStringLiteral("ADB returned an invalid device property. No report was produced."));
            return;
        }
        value = value.trimmed();
        const Property &property = properties[m_propertyIndex];
        if (!value.isEmpty() && !QRegularExpression(QString::fromLatin1(property.pattern)).match(value).hasMatch()) {
            requestEnd(Status::InvalidOutput, QStringLiteral("A device property did not match the expected format. No report was produced."));
            return;
        }
        if (property.reportName == QByteArray("securityPatch") && !value.isEmpty()
            && !QDate::fromString(value, QStringLiteral("yyyy-MM-dd")).isValid()) {
            requestEnd(Status::InvalidOutput, QStringLiteral("The device reported an invalid security patch date."));
            return;
        }
        // Avoid propagating a device identifier even if a misconfigured device
        // repeats it in one of the otherwise allowlisted property values.
        if (!m_selected.serial.isEmpty()) value.replace(m_selected.serial, QStringLiteral("[redacted]"));
        m_properties.insert(QString::fromLatin1(property.reportName), value.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(value));
        ++m_propertyIndex;
        runCommand({QStringLiteral("devices"), QStringLiteral("-l")}, Phase::Devices);
    }
}

QJsonObject Service::makeReport() const
{
    const QByteArray digest = QCryptographicHash::hash(m_sessionSalt + m_selected.serial.toUtf8(), QCryptographicHash::Sha256).toHex();
    QString version = m_tool.version;
    version.replace(m_selected.serial, QStringLiteral("[redacted]"));
    QJsonObject toolInfo;
    toolInfo.insert(QStringLiteral("version"), version);
    toolInfo.insert(QStringLiteral("sha256"), m_tool.sha256);
    QJsonObject report;
    report.insert(QStringLiteral("schemaVersion"), 1);
    report.insert(QStringLiteral("application"), QStringLiteral("Legatto Device Studio"));
    report.insert(QStringLiteral("applicationVersion"), QStringLiteral("0.1.0-dev"));
    report.insert(QStringLiteral("generatedAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    report.insert(QStringLiteral("deviceId"), QStringLiteral("device-") + QString::fromLatin1(digest.left(24)));
    report.insert(QStringLiteral("transport"), QStringLiteral("adb"));
    report.insert(QStringLiteral("tool"), toolInfo);
    report.insert(QStringLiteral("device"), m_properties);
    return report;
}

void Service::cancel()
{
    if (m_busy) requestEnd(Status::Cancelled, QStringLiteral("Operation cancelled. No diagnostic report was produced."));
}

void Service::requestEnd(Status status, const QString &message)
{
    if (!m_busy || m_ending) return;
    m_ending = true;
    m_endStatus = status;
    m_endMessage = message;
    m_commandTimer.stop();
    m_operationTimer.stop();
    if (m_process.state() != QProcess::NotRunning) m_process.kill();
    else if (!m_commandActive) finalize();
}

void Service::finalize()
{
    if (!m_busy || m_commandActive || m_process.state() != QProcess::NotRunning) return;
    Result result;
    result.operation = m_operation;
    result.status = m_endStatus;
    result.message = m_endMessage;
    result.tool = m_tool;
    if (result.status == Status::Success) {
        result.devices = m_resultDevices;
        result.report = m_report;
    }
    m_stdout.clear();
    m_properties = QJsonObject();
    m_report = QJsonObject();
    m_busy = false;
    m_ending = false;
    const quint64 generation = m_generation;
    const QPointer<Service> alive(this);
    emit completed(result);
    // A completed slot may synchronously start the next job or destroy the
    // service. Do not send a stale idle notification after that next job starts.
    if (alive && !m_busy && generation == m_generation) emit busyChanged(false);
}

} // namespace Diagnostics
