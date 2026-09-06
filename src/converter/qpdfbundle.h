/* Pinned files from the SHA-256 verified official qpdf 12.3.2 archive. */
#pragma once
#include <QMap>
#include <QString>
namespace au::converter::detail {
inline const QMap<QString, QString> QpdfFiles {
    { QStringLiteral("concrt140.dll"), QStringLiteral("2405355f0a58067b258f8df33c327e3a3d716eaac5a3a5aebb757842d85bd376") },
    { QStringLiteral("msvcp140_1.dll"), QStringLiteral("bfad5aef4c63a669e3c140655cdfdf395b6c979b400a447bd5dcb65ed8826c3d") },
    { QStringLiteral("msvcp140_2.dll"), QStringLiteral("3ea06f0ee098b4823cb79599df3780e7f23cce52c19aac31d2a0d47efe33a5e9") },
    { QStringLiteral("msvcp140_atomic_wait.dll"), QStringLiteral("640b2aefced484d0368eea5bdd06addd0658a3a70a49256e560d6923b404a479") },
    { QStringLiteral("msvcp140_codecvt_ids.dll"), QStringLiteral("f2069a52880ec885ee7f0511186100eb7fada0411a2b4948fafea7735b878a18") },
    { QStringLiteral("msvcp140.dll"), QStringLiteral("0f885b509a685d2bbfa652fed26b5fb31d88fbdab0a978c641d1c7b8aa460aa9") },
    { QStringLiteral("qpdf.exe"), QStringLiteral("43f79db620ce09529a67572a5de87aec4065b95f11ba6e5918db557f943a7eac") },
    { QStringLiteral("qpdf30.dll"), QStringLiteral("623338ff5a9caab476f9e80ccc40c28c194208f4bc5d8e51eac7fca792e2e969") },
    { QStringLiteral("vcruntime140_1.dll"), QStringLiteral("1f2d41c4aa5db0bc33ebf7b66d72943a817d7ce6cbe880502a9403823633093f") },
    { QStringLiteral("vcruntime140.dll"), QStringLiteral("d5e4d9a3e835fa679450145d6a7d94e36573a509317111904d9b3712c30d9066") },
};
}
