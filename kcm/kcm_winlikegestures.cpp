/*
    SPDX-FileCopyrightText: 2026 LiberaTeMetuMortis <78408599+LiberaTeMetuMortis@users.noreply.github.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kcm_winlikegestures.h"

#include "gestureconfig.h"

#include <KConfigGroup>
#include <KPluginFactory>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>

K_PLUGIN_CLASS_WITH_JSON(KCMWinLikeGestures, "kcm_winlikegestures.json")

namespace
{
const QString s_configName = QStringLiteral("kwinwinlikegesturesrc");
const QString s_group = QStringLiteral("Gestures");
const QString s_pluginId = QStringLiteral("winlikegestures");
const QString s_enabledKey = s_pluginId + QStringLiteral("Enabled");

// Mirrors the fallbacks in GestureConfig::load().
const QMap<QString, QString> s_tuningDefaults = {
    {QStringLiteral("Threshold"), QStringLiteral("30")},
    {QStringLiteral("PinchThreshold"), QStringLiteral("0.2")},
    {QStringLiteral("SwitcherStep"), QStringLiteral("100")},
    {QStringLiteral("SwitcherStepVertical"), QStringLiteral("60")},
};

KConfigGroup kwinPlugins()
{
    return KSharedConfig::openConfig(QStringLiteral("kwinrc"))->group(QStringLiteral("Plugins"));
}

// Load or unload in the running KWin. Async: never block System Settings on KWin.
void setPluginLoaded(bool loaded)
{
    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KWin"),
                                                          QStringLiteral("/Plugins"),
                                                          QStringLiteral("org.kde.KWin.Plugins"),
                                                          loaded ? QStringLiteral("LoadPlugin") : QStringLiteral("UnloadPlugin"));
    message << s_pluginId;
    QDBusConnection::sessionBus().asyncCall(message);
}
} // namespace

KCMWinLikeGestures::KCMWinLikeGestures(QObject *parent, const KPluginMetaData &metaData)
    : KQuickConfigModule(parent, metaData)
    , m_config(KSharedConfig::openConfig(s_configName, KConfig::SimpleConfig))
    , m_defaults(WinLike::GestureConfig::defaults())
{
    m_defaults.insert(s_tuningDefaults);
    setButtons(Apply | Default);
}

QString KCMWinLikeGestures::defaultValue(const QString &key) const
{
    if (const auto it = m_defaults.constFind(key); it != m_defaults.cend()) {
        return *it;
    }
    return key.contains(QLatin1Char('.')) ? QStringLiteral("inherit") : QStringLiteral("passthrough");
}

QString KCMWinLikeGestures::value(const QString &key) const
{
    return m_overrides.value(key, defaultValue(key));
}

void KCMWinLikeGestures::setValue(const QString &key, const QString &value)
{
    // Keep only real overrides so the file stays minimal and future default
    // changes still reach keys the user never touched.
    if (value == defaultValue(key)) {
        m_overrides.remove(key);
    } else {
        m_overrides.insert(key, value);
    }
    changed();
}

void KCMWinLikeGestures::resetValue(const QString &key)
{
    m_overrides.remove(key);
    changed();
}

void KCMWinLikeGestures::setPluginEnabled(bool enabled)
{
    if (m_pluginEnabled == enabled) {
        return;
    }
    m_pluginEnabled = enabled;
    Q_EMIT pluginEnabledChanged();
    changed();
}

QStringList KCMWinLikeGestures::kwinShortcuts() const
{
    // Runs in System Settings, not inside KWin, so a blocking call is fine here.
    const QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kglobalaccel"),
                                                                QStringLiteral("/component/kwin"),
                                                                QStringLiteral("org.kde.kglobalaccel.Component"),
                                                                QStringLiteral("shortcutNames"));
    const QDBusReply<QStringList> reply = QDBusConnection::sessionBus().call(message);
    QStringList names = reply.isValid() ? reply.value() : QStringList();
    names.sort(Qt::CaseInsensitive);
    return names;
}

void KCMWinLikeGestures::load()
{
    m_config->reparseConfiguration();
    m_overrides.clear();
    const QMap<QString, QString> entries = m_config->group(s_group).entryMap();
    for (auto it = entries.cbegin(); it != entries.cend(); ++it) {
        if (it.value() != defaultValue(it.key())) {
            m_overrides.insert(it.key(), it.value());
        }
    }
    m_savedOverrides = m_overrides;

    kwinPlugins().config()->reparseConfiguration();
    m_pluginEnabled = kwinPlugins().readEntry(s_enabledKey, false);
    m_savedPluginEnabled = m_pluginEnabled;
    Q_EMIT pluginEnabledChanged();
    changed();
}

void KCMWinLikeGestures::save()
{
    KConfigGroup group = m_config->group(s_group);
    // Remove entries that are back to their default value.
    const QStringList existing = group.keyList();
    for (const QString &key : existing) {
        if (!m_overrides.contains(key)) {
            group.deleteEntry(key, KConfig::Notify);
        }
    }
    for (auto it = m_overrides.cbegin(); it != m_overrides.cend(); ++it) {
        group.writeEntry(it.key(), it.value(), KConfig::Notify);
    }
    m_config->sync();
    m_savedOverrides = m_overrides;

    if (m_pluginEnabled != m_savedPluginEnabled) {
        KConfigGroup plugins = kwinPlugins();
        plugins.writeEntry(s_enabledKey, m_pluginEnabled);
        plugins.sync();
        setPluginLoaded(m_pluginEnabled);
        m_savedPluginEnabled = m_pluginEnabled;
    }
    changed();
}

void KCMWinLikeGestures::defaults()
{
    // Restores the Windows-style bindings. Leaves the on/off switch alone:
    // "Defaults" should not silently turn the feature off.
    m_overrides.clear();
    changed();
}

void KCMWinLikeGestures::changed()
{
    ++m_revision;
    Q_EMIT revisionChanged();
    setNeedsSave(m_overrides != m_savedOverrides || m_pluginEnabled != m_savedPluginEnabled);
    setRepresentsDefaults(m_overrides.isEmpty());
}

#include "kcm_winlikegestures.moc"
