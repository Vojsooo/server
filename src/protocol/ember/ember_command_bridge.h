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

#include "ember_session.h"

#include "../amcp/amcp_command_repository.h"

#include <common/executor.h>

#include <memory>
#include <string>
#include <vector>

namespace libember { namespace glow {
class GlowFunctionBase;
class GlowInvocation;
}}

namespace caspar { namespace protocol { namespace ember {

class ember_command_bridge final
{
  public:
    struct invocation_result
    {
        bool         success = false;
        std::wstring message;
    };

    struct play_request
    {
        int          channel_index         = 1;
        int          layer                 = 1;
        bool         loop                  = false;
        std::wstring file_name;
        int          seek                  = 0;
        int          length                = 0;
        std::wstring filter;
        bool         clear_on_404          = false;
        int          transition            = 0;
        int          transition_duration   = 12;
        std::wstring tween;
        int          direction             = 0;
    };

    explicit ember_command_bridge(const std::shared_ptr<amcp::amcp_command_repository>& repo);

    static std::vector<int> amcp_execute_path();
    static void             describe_amcp_execute_function(libember::glow::GlowFunctionBase* function);

    bool supports_function(const std::vector<int>& path) const;
    invocation_result execute_native_command_line(const std::wstring&            command_line,
                                                  const std::shared_ptr<ember_session>& session);
    invocation_result execute_play(const play_request&                 request,
                                   const std::shared_ptr<ember_session>& session);

    invocation_result invoke(const std::vector<int>&                 path,
                             const libember::glow::GlowInvocation&   invocation,
                             const std::shared_ptr<ember_session>& session);

  private:
    invocation_result invoke_amcp_execute(const libember::glow::GlowInvocation&   invocation,
                                          const std::shared_ptr<ember_session>& session);
    invocation_result execute_command_line(const std::wstring&                  command_line,
                                           const std::shared_ptr<ember_session>& session);
    invocation_result run_command(const std::shared_ptr<amcp::AMCPCommand>& command,
                                  const std::shared_ptr<ember_session>&      session) const;
    int               queue_index_for(const std::shared_ptr<amcp::AMCPCommand>& command) const;

    std::shared_ptr<amcp::amcp_command_repository> repo_;
    std::vector<std::unique_ptr<caspar::executor>> queues_;
};

}}} // namespace caspar::protocol::ember
