/*
 * Copyright (c) 2026
 *
 * This file is part of CasparCG (www.casparcg.com).
 *
 * CasparCG is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "ember_command_bridge.h"
#include "ember_system_info.h"
#include "ember_registry.h"

#include "../amcp/amcp_command_repository.h"
#include "../amcp/amcp_shared.h"
#include "../util/protocol_strategy.h"

#include <common/memory.h>

#include <chrono>
#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace libember { namespace dom {
class Node;
}}
namespace libember { namespace glow {
class Value;
}}

namespace caspar { namespace protocol { namespace ember {

class ember_provider final
{
  public:
    struct clip_command_state
    {
        int          layer               = 1;
        bool         loop                = false;
        std::wstring file_name;
        int          seek                = 0;
        int          length              = 0;
        std::wstring filter;
        bool         clear_on_404        = false;
        bool         auto_play           = false;
        int          transition          = 0;
        int          transition_duration = 12;
        std::wstring tween;
        int          direction           = 0;
        bool         sting_enabled       = false;
        std::wstring sting_mask;
        int          sting_trigger_point = 0;
        std::wstring sting_overlay;
        int          sting_audio_fade_start    = 0;
        int          sting_audio_fade_duration = 0;
        std::wstring last_reply;
        bool         last_success        = true;
    };

    struct layer_command_state
    {
        int          layer        = 1;
        std::wstring last_reply;
        bool         last_success = true;
    };

    struct clear_command_state
    {
        int          layer        = 0;
        std::wstring last_reply;
        bool         last_success = true;
    };

    struct refresh_command_state
    {
        std::wstring last_reply;
        bool         last_success = true;
    };

    struct call_command_state
    {
        int          layer        = 1;
        std::wstring arguments;
        std::wstring last_reply;
        bool         last_success = true;
    };

    explicit ember_provider(const spl::shared_ptr<std::vector<amcp::channel_context>>& channels,
                            const std::shared_ptr<amcp::amcp_command_repository>&      amcp_command_repository,
                            std::chrono::milliseconds                                   monitor_interval = std::chrono::milliseconds(1000),
                            std::shared_ptr<ember_registry> registry = std::make_shared<ember_registry>());
    ~ember_provider();

    void send_provider_state(const IO::client_connection<char>::ptr& client, bool online = true) const;
    void handle_request(libember::dom::Node* request_root, const std::shared_ptr<ember_session>& session);

    const std::shared_ptr<ember_registry>& registry() const;

  private:
    void register_session(const std::shared_ptr<ember_session>& session);
    std::vector<std::shared_ptr<ember_session>> active_sessions() const;
    std::vector<IO::client_connection<char>::ptr> active_clients() const;
    void monitor_layer_changes();
    void meter_stream_loop();
    long monitor_interval_ms() const;
    void set_monitor_interval(std::chrono::milliseconds interval);
    void broadcast_monitor_interval_update(long interval_ms) const;
    void broadcast_directory_response() const;
    void send_directory_response(const IO::client_connection<char>::ptr& client) const;
    bool handle_subscription_command(const std::vector<int>&              path,
                                     int                                  command_number,
                                     const std::shared_ptr<ember_session>& session) const;
    void send_audio_meter_streams(const std::shared_ptr<ember_session>& session) const;
    bool handle_parameter_write(const std::vector<int>&              path,
                                const libember::glow::Value&         value,
                                const std::shared_ptr<ember_session>& session);
    bool handle_live_layer_mixer_write(const std::vector<int>&              path,
                                       const libember::glow::Value&         value,
                                       const std::shared_ptr<ember_session>& session);
    spl::shared_ptr<std::vector<amcp::channel_context>> channels_;
    std::shared_ptr<ember_registry>                     registry_;
    ember_command_bridge                                command_bridge_;
    mutable std::mutex                                  clip_commands_mutex_;
    std::map<int, clip_command_state>                   play_controls_;
    std::map<int, clip_command_state>                   loadbg_controls_;
    std::map<int, clip_command_state>                   load_controls_;
    std::map<int, layer_command_state>                  pause_controls_;
    std::map<int, layer_command_state>                  resume_controls_;
    std::map<int, layer_command_state>                  stop_controls_;
    std::map<int, clear_command_state>                  clear_controls_;
    std::map<int, refresh_command_state>                refresh_controls_;
    std::map<int, call_command_state>                   call_controls_;
    std::map<int, call_command_state>                   callbg_controls_;
    mutable std::mutex                                  media_clips_mutex_;
    std::vector<std::wstring>                           media_clips_;
    mutable std::mutex                                  sessions_mutex_;
    mutable std::vector<std::weak_ptr<ember_session>>   sessions_;
    std::atomic<bool>                                   stop_monitor_{false};
    mutable std::mutex                                  monitor_interval_mutex_;
    std::condition_variable                             monitor_interval_cv_;
    std::chrono::milliseconds                           monitor_interval_;
    bool                                                monitor_interval_updated_ = false;
    mutable std::mutex                                  configuration_mutex_;
    mutable ember_system_info_sampler                   system_info_sampler_;
    std::thread                                         monitor_thread_;
    std::thread                                         meter_thread_;
};

}}} // namespace caspar::protocol::ember
