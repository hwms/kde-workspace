/*
    Copyright 2007 Robert Knight <robertknight@gmail.com>
    Copyright 2008-2009 Sebastian Sauer <mail@dipe.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

// Own
#include "menuview.h"

// Qt
#include <QtCore/QAbstractItemModel>
#include <QtCore/QStack>
#include <QtGui/QApplication>
#include <QtGui/QMouseEvent>
#include <QtCore/QPersistentModelIndex>

// KDE
#include <KDebug>
#include <KUrl>
#include <KIconLoader>

// Local
#include "core/models.h"
#include "core/itemhandlers.h"

Q_DECLARE_METATYPE(QPersistentModelIndex)
Q_DECLARE_METATYPE(QAction*)

using namespace Kickoff;

/// @internal d-pointer class
class MenuView::Private
{
public:
    enum { ActionRole = Qt::UserRole + 52 };

    Private(MenuView *q) : q(q), column(0), launcher(new UrlItemLauncher(q)), formattype(MenuView::DescriptionName) {}

    QAction *createActionForIndex(QAbstractItemModel *model, const QModelIndex& index, QMenu *parent) {
        Q_ASSERT(index.isValid());
        QAction *action = 0;
        if (model->hasChildren(index)) {
            KMenu *childMenu = new KMenu(parent);
            childMenu->installEventFilter(q);
            action = childMenu->menuAction();
            if (model->canFetchMore(index)) {
                model->fetchMore(index);
            }
            buildBranch(childMenu, model, index);
        } else {
            action = q->createLeafAction(index, parent);
        }
        q->updateAction(model, action, index);
        return action;
    }

    void buildBranch(QMenu *menu, QAbstractItemModel *model, const QModelIndex& parent) {
        int rowCount = model->rowCount(parent);
        for (int i = 0; i < rowCount; i++) {
            QAction *action = createActionForIndex(model, model->index(i, column, parent), menu);
            menu->addAction(action);
        }
    }

    MenuView * const q;
    int column;
    UrlItemLauncher *launcher;
    MenuView::FormatType formattype;
    QPoint mousePressPos;
};

MenuView::MenuView(QWidget *parent)
    : KMenu(parent)
    , d(new Private(this))
{
    installEventFilter(this);
}

MenuView::~MenuView()
{
    delete d;
}

QAction *MenuView::createLeafAction(const QModelIndex&, QObject *parent)
{
    return new QAction(parent);
}

void MenuView::updateAction(QAbstractItemModel *model, QAction *action, const QModelIndex& index)
{
    QString text = index.data(Qt::DisplayRole).value<QString>().replace("&", "&&"); // describing text, e.g. "Spreadsheet" or "Rekall" (right, sometimes the text is also used for the generic app-name)
    QString name = index.data(Kickoff::SubTitleRole).value<QString>().replace("&", "&&"); // the generic name, e.g. "kspread" or "OpenOffice.org Spreadsheet" or just "" (right, it's a mess too)
    if (action->menu() != 0) { // if it is an item with sub-menuitems, we probably like to thread them another way...
        action->setText(text);
    } else {
        switch (d->formattype) {
        case Name: {
            if (name.isEmpty()) {
                action->setText(text);
            } else {
                action->setText(name);
            }
        }
        break;
        case Description: {
            if (name.contains(text, Qt::CaseInsensitive)) {
                text = name;
            }
            action->setText(text);
        }
        break;
        case NameDescription: // fall through
        case NameDashDescription: // fall through
        case DescriptionName: {
            if (!name.isEmpty()) { // seems we have a program, but some of them don't define a name at all
                if (text.contains(name, Qt::CaseInsensitive)) { // sometimes the description contains also the name
                    action->setText(text);
                } else if (name.contains(text, Qt::CaseInsensitive)) { // and sometimes the name also contains the description
                    action->setText(name);
                } else { // seems we have a perfect desktop-file (likely a KDE one, heh) and name+description are clear separated
                    if (d->formattype == NameDescription) {
                        action->setText(QString("%1 %2").arg(name).arg(text));
                    } else if (d->formattype == NameDashDescription) {
                        action->setText(QString("%1 - %2").arg(name).arg(text));
                    } else {
                        action->setText(QString("%1 (%2)").arg(text).arg(name));
                    }
                }
            } else { // if there is no name, let's just use the describing text
                action->setText(text);
            }
        }
        break;
        }
    }

    action->setIcon(index.data(Qt::DecorationRole).value<QIcon>());

    // we map modelindex and action together
    action->setData(qVariantFromValue(QPersistentModelIndex(index)));

    // don't emit the dataChanged-signal cause else we may end in a infinite loop
    model->blockSignals(true);
    model->setData(index, qVariantFromValue(action), Private::ActionRole);
    model->blockSignals(false);
}

bool MenuView::eventFilter(QObject *watched, QEvent *event)
{
    switch(event->type()) {
    case QEvent::MouseMove: {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        QMenu *watchedMenu = qobject_cast<QMenu*>(watched);
        const int mousePressDistance = !d->mousePressPos.isNull() ? (mouseEvent->pos() - d->mousePressPos).manhattanLength() : 0;

        if (watchedMenu && mouseEvent->buttons() & Qt::LeftButton
                && mousePressDistance >= QApplication::startDragDistance()) {
            QAction *action = watchedMenu->actionAt(mouseEvent->pos());
            if (!action) {
                return KMenu::eventFilter(watched, event);
            }

            QPersistentModelIndex index = action->data().value<QPersistentModelIndex>();
            if (!index.isValid()) {
                return KMenu::eventFilter(watched, event);
            }

            QString urlString = index.data(UrlRole).toString();
            if (urlString.isNull()) {
                return KMenu::eventFilter(watched, event);
            }

            QMimeData *mimeData = new QMimeData();
            mimeData->setData("text/uri-list", urlString.toAscii());
            mimeData->setText(mimeData->text());
            QDrag *drag = new QDrag(this);
            drag->setMimeData(mimeData);

            QIcon icon = action->icon();
            drag->setPixmap(icon.pixmap(IconSize(KIconLoader::Desktop)));

            d->mousePressPos = QPoint();

            Qt::DropAction dropAction = drag->exec();
            Q_UNUSED(dropAction);

            return true;
        }
    } break;
    case QEvent::MouseButtonPress: {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        QMenu *watchedMenu = qobject_cast<QMenu*>(watched);
        if (watchedMenu) {
            d->mousePressPos = mouseEvent->pos();
        }
    } break;
    case QEvent::MouseButtonRelease: {
        QMenu *watchedMenu = qobject_cast<QMenu*>(watched);
        if (watchedMenu) {
            d->mousePressPos = QPoint();
        }
    } break;
    case QEvent::Hide: {
        emit afterBeingHidden();
    } break;
    default: break;
    }

    return KMenu::eventFilter(watched, event);
}

void MenuView::addModel(QAbstractItemModel *model, bool mergeFirstLevel)
{
    if(mergeFirstLevel) {
        const int count = model->rowCount();
        for(int row = 0; row < count; ++row) {
            QModelIndex index = model->index(row, 0, QModelIndex());
            Q_ASSERT(index.isValid());
            
            model->blockSignals(true);
            model->setData(index, qVariantFromValue(this->menuAction()), Private::ActionRole);
            model->blockSignals(false);

            d->buildBranch(this, model, index);
        }
    } else {
        d->buildBranch(this, model, QModelIndex());
    }

    connect(model, SIGNAL(rowsInserted(QModelIndex, int, int)), this, SLOT(rowsInserted(QModelIndex, int, int)));
    connect(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex, int, int)), this, SLOT(rowsAboutToBeRemoved(QModelIndex, int, int)));
    connect(model, SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(dataChanged(QModelIndex, QModelIndex)));
    connect(model, SIGNAL(modelReset()), this, SLOT(modelReset()));
}

UrlItemLauncher *MenuView::launcher() const
{
    return d->launcher;
}

QModelIndex MenuView::indexForAction(QAction *action) const
{
    Q_ASSERT(action != 0);
    QPersistentModelIndex index = action->data().value<QPersistentModelIndex>();
    return index;
}

QAction *MenuView::actionForIndex(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return this->menuAction();
    }

    const QAbstractItemModel *model = index.model();
    Q_ASSERT(model);
    QVariant v = model->data(index, Private::ActionRole);
    Q_ASSERT(v.isValid());
    QAction* a = v.value<QAction*>();
    Q_ASSERT(a);
    return a;
}

bool MenuView::isValidIndex(const QModelIndex& index) const
{
    QVariant v = (index.isValid() && index.model()) ? index.model()->data(index, Private::ActionRole) : QVariant();
    return v.isValid() && v.value<QAction*>();
}

void MenuView::rowsInserted(const QModelIndex& parent, int start, int end)
{
    kDebug()<<start<<end;
#if 0
    Q_ASSERT(parent.isValid());
    Q_ASSERT(parent.model());

    QAction *menuAction = actionForIndex(parent);
    Q_ASSERT(menuAction);
    QMenu *menu = menuAction->menu();
    Q_ASSERT(menu);

    QAbstractItemModel *model = const_cast<QAbstractItemModel*>(parent.model());
    Q_ASSERT(model);

    QList<QAction*> newActions;
    for (int row = start; row <= end; row++) {
        QModelIndex index = model->index(row, d->column, parent);
        QAction *newAction = d->createActionForIndex(model, index, menu);
        newActions << newAction;
    }

    int lastidx = 0;
    int st = start;
    for(int i = 0; i < menu->actions().count(); ++i) {
        QAction *action = menu->actions()[i];
        Q_ASSERT(action);
        QModelIndex index = indexForAction(action);
        if(index.isValid() && index.model() == model) {
            lastidx = i;
            if(0 == st--)
                break;
        }
    }
    if (lastidx < menu->actions().count()) {
        menu->insertActions(menu->actions()[lastidx], newActions);
    } else {
        menu->addActions(newActions);
    }

    /*
    //if (!isValidIndex(parent)) return; // can happen if the models data is incomplete yet
    QAbstractItemModel *model = const_cast<QAbstractItemModel*>(parent.model());
    Q_ASSERT(model);
    QAction *menuAction = actionForIndex(parent);
    Q_ASSERT(menuAction);
    QMenu *menu = menuAction->menu();
    Q_ASSERT(menu);
    QList<QAction*> newActions;
    for (int row = start; row <= end; row++) {
        QModelIndex index = model->index(row, d->column, parent);
        QAction *newAction = d->createActionForIndex(model, index, menu);
        newActions << newAction;
    }
    if (start < menu->actions().count()) {
        menu->insertActions(menu->actions()[start], newActions);
    } else {
        menu->addActions(newActions);
    }
    */
#else
    modelReset();
#endif
}

void MenuView::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
{
    kDebug()<<start<<end;
#if 0
    Q_ASSERT(parent.isValid());
    Q_ASSERT(parent.model());

    QAbstractItemModel *model = const_cast<QAbstractItemModel*>(parent.model());
    Q_ASSERT(model);

    for (int row = end; row >= start; --row) {
        QModelIndex index = model->index(row, d->column, parent);
        QAction *action = actionForIndex(index);
        Q_ASSERT(action);
        QMenu *menu = dynamic_cast<QMenu*>(action->parent());
        Q_ASSERT(menu);
        menu->removeAction(action);
    }

    /*
    QAction *menuAction = 0;
    if(parent.model()->data(parent, Private::ActionRole).isValid()) {
        menuAction = actionForIndex(parent);
    } else {
    }
    Q_ASSERT(menuAction);
    QMenu *menu = menuAction->menu();
    Q_ASSERT(menu);
    QList<QAction*> actions = menu->actions();
    Q_ASSERT(end < actions.count());
    for (int row = end; row >= start; row--) {
        menu->removeAction(actions[row]);
    }
    */
#else
    modelReset();
#endif
}

void MenuView::dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    kDebug();
#if 0
    Q_ASSERT(isValidIndex(topLeft));
    Q_ASSERT(isValidIndex(bottomRight));
    //if (!isValidIndex(parent)) return; // can happen if the models data is incomplete yet

    QAbstractItemModel *model = const_cast<QAbstractItemModel*>(topLeft.model());
    Q_ASSERT(model);

    QAction *menuAction = actionForIndex(topLeft.parent());
    Q_ASSERT(menuAction);

    QMenu *menu = menuAction->menu();
    Q_ASSERT(menu);

    QList<QAction*> actions = menu->actions();
    Q_ASSERT(bottomRight.row() < actions.count());

    for (int row = topLeft.row(); row <= bottomRight.row(); row++) {
        updateAction(model, actions[row], model->index(row, d->column, topLeft.parent()));
    }
#else
    modelReset();
#endif
}

void MenuView::modelReset()
{
    // We need to force clearance of the menu and rebuild from scratch
    deleteLater();
}

void MenuView::setColumn(int column)
{
    d->column = column;
    modelReset();
}

int MenuView::column() const
{
    return d->column;
}

MenuView::FormatType MenuView::formatType() const
{
    return d->formattype;
}

void MenuView::setFormatType(MenuView::FormatType formattype)
{
    d->formattype = formattype;
}

void MenuView::actionTriggered(QAction *action)
{
    QModelIndex index = indexForAction(action);
    Q_ASSERT(index.isValid());
    d->launcher->openItem(index);
}

#include "menuview.moc"
