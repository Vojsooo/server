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

#include "ember_command_bridge.h"

#include "../util/tokenize.h"

#include <common/except.h>
#include <common/utf.h>

#include <ember/Ember.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

namespace caspar { namespace protocol { namespace ember {
namespace {

std::wstring quote_amcp_token(const std::wstring& value)
{
    if (value.find_first_of(L" \\\"") == std::wstring::npos)
        return value;

    std::wstring result = L"\"";
    for (auto ch : value) {
        if (ch == L'\\' || ch == L'"')
            result += L'\\';
        result += ch;
    }
    result += L"\"";
    return result;
}

std::wstring transition_token(int transition)
{
    switch (transition) {
        case 1:
            return L"CUT";
        case 2:
            return L"MIX";
        case 3:
            return L"PUSH";
        case 4:
            return L"SLIDE";
        case 5:
            return L"WIPE";
        case 6:
            return L"FADECUT";
        case 7:
            return L"CUTFADE";
        case 8:
            return L"VFADE";
        default:
            return L"";
    }
}

std::wstring direction_token(int direction)
{
    switch (direction) {
        case 1:
            return L"FROMLEFT";
        case 2:
            return L"FROMRIGHT";
        case 3:
            return L"FROMTOP";
        case 4:
            return L"FROMBOTTOM";
        default:
            return L"";
    }
}

std::wstring compose_play_command(const ember_command_bridge::play_request& request)
{
    std::wstring command = L"PLAY " + std::to_wstring(request.channel_index);

    if (request.layer > 0)
        command += L"-" + std::to_wstring(request.layer);

    if (!request.file_name.empty()) {
        command += L" ";
        command += quote_amcp_token(request.file_name);

        if (request.loop)
            command += L" LOOP";

        if (request.seek > 0)
            command += L" SEEK " + std::to_wstring(request.seek);

        if (request.length > 0)
            command += L" LENGTH " + std::to_wstring(request.length);

        if (!request.filter.empty())
            command += L" FILTER " + quote_amcp_token(request.filter);

        if (request.clear_on_404)
            command += L" CLEAR_ON_404";

        const auto transition = transition_token(request.transition);
        if (!transition.empty() && request.transition_duration > 0) {
            command += L" " + transition + L" " + std::to_wstring(request.transition_duration);

            if (!request.tween.empty())
                command += L" " + request.tween;

            const auto direction = direction_token(request.direction);
            if (!direction.empty())
                command += L" " + direction;
        }
    }

    return command;
}

std::wstring join_amcp_result(const std::wstring& direct_result, const std::wstring& captured_output)
{
    if (direct_result.empty())
        return captured_output;

    if (captured_output.empty())
        return direct_result;

    return direct_result + captured_output;
}

} // namespace

ember_command_bridge::ember_command_bridge(const std::shared_ptr<amcp::amcp_command_repository>& repo)
    : repo_(repo)
{
    queues_.push_back(std::make_unique<caspar::executor>(L"Ember+ AMCP General Queue"));

    for (auto& channel : *repo_->channels()) {
        queues_.push_back(std::make_unique<caspar::executor>(
            L"Ember+ AMCP Channel " + std::to_wstring(channel.raw_channel->index()) + L" Queue"));
    }
}

std::vector<int> ember_command_bridge::amcp_execute_path() { return {5, 1}; }

void ember_command_bridge::describe_amcp_execute_function(libember::glow::GlowFunctionBase* function)
{
    function->setIdentifier("amcp_execute");
    function->setDescription("Execute a raw AMCP command line and return the AMCP reply string.");

    auto* arguments = function->arguments();
    arguments->clear();
    arguments->insert(arguments->end(),
                      new libember::glow::GlowTupleItemDescription(libember::glow::ParameterType::String, "command"));

    auto* result = function->result();
    result->clear();
    result->insert(result->end(),
                   new libember::glow::GlowTupleItemDescription(libember::glow::ParameterType::String, "reply"));
}

bool ember_command_bridge::supports_function(const std::vector<int>& path) const { return path == amcp_execute_path(); }

ember_command_bridge::invocation_result
ember_command_bridge::execute_native_command_line(const std::wstring&            command_line,
                                                 const std::shared_ptr<ember_session>& session)
{
    return execute_command_line(command_line, session);
}

ember_command_bridge::invocation_result
ember_command_bridge::execute_play(const play_request&                  request,
                                   const std::shared_ptr<ember_session>& session)
{
    return execute_command_line(compose_play_command(request), session);
}

ember_command_bridge::invocation_result
ember_command_bridge::invoke(const std::vector<int>&               path,
                             const libember::glow::GlowInvocation& invocation,
                             const std::shared_ptr<ember_session>& session)
{
    if (supports_function(path))
        return invoke_amcp_execute(invocation, session);

    return {false, L"Unsupported Ember+ function path."};
}

ember_command_bridge::invocation_result
ember_command_bridge::invoke_amcp_execute(const libember::glow::GlowInvocation& invocation,
                                          const std::shared_ptr<ember_session>& session)
{
    std::vector<libember::glow::Value> arguments;
    invocation.typedArguments(std::back_inserter(arguments));

    if (arguments.empty())
        return {false, L"Missing AMCP command argument."};

    std::wstring command_line;
    for (const auto& argument : arguments) {
        const auto value = u16(argument.toString());
        if (value.empty())
            continue;

        if (!command_line.empty())
            command_line += L" ";

        command_line += value;
    }

    if (command_line.empty())
        return {false, L"Empty AMCP command argument."};

    return execute_command_line(command_line, session);
}

ember_command_bridge::invocation_result
ember_command_bridge::execute_command_line(const std::wstring&                  command_line,
                                           const std::shared_ptr<ember_session>& session)
{
    auto normalized = boost::trim_copy(command_line);
    if (normalized.empty())
        return {false, L"Empty AMCP command argument."};

    CASPAR_LOG(info) << L"[ember] Executing from " << session->client()->address() << L": " << normalized << L"\r\n";

    std::list<std::wstring> tokens;
    IO::tokenize(normalized, tokens);

    if (!tokens.empty() && !tokens.front().empty() && tokens.front().front() == L'/')
        tokens.pop_front();

    std::wstring request_id;
    if (!tokens.empty() && boost::iequals(tokens.front(), L"REQ")) {
        tokens.pop_front();
        if (!tokens.empty()) {
            request_id = tokens.front();
            tokens.pop_front();
        }
    }

    if (tokens.empty())
        return {false, L"400 ERROR\r\n"};

    auto amcp_client = session->amcp_client();
    auto command     = repo_->parse_command(amcp_client, tokens, request_id);

    if (!command)
        return {false, L"400 ERROR\r\n"};

    if (!repo_->check_channel_lock(amcp_client, command->channel_index()))
        return {false, L"503 " + command->name() + L" FAILED\r\n"};

    auto& queue = *queues_.at(queue_index_for(command));
    if (queue.size() > 128)
        return {false, L"504 QUEUE OVERFLOW\r\n"};

    return queue.invoke([=]() { return run_command(command, session); });
}

ember_command_bridge::invocation_result
ember_command_bridge::run_command(const std::shared_ptr<amcp::AMCPCommand>& command,
                                  const std::shared_ptr<ember_session>&      session) const
{
    session->take_amcp_output();

    try {
        auto result = command->Execute(repo_->channels()).get();
        return {true, join_amcp_result(result, session->take_amcp_output())};
    } catch (file_not_found&) {
        return {false, L"404 " + command->name() + L" FAILED\r\n"};
    } catch (expected_user_error&) {
        return {false, L"403 " + command->name() + L" FAILED\r\n"};
    } catch (user_error&) {
        return {false, L"403 " + command->name() + L" FAILED\r\n"};
    } catch (std::out_of_range&) {
        return {false, L"402 " + command->name() + L" FAILED\r\n"};
    } catch (boost::bad_lexical_cast&) {
        return {false, L"403 " + command->name() + L" FAILED\r\n"};
    } catch (...) {
        CASPAR_LOG_CURRENT_EXCEPTION();
        return {false, L"501 " + command->name() + L" FAILED\r\n"};
    }
}

int ember_command_bridge::queue_index_for(const std::shared_ptr<amcp::AMCPCommand>& command) const
{
    const auto channel_index = command->channel_index();
    return channel_index >= 0 ? channel_index + 1 : 0;
}

}}} // namespace caspar::protocol::ember
