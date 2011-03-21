/*
 *   Copyright 2011 Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
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

import Qt 4.7
import org.kde.plasma.graphicswidgets 0.1 as PlasmaWidgets
import org.kde.plasma.core 0.1 as PlasmaCore

Item {
    id: main

    width: 250
    height: 200

    property string title: tooltipDialog.appletDelegate.title


    PlasmaWidgets.IconWidget {
        id: tooltipIconWidget
        anchors.left: parent.left
        anchors.top: parent.top
        width: 64
        height: 64
        icon: tooltipDialog.icon
    }
    Column {
        id: nameColumn
        anchors.left: tooltipIconWidget.right
        anchors.top: parent.top
        anchors.right: parent.right
        Text {
            color: theme.textColor
            text: title
            font.bold:true
            anchors.left: parent.left
            anchors.right: parent.right
            height: paintedHeight
            wrapMode: Text.Wrap
        }
        Text {
            text: tooltipDialog.description
            color: theme.textColor
            anchors.left: parent.left
            anchors.right: parent.right
            wrapMode: Text.Wrap
        }
    }
    Grid {
        anchors.top: tooltipIconWidget.bottom
        anchors.topMargin: 16
        rows: 2
        columns: 2
        Text {
            text: "License:"
            color: theme.textColor
            anchors.right: licenseText.left
            wrapMode: Text.Wrap
        }
        Text {
            id: licenseText
            text: tooltipDialog.license
            color: theme.textColor
            wrapMode: Text.Wrap
        }
        Text {
            text: "Author:"
            color: theme.textColor
            anchors.right: authorText.left
            wrapMode: Text.Wrap
        }
        Text {
            id: authorText
            text: tooltipDialog.author
            color: theme.textColor
            wrapMode: Text.Wrap
        }
    }
}
