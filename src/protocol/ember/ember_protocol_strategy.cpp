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

#include "ember_protocol_strategy.h"

#include "ember_provider.h"
#include "ember_session.h"

#include <ember/Ember.hpp>
#include <s101/CommandType.hpp>
#include <s101/Dtd.hpp>
#include <s101/MessageType.hpp>
#include <s101/PackageFlag.hpp>
#include <s101/StreamDecoder.hpp>
#include <s101/StreamEncoder.hpp>

namespace caspar { namespace protocol { namespace ember {
namespace {

std::string make_keepalive_response_packet()
{
    libs101::StreamEncoder<unsigned char> encoder;
    encoder.encode(0x00);
    encoder.encode(libs101::MessageType::EmBER);
    encoder.encode(libs101::CommandType::KeepAliveResponse);
    encoder.encode(0x01);
    encoder.finish();

    return std::string(encoder.begin(), encoder.end());
}

class ember_client_strategy final : public IO::protocol_strategy<char>
{
    using decoder_t = libs101::StreamDecoder<unsigned char>;

  public:
    ember_client_strategy(std::wstring                         name,
                          std::shared_ptr<ember_provider>      provider,
                          const IO::client_connection<char>::ptr& client)
        : name_(std::move(name))
        , provider_(std::move(provider))
        , session_(std::make_shared<ember_session>(client))
        , dom_reader_(libember::glow::GlowNodeFactory::getFactory())
    {
        provider_->send_provider_state(session_->client());
    }

    void parse(const std::string& data) override { decoder_.read(data.begin(), data.end(), dispatch, this); }

  private:
    static void dispatch(decoder_t::const_iterator first, decoder_t::const_iterator last, ember_client_strategy* self)
    {
        self->handle_frame(first, last);
    }

    void handle_frame(decoder_t::const_iterator first, decoder_t::const_iterator last)
    {
        if (first == last)
            return;

        ++first; // Slot
        if (first == last)
            return;

        const auto message = libs101::MessageType(*first++);
        if (message != libs101::MessageType::EmBER)
            return;

        if (first == last)
            return;

        const auto command = libs101::CommandType(*first++);
        if (command == libs101::CommandType::KeepAliveRequest) {
            session_->client()->send(make_keepalive_response_packet(), true);
            return;
        }

        if (command != libs101::CommandType::EmBER)
            return;

        if (first == last)
            return;
        ++first; // Version

        if (first == last)
            return;
        const auto flags = libs101::PackageFlag(*first++);

        if (first == last)
            return;
        const auto dtd = libs101::Dtd(*first++);
        if (dtd != libs101::Dtd::Glow) {
            CASPAR_LOG(debug) << L"[ember] Ignoring non-Glow Ember packet on " << name_;
            if ((flags.value() & libs101::PackageFlag::LastPackage) != 0)
                dom_reader_.reset();
            return;
        }

        if (first == last)
            return;
        auto app_bytes = *first++;
        while (app_bytes-- > 0 && first != last)
            ++first;

        if ((flags.value() & libs101::PackageFlag::FirstPackage) != 0)
            dom_reader_.reset();

        try {
            dom_reader_.read(first, last);
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            dom_reader_.reset();
            return;
        }

        if ((flags.value() & libs101::PackageFlag::LastPackage) == 0)
            return;

        if (dom_reader_.isRootReady()) {
            std::unique_ptr<libember::dom::Node> root(dom_reader_.detachRoot());
            provider_->handle_request(root.get(), session_);
        } else {
            CASPAR_LOG(debug) << L"[ember] Dropping incomplete Glow request on " << name_;
        }

        dom_reader_.reset();
    }

    const std::wstring                    name_;
    const std::shared_ptr<ember_provider> provider_;
    const std::shared_ptr<ember_session>  session_;
    decoder_t                             decoder_;
    libember::dom::AsyncDomReader         dom_reader_;
};

class ember_client_strategy_factory final : public IO::protocol_strategy_factory<char>
{
  public:
    ember_client_strategy_factory(std::wstring                    name,
                                  const std::shared_ptr<ember_provider>& provider)
        : name_(std::move(name))
        , provider_(provider)
    {
    }

    IO::protocol_strategy<char>::ptr create(const IO::client_connection<char>::ptr& client_connection) override
    {
        return spl::make_shared<ember_client_strategy>(name_, provider_, client_connection);
    }

  private:
    std::wstring                    name_;
    std::shared_ptr<ember_provider> provider_;
};

} // namespace

IO::protocol_strategy_factory<char>::ptr create_ember_plus_strategy_factory(const std::wstring&                   name,
                                                                            const std::shared_ptr<ember_provider>& provider)
{
    return spl::make_shared<ember_client_strategy_factory>(name, provider);
}

}}} // namespace caspar::protocol::ember
