# Emoji switch

`experience/emoji/dialogs`, on by default.

When it is on, one emoji is placed in front of the body of a dialog, message
box or notification toast. The emoji depends on the kind of message:

| Kind | Emoji |
| --- | --- |
| Info, dialog | 💬 |
| Success | 🎉 |
| Warning | ⚠️ |
| Error | 🛑 |

Emoji never appear in:

- buttons or any other action label;
- field labels;
- menu items;
- accessible names, so a screen reader is never asked to pronounce one;
- tooltips, which are help text sitting next to a control.

The rule lives in one place, `MessageStyler::styleWith()`, and is asserted by
`src/experience/tests/messagestyler_tests.cpp`: a tooltip is returned unchanged
even with the switch on, and a body with the switch off is byte for byte the
plain text.

The switch is independent of the funny levels. At funny level 1 the text stays
completely plain, and the emoji still appears if the switch is on.
