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

#include "../StdAfx.h"

#include "ember_session.h"

#include <mutex>
#include <set>

namespace caspar { namespace protocol { namespace ember {

struct ember_session_capture_state
{
    explicit ember_session_capture_state(IO::client_connection<char>::ptr client)
        : client(std::move(client))
    {
    }

    IO::client_connection<char>::ptr client;
    mutable std::mutex               mutex;
    std::wstring                     output;
    std::set<int>                    stream_subscriptions;
};

namespace {

class amcp_capture_client_connection final : public IO::client_connection<wchar_t>
{
  public:
    explicit amcp_capture_client_connection(const std::shared_ptr<ember_session_capture_state>& capture_state)
        : capture_state_(capture_state)
    {
    }

    void send(std::wstring&& data, bool /*skip_log*/) override
    {
        std::lock_guard<std::mutex> lock(capture_state_->mutex);
        capture_state_->output += data;
    }

    void disconnect() override { capture_state_->client->disconnect(); }

    std::wstring address() const override { return capture_state_->client->address(); }

    void add_lifecycle_bound_object(const std::wstring& key, const std::shared_ptr<void>& lifecycle_bound) override
    {
        capture_state_->client->add_lifecycle_bound_object(key, lifecycle_bound);
    }

    std::shared_ptr<void> remove_lifecycle_bound_object(const std::wstring& key) override
    {
        return capture_state_->client->remove_lifecycle_bound_object(key);
    }

  private:
    std::shared_ptr<ember_session_capture_state> capture_state_;
};

} // namespace

ember_session::ember_session(const IO::client_connection<char>::ptr& client)
    : capture_state_(std::make_shared<ember_session_capture_state>(client))
    , client_(client)
    , amcp_client_(spl::make_shared<amcp_capture_client_connection>(capture_state_))
{
}

const IO::client_connection<char>::ptr& ember_session::client() const { return client_; }

IO::ClientInfoPtr ember_session::amcp_client() const { return amcp_client_; }

std::wstring ember_session::take_amcp_output() const
{
    std::lock_guard<std::mutex> lock(capture_state_->mutex);
    auto                        output = std::move(capture_state_->output);
    capture_state_->output.clear();
    return output;
}

void ember_session::subscribe_stream(int stream_identifier) const
{
    std::lock_guard<std::mutex> lock(capture_state_->mutex);
    capture_state_->stream_subscriptions.insert(stream_identifier);
}

void ember_session::unsubscribe_stream(int stream_identifier) const
{
    std::lock_guard<std::mutex> lock(capture_state_->mutex);
    capture_state_->stream_subscriptions.erase(stream_identifier);
}

void ember_session::clear_stream_subscriptions() const
{
    std::lock_guard<std::mutex> lock(capture_state_->mutex);
    capture_state_->stream_subscriptions.clear();
}

std::vector<int> ember_session::stream_subscriptions() const
{
    std::lock_guard<std::mutex> lock(capture_state_->mutex);
    return std::vector<int>(capture_state_->stream_subscriptions.begin(), capture_state_->stream_subscriptions.end());
}

}}} // namespace caspar::protocol::ember
