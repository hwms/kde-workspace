/*
 *  Copyright (C) 2007 Andriy Rysin (rysin@kde.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#ifndef KXKBAPPLET_H
#define KXKBAPPLET_H


#include <QMouseEvent>
#include <QPixmap>

#include <plasma/applet.h>


class QSizeF;
class KxkbCore;

class KxkbApplet : public Plasma::Applet
{
  Q_OBJECT
public:
    explicit KxkbApplet(QObject *parent, const QVariantList &args);
    ~KxkbApplet();
    
    void paintInterface(QPainter *painter,
                    const QStyleOptionGraphicsItem *option,
                                    const QRect& contentsRect);
    QSizeF contentSizeHint() const;

private:
    KxkbCore* m_kxkbCore;
};

K_EXPORT_PLASMA_APPLET(kxkb, KxkbApplet)

#endif
