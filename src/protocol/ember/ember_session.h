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

#include "../util/ClientInfo.h"
#include "../util/protocol_strategy.h"

#include <common/memory.h>

#include <memory>
#include <string>

namespace caspar { namespace protocol { namespace ember {

struct ember_session_capture_state;

class ember_session final
{
  public:
    explicit ember_session(const IO::client_connection<char>::ptr& client);

    const IO::client_connection<char>::ptr& client() const;
    IO::ClientInfoPtr                       amcp_client() const;
    std::wstring                           take_amcp_output() const;

  private:
    std::shared_ptr<ember_session_capture_state> capture_state_;
    IO::client_connection<char>::ptr             client_;
    IO::ClientInfoPtr                            amcp_client_;
};

}}} // namespace caspar::protocol::ember
