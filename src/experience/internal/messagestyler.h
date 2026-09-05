/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include "framework/global/modularity/ioc.h"

#include "imessagestyler.h"
#include "iexperienceconfiguration.h"

namespace au::experience {
class MessageStyler : public IMessageStyler
{
    muse::GlobalInject<IExperienceConfiguration> configuration;

public:
    ~MessageStyler() override = default;

    QString style(MessageKind kind, const QString& plainText) const override;
    QString styleWith(MessageKind kind, const QString& plainText, LanguageMode mode, int englishLevel, int cantoneseLevel,
                      bool emoji) const override;
};
}
