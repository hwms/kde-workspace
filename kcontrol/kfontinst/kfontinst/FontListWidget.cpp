////////////////////////////////////////////////////////////////////////////////
//
// Class Name    : CFontListWidget
// Author        : Craig Drummond
// Project       : K Font Installer (kfontinst-kcontrol)
// Creation Date : 20/04/2001
// Version       : $Revision$ $Date$
//
////////////////////////////////////////////////////////////////////////////////
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
////////////////////////////////////////////////////////////////////////////////
// (C) Craig Drummond, 2001
////////////////////////////////////////////////////////////////////////////////

#include "FontListWidget.h"
#include "FontEngine.h"
#include "Config.h"
#include "XConfig.h"
#include "KfiGlobal.h"
#include "KfiCmModule.h"
#include "Misc.h"
#include <qgroupbox.h>
#include <qpushbutton.h>
#include <klocale.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <qlistview.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qbitmap.h>
#include <qpainter.h>

static const char * constFontOpenError = "ERROR: Could not open font";

class CDirectoryItem : public CFontListWidget::CListViewItem
{
    public:

    CDirectoryItem(CFontListWidget *listWidget, QStringList &list, QListView *parent, const QString &dir, const QString &name, const QString &icon)
        : CFontListWidget::CListViewItem(parent, name, CFontListWidget::CListViewItem::DIR),
          itsName(dir),
          itsParentDir(NULL),
          itsListWidget(listWidget),
          itsList(list)
    {
        if(QString::null!=icon)
            setPixmap(0, KGlobal::iconLoader()->loadIcon(icon, KIcon::Small));

        setOpen(0==itsList.count() || -1!=itsList.findIndex(fullName()) ? true : false);
    }

    CDirectoryItem(CFontListWidget *listWidget, QStringList &list, CDirectoryItem *parent, const QString &name)
        : CFontListWidget::CListViewItem(parent, name, CListViewItem::DIR),
          itsName(name),
          itsParentDir(parent),
          itsListWidget(listWidget),
          itsList(list)
    {
        bool readable=QDir(fullName()).isReadable();

        setPixmap(0, KGlobal::iconLoader()->loadIcon(readable ? "folder" : "folder_locked" , KIcon::Small));

        if(readable && -1!=itsList.findIndex(fullName()))
            setOpen(true);
    }

    virtual ~CDirectoryItem()
    {
    }

    void setup()
    {
        setExpandable(QDir(fullName()).isReadable() ? true : false);
        QListViewItem::setup();
    }

    void    setOpen(bool open);
    QString fullName() const;
    QString dir() const
    { 
        return fullName();
    }

    private:

    QString         itsName;
    CDirectoryItem  *itsParentDir;
    CFontListWidget *itsListWidget;
    QStringList     &itsList;
};

class CFontItem : public CFontListWidget::CListViewItem
{
    public:

    CFontItem(QListView *parent, const QString &name, EType type) 
        : CFontListWidget::CListViewItem(parent, name, CFontListWidget::CListViewItem::FONT)
    {
    }

    CFontItem(QListViewItem *parent, const QString &name, EType type)
        : CFontListWidget::CListViewItem(parent, name, CFontListWidget::CListViewItem::FONT)
    {
    }

    virtual ~CFontItem()
    {
    }

    protected:

    void setupDisplay();
};

class CAdvancedFontItem : public CFontItem
{
    public:
 
    CAdvancedFontItem(CDirectoryItem *parent, const QString &fileName)
        : CFontItem(parent, fileName, CFontListWidget::CListViewItem::FONT),
          itsParentDir(parent)
    {
        setupDisplay();
    }

    virtual ~CAdvancedFontItem()
    {
    }

    CDirectoryItem * getParentDir()
    {
        return itsParentDir;
    }

    QString dir() const
    {
        return itsParentDir->dir();
    }

    QString fullName() const
    {
        return itsParentDir->fullName()+"/"+text(0);
    }

    private:

    CDirectoryItem *itsParentDir;
};

class CBasicFontItem : public CFontItem
{
    public:

    CBasicFontItem(QListView *parent, const QString &fileName, const QString &path)
        : CFontItem(parent, fileName, CFontListWidget::CListViewItem::FONT),
          itsFileName(fileName),
          itsPath(path)
    {
        setupDisplay();
    }

    virtual ~CBasicFontItem()
    {
    }

    QString dir() const
    {
        return itsPath;
    }

    QString fullName() const
    {
        return itsPath+"/"+itsFileName;
    }

    private:

    QString itsFileName,
            itsPath;
};

QString CFontListWidget::CListViewItem::key(int column, bool ascending) const
{
    QString k;

    if(ascending)
        k=(itsType==DIR ? "1" : "2");
    else
        k=(itsType==DIR ? "2" : "1");

    k+=text(column);
    return k;
}

void CFontListWidget::CListViewItem::paintCell(QPainter *painter, const QColorGroup &colourGroup, int column, int width, int align)
{
    if(itsType==DIR && CKfiGlobal::xcfg().ok() && CKfiGlobal::xcfg().inPath(fullName()))
    {
        QFont f=painter->font();

        f.setBold(true);

        if(CKfiGlobal::xcfg().isUnscaled(fullName()))
            f.setItalic(true);

        painter->setFont(f);
    }

    QListViewItem::paintCell(painter, colourGroup, column, width, align);
}

void CDirectoryItem::setOpen(bool open)
{
    bool readable=true;

    if(NULL!=itsParentDir) // Then it's not a top level folder
        setPixmap(0, KGlobal::iconLoader()->loadIcon(open ? "folder_open" : "folder", KIcon::Small));

    if(open)
    {
        QDir dir(fullName());

        if(-1==itsList.findIndex(fullName()))
            itsList.append(fullName());

        if(!dir.isReadable())
        {
            readable=false;
            setExpandable(false);
        }
        else
        {
            const QFileInfoList *files=dir.entryInfoList();

            if(files)
            {
                QFileInfoListIterator it(*files);
                QFileInfo             *fInfo;

                itsListWidget->progressInit(i18n("Scanning folder %1:").arg(fullName()), files->count());
                for(; NULL!=(fInfo=it.current()); ++it)
                {
                    if("."!=fInfo->fileName() && ".."!=fInfo->fileName())
                    {
                        itsListWidget->progressShow(fInfo->fileName());
                        if(fInfo->isDir())
                            new CDirectoryItem(itsListWidget, itsList, this, fInfo->fileName());
                        else
                            if(CFontEngine::isAFont(fInfo->fileName().local8Bit()))
                                new CAdvancedFontItem(this, fInfo->fileName());
                    }
                }
                itsListWidget->progressStop();
            }
        }
    }
    else // Deleteing the items allows directories to be rescanned - although this may be slow if it has lots of fonts...
    {
        QListViewItem *item=firstChild();

        if(-1!=itsList.findIndex(fullName()))
            itsList.remove(fullName());

        while(NULL!=item)
        {
            QListViewItem *next=item->nextSibling();
            delete item;
            item=next;
        }
    }

    if(readable)
        QListViewItem::setOpen(open);
}

QString CDirectoryItem::fullName() const
{
    QString name;

    if(itsParentDir)
    {
        name=itsParentDir->fullName();
        name.append(itsName);
        name.append("/");
    }
    else
        name=itsName;

    return name;
}

void CFontItem::setupDisplay()
{
    switch(CFontEngine::getType(fullName().local8Bit()))
    {
        case CFontEngine::TRUE_TYPE:
            setPixmap(0, KGlobal::iconLoader()->loadIcon("font_truetype", KIcon::Small));
            break;
        case CFontEngine::TYPE_1:
            setPixmap(0, KGlobal::iconLoader()->loadIcon("font_type1", KIcon::Small));
            break;
        case CFontEngine::SPEEDO:
            setPixmap(0, KGlobal::iconLoader()->loadIcon("font_speedo", KIcon::Small));
            break;
        default:
        case CFontEngine::BITMAP:
            setPixmap(0, KGlobal::iconLoader()->loadIcon("font_bitmap", KIcon::Small));
            break;
    }

    if(CKfiGlobal::fe().openFont(fullName()))
    {
        setText(1, CKfiGlobal::fe().getFullName().latin1());
        CKfiGlobal::fe().closeFont();
    }
    else
        setText(1, constFontOpenError);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CFontListWidget::CFontListWidget(QWidget *parent, CConfig::EListWidget t, bool useSubDirs, bool showButton2Advanced,
                    const QString &boxLabel, const QString &button1Label, const QString &button2Label,
                    const QString &basicDir,
                    const QString &dir1, const QString &dir1Name, const QString &dir1Icon,
                    const QString &dir2, const QString &dir2Name, const QString &dir2Icon)
               : CFontListWidgetData(parent),
                 itsAdvancedMode(CKfiGlobal::cfg().getAdvancedMode()),
                 itsShowingProgress(false),
                 itsAdvancedData(dir1, dir1Name, dir1Icon, dir2, dir2Name, dir2Icon, showButton2Advanced),
                 itsBasicData(basicDir, useSubDirs),
                 itsBoxTitle(boxLabel),
                 itsType(t),
                 itsOpenItems(CKfiGlobal::cfg().getAdvancedDirs(t))
{
    itsBox->setTitle(boxLabel);
    itsButton1->setText(button1Label);
    itsButton1->setEnabled(false);
    itsButton2->setText(button2Label);
    itsList->setTreeStepSize(12);
}

void CFontListWidget::setAdvanced(bool on)
{
    if(on!=itsAdvancedMode)
    {
        itsAdvancedMode=on;
        scan();
    }
}

unsigned int CFontListWidget::getNumSelected(CListViewItem::EType type)
{
    unsigned int  num=0;
    CListViewItem *item=(CListViewItem *)(itsList->firstChild());
 
    while(NULL!=item)
    {
        if(item->isSelected() && item->getType()==type)
            num++;
        item=(CListViewItem *)(item->itemBelow());
    }
    return num;
}

void CFontListWidget::getNumSelected(int &numTT, int &numT1)
{
    CListViewItem *item=(CListViewItem *)itsList->firstChild();
 
    numTT=numT1=0;
 
    while(item!=NULL)
    {
        if(item->isSelected())
            if(CListViewItem::FONT==item->getType())
                if(CFontEngine::isATtf(item->text(0).local8Bit()))
                    numTT++;
                else
                    if(CFontEngine::isAType1(item->text(0).local8Bit()))
                        numT1++;
        item=(CListViewItem *)(item->itemBelow());
    }
}

static bool contains(QListViewItem *first, const QString &file)
{
    QListViewItem *item=first;

    while(item!=NULL)
    {
        if(item->text(0)==file)
            return true;

        item=item->nextSibling();
    }

    return false;
}

void CFontListWidget::addFont(const QString &path, const QString &file)
{
    if(itsAdvancedMode) // Need to find branch, and whether it's open...
    {
        CListViewItem *item=(CListViewItem *)(itsList->firstChild());
 
        while(NULL!=item)
        {
            if(item->getType()==CListViewItem::DIR)
                if(item->fullName()==path)
                {
                    if(item->isOpen() && !contains(item->firstChild(), file))
                        new CAdvancedFontItem((CDirectoryItem *)item, file) ;
                    break;
                }
            item=(CListViewItem *)(item->itemBelow());
        }
    }
    else
        if(!contains(itsList->firstChild(), file))
            new CBasicFontItem(itsList, file, path);
}

void CFontListWidget::addSubDir(const QString &top, const QString &sub)
{
    if(itsAdvancedMode) // Need to find branch, and whether it's open...
    {
        CListViewItem *item=(CListViewItem *)(itsList->firstChild());
 
        while(NULL!=item)
        {
            if(item->getType()==CListViewItem::DIR)
                if(item->fullName()==top)
                {
                    if(item->isOpen() && !contains(item->firstChild(), sub))
                        new CDirectoryItem(this, itsOpenItems, (CDirectoryItem *)item, sub);
                    break;
                }
            item=(CListViewItem *)(item->itemBelow());
        }
    }
}

static QListViewItem * locateItem(QListView *list, const QString &top)
{
    QListViewItem *item=list->firstChild();

    while(NULL!=item)
    {
        if(CFontListWidget::CListViewItem::DIR==((CFontListWidget::CListViewItem *)item)->getType() &&
           ((CFontListWidget::CListViewItem *)item)->fullName()==top)
            return item;

        item=item->itemBelow();
    }

    return NULL;
}

void CFontListWidget::scan()
{
    itsList->clear();

    itsButton1->setEnabled(false);
    if(itsAdvancedMode)
    {
        QListViewItem *item=NULL;

        itsList->setColumnText(0, "Folder/File");
        itsBox->setTitle(itsBoxTitle);
        if(itsAdvancedData.button2)
            itsButton2->show();
        else
            itsButton2->hide();

        addDir(itsAdvancedData.dir1, itsAdvancedData.dir1Name, itsAdvancedData.dir1Icon);
 
        if(QString::null!=itsAdvancedData.dir2)
            addDir(itsAdvancedData.dir2, itsAdvancedData.dir2Name, itsAdvancedData.dir2Icon);

        itsList->setEnabled(true);

        if(NULL!=(item=locateItem(itsList, CKfiGlobal::cfg().getAdvancedTopItem(itsType))))
            itsList->ensureItemVisible(item);
    }
    else
    {
        itsButton2->show();
        itsList->setColumnText(0, "File");
        itsBox->setTitle(itsBoxTitle + " " + itsBasicData.dir);
        scanDir(itsBasicData.dir);

        if(itsList->childCount())
            itsList->setEnabled(true);
        else
        {
            new QListViewItem(itsList, QString::null, i18n("This folder does not contain any fonts."));
            itsList->setEnabled(false);
        }
    }
}

void CFontListWidget::hideEvent(QHideEvent *event)
{
    saveListData();
    CFontListWidgetData::hideEvent(event);
}

void CFontListWidget::addDir(const QString &dir, const QString &name, const QString &icon)
{
    new CDirectoryItem(this, itsOpenItems, itsList, dir, name, icon);
}

void CFontListWidget::scanDir(const QString &dir, int sub)
{
    QDir d(dir);

    if(d.isReadable())
    {
        const QFileInfoList *files=d.entryInfoList();
 
        if(files)
        {
            QFileInfoListIterator it(*files);
            QFileInfo             *fInfo;

            if(0==sub && files->count())
                progressInit(i18n("Scanning folder %1:").arg(dir), 0);
 
            for(; NULL!=(fInfo=it.current()); ++it)
                if("."!=fInfo->fileName() && ".."!=fInfo->fileName())
                    if(fInfo->isDir())
                    {
                        if(itsBasicData.useSubDirs && sub<CMisc::MAX_SUB_DIRS)
                            scanDir(dir+fInfo->fileName()+"/", sub+1);
                    }
                    else
                        if(CFontEngine::isAType1(fInfo->fileName().local8Bit()) || CFontEngine::isATtf(fInfo->fileName().local8Bit()))
                        {
                            progressShow(fInfo->fileName());
                            new CBasicFontItem(itsList, fInfo->fileName(), dir);
                        }

            if(0==sub && files->count())
                progressStop();
        }
    }
}

void CFontListWidget::selectionChanged()
{
    CListViewItem *cur=(CListViewItem *)(itsList->currentItem());

    if(cur && cur->isSelected())
    {
        CListViewItem *item=(CListViewItem *)(itsList->firstChild());

        switch(cur->getType())
        {
            case CListViewItem::DIR:
                // De-select everything else...
                while(NULL!=item)
                {
                    if(item->isSelected() && item!=cur)
                    {
                        item->setSelected(false);
                        item->repaint();
                    }

                    item=(CListViewItem *)(item->itemBelow());
                }
                break;
            case CListViewItem::FONT:
                // Only allows fonts in the same dir to be selected...
                while(NULL!=item)
                {
                    if(item->isSelected() && item!=cur &&
                      (item->getType()==CListViewItem::DIR || item->parent()!=cur->parent()))
                    {
                        item->setSelected(false);
                        item->repaint();
                    }
                    item=(CListViewItem *)(item->itemBelow());
                }
                break;
        }

        if(!itsShowingProgress)
            if(cur->getType()==CListViewItem::DIR)
                emit directorySelected(cur->fullName());
            else
                emit fontSelected(cur->dir(), cur->text(0));
    }
    else
    {
        itsButton1->setEnabled(false);
        emit directorySelected(QString::null);
    }
}

CFontListWidget::CListViewItem * CFontListWidget::getFirstSelectedItem()
{
    CListViewItem *item=(CListViewItem *)itsList->firstChild();
 
    while(NULL!=item)
    {
        if(item->isSelected())
            return item;
 
        item=(CListViewItem *)(item->itemBelow());
    }

    return NULL;
}

void CFontListWidget::progressInit(const QString &title, int numSteps)
{
    static const int constMaxSteps=25;

    if(0==numSteps || numSteps>constMaxSteps)
    {
        itsShowingProgress=true;
        emit initProgress(title, numSteps);
    }
}

void CFontListWidget::progressShow(const QString &step)
{
    if(itsShowingProgress)
        emit progress(step);
}

void CFontListWidget::progressStop()
{
    if(itsShowingProgress)
    {
        emit stopProgress();
        itsShowingProgress=false;
    }
}

void CFontListWidget::saveListData()
{
    QListViewItem         *item=itsList->itemAt(QPoint(0, 0));
    QStringList::Iterator it;
 
    if(item && ((CListViewItem*)item)->fullName()!=CKfiGlobal::cfg().getAdvancedTopItem(itsType))
        CKfiGlobal::cfg().setAdvancedTopItem(itsType, ((CListViewItem*)item)->fullName());
 
    for(it=itsOpenItems.begin(); it!=itsOpenItems.end(); ++it)
        if(-1==CKfiGlobal::cfg().getAdvancedDirs(itsType).findIndex(*it))
        {
            CKfiGlobal::cfg().setAdvancedDirs(itsType, itsOpenItems);
            break;
        }
}

#include "FontListWidget.moc"
