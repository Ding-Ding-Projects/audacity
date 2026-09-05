# Search field inventory

Every search and filter field in the application is listed here, together with
the regular expression builder that serves it. The list is hand written and
guarded: `src/companion/tests/search_inventory_tests.cpp` scans `src/**/*.qml`
at test time and fails when an `M3SearchBar` has no row here, when a row names
a file or an object name that no longer exists, or when a row claims a builder
that its file does not carry.

## How to read the table

- **Object name** is the `objectName` on the `M3SearchBar`. Every field carries
  one, both so that this inventory can be checked and so that the accessibility
  tree and the automated captures can address the field.
- **Builder anchored** is `yes` when the file itself carries a
  `RegexBuilderSheet` beside the field, `forwarded` when the field raises
  `regexBuilderRequested` for a host to answer, and `demonstration` for the
  component gallery, which shows the search bar itself rather than serving a
  real search. A `forwarded` row names its host.
- **Store** is the name the builder persists its saved test cases under, in
  `<app data>/companion/regex/<store>.json`. Distinct store names are what keep
  the fields' saved cases apart.

## The fields

| Surface | QML file | Object name | Builder anchored | Store or host |
| --- | --- | --- | --- | --- |
| Command palette | `src/companion/qml/Audacity/Companion/CommandPalette.qml` | `CommandPaletteSearch` | yes | `command-palette` |
| Preferences, settings search | `src/preferences/qml/Audacity/Preferences/PreferencesDialog.qml` | `PreferencesSearch` | yes | `preferences` |
| Preferences, advanced options search | `src/preferences/qml/Audacity/Preferences/internal/AdvancedTopSection.qml` | `AdvancedPreferencesSearch` | forwarded | `src/preferences/qml/Audacity/Preferences/AdvancedPreferencesPage.qml` |
| History panel | `src/projectscene/qml/Audacity/ProjectScene/historypanel/HistoryPanel.qml` | `HistoryPanelSearch` | yes | `history-panel` |
| Plugin manager top panel | `src/effects/effects_base/qml/Audacity/Effects/PluginManagerTopPanel.qml` | `PluginManagerSearch` | yes | `plugin-manager` |
| Home, plugins page | `src/appshell/qml/Audacity/AppShell/HomePage/PluginsPage.qml` | `PluginsPageSearch` | yes | `plugins-page` |
| Projects page, recent projects | `src/project/qml/Audacity/Project/ProjectsPage.qml` | `ProjectsPageSearch` | yes | `projects-page` |
| New project, template titles | `src/project/qml/Audacity/Project/internal/NewProject/TitleListView.qml` | `NewProjectTitleSearch` | yes | `new-project-titles` |
| Developer tools, settings list | `src/appshell/qml/Audacity/AppShell/DevTools/Preferences/SettingsPage.qml` | `DevToolsSettingsSearch` | forwarded | `src/appshell/qml/Audacity/AppShell/DevTools/DevToolsPage.qml` |
| `M3Menu` filter field | `src/uicomponents/qml/Audacity/M3/M3Menu.qml` | `M3MenuSearch` | forwarded | the menu's host, through `M3Menu.regexBuilderRequested` |
| Version history panel | `src/chronicle/qml/Audacity/Chronicle/VersionHistoryPanel.qml` | `VersionHistorySearch` | yes | `version-history` |
| Changelog dialog | `src/chronicle/qml/Audacity/Chronicle/ChangelogDialog.qml` | `ChangelogSearch` | forwarded | the dialog's host, through `regexBuilderRequested` |
| Close tabs popup, match query | `src/chronicle/qml/Audacity/Chronicle/CloseTabsPopup.qml` | `CloseTabsQuery` | forwarded | the tab strip that opened the popup |
| Tab search, this strip | `src/chronicle/qml/Audacity/Chronicle/TabSearchPopup.qml` | `TabSearchStripQuery` | forwarded | the tab strip that opened the popup |
| Tab search, inside a group | `src/chronicle/qml/Audacity/Chronicle/TabSearchPopup.qml` | `TabSearchGroupQuery` | forwarded | the tab strip that opened the popup |
| Tab search, groups by name | `src/chronicle/qml/Audacity/Chronicle/TabSearchPopup.qml` | `TabSearchGroupsQuery` | forwarded | the tab strip that opened the popup |
| Toolkit, local model manager search | `src/toolkit/qml/Audacity/Toolkit/OllamaPage.qml` | `OllamaModelSearch` | yes | `toolkit-ollama` |
| Toolkit, documentation browser search | `src/toolkit/qml/Audacity/Toolkit/DocsBrowserPage.qml` | `DocsBrowserSearch` | yes | `toolkit-docs` |
| Tab search, every strip | `src/chronicle/qml/Audacity/Chronicle/TabSearchPopup.qml` | `TabSearchAllQuery` | forwarded | the tab strip that opened the popup |
| Notification centre | `src/experience/qml/Audacity/Experience/NotificationCentre.qml` | `NotificationCentreSearch` | forwarded | the application window that opens the centre |
| Component gallery, plain search bar | `src/appshell/qml/Audacity/AppShell/DevTools/Gallery/M3ComponentsGallery.qml` | `GallerySearchBarPlain` | demonstration | component demonstration, no builder by design |
| Component gallery, search bar with the builder button | `src/appshell/qml/Audacity/AppShell/DevTools/Gallery/M3ComponentsGallery.qml` | `GallerySearchBarWithRegexBuilder` | demonstration | component demonstration, no builder by design |

## Known gaps

Several fields are deliberately `forwarded` rather than anchored:

- The **advanced preferences search** sits inside a `BaseSection`, which is a
  column and cannot host an anchored sheet. It raises
  `regexBuilderRequested(pattern)` and `AdvancedPreferencesPage` answers it with
  its own builder under the store `preferences-advanced`, so its saved cases
  are still separate from the dialog wide settings search beside it.
- The **developer tools settings list** has a `ColumnLayout` root for the same
  reason. It keeps the `regexBuilderRequested` hook for its host page.
- The **component gallery** shows `M3SearchBar` itself, including its builder
  button. Attaching a working builder there would make the gallery a second
  place to maintain the wiring, so the demonstration bars raise the signal and
  nothing answers it.

`M3Menu` is a component rather than a surface: it forwards
`regexBuilderRequested` so that whichever surface opened the menu can anchor a
builder for it. No menu in the application requests one today.

The **changelog dialog**, the **tab search and close tabs popups** and the
**notification centre** are popups and sheets rather than anchored surfaces:
a side sheet inside a popup would be clipped by the popup's own window. Each of
them raises `regexBuilderRequested` for the surface that opened it, which is
where the builder belongs. The version history panel, which is an ordinary
docked panel, anchors its own builder.
