#include "securityhelpers.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>
#include <QUrl>
#include <cmath>
#include <limits>

namespace {
bool fail(QString *error, const QString &message)
{
    if (error) *error = message;
    return false;
}
}

namespace Security {

QByteArray encodeLoginForm(const QString &systemId, const QString &username,
                          const QString &password)
{
    // Encode each value, not the complete form. This also encodes literal '+'.
    return QByteArray("sysID=") + QUrl::toPercentEncoding(systemId)
        + "&UserName=" + QUrl::toPercentEncoding(username)
        + "&Password=" + QUrl::toPercentEncoding(password);
}

bool saveLoginPreferences(const QString &path, const LoginPreferences &preferences,
                          QString *error)
{
    if (error) error->clear();
    if (path.isEmpty() || QFileInfo(path).isSymLink())
        return fail(error, QStringLiteral("Invalid or linked settings file."));

    QJsonObject object;
    object.insert(QStringLiteral("schemaVersion"), 2);
    object.insert(QStringLiteral("rememberUsername"), preferences.rememberUsername);
    if (preferences.rememberUsername)
        object.insert(QStringLiteral("username"), preferences.username);
    const QByteArray contents = QJsonDocument(object).toJson(QJsonDocument::Compact);

    QSaveFile file(path);
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly))
        return fail(error, QStringLiteral("Cannot open settings for a safe replacement."));
    if (file.write(contents) != contents.size()) {
        file.cancelWriting();
        return fail(error, QStringLiteral("Cannot write settings."));
    }
    if (!file.commit())
        return fail(error, QStringLiteral("Cannot commit settings safely."));
    return true;
}

bool loadLoginPreferences(const QString &path, LoginPreferences *preferences,
                          QString *error)
{
    if (error) error->clear();
    if (!preferences)
        return fail(error, QStringLiteral("Missing preferences destination."));
    *preferences = LoginPreferences();
    if (path.isEmpty() || QFileInfo(path).isSymLink())
        return fail(error, QStringLiteral("Invalid or linked settings file."));
    if (!QFile::exists(path)) return true;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() > 65536)
        return fail(error, QStringLiteral("Cannot read settings safely."));
    const QByteArray contents = file.read(65537);
    if (file.error() != QFileDevice::NoError || contents.size() > 65536)
        return fail(error, QStringLiteral("Cannot read settings safely."));
    file.close();

    LoginPreferences parsed;
    if (contents.trimmed().startsWith('{')) {
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(contents, &parseError);
        const QJsonObject object = document.object();
        if (parseError.error != QJsonParseError::NoError || !document.isObject()
            || object.value("schemaVersion") != QJsonValue(2)
            || !object.value("rememberUsername").isBool()
            || (object.value("rememberUsername").toBool()
                && !object.value("username").isString()))
            return fail(error, QStringLiteral("Invalid settings format; remove the old settings file before login."));
        parsed.rememberUsername = object.value("rememberUsername").toBool();
        if (parsed.rememberUsername) parsed.username = object.value("username").toString();
    } else {
        // Parse only non-secret legacy fields, independent of their order.
        // Never restore or copy a legacy password, including when rememberMe=0.
        const QList<QByteArray> lines = contents.split('\n');
        for (QByteArray line : lines) {
            if (line.endsWith('\r')) line.chop(1);
            const int separator = line.indexOf('=');
            if (separator < 0) continue;
            const QByteArray key = line.left(separator);
            if (key == "rememberMe") parsed.rememberUsername = line.mid(separator + 1) == "1";
            else if (key == "userName") parsed.username = QString::fromUtf8(line.mid(separator + 1));
        }
        if (!parsed.rememberUsername) parsed.username.clear();
    }

    // Whitelist serialization also strips unknown fields from existing JSON.
    // Do not expose remembered data until the replacement has succeeded.
    if (!saveLoginPreferences(path, parsed, error)) return false;
    *preferences = parsed;
    return true;
}

bool parseChargeReply(const QByteArray &json, ChargeResult *result)
{
    if (!result) return false;
    *result = ChargeResult();
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(json, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) return false;
    const QJsonObject object = document.object();
    bool hasErrorFlag = false;
    for (const QString &key : {QStringLiteral("isError"), QStringLiteral("IsError")}) {
        if (!object.contains(key)) continue;
        hasErrorFlag = true;
        if (!object.value(key).isBool() || object.value(key).toBool()) return false;
    }
    if (!hasErrorFlag || !object.value("Model").isObject()) return false;
    const QJsonObject model = object.value("Model").toObject();
    if (!model.value("OperationId").isDouble() || !model.value("Credit").isDouble()) return false;
    const double id = model.value("OperationId").toDouble();
    const double credit = model.value("Credit").toDouble();
    if (!std::isfinite(id) || id < 1 || id > 16777216 || std::floor(id) != id
        || !std::isfinite(credit) || credit < 0 || credit > std::numeric_limits<float>::max())
        return false;
    result->operationId = static_cast<float>(id);
    result->credit = static_cast<float>(credit);
    return true;
}

} // namespace Security
