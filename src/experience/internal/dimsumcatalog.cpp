/*
 * Audacity: A Digital Audio Editor
 */
#include "dimsumcatalog.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

using namespace au::experience;

QString DimSumDish::bilingualLabel() const
{
    return nameEnglish + QStringLiteral(" · ") + nameTraditionalChinese;
}

QVector<DimSumDish> DimSumCatalog::parse(const QByteArray& json)
{
    QVector<DimSumDish> result;

    const QJsonDocument doc = QJsonDocument::fromJson(json);
    if (!doc.isObject()) {
        return result;
    }

    const QJsonArray dishes = doc.object().value(QStringLiteral("dishes")).toArray();
    for (const QJsonValue& value : dishes) {
        if (!value.isObject()) {
            continue;
        }

        const QJsonObject obj = value.toObject();
        const QJsonObject name = obj.value(QStringLiteral("name")).toObject();

        DimSumDish dish;
        dish.id = obj.value(QStringLiteral("id")).toString();
        dish.nameEnglish = name.value(QStringLiteral("en")).toString();
        dish.nameTraditionalChinese = name.value(QStringLiteral("zhHant")).toString();

        // The catalog's own schema names a dish's release asset directly as
        // "photoAsset" in a minimal fixture, or nests it under an "image"
        // object with a repository-relative "path" in the real published
        // catalog. Accept either; when only a path is given, the asset file
        // name is that path's own base name.
        dish.photoAsset = obj.value(QStringLiteral("photoAsset")).toString();
        if (dish.photoAsset.isEmpty()) {
            const QString imagePath = obj.value(QStringLiteral("image")).toObject()
                                      .value(QStringLiteral("path")).toString();
            if (!imagePath.isEmpty()) {
                dish.photoAsset = QFileInfo(imagePath).fileName();
            }
        }

        if (dish.isValid()) {
            result.push_back(dish);
        }
    }

    return result;
}
