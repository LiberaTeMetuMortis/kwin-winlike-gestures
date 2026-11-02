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
    m_defaults.insert(WinLike::GestureConfig::tuningDefaults());
    setButtons(Apply | Default);
}

QString KCMWinLikeGestures::defaultValue(const QString &key) const
{
    if (const auto it = m_defaults.constFind(key); it != m_defaults.cend()) {
        return *it;
    }
    // A state key ("Swipe3Up.Overview") without a built-in rule is no rule.
    return key.contains(QLatin1Char('.')) ? QStringLiteral("inherit") : QStringLiteral("passthrough");
}

QString KCMWinLikeGestures::value(const QString &key) const
{
    const QString v = m_overrides.value(key, defaultValue(key));
    // On a normal gesture, "inherit" written by hand means no binding.
    if (v == QLatin1String("inherit") && !key.contains(QLatin1Char('.'))) {
        return QStringLiteral("passthrough");
    }
    return v;
}

QStringList KCMWinLikeGestures::gestureKeys() const
{
    return WinLike::GestureConfig::gestureKeys();
}

QStringList KCMWinLikeGestures::situations() const
{
    return WinLike::GestureConfig::situations();
}

QStringList KCMWinLikeGestures::rules(const QString &suffix) const
{
    return WinLike::GestureConfig::ruleKeys(suffix, m_overrides);
}

void KCMWinLikeGestures::addRule(const QString &key)
{
    // A built-in rule comes back as it was; a new one starts as "Do nothing",
    // so it is visibly a rule and gets saved.
    setValue(key, m_defaults.value(key, QStringLiteral("block")));
}

void KCMWinLikeGestures::removeRule(const QString &key)
{
    // Back to no rule: drops a user rule, writes "inherit" over a built-in one.
    setValue(key, QStringLiteral("inherit"));
}

void KCMWinLikeGestures::removeSituation(const QString &suffix)
{
    for (const QString &key : rules(suffix)) {
        removeRule(key);
    }
}

QKeySequence KCMWinLikeGestures::keySequence(const QString &key) const
{
    const QString v = value(key);
    if (!v.startsWith(QLatin1String("key:"))) {
        return {};
    }
    return QKeySequence::fromString(v.mid(4), QKeySequence::PortableText);
}

void KCMWinLikeGestures::setKeySequence(const QString &key, const QKeySequence &sequence)
{
    if (sequence.isEmpty()) {
        setValue(key, QStringLiteral("block"));
        return;
    }
    // Only the first combination is pressed, so only that one is stored.
    setValue(key, QStringLiteral("key:") + QKeySequence(sequence[0]).toString(QKeySequence::PortableText));
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
    // Fetched once, on first use: the call goes into KWin (kglobalaccel lives
    // there) and blocks until KWin answers, so asking once per row made the
    // page slow to open. Blocking is fine here since we're not inside KWin.
    if (m_kwinShortcutsLoaded) {
        return m_kwinShortcuts;
    }
    m_kwinShortcutsLoaded = true;
    const QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kglobalaccel"),
                                                                QStringLiteral("/component/kwin"),
                                                                QStringLiteral("org.kde.kglobalaccel.Component"),
                                                                QStringLiteral("shortcutNames"));
    const QDBusReply<QStringList> reply = QDBusConnection::sessionBus().call(message);
    m_kwinShortcuts = reply.isValid() ? reply.value() : QStringList();
    m_kwinShortcuts.sort(Qt::CaseInsensitive);
    return m_kwinShortcuts;
}

void KCMWinLikeGestures::load()
{
    m_config->reparseConfiguration();
    m_overrides.clear();
    // Compared with the built-in defaults, not with what a row displays: a
    // state key equal to its normal gesture was written on purpose.
    m_overrides = WinLike::GestureConfig::overrides(m_config->group(s_group).entryMap());
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
