#include "profilepaths.h"
extern "C" Q_DECL_EXPORT bool consumerProfileActive() { return au::profile::Paths::active(); }
extern "C" Q_DECL_EXPORT QString consumerProfileRoot() { return au::profile::Paths::root(); }
