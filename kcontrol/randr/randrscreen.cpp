/*
 * Copyright (c) 2007      Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
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

#include <KDebug>
#include <KConfig>
#include <QX11Info>
#include <QAction>
#include "randrscreen.h"
#include "randrcrtc.h"
#include "randroutput.h"
#include "randrmode.h"

#ifdef HAS_RANDR_1_2
RandRScreen::RandRScreen(int screenIndex)
	: m_resources(0L)
{
	m_index = screenIndex;
	m_rect = QRect(0, 0, XDisplayWidth(QX11Info::display(), m_index), XDisplayHeight(QX11Info::display(), m_index));

	load();
	loadSettings();

	// select for randr input events
	int mask = RRScreenChangeNotifyMask | 
		   RRCrtcChangeNotifyMask | 
		   RROutputChangeNotifyMask | 
		   RROutputPropertyNotifyMask;
	XRRSelectInput(QX11Info::display(), rootWindow(), 0);
	XRRSelectInput(QX11Info::display(), rootWindow(), mask); 
}

RandRScreen::~RandRScreen()
{
	if (m_resources)
		XRRFreeScreenResources(m_resources);
}

int RandRScreen::index() const
{
	return m_index;
}

XRRScreenResources* RandRScreen::resources() const
{
	return m_resources;
}

Window RandRScreen::rootWindow() const
{
	return RootWindow(QX11Info::display(), m_index);
}

void RandRScreen::loadSettings()
{
	int minW, minH, maxW, maxH;

	Status status = XRRGetScreenSizeRange(QX11Info::display(), rootWindow(),
					 &minW, &minH, &maxW, &maxH);
	//FIXME: we should check the status here
	Q_UNUSED(status);
	m_minSize = QSize(minW, minH);
	m_maxSize = QSize(maxW, maxH);

	if (m_resources)
		XRRFreeScreenResources(m_resources);

	m_resources = XRRGetScreenResources(QX11Info::display(), rootWindow());
	Q_ASSERT(m_resources);

	if (RandR::timestamp != m_resources->timestamp)
		RandR::timestamp = m_resources->timestamp;

	// get all modes
	for (int i = 0; i < m_resources->nmode; ++i)
	{
		if (!m_modes.contains(m_resources->modes[i].id))
			m_modes[m_resources->modes[i].id] = RandRMode(&m_resources->modes[i]);

	}

	//get all crtcs
	for (int i = 0; i < m_resources->ncrtc; ++i)
	{
		if (m_crtcs.contains(m_resources->crtcs[i]))
			m_crtcs[m_resources->crtcs[i]]->loadSettings();
		else
		{
			RandRCrtc *c = new RandRCrtc(this, m_resources->crtcs[i]);
			connect(c, SIGNAL(crtcChanged(RRCrtc, int)), this, SIGNAL(configChanged()));
			connect(c, SIGNAL(crtcChanged(RRCrtc, int)), this, SLOT(save()));
			m_crtcs[m_resources->crtcs[i]] = c;
		}

	}

	//get all outputs
	for (int i = 0; i < m_resources->noutput; ++i)
	{
		if (m_outputs.contains(m_resources->outputs[i]))
			m_outputs[m_resources->outputs[i]]->loadSettings();
		else
		{
			RandROutput *o = new RandROutput(this, m_resources->outputs[i]);
			connect(o, SIGNAL(outputChanged(RROutput, int)), this, SLOT(slotOutputChanged(RROutput, int)));
			m_outputs[m_resources->outputs[i]] = o;
		}
	}

}

void RandRScreen::handleEvent(XRRScreenChangeNotifyEvent* event)
{
	m_rect.setWidth(event->width);
	m_rect.setHeight(event->height);

	emit configChanged();
}

void RandRScreen::handleRandREvent(XRRNotifyEvent* event)
{
	RandRCrtc *c;
	RandROutput *o;
	XRRCrtcChangeNotifyEvent *crtcEvent;
	XRROutputChangeNotifyEvent *outputEvent;
	XRROutputPropertyNotifyEvent *propertyEvent;

	// forward events to crtcs and outputs
	switch (event->subtype) {
		case RRNotify_CrtcChange:
			crtcEvent = (XRRCrtcChangeNotifyEvent*)event;
			c = crtc(crtcEvent->crtc);
			Q_ASSERT(c);
			c->handleEvent(crtcEvent);
			return;

		case RRNotify_OutputChange:
			outputEvent = (XRROutputChangeNotifyEvent*)event;
			o = output(outputEvent->output);
			Q_ASSERT(o);
			o->handleEvent(outputEvent);
			return;

		case RRNotify_OutputProperty:
			propertyEvent = (XRROutputPropertyNotifyEvent*)event;
			o = output(propertyEvent->output);
			Q_ASSERT(o);
			o->handlePropertyEvent(propertyEvent);
			return;
	}	
}

QSize RandRScreen::minSize() const
{
	return m_minSize;
}

QSize RandRScreen::maxSize() const
{
	return m_maxSize;
}

CrtcMap RandRScreen::crtcs() const
{
	return m_crtcs;
}

RandRCrtc* RandRScreen::crtc(RRCrtc id) const
{
	if (m_crtcs.contains(id))
		return m_crtcs[id];

	return NULL;
}

OutputMap RandRScreen::outputs() const
{
	return m_outputs;
}

RandROutput* RandRScreen::output(RROutput id) const
{
	if (m_outputs.contains(id))
		return m_outputs[id];

	return NULL;
}

ModeMap RandRScreen::modes() const
{
	return m_modes;
}

RandRMode RandRScreen::mode(RRMode id) const
{
	if (m_modes.contains(id))
		return m_modes[id];

	return RandRMode(0);
}

bool RandRScreen::adjustSize(QRect minimumSize)
{
	//try to find a size in which all outputs fit
	
	//start with a given minimum rect
	QRect rect = minimumSize;

	OutputMap::const_iterator it;
	for (it = m_outputs.constBegin(); it != m_outputs.constEnd(); ++it)
	{
		// outputs that are not active should not be taken into account
		// when calculating the screen size
		RandROutput *o = (*it);
		if (!o->isActive())
			continue;
		rect = rect.united(o->rect());
	}


	// check bounds
	if (rect.width() < m_minSize.width())
		rect.setWidth(m_minSize.width());
	if (rect.height() < m_minSize.height())
		rect.setHeight(m_minSize.height());

	if (rect.width() > m_maxSize.width())
		return false;
	if (rect.height() > m_maxSize.height())
		return false;

	return setSize(rect.size());
}

bool RandRScreen::setSize(QSize s)
{
	if (s == m_rect.size())
		return true;

	if (s.width() < m_minSize.width() || 
	    s.height() < m_minSize.height() ||
	    s.width() > m_maxSize.width() ||
	    s.height() > m_maxSize.height())
		return false;

	int widthMM, heightMM;
	float dpi;

	/* values taken from xrandr */
	dpi = (25.4 * DisplayHeight(QX11Info::display(), m_index)) / DisplayHeightMM(QX11Info::display(), m_index);
	widthMM =  (int) ((25.4 * s.width()) / dpi);
	heightMM = (int) ((25.4 * s.height()) / dpi);

	XRRSetScreenSize(QX11Info::display(), rootWindow(), s.width(), s.height(), widthMM, heightMM);
	m_rect.setSize(s);

	return true;
}

bool RandRScreen::outputsUnified() const
{
	return m_outputsUnified;
}

int RandRScreen::unifiedRotations() const
{

	bool first = true;
	int rotations = RandR::Rotate0;

	CrtcMap::const_iterator it;
	for (it = m_crtcs.constBegin(); it != m_crtcs.constEnd(); ++it)
	{
		if (!(*it)->connectedOutputs().count())
			continue;

		if (first)
		{
			rotations = (*it)->rotations();
			first = false;
		}
		else
			rotations &= (*it)->rotations();
	}

	return rotations;
}

SizeList RandRScreen::unifiedSizes() const
{
	SizeList sizeList;
	bool first = true;
	OutputMap::const_iterator it;

	foreach(RandROutput *output, m_outputs)
	{
		if (!output->isConnected())
			continue;

		if (first)
		{
			// we start using the list from the first output
			sizeList = output->sizes();
			first = false;
		}
		else
		{
			SizeList outputSizes = output->sizes();
			for (int i = sizeList.count() - 1; i >=0; --i)
			{
				// check if the current output has the i-th size of the sizeList
				// if not, remove from the list
				if (outputSizes.indexOf(sizeList[i]) == -1)
					sizeList.removeAt(i);
			}
		}
	}

	return sizeList;
}

QRect RandRScreen::rect() const

{
	return m_rect;
}

void RandRScreen::load(KConfig &config)
{
	KConfigGroup group = config.group("Screen_" + QString::number(m_index));
	m_outputsUnified = group.readEntry("OutputsUnified", true);
	m_unifiedRect = group.readEntry("UnifiedRect", QRect());
	m_unifiedRotation = group.readEntry("UnifiedRotation", (int) RandR::Rotate0);

	slotUnifyOutputs(m_outputsUnified);

	foreach(RandROutput *output, m_outputs)
	{
		if (output->isConnected())
			output->load(config);
	}

}

void RandRScreen::save(KConfig &config)
{
	KConfigGroup group = config.group("Screen_" + QString::number(m_index));
	group.writeEntry("OutputsUnified", m_outputsUnified);
	group.writeEntry("UnifiedRect", m_unifiedRect);
	group.writeEntry("UnifiedRotation", m_unifiedRotation);

	foreach(RandROutput *output, m_outputs)
	{
		if (output->isConnected())
			output->save(config);
	}
}

void RandRScreen::save()
{
	save(*KGlobal::config());
}

void RandRScreen::load()
{
	load(*KGlobal::config());
}

bool RandRScreen::applyProposed(bool confirm)
{
	bool succeed = true;
	QRect r;
	foreach(RandROutput *output, m_outputs)
	{
		r = output->rect();
		if (!output->applyProposed())
		{
			succeed = false;
			break;
		}
	}

	// if we could apply the config clean, ask for confirmation
	if (succeed && confirm)
		succeed = RandR::confirm(r);

	// if we succeded applying and the user confirmer the changes,
	// just return from here 
	if (succeed)
		return true;

	//Revert changes if not succeed
	foreach(RandROutput *o, m_outputs)
	{
		if (o->isConnected())
		{
			o->proposeOriginal();
			o->applyProposed();
		}
	}
	return false;
}

void RandRScreen::unifyOutputs()
{
	// iterate over all outputs and make sure all connected outputs get activated
	// and use the right size
	foreach(RandROutput *o, m_outputs)
	{
		// if the output is not connected we don't need to do anything
		if (!o->isConnected())
		       continue;

		// if the output is connected and already has the same rect and rotation
		// as the unified ones, continue
		if (o->isActive() && o->rect() == m_unifiedRect 
				  && o->rotation() == m_unifiedRotation)
			continue;

		o->proposeRect(m_unifiedRect);
		o->proposeRotation(m_unifiedRotation);
		o->applyProposed(RandR::ChangeSize | 
				 RandR::ChangePosition | 
				 RandR::ChangeRotation, false);
	}

	// FIXME: if by any reason we were not able to unify the outputs, we should 
	// do something
}

void RandRScreen::slotResizeUnified(QAction *action)
{
	m_unifiedRect.setSize(action->data().toSize()); 
	unifyOutputs();
}

void RandRScreen::slotUnifyOutputs(bool unified)
{
	m_outputsUnified = unified;

	if (!unified)
	{
		foreach(RandROutput *o, m_outputs)
		{
			if (o->isConnected())
				o->load(*KGlobal::config());
		}
	}
	else
	{
		SizeList sizes = unifiedSizes();

		if (!sizes.count())
		{
			// FIXME: this should be better handle
			return;
		}

		QSize s = m_unifiedRect.size();

		// if the last size we used is not available, use the first one
		// from the list
		if (sizes.indexOf(s) == -1)
			s = sizes[0];

		m_unifiedRect.setTopLeft(QPoint(0,0));
		m_unifiedRect.setSize(s);
		unifyOutputs();
	}
}

void RandRScreen::slotRotateUnified(QAction *action)
{
	int rotation = action->data().toInt(); 
	m_unifiedRotation = rotation;
	
	unifyOutputs();
}

void RandRScreen::slotOutputChanged(RROutput id, int changes)
{
	Q_UNUSED(id);
	Q_UNUSED(changes);

	if (changes & RandR::ChangeSize || changes & RandR::ChangePosition)
	{

		RandROutput *o = m_outputs[id];
		Q_ASSERT(o);

		// if we are using a unified layout, we should not allow an output
		// to use another position or size. So ask it to come back to the unified 
		// size.
		if (m_outputsUnified)
		{
			if (o->isConnected() && o->rect() != m_unifiedRect)
				unifyOutputs();
		}
		// TODO: handle the changes not to allow overlapping on non-unified 
		// setups
	}

	save();
	emit configChanged();
}

#include "randrscreen.moc"

#endif

