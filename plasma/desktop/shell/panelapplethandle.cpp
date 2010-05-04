/*
 * Copyright 20010 by Marco Martin <notmart@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "panelapplethandle.h"
#include "toolbutton.h"

#include <QApplication>
#include <QBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QTimer>

#include <KWindowSystem>

#include <Plasma/Applet>
#include <Plasma/Containment>
#include <Plasma/Corona>
#include <Plasma/Svg>
#include <Plasma/WindowEffects>

PanelAppletHandle::PanelAppletHandle(QWidget *parent, Qt::WindowFlags f)
    : Plasma::Dialog(parent, f)
{
    m_icons = new Plasma::Svg(this);
    m_icons->setImagePath("widgets/configuration-icons");
    KWindowSystem::setType(winId(), NET::Dock);
    setWindowFlags(Qt::X11BypassWindowManagerHint);
    hide();

    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    connect(m_hideTimer, SIGNAL(timeout()), this, SLOT(hide()));

    m_layout = new QBoxLayout(QBoxLayout::LeftToRight, this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_configureButton = new ToolButton(this);
    m_configureButton->setIcon(m_icons->pixmap("configure"));
    m_layout->addWidget(m_configureButton);
    connect(m_configureButton, SIGNAL(clicked()), this, SLOT(configureApplet()));

    m_layout->addStretch();
    m_title = new QLabel(this);
    m_layout->addWidget(m_title);
    m_layout->addStretch();

    m_closeButton = new ToolButton(this);
    m_closeButton->setIcon(m_icons->pixmap("close"));
    m_layout->addWidget(m_closeButton);
    connect(m_closeButton, SIGNAL(clicked()), this, SLOT(closeApplet()));

    m_moveAnimation = new QPropertyAnimation(this, "pos", this);

    m_layout->activate();
    resize(minimumSizeHint());
}

PanelAppletHandle::~PanelAppletHandle()
{
}

void PanelAppletHandle::setApplet(Plasma::Applet *applet)
{
    if (applet == m_applet.data()) {
        show();
        return;
    }

    m_applet = applet;
    m_hideTimer->stop();

    if (applet) {
        m_title->setText(applet->name());
        m_layout->activate();
        resize(minimumSizeHint());

        if (applet->formFactor() == Plasma::Vertical) {
            m_layout->setDirection(QBoxLayout::TopToBottom);
            m_title->hide();
        } else if (QApplication::layoutDirection() == Qt::RightToLeft) {
            m_layout->setDirection(QBoxLayout::RightToLeft);
            m_title->show();
        } else {
            m_layout->setDirection(QBoxLayout::LeftToRight);
            m_title->show();
        }
        if (isVisible()) {
            m_moveAnimation->setStartValue(pos());
            m_moveAnimation->setEndValue(applet->containment()->corona()->popupPosition(applet, size(), Qt::AlignCenter));
            m_moveAnimation->setDuration(250);
            m_moveAnimation->start();
        } else {
            move(applet->containment()->corona()->popupPosition(applet, size(), Qt::AlignCenter));
            Plasma::WindowEffects::slideWindow(this, applet->location());
            show();
        }
    }
}

void PanelAppletHandle::startHideTimeout()
{
    m_hideTimer->start(800);
    if (m_applet) {
        Plasma::WindowEffects::slideWindow(this, m_applet.data()->location());
    }
}

void PanelAppletHandle::configureApplet()
{
    if (m_applet) {
        m_applet.data()->showConfigurationInterface();
    }
}

void PanelAppletHandle::closeApplet()
{
    if (m_applet) {
        m_applet.data()->destroy();
    }
}

void PanelAppletHandle::enterEvent(QEvent *event)
{
    Q_UNUSED(event)

    m_hideTimer->stop();
}

void PanelAppletHandle::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)

    m_hideTimer->start(800);
}

void PanelAppletHandle::mousePressEvent(QMouseEvent *event)
{
    emit mousePressed(m_applet.data(), event);
}

void PanelAppletHandle::mouseMoveEvent(QMouseEvent *event)
{
    emit mouseMoved(m_applet.data(), event);
}

void PanelAppletHandle::mouseReleaseEvent(QMouseEvent *event)
{
    emit mouseReleased(m_applet.data(), event);
}


#include "panelapplethandle.moc"
