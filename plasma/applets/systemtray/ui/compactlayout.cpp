/***************************************************************************
 *   compactlayout.cpp                                                     *
 *                                                                         *
 *   Copyright (C) 2008 Jason Stubbs <jasonbstubbs@gmail.com>              *
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

#include "compactlayout.h"

#include <QtCore/QHash>

#include <QtGui/QGraphicsWidget>


namespace SystemTray
{


class CompactLayout::Private
{
public:
    Private(CompactLayout *q)
        : q(q),
          spacing(4.0)
    {
    }

    QHash<QGraphicsLayoutItem*, QRectF> calculateGeometries(const QRectF &rect,
                                                            Qt::SizeHint which,
                                                            const QSizeF &constraint) const;
    void addPadding(QHash<QGraphicsLayoutItem*, QRectF> &geometries,
                    const QSizeF &constraint);
    QSizeF hackedConstraint(const QSizeF &constraint) const;
    void updateParentWidget(QGraphicsWidget *item);
    QRectF boundingRect(const QList<QRectF> &rects) const;

    CompactLayout *q;
    qreal spacing;
    QList<QGraphicsLayoutItem*> items;
};


CompactLayout::CompactLayout(QGraphicsLayoutItem *parent)
    : QGraphicsLayout(parent),
      d(new Private(this))
{
}


CompactLayout::~CompactLayout()
{
    delete d;
}


qreal CompactLayout::spacing() const
{
    return d->spacing;
}


void CompactLayout::setSpacing(qreal spacing)
{
    d->spacing = spacing;
}


void CompactLayout::insertItem(int index, QGraphicsLayoutItem *item)
{
    index = qBound(0, index, d->items.count() - 1);

    item->setParentLayoutItem(this);

    QGraphicsWidget *widget = dynamic_cast<QGraphicsWidget *>(item);
    if (widget) {
        d->updateParentWidget(widget);
    }

    d->items.insert(index, item);
    updateGeometry();
    activate();
}

void CompactLayout::addItem(QGraphicsLayoutItem *item)
{
    insertItem(d->items.count() - 1, item);
}

void CompactLayout::Private::updateParentWidget(QGraphicsWidget *item)
{
    QGraphicsLayoutItem *parentItem = q->parentLayoutItem();
    while (parentItem && parentItem->isLayout()) {
        parentItem = parentItem->parentLayoutItem();
    }

    if (parentItem) {
        item->setParentItem(static_cast<QGraphicsWidget*>(parentItem));
    }
}


void CompactLayout::removeItem(QGraphicsLayoutItem *item)
{
    d->items.removeAll(item);

    updateGeometry();
    activate();
}


bool CompactLayout::containsItem(QGraphicsLayoutItem *item) const
{
    return d->items.contains(item);
}


int CompactLayout::count() const
{
    return d->items.count();
}


void CompactLayout::setGeometry(const QRectF &rect)
{
    //kDebug() << rect;
    QHash<QGraphicsLayoutItem*, QRectF> geometries;
    geometries = d->calculateGeometries(rect, Qt::PreferredSize, rect.size());
    d->addPadding(geometries, rect.size());

    QHashIterator<QGraphicsLayoutItem*, QRectF> i(geometries);
    while (i.hasNext()) {
        i.next();
        QGraphicsLayoutItem *item = i.key();
        item->setGeometry(i.value());
    }
}


void CompactLayout::Private::addPadding(QHash<QGraphicsLayoutItem*, QRectF> &geometries, const QSizeF &constraint)
{
    QSizeF size = boundingRect(geometries.values()).size();

    qreal xAdjustment = (constraint.width() - size.width()) / 2.0;
    qreal yAdjustment = (constraint.height() - size.height()) / 2.0;

    if (xAdjustment || yAdjustment) {
        foreach (QGraphicsLayoutItem *item, items) {
            geometries[item].moveLeft(geometries[item].left() + xAdjustment);
            geometries[item].moveTop(geometries[item].top() + yAdjustment);
        }
    }
}


QGraphicsLayoutItem* CompactLayout::itemAt(int index) const
{
    return d->items.at(index);
}


void CompactLayout::removeAt(int index)
{
    d->items.removeAt(index);
}


QSizeF CompactLayout::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const
{
    if (which != Qt::PreferredSize) {
        return QSizeF();
    }

    QHash<QGraphicsLayoutItem*, QRectF> geometries =
        d->calculateGeometries(geometry(), which, d->hackedConstraint(constraint));

    return d->boundingRect(geometries.values()).size();
}


QRectF CompactLayout::Private::boundingRect(const QList<QRectF> &rects) const
{
    QRectF boundingRect;

    foreach (const QRectF &rect, rects) {
        if (boundingRect.isNull()) {
            boundingRect = rect;
        } else {
            boundingRect = boundingRect.united(rect);
        }
    }

    return boundingRect;
}


QHash<QGraphicsLayoutItem*, QRectF> CompactLayout::Private::calculateGeometries(const QRectF &geom, Qt::SizeHint which, const QSizeF &constraint) const
{
    QSizePolicy sizePolicy = q->parentLayoutItem()->sizePolicy();

    QHash<QGraphicsLayoutItem*, QRectF> geometries;
    QList<qreal> xPositions;
    QList<qreal> yPositions;

    xPositions << geom.left();
    yPositions << geom.top();

    foreach (QGraphicsLayoutItem *item, items) {
        QRectF rect;
        rect.setSize(item->effectiveSizeHint(which));

        rect.setWidth(qBound(item->minimumWidth(), rect.width(), constraint.width()));
        rect.setHeight(qBound(item->minimumHeight(), rect.height(), constraint.height()));

        // Try to find an empty space for the item within the bounds
        // of the already positioned out items
        foreach (qreal x, xPositions) {
            rect.moveLeft(x);
            if (rect.right() >= xPositions.last()) {
                continue;
            }

            foreach (qreal y, yPositions) {
                rect.moveTop(y);
                if (rect.bottom() >= yPositions.last()) {
                    continue;
                }

                bool overlapping = false;
                foreach (const QRectF &existingRect, geometries) {
                    if (existingRect.intersects(rect)) {
                        overlapping = true;
                    }
                }

                if (!overlapping) {
                    goto positioning_done;
                }
            }
        }

        // It didn't fit anywhere, so the current bounds will need to
        // be extended.
        Qt::Orientation direction;

        // Extend based on constraints
        if (yPositions.last() + rect.height() > constraint.height()) {
            direction = Qt::Horizontal;
        } else if (xPositions.last() + rect.width() > constraint.width()) {
            direction = Qt::Vertical;
        // Then extend based on expanding policy
        } else if (sizePolicy.horizontalPolicy() & QSizePolicy::ExpandFlag) {
            direction = Qt::Horizontal;
        } else if (sizePolicy.verticalPolicy() & QSizePolicy::ExpandFlag) {
            direction = Qt::Vertical;
        // Otherwise try to keep the shape of a square
        } else if (yPositions.last() >= xPositions.last()) {
            direction = Qt::Horizontal;
        } else {
            direction = Qt::Vertical;
        }

        if (direction == Qt::Horizontal) {
            rect.moveTop(yPositions.first());
            rect.moveLeft(xPositions.last());
        } else {
            rect.moveLeft(xPositions.first());
            rect.moveTop(yPositions.last());
        }

    positioning_done:
        if (!xPositions.contains(rect.right() + spacing)) {
            xPositions.append(rect.right() + spacing);
            qSort(xPositions);
        }

        if (!yPositions.contains(rect.bottom() + spacing)) {
            yPositions.append(rect.bottom() + spacing);
            qSort(yPositions);
        }

        geometries[item] = rect;
    }

    return geometries;
}


QSizeF CompactLayout::Private::hackedConstraint(const QSizeF &constraint) const
{
    // Qt doesn't seem to ever specify constraints to sizeHint()
    // but the layout needs to know what the constraints are.
    // This function returns a new constraint with the size of
    // the containing view when Qt hasn't passed a constraint.

    if (constraint.width() != -1 || constraint.height() != -1) {
        return constraint;
    }

    const QGraphicsWidget *widget = 0;
    const QGraphicsLayoutItem *item = q;

    while (item && !widget) {
        item = item->parentLayoutItem();
        if (!item->isLayout()) {
            widget = static_cast<const QGraphicsWidget*>(item);
        }
    }

    if (!widget) {
        return constraint;
    }

    QSizeF parentSize;
    qreal xMargins = 0.0, yMargins = 0.0;

    while (widget->parentWidget()) {
        widget = widget->parentWidget();
        parentSize = widget->size();

        qreal left, top, right, bottom;

        if (widget->layout()) {
            widget->layout()->getContentsMargins(&left, &top, &right, &bottom);
        } else {
            widget->getContentsMargins(&left, &top, &right, &bottom);
        }

        xMargins += left + right;
        yMargins += top + bottom;
    }

    return parentSize - QSizeF(xMargins, yMargins);
}


}
