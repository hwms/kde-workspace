/*
 *   Copyright (C) 2006 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "krunnerdialog.h"

#include <QPainter>
#include <QSvgRenderer>
#include <QResizeEvent>
#include <QMouseEvent>
#ifdef Q_WS_X11
#include <QX11Info>
#endif
#include <QBitmap>

#include <KDebug>
#include <KWindowSystem>
#include <KPluginInfo>
#ifdef Q_WS_X11
#include <NETRootInfo>
#endif

#include "kworkspace/kdisplaymanager.h"

#include <Plasma/AbstractRunner>
#include <Plasma/FrameSvg>
#include <Plasma/RunnerManager>
#include <Plasma/Theme>
#include <Plasma/WindowEffects>

#include "configdialog.h"
#include "krunnerapp.h"
#include "krunnersettings.h"

#ifdef Q_WS_X11
#include <X11/Xlib.h>
#endif

KRunnerDialog::KRunnerDialog(Plasma::RunnerManager *runnerManager, QWidget *parent, Qt::WindowFlags f)
    : QWidget(parent, f),
      m_runnerManager(runnerManager),
      m_configWidget(0),
      m_lastPressPos(-1),
      m_oldScreen(-1),
      m_floating(!KRunnerSettings::freeFloating()),
      m_resizing(false),
      m_rightResize(false),
      m_vertResize(false)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    //setButtons(0);
    setWindowTitle(i18n("Run Command"));
    setWindowIcon(KIcon("system-run"));

    QPalette pal = palette();
    pal.setColor(backgroundRole(), Qt::transparent);
    setPalette(pal);

    m_iconSvg = new Plasma::Svg(this);
    m_iconSvg->setImagePath("widgets/configuration-icons");

    m_background = new Plasma::FrameSvg(this);
    m_background->setImagePath("dialogs/krunner");
    setFreeFloating(KRunnerSettings::freeFloating());

    connect(Kephal::Screens::self(), SIGNAL(screenRemoved(int)),
            this, SLOT(screenRemoved(int)));
    connect(Kephal::Screens::self(), SIGNAL(screenResized(Kephal::Screen*,QSize,QSize)),
            this, SLOT(screenChanged(Kephal::Screen*)));
    connect(Kephal::Screens::self(), SIGNAL(screenMoved(Kephal::Screen*,QPoint,QPoint)),
            this, SLOT(screenChanged(Kephal::Screen*)));
    connect(KWindowSystem::self(), SIGNAL(workAreaChanged()), this, SLOT(resetScreenPos()));

    connect(m_background, SIGNAL(repaintNeeded()), this, SLOT(update()));

    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(themeUpdated()));
    themeUpdated();
}

KRunnerDialog::~KRunnerDialog()
{
    //kDebug( )<< "!!!!!!!!!! deleting" << m_floating << m_screenPos.count();
    if (!m_floating) {
        KConfigGroup cg(KGlobal::config(), "EdgePositions");
        QHashIterator<int, QPoint> it(m_screenPos);
        while (it.hasNext()) {
            it.next();
            //kDebug() << "saving" << "Screen" + QString::number(it.key()) << it.value();
            cg.writeEntry("Screen" + QString::number(it.key()), it.value());
        }
    }
}

void KRunnerDialog::screenRemoved(int screen)
{
    m_screenPos.remove(screen);
}

void KRunnerDialog::screenChanged(Kephal::Screen* screen)
{
    m_screenPos.remove(screen->id());
    if (m_oldScreen == screen->id()) {
        m_oldScreen = -1;
    }
}

void KRunnerDialog::resetScreenPos()
{
    if (!m_floating) {
        QMutableHashIterator<int, QPoint> it(m_screenPos);
        QRect r = KWindowSystem::workArea();
        while (it.hasNext()) {
            QPoint &p = it.next().value();

            if (r.left() > p.x()) {
                p.setX(r.left());
            } else if (r.right() < p.x() + width() - 1) {
                p.setX(r.right() - width());
            }

            p.setY(r.top());
        }

        m_oldScreen = -1;
        if (isVisible()) {
            positionOnScreen();
        }
    }
}

void KRunnerDialog::positionOnScreen()
{
    int screen = Kephal::ScreenUtils::primaryScreenId();
    if (Kephal::ScreenUtils::numScreens() > 1) {
        if (isVisible()) {
            screen = Kephal::ScreenUtils::screenId(geometry().center());
        } else {
            screen = Kephal::ScreenUtils::screenId(QCursor::pos());
        }
    }

    QRect r = Kephal::ScreenUtils::screenGeometry(screen);
    if (m_oldScreen != screen) {
        //kDebug() << "old screen be the new screen" << m_oldScreen << screen;
        if (m_oldScreen != -1) {
            m_screenPos.insert(m_oldScreen, pos());
        }

        m_oldScreen = screen;
        if (m_screenPos.contains(screen)) {
            //kDebug() << "moving to" << m_screenPos[screen];
            move(m_screenPos[screen]);
        } else {
            const int w = width();
            const int dx = r.left() + (r.width() / 2) - (w / 2);
            int dy = r.top();
            if (m_floating) {
                dy += r.height() / 3;
            }

            move(dx, dy);
        }
    }

    if (!m_floating) {
        checkBorders(r);
    }

    show();

    if (m_floating) {
        KWindowSystem::setOnDesktop(winId(), KWindowSystem::currentDesktop());
    } else {
        KWindowSystem::setOnAllDesktops(winId(), true);
        Plasma::WindowEffects::slideWindow(this, Plasma::TopEdge);
    }

    KWindowSystem::forceActiveWindow(winId());
    //kDebug() << "moving to" << m_screenPos[screen];
}

void KRunnerDialog::moveEvent(QMoveEvent *)
{
    m_screenPos.insert(m_oldScreen, pos());
}

void KRunnerDialog::setFreeFloating(bool floating)
{
    if (m_floating == floating) {
        return;
    }

    m_floating = floating;
    m_screenPos.clear();
    m_oldScreen = -1;
    unsetCursor();

    if (m_floating) {
        m_background->setEnabledBorders(Plasma::FrameSvg::AllBorders);
        KWindowSystem::setType(winId(), NET::Normal);
    } else {
        // load the positions for each screen from our config
        const int numScreens = Kephal::ScreenUtils::numScreens();
        KConfigGroup cg(KGlobal::config(), "EdgePositions");
        for (int i = 0; i < numScreens; ++i) {
            QPoint p = cg.readEntry("Screen" + QString::number(i), QPoint());
            if (!p.isNull()) {
                QRect r = Kephal::ScreenUtils::screenGeometry(i);
                m_screenPos.insert(i, QPoint(p.x(), r.top()));
            }
        }
        QRect r = Kephal::ScreenUtils::screenGeometry(m_oldScreen > -1 ? m_oldScreen : 0);
        checkBorders(r);
        KWindowSystem::setType(winId(), NET::Dock);
    }

    // recalc the contents margins
    themeUpdated();

    if (isVisible()) {
        positionOnScreen();
    }
}

bool KRunnerDialog::freeFloating() const
{
    return m_floating;
}

bool KRunnerDialog::isManualResizing() const
{
    return m_resizing;
}

void KRunnerDialog::setStaticQueryMode(bool staticQuery)
{
    Q_UNUSED(staticQuery)
}

void KRunnerDialog::switchUser()
{
    const KService::Ptr service = KService::serviceByStorageId("plasma-runner-sessions.desktop");
    KPluginInfo info(service);

    if (info.isValid()) {
        SessList sessions;
        KDisplayManager dm;
        dm.localSessions(sessions);

        if (sessions.isEmpty()) {
            // no sessions to switch between, let's just start up another session directly
            Plasma::AbstractRunner *sessionRunner = m_runnerManager->runner(info.pluginName());
            if (sessionRunner) {
                Plasma::QueryMatch switcher(sessionRunner);
                sessionRunner->run(*m_runnerManager->searchContext(), switcher);
            }
        } else {
            display(QString());
            //TODO: create a "single runner" mode
            //m_header->setText(i18n("Switch users"));
            //m_header->setPixmap("system-switch-user");

            //TODO: ugh, magic strings. See sessions/sessionrunner.cpp
            setStaticQueryMode(true);
            m_runnerManager->launchQuery("SESSIONS", info.pluginName());
        }
    }
}

void KRunnerDialog::toggleConfigDialog()
{
    if (m_configWidget) {
        delete m_configWidget;
        m_configWidget = 0;

        if (!m_floating) {
            KWindowSystem::setType(winId(), NET::Dock);
        }
    } else {
        m_configWidget = new KRunnerConfigWidget(m_runnerManager, this);
        connect(m_configWidget, SIGNAL(finished()), this, SLOT(configCompleted()));
        setConfigWidget(m_configWidget);
        KWindowSystem::setType(winId(), NET::Normal);
    }
}

void KRunnerDialog::configCompleted()
{
    if (m_configWidget) {
        m_configWidget->deleteLater();
        m_configWidget = 0;
    }

    if (!m_floating) {
        KWindowSystem::setType(winId(), NET::Dock);
    }
}

void KRunnerDialog::themeUpdated()
{
    m_leftBorderWidth = qMax(0, int(m_background->marginSize(Plasma::LeftMargin)));
    m_rightBorderWidth = qMax(0, int(m_background->marginSize(Plasma::RightMargin)));
    m_bottomBorderHeight = qMax(0, int(m_background->marginSize(Plasma::BottomMargin)));
    // the -2 in the non-floating case is not optimal, but it gives it a bit of a "more snug to the
    // top" feel; best would be if we could tell exactly where the edge/shadow of the frame svg was
    // but this works nicely
    const int topHeight = m_floating ? qMax(0, int(m_background->marginSize(Plasma::TopMargin)))
                                     : Plasma::Theme::defaultTheme()->windowTranslucencyEnabled() ?
                                         qMax(1, m_bottomBorderHeight / 2)
                                       : qMax(1, m_bottomBorderHeight - 2);

    //kDebug() << m_leftBorderWidth<< topHeight<< m_rightBorderWidth<< m_bottomBorderHeight;
    // the +1 gives us the extra mouseMoveEvent needed to always reset the resize cursor
    setContentsMargins(m_leftBorderWidth + 1, topHeight, m_rightBorderWidth + 1, m_bottomBorderHeight + 1);
}

void KRunnerDialog::paintEvent(QPaintEvent *e)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setClipRect(e->rect());
    //kDebug() << "clip rect set to: " << e->rect();

    m_background->paintFrame(&p);
}

bool KRunnerDialog::event(QEvent *event)
{
    if (event->type() == QEvent::Paint) {
        QPainter p(this);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.fillRect(rect(), Qt::transparent);
    }

    return QWidget::event(event);
}

void KRunnerDialog::showEvent(QShowEvent *)
{
    unsigned long state = NET::SkipTaskbar | NET::KeepAbove | NET::StaysOnTop;
    if (m_floating) {
        KWindowSystem::clearState(winId(), state);
    } else {
        KWindowSystem::setState(winId(), state);
    }
    m_runnerManager->setupMatchSession();
}

void KRunnerDialog::hideEvent(QHideEvent *)
{
    m_runnerManager->matchSessionComplete();
    delete m_configWidget;
    m_configWidget = 0;
}

void KRunnerDialog::updateMask()
{
#ifdef Q_WS_X11
    // Enable the mask only when compositing is disabled;
    // As this operation is quite slow, it would be nice to find some
    // way to workaround it for no-compositing users.

    if (!QX11Info::isCompositingManagerRunning()) {
        setMask(m_background->mask());
    } else {
        const QRegion mask = m_background->mask();
        Plasma::WindowEffects::enableBlurBehind(winId(), true, mask);
        Plasma::WindowEffects::overrideShadow(winId(), true);
    }

#else
    setMask(m_background->mask());
#endif
}

void KRunnerDialog::resizeEvent(QResizeEvent *e)
{
    m_background->resizeFrame(e->size());

    if (m_resizing && !m_vertResize && !m_floating) {
        QRect r = Kephal::ScreenUtils::screenGeometry(m_oldScreen);
        //kDebug() << "if" << x() << ">" << r.left() << "&&" << r.right() << ">" << (x() + width());
        const Plasma::FrameSvg::EnabledBorders borders = m_background->enabledBorders();
        if (borders & Plasma::FrameSvg::LeftBorder) {
            const int dx = x() + (e->oldSize().width() / 2) - (width() / 2);
            int dy = r.top();
            move(qBound(r.left(), dx, r.right() - width() + 1), dy);
            m_screenPos.insert(m_oldScreen, pos());
            if (!checkBorders(r)) {
                updateMask();
            }
        } else {
            updateMask();
        }
    } else {
        updateMask();
    }
}

void KRunnerDialog::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        m_vertResize = e->y() > height() - qMax(5, m_bottomBorderHeight);
        m_rightResize = e->x() > width() - qMax(5, m_rightBorderWidth);
        const bool leftResize = e->x() < qMax(5, m_leftBorderWidth);
        if (!m_floating && (leftResize || m_rightResize || m_vertResize)) {
            // let's do a resize! :)
            m_lastPressPos = m_vertResize ? e->globalY() : e->globalX();
            grabMouse();
            m_resizing = true;
        } else if (m_floating) {
#ifdef Q_WS_X11
            // We have to release the mouse grab before initiating the move operation.
            // Ideally we would call releaseMouse() to do this, but when we only have an
            // implicit passive grab, Qt is unaware of it, and will refuse to release it.
            XUngrabPointer(x11Info().display(), CurrentTime);

            // Ask the window manager to start an interactive move operation.
            NETRootInfo rootInfo(x11Info().display(), NET::WMMoveResize);
            rootInfo.moveResizeRequest(winId(), e->globalX(), e->globalY(), NET::Move);

#endif
        } else {
            grabMouse();
            m_lastPressPos = e->globalX();
        }

        e->accept();
    }
}

void KRunnerDialog::mouseReleaseEvent(QMouseEvent *)
{
    if (!m_floating) {
        releaseMouse();
        unsetCursor();
        m_lastPressPos = -1;
        m_resizing = false;
    }
}

bool KRunnerDialog::checkBorders(const QRect &screenGeom)
{
    Q_ASSERT(!m_floating);
    Plasma::FrameSvg::EnabledBorders borders = Plasma::FrameSvg::BottomBorder;

    if (x() > screenGeom.left()) {
        borders |= Plasma::FrameSvg::LeftBorder;
    }

    if (x() + width() < screenGeom.right()) {
        borders |= Plasma::FrameSvg::RightBorder;
    }

    if (borders != m_background->enabledBorders()) {
        m_background->setEnabledBorders(borders);
        themeUpdated();
        updateMask();
        update();
        return true;
    }

    return false;
}

void KRunnerDialog::leaveEvent(QEvent *)
{
    unsetCursor();
}

void KRunnerDialog::mouseMoveEvent(QMouseEvent *e)
{
    //kDebug() << e->x() << m_leftBorderWidth << width() << m_rightBorderWidth;
    if (m_floating) {
        return;
    }

    if (m_lastPressPos != -1) {
        if (m_resizing) {
            if (m_vertResize) {
                const int deltaY = e->globalY() - m_lastPressPos;
                resize(width(), height() + deltaY);
                m_lastPressPos = e->globalY();
            } else {
                QRect r = Kephal::ScreenUtils::screenGeometry(m_oldScreen);
                const int deltaX = (m_rightResize ? -1 : 1) * (m_lastPressPos - e->globalX());
                int newWidth = width() + deltaX;
                // don't let it grow beyond the opposite screen edge
                if (m_rightResize) {
                    if (m_leftBorderWidth > 0) {
                        newWidth += qMin(deltaX, x() - r.left());
                    }
                } else if (m_rightBorderWidth > 0) {
                    newWidth += qBound(0, deltaX, r.right() - (x() + width() - 1));
                } else if (newWidth > minimumWidth() && newWidth < width()) {
                    move(r.right() - newWidth + 1, y());
                }

                if (newWidth > minimumWidth()) {
                    resize(newWidth, height());
                    m_lastPressPos = e->globalX();
                }
            }
        } else {
            QRect r = Kephal::ScreenUtils::screenGeometry(m_oldScreen);
            int newX = qBound(r.left(), x() - (m_lastPressPos - e->globalX()), r.right() - width() + 1);
            if (abs(r.center().x() - (newX + (width() / 2))) < 20) {
                newX = r.center().x() - (width() / 2);
            } else {
                m_lastPressPos = e->globalX();
            }

            move(newX, y());
            checkBorders(r);
        }
    } else {
        checkCursor(e->pos());
    }
}

void KRunnerDialog::timerEvent(QTimerEvent *event)
{
    killTimer(event->timerId());
    if (checkCursor(mapFromGlobal(QCursor::pos()))) {
        startTimer(100);
    }
}

bool KRunnerDialog::checkCursor(const QPoint &pos)
{
    //Plasma::FrameSvg borders = m_background->enabledBoders();
    if ((m_leftBorderWidth > 0 && pos.x() < qMax(5, m_leftBorderWidth)) ||
        (m_rightBorderWidth > 0 && pos.x() > width() - qMax(5, m_rightBorderWidth))) {
        if (cursor().shape() != Qt::SizeHorCursor) {
            setCursor(Qt::SizeHorCursor);
            startTimer(100);
            return false;
        }

        return true;
    } else if ((pos.y() > height() - qMax(5, m_bottomBorderHeight)) && (pos.y() < height())) {
        if (cursor().shape() != Qt::SizeVerCursor) {
            setCursor(Qt::SizeVerCursor);
            startTimer(100);
            return false;
        }

        return true;
    }

    unsetCursor();
    return false;
}

#include "krunnerdialog.moc"
