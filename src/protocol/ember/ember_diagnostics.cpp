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

#include "ember_diagnostics.h"

#include <common/diagnostics/graph.h>
#include <common/memory.h>
#include <common/utf.h>

#include <core/diagnostics/call_context.h>

#include <boost/algorithm/string.hpp>

#include <atomic>
#include <mutex>
#include <unordered_map>
#include <utility>

namespace caspar { namespace protocol { namespace ember {
namespace {

struct graph_snapshot
{
    core::diagnostics::call_context       context;
    std::wstring                          text;
    std::map<std::string, double>         values;
    std::map<std::string, std::uint64_t> counters;
};

std::string sanitize_identifier(const std::string& value)
{
    std::string identifier;
    bool        capitalize = true;

    for (auto ch : value) {
        const auto uch = static_cast<unsigned char>(ch);
        if (!std::isalnum(uch)) {
            capitalize = true;
            continue;
        }

        identifier += capitalize ? static_cast<char>(std::toupper(uch)) : ch;
        capitalize = false;
    }

    if (identifier.empty())
        identifier = "Graph";

    if (std::isdigit(static_cast<unsigned char>(identifier.front())))
        identifier.insert(identifier.begin(), 'G');

    return identifier;
}

std::string source_identifier_from_text(const std::wstring& text)
{
    auto source = u8(text);
    const auto bracket = source.find('[');
    if (bracket != std::string::npos)
        source = source.substr(0, bracket);

    boost::trim(source);
    return sanitize_identifier(source);
}

class diagnostics_registry final
{
  public:
    std::uint64_t register_graph(core::diagnostics::call_context context)
    {
        const auto id = next_id_.fetch_add(1);
        std::lock_guard<std::mutex> lock(mutex_);
        graphs_.emplace(id, graph_snapshot{context, L"", {}, {}});
        return id;
    }

    void unregister_graph(std::uint64_t id)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        graphs_.erase(id);
    }

    void set_text(std::uint64_t id, const std::wstring& text)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = graphs_.find(id);
        if (it != graphs_.end())
            it->second.text = text;
    }

    void set_value(std::uint64_t id, const std::string& name, double value)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = graphs_.find(id);
        if (it != graphs_.end())
            it->second.values[name] = value;
    }

    void increment_counter(std::uint64_t id, const std::string& name)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = graphs_.find(id);
        if (it != graphs_.end())
            it->second.counters[name] += 1;
    }

    diagnostics_snapshot_map_t snapshot() const
    {
        diagnostics_snapshot_map_t result;

        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& graph_entry : graphs_) {
            const auto& graph = graph_entry.second;
            if (graph.context.video_channel < 1)
                continue;

            auto source_identifier = source_identifier_from_text(graph.text);
            auto source_description = source_identifier;

            auto& channel_entry = result[graph.context.video_channel];
            auto& source_map = graph.context.layer >= 0 ? channel_entry.layer_sources[graph.context.layer]
                                                        : channel_entry.channel_sources;
            auto& aggregated = source_map[source_identifier];

            aggregated.identifier  = source_identifier;
            aggregated.description = source_description;

            for (const auto& value_entry : graph.values)
                aggregated.values[value_entry.first] = value_entry.second;

            for (const auto& counter_entry : graph.counters)
                aggregated.counters[counter_entry.first] += counter_entry.second;
        }

        return result;
    }

  private:
    mutable std::mutex                                mutex_;
    std::unordered_map<std::uint64_t, graph_snapshot> graphs_;
    std::atomic<std::uint64_t>                        next_id_{1};
};

diagnostics_registry& registry_instance()
{
    static diagnostics_registry instance;
    return instance;
}

class ember_graph_sink final : public diagnostics::spi::graph_sink
{
  public:
    ember_graph_sink()
        : id_(registry_instance().register_graph(core::diagnostics::call_context::for_thread()))
    {
    }

    ~ember_graph_sink() override { registry_instance().unregister_graph(id_); }

    void activate() override {}

    void set_text(const std::wstring& value) override { registry_instance().set_text(id_, value); }

    void set_value(const std::string& name, double value) override { registry_instance().set_value(id_, name, value); }

    void set_color(const std::string& /*name*/, int /*color*/) override {}

    void set_tag(diagnostics::tag_severity /*severity*/, const std::string& name) override
    {
        registry_instance().increment_counter(id_, name);
    }

    void auto_reset() override {}

  private:
    std::uint64_t id_;
};

std::once_flag registration_once;

} // namespace

void register_diagnostics_sink()
{
    std::call_once(registration_once, [] {
        diagnostics::spi::register_sink_factory([] { return spl::make_shared<ember_graph_sink>(); });
    });
}

diagnostics_snapshot_map_t diagnostics_snapshot() { return registry_instance().snapshot(); }

}}} // namespace caspar::protocol::ember
