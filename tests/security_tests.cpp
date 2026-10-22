#include <QtTest>
#include <QFile>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include "securityhelpers.h"
#include "portselection.h"

class SecurityTests : public QObject
{
    Q_OBJECT
private:
    static QByteArray read(const QString &path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) return {};
        return file.readAll();
    }
    static bool write(const QString &path, const QByteArray &bytes) {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
    }
private slots:
    void formEncoding_data() {
        QTest::addColumn<QString>("password");
        QTest::newRow("plain") << QString("Test123");
        QTest::newRow("plus") << QString("Test+123");
        QTest::newRow("ampersand") << QString("Test&123");
        QTest::newRow("reserved") << QString("%=#?+& /\r\n");
        QTest::newRow("unicode") << QString::fromUtf8("caf\303\251 \360\237\224\222");
        QTest::newRow("empty") << QString();
    }
    void formEncoding() {
        QFETCH(QString, password);
        const auto encoded = Security::encodeLoginForm("sys+&=", "u+test@example.invalid", password);
        const auto fields = encoded.split('&');
        QCOMPARE(fields.size(), 3);
        const QList<QString> expected = {"sys+&=", "u+test@example.invalid", password};
        for (int i=0; i<fields.size(); ++i) {
            QByteArray value = fields.at(i).mid(fields.at(i).indexOf('=') + 1);
            value.replace('+', ' '); // application/x-www-form-urlencoded decoding
            QCOMPARE(QUrl::fromPercentEncoding(value), expected.at(i));
        }
    }
    void migrateLegacy_data() {
        QTest::addColumn<bool>("remember");
        QTest::newRow("remember-on") << true;
        QTest::newRow("remember-off") << false;
    }
    void migrateLegacy() {
        QFETCH(bool, remember);
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath("legacySettings");
        // Out-of-order keys and CRLF must not alter the migration policy.
        const QByteArray legacy = QByteArray("userName=test=alias@example.invalid\r\npass=DUMMY_SECRET_ONLY\r\nrememberMe=")
            + (remember ? "1" : "0") + "\r\n";
        QVERIFY(write(path, legacy));
        Security::LoginPreferences preferences;
        QString error;
        QVERIFY2(Security::loadLoginPreferences(path, &preferences, &error), qPrintable(error));
        QCOMPARE(preferences.rememberUsername, remember);
        QCOMPARE(preferences.username, remember ? QString("test=alias@example.invalid") : QString());
        const QByteArray saved = read(path);
        QVERIFY(!saved.contains("DUMMY_SECRET_ONLY"));
        QVERIFY(!saved.contains("pass"));
        const auto object = QJsonDocument::fromJson(saved).object();
        QCOMPARE(object.value("schemaVersion").toInt(), 2);
        QCOMPARE(object.contains("username"), remember);
    }
    void uncheckingForgetsUsername() {
        QTemporaryDir dir;
        const QString path = dir.filePath("settings");
        Security::LoginPreferences preferences;
        preferences.rememberUsername = true;
        preferences.username = "remembered@example.invalid";
        QVERIFY(Security::saveLoginPreferences(path, preferences));
        preferences.rememberUsername = false;
        QVERIFY(Security::saveLoginPreferences(path, preferences));
        QVERIFY(!read(path).contains("remembered@example.invalid"));
        QVERIFY(!QJsonDocument::fromJson(read(path)).object().contains("username"));
    }
    void unexpectedJsonSecretsAreDiscarded() {
        QTemporaryDir dir;
        const QString path = dir.filePath("settings");
        QVERIFY(write(path, R"({"schemaVersion":2,"rememberUsername":true,"username":"user","password":"DUMMY_SECRET_ONLY","token":"DUMMY_TOKEN_ONLY"})"));
        Security::LoginPreferences preferences;
        QVERIFY(Security::loadLoginPreferences(path, &preferences));
        const auto contents = read(path);
        QVERIFY(!contents.contains("DUMMY_SECRET_ONLY"));
        QVERIFY(!contents.contains("DUMMY_TOKEN_ONLY"));
        QCOMPARE(QJsonDocument::fromJson(contents).object().size(), 3);
    }
    void malformedJsonFailsClosed() {
        QTemporaryDir dir;
        const QString path = dir.filePath("settings");
        QVERIFY(write(path, "{broken json"));
        Security::LoginPreferences preferences;
        preferences.rememberUsername = true;
        preferences.username = "stale";
        QString error;
        QVERIFY(!Security::loadLoginPreferences(path, &preferences, &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(!preferences.rememberUsername);
        QVERIFY(preferences.username.isEmpty());
        QCOMPARE(read(path), QByteArray("{broken json"));
    }
    void missingFileUsesSafeDefaults() {
        QTemporaryDir dir;
        Security::LoginPreferences preferences;
        preferences.rememberUsername = true;
        preferences.username = "stale";
        QVERIFY(Security::loadLoginPreferences(dir.filePath("missing"), &preferences));
        QVERIFY(!preferences.rememberUsername);
        QVERIFY(preferences.username.isEmpty());
    }
    void writeFailureIsReported() {
        QTemporaryDir dir;
        Security::LoginPreferences preferences;
        QString error;
        QVERIFY(!Security::saveLoginPreferences(dir.path(), preferences, &error));
        QVERIFY(!error.isEmpty());
    }
    void linkedSettingsAreRejected() {
#ifdef Q_OS_WIN
        QSKIP("Windows shortcut/ACL behavior requires a native Windows test.");
#else
        QTemporaryDir dir;
        const QString target = dir.filePath("target");
        const QString link = dir.filePath("linked");
        QVERIFY(write(target, "untouched"));
        QVERIFY(QFile::link(target, link));
        Security::LoginPreferences preferences;
        QVERIFY(!Security::loadLoginPreferences(link, &preferences));
        QVERIFY(!Security::saveLoginPreferences(link, preferences));
        QCOMPARE(read(target), QByteArray("untouched"));
#endif
    }
    void chargeSchema_data() {
        QTest::addColumn<QByteArray>("json");
        QTest::addColumn<bool>("valid");
        const QByteArray model = R"("Model":{"OperationId":123,"Credit":5.5})";
        QTest::newRow("lowercase-success") << QByteArray("{\"isError\":false," + model + "}") << true;
        QTest::newRow("uppercase-success") << QByteArray("{\"IsError\":false," + model + "}") << true;
        QTest::newRow("both-false") << QByteArray("{\"isError\":false,\"IsError\":false," + model + "}") << true;
        QTest::newRow("lowercase-error") << QByteArray("{\"isError\":true," + model + "}") << false;
        QTest::newRow("uppercase-error") << QByteArray("{\"IsError\":true," + model + "}") << false;
        QTest::newRow("contradictory") << QByteArray("{\"isError\":false,\"IsError\":true," + model + "}") << false;
        QTest::newRow("wrong-flag-type") << QByteArray("{\"isError\":\"false\"," + model + "}") << false;
        QTest::newRow("missing-flag") << QByteArray("{" + model + "}") << false;
        QTest::newRow("missing-model") << QByteArray(R"({"isError":false})") << false;
        QTest::newRow("empty") << QByteArray() << false;
        QTest::newRow("malformed") << QByteArray("<html>error</html>") << false;
        QTest::newRow("large-id") << QByteArray(R"({"isError":false,"Model":{"OperationId":16777217,"Credit":5}})") << false;
        QTest::newRow("fractional-id") << QByteArray(R"({"isError":false,"Model":{"OperationId":1.5,"Credit":5}})") << false;
        QTest::newRow("zero-id") << QByteArray(R"({"isError":false,"Model":{"OperationId":0,"Credit":5}})") << false;
        QTest::newRow("negative-credit") << QByteArray(R"({"isError":false,"Model":{"OperationId":123,"Credit":-1}})") << false;
        QTest::newRow("missing-credit") << QByteArray(R"({"isError":false,"Model":{"OperationId":123}})") << false;
        QTest::newRow("string-id") << QByteArray(R"({"isError":false,"Model":{"OperationId":"123","Credit":5}})") << false;
    }
    void chargeSchema() {
        QFETCH(QByteArray, json);
        QFETCH(bool, valid);
        Security::ChargeResult result;
        result.operationId = 999;
        result.credit = 999;
        QCOMPARE(Security::parseChargeReply(json, &result), valid);
        QCOMPARE(result.operationId, valid ? 123.0f : 0.0f);
        QCOMPARE(result.credit, valid ? 5.5f : 0.0f);
    }
    void serialSelectionPreservesIdentityOnReorder() {
        QComboBox combo;
        refreshPortSelection(&combo, {"COM3", "COM8"}, {"first", "second"});
        QCOMPARE(combo.currentIndex(), -1);
        combo.setCurrentIndex(1);
        refreshPortSelection(&combo, {"COM8", "COM3"}, {"second", "first"});
        QCOMPARE(combo.currentData().toString(), QString("COM8"));
        QCOMPARE(combo.currentIndex(), 0);
    }
    void serialUnplugRemovesStaleSelection() {
        QComboBox combo;
        refreshPortSelection(&combo, {"COM3", "COM8"}, {"first", "second"});
        combo.setCurrentIndex(1);
        refreshPortSelection(&combo, {"COM3"}, {"first"});
        QCOMPARE(combo.count(), 1);
        QCOMPARE(combo.currentIndex(), -1);
        refreshPortSelection(&combo, {}, {});
        QCOMPARE(combo.count(), 0);
        QCOMPARE(combo.currentIndex(), -1);
    }
    void invalidPortSnapshotClearsChoices() {
        QComboBox combo;
        refreshPortSelection(&combo, {"COM3"}, {"first"});
        refreshPortSelection(&combo, {"COM8"}, {});
        QCOMPARE(combo.count(), 0);
    }
};
QTEST_MAIN(SecurityTests)
#include "security_tests.moc"
