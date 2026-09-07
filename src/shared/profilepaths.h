/* Audacity: isolated verification profiles. */
#pragma once
#include <QStandardPaths>
#include <QStringList>
#if defined(AUDACITY_PROFILEPATHS_SHARED)
# if defined(AUDACITY_PROFILEPATHS_BUILD)
#  define AU_PROFILE_API Q_DECL_EXPORT
# else
#  define AU_PROFILE_API Q_DECL_IMPORT
# endif
#else
# define AU_PROFILE_API
#endif
namespace au::profile {
// One compiled provider in muse_global. No header-local mutable state.
class AU_PROFILE_API Paths final {
public:
    static bool initialize(const QString& root, QString* error);
    static bool initializeArguments(const QStringList& arguments, QString* error);
    static void settingsAccessed();
    static bool active();
    static QString root();
    static QString writableLocation(QStandardPaths::StandardLocation location);
    static QString temporaryPath();
    static QString ipcName(const QString& name);
    static QStringList childArguments(const QStringList& arguments);
    static bool contains(const QString& parent, const QString& child);
};
}
