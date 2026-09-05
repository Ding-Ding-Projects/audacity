# muse patch overlay

Audacity 4 builds against the MuseScore `muse` framework, which is a git submodule
pinned to commit `ca86211f8d5e45405de999827c43bdb24e4d682e`. A significant part of the
window chrome is drawn by muse itself: dock title bars, dock tab bars, separators, drop
indicators, context menus, popup views, dialog frames, tooltips, scroll bars and the
shared controls used by muse-internal dialogs.

The Material Design 3 rewrite has to restyle those files. Forking muse is not possible
in this environment (see `docs/design/MUSE_OVERLAY.md` for the exact blocker), so the
changes are kept here as numbered unified diffs and applied to the submodule working
tree at configure time. The submodule pointer never changes and nothing is committed
inside `muse/`.

## Patches

| Patch | What it changes |
| --- | --- |
| `0001-m3-roles-singleton.patch` | Adds the `Muse.Ui` singleton `M3Roles.qml` and registers it in the module CMake file |
| `0002-m3-menus.patch` | Menu container, menu items, list item state layers and dividers |
| `0003-m3-popups-and-dialogs.patch` | Popup content frame, popup view, dialog view, bottom sheet panel |
| `0004-m3-tooltip-and-scrollbar.patch` | Plain and rich tooltips, thin expanding scroll bar |
| `0005-m3-controls.patch` | Buttons, selection controls, slider, progress, tabs, text fields, dropdown |
| `0006-m3-dock-chrome.patch` | Dock frame, title bar, tab bar, tab, separator, floating window, drop indicators |
| `0007-m3-interactive-dialogs.patch` | Standard message dialogs from the muse interactive module |

## Mechanism

`buildscripts/cmake/ApplyMusePatches.cmake` is included from the top-level
`CMakeLists.txt` right after `MUSE_FRAMEWORK_PATH` is set. For every patch it runs

    git -C muse apply --check --reverse -p1 <patch>

If that succeeds the patch is already applied and is skipped, which makes reconfiguring
idempotent. Otherwise the patch is checked and applied with `git -C muse apply -p1`.
If a patch neither reverses nor applies, configure fails with an explanatory message.

The overlay can be turned off with `-DAU_APPLY_MUSE_PATCHES=OFF`.

## Working on the patches

Edit the files inside `muse/` directly, then regenerate:

    python3 buildscripts/tools/muse_patches.py regenerate

Other subcommands:

    python3 buildscripts/tools/muse_patches.py status
    python3 buildscripts/tools/muse_patches.py apply
    python3 buildscripts/tools/muse_patches.py revert

`regenerate` writes `git -C muse diff` split by the logical file groups declared in
`buildscripts/tools/muse_patches.py`. To add a file to the overlay, add its path to the
right group (or add a new group) and regenerate.
