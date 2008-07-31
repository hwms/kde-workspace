/*  Copyright 2007 by Alexis Ménard <darktears31@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
#ifndef NOTIFIERVIEW_H
#define NOTIFIERVIEW_H

// Qt
#include <QTreeView>

class QModelIndex;

namespace Notifier
{

  class NotifierView : public QTreeView
  {
  Q_OBJECT

  public:
      NotifierView(QWidget *parent = 0);
      virtual ~NotifierView();

      void setModel(QAbstractItemModel * model);

  public slots:
      void modelRowsRemoved(const QModelIndex &, int start, int end);

  protected:
      void resizeEvent(QResizeEvent * event);
      void mouseMoveEvent(QMouseEvent *event);
      void leaveEvent(QEvent *event);
      QModelIndex moveCursor(CursorAction cursorAction,Qt::KeyboardModifiers );
      void paintEvent(QPaintEvent *event);
      void rowsInserted(const QModelIndex & parent, int start, int end);

  private:
      QModelIndex m_hoveredIndex;
  };

}
#endif // NOTIFIERVIEW_H
