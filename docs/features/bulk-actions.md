# Bulk actions

`BulkSelectionModel` and the `BulkSelectionController` QML surface add
multi-select, shift-range selection, keyboard operation, honestly-scoped
select-all, invert and a reviewable count to any list the toolkit module
owns, currently the local model manager's model list and the documentation
browser's bookmark list.

## Behaviour

- Select-all distinguishes "this page" from "every match" and the current
  scope is always shown next to the selection count.
- Invert flips the current selection against the reported total, including
  the "every match" state.
- A destructive bulk action (for example removing a bookmark) is never
  performed directly by this control; it reports the chosen indexes to the
  host page, which is responsible for routing a genuinely destructive action
  through its own super confirmation surface before applying it.

## Configuration

`totalCount`, `pageStart` and `pageEnd` are supplied by the host page so the
select-all wording matches what is actually on screen.

## Failure modes

None of the controller's own operations can fail; it only tracks indexes.

## Security considerations

The controller performs no action on its own model; it cannot bypass a
host's own confirmation for a destructive action, since it never calls one.

## Verification

`bulkselectionmodel_tests.cpp` covers toggling, shift-range selection,
page-scoped and all-matches select-all, invert in both scopes, and the
selected index list in each state.
