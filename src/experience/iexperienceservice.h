/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include "framework/global/modularity/imoduleinterface.h"
#include "framework/global/async/notification.h"

#include "experiencetypes.h"

namespace au::experience {
//! Turns the companion settings into behaviour: the language mode, the extra
//! translator, the personal vocabulary and the low stimulation scheme.
class IExperienceService : MODULE_GLOBAL_INTERFACE
{
    INTERFACE_ID(au::experience::IExperienceService)

public:
    virtual ~IExperienceService() = default;

    //! Reads the file, stores the parsed table in the application data
    //! directory and installs it. The file contents are never logged.
    virtual VocabularyLoadResult loadVocabularyFile(const QString& filePath) = 0;
    virtual void clearVocabulary() = 0;
    virtual int vocabularyEntryCount() const = 0;

    //! True when a change has been made that the running interface cannot show
    //! in full until the application is restarted.
    virtual bool restartRequired() const = 0;
    virtual muse::async::Notification restartRequiredChanged() const = 0;

    //! True when the Cantonese catalogue could not be found, so bilingual mode
    //! has nothing to compose with.
    virtual bool cantoneseCatalogueAvailable() const = 0;
};
}
