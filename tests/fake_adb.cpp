// A native, hardware-free test double. It never starts ADB or accesses a phone.
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <cstdio>
#include <cstdlib>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {
void out(const QByteArray &bytes) { std::fwrite(bytes.constData(), 1, size_t(bytes.size()), stdout); std::fflush(stdout); }
void err(const QByteArray &bytes) { std::fwrite(bytes.constData(), 1, size_t(bytes.size()), stderr); std::fflush(stderr); }
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
#ifdef Q_OS_WIN
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif
    QStringList arguments = app.arguments().mid(1);
    const QString tracePath = qEnvironmentVariable("LEGATTO_FAKE_TRACE");
    if (!tracePath.isEmpty()) {
        QFile trace(tracePath);
        if (!trace.open(QIODevice::Append)) return 90;
        QJsonObject record;
        record.insert(QStringLiteral("arguments"), QJsonArray::fromStringList(arguments));
        record.insert(QStringLiteral("ANDROID_SERIAL"), qEnvironmentVariable("ANDROID_SERIAL"));
        record.insert(QStringLiteral("ADB_SERVER_SOCKET"), qEnvironmentVariable("ADB_SERVER_SOCKET"));
        record.insert(QStringLiteral("ADB_TRACE"), qEnvironmentVariable("ADB_TRACE"));
        trace.write(QJsonDocument(record).toJson(QJsonDocument::Compact) + '\n');
    }
    const QStringList host{QStringLiteral("-H"), QStringLiteral("127.0.0.1"), QStringLiteral("-P"), QStringLiteral("5037")};
    if (arguments.mid(0, 4) != host) return 91;
    arguments = arguments.mid(4);
    const QString mode = qEnvironmentVariable("LEGATTO_FAKE_MODE", QStringLiteral("normal"));
    if (mode == QStringLiteral("timeout")) { QThread::msleep(30000); return 0; }
    if (mode == QStringLiteral("slow")) QThread::msleep(200);
    if (mode == QStringLiteral("fail")) { err("DUMMY_TOKEN_DO_NOT_EXPORT C:\\private\\secret\n"); return 23; }
    if (mode == QStringLiteral("crash")) {
#ifdef Q_OS_WIN
        // Qt recognizes this native failure status as CrashExit. Terminating
        // ourselves avoids CRT/WER dialogs and does not dereference bad memory.
        TerminateProcess(GetCurrentProcess(), 0xC0000005u);
        return 99;
#else
        std::abort();
#endif
    }
    if (mode == QStringLiteral("flood")) { out(QByteArray(128 * 1024, 'x')); return 0; }
    if (mode == QStringLiteral("stderr_flood")) { err(QByteArray(128 * 1024, 'x')); return 0; }
    if (mode == QStringLiteral("stderr")) err("DUMMY_TOKEN_DO_NOT_EXPORT C:\\private\\secret\n");
    if (arguments == QStringList{QStringLiteral("version")}) {
        if (mode == QStringLiteral("invalid_version")) out("This is not an ADB version.\n");
        else out("Android Debug Bridge version 1.0.41\nVersion 36.0.0-test\nInstalled as C:\\private\\adb.exe\n");
        return 0;
    }
    if (arguments == QStringList{QStringLiteral("devices"), QStringLiteral("-l")}) {
        if (mode == QStringLiteral("malformed")) { out("unexpected daemon text\nFAKE-DEVICE-A device\n"); return 0; }
        if (mode == QStringLiteral("unknown_attribute")) { out("List of devices attached\nFAKE-DEVICE-A device extra:unknown\n"); return 0; }
        if (mode == QStringLiteral("partial")) {
            out("* daemon not running; starting now at tcp:5037\n* daemon started successfully\nList of devi");
            QThread::msleep(20);
            out("ces attached\nFAKE-DEV");
            QThread::msleep(20);
            out("ICE-A\tdevice product:test_a model:Moto_Test device:test transport_id:1\n");
            return 0;
        }
        out("List of devices attached\n");
        if (mode == QStringLiteral("empty")) return 0;
        if (mode == QStringLiteral("unauthorized")) out("FAKE-DEVICE-A\tunauthorized transport_id:1\n");
        else if (mode == QStringLiteral("offline")) out("FAKE-DEVICE-A\toffline transport_id:1\n");
        else if (mode == QStringLiteral("no_permissions")) out("FAKE-DEVICE-A\tno permissions (missing udev rules)\n");
        else if (mode != QStringLiteral("disconnected")) {
            out("FAKE-DEVICE-A\tdevice product:test_a model:Moto_Test device:test transport_id:");
            out(mode == QStringLiteral("transport_changed") ? "9\n" : "1\n");
        }
        if (mode == QStringLiteral("duplicate")) out("FAKE-DEVICE-A\tdevice product:test_a model:Moto_Test device:test transport_id:2\n");
        else out("FAKE-DEVICE-B\tdevice product:test_b model:Other_Test device:other transport_id:2\n");
        return 0;
    }
    if (arguments.size() == 5 && arguments.at(0) == QStringLiteral("-s")
        && (arguments.at(1) == QStringLiteral("FAKE-DEVICE-A") || arguments.at(1) == QStringLiteral("FAKE-DEVICE-B"))
        && arguments.at(2) == QStringLiteral("shell") && arguments.at(3) == QStringLiteral("getprop")) {
        if (mode == QStringLiteral("inspect_timeout")) { QThread::msleep(30000); return 0; }
        if (mode == QStringLiteral("bad_property")) { out("C:\\private\\secret\n"); return 0; }
        if (mode == QStringLiteral("missing_properties")) { out("\n"); return 0; }
        const QString key = arguments.at(4);
        if (key == QStringLiteral("ro.product.manufacturer"))
            out(mode == QStringLiteral("leaky_property") ? "FAKE-DEVICE-A\n" : "Motorola\n");
        else if (key == QStringLiteral("ro.product.model")) out("Moto Test\n");
        else if (key == QStringLiteral("ro.build.version.release")) out("14\n");
        else if (key == QStringLiteral("ro.build.display.id")) out("UT_TEST_2026\n");
        else if (key == QStringLiteral("ro.build.version.security_patch"))
            out(mode == QStringLiteral("invalid_date") ? "2026-99-99\n" : "2026-09-01\n");
        else return 92;
        return 0;
    }
    err("Test fixture rejected a command outside the diagnostics allowlist.\n");
    return 93;
}
