/*
    SPDX-FileCopyrightText: 2026 LiberaTeMetuMortis <78408599+LiberaTeMetuMortis@users.noreply.github.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KQuickConfigModule>
#include <KSharedConfig>

#include <QMap>
#include <QStringList>

/**
 * System Settings page for the winlikegestures KWin plugin.
 *
 * Holds the user's overrides of ~/.config/kwinwinlikegesturesrc [Gestures] in
 * memory; QML reads effective values (override, else built-in default) and
 * writes new ones. Saving writes only values that differ from the defaults,
 * with KConfig::Notify so the running plugin reloads straight away.
 */
class KCMWinLikeGestures : public KQuickConfigModule
{
    Q_OBJECT
    Q_PROPERTY(bool pluginEnabled READ pluginEnabled WRITE setPluginEnabled NOTIFY pluginEnabledChanged)
    Q_PROPERTY(QStringList kwinShortcuts READ kwinShortcuts CONSTANT)
    // Bumped on every change so QML bindings that call value() re-evaluate.
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)

public:
    KCMWinLikeGestures(QObject *parent, const KPluginMetaData &metaData);

    void load() override;
    void save() override;
    void defaults() override;

    bool pluginEnabled() const { return m_pluginEnabled; }
    void setPluginEnabled(bool enabled);
    QStringList kwinShortcuts() const;
    int revision() const { return m_revision; }

    // Effective value of a key: the user's override, else the built-in default,
    // else "inherit" for state keys ("Swipe3Down.Overview") and "passthrough".
    Q_INVOKABLE QString value(const QString &key) const;
    Q_INVOKABLE void setValue(const QString &key, const QString &value);
    Q_INVOKABLE void resetValue(const QString &key);

Q_SIGNALS:
    void pluginEnabledChanged();
    void revisionChanged();

private:
    QString defaultValue(const QString &key) const;
    void changed();

    KSharedConfig::Ptr m_config;
    QMap<QString, QString> m_defaults;
    QMap<QString, QString> m_overrides;
    QMap<QString, QString> m_savedOverrides;
    bool m_pluginEnabled = false;
    bool m_savedPluginEnabled = false;
    int m_revision = 0;
    mutable QStringList m_kwinShortcuts;
    mutable bool m_kwinShortcutsLoaded = false;
};
