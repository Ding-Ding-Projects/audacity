/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include <QString>

#include "framework/global/modularity/imoduleinterface.h"

#include "experiencetypes.h"

namespace au::experience {
//! Styles the tone of a message. It never changes a fact, a number, a
//! keyboard shortcut, a file name or a control label.
class IMessageStyler : MODULE_GLOBAL_INTERFACE
{
    INTERFACE_ID(au::experience::IMessageStyler)

public:
    virtual ~IMessageStyler() = default;

    //! Returns plainText decorated for the current language mode and funny
    //! levels. The result is deterministic: the same inputs always give the
    //! same output.
    virtual QString style(MessageKind kind, const QString& plainText) const = 0;

    //! The same call with the language mode, levels and emoji switch given
    //! explicitly. Used by the tests and by the settings preview.
    virtual QString styleWith(MessageKind kind, const QString& plainText, LanguageMode mode, int englishLevel, int cantoneseLevel,
                              bool emoji) const = 0;
};
}
