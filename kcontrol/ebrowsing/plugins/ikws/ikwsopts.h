/*
 * Copyright (c) 2000 Yves Arrouye <yves@realnames.com>
 *
 * Requires the Qt widget libraries, available at no cost at
 * http://www.troll.no/
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


#ifndef __IKWSOPTS_H___
#define __IKWSOPTS_H___

#include <qtabwidget.h>
#include <qlayout.h>

#include <kcmodule.h>

#include <kuriikwsfiltereng.h>

class QCheckBox;
class QComboBox;
class QGroupBox;
class QPushButton;
class QLabel;
class QLineEdit;
class QListBox;
class QListView;
class QListViewItem;

class InternetKeywordsOptions : public KCModule {
    Q_OBJECT

public:
    InternetKeywordsOptions(QWidget *parent = 0, const char *name = 0);

    void load();
    void save();
    void defaults();

	QString quickHelp() const;

protected:

protected slots:
    void moduleChanged(bool state);

    void changeInternetKeywordsEnabled();
    void changeSearchKeywordsEnabled();
    void changeSearchFallback(const QString &name);

    void textChanged(const QString &);

    void changeSearchProvider();
    void deleteSearchProvider();
    void updateSearchProvider(QListViewItem *);

private:
    QListViewItem *displaySearchProvider(const KURISearchFilterEngine::SearchEntry &e, bool fallback = false);

    // Internet Keywords.
    QGroupBox *gb_keywords;
    QCheckBox *cb_enableInternetKeywords;
    QComboBox *cmb_searchFallback;
    QLabel *lb_searchFallback;

    //Search Engine keywords.
    QCheckBox *cb_enableSearchKeywords;
    QGroupBox *gb_search;
    QListView *lv_searchProviders;

    QLabel *lb_searchProviderName;
    QLineEdit *le_searchProviderName;

    QLabel *lb_searchProviderShortcuts;
    QLineEdit *le_searchProviderShortcuts;

    QLabel *lb_searchProviderURI;
    QLineEdit *le_searchProviderURI;

    QPushButton *pb_chgSearchProvider;
    QPushButton *pb_delSearchProvider;
};

#endif
