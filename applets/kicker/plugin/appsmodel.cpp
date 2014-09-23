/***************************************************************************
 *   Copyright (C) 2012 Aurélien Gâteau <agateau@kde.org>                  *
 *   Copyright (C) 2013-2014 by Eike Hein <hein@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "appsmodel.h"
#include "actionlist.h"
#include "containmentinterface.h"
#include "menuentryeditor.h"
#include "config-workspace.h"

#ifdef PackageKitQt5_FOUND
#include "findpackagenamejob.h"
#endif

#include <QTimer>
#include <QProcess>
#include <QQmlPropertyMap>
#include <QStandardPaths>

#include <KLocalizedString>
#include <KRun>
#include <KSycoca>
#include <KShell>
#include <KJob>

AppGroupEntry::AppGroupEntry(KServiceGroup::Ptr group, AppsModel *parentModel,
    bool flat, int appNameFormat)
{
    m_name = group->caption();
    m_icon = QIcon::fromTheme(group->icon());
    AppsModel* model = new AppsModel(group->entryPath(), flat, parentModel);
    model->setAppletInterface(parentModel->appletInterface());
    model->setAppNameFormat(appNameFormat);
    m_model = model;
    QObject::connect(parentModel, SIGNAL(appletInterfaceChanged(QObject*)), model, SLOT(setAppletInterface(QObject*)));
    QObject::connect(parentModel, SIGNAL(refreshing()), m_model, SLOT(deleteLater()));
    QObject::connect(m_model, SIGNAL(appLaunched(QString)), parentModel, SIGNAL(appLaunched(QString)));
}

AppEntry::AppEntry(KService::Ptr service, NameFormat nameFormat)
: m_service(service)
{
    const QString &name = service->name();
    QString genericName = service->genericName();

    if (genericName.isEmpty()) {
        genericName = service->comment();
    }

    if (nameFormat == NameOnly || genericName.isEmpty() || name == genericName) {
        m_name = name;
    } else if (nameFormat == GenericNameOnly) {
        m_name = genericName;
    } else if (nameFormat == NameAndGenericName) {
        m_name = i18nc("App name (Generic name)", "%1 (%2)", name, genericName);
    } else {
        m_name = i18nc("Generic name (App name)", "%1 (%2)", genericName, name);
    }

    m_icon = QIcon::fromTheme(service->icon());
    m_service = service;
}

MenuEntryEditor *AppsModel::m_menuEntryEditor = 0;

AppsModel::AppsModel(const QString &entryPath, bool flat, QObject *parent)
: AbstractModel(parent)
, m_entryPath(entryPath)
, m_changeTimer(0)
, m_flat(flat)
, m_appNameFormat(AppEntry::NameOnly)
, m_sortNeeded(false)
, m_appletInterface(0)
{
    if (!m_menuEntryEditor) {
        m_menuEntryEditor = new MenuEntryEditor();
    }
}

AppsModel::~AppsModel()
{
    qDeleteAll(m_entryList);
}

QVariant AppsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_entryList.count()) {
        return QVariant();
    }

    const AbstractEntry *entry = m_entryList.at(index.row());

    if (role == Qt::DisplayRole) {
        return entry->name();
    } else if (role == Qt::DecorationRole) {
        return entry->icon();
    } else if (role == Kicker::IsParentRole) {
        return (entry->type() == AbstractEntry::GroupType);
    } else if (role == Kicker::HasChildrenRole) {
        if (entry->type() == AbstractEntry::GroupType) {
            const AbstractGroupEntry *groupEntry = static_cast<const AbstractGroupEntry *>(entry);

            if (groupEntry->model() && groupEntry->model()->count()) {
                return true;
            }
        }
    } else if (role == Kicker::FavoriteIdRole) {
        if (entry->type() == AbstractEntry::RunnableType) {
            return QVariant("app:" + static_cast<AppEntry *>(m_entryList.at(index.row()))->service()->storageId());
        }
    } else if (role == Kicker::HasActionListRole) {
        if (entry->type() == AbstractEntry::RunnableType || !m_hiddenEntries.isEmpty()) {
            return true;
        } else if (entry->type() == AbstractEntry::GroupType) {
            const AbstractGroupEntry *groupEntry = static_cast<const AbstractGroupEntry *>(entry);

            if (groupEntry->model()) {
                const AppsModel *appsModel = qobject_cast<const AppsModel *>(groupEntry->model());

                if (appsModel && !appsModel->hiddenEntries().isEmpty()) {
                    return true;
                }
            }
        }
    } else if (role == Kicker::ActionListRole) {
        QVariantList actionList;

        if (entry->type() == AbstractEntry::RunnableType) {
            const KService::Ptr service = static_cast<const AppEntry *>(entry)->service();

            if (ContainmentInterface::mayAddLauncher(m_appletInterface, ContainmentInterface::Desktop)) {
                actionList << Kicker::createActionItem(i18n("Add to Desktop"), "addToDesktop");
            }

            if (ContainmentInterface::mayAddLauncher(m_appletInterface, ContainmentInterface::Panel)) {
                actionList << Kicker::createActionItem(i18n("Add to Panel"), "addToPanel");
            }

            if (m_menuEntryEditor->canEdit(service->entryPath())) {
                actionList << Kicker::createSeparatorActionItem();

                QVariantMap editAction = Kicker::createActionItem(i18n("Edit Application..."), "editApplication");
                editAction["icon"] = "kmenuedit"; // TODO: Using the KMenuEdit icon might be misleading.
                actionList << editAction;
            }

#ifdef PackageKitQt5_FOUND
            QStringList files(service->entryPath());

            if (service->isApplication()) {
                files += QStandardPaths::findExecutable(KShell::splitArgs(service->exec()).first());
            }

            FindPackageJob* job = new FindPackageJob(files); // TODO: Would be great to make this async.

            if (job->exec() && !job->packageNames().isEmpty()) {
                QString packageName = job->packageNames().first();

                QVariantMap removeAction = Kicker::createActionItem(i18n("Remove '%1'...", packageName), "removeApplication", packageName);
                removeAction["icon"] = "applications-other";
                actionList << removeAction;
            }
#endif

            if (appletConfig() && appletConfig()->contains("hiddenApplications")) {
                const QStringList &hiddenApps = appletConfig()->value("hiddenApplications").toStringList();

                if (!hiddenApps.contains(service->menuId())) {
                    actionList << Kicker::createActionItem(i18n("Hide Application"), "hideApplication");
                }
            }
        }

        if (!m_hiddenEntries.isEmpty()) {
            actionList << Kicker::createSeparatorActionItem();
            actionList << Kicker::createActionItem(i18n("Unhide Applications in this Submenu"), "unhideSiblingApplications");
        }

        if (entry->type() == AbstractEntry::GroupType) {
            const AbstractGroupEntry *groupEntry = static_cast<const AbstractGroupEntry *>(entry);

            if (groupEntry->model()) {
                const AppsModel *appsModel = qobject_cast<const AppsModel *>(groupEntry->model());

                if (appsModel && !appsModel->hiddenEntries().isEmpty()) {
                    actionList << Kicker::createActionItem(i18n("Unhide Applications in '%1'", entry->name()), "unhideChildApplications");
                }
            }
        }

        return actionList;
    } else if (role == Kicker::UrlRole) {
        if (entry->type() == AbstractEntry::RunnableType) {
            return QUrl::fromLocalFile(static_cast<AppEntry *>(m_entryList.at(index.row()))->service()->entryPath());
        }
    }

    return QVariant();
}

int AppsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entryList.count();
}

bool AppsModel::trigger(int row, const QString &actionId, const QVariant &argument)
{
    Q_UNUSED(argument)

    if (row < 0 || row >= m_entryList.count()) {
        return false;
    }

    const AbstractEntry *entry = m_entryList.at(row);
    KService::Ptr service;

    if (entry->type() == AbstractEntry::RunnableType) {
        service  = static_cast<const AppEntry *>(entry)->service();
    }

    if (actionId == "addToDesktop" && ContainmentInterface::mayAddLauncher(m_appletInterface, ContainmentInterface::Desktop)) {
        ContainmentInterface::addLauncher(m_appletInterface, ContainmentInterface::Desktop, service->entryPath());
    } else if (actionId == "addToPanel" && ContainmentInterface::mayAddLauncher(m_appletInterface, ContainmentInterface::Panel)) {
        ContainmentInterface::addLauncher(m_appletInterface, ContainmentInterface::Panel, service->entryPath());
    } else if (actionId == "addToTaskManager" && ContainmentInterface::mayAddLauncher(m_appletInterface, ContainmentInterface::TaskManager, service->entryPath())) {
        ContainmentInterface::addLauncher(m_appletInterface, ContainmentInterface::TaskManager, service->entryPath());
    } else if (actionId == "editApplication" && m_menuEntryEditor->canEdit(service->entryPath())) {
        QMetaObject::invokeMethod(m_menuEntryEditor, "edit", Qt::QueuedConnection,
            Q_ARG(QString, service->entryPath()),
            Q_ARG(QString, service->menuId()));

        return true;
    } else if (actionId == "removeApplication") {
        if (appletConfig() && appletConfig()->contains("removeApplicationCommand")) {
            const QStringList &removeAppCmd = KShell::splitArgs(appletConfig()->value("removeApplicationCommand").toString());

            if (!removeAppCmd.isEmpty()) {
                return QProcess::startDetached(removeAppCmd.first(), removeAppCmd.mid(1) << argument.toString());
            }
        }
    } else if (actionId == "hideApplication") {
        if (appletConfig() && appletConfig()->contains("hiddenApplications")) {
            QStringList hiddenApps = appletConfig()->value("hiddenApplications").toStringList();

            if (!hiddenApps.contains(service->menuId())) {
                hiddenApps << service->menuId();

                appletConfig()->insert("hiddenApplications", hiddenApps);
                QMetaObject::invokeMethod(appletConfig(), "valueChanged", Qt::DirectConnection,
                    Q_ARG(QString, "hiddenApplications"),
                    Q_ARG(QVariant, hiddenApps));

                refresh();
            }
        }
    } else if (actionId == "unhideSiblingApplications") {
        if (appletConfig() && appletConfig()->contains("hiddenApplications")) {
            QStringList hiddenApps = appletConfig()->value("hiddenApplications").toStringList();

            foreach(const QString& app, m_hiddenEntries) {
                hiddenApps.removeOne(app);
            }

            appletConfig()->insert("hiddenApplications", hiddenApps);
            QMetaObject::invokeMethod(appletConfig(), "valueChanged", Qt::DirectConnection,
                Q_ARG(QString, "hiddenApplications"),
                Q_ARG(QVariant, hiddenApps));

            m_hiddenEntries.clear();

            refresh();
        }
    } else if (actionId == "unhideChildApplications") {
        if (entry->type() == AbstractEntry::GroupType
            && appletConfig() && appletConfig()->contains("hiddenApplications")) {
            const AbstractGroupEntry *groupEntry = static_cast<const AbstractGroupEntry *>(entry);

            if (groupEntry->model()) {
                const AppsModel *appsModel = qobject_cast<const AppsModel *>(groupEntry->model());

                QStringList hiddenApps = appletConfig()->value("hiddenApplications").toStringList();

                foreach(const QString& app, appsModel->hiddenEntries()) {
                    hiddenApps.removeOne(app);
                }

                appletConfig()->insert("hiddenApplications", hiddenApps);
                QMetaObject::invokeMethod(appletConfig(), "valueChanged", Qt::DirectConnection,
                    Q_ARG(QString, "hiddenApplications"),
                    Q_ARG(QVariant, hiddenApps));

                refresh();
            }
        }
    } else if (actionId.isEmpty() && service) {
        bool ran = KRun::run(*service, QList<QUrl>(), 0);

        if (ran) {
            emit appLaunched(service->storageId());
        }

        return ran;
    }

    return false;
}

AbstractModel *AppsModel::modelForRow(int row)
{
    if (row < 0 || row >= m_entryList.count() || m_entryList.at(row)->type() != AbstractEntry::GroupType) {
        return 0;
    }

    return static_cast<AbstractGroupEntry *>(m_entryList.at(row))->model();
}

bool AppsModel::flat() const
{
    return m_flat;
}

void AppsModel::setFlat(bool flat)
{
    if (m_flat != flat) {
        m_flat = flat;

        refresh();

        emit flatChanged();
    }
}

int AppsModel::appNameFormat() const
{
    return m_appNameFormat;
}

void AppsModel::setAppNameFormat(int format)
{
    if (m_appNameFormat != (AppEntry::NameFormat)format) {
        m_appNameFormat = (AppEntry::NameFormat)format;

        refresh();

        emit appNameFormatChanged();
    }
}

QStringList AppsModel::hiddenEntries() const
{
    return m_hiddenEntries;
}

QObject* AppsModel::appletInterface() const
{
    return m_appletInterface;
}

void AppsModel::setAppletInterface(QObject* appletInterface)
{
    if (m_appletInterface != appletInterface) {
        m_appletInterface = appletInterface;

        refresh();

        emit appletInterfaceChanged(m_appletInterface);
    }
}

void AppsModel::refresh()
{
    beginResetModel();

    emit refreshing();

    qDeleteAll(m_entryList);
    m_entryList.clear();
    m_hiddenEntries.clear();

    if (m_entryPath.isEmpty()) {
        KServiceGroup::Ptr group = KServiceGroup::root();
        KServiceGroup::List list = group->entries(true);

        for (KServiceGroup::List::ConstIterator it = list.constBegin(); it != list.constEnd(); it++) {
            const KSycocaEntry::Ptr p = (*it);

            if (p->isType(KST_KServiceGroup)) {
                KServiceGroup::Ptr subGroup(static_cast<KServiceGroup*>(p.data()));

                if (!subGroup->noDisplay() && subGroup->childCount() > 0) {
                    m_entryList << new AppGroupEntry(subGroup, this, m_flat, m_appNameFormat);
                }
            }
        }

        m_changeTimer = new QTimer(this);
        m_changeTimer->setSingleShot(true);
        m_changeTimer->setInterval(100);
        connect(m_changeTimer, SIGNAL(timeout()), this, SLOT(refresh()));

        connect(KSycoca::self(), SIGNAL(databaseChanged(QStringList)), SLOT(checkSycocaChanges(QStringList)));
    } else {
        KServiceGroup::Ptr group = KServiceGroup::group(m_entryPath);
        processServiceGroup(group);

        if (m_sortNeeded) {
            qSort(m_entryList.begin(), m_entryList.end(), AbstractEntry::lessThan);

            m_sortNeeded = false;
        }
    }

    endResetModel();

    emit countChanged();
}

void AppsModel::processServiceGroup(KServiceGroup::Ptr group)
{
    if (!group || !group->isValid()) {
        return;
    }

    KServiceGroup::List list = group->entries(true);

    QStringList hiddenApps;

    if (appletConfig() && appletConfig()->contains("hiddenApplications")) {
        hiddenApps = appletConfig()->value("hiddenApplications").toStringList();
    }

    for (KServiceGroup::List::ConstIterator it = list.constBegin();
        it != list.constEnd(); it++) {
        const KSycocaEntry::Ptr p = (*it);

        if (p->isType(KST_KService)) {
            const KService::Ptr service(static_cast<KService*>(p.data()));

            if (service->noDisplay()) {
                continue;
            }

            if (hiddenApps.contains(service->menuId())) {
                m_hiddenEntries << service->menuId();

                continue;
            }

            bool found = false;

            foreach (const AbstractEntry *entry, m_entryList) {
                if (entry->type() == AbstractEntry::RunnableType
                    && static_cast<const AppEntry *>(entry)->service()->storageId() == service->storageId()) {
                    found = true;
                }
            }

            if (!found) {
                m_entryList << new AppEntry(service, m_appNameFormat);
            }
        } else if (p->isType(KST_KServiceGroup)) {
            if (m_flat) {
                const KServiceGroup::Ptr serviceGroup(static_cast<KServiceGroup*>(p.data()));
                processServiceGroup(serviceGroup);

                m_sortNeeded = true;
            } else {
                const KServiceGroup::Ptr subGroup(static_cast<KServiceGroup*>(p.data()));

                if (!subGroup->noDisplay() && subGroup->childCount() > 0) {
                    AppGroupEntry *groupEntry = new AppGroupEntry(subGroup, this, m_flat, m_appNameFormat);
                    connect(groupEntry->model(), SIGNAL(countChanged()), this, SLOT(refresh()));
                    m_entryList << groupEntry;
                }
            }
        }
    }
}

void AppsModel::checkSycocaChanges(const QStringList &changes)
{
    if (changes.contains("services") || changes.contains("apps") || changes.contains("xdgdata-apps")) {
        m_changeTimer->start();
    }
}

QQmlPropertyMap *AppsModel::appletConfig() const
{
    if (m_appletInterface) {
        return qobject_cast<QQmlPropertyMap *>(m_appletInterface->property("configuration").value<QObject *>());
    }

    return 0;
}

#include "appsmodel.moc"
