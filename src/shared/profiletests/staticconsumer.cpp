#include "profilepaths.h"
bool staticConsumerActive() { return au::profile::Paths::active(); }
QString staticConsumerRoot() { return au::profile::Paths::root(); }
