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
#include "languagepagemodel.h"

using namespace au::appshell;

static const muse::Settings::Key BILINGUAL_KEY("ui", "ui/language/bilingual");

LanguagePageModel::LanguagePageModel(QObject* parent)
    : QObject(parent)
{
    settings()->setDefaultValue(BILINGUAL_KEY, muse::Val(false));
}

QString LanguagePageModel::currentLanguageCode() const
{
    return languagesConfiguration()->currentLanguageCode().val;
}

void LanguagePageModel::setCurrentLanguageCode(const QString& code)
{
    if (code == currentLanguageCode()) {
        return;
    }

    languagesConfiguration()->setCurrentLanguageCode(code);
    emit currentLanguageChanged();
}

bool LanguagePageModel::bilingual() const
{
    return settings()->value(BILINGUAL_KEY).toBool();
}

void LanguagePageModel::setBilingual(bool bilingual)
{
    if (bilingual == this->bilingual()) {
        return;
    }

    settings()->setSharedValue(BILINGUAL_KEY, muse::Val(bilingual));
    emit currentLanguageChanged();
}
