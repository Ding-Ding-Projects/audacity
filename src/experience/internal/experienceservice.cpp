/*
 * Audacity: A Digital Audio Editor
 */
#include "experienceservice.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>

#include "framework/global/settings.h"
#include "framework/global/translation.h"
#include "log.h"

using namespace muse;

namespace au::experience {
namespace {
static const std::string moduleName("experience");

static const Settings::Key LANGUAGE_CODE("languages", "language");
static const Settings::Key M3_VARIANT("ui", "ui/m3/variant");
static const Settings::Key M3_REDUCED_MOTION("ui", "ui/m3/reducedMotion");

static const Settings::Key SAVED_VARIANT(moduleName, "experience/modes/savedVariant");
static const Settings::Key SAVED_REDUCED_MOTION(moduleName, "experience/modes/savedReducedMotion");
static const Settings::Key LOW_STIMULATION_APPLIED(moduleName, "experience/modes/lowStimulationApplied");

//! The desaturated scheme variant the low stimulation mode uses.
static const std::string CALM_VARIANT("neutral");

static const QString ENGLISH_CODE = QStringLiteral("en_US");
static const QString CANTONESE_CODE = QStringLiteral("yue_HK");
}

ExperienceService::~ExperienceService()
{
    if (m_translator && m_translatorInstalled && QCoreApplication::instance()) {
        QCoreApplication::instance()->removeTranslator(m_translator);
    }
    delete m_translator;
}

void ExperienceService::init()
{
    settings()->setDefaultValue(SAVED_VARIANT, Val(std::string()));
    settings()->setDefaultValue(SAVED_REDUCED_MOTION, Val(false));
    settings()->setDefaultValue(LOW_STIMULATION_APPLIED, Val(false));

    m_lowStimulationApplied = settings()->value(LOW_STIMULATION_APPLIED).toBool();

    m_translator = new ExperienceTranslator();
    m_schoolMode = new SchoolModeService(this);

    QStringList cataloguePaths;
    for (const QString& resourceName : languagesConfiguration()->languageResourceNames()) {
        const io::path_t builtin = languagesConfiguration()->builtinLanguageFilePath(resourceName, CANTONESE_CODE);
        if (fileSystem()->exists(builtin)) {
            cataloguePaths.append(builtin.toQString());
        }

        const io::path_t user = languagesConfiguration()->userLanguageFilePath(resourceName, CANTONESE_CODE);
        if (fileSystem()->exists(user)) {
            cataloguePaths.append(user.toQString());
        }
    }

    m_cantoneseAvailable = m_translator->loadCantoneseCatalogues(cataloguePaths) > 0;
    if (!m_cantoneseAvailable) {
        LOGW() << "No Cantonese (Hong Kong) catalogue was found, bilingual mode will show English only";
    }

    applySchoolMode();
    applyLanguageMode();
    refreshTranslator();
    applyLowStimulation();

    configuration()->languageModeChanged().onReceive(this, [this](LanguageMode) {
        applyLanguageMode();
        refreshTranslator();
        setRestartRequired(true);
    });

    connect(m_schoolMode, &SchoolModeService::stateChanged, this, [this]() {
        // The shared record can be changed by another participating
        // application. Apply it here so the active surface updates live.
        applySchoolMode();
        applyLanguageMode();
        refreshTranslator();
    });

    configuration()->attentionModesChanged().onNotify(this, [this]() {
        applyLowStimulation();
    });
}

void ExperienceService::applyLanguageMode()
{
    const LanguageMode mode = effectiveLanguageMode();
    const QString code = mode == LanguageMode::Cantonese ? CANTONESE_CODE : ENGLISH_CODE;
    settings()->setSharedValue(LANGUAGE_CODE, Val(code.toStdString()));
}

LanguageMode ExperienceService::effectiveLanguageMode() const
{
    return m_schoolMode && m_schoolMode->isOn() ? LanguageMode::English : configuration()->languageMode();
}

void ExperienceService::applySchoolMode()
{
    if (m_schoolMode && m_schoolMode->isOn()) {
        // The stored table remains on disk. Clearing only the live table keeps
        // the lock reversible and restores the user's exact prior choice.
        if (m_translator) {
            m_translator->setVocabulary({});
        }
        return;
    }

    loadStoredVocabulary();
}

void ExperienceService::refreshTranslator()
{
    if (!m_translator || !QCoreApplication::instance()) {
        return;
    }

    m_translator->setLanguageMode(effectiveLanguageMode());

    const bool wanted = !m_translator->isEmpty();
    if (wanted && !m_translatorInstalled) {
        QCoreApplication::instance()->installTranslator(m_translator);
        m_translatorInstalled = true;
    } else if (!wanted && m_translatorInstalled) {
        QCoreApplication::instance()->removeTranslator(m_translator);
        m_translatorInstalled = false;
    } else if (wanted) {
        //! Reinstalling makes Qt send a LanguageChange event, which is what
        //! asks the interface to look up its strings again.
        QCoreApplication::instance()->removeTranslator(m_translator);
        QCoreApplication::instance()->installTranslator(m_translator);
    }
}

void ExperienceService::applyLowStimulation()
{
    const bool wanted = configuration()->lowStimulationMode();
    if (wanted == m_lowStimulationApplied) {
        return;
    }

    if (wanted) {
        //! Remember what the reader chose, so that turning the mode off gives
        //! it back rather than guessing a default.
        settings()->setSharedValue(SAVED_VARIANT, settings()->value(M3_VARIANT));
        settings()->setSharedValue(SAVED_REDUCED_MOTION, settings()->value(M3_REDUCED_MOTION));
        settings()->setSharedValue(M3_VARIANT, Val(CALM_VARIANT));
        settings()->setSharedValue(M3_REDUCED_MOTION, Val(true));
    } else {
        const std::string savedVariant = settings()->value(SAVED_VARIANT).toString();
        if (!savedVariant.empty()) {
            settings()->setSharedValue(M3_VARIANT, Val(savedVariant));
        }
        settings()->setSharedValue(M3_REDUCED_MOTION, settings()->value(SAVED_REDUCED_MOTION));
    }

    m_lowStimulationApplied = wanted;
    settings()->setSharedValue(LOW_STIMULATION_APPLIED, Val(wanted));
}

void ExperienceService::loadStoredVocabulary()
{
    const QString path = configuration()->vocabularyStoragePath();
    QFile file(path);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return;
    }

    const QByteArray data = file.readAll();
    file.close();

    const PersonalVocabulary::ParseResult parsed = PersonalVocabulary::parse(data);
    if (parsed.ok && m_translator) {
        m_translator->setVocabulary(parsed.entries);
    }
}

VocabularyLoadResult ExperienceService::loadVocabularyFile(const QString& filePath)
{
    VocabularyLoadResult result;
    result.fileName = QFileInfo(filePath).fileName();

    QFile file(filePath);
    if (!file.exists()) {
        result.error = muse::qtrc("experience", "The file could not be found.");
        return result;
    }

    if (file.size() > PersonalVocabulary::MAX_BYTES) {
        result.error = muse::qtrc("experience", "The file is larger than 256 KB.");
        return result;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        result.error = muse::qtrc("experience", "The file could not be opened.");
        return result;
    }

    const QByteArray data = file.readAll();
    file.close();

    const PersonalVocabulary::ParseResult parsed = PersonalVocabulary::parse(data);
    if (!parsed.ok) {
        //! Only the reason is reported. The words themselves stay private.
        result.error = parsed.error;
        return result;
    }

    const io::path_t storagePath = io::path_t(configuration()->vocabularyStoragePath());
    fileSystem()->makePath(io::dirpath(storagePath));

    QFile storage(configuration()->vocabularyStoragePath());
    if (!storage.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        result.error = muse::qtrc("experience", "The vocabulary could not be stored.");
        return result;
    }
    storage.write(PersonalVocabulary::serialize(parsed.entries));
    storage.close();

    if (m_translator) {
        m_translator->setVocabulary(parsed.entries);
    }
    refreshTranslator();

    configuration()->setVocabularyFileName(result.fileName);
    setRestartRequired(true);

    result.ok = true;
    result.entryCount = parsed.entries.size();
    return result;
}

void ExperienceService::clearVocabulary()
{
    QFile::remove(configuration()->vocabularyStoragePath());

    if (m_translator) {
        m_translator->setVocabulary({});
    }
    refreshTranslator();

    configuration()->setVocabularyFileName(QString());
    setRestartRequired(true);
}

int ExperienceService::vocabularyEntryCount() const
{
    return m_translator ? m_translator->vocabulary().size() : 0;
}

bool ExperienceService::restartRequired() const
{
    return m_restartRequired;
}

muse::async::Notification ExperienceService::restartRequiredChanged() const
{
    return m_restartRequiredChanged;
}

bool ExperienceService::cantoneseCatalogueAvailable() const
{
    return m_cantoneseAvailable;
}

void ExperienceService::setRestartRequired(bool value)
{
    if (m_restartRequired == value) {
        return;
    }
    m_restartRequired = value;
    m_restartRequiredChanged.notify();
}
}
