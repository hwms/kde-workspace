//////////////////////////////////////////////////////////////////////////////
// oxygenbutton.cpp
// -------------------
// Ozone window decoration for KDE. Buttons.
// -------------------
// Copyright (c) 2006, 2007 Riccardo Iaconelli <riccardo@kde.org>
// Copyright (c) 2006, 2007 Casper Boemann <cbr@boemann.dk>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//////////////////////////////////////////////////////////////////////////////
#include <math.h>
#include <QPainterPath>
#include <QPainter>
#include <QPen>
#include <QBitmap>

#include <kdecoration.h>
#include <kglobal.h>
#include <KColorUtils>
#include <kdebug.h>
#include <KColorScheme>

#include "oxygenclient.h"
#include "oxygenbutton.h"
#include "oxygen.h"

namespace Ozone
{
namespace Oxygen
{
// class OxygenClient;
/*
extern int BUTTONSIZE;
extern int DECOSIZE;*/

// static const int DECOSIZE        = 8;
//////////////////////////////////////////////////////////////////////////////
// OxygenButton Class                                                      //
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// OxygenButton()
// ---------------
// Constructor

OxygenButton::OxygenButton(OxygenClient &parent,
                             const QString& tip, ButtonType type)
    : KCommonDecorationButton((::ButtonType)type, &parent)
    , client_(parent)
    , helper_(parent.helper_)
    , type_(type)
    , lastmouse_(0)
    , colorCacheInvalid_(true)
{
    setAutoFillBackground(false);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setFixedSize(OXYGEN_BUTTONSIZE, OXYGEN_BUTTONSIZE);
    setCursor(Qt::ArrowCursor);
    setToolTip(tip);
}

OxygenButton::~OxygenButton()
{
}

//declare function from oxygenclient.cpp
QColor reduceContrast(const QColor &c0, const QColor &c1, double t);


QColor OxygenButton::buttonDetailColor(const QPalette &palette)
{
    if (client_.isActive())
        return palette.color(QPalette::Active, QPalette::ButtonText);
    else {
        if (colorCacheInvalid_) {
            QColor ab = palette.color(QPalette::Active, QPalette::Button);
            QColor af = palette.color(QPalette::Active, QPalette::ButtonText);
            QColor nb = palette.color(QPalette::Inactive, QPalette::Button);
            QColor nf = palette.color(QPalette::Inactive, QPalette::ButtonText);

            colorCacheInvalid_ = false;
            cachedButtonDetailColor_ = reduceContrast(nb, nf, qMax(qreal(2.5), KColorUtils::contrastRatio(ab, KColorUtils::mix(ab, af, 0.4))));
        }
        return cachedButtonDetailColor_;
    }
}

//////////////////////////////////////////////////////////////////////////////
// sizeHint()
// ----------
// Return size hint

QSize OxygenButton::sizeHint() const
{
    return QSize(OXYGEN_BUTTONSIZE, OXYGEN_BUTTONSIZE);
}

//////////////////////////////////////////////////////////////////////////////
// enterEvent()
// ------------
// Mouse has entered the button

void OxygenButton::enterEvent(QEvent *e)
{
    KCommonDecorationButton::enterEvent(e);
    if (status_ != Oxygen::Pressed) {
        status_ = Oxygen::Hovered;
    }
    update();
}

//////////////////////////////////////////////////////////////////////////////
// leaveEvent()
// ------------
// Mouse has left the button

void OxygenButton::leaveEvent(QEvent *e)
{
    KCommonDecorationButton::leaveEvent(e);
    // if we wanted to do mouseovers, we would keep track of it here
    status_ = Oxygen::Normal;
    update();
}

//////////////////////////////////////////////////////////////////////////////
// pressSlot()
// ------------
// Mouse has pressed the button

void OxygenButton::pressSlot()
{
    status_ = Oxygen::Pressed;
    update();
}
//////////////////////////////////////////////////////////////////////////////
// drawButton()
// ------------
// Draw the button

void OxygenButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QPalette pal = palette(); // de-const-ify

    if (type_ == ButtonMenu) {
        // we paint the mini icon (which is 16 pixels high)
        int dx = (width() - 16) / 2;
        int dy = (height() - 16) / 2;
        painter.drawPixmap(dx, dy, client_.icon().pixmap(16));
        return;
    }

    // Set palette to the right group.
    // TODO - fix KWin to do this for us :-).
    if (client_.isActive())
        pal.setCurrentColorGroup(QPalette::Active);
    else
        pal.setCurrentColorGroup(QPalette::Inactive);

    if(client_.maximizeMode() == OxygenClient::MaximizeRestore)
        painter.translate(0,-1);

    QColor bg = helper_.backgroundTopColor(OxygenFactory::blendTitlebarColors()?pal.window().color()
        :client_.options()->color(KDecorationDefines::ColorTitleBar,client_.isActive()));

    QLinearGradient lg = helper_.decoGradient(QRect(4,4,13,13), buttonDetailColor(pal));

    QColor bt = OxygenFactory::blendTitlebarColors()?pal.button().color()
            :client_.options()->color(KDecorationDefines::ColorButtonBg,client_.isActive());
    if(status_ == Oxygen::Hovered) {
        if(type_ == ButtonClose) {
            QColor color = KColorScheme(pal.currentColorGroup()).foreground(KColorScheme::NegativeText).color();
            lg = helper_.decoGradient(QRect(4,4,13,13), color);
            painter.drawPixmap(0, 0, helper_.windecoButtonFocused(bt, color,7));
        }
        else{
            QColor color = KColorScheme(pal.currentColorGroup()).decoration(KColorScheme::HoverColor).color();
            painter.drawPixmap(0, 0, helper_.windecoButtonFocused(bt, color, 7));
        }
    }
    else
        painter.drawPixmap(0, 0, helper_.windecoButton(bt));

    painter.setRenderHints(QPainter::Antialiasing);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(lg, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    switch(type_)
    {
        case ButtonSticky:
            painter.drawPoint(QPointF(10.5,10.5));
            break;
        case ButtonHelp:
            painter.translate(1.5, 1.5);
            painter.drawArc(7,5,4,4,135*16, -180*16);
            painter.drawArc(9,8,4,4,135*16,45*16);
            painter.drawPoint(9,12);
            break;
        case ButtonMin:
            painter.drawLine(QPointF( 7.5, 9.5), QPointF(10.5,12.5));
            painter.drawLine(QPointF(10.5,12.5), QPointF(13.5, 9.5));
            break;
        case ButtonMax:
            switch(client_.maximizeMode())
            {
                case OxygenClient::MaximizeRestore:
                case OxygenClient::MaximizeVertical:
                case OxygenClient::MaximizeHorizontal:
                    painter.drawLine(QPointF( 7.5,11.5), QPointF(10.5, 8.5));
                    painter.drawLine(QPointF(10.5, 8.5), QPointF(13.5,11.5));
                    break;
                case OxygenClient::MaximizeFull:
                {
                    painter.translate(1.5, 1.5);
                    //painter.setBrush(lg);
                    QPoint points[4] = {QPoint(9, 6), QPoint(12, 9), QPoint(9, 12), QPoint(6, 9)};
                    //QPoint points[4] = {QPoint(9, 5), QPoint(13, 9), QPoint(9, 13), QPoint(5, 9)};
                    painter.drawPolygon(points, 4);
                    break;
                }
            }
            break;
        case ButtonClose:
            painter.drawLine(QPointF( 7.5,7.5), QPointF(13.5,13.5));
            painter.drawLine(QPointF(13.5,7.5), QPointF( 7.5,13.5));
            break;
        case ButtonAbove:
            if(isChecked()) {
                QPen newPen = painter.pen();
                newPen.setColor(KColorScheme(pal.currentColorGroup()).decoration(KColorScheme::HoverColor).color());
                painter.setPen(newPen);
            }

            painter.drawLine(QPointF( 7.5,14), QPointF(10.5,11));
            painter.drawLine(QPointF(10.5,11), QPointF(13.5,14));
            painter.drawLine(QPointF( 7.5,10), QPointF(10.5, 7));
            painter.drawLine(QPointF(10.5, 7), QPointF(13.5,10));
            break;
        case ButtonBelow:
            if(isChecked()) {
                QPen newPen = painter.pen();
                newPen.setColor(KColorScheme(pal.currentColorGroup()).decoration(KColorScheme::HoverColor).color());
                painter.setPen(newPen);
            }

            painter.drawLine(QPointF( 7.5,11), QPointF(10.5,14));
            painter.drawLine(QPointF(10.5,14), QPointF(13.5,11));
            painter.drawLine(QPointF( 7.5, 7), QPointF(10.5,10));
            painter.drawLine(QPointF(10.5,10), QPointF(13.5, 7));
            break;
        default:
            break;
    }
}








} //namespace Oxygen
} //namespace Ozone
