#pragma once

#include "raft_package_asio.hxx"

inline int launch_servers(const std::vector<RaftAsioPkg*>& pkgs,
                          bool enable_ssl,
                          bool use_global_asio = false,
                          bool use_bg_snapshot_io = true,
                          const raft_server::init_options& opt =
                              raft_server::init_options(),
                          size_t initial_sleep_time_ms = 1000,
                          int32_t streaming_mode_gap = 0)
{
    size_t num_srvs = pkgs.size();
    CHK_GT(num_srvs, 0);

    for (auto& entry: pkgs) {
        RaftAsioPkg* pp = entry;
        pp->initServer(enable_ssl, use_global_asio, use_bg_snapshot_io, opt,
                       streaming_mode_gap > 0);
        if (streaming_mode_gap > 0) {
            raft_params param = pp->raftServer->get_current_params();
            param.max_log_gap_in_stream_ = streaming_mode_gap;
            pp->raftServer->update_params(param);
        }
    }

    // Wait longer than upper timeout.
    TestSuite::sleep_ms(initial_sleep_time_ms);
    return 0;
}

inline int make_group(const std::vector<RaftAsioPkg*>& pkgs) {
    size_t num_srvs = pkgs.size();
    CHK_GT(num_srvs, 0);

    RaftAsioPkg* leader = pkgs[0];

    for (size_t ii = 1; ii < num_srvs; ++ii) {
        RaftAsioPkg* ff = pkgs[ii];

        // Add to leader.
        leader->raftServer->add_srv( *(ff->getTestMgr()->get_srv_config()) );

        // Wait longer than upper timeout.
        TestSuite::sleep_sec(1);
    }
    return 0;
}

inline void async_handler(std::list<ulong>* idx_list,
                          std::mutex* idx_list_lock,
                          ptr<buffer>& result,
                          ptr<std::exception>& err)
{
    if (!result.get()) {
        // It may be null during shutdown.
        return;
    }
    result->pos(0);
    ulong idx = result->get_ulong();
    if (idx_list) {
        std::lock_guard<std::mutex> l(*idx_list_lock);
        idx_list->push_back(idx);
    }
}

