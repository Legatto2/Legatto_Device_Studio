#ifndef LEGATTO_DIAGNOSTICSCORE_H
#define LEGATTO_DIAGNOSTICSCORE_H

#include <QByteArray>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QTimer>

namespace Diagnostics {

struct Device {
    QString serial;
    QString state;
    QString model;
    QString product;
    QString transportId;
    bool isAuthorized() const { return state == QStringLiteral("device"); }
};

struct Tool {
    QString path;
    QString sha256;
    QString version;
};

enum class Status {
    Success,
    InvalidRequest,
    FailedToStart,
    ProcessFailed,
    Crashed,
    TimedOut,
    Cancelled,
    OutputLimit,
    InvalidOutput,
    ToolChanged
};

struct Result {
    Status status = Status::InvalidRequest;
    // Stable operation names: "verify", "scan", "inspect".
    QString operation;
    // This is a local, generic message; it never contains raw process output.
    QString message;
    Tool tool;
    QList<Device> devices;
    QJsonObject report;
};

struct Limits {
    int commandTimeoutMs = 10000;
    int operationTimeoutMs = 45000;
    qint64 maxOutputBytes = 65536;
};

class Service : public QObject {
    Q_OBJECT
public:
    explicit Service(QObject *parent = nullptr);
    explicit Service(const Limits &limits, QObject *parent = nullptr);
    ~Service() override;

    // Fingerprinting is not a trust/signature assertion. This does not execute
    // the binary. The caller must select an executable from a trusted source.
    bool setAdbPath(const QString &path, QString *error = nullptr);
    Tool tool() const { return m_tool; }
    bool isBusy() const { return m_busy; }

    // An accepted request emits exactly one completed result. Busy/unconfigured
    // requests return false without changing the active operation.
    bool verifyTool();
    bool scanDevices();
    // Must be selected from the most recent scan. Identity is rechecked before
    // each property read and once more before constructing the report.
    bool inspectDevice(const QString &serial);

public slots:
    void cancel();

signals:
    void completed(const Diagnostics::Result &result);
    void busyChanged(bool busy);
    void progress(const QString &message);

private:
    enum class Phase { Version, Devices, Property };

    bool begin(const QString &operation);
    void runVersionOrDevices();
    void runCommand(const QStringList &arguments, Phase phase);
    void collectOutput();
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void commandSucceeded();
    void requestEnd(Status status, const QString &message);
    void finalize();
    bool checkToolIdentity();
    QJsonObject makeReport() const;

    Limits m_limits;
    QProcess m_process;
    QTimer m_commandTimer;
    QTimer m_operationTimer;
    Tool m_tool;
    QList<Device> m_lastDevices;
    QList<Device> m_resultDevices;
    Device m_selected;
    QByteArray m_sessionSalt;
    QByteArray m_stdout;
    qint64 m_outputBytes = 0;
    bool m_busy = false;
    bool m_ending = false;
    bool m_commandActive = false;
    quint64 m_generation = 0;
    Phase m_phase = Phase::Version;
    QString m_operation;
    Status m_endStatus = Status::InvalidRequest;
    QString m_endMessage;
    int m_propertyIndex = 0;
    QJsonObject m_properties;
    QJsonObject m_report;
};

} // namespace Diagnostics

Q_DECLARE_METATYPE(Diagnostics::Device)
Q_DECLARE_METATYPE(Diagnostics::Tool)
Q_DECLARE_METATYPE(Diagnostics::Status)
Q_DECLARE_METATYPE(Diagnostics::Result)

#endif
