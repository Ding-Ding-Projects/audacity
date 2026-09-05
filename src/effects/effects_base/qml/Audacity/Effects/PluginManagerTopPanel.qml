/*
 * Audacity: A Digital Audio Editor
 */
import QtQuick 2.15
import QtQuick.Layouts
import Muse.UiComponents
import Audacity.UiComponents
import Audacity.Effects
import Audacity.M3
import Audacity.Companion

Item {
    id: root

    required property PluginManagerTableViewModel tableViewModel
    readonly property int contentHeight: 30

    property NavigationPanel navigationPanel: NavigationPanel {
        name: "PluginManagerTopPanel"
        direction: NavigationPanel.Horizontal
        accessible.role: MUAccessible.ComboBox
        accessible.name: showModel.label + ", " + typeModel.label + ", " + categoryModel.label + ", " + statusModel.label + ", " + searchField.accessibleName
    }

    signal searchTextChanged(string newText)

    function focusOnFirst() {
        dropdownsRepeater.itemAt(0).navigation.requestActive()
    }

    function readInfo() {
        accessibleInfo.ignored = false
        accessibleInfo.focused = true
    }

    AccessibleItem {
        id: accessibleInfo
        visualItem: root
        role: MUAccessible.ComboBox
        name: showModel.label + ", " + typeModel.label + ", " + categoryModel.label + ", " + statusModel.label + ", " + searchField.accessibleName
    }

    Component.onCompleted: {
        tableViewModel.enabledDisabledSelectedIndex = Qt.binding(function () {
            return showModel.selectedIndex
        })
        tableViewModel.effectFamilySelectedIndex = Qt.binding(function () {
            return typeModel.selectedIndex
        })
        tableViewModel.effectTypeSelectedIndex = Qt.binding(function () {
            return categoryModel.selectedIndex
        })
        tableViewModel.statusSelectedIndex = Qt.binding(function () {
            return statusModel.selectedIndex
        })
    }

    DropdownOptionsModel {
        id: showModel
        label: qsTrc("effects", "Show:")
        options: tableViewModel.enabledDisabledOptions
    }

    DropdownOptionsModel {
        id: typeModel
        label: qsTrc("effects", "Type:")
        options: tableViewModel.effectFamilyOptions
    }

    DropdownOptionsModel {
        id: categoryModel
        label: qsTrc("effects", "Category:")
        options: tableViewModel.effectTypeOptions
    }

    DropdownOptionsModel {
        id: statusModel
        label: qsTrc("effects", "Status:")
        options: tableViewModel.statusOptions
    }

    RowLayout {
        id: rowLayout

        anchors.fill: parent
        spacing: 16

        Repeater {
            id: dropdownsRepeater

            model: [showModel, typeModel, categoryModel, statusModel]

            DropdownWithTitle {
                id: dropdown

                Layout.preferredWidth: 260
                Layout.preferredHeight: root.contentHeight

                title: modelData.label
                model: modelData.options
                current: modelData.currentTitle
                allowOptionToggle: false

                navigation.name: modelData.label + "Dropdown"
                navigation.panel: root.navigationPanel

                Component.onCompleted: {
                    // Don't know why `navigation.order: index` doesn't work here
                    navigation.order = index
                }

                onHandleMenuItem: function (itemId) {
                    modelData.select(itemId)
                }
            }
        }

        M3SearchBar {
            id: searchField

            objectName: "PluginManagerSearch"

            showRegexBuilder: true

            Layout.fillWidth: true
            Layout.preferredHeight: root.contentHeight

            navigation.name: "SearchField"
            navigation.panel: root.navigationPanel
            navigation.order: dropdownsRepeater.count

            onSearchTextChanged: {
                root.searchTextChanged(searchField.searchText)
            }

            onRegexBuilderRequested: {
                regexBuilder.pattern = searchField.searchText
                regexBuilder.open()
            }
        }
    }

    // The regular expression builder for this field. Each search surface owns its
    // own instance, so its pattern, flags, sample and saved test cases are
    // isolated from every other search field in the application.
    RegexBuilderSheet {
        id: regexBuilder

        anchors.fill: parent

        storeName: "plugin-manager"
        fieldLabel: "Plugin manager"

        onPatternAccepted: function (pattern) {
            searchField.searchText = pattern
        }
    }
}
