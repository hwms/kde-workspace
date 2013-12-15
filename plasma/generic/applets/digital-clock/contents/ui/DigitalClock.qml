/*
 * Copyright 2013  Heena Mahour <heena393@gmail.com>
 * Copyright 2013 Sebastian Kügler <sebas@kde.org>
 * Copyright 2013 Martin Klapetek <mklapetek@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as Components

Item {
    id: main

    property int minimumWidth: time.paintedWidth
    property int maximumWidth: minimumWidth

    property int minimumHeight: time.paintedHeight
    property int maximumHeight: minimumHeight

    property int formFactor: plasmoid.formFactor
    property int timePixelSize: theme.defaultFont.pixelSize
    property int timezonePixelSize: theme.smallestFont.pixelSize

    property bool fillWidth: true
    property bool fillHeight: true

    property bool constrained: formFactor == PlasmaCore.Types.Vertical || formFactor == PlasmaCore.Types.Horizontal

    property bool vertical: plasmoid.formFactor == PlasmaCore.Types.Vertical

    onWidthChanged: geotimer.start()
    onHeightChanged: geotimer.start()

    Timer {
        id: geotimer
        interval: 10 // just to compress resize events of width and height
        onTriggered: updateSize()
    }

    Components.Label  {
        id: time
        font.bold: plasmoid.configuration.boldText
        font.italic: plasmoid.configuration.italicText
        font.pixelSize: Math.min(main.width/6, main.height)
        style: Text.Raised; styleColor: "black"
        width: Math.max(paintedWidth, time.paintedWidth)
        opacity: 0.8
        color: "white"
        // We need to adjust the timeformat a bit, see more at timeFormatCorrection(..) comments
        text: Qt.formatTime(dataSource.data["Local"]["Time"], timeFormatCorrection(Qt.locale().timeFormat(Locale.ShortFormat)));
        horizontalAlignment: main.AlignHCenter
        verticalAlignment: main.AlignVCenter
        anchors {
            centerIn: parent
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                plasmoid.expanded = !plasmoid.expanded;
                calTimer.start();
            }
            Timer {
                id: calTimer
                interval: 100
                onTriggered: {
                    if (calendarLoader.source == "") {
                        calendarLoader.source = "CalendarView.qml"
                    }
                }
            }
            PlasmaCore.ToolTip {
                id: tooltip
                target: time
                mainText: "Current Time"
                subText: Qt.formatDate(dataSource.data["Local"]["Date"],"dddd, MMM d yyyy") + "\n" + Qt.formatTime(dataSource.data["Local"]["Time"], Qt.DefaultLocaleShortDate)
                image: "preferences-system-time"
            }
        }
    }

    Components.Label {
        id: sizehelper
        text: time.text
        visible: false
    }

    function updateSize() {
        // Start at minimum size, and increase the hidden label
        // size until we fit into the plasmoid with a bit of margin
        // It would be nice if we had proper font metrics in QML.
        var maxSize = theme.smallestFont.pixelSize;
        var threshold = theme.mSize(theme.defaultFont).height / 2;
        var stop = false;

        while (!stop) {
            maxSize = maxSize + 1;
            sizehelper.font.pixelSize = maxSize;
            var pw = sizehelper.paintedWidth;

            if (!constrained) { // free floating cases
                if (maxSize + threshold >= main.height) {
                    stop = true;
                } else if (pw + threshold >= main.width) {
                    stop = true;
                }
            } else { // panel situations
                if (main.vertical) {
                    if (pw + threshold >= main.width) {
                        stop = true;
                    }
                } else {
                    if (maxSize + threshold >= main.height) {
                        stop = true;
                    }
                }
            }
        }
        time.font.pixelSize = maxSize;
    }

    // Qt's QLocale does not offer any modular time creating like Klocale did
    // eg. no "gimme time with seconds" or "gimme time without seconds and with timezone".
    // QLocale supports only two formats - Long and Short. Long is unusable in many situations
    // and Short does not provide seconds. So if seconds are enabled, we need to add it here.
    //
    // What happens here is that it looks for the delimiter between "h" and "m", takes it
    // and appends it after "mm" and then appends "ss" for the seconds. Also it checks
    // if the format string already does not contain the seconds part.
    function timeFormatCorrection(timeFormatString) {
        if (plasmoid.configuration.showSeconds && timeFormatString.indexOf('s') == -1) {
            timeFormatString = timeFormatString.replace(/(.*h)(.+)(mm)(.*)/gi,
                                                        function(match, firstPart, delimiter, secondPart, rest, offset, original) {
                return firstPart + delimiter + secondPart + delimiter + "ss" + rest
            });
        }

        //FIXME: this always appends the timezone part at the end, it should probably be
        //       Locale-driven, however QLocale does not provide any hint about where to
        //       put it
        if (plasmoid.configuration.showTimezone && timeFormatString.indexOf('t') == -1) {
            timeFormatString = timeFormatString + " t";
        }

        return timeFormatString;
    }
}