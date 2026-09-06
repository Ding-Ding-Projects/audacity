# Completeness delivery boundary

The project keeps two handwritten inventories. The canonical inventory records
whether every required capability exists. The per-surface inventory records the
visible surface and evidence that proves each capability. Separate canonical
lists make a disappeared row fail visibly.

Use report integrity during ordinary work:

```sh
python buildscripts/checks/completeness_guard.py --strict
```

It checks canonical coverage, duplicate rows, referenced paths, translation
contexts, and reasons for missing or inapplicable rows. It does not claim the
product is complete. Ordinary configure, build, and release flows are not made
dependent on this verdict.

Use the delivery gate only when evaluating complete delivery:

```sh
python buildscripts/checks/completeness_guard.py --completion
```

Completion fails closed until every canonical feature is implemented and every
per-surface row names implementation, documentation, localized copy,
persistence, focused test, built-artifact interaction, capture, and capture
provenance. A source path, mock, or uncited image cannot stand in for a real
interaction receipt. The current inventory is intentionally red in this mode,
because it records partial and missing features, unrecorded interactions,
missing capture receipts, and absent Status Hub enrollment honestly.

`buildscripts/checks/test_completeness_guard.py` proves report integrity green,
completion red for the current state, then deliberately removes a canonical row
from each inventory, an implementation file, and a receipt boundary. Each
applicable verdict must turn red.
