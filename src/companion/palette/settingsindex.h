/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace au::companion {
/*!
 * \brief The hand written index of the preferences surface.
 *
 * The palette cannot discover the preferences controls by itself, because the
 * preferences pages are ordinary QML that is only instantiated once the dialog
 * is open. The index in \c settingsindex.json names every control instead:
 * the page it lives on, the group it sits in, its label, its control type, the
 * muse setting key behind it when there is one, and the label the teleport
 * highlighter looks for inside the page.
 *
 * The guard test in \c tests/palette_index_tests.cpp reads the same file and
 * the preferences QML next to it, and fails when a control has been added to
 * the QML without a row here.
 */
class SettingsIndex
{
public:
    //! Loads the index from the module resources. Returns an empty index and
    //! logs when the resource is missing or malformed.
    static const SettingsIndex& instance();

    //! Loads an index from an explicit JSON document. Used by the tests.
    static SettingsIndex fromJson(const QByteArray& json, QString* error = nullptr);

    //! One row per preferences page.
    const QVariantList& pages() const { return m_pages; }
    //! One row per preferences control.
    const QVariantList& settings() const { return m_settings; }
    //! The appearance controls the palette drives directly.
    const QVariantList& appearance() const { return m_appearance; }

    bool isEmpty() const { return m_settings.isEmpty() && m_appearance.isEmpty(); }

    //! Every distinct teleport target label in the index.
    QStringList targets() const;

private:
    QVariantList m_pages;
    QVariantList m_settings;
    QVariantList m_appearance;
};
}
