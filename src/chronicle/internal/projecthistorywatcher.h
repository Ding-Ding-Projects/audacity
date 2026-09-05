/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include "framework/global/modularity/ioc.h"
#include "framework/global/async/asyncable.h"

#include "context/iglobalcontext.h"

#include "iversionhistoryservice.h"

namespace au::chronicle {
/*!
 * Watches the open project and asks the version history to record a revision
 * when something worth recording happens.
 *
 * It lives in the module's context setup rather than in the service, because
 * the global context is a context scoped interface while the history service
 * itself is global and outlives any one window.
 */
class ProjectHistoryWatcher : public muse::async::Asyncable, public muse::Contextable
{
    muse::ContextInject<au::context::IGlobalContext> globalContext { this };
    muse::GlobalInject<IVersionHistoryService> versionHistory;

public:
    explicit ProjectHistoryWatcher(const muse::modularity::ContextPtr& ctx)
        : muse::Contextable(ctx) {}

    void init();

private:
    void onCurrentProjectChanged();

    //! Guards against recording the same save twice when several
    //! notifications arrive for one action.
    bool m_lastNeedSave = false;
};
}
