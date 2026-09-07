# Per-surface completeness inventory

This is a handwritten inventory independent of `completeness-inventory.md`.
The canonical inventory records feature facts. This table records the specific
surface which must prove each feature. Missing built-artifact interaction,
capture, or receipt evidence is written as `unverified`, never inferred from
source code or a design image.

This table is a narrative source-level record, not delivery proof. Concrete
product/surface/feature coverage lives in `completion-evidence.json`, with its
explicit surface registry in `concrete-surfaces.json`. Completion requires
`completeness_guard.py --completion --candidate <full-audited-commit-sha>` and
candidate-bound receipts described in `docs/features/completeness.md`. Multiple
concrete surfaces may describe the same capability independently here.

| Surface | Feature | Implementation | Documentation | Localized copy | Persistence | Focused test | Real interaction | Capture | Capture provenance | Status | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Front screen | Language modes | `src/experience/internal/experienceconfiguration.cpp` | `docs/features/language-modes.md` | context `experience` | `experience/language/mode` | unverified | unverified | unverified | real launch receipt pending | implemented | No front-screen evidence recorded. |
| Preferences | Funny level, English | `src/experience/internal/messagestyler.cpp` | `docs/features/funny-levels.md` | context `experience` | `experience/funny/english` | unverified | unverified | unverified | unverified | implemented | Evidence not yet recorded here. |
| Preferences | Funny level, Cantonese | `src/experience/internal/messagestyler.cpp` | `docs/features/funny-levels.md` | context `experience` | `experience/funny/cantonese` | unverified | unverified | unverified | unverified | implemented | Evidence not yet recorded here. |
| Preferences | Emoji switch | `src/experience/internal/messagestyler.cpp` | `docs/features/emoji-switch.md` | context `experience` | `experience/emoji/dialogs` | unverified | unverified | unverified | unverified | implemented | Evidence not yet recorded here. |
| Preferences | School mode | `src/experience/internal/schoolmode.cpp` | `docs/features/school-mode.md` | context `preferences` | `<app data>/shared/school-mode.json` | unverified | unverified | unverified | unverified | partial | Existing row records consumer and capture gaps. |
| Preferences | Narrator (TTS) | `src/experience/internal/narratorservice.cpp` | `docs/features/narrator.md` | context `preferences` | `experience/narrator/enabled` | unverified | unverified | unverified | unverified | partial | No event calls narration. |
| Schedule editor | Scheduled settings | `src/experience/internal/settingsscheduler.cpp` | `docs/features/scheduled-settings.md` | context `experience` | `experience/schedule/entries` | unverified | unverified | unverified | unverified | implemented | No receipt recorded. |
| Schedule editor | External settings sources (Home Assistant) | `src/experience/internal/settingsscheduler.cpp` | `docs/features/scheduled-settings.md` | context `experience` | `experience/schedule/entries` | unverified | unverified | unverified | unverified | partial | No client or secure source resolution exists. |
| Experience overlay | Dim sum surprise | `src/experience/internal/dimsumsurprise.cpp` | `docs/features/dim-sum-surprise.md` | context `experience` | n/a | unverified | unverified | unverified | unverified | implemented | Existing captures lack a receipt in this inventory. |
| Search popover | Regex builder | `src/companion/regex/regexengine.cpp` | `docs/features/regex-builder.md` | context `experience` | `<app data>/companion/regex/<store>.json` | unverified | unverified | unverified | unverified | implemented | No receipt recorded. |
| Notification centre | Notifications | `src/experience/internal/notificationcenter.cpp` | `docs/features/notifications.md` | context `experience` | n/a | unverified | unverified | unverified | unverified | partial | Required controls and capture remain incomplete. |
| Appearance editor | Material 3 appearance and per element editor | `src/personalize/internal/appearanceoverrides.cpp` | `docs/features/appearance-editor.md` | context `experience` | `<app data>/personalize/appearance-overrides.json` | unverified | unverified | unverified | unverified | partial | Focused test remains absent. |
| Project tabs | Tabs, groups and tab search | `src/chronicle/view/tabstripmodel.cpp` | `docs/features/tab-navigation.md` | context `experience` | `experience/tabs/dock` | unverified | unverified | unverified | unverified | implemented | No receipt recorded. |
| Documentation browser | Landing page and offline docs | `src/toolkit/internal/docsindex.cpp` | `docs/features/docs-browser.md` | context `experience` | n/a | unverified | unverified | unverified | unverified | implemented | Built route and capture unverified. |
| Command palette | Command palette | `src/companion/palette/commandpalettemodel.cpp` | `docs/features/command-palette.md` | context `experience` | `experience/palette/size` | unverified | unverified | unverified | unverified | implemented | No receipt recorded. |
| Confirmation dialog | Destructive action super confirmation | `src/experience/qml/Audacity/Experience/SuperConfirmationDialog.qml` | `docs/features/super-confirmation.md` | context `experience` | n/a | unverified | unverified | unverified | unverified | partial | Focused test and capture absent. |
| Version history | Local version history | `src/chronicle/view/versionhistorymodel.cpp` | `docs/features/local-history.md` | context `experience` | `<appDataDir>/history/<project-id>.git` | unverified | unverified | unverified | unverified | implemented | Panel capture remains absent. |
| Changelog dialog | Changelog viewer | `src/chronicle/view/changelogmodel.cpp` | `docs/features/changelog.md` | context `experience` | n/a | unverified | unverified | unverified | unverified | implemented | No receipt recorded. |
| External editor handoff | External editor handoff | `src/toolkit/internal/externaleditorservice.cpp` | `docs/features/external-editor.md` | context `experience` | `<app data>/toolkit/external-editor.json` | unverified | unverified | unverified | unverified | partial | Focused test and capture absent. |
| Export sheet | Universal export | `src/toolkit/internal/exportservice.cpp` | `docs/features/exports.md` | context `experience` | n/a | unverified | unverified | unverified | unverified | implemented | No receipt recorded. |
| Documentation browser | Bulk actions | `src/toolkit/internal/bulkselectionmodel.cpp` | `docs/features/bulk-actions.md` | context `experience` | n/a | unverified | unverified | unverified | unverified | partial | Not wired to every list. |
| All interactive surfaces | Accessibility (keyboard, focus, names, contrast) | `src/uicomponents/qml/Audacity/M3/M3FocusRing.qml` | n/a | n/a | n/a | unverified | unverified | unverified | unverified | partial | Per-surface audit is absent. |
| All interactive surfaces | Responsive sizing (narrow widths, 200% scale) | n/a | n/a | n/a | n/a | unverified | unverified | unverified | unverified | missing | No comprehensive sizing matrix exists. |
| Preferences | Personal vocabulary JSON upload | `src/experience/internal/personalvocabulary.cpp` | `docs/features/personal-vocabulary.md` | context `experience` | `<app data>/experience/personal-vocabulary.json` | unverified | unverified | unverified | unverified | implemented | No receipt recorded. |
| Personalize locks | Toy locks | `src/personalize/internal/lockregistry.cpp` | `docs/features/toy-locks.md` | context `experience` | `<app data>/personalize/locks.json` | unverified | unverified | unverified | unverified | partial | Focused test and capture absent. |
| Support Tickets | Support Tickets | `src/personalize/internal/supporttickets.cpp` | `docs/features/support-tickets.md` | context `experience` | `<app data>/personalize/support-tickets.json` | unverified | unverified | unverified | unverified | partial | Focused test and capture absent. |
| Browser extension | Browser extension download capture dialogs | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | not applicable | No browser extension exists. |
| Account recovery | Unlock ladder | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | not applicable | No account lockout exists. |
| Documentation site | Shared link embed graphic | `docs/site/index.html` | n/a | n/a | n/a | unverified | unverified | unverified | unverified | implemented | Deployed rendering is unverified. |
| Preferences | ADHD modes (attention support) | `src/experience/internal/experienceconfiguration.cpp` | `docs/features/attention-support-modes.md` | context `experience` | `experience/modes/focus` | unverified | unverified | unverified | unverified | partial | Focused tests and capture absent. |
| Personalize settings | App logo customization | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | missing | No feature exists. |
| File converter | Universal file converter | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | missing | No feature exists. |
| Local model manager | Local model manager (Ollama suite) | `src/toolkit/internal/ollamaclient.cpp` | `docs/features/ollama-suite-manager.md` | context `experience` | `<app data>/toolkit/ollama.json` | unverified | unverified | unverified | unverified | implemented | No receipt recorded. |
| Front screen | Version and build time on the front screen | `src/appshell/qml/Audacity/AppShell/aboutmodel.cpp` | n/a | n/a | n/a | unverified | unverified | unverified | unverified | partial | Currently confined to About. |
| Update banner | Automatic updates | `src/squirrelupdate/internal/squirrelupdateservice.cpp` | `docs/features/automatic-updates.md` | context `experience` | `experience/update/checkIntervalHours` | unverified | unverified | unverified | unverified | implemented | No receipt recorded. |
| Status surface | Status Hub row | n/a | `docs/features/status-reporting.md` | n/a | n/a | n/a | n/a | n/a | n/a | not applicable | Enrollment is absent and remains an unmet global contract. |
| Documentation browser | Docs browser bookmark export/bulk | `src/toolkit/internal/docsindex.cpp` | `docs/features/docs-browser.md` | context `experience` | `<app data>/toolkit/docs-bookmarks.json` | unverified | unverified | unverified | unverified | partial | Focused persistence test absent. |
| Personalize settings | Renaming the application | `src/personalize/internal/displaynamesettings.cpp` | `docs/features/app-rename.md` | context `experience` | `<application data directory>/personalize/display-name.txt` | unverified | unverified | unverified | unverified | partial | Focused test and capture absent. |
| Authenticator | Built in authenticator (TOTP) | `src/personalize/internal/totpengine.cpp` | `docs/features/authenticator.md` | context `experience` | `<app data>/personalize/authenticator.json` | unverified | unverified | unverified | unverified | implemented | No receipt recorded. |
