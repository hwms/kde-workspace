/*****************************************************************************
 *  This file is part of the KDE libraries                                    *
 *  Copyright (C) 2012 by Shaun Reich <shaun.reich@kdemail.net>               *
 *                                                                            *
 *  This library is free software; you can redistribute it and/or modify      *
 *  it under the terms of the GNU Lesser General Public License as published  *
 *  by the Free Software Foundation; either version 2 of the License or (at   *
 *  your option) any later version.                                           *
 *                                                                            *
 *  This library is distributed in the hope that it will be useful,           *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         *
 *  Library General Public License for more details.                          *
 *                                                                            *
 *  You should have received a copy of the GNU Lesser General Public License  *
 *  along with this library; see the file COPYING.LIB.                        *
 *  If not, see <http://www.gnu.org/licenses/>.                               *
 *****************************************************************************/

#include "svgviewer.h"

#include <QApplication>
#include <QStandardItemModel>

#include <KIconLoader>
#include <KIconTheme>
#include <KMenu>
#include <KStandardAction>
#include <KStringHandler>
#include <KAction>

SvgViewer::SvgViewer(QWidget* parent)
    : KDialog(parent)
{
    setWindowTitle(i18n("Plasma SVG Viewer"));
    QWidget* mainWidget = new QWidget(this);
    setMainWidget(mainWidget);
    setupUi(mainWidget);

    m_dataModel = new QStandardItemModel(this);

    connect(m_engines, SIGNAL(activated(QString)), this, SLOT(showEngine(QString)));

    connect(m_sourceRequesterButton, SIGNAL(clicked(bool)), this, SLOT(requestSource()));
    connect(m_serviceRequesterButton, SIGNAL(clicked(bool)), this, SLOT(requestServiceForSource()));
    m_data->setModel(m_dataModel);
    m_data->setWordWrap(true);

    addAction(KStandardAction::quit(qApp, SLOT(quit()), this));

    connect(m_data, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(showDataContextMenu(QPoint)));

    m_data->setContextMenuPolicy(Qt::CustomContextMenu);
    //connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(cleanUp()));
}

SvgViewer::~SvgViewer()
{
}

void SvgViewer::listEngines()
{
    m_engines->clear();
    KPluginInfo::List engines = m_engineManager->listEngineInfo(m_app);
    qSort(engines);

    foreach (const KPluginInfo engine, engines) {
        m_engines->addItem(KIcon(engine.icon()), engine.pluginName());
    }

    m_engines->setCurrentIndex(-1);
}

void SvgViewer::showEngine(const QString& name)
{
    m_dataModel->clear();
    m_dataModel->setColumnCount(4);
    QStringList headers;
    headers << i18n("DataSource") << i18n("Key") << i18n("Value") << i18n("Type");
    m_dataModel->setHorizontalHeaderLabels(headers);
    m_engine = 0;
    m_sourceCount = 0;

    if (!m_engineName.isEmpty()) {
        m_engineManager->unloadEngine(m_engineName);
    }

    m_engineName = name;
    if (m_engineName.isEmpty()) {
        updateTitle();
        return;
    }

    m_engine = m_engineManager->loadEngine(m_engineName);
    if (!m_engine) {
        m_engineName.clear();
        updateTitle();
        return;
    }

    m_sourceRequesterButton->setEnabled(true);
    m_updateInterval->setEnabled(true);
    m_sourceRequester->setEnabled(true);
    m_sourceRequester->setFocus();
    m_serviceRequester->setEnabled(true);
    m_serviceRequesterButton->setEnabled(true);
    updateTitle();
}

#include "svgviewer.moc"
