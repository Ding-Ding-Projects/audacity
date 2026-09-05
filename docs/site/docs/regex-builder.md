# The regex builder

Every search field on this site, and every search field in the application,
opens the same regex builder from a small **.\*** button beside the field.

Plain text matching is the default; switching to regex is an explicit,
visible toggle. The builder offers guided tokens (character classes,
anchors, groups, quantifiers including lazy forms, lookaround,
backreferences, and flags), a live match table with captures against a
sample, a replacement preview, a plain-English explanation of the pattern
token by token, saved test cases, JSON import/export, and a bounded timing
run that warns about nested quantifiers that can cause catastrophic
backtracking.
