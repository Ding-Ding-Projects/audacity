/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>

namespace au::experience {
//! One dish entry read from the public dim sum catalog
//! (Ding-Ding-Projects/dim-sum-photos, catalog/index.json).
struct DimSumDish
{
    QString id;
    QString nameEnglish;
    QString nameTraditionalChinese;
    //! Asset file name inside a published catalog-v1* release, used to
    //! resolve the downloadable photo URL. May be empty if the catalog
    //! record carries no photo yet.
    QString photoAsset;

    bool isValid() const { return !id.isEmpty() && !nameEnglish.isEmpty() && !nameTraditionalChinese.isEmpty(); }

    //! "Shrimp dumpling . Ha Gow" style bilingual label. Uses a middle dot
    //! separator; never mixes in decorative punctuation that could be
    //! mistaken for part of either name.
    QString bilingualLabel() const;
};

//! Parses the public catalog's index.json into a list of dishes. Never
//! throws; a malformed or empty document simply yields an empty list.
class DimSumCatalog
{
public:
    static QVector<DimSumDish> parse(const QByteArray& json);
};
}
