#include "appsettings.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QRandomGenerator>
#include <QSettings>
#include <QStandardPaths>

namespace {

constexpr auto kKeyWorkMinutes = "break/workMinutes";
constexpr auto kKeyBreakSeconds = "break/breakSeconds";
constexpr auto kKeyPreNotifySeconds = "break/preNotifySeconds";
constexpr auto kKeyMessageHtml = "message/html";
constexpr auto kKeyPasswordHash = "security/passwordHash";
constexpr auto kKeyPasswordSalt = "security/passwordSalt";
constexpr auto kKeyAutoStart = "app/autoStart";

constexpr auto kDefaultPassword = "123456";
constexpr int kSaltBytes = 16;

//! 数值 clamp 到 [min, max]。
int clampInt(int value, int min, int max)
{
    return value < min ? min : (value > max ? max : value);
}

QByteArray hashPassword(const QString &password, const QByteArray &salt)
{
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(salt);
    hash.addData(password.toUtf8());
    return hash.result();
}

} // namespace

AppSettings::AppSettings(QObject *parent)
    : QObject(parent)
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    m_settings = new QSettings(dir + QStringLiteral("/config.ini"), QSettings::IniFormat, this);
    load();
}

AppSettings::~AppSettings() = default;

QString AppSettings::defaultMessageHtml()
{
    // 标题行大字号加粗，副标题行小字号；配色针对纯黑遮罩背景
    return QStringLiteral(
        "<html><body style=\"text-align:center;\">"
        "<p align=\"center\"><span style=\"font-size:48pt; font-weight:700; color:#ffffff;\">"
        "该休息了</span></p>"
        "<p align=\"center\"><span style=\"font-size:18pt; color:#bbbbbb;\">"
        "起身活动一下眼睛</span></p>"
        "</body></html>");
}

void AppSettings::load()
{
    // 逐字段回落默认值，不因单个字段损坏而整体重置配置
    m_workMinutes = clampInt(m_settings->value(kKeyWorkMinutes, kWorkMinutesDefault).toInt(),
                             kWorkMinutesMin, kWorkMinutesMax);
    m_breakSeconds = clampInt(m_settings->value(kKeyBreakSeconds, kBreakSecondsDefault).toInt(),
                              kBreakSecondsMin, kBreakSecondsMax);
    m_preNotifySeconds = clampInt(m_settings->value(kKeyPreNotifySeconds, kPreNotifySecondsDefault).toInt(),
                                  kPreNotifySecondsMin, kPreNotifySecondsMax);

    m_messageHtml = m_settings->value(kKeyMessageHtml).toString();
    if (m_messageHtml.trimmed().isEmpty())
        m_messageHtml = defaultMessageHtml();

    m_autoStart = m_settings->value(kKeyAutoStart, false).toBool();

    m_passwordSalt = QByteArray::fromHex(m_settings->value(kKeyPasswordSalt).toByteArray());
    m_passwordHash = QByteArray::fromHex(m_settings->value(kKeyPasswordHash).toByteArray());
    if (m_passwordSalt.size() != kSaltBytes || m_passwordHash.size() != QCryptographicHash::hashLength(QCryptographicHash::Sha256))
        storePassword(QString::fromLatin1(kDefaultPassword));

    // 首次启动（或字段缺失）时把生效值补齐写回，保证配置文件内容完整可排查
    m_settings->setValue(kKeyWorkMinutes, m_workMinutes);
    m_settings->setValue(kKeyBreakSeconds, m_breakSeconds);
    m_settings->setValue(kKeyPreNotifySeconds, m_preNotifySeconds);
    m_settings->setValue(kKeyMessageHtml, m_messageHtml);
    m_settings->setValue(kKeyAutoStart, m_autoStart);
    m_settings->sync();

    applyAutoStart(m_autoStart);
}

void AppSettings::setWorkMinutes(int minutes)
{
    const int value = clampInt(minutes, kWorkMinutesMin, kWorkMinutesMax);
    if (value == m_workMinutes)
        return;
    m_workMinutes = value;
    m_settings->setValue(kKeyWorkMinutes, value);
    m_settings->sync();
    emit workMinutesChanged();
}

void AppSettings::setBreakSeconds(int seconds)
{
    const int value = clampInt(seconds, kBreakSecondsMin, kBreakSecondsMax);
    if (value == m_breakSeconds)
        return;
    m_breakSeconds = value;
    m_settings->setValue(kKeyBreakSeconds, value);
    m_settings->sync();
    emit breakSecondsChanged();
}

void AppSettings::setPreNotifySeconds(int seconds)
{
    const int value = clampInt(seconds, kPreNotifySecondsMin, kPreNotifySecondsMax);
    if (value == m_preNotifySeconds)
        return;
    m_preNotifySeconds = value;
    m_settings->setValue(kKeyPreNotifySeconds, value);
    m_settings->sync();
    emit preNotifySecondsChanged();
}

void AppSettings::setMessageHtml(const QString &html)
{
    // 空文案会导致纯黑无内容的遮罩，容易被误认为系统死机，因此回落到默认文案
    const QString value = html.trimmed().isEmpty() ? defaultMessageHtml() : html;
    if (value == m_messageHtml)
        return;
    m_messageHtml = value;
    m_settings->setValue(kKeyMessageHtml, value);
    m_settings->sync();
    emit messageHtmlChanged();
}

void AppSettings::setAutoStart(bool enabled)
{
    if (enabled == m_autoStart)
        return;
    m_autoStart = enabled;
    m_settings->setValue(kKeyAutoStart, enabled);
    m_settings->sync();
    applyAutoStart(enabled);
    emit autoStartChanged();
}

bool AppSettings::verifyPassword(const QString &password) const
{
    return hashPassword(password, m_passwordSalt) == m_passwordHash;
}

bool AppSettings::changePassword(const QString &oldPassword, const QString &newPassword)
{
    if (!verifyPassword(oldPassword) || newPassword.isEmpty())
        return false;
    storePassword(newPassword);
    m_settings->sync();
    return true;
}

void AppSettings::storePassword(const QString &password)
{
    m_passwordSalt.resize(kSaltBytes);
    QRandomGenerator *rng = QRandomGenerator::system();
    for (int i = 0; i < kSaltBytes; ++i)
        m_passwordSalt[i] = static_cast<char>(rng->bounded(256));
    m_passwordHash = hashPassword(password, m_passwordSalt);
    m_settings->setValue(kKeyPasswordSalt, QString::fromLatin1(m_passwordSalt.toHex()));
    m_settings->setValue(kKeyPasswordHash, QString::fromLatin1(m_passwordHash.toHex()));
}

void AppSettings::applyAutoStart(bool enabled)
{
#ifdef Q_OS_WIN
    QSettings runKey(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                     QSettings::NativeFormat);
    const QString name = QStringLiteral("ForcedBreak");
    if (enabled) {
        const QString exePath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        runKey.setValue(name, QStringLiteral("\"%1\"").arg(exePath));
    } else {
        runKey.remove(name);
    }
#else
    Q_UNUSED(enabled);
#endif
}
