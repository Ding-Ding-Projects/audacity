#include "converterpresentationmodel.h"
#include "convertercatalog.h"
#include <QtConcurrent>
using namespace au::converter;
void ConverterPresentationModel::probe() { start({}, true); }
void ConverterPresentationModel::convert(const QString& input, const QString& output, const QString& format)
{
    if (input.isEmpty() || output.isEmpty() || format.isEmpty()) { m_status = "Choose explicit input, output, and format paths."; emit changed(); return; }
    ConversionRequest request { input, output, format, false }; start(request, false);
}
void ConverterPresentationModel::cancel() { m_cancel->store(true); }
void ConverterPresentationModel::start(const ConversionRequest& request, bool probeOnly)
{
    if (m_busy) return; m_busy = true; m_progress = 0; m_status = probeOnly ? "Checking bundled capabilities…" : "Converting without overwrite…"; m_cancel = std::make_shared<std::atomic_bool>(false); const auto cancel = m_cancel; emit changed();
    auto* watcher = new QFutureWatcher<ConversionResult>(this);
    connect(watcher, &QFutureWatcher<ConversionResult>::finished, this, [this, watcher, probeOnly] { const auto result = watcher->result(); m_busy = false; m_progress = 100; m_status = result.message; emit changed(); emit finished(probeOnly ? result.status == ConversionStatus::Converted : result.status == ConversionStatus::Converted, result.message); watcher->deleteLater(); });
    watcher->setFuture(QtConcurrent::run([request, probeOnly, cancel] { if (probeOnly) { const auto adapters = ConverterCatalog::adapters(); for (const auto& adapter : adapters) if (adapter.enabled) return ConversionResult { ConversionStatus::Converted, {}, QStringLiteral("Bundled capability available: %1").arg(adapter.displayName) }; return ConversionResult { ConversionStatus::Rejected, {}, QStringLiteral("No bundled converter capability is currently available.") }; } return ConversionEngine().convert(request, cancel.get()); }));
}
