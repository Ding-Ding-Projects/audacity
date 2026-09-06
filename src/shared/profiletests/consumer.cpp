#include "profilepaths.h"
Q_DECL_EXPORT bool consumerProfileActive() { return au::profile::Paths::active(); }
Q_DECL_EXPORT QString consumerProfileRoot() { return au::profile::Paths::root(); }
