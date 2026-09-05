/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include "framework/global/async/asyncable.h"
#include "framework/global/modularity/ioc.h"
#include "framework/global/io/ifilesystem.h"
#include "framework/languages/ilanguagesconfiguration.h"

#include "iexperienceservice.h"
#include "iexperienceconfiguration.h"
#include "experiencetranslator.h"

namespace au::experience {
class ExperienceService : public IExperienceService, public muse::async::Asyncable
{
    muse::GlobalInject<IExperienceConfiguration> configuration;
    muse::GlobalInject<muse::io::IFileSystem> fileSystem;
    muse::GlobalInject<muse::languages::ILanguagesConfiguration> languagesConfiguration;

public:
    ExperienceService() = default;
    ~ExperienceService() override;

    void init();

    VocabularyLoadResult loadVocabularyFile(const QString& filePath) override;
    void clearVocabulary() override;
    int vocabularyEntryCount() const override;

    bool restartRequired() const override;
    muse::async::Notification restartRequiredChanged() const override;

    bool cantoneseCatalogueAvailable() const override;

private:
    void applyLanguageMode();
    void applyLowStimulation();
    void refreshTranslator();
    void setRestartRequired(bool value);
    void loadStoredVocabulary();

    ExperienceTranslator* m_translator = nullptr;
    bool m_translatorInstalled = false;
    bool m_cantoneseAvailable = false;
    bool m_restartRequired = false;
    bool m_lowStimulationApplied = false;
    muse::async::Notification m_restartRequiredChanged;
};
}
