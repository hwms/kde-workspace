/*
 * locale.cpp
 *
 * Copyright (c) 1998 Matthias Hoelzer (hoelzer@physik.uni-wuerzburg.de)
 * Copyright (c) 1999 Preston Brown <pbrown@kde.org>
 * Copyright (c) 1999 Hans Petter Bieker <bieker@pvv.ntnu.no>
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
#include <qdir.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qgroupbox.h>
#include <qobjectlist.h>

#include <kconfig.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstddirs.h>
#include <ksimpleconfig.h>

#include "klangcombo.h"
#include "klocalesample.h"
#include "locale.h"
#include "locale.moc"
#include "main.h"

#define i18n(a) (a)

KLocaleConfig::KLocaleConfig(QWidget *parent, const char *name)
  : KConfigWidget (parent, name)
{
    locale =  KGlobal::locale();

    QVBoxLayout *tl = new QVBoxLayout(this, 10, 10);
    QGroupBox *gbox = new QGroupBox(this, i18n("Locale"));
    tl->addWidget(gbox);

    QGridLayout *tl1 = new QGridLayout(gbox, 5, 4, 5);
    tl1->addRowSpacing(0, 15);
    tl1->addRowSpacing(4, 10);
    tl1->addColSpacing(0, 10);
    tl1->addColSpacing(3, 10);
    tl1->setColStretch(2, 1);

    changedFlag = FALSE;

    QLabel *label = new QLabel(gbox, i18n("&Country"));
    comboCountry = new KLanguageCombo(gbox);
    comboCountry->setFixedHeight(comboCountry->sizeHint().height());
    label->setBuddy(comboCountry);
    connect(comboCountry,SIGNAL(activated(int)),this,SLOT(changedCountry(int)));
    tl1->addWidget(label, 1, 1);
    tl1->addWidget(comboCountry, 1, 2);

    label = new QLabel(gbox, i18n("&Language"));
    comboLang = new KLanguageCombo(gbox);
    comboLang->setFixedHeight(comboLang->sizeHint().height());
    label->setBuddy(comboLang);
    connect(comboLang,SIGNAL(activated(int)),this,SLOT(changedLanguage(int)));
    tl1->addWidget(label, 2, 1);
    tl1->addWidget(comboLang, 2, 2);

    label = new QLabel(gbox, i18n("&Numbers"));
    comboNumber = new KLanguageCombo(gbox);
    comboNumber->setFixedHeight(comboNumber->sizeHint().height());
    label->setBuddy(comboNumber);
    connect(comboNumber,SIGNAL(activated(int)),this,SLOT(changedNumber(int)));
    tl1->addWidget(label, 3, 1);
    tl1->addWidget(comboNumber, 3, 2);

    label = new QLabel(gbox, i18n("&Money"));
    comboMoney = new KLanguageCombo(gbox);
    comboMoney->setFixedHeight(comboMoney->sizeHint().height());
    label->setBuddy(comboMoney);
    connect(comboMoney,SIGNAL(activated(int)),this,SLOT(changedMoney(int)));
    tl1->addWidget(label, 4, 1);
    tl1->addWidget(comboMoney, 4, 2);

    label = new QLabel(gbox, i18n("&Date and time"));
    comboDate = new KLanguageCombo(gbox);
    comboDate->setFixedHeight(comboDate->sizeHint().height());
    label->setBuddy(comboDate);
    connect(comboDate,SIGNAL(activated(int)),this,SLOT(changedTime(int)));
    tl1->addWidget(label, 5, 1);
    tl1->addWidget(comboDate, 5, 2);

    tl1->activate();

    gbox = new QGroupBox(this, i18n("Examples"));
    tl->addWidget(gbox);
    sample = new KLocaleSample(gbox);

    tl->addStretch(1);
    tl->activate();

    update();
    loadSettings();
}

KLocaleConfig::~KLocaleConfig ()
{
}

void KLocaleConfig::loadLocaleList(KLanguageCombo *combo, const QString &sub, const QStringList &first)
{
  QString name;

  // clear the list
  combo->clear();

  QStringList prilang;
  // add the primary languages for the country to the list
  for ( QStringList::ConstIterator it = first.begin(); it != first.end(); ++it )
    {
        QString str = locate("locale", sub + *it + "/entry.desktop");
        if (!str.isNull())
          prilang << str;
    }

  // add all languages to the list
  QStringList alllang = KGlobal::dirs()->findAllResources("locale",
							   sub + "*/entry.desktop");
  QStringList langlist = prilang;
  if (langlist.count() > 0)
    langlist << QString::null; // separator
  langlist += alllang;

  for ( QStringList::ConstIterator it = langlist.begin();
	it != langlist.end(); ++it )
    {
        if ((*it).isNull())
        {
	  combo->insertSeparator();
          continue;
        }
	KSimpleConfig entry(*it);
	entry.setGroup("KCM Locale");
	name = entry.readEntry("Name");
	if (name.isEmpty())
	    name = "without name!";
	
	QString path = *it;
	int index = path.findRev('/');
	path = path.left(index);
	index = path.findRev('/');
	path = path.mid(index+1);
	combo->insertLanguage(path, name, sub);
    }
}

void KLocaleConfig::loadSettings()
{
  loadLocaleList(comboCountry, "l10n/", QStringList());
  loadLocaleList(comboNumber, "l10n/", QStringList());
  loadLocaleList(comboMoney, "l10n/", QStringList());
  loadLocaleList(comboDate, "l10n/", QStringList());

  KConfig *config = KGlobal::config();
  config->setGroup("Locale");

  QString str = config->readEntry("Country");
  comboCountry->setCurrentItem(str);

  KSimpleConfig ent(locate("locale", "l10n/" + str + "/entry.desktop"), true);
  ent.setGroup("KCM Locale");
  QStringList langs = ent.readListEntry("Languages");
  if (langs.isEmpty()) langs = QString("C");

  loadLocaleList(comboLang, 0, langs);

  // Language
  str = config->readEntry("Language");
  str = str.left(str.find(':')); // for compatible -- FIXME in KDE3
  comboLang->setCurrentItem(str);

  // Numeric
  str = config->readEntry("Numeric");
  comboNumber->setCurrentItem(str);

  // Money
  str = config->readEntry("Monetary");
  comboMoney->setCurrentItem(str);

  // Date and time
  str = config->readEntry("Time");
  comboDate->setCurrentItem(str);
}

void KLocaleConfig::readLocale(const QString &path, QString &name, const QString &sub) const
{
  KSimpleConfig entry(locate("locale", sub + path + "/entry.desktop"));
  entry.setGroup("KCM Locale");
  name = entry.readEntry("Name", "without name");
}

void KLocaleConfig::applySettings()
{
  KConfigBase *config = KGlobal::config();

  config->setGroup("Locale");

  config->writeEntry("Country", comboCountry->currentTag());
  config->writeEntry("Language", comboLang->currentTag());
  config->writeEntry("Numeric", comboNumber->currentTag());
  config->writeEntry("Monetary", comboMoney->currentTag());
  config->writeEntry("Time", comboDate->currentTag());

  config->sync();

  if (changedFlag)
    KMessageBox::information(this,
			     i18n("Changed language settings apply only to newly started "
				  "applications.\nTo change the language of all "
				  "programs, you will have to logout first."),
                             i18n("Applying language settings"));

  changedFlag = FALSE;
}

void KLocaleConfig::defaultSettings()
{
  changedFlag = FALSE;

  locale->setLanguage("C",
                      "C", // FIXME -- this is obsoleted
                      "C",
                      "C",
                      "C");
  loadLocaleList(comboCountry, "l10n/", QStringList());
  loadLocaleList(comboLang, 0, QStringList());
  loadLocaleList(comboNumber, "l10n/", QStringList());
  loadLocaleList(comboMoney, "l10n/", QStringList());
  loadLocaleList(comboDate, "l10n/", QStringList());

  comboCountry->setCurrentItem("C");
  comboLang->setCurrentItem("C");
  comboNumber->setCurrentItem("C");
  comboMoney->setCurrentItem("C");
  comboDate->setCurrentItem("C");
  ((KLocaleApplication*)kapp)->reset();
}

void KLocaleConfig::changedCountry(int i)
{
  changedFlag = TRUE;

  QString country = comboCountry->tag(i);

  KSimpleConfig ent(locate("locale", "l10n/" + country + "/entry.desktop"), true);
  ent.setGroup("KCM Locale");
  QStringList langs = ent.readListEntry("Languages");
  if (langs.isEmpty()) langs = QString("C");

  locale->setLanguage(*langs.at(0),
                      *langs.at(0), // FIXME -- this is obsoleted
                      country,
                      country,
                      country);

  int j;
  QString name;
  for (j = 0; j < comboCountry->count(); j++)
  {
    readLocale(comboCountry->tag(j), name, "l10n/");
    comboCountry->changeLanguage(name, j);
    comboNumber->changeLanguage(name, j);
    comboMoney->changeLanguage(name, j);
    comboDate->changeLanguage(name, j);
  }

  loadLocaleList(comboLang, 0, langs);

  comboLang->setCurrentItem((*langs.at(0)).isNull()?QString::fromLatin1("C"):*langs.at(0));
  comboNumber->setCurrentItem(country);
  comboMoney->setCurrentItem(country);
  comboDate->setCurrentItem(country);

  update();
  ((KLocaleApplication*)kapp)->reset();
}

void KLocaleConfig::changedLanguage(int i)
{
  changedFlag = TRUE;

  locale->setLanguage(comboLang->tag(i),
                      comboLang->tag(i),
                      QString::null,
                      QString::null,
                      QString::null);
  int j;
  QString name;
  for (j = 0; j < comboCountry->count(); j++)
  {
    readLocale(comboCountry->tag(j), name, "l10n/");
    comboCountry->changeLanguage(name, j);
    comboNumber->changeLanguage(name, j);
    comboMoney->changeLanguage(name, j);
    comboDate->changeLanguage(name, j);
  }

  for (j = 0; j < comboLang->count(); j++)
  {
    readLocale(comboLang->tag(j), name, 0);
    comboLang->changeLanguage(name, j);
  }

  update();
}

void KLocaleConfig::changedNumber(int i)
{
  changedFlag = TRUE;

  locale->setLanguage(QString::null,
                      QString::null,
                      comboMoney->tag(i),
                      QString::null,
                      QString::null);
  ((KLocaleApplication*)kapp)->resetNum();
  update();
}

void KLocaleConfig::changedMoney(int i)
{
  changedFlag = TRUE;

  locale->setLanguage(QString::null,
                      QString::null,
                      QString::null,
                      comboDate->tag(i),
                      QString::null);
  ((KLocaleApplication*)kapp)->resetMon();
  update();
}

void KLocaleConfig::changedTime(int i)
{
  changedFlag = TRUE;

  locale->setLanguage(QString::null,
                      QString::null,
                      QString::null,
                      QString::null,
                      comboDate->tag(i));
  update();
}

void KLocaleConfig::update()
{
  sample->update();

  KLocaleApplication::reTranslate();
}
