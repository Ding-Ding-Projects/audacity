/* Audacity: A Digital Audio Editor */
#include "narratorservice.h"
#ifdef Q_OS_WIN
#include <windows.h>
#include <sapi.h>
#include <sphelper.h>
#include <QStringList>
namespace au::experience {
namespace {
class SapiBackend final : public NarratorBackend {
public:
    SapiBackend() {
        const HRESULT initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        m_uninitialize = SUCCEEDED(initialized);
        if (FAILED(initialized) && initialized != RPC_E_CHANGED_MODE) return;
        CoCreateInstance(CLSID_SpVoice, nullptr, CLSCTX_INPROC_SERVER, IID_ISpVoice, reinterpret_cast<void**>(&m_voice));
    }
    ~SapiBackend() override {
        cancel();
        if (m_voice) m_voice->Release();
        if (m_uninitialize) CoUninitialize();
    }
    NarratorEngineKind kind() const override { return m_voice ? NarratorEngineKind::WindowsSapi : NarratorEngineKind::None; }
    QVector<NarratorVoice> voices() const override {
        QVector<NarratorVoice> result;
        if (!m_voice) return result;
        IEnumSpObjectTokens* enumerator = nullptr;
        if (FAILED(SpEnumTokens(SPCAT_VOICES, nullptr, nullptr, &enumerator))) return result;
        ISpObjectToken* token = nullptr;
        int examined = 0;
        while (examined++ < 256 && enumerator->Next(1, &token, nullptr) == S_OK) {
            ISpDataKey* attributes = nullptr;
            WCHAR* language = nullptr;
            WCHAR* id = nullptr;
            WCHAR* description = nullptr;
            if (SUCCEEDED(token->OpenKey(L"Attributes", &attributes))) attributes->GetStringValue(L"Language", &language);
            token->GetId(&id);
            SpGetDescription(token, &description);
            if (id && language) {
                for (const auto& code : QString::fromWCharArray(language).split(';')) {
                    bool valid = false;
                    const uint locale = code.toUInt(&valid, 16);
                    if (!valid) continue;
                    NarratorLanguage mapped;
                    if ((locale & 0x3ff) == LANG_ENGLISH) mapped = NarratorLanguage::English;
                    else if (locale == 0x0c04 || locale == 0x1404) mapped = NarratorLanguage::Cantonese;
                    else continue; // Mandarin is never mislabeled as Cantonese.
                    result.push_back({ QString::fromWCharArray(id), description ? QString::fromWCharArray(description)
                        : QString::fromWCharArray(id), mapped });
                    break;
                }
            }
            CoTaskMemFree(language); CoTaskMemFree(id); CoTaskMemFree(description);
            if (attributes) attributes->Release();
            token->Release(); token = nullptr;
        }
        enumerator->Release();
        return result;
    }
    bool start(const QString& text, const QString& voiceId, double rate, double pitch) override {
        if (!m_voice) return false;
        ISpObjectToken* token = nullptr;
        if (FAILED(SpGetTokenFromId(reinterpret_cast<const wchar_t*>(voiceId.utf16()), &token, FALSE))) return false;
        const HRESULT selected = m_voice->SetVoice(token);
        token->Release();
        if (FAILED(selected) || FAILED(m_voice->SetRate(qRound(rate * 10)))) return false;
        // Escape user content before adding the documented SAPI XML pitch wrapper.
        const QString xml = QStringLiteral("<pitch absmiddle=\"%1\">%2</pitch>")
            .arg(qRound(pitch * 10)).arg(text.toHtmlEscaped());
        return SUCCEEDED(m_voice->Speak(reinterpret_cast<const wchar_t*>(xml.utf16()), SPF_ASYNC | SPF_IS_XML, nullptr));
    }
    State poll() override {
        SPVOICESTATUS status {};
        if (!m_voice || FAILED(m_voice->GetStatus(&status, nullptr)) || FAILED(status.hrLastResult)) return State::Failed;
        return status.dwRunningState == SPRS_DONE ? State::Finished : State::Speaking;
    }
    void cancel() override {
        if (m_voice) m_voice->Speak(nullptr, SPF_ASYNC | SPF_PURGEBEFORESPEAK, nullptr);
    }
private:
    ISpVoice* m_voice = nullptr;
    bool m_uninitialize = false;
};
}
std::unique_ptr<NarratorBackend> makeNativeNarratorBackend() { return std::make_unique<SapiBackend>(); }
}
#else
namespace au::experience {
std::unique_ptr<NarratorBackend> makeNativeNarratorBackend() { return {}; }
}
#endif

