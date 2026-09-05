/*
* Audacity: A Digital Audio Editor
*
* M3DatePicker
*
* A Material 3 docked date picker: a month grid with a header, month
* navigation, today marker and a fully rounded selection indicator. Emits the
* chosen date as a JavaScript Date.
*
* API:
*     selectedDate, displayedMonth, minimumDate, maximumDate,
*     dateSelected(date), navigationPanel
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

M3Surface {
    id: root

    property date selectedDate: new Date()
    property date displayedMonth: new Date()

    property var minimumDate: undefined
    property var maximumDate: undefined

    property NavigationPanel navigationPanel: null

    signal dateSelected(date value)

    level: 3
    radius: M3.shape.large

    implicitWidth: 328
    implicitHeight: 456

    readonly property var weekdayNames: ["M", "T", "W", "T", "F", "S", "S"]

    readonly property int displayedYear: root.displayedMonth.getFullYear()
    readonly property int displayedMonthIndex: root.displayedMonth.getMonth()

    // Monday is the first column, matching the weekday header above.
    readonly property int leadingBlanks: {
        var first = new Date(root.displayedYear, root.displayedMonthIndex, 1)
        return (first.getDay() + 6) % 7
    }

    readonly property int daysInMonth: new Date(root.displayedYear,
                                                root.displayedMonthIndex + 1, 0).getDate()

    readonly property string monthLabel: Qt.formatDate(root.displayedMonth, "MMMM yyyy")

    function isSameDay(a, b) {
        return a.getFullYear() === b.getFullYear()
                && a.getMonth() === b.getMonth()
                && a.getDate() === b.getDate()
    }

    function dateFor(day) {
        return new Date(root.displayedYear, root.displayedMonthIndex, day)
    }

    function isSelectable(day) {
        var candidate = root.dateFor(day)
        if (root.minimumDate !== undefined && candidate < root.minimumDate) {
            return false
        }
        if (root.maximumDate !== undefined && candidate > root.maximumDate) {
            return false
        }
        return true
    }

    function shiftMonth(delta) {
        root.displayedMonth = new Date(root.displayedYear, root.displayedMonthIndex + delta, 1)
    }

    Column {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        Item {
            width: parent.width
            height: 56

            StyledTextLabel {
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignLeft
                text: root.monthLabel
                font: M3.typography.titleMedium
                color: M3.color.onSurfaceVariant
            }

            Row {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: 0

                M3IconButton {
                    icon: IconCode.SMALL_ARROW_LEFT
                    accessibleName: "Previous month"
                    navigation.panel: root.navigationPanel
                    navigation.row: 0
                    navigation.column: 0
                    onClicked: root.shiftMonth(-1)
                }

                M3IconButton {
                    icon: IconCode.SMALL_ARROW_RIGHT
                    accessibleName: "Next month"
                    navigation.panel: root.navigationPanel
                    navigation.row: 0
                    navigation.column: 1
                    onClicked: root.shiftMonth(1)
                }
            }
        }

        Row {
            width: parent.width

            Repeater {
                model: root.weekdayNames

                delegate: StyledTextLabel {
                    required property var modelData

                    width: (root.width - 24) / 7
                    height: 32
                    text: modelData
                    font: M3.typography.bodySmall
                    color: M3.color.onSurface
                }
            }
        }

        Grid {
            id: grid

            width: parent.width
            columns: 7

            Repeater {
                model: root.leadingBlanks

                delegate: Item {
                    width: (root.width - 24) / 7
                    height: 44
                }
            }

            Repeater {
                id: dayRepeater

                model: root.daysInMonth

                delegate: Item {
                    id: dayCell

                    required property int index

                    readonly property int day: dayCell.index + 1
                    readonly property date cellDate: root.dateFor(dayCell.day)
                    readonly property bool selected: root.isSameDay(dayCell.cellDate, root.selectedDate)
                    readonly property bool today: root.isSameDay(dayCell.cellDate, new Date())
                    readonly property bool selectable: root.isSelectable(dayCell.day)

                    width: (root.width - 24) / 7
                    height: 44

                    NavigationControl {
                        id: dayNav

                        name: "M3DatePickerDay" + dayCell.day
                        panel: root.navigationPanel
                        row: 1 + Math.floor((root.leadingBlanks + dayCell.index) / 7)
                        column: (root.leadingBlanks + dayCell.index) % 7
                        enabled: dayCell.selectable && root.enabled && root.visible
                        accessible.role: MUAccessible.ListItem
                        accessible.name: Qt.formatDate(dayCell.cellDate, "d MMMM yyyy")
                        accessible.selected: dayCell.selected
                        accessible.visualItem: dayCircle

                        onTriggered: {
                            root.selectedDate = dayCell.cellDate
                            root.dateSelected(dayCell.cellDate)
                        }
                    }

                    Rectangle {
                        id: dayCircle

                        anchors.centerIn: parent
                        width: 40
                        height: 40
                        radius: 20
                        antialiasing: true

                        color: dayCell.selected ? M3.color.primary : "transparent"
                        border.width: dayCell.today && !dayCell.selected ? 1 : 0
                        border.color: M3.color.primary

                        M3StateLayer {
                            anchors.fill: parent
                            radius: dayCircle.radius
                            color: dayCell.selected ? M3.color.onPrimary : M3.color.onSurface
                            active: dayCell.selectable
                            hovered: dayMouse.containsMouse
                            pressed: dayMouse.containsPress
                            focused: dayNav.highlight
                        }

                        StyledTextLabel {
                            anchors.centerIn: parent
                            text: String(dayCell.day)
                            font: M3.typography.bodyLarge
                            color: {
                                if (!dayCell.selectable) {
                                    return Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g,
                                                   M3.color.onSurface.b, M3.stateLayer.disabledContent)
                                }
                                return dayCell.selected ? M3.color.onPrimary : M3.color.onSurface
                            }
                        }
                    }

                    M3FocusRing {
                        anchors.fill: dayCircle
                        shapeRadius: dayCircle.radius
                        visible: dayNav.highlight
                    }

                    MouseArea {
                        id: dayMouse

                        anchors.fill: parent
                        hoverEnabled: true
                        enabled: dayCell.selectable
                        cursorShape: Qt.PointingHandCursor

                        onClicked: {
                            dayNav.requestActive()
                            root.selectedDate = dayCell.cellDate
                            root.dateSelected(dayCell.cellDate)
                        }
                    }
                }
            }
        }
    }
}
