/***********************************************************************************************************************
 * KDE System Tray (Plasmoid)
 *
 * Copyright (C) 2012 ROSA  <support@rosalab.ru>
 * License: GPLv2+
 * Authors: Dmitry Ashkadov <dmitry.ashkadov@rosalab.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .
 **********************************************************************************************************************/


#ifndef __SYSTEMTRAY__UITASK_H
#define __SYSTEMTRAY__UITASK_H

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <QtCore/QObject>
#include <QtCore/QVariant>

#include "../core/task.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class QGraphicsWidget;

namespace Plasma
{
class Applet;
}


namespace SystemTray
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations
class TasksPool;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// class UiTask
/// Provides access to properties of every task from QML code
class UiTask: public QObject
{
    Q_OBJECT

    Q_ENUMS(TaskHideState)
    Q_ENUMS(TaskType)
    Q_ENUMS(TaskStatus)
    Q_ENUMS(TaskCategory)

    Q_PROPERTY(TaskHideState hideState READ hideState NOTIFY changedHideState)
    Q_PROPERTY(TaskType type READ type CONSTANT)
    Q_PROPERTY(QGraphicsWidget *widget READ widget CONSTANT)
    Q_PROPERTY(TaskStatus status READ status NOTIFY changedStatus)
    Q_PROPERTY(QVariant task READ task CONSTANT)
    Q_PROPERTY(QString typeId READ typeId CONSTANT) // TODO: it may change
    Q_PROPERTY(QString taskId READ taskId CONSTANT)
    Q_PROPERTY(QString name READ name NOTIFY changedName)
    Q_PROPERTY(TaskCategory category READ category)  // TODO: it may change

public:
    enum TaskCategory
    {
        TaskCategoryUnknown = Task::UnknownCategory,
        TaskCategoryApplicationStatus = Task::ApplicationStatus,
        TaskCategoryCommunications = Task::Communications,
        TaskCategorySystemServices = Task::SystemServices,
        TaskCategoryHardware = Task::Hardware
    };

    enum TaskHideState
    {
        TaskHideStateAuto = 0,
        TaskHideStateHidden,
        TaskHideStateShown
    };

    enum TaskType
    {
        TaskTypeUnknown = 0,
        TaskTypePlasmoid,
        TaskTypeX11Task,
        TaskTypeStatusItem
    };

    enum TaskStatus
    {
        TaskStatusUnknown   = Task::UnknownStatus,
        TaskStatusPassive   = Task::Passive,
        TaskStatusActive    = Task::Active,
        TaskStatusAttention = Task::NeedsAttention
    };

    explicit UiTask(TasksPool &pool, QString task_id, Task *task);
    virtual ~UiTask();

    TaskHideState hideState() const;
    void setHideState(TaskHideState state);
    TaskStatus status() const;
    TaskCategory category() const;
    TaskType type() const;
    QVariant task() const;
    QString taskId() const;
    QGraphicsWidget *widget() const;
    Plasma::Applet *host() const;

    QString name() const;
    QString typeId() const;

    static UiTask::TaskType DefineTaskType(Task *t);


signals:
    void changedHideState();
    void changedStatus();
    void changedName();

public slots:
    void _onChangedStatus();

private:
    struct _Private;
    _Private * const d;
};


} // namespace SystemTray

#endif // __SYSTEMTRAY__UITASK_H