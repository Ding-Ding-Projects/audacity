/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore BVBA and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * The language page. The bilingual choice is kept as a setting until the
 * bilingual strings land.
 */
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.AppShell

Page {
    id: root

    title: qsTrc("appshell/gettingstarted", "Language")

    titleContentSpacing: 16

    property NavigationPanel languagePanel: NavigationPanel {
        name: "LanguagePanel"
        enabled: root.enabled && root.visible
        section: root.navigationSection
        order: root.navigationStartRow + 1
        direction: NavigationPanel.Vertical
        accessible.name: qsTrc("appshell/gettingstarted", "Language")
    }

    readonly property var languages: [
        {
            "code": "en",
            "title": qsTrc("appshell/gettingstarted", "English"),
            "bilingual": false
        },
        {
            "code": "zh_HK",
            "title": qsTrc("appshell/gettingstarted", "Cantonese (Hong Kong)"),
            "bilingual": false
        },
        {
            "code": "en",
            "title": qsTrc("appshell/gettingstarted", "Bilingual"),
            "bilingual": true
        }
    ]

    LanguagePageModel {
        id: model
    }

    readonly property int currentIndex: {
        if (model.bilingual) {
            return 2;
        }

        return model.currentLanguageCode === "zh_HK" ? 1 : 0;
    }

    function selectLanguage(index) {
        var language = root.languages[index];
        model.bilingual = language.bilingual;
        model.currentLanguageCode = language.code;
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24

        spacing: 8

        Repeater {
            model: root.languages

            delegate: M3RadioButton {
                id: languageButton

                required property int index
                required property var modelData

                Layout.fillWidth: true

                text: languageButton.modelData.title
                checked: root.currentIndex === languageButton.index

                navigation.panel: root.languagePanel
                navigation.row: languageButton.index
                navigation.name: languageButton.modelData.title

                onToggled: {
                    root.selectLanguage(languageButton.index);
                }
            }
        }

        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true
        }
    }
}
