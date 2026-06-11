import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TestRequests 1.0

Rectangle {
    id: root

    // 0=Jan..11=Dec
    property int shownMonth: (new Date()).getMonth()
    property int shownYear: (new Date()).getFullYear()

    property date selectedDate: new Date()
    property var dayCounts: ({}) // "yyyy-MM-dd" -> count

    signal monthChanged(int year, int month0)
    signal dateClicked(date d)

    radius: Style.radiusLg
    color: Style.panel
    border.width: Style.borderWidth
    border.color: Style.border

    implicitHeight: layout.implicitHeight + Style.padLg * 2

    readonly property date today: new Date()
    readonly property var loc: Qt.locale()

    ListModel { id: calModel }

    function pad2(n) { return (n < 10 ? "0" : "") + n }
    function isoDate(d) { return d.getFullYear() + "-" + pad2(d.getMonth()+1) + "-" + pad2(d.getDate()) }

    // JS Date.getDay(): 0=Sun..6=Sat -> 0=Mon..6=Sun
    function weekdayMon0(d) { return (d.getDay() + 6) % 7 }

    function sameDay(a, b) {
        return a && b
            && a.getFullYear() === b.getFullYear()
            && a.getMonth() === b.getMonth()
            && a.getDate() === b.getDate()
    }

    function rebuild() {
        calModel.clear()

        var y0 = root.shownYear
        var m0 = root.shownMonth

        var first = new Date(y0, m0, 1)
        var offset = weekdayMon0(first)

        var start = new Date(y0, m0, 1 - offset)

        for (var i = 0; i < 42; ++i) {
            var d = new Date(start.getFullYear(), start.getMonth(), start.getDate() + i)
            calModel.append({
                yy: d.getFullYear(),
                mm: d.getMonth(),
                dayNum: d.getDate(),
                inMonth: (d.getMonth() === m0),
                iso: isoDate(d)
            })
        }
    }

    function animateMonthChange() {
        gridArea.opacity = 0.0
        rebuild()
        gridArea.opacity = 1.0
    }

    onShownMonthChanged: animateMonthChange()
    onShownYearChanged: animateMonthChange()
    Component.onCompleted: rebuild()

    function prevMonth() {
        if (shownMonth === 0) { shownMonth = 11; shownYear -= 1 }
        else shownMonth -= 1
        monthChanged(shownYear, shownMonth)
    }

    function nextMonth() {
        if (shownMonth === 11) { shownMonth = 0; shownYear += 1 }
        else shownMonth += 1
        monthChanged(shownYear, shownMonth)
    }

    ColumnLayout {
        id: layout
        anchors.fill: parent
        anchors.margins: Style.padLg
        spacing: Style.padSm

        RowLayout {
            Layout.fillWidth: true
            spacing: Style.padSm

            ToolButton {
                text: "chevron_left"
                font.family: Style.iconFontFamily
                font.pixelSize: 20
                onClicked: root.prevMonth()
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 1

                Label {
                    text: root.loc.monthName(root.shownMonth, Locale.LongFormat) + " " + root.shownYear
                    color: Style.text
                    font.family: Style.fontPrimaryBold
                    font.pixelSize: Style.fontLg
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }

                Label {
                    text: "Pick a date to see scheduled tests"
                    color: Style.subText
                    font.family: Style.fontSecondary
                    font.pixelSize: Style.fontSm
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }
            }

            ToolButton {
                text: "chevron_right"
                font.family: Style.iconFontFamily
                font.pixelSize: 20
                onClicked: root.nextMonth()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 0

            Repeater {
                model: 7
                delegate: Item {
                    Layout.fillWidth: true
                    height: 24
                    Label {
                        anchors.centerIn: parent
                        text: ["Mon","Tue","Wed","Thu","Fri","Sat","Sun"][index]
                        font.family: Style.fontSecondaryBold
                        font.pixelSize: Style.fontSm
                        color: Style.subText
                    }
                }
            }
        }

        Item {
            id: gridArea
            Layout.fillWidth: true
            Layout.preferredHeight: 320
            Layout.minimumHeight: 320

            readonly property int cols: 7
            readonly property int rows: 6
            readonly property int gap: 6

            readonly property real cellW: (width - (gap * (cols - 1))) / cols
            readonly property real cellH: (height - (gap * (rows - 1))) / rows

            // fade transition when month changes
            opacity: 1.0
            Behavior on opacity { NumberAnimation { duration: 140 } }

            Grid {
                id: grid
                anchors.fill: parent
                columns: gridArea.cols
                rowSpacing: gridArea.gap
                columnSpacing: gridArea.gap

                Repeater {
                    model: calModel

                    delegate: Item {
                        width: gridArea.cellW
                        height: gridArea.cellH

                        readonly property int yy: model.yy
                        readonly property int mm: model.mm
                        readonly property int dayNum: model.dayNum
                        readonly property bool inMonth: model.inMonth
                        readonly property string iso: model.iso

                        readonly property int count: (root.dayCounts && root.dayCounts[iso] !== undefined)
                                                    ? root.dayCounts[iso]
                                                    : 0

                        readonly property date d: new Date(yy, mm, dayNum)
                        readonly property bool isSelected: root.sameDay(d, root.selectedDate)
                        readonly property bool isToday: root.sameDay(d, root.today)

                        HoverHandler { id: hover }

                        Rectangle {
                            anchors.fill: parent
                            radius: Style.radiusSm
                            opacity: inMonth ? 1.0 : 0.35

                            color: isSelected
                                   ? Qt.rgba(Style.focus.r, Style.focus.g, Style.focus.b, 0.10)
                                   : hover.hovered
                                     ? Qt.rgba(Style.focus.r, Style.focus.g, Style.focus.b, 0.06)
                                     : "transparent"

                            border.width: 1
                            border.color: isSelected
                                         ? Qt.rgba(Style.focus.r, Style.focus.g, Style.focus.b, 0.55)
                                         : isToday
                                           ? Qt.rgba(Style.text.r, Style.text.g, Style.text.b, 0.14)
                                           : Qt.rgba(Style.border.r, Style.border.g, Style.border.b, 0.85)

                            Behavior on color { ColorAnimation { duration: 120 } }
                            Behavior on border.color { ColorAnimation { duration: 120 } }

                            Label {
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.margins: 8
                                text: dayNum
                                color: Style.text
                                font.family: isSelected ? Style.fontSecondaryBold : Style.fontSecondary
                                font.pixelSize: Style.fontSm
                            }

                            Rectangle {
                                visible: count > 0
                                width: 6; height: 6; radius: 3
                                anchors.left: parent.left
                                anchors.bottom: parent.bottom
                                anchors.margins: 8
                                color: Style.accent
                                opacity: inMonth ? 0.95 : 0.55
                            }

                            Rectangle {
                                visible: count > 0
                                height: 18
                                radius: 9
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: 6
                                color: Qt.rgba(Style.accent.r, Style.accent.g, Style.accent.b, 0.14)
                                border.width: 1
                                border.color: Qt.rgba(Style.accent.r, Style.accent.g, Style.accent.b, 0.35)
                                implicitWidth: badgeText.implicitWidth + 10

                                Label {
                                    id: badgeText
                                    anchors.centerIn: parent
                                    text: count
                                    font.family: Style.fontSecondaryBold
                                    font.pixelSize: 11
                                    color: Style.text
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.dateClicked(d)
                        }
                    }
                }
            }
        }

    }
}
