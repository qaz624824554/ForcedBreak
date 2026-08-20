#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

class QSettings;

/*!
 * 配置读写（QSettings INI 封装）。
 *
 * 配置文件位于 QStandardPaths::AppConfigLocation/config.ini。
 * 所有 setter 立即落盘；读取时对缺失字段回落默认值、对越界数值 clamp。
 * 密码以 SHA-256 + 16 字节随机盐的形式存储，绝不存明文。
 */
class AppSettings : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(int workMinutes READ workMinutes WRITE setWorkMinutes NOTIFY workMinutesChanged)
    Q_PROPERTY(int breakSeconds READ breakSeconds WRITE setBreakSeconds NOTIFY breakSecondsChanged)
    Q_PROPERTY(int preNotifySeconds READ preNotifySeconds WRITE setPreNotifySeconds NOTIFY preNotifySecondsChanged)
    Q_PROPERTY(QString messageHtml READ messageHtml WRITE setMessageHtml NOTIFY messageHtmlChanged)
    Q_PROPERTY(bool autoStart READ autoStart WRITE setAutoStart NOTIFY autoStartChanged)

public:
    // 合法区间与默认值（见设计文档第 8 节）
    static constexpr int kWorkMinutesMin = 1;
    static constexpr int kWorkMinutesMax = 480;
    static constexpr int kWorkMinutesDefault = 45;
    static constexpr int kBreakSecondsMin = 5;
    static constexpr int kBreakSecondsMax = 3600;
    static constexpr int kBreakSecondsDefault = 300;
    // 提前提醒量为 0 表示关闭提醒
    static constexpr int kPreNotifySecondsMin = 0;
    static constexpr int kPreNotifySecondsMax = 3600;
    static constexpr int kPreNotifySecondsDefault = 60;

    explicit AppSettings(QObject *parent = nullptr);
    ~AppSettings() override;

    int workMinutes() const { return m_workMinutes; }
    void setWorkMinutes(int minutes);

    int breakSeconds() const { return m_breakSeconds; }
    void setBreakSeconds(int seconds);

    int preNotifySeconds() const { return m_preNotifySeconds; }
    void setPreNotifySeconds(int seconds);

    QString messageHtml() const { return m_messageHtml; }
    void setMessageHtml(const QString &html);

    bool autoStart() const { return m_autoStart; }
    void setAutoStart(bool enabled);

    //! 校验解锁密码是否正确。
    Q_INVOKABLE bool verifyPassword(const QString &password) const;

    //! 修改解锁密码；旧密码错误或新密码为空时返回 false 且不做任何改动。
    Q_INVOKABLE bool changePassword(const QString &oldPassword, const QString &newPassword);

    //! 默认提醒文案（messageHtml 为空或解析失败时的回落内容）。
    static QString defaultMessageHtml();

    //! defaultMessageHtml() 的 QML 可调用版本（QML 无法调用静态方法）。
    Q_INVOKABLE QString defaultMessage() const { return defaultMessageHtml(); }

signals:
    void workMinutesChanged();
    void breakSecondsChanged();
    void preNotifySecondsChanged();
    void messageHtmlChanged();
    void autoStartChanged();

private:
    void load();
    void storePassword(const QString &password);
    //! 同步开机自启动到注册表 HKCU\...\Run（非 Windows 平台为空操作）。
    void applyAutoStart(bool enabled);

    QSettings *m_settings = nullptr;
    int m_workMinutes = kWorkMinutesDefault;
    int m_breakSeconds = kBreakSecondsDefault;
    int m_preNotifySeconds = kPreNotifySecondsDefault;
    QString m_messageHtml;
    bool m_autoStart = false;
    QByteArray m_passwordSalt;  // 原始字节
    QByteArray m_passwordHash;  // 原始字节
};
