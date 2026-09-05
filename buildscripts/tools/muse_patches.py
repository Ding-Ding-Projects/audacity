#!/usr/bin/env python3
"""Manage the Material Design 3 patch overlay applied to the muse submodule.

Audacity 4 uses the MuseScore "muse" framework as a pinned git submodule. Part of
the user interface (dock chrome, menus, popups, dialogs, tooltips, scroll bars and
the shared controls) is drawn by muse itself, so a Material Design 3 rewrite has to
change those files. Forking muse is not possible in this environment, therefore the
changes live as numbered patches in buildscripts/muse-patches and are applied to the
submodule working tree at configure time.

Subcommands:
    apply       apply every patch that is not applied yet (idempotent)
    revert      reverse every applied patch, newest first (idempotent)
    regenerate  rewrite the patch files from the current submodule working tree
    status      report, per patch, whether it is applied

The submodule pointer is never changed: nothing is committed inside muse.
"""

import argparse
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
MUSE = os.path.join(ROOT, "muse")
PATCH_DIR = os.path.join(ROOT, "buildscripts", "muse-patches")

# Ordered logical file groups. Each entry becomes one patch file.
GROUPS = [
    (
        "0001-m3-roles-singleton",
        "Add the Material Design 3 colour role singleton",
        [
            "framework/ui/qml/Muse/Ui/M3Roles.qml",
            "framework/ui/qml/Muse/Ui/CMakeLists.txt",
        ],
    ),
    (
        "0002-m3-menus",
        "Material Design 3 menu anatomy",
        [
            "framework/uicomponents/qml/Muse/UiComponents/internal/StyledMenu.qml",
            "framework/uicomponents/qml/Muse/UiComponents/internal/StyledMenuItem.qml",
            "framework/uicomponents/qml/Muse/UiComponents/ListItemBlank.qml",
            "framework/uicomponents/qml/Muse/UiComponents/SeparatorLine.qml",
        ],
    ),
    (
        "0003-m3-popups-and-dialogs",
        "Material Design 3 popup, dialog and bottom sheet frames",
        [
            "framework/uicomponents/qml/Muse/UiComponents/internal/PopupContent.qml",
            "framework/uicomponents/qml/Muse/UiComponents/StyledPopupView.qml",
            "framework/uicomponents/qml/Muse/UiComponents/StyledDialogView.qml",
            "framework/uicomponents/qml/Muse/UiComponents/PopupPanel.qml",
        ],
    ),
    (
        "0004-m3-tooltip-and-scrollbar",
        "Material Design 3 tooltips and scroll bars",
        [
            "framework/uicomponents/qml/Muse/UiComponents/StyledToolTip.qml",
            "framework/uicomponents/qml/Muse/UiComponents/StyledScrollBar.qml",
        ],
    ),
    (
        "0005-m3-controls",
        "Material Design 3 buttons, selection controls, fields and tabs",
        [
            "framework/uicomponents/qml/Muse/UiComponents/FlatButton.qml",
            "framework/uicomponents/qml/Muse/UiComponents/FlatToggleButton.qml",
            "framework/uicomponents/qml/Muse/UiComponents/CheckBox.qml",
            "framework/uicomponents/qml/Muse/UiComponents/RoundedRadioButton.qml",
            "framework/uicomponents/qml/Muse/UiComponents/StyledSlider.qml",
            "framework/uicomponents/qml/Muse/UiComponents/ProgressBar.qml",
            "framework/uicomponents/qml/Muse/UiComponents/StyledBusyIndicator.qml",
            "framework/uicomponents/qml/Muse/UiComponents/StyledTabBar.qml",
            "framework/uicomponents/qml/Muse/UiComponents/StyledTabButton.qml",
            "framework/uicomponents/qml/Muse/UiComponents/TextInputField.qml",
            "framework/uicomponents/qml/Muse/UiComponents/SearchField.qml",
            "framework/uicomponents/qml/Muse/UiComponents/StyledDropdown.qml",
        ],
    ),
    (
        "0006-m3-dock-chrome",
        "Material Design 3 dock window chrome",
        [
            "framework/dockwindow/qml/Muse/Dock/DockFrame.qml",
            "framework/dockwindow/qml/Muse/Dock/DockTitleBar.qml",
            "framework/dockwindow/qml/Muse/Dock/DockTabBar.qml",
            "framework/dockwindow/qml/Muse/Dock/DockPanelTab.qml",
            "framework/dockwindow/qml/Muse/Dock/DockSeparator.qml",
            "framework/dockwindow/qml/Muse/Dock/DockFloatingWindow.qml",
            "framework/dockwindow/qml/Muse/Dock/DockingHolder.qml",
        ],
    ),
    (
        "0008-m3-button-box",
        "Let a button box hold a host application button component",
        [
            "framework/uicomponents/qml/Muse/UiComponents/ButtonBox.qml",
        ],
    ),
    (
        "0009-m3-shortcuts-page",
        "Material Design 3 shortcut preferences page",
        [
            "framework/uicomponents/qml/Muse/UiComponents/ValueList.qml",
            "framework/uicomponents/qml/Muse/UiComponents/internal/ValueListItem.qml",
            "framework/shortcuts/qml/Muse/Shortcuts/ShortcutsPage.qml",
            "framework/shortcuts/qml/Muse/Shortcuts/internal/ShortcutsList.qml",
            "framework/shortcuts/qml/Muse/Shortcuts/internal/ShortcutsTopPanel.qml",
        ],
    ),
    (
        "0010-m3-list-table-and-avatar",
        "Material Design 3 tables, page indicators and account avatars",
        [
            "framework/uicomponents/qml/Muse/UiComponents/StyledTableView.qml",
            "framework/uicomponents/qml/Muse/UiComponents/PageIndicator.qml",
            "framework/cloud/qml/Muse/Cloud/AccountAvatar.qml",
        ],
    ),
    (
        "0007-m3-interactive-dialogs",
        "Material Design 3 standard message dialogs",
        [
            "framework/interactive/qml/Muse/Interactive/StandardDialog.qml",
            "framework/interactive/qml/Muse/Interactive/StandardDialogPanel.qml",
        ],
    ),
]


def git(args, check=True, capture=True):
    return subprocess.run(
        ["git", "-C", MUSE] + args,
        check=check,
        capture_output=capture,
        text=True,
    )


def patch_files():
    if not os.path.isdir(PATCH_DIR):
        return []
    return [
        os.path.join(PATCH_DIR, name)
        for name in sorted(os.listdir(PATCH_DIR))
        if name.endswith(".patch")
    ]


def can(args):
    return git(args, check=False).returncode == 0


def is_applied(path):
    return can(["apply", "--check", "--reverse", "-p1", path])


def can_apply(path):
    return can(["apply", "--check", "-p1", path])


def cmd_apply(_args):
    for path in patch_files():
        name = os.path.basename(path)
        if is_applied(path):
            print("already applied: %s" % name)
            continue
        if not can_apply(path):
            result = git(["apply", "--check", "-p1", path], check=False)
            sys.stderr.write(
                "error: %s does not apply to the muse submodule.\n%s\n"
                % (name, result.stderr)
            )
            return 1
        git(["apply", "-p1", path])
        print("applied: %s" % name)
    return 0


def cmd_revert(_args):
    for path in reversed(patch_files()):
        name = os.path.basename(path)
        if is_applied(path):
            git(["apply", "--reverse", "-p1", path])
            print("reverted: %s" % name)
        elif can_apply(path):
            print("not applied: %s" % name)
        else:
            sys.stderr.write("error: %s is neither applied nor revertible.\n" % name)
            return 1
    return 0


def cmd_status(_args):
    for path in patch_files():
        name = os.path.basename(path)
        if is_applied(path):
            state = "applied"
        elif can_apply(path):
            state = "not applied"
        else:
            state = "conflicting"
        print("%-40s %s" % (name, state))
    return 0


def cmd_regenerate(_args):
    os.makedirs(PATCH_DIR, exist_ok=True)
    for name, title, paths in GROUPS:
        present = [p for p in paths if os.path.exists(os.path.join(MUSE, p))]
        if present:
            # Intent to add, so that new files show up in the diff.
            git(["add", "-N", "--"] + present, check=False)
        diff = git(["diff", "--binary", "--"] + paths).stdout
        target = os.path.join(PATCH_DIR, name + ".patch")
        if not diff.strip():
            if os.path.exists(target):
                os.remove(target)
                print("removed empty patch: %s.patch" % name)
            continue
        header = (
            "Subject: [PATCH] %s\n\n"
            "Generated by buildscripts/tools/muse_patches.py regenerate.\n"
            "Applied to the muse submodule working tree at configure time.\n"
            "---\n" % title
        )
        with open(target, "w") as handle:
            handle.write(header + diff)
        print("wrote: %s.patch" % name)
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("apply")
    sub.add_parser("revert")
    sub.add_parser("regenerate")
    sub.add_parser("status")
    args = parser.parse_args()
    return {
        "apply": cmd_apply,
        "revert": cmd_revert,
        "regenerate": cmd_regenerate,
        "status": cmd_status,
    }[args.command](args)


if __name__ == "__main__":
    sys.exit(main())
