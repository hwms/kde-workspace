/*
 *   Copyright 2006 Aaron Seigo <aseigo@kde.org>
 *   Copyright 2007 Matt Broadstone <mbroadst@gmail.com>
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

// plasma.loadEngine("hardware")
// LineGraph graph
// plasma.connect(graph, "hardware", "cpu");

#include "plasmaapp.h"

#include <unistd.h>

#ifndef _SC_PHYS_PAGES
    #ifdef Q_OS_FREEBSD
    #include <sys/types.h>
    #include <sys/sysctl.h>
    #endif

    #ifdef Q_OS_NETBSD
    #include <sys/param.h>
    #include <sys/sysctl.h>
    #endif
#endif

#include <QApplication>
#include <QDesktopWidget>
#include <QPixmapCache>
#include <QtDBus/QtDBus>

//#include <KCrash>
#include <KDebug>
#include <KCmdLineArgs>
#include <KWindowSystem>
//#include <KAction>

//#include <ksmserver_interface.h>

#include <plasma/containment.h>
#include <plasma/theme.h>

#include "savercorona.h"
#include "saverview.h"

#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>

Display* dpy = 0;
Colormap colormap = 0;
Visual *visual = 0;

void checkComposite()
{
    dpy = XOpenDisplay(0); // open default display
    if (!dpy) {
        kError() << "Cannot connect to the X server" << endl;
        return;
    }

    int screen = DefaultScreen(dpy);
    int eventBase, errorBase;

    if (XRenderQueryExtension(dpy, &eventBase, &errorBase)) {
        int nvi;
        XVisualInfo templ;
        templ.screen  = screen;
        templ.depth   = 32;
        templ.c_class = TrueColor;
        XVisualInfo *xvi = XGetVisualInfo(dpy,
                                          VisualScreenMask | VisualDepthMask | VisualClassMask,
                                          &templ, &nvi);
        for (int i = 0; i < nvi; ++i) {
            XRenderPictFormat *format = XRenderFindVisualFormat(dpy, xvi[i].visual);
            if (format->type == PictTypeDirect && format->direct.alphaMask) {
                visual = xvi[i].visual;
                colormap = XCreateColormap(dpy, RootWindow(dpy, screen), visual, AllocNone);
                break;
            }
        }
    }

    kDebug() << (colormap ? "Plasma has an argb visual" : "Plasma lacks an argb visual") << visual << colormap;
    kDebug() << ((KWindowSystem::compositingActive() && colormap) ? "Plasma can use COMPOSITE for effects"
                                                                    : "Plasma is COMPOSITE-less") << "on" << dpy;
}

PlasmaApp* PlasmaApp::self()
{
    if (!kapp) {
        checkComposite();
        return new PlasmaApp(dpy, visual ? Qt::HANDLE(visual) : 0, colormap ? Qt::HANDLE(colormap) : 0);
    }

    return qobject_cast<PlasmaApp*>(kapp);
}

PlasmaApp::PlasmaApp(Display* display, Qt::HANDLE visual, Qt::HANDLE colormap)
    : KUniqueApplication(display, visual, colormap),
      m_corona(0),
      m_view(0)
{
    kDebug();
    //FIXME what's this?
    KGlobal::locale()->insertCatalog("libplasma");

//    new PlasmaAppAdaptor(this);
//    QDBusConnection::sessionBus().registerObject("/App", this);

    //FIXME this is probably totally invalid
    // Enlarge application pixmap cache
    // Calculate the size required to hold background pixmaps for all screens.
    // Add 10% so that other (smaller) pixmaps can also be cached.
    int cacheSize = 0;
    QDesktopWidget *desktop = QApplication::desktop();
    for (int i = 0; i < desktop->numScreens(); i++) {
        QRect geometry = desktop->screenGeometry(i);
        cacheSize += 4 * geometry.width() * geometry.height() / 1024;
    }
    cacheSize += cacheSize / 10;

    // Calculate the size of physical system memory; _SC_PHYS_PAGES *
    // _SC_PAGESIZE is documented to be able to overflow 32-bit integers,
    // so apply a 10-bit shift. FreeBSD 6-STABLE doesn't have _SC_PHYS_PAGES
    // (it is documented in FreeBSD 7-STABLE as "Solaris and Linux extension")
    // so use sysctl in those cases.
#if defined(_SC_PHYS_PAGES)
    int memorySize = sysconf(_SC_PHYS_PAGES);
    memorySize *= sysconf(_SC_PAGESIZE) / 1024;
#else
#ifdef Q_OS_FREEBSD
    int sysctlbuf[2];
    size_t size = sizeof(sysctlbuf);
    int memorySize;
    // This could actually use hw.physmem instead, but I can't find
    // reliable documentation on how to read the value (which may 
    // not fit in a 32 bit integer).
    if (!sysctlbyname("vm.stats.vm.v_page_size", sysctlbuf, &size, NULL, 0)) {
        memorySize = sysctlbuf[0] / 1024;
        size = sizeof(sysctlbuf);
        if (!sysctlbyname("vm.stats.vm.v_page_count", sysctlbuf, &size, NULL, 0)) {
            memorySize *= sysctlbuf[0];
        }
    }
#endif
#ifdef Q_OS_NETBSD
    size_t memorySize;
    size_t len;
    static int mib[] = { CTL_HW, HW_PHYSMEM };

    len = sizeof(memorySize);
    sysctl(mib, 2, &memorySize, &len, NULL, 0);
    memorySize /= 1024;
#endif
    // If you have no suitable sysconf() interface and are not FreeBSD,
    // then you are out of luck and get a compile error.
#endif

    // Increase the pixmap cache size to 1% of system memory if it isn't already
    // larger so as to maximize cache usage. 1% of 1GB ~= 10MB.
    if (cacheSize < memorySize / 100) {
        cacheSize = memorySize / 100;
    }

    kDebug() << "Setting the pixmap cache size to" << cacheSize << "kilobytes";
    QPixmapCache::setCacheLimit(cacheSize);

    KConfigGroup cg(KGlobal::config(), "General");
    Plasma::Theme::defaultTheme()->setFont(cg.readEntry("desktopFont", font()));

    //FIXME the flag, it does nothing!
    setIsFullscreen(KCmdLineArgs::parsedArgs()->isSet("fullscreen"));

    // this line initializes the corona.
    corona();

    connect(this, SIGNAL(aboutToQuit()), this, SLOT(cleanup()));
}

PlasmaApp::~PlasmaApp()
{
    //TODO: This manual sync() should not be necessary. Remove it when
    // KConfig was fixed
    KGlobal::config()->sync();
}

void PlasmaApp::cleanup()
{
    if (m_corona) {
        m_corona->saveLayout();
    }

    delete m_view;
    delete m_corona;
}

void PlasmaApp::setIsFullscreen(bool isFullscreen)
{
    m_isFullscreen = isFullscreen;

    if (isFullscreen) {
        connect(QApplication::desktop(), SIGNAL(resized(int)), SLOT(adjustSize(int)));
    } else {
        disconnect(QApplication::desktop(), SIGNAL(resized(int)), this, SLOT(adjustSize(int)));
    }
}

bool PlasmaApp::isFullscreen() const
{
    return m_isFullscreen;
}

void PlasmaApp::adjustSize(int screen)
{
    if (! m_view) {
        return;
    }
    //FIXME someone needs to tell us what size to use if we've got >1 screen
    QDesktopWidget *desktop = QApplication::desktop();
    QRect geom = desktop->screenGeometry(0);
    m_view->setGeometry(geom);
}

//FIXME we need our own corona class
Plasma::Corona* PlasmaApp::corona()
{
    if (!m_corona) {
        SaverCorona *c = new SaverCorona(this);
        connect(c, SIGNAL(containmentAdded(Plasma::Containment*)),
                this, SLOT(createView(Plasma::Containment*)));
        kDebug() << "connected to containmentAdded";
        /*
        foreach (DesktopView *view, m_desktops) {
            connect(c, SIGNAL(screenOwnerChanged(int,int,Plasma::Containment*)),
                            view, SLOT(screenOwnerChanged(int,int,Plasma::Containment*)));
        }*/

        c->setItemIndexMethod(QGraphicsScene::NoIndex);
        c->initializeLayout();
        kDebug() << "layout should exist";
        //c->checkScreens();
        m_corona = c;
    }

    return m_corona;
}

bool PlasmaApp::hasComposite()
{
    return colormap && KWindowSystem::compositingActive();
}

//I think we need this for when the corona loads the default setup
//but maybe something simpler would suffice
void PlasmaApp::createView(Plasma::Containment *containment)
{
    kDebug() << "Containment name:" << containment->name()
             << "| type" << containment->containmentType()
             <<  "| screen:" << containment->screen()
             << "| geometry:" << containment->geometry()
             << "| zValue:" << containment->zValue();

    if (m_view) {
        // we already have a view for this screen
        return;
    }

    kDebug() << "creating a view for" << containment->screen() << "and we have"
        << QApplication::desktop()->numScreens() << "screens";

    // we have a new screen. neat.
    m_view = new SaverView(containment, 0);
                /*if (m_corona) {
                    connect(m_corona, SIGNAL(screenOwnerChanged(int,int,Plasma::Containment*)),
                            view, SLOT(screenOwnerChanged(int,int,Plasma::Containment*)));
                }*/
    //FIXME is this the right geometry for multi-screen?
    m_view->setGeometry(QApplication::desktop()->screenGeometry(containment->screen()));
    m_view->showView();
}


#include "plasmaapp.moc"
