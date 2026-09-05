/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore BVBA and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef AU_APPSHELL_LANGUAGEPAGEMODEL_H
#define AU_APPSHELL_LANGUAGEPAGEMODEL_H

#include <QObject>
#include <QtQml/qqmlregistration.h>

#include "modularity/ioc.h"
#include "framework/global/settings.h"
#include "framework/languages/ilanguagesconfiguration.h"

namespace au::appshell {
//! NOTE The first launch language choice. English and Cantonese (Hong Kong)
//! are ordinary language codes. The bilingual choice is kept as its own
//! setting until the bilingual strings land.
class LanguagePageModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    muse::GlobalInject<muse::languages::ILanguagesConfiguration> languagesConfiguration;

    Q_PROPERTY(QString currentLanguageCode READ currentLanguageCode WRITE setCurrentLanguageCode NOTIFY currentLanguageChanged)
    Q_PROPERTY(bool bilingual READ bilingual WRITE setBilingual NOTIFY currentLanguageChanged)

public:
    explicit LanguagePageModel(QObject* parent = nullptr);

    QString currentLanguageCode() const;
    void setCurrentLanguageCode(const QString& code);

    bool bilingual() const;
    void setBilingual(bool bilingual);

signals:
    void currentLanguageChanged();
};
}

#endif // AU_APPSHELL_LANGUAGEPAGEMODEL_H
