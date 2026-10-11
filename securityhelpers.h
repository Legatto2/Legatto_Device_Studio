#ifndef SECURITYHELPERS_H
#define SECURITYHELPERS_H

#include <QByteArray>
#include <QString>

namespace Security {

// Intentionally has no password or token field. Remember only a username.
struct LoginPreferences {
    bool rememberUsername = false;
    QString username;
};

QByteArray encodeLoginForm(const QString &systemId, const QString &username,
                          const QString &password);
bool saveLoginPreferences(const QString &path, const LoginPreferences &preferences,
                          QString *error = nullptr);
// Migrates the legacy file in place, discarding passwords before returning success.
// This removes active file contents; it is not secure erasure of disk/backups.
bool loadLoginPreferences(const QString &path, LoginPreferences *preferences,
                          QString *error = nullptr);

// Transitional boundary until operation/account globals are replaced.
// Reject IDs that cannot be represented exactly by the legacy float field.
struct ChargeResult {
    float operationId = 0;
    float credit = 0;
};
bool parseChargeReply(const QByteArray &json, ChargeResult *result);

} // namespace Security
#endif
