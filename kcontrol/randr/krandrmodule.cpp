/* 
 * Copyright (c) 2002 Hamish Rodda <meddie@yoyo.its.monash.edu.au>
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
 
#include <qbuttongroup.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <qlayout.h>
#include <qvbuttongroup.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qhbox.h>

#include <kdebug.h>
#include <klocale.h>
#include <kcmodule.h>
#include <kglobal.h>
#include <kgenericfactory.h>
#include <kcombobox.h>

#include "krandrmodule.h"

// DLL Interface for kcontrol
typedef KGenericFactory<KRandRModule, QWidget > KSSFactory;
K_EXPORT_COMPONENT_FACTORY (libkcm_randr, KSSFactory("randr") );


KRandRModule::KRandRModule(QWidget *parent, const char *name, const QStringList&)
    : KCModule(parent, name)
	, m_changed(false)
{
	if (!isValid()) {
		QVBoxLayout *topLayout = new QVBoxLayout(this);
		topLayout->addWidget(new QLabel(i18n("Sorry, your X server does not support resizing and rotating the display. Please update to version 4.3 or greater.  You need the X Resize And Rotate extension (RANDR) to use this feature.  Error code: %1").arg(errorCode()), this));
		return;
	}
	
	QVBoxLayout *topLayout = new QVBoxLayout(this);
	topLayout->setAutoAdd(true);

	QHBox* screenBox = new QHBox(this);
	new QLabel(i18n("Settings for screen: "), screenBox);
	m_screenSelector = new KComboBox(screenBox);

	for (int s = 0; s < m_numScreens; s++) {
		m_screenSelector->insertItem(i18n("Screen %1").arg(s+1));
	}

	m_screenSelector->setCurrentItem(m_currentScreenIndex);

	connect(m_screenSelector, SIGNAL(activated(int)), SLOT(slotScreenChanged(int)));
	
	if (m_screens.count() <= 1)
		m_screenSelector->setEnabled(false);
	
	m_sizeGroup = new QVButtonGroup(i18n("Screen Size"), this);
	
	m_rotationGroup = new QButtonGroup(2, Qt::Horizontal, i18n("Orientation (degrees anticlockwise)"), this);
	m_rotationGroup->setRadioButtonExclusive(true);
	
	QHBox* refreshBox = new QHBox(this);
	new QLabel(i18n("Refresh rate"), refreshBox);
	m_refreshRates = new KComboBox(refreshBox);
	connect(m_refreshRates, SIGNAL(activated(const QString&)), SLOT(slotRefreshChanged(const QString&)));
	
	slotScreenChanged(DefaultScreen(qt_xdisplay()));

	setButtons(KCModule::Default | KCModule::Apply);
}

void KRandRModule::addRotationButton(int thisRotation, bool checkbox)
{
	Q_ASSERT(m_rotationGroup);
	if (!checkbox) {
		QRadioButton* thisButton = new QRadioButton(RandRScreen::rotationName(thisRotation), m_rotationGroup);
		thisButton->setEnabled(thisRotation & m_currentScreen->rotations);
		connect(thisButton, SIGNAL(clicked()), SLOT(slotRotationChanged()));
	} else {
		QCheckBox* thisButton = new QCheckBox(RandRScreen::rotationName(thisRotation), m_rotationGroup);
		thisButton->setEnabled(thisRotation & m_currentScreen->rotations);
		connect(thisButton, SIGNAL(clicked()), SLOT(slotRotationChanged()));
	}
}

void KRandRModule::load()
{
	emit changed(false);
}

void KRandRModule::slotScreenChanged(int screen)
{
	m_currentScreenIndex = screen;
	m_currentScreen = m_screens.at(m_currentScreenIndex);
	Q_ASSERT(m_currentScreen);

	// Clear resolutions
	for (int i = m_sizeGroup->count() - 1; i >= 0; i--) {
		m_sizeGroup->remove(m_sizeGroup->find(i));
	}
	
	// Add new resolutions
	for (int i = 0; i < (int)m_currentScreen->sizes.count(); i++) {
		QRadioButton* thisButton = new QRadioButton(i18n("%1 x %2 (%3mm x %4mm)").arg(m_currentScreen->sizes[i].width).arg(m_currentScreen->sizes[i].height).arg(m_currentScreen->sizes[i].mwidth).arg(m_currentScreen->sizes[i].mheight), m_sizeGroup);
		m_sizeGroup->insert(thisButton);
		connect(thisButton, SIGNAL(clicked()), SLOT(slotSizeChanged()));
	}
	
	// Clear rotations
	for (int i = m_rotationGroup->count() - 1; i >= 0; i--) {
		m_rotationGroup->remove(m_rotationGroup->find(i));
	}
	
	// Create rotations
	for (int i = 0; i < 6; i++)
		addRotationButton(1 << i, i > 3);
	
	populateRefreshRates();
	
	update();
	
	setChanged();
}

void KRandRModule::slotRotationChanged()
{
	if (m_rotationGroup->find(0)->isOn())
		m_currentScreen->proposedRotation = RR_Rotate_0;
	else if (m_rotationGroup->find(1)->isOn())
		m_currentScreen->proposedRotation = RR_Rotate_90;
	else if (m_rotationGroup->find(2)->isOn())
		m_currentScreen->proposedRotation = RR_Rotate_180;
	else {
		Q_ASSERT(m_rotationGroup->find(3)->isOn());
		m_currentScreen->proposedRotation = RR_Rotate_270;
	}
	
	if (m_rotationGroup->find(4)->isOn())
		m_currentScreen->proposedRotation |= RR_Reflect_X;
	
	if (m_rotationGroup->find(5)->isOn())
		m_currentScreen->proposedRotation |= RR_Reflect_Y;
	
	setChanged();
}

void KRandRModule::slotSizeChanged()
{
	int oldProposed = m_currentScreen->proposedSize;
	
	m_currentScreen->proposedSize = static_cast<SizeID>(m_sizeGroup->id(m_sizeGroup->selected()));
	
	if (m_currentScreen->proposedSize != oldProposed)
		populateRefreshRates();
	
	setChanged();
}

void KRandRModule::slotRefreshChanged(const QString& rate)
{
	m_currentScreen->proposeRefreshRate(rate);
	setChanged();
}

void KRandRModule::populateRefreshRates()
{
	m_refreshRates->clear();
	
	QStringList rr = m_currentScreen->refreshRates(m_currentScreen->proposedSize);
	
	for (QStringList::Iterator it = rr.begin(); it != rr.end(); it++) {
		m_refreshRates->insertItem(*it);
	}
	
	if (rr.count() < 2)
		m_refreshRates->setEnabled(false);
	else
		m_refreshRates->setEnabled(true);
}


void KRandRModule::defaults()
{
	if (m_currentScreen->changedFromOriginal()) {
		m_currentScreen->proposeOriginal();
		m_currentScreen->applyProposed();
	} else {
		m_currentScreen->proposeOriginal();
	}
	
	update();
}

void KRandRModule::save()
{
	apply();
	setChanged();
}

void KRandRModule::setChanged()
{
	bool isChanged = false;
	
	for (RandRScreen* screen = m_screens.first(); screen; screen = m_screens.next()) {
		if (screen->proposedChanged()) {
			isChanged = true;
			break;
		}
	}
	
	if (isChanged != m_changed) {
		m_changed = isChanged;
		emit changed(m_changed);
	}
}

void KRandRModule::apply()
{
	if (m_changed) {
		for (RandRScreen* screen = m_screens.first(); screen; screen = m_screens.next()) {
			if (screen->proposedChanged()) {
				switch (screen->applyProposed()) {
					case RRSetConfigSuccess:
						if (!confirm()) {
							screen->proposeOriginal();
							screen->applyProposed();
						}
						break;
					case RRSetConfigInvalidConfigTime:
					case RRSetConfigInvalidTime:
					case RRSetConfigFailed:
						break;
				}
			}
		}
		
		update();
	}
}


void KRandRModule::update()
{
	m_sizeGroup->blockSignals(true);
	m_sizeGroup->setButton(m_currentScreen->proposedSize);
	m_sizeGroup->blockSignals(false);

	m_rotationGroup->blockSignals(true);
	switch (m_currentScreen->proposedRotation & 15) {
		case RR_Rotate_0:
			m_rotationGroup->setButton(0);
			break;
		case RR_Rotate_90:
			m_rotationGroup->setButton(1);
			break;
		case RR_Rotate_180:
			m_rotationGroup->setButton(2);
			break;
		case RR_Rotate_270:
			m_rotationGroup->setButton(3);
			break;
		default:
			// Shouldn't hit this one
			Q_ASSERT(m_currentScreen->proposedRotation & 15);
			break;
	}
	m_rotationGroup->find(4)->setDown(m_currentScreen->proposedRotation && RR_Reflect_X);
	m_rotationGroup->find(5)->setDown(m_currentScreen->proposedRotation && RR_Reflect_Y);
	m_rotationGroup->blockSignals(false);
	
	m_refreshRates->blockSignals(true);
	m_refreshRates->setCurrentItem(m_currentScreen->proposedRefreshRateIndex);
	m_refreshRates->blockSignals(false);
}

bool KRandRModule::confirm()
{
    // uncomment the line below and edit out the KTimerDialog stuff to get
    // a version which works on today's kdelibs (no accept dialog is presented)

    return true;

    /*KTimerDialog *acceptDialog = new KTimerDialog(
                                            15000,
                                            KTimerDialog::CountDown,
                                            this,
                                            "mainKTimerDialog",
                                            true,
                                            i18n("Confirm display setting change"),
                                            KTimerDialog::Ok|KTimerDialog::Cancel,
                                            KTimerDialog::Cancel);

    QLabel *label = new QLabel( i18n("Your screen orientation and size have "
                                     "been changed to the requested settings.\n"
                                     "Please indicate whether you wish to keep "
                                     "this configuration.\nIn 15 seconds your "
                                     "configuration will revert to normal."),
                                acceptDialog, "userSpecifiedLabel" );

    acceptDialog->setMainWidget( label );

    return acceptDialog->exec();*/
}
