/*****************************************************************
ksmserver - the KDE session management server

Copyright (C) 2000 Matthias Ettrich <ettrich@kde.org>
******************************************************************/

#include <config.h>

#include "shutdown.h"
#include <qapplication.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qvbuttongroup.h>
#include <qlabel.h>
#include <qvbox.h>
#include <qpainter.h>
#include <qtimer.h>
#include <qstyle.h>
#include <qcursor.h>
#include <qmessagebox.h>
#include <qbuttongroup.h>

#include <klocale.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kpushbutton.h>
#include <kstdguiitem.h>
#include <kiconloader.h>
#include <kwin.h>

#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <stdlib.h>

#include <X11/Xlib.h>

#include "shutdown.moc"

KSMShutdownFeedback * KSMShutdownFeedback::s_pSelf = 0L;

KSMShutdownFeedback::KSMShutdownFeedback()
 : QWidget( 0L, "feedbackwidget", WType_Popup )
{
    setBackgroundMode( QWidget::NoBackground );
    setGeometry( QApplication::desktop()->geometry() );
}


void KSMShutdownFeedback::paintEvent( QPaintEvent* )
{
    QPainter p;
    QBrush b( Qt::Dense4Pattern );
    p.begin( this );
    p.fillRect( rect(), b);
    p.end();
}

//////

KSMShutdownDlg::KSMShutdownDlg( QWidget* parent,
  bool saveSession,
  bool maysd, bool maynuke,
  KApplication::ShutdownType sdtype, KApplication::ShutdownMode sdmode )
    : QDialog( parent, 0, TRUE, WType_Popup ) 
    // this is a WType_Popup on purpose. Do not change that! Not
    // having a popup here has severe side effects.
{
    QVBoxLayout* vbox = new QVBoxLayout( this );
    QFrame* frame = new QFrame( this );
    frame->setFrameStyle( QFrame::StyledPanel | QFrame::Raised );
#if QT_VERSION < 300
    frame->setLineWidth( style().defaultFrameWidth() );
#else
    frame->setLineWidth( style().pixelMetric( QStyle::PM_DefaultFrameWidth, frame ) );
#endif
    vbox->addWidget( frame );
    vbox = new QVBoxLayout( frame, 15, 5 );

    QString username;
    char buf[128];
    char *user = getlogin();
    struct utsname info; 
    gethostname(buf, 128);
    if (!user) user = getenv("LOGNAME");
    if (!user) username = ""; else username = QString("%1").arg(user);
    QLabel* label = new QLabel( i18n("End session for %1").arg(username), frame );
    QFont fnt = label->font();
    fnt.setBold( true );
    fnt.setPixelSize( fnt.pixelSize() * 3 / 2 );
    label->setFont( fnt );
    vbox->addWidget( label, 0, AlignHCenter );

    if (maysd)
    {
        QHBoxLayout* hbox = new QHBoxLayout( vbox );
	QLabel* icon = new QLabel( frame );
	icon->setPixmap( UserIcon( "shutdownkonq" ) );
	hbox->addWidget( icon, AlignCenter );
        QButtonGroup *tgrp = new QButtonGroup( frame );
	tgrp->setPaletteBackgroundColor( tgrp->colorGroup().midlight() );
	tgrp->setFrameStyle( QFrame::Panel | QFrame::Sunken );

	tgrp->setColumnLayout( 0, Qt::Vertical );
	tgrp->layout()->setSpacing( 6 );
	tgrp->layout()->setMargin( 11 );
	QGridLayout* grid = new QGridLayout( tgrp->layout() );
	grid->setAlignment( Qt::AlignTop );

	QLabel* whatNext = new QLabel( i18n("What do you want to do next?"), tgrp );
        rHalt = new QRadioButton( i18n("&Turn off computer"), tgrp );
        rReboot = new QRadioButton( i18n("&Restart computer"), tgrp );
        rLogout = new QRadioButton( i18n("&Login as different user"), tgrp );

	grid->addWidget( rReboot, 2, 1 );
	grid->addWidget( rLogout, 3, 1 );
	grid->addWidget( rHalt, 1, 1 );
	grid->addMultiCellWidget( whatNext, 0, 0, 0, 1 );
	QSpacerItem* spacer = new QSpacerItem( 20, 0, QSizePolicy::Expanding, QSizePolicy::Minimum );
	grid->addItem( spacer, 1, 0 );


        hbox->addWidget( tgrp, AlignTop );
        connect( tgrp, SIGNAL( clicked(int) ), SLOT( slotSdMode(int) ) );
#if 0
        mgrp = new QVButtonGroup( i18n("Shutdown mode"), frame );
        rSched = new QRadioButton( i18n("Sch&edule"), mgrp );
        if (maynuke)
            rForce = new QRadioButton( i18n("&Force Now"), mgrp );
        rTry = new QRadioButton( i18n("&Try Now"), mgrp );
        hbox->addWidget( mgrp, AlignTop );
#endif
    }

    checkbox = new QCheckBox( i18n("&Save session for future logins"), frame );
    vbox->addWidget( checkbox, 0, AlignCenter  );
    vbox->addStretch();

#if 0
    QFrame *line = new QFrame( frame );
    line->setFrameShape( QFrame::HLine );
    line->setFrameShadow( QFrame::Sunken );
    vbox->addWidget( line );
#endif

    QHBoxLayout* hbox = new QHBoxLayout( vbox );
    hbox->addStretch();
    KPushButton* yes = new KPushButton( maysd ?
                                         KStdGuiItem::ok() :
                                         KGuiItem( i18n( "&Logout" ) ),
                                        frame );
    connect( yes, SIGNAL( clicked() ), SLOT( accept() ) );
    yes->setDefault( TRUE );
    hbox->addWidget( yes );
    hbox->addStretch();
    KPushButton* cancel = new KPushButton( KStdGuiItem::cancel(), frame );
    connect( cancel, SIGNAL( clicked() ), SLOT( reject() ) );
    hbox->addWidget( cancel );
    hbox->addStretch();

    checkbox->setFocus();

    checkbox->setChecked( saveSession );
    if (maysd)
    {
        if (sdtype == KApplication::ShutdownTypeHalt)
            rHalt->setChecked( true );
        else if (sdtype == KApplication::ShutdownTypeReboot)
            rReboot->setChecked( true );
        else
            rLogout->setChecked( true );
	slotSdMode(0);

#if 0
        if (sdmode == KApplication::ShutdownModeSchedule)
            rSched->setChecked( true );
        else if (sdmode == KApplication::ShutdownModeTryNow)
            rTry->setChecked( true );
        else
            rForce->setChecked( true );
#endif
    }
}

void KSMShutdownDlg::slotSdMode(int)
{
#if 0
    mgrp->setEnabled( !rLogout->isChecked() );
#endif
}

bool KSMShutdownDlg::confirmShutdown( bool& saveSession,
				      bool maysd, bool maynuke,
				      KApplication::ShutdownType& sdtype, KApplication::ShutdownMode& sdmode )
{
    kapp->enableStyles();
    KSMShutdownDlg* l = new KSMShutdownDlg( 0,
					    //KSMShutdownFeedback::self(),
                                            saveSession,
					    maysd, maynuke, sdtype, sdmode );

    // Show dialog (will save the background in showEvent)
    QSize sh = l->sizeHint();
    QDesktopWidget *desktop = KApplication::desktop();
    QRect rect = desktop->screenGeometry(desktop->screenNumber(QCursor::pos()));
    l->move(rect.x() + (rect.width() - sh.width())/2,
    	    rect.y() + (rect.height() - sh.height())/2);
    bool result = l->exec();

    if (maysd)
    {
        sdtype = l->rHalt->isChecked()   ? KApplication::ShutdownTypeHalt :
                 l->rReboot->isChecked() ? KApplication::ShutdownTypeReboot :
                                           KApplication::ShutdownTypeNone;

#if 0
	sdmode = l->rSched->isChecked() ? KApplication::ShutdownModeSchedule :
                 l->rTry->isChecked()   ? KApplication::ShutdownModeTryNow :
                                          KApplication::ShutdownModeForceNow;
#endif
    }
    saveSession = l->checkbox->isChecked();

    delete l;

    kapp->disableStyles();
    return result;
}

