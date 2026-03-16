#pragma once

#include <cstdint>
#include <map>
#include <string>

namespace caspar { namespace protocol { namespace ember {

struct diagnostic_source_snapshot
{
    std::string                      identifier;
    std::string                      description;
    std::map<std::string, double>    values;
    std::map<std::string, std::uint64_t> counters;
};

struct channel_diagnostics_snapshot
{
    std::map<std::string, diagnostic_source_snapshot>                    channel_sources;
    std::map<int, std::map<std::string, diagnostic_source_snapshot>>     layer_sources;
};

using diagnostics_snapshot_map_t = std::map<int, channel_diagnostics_snapshot>;

void                       register_diagnostics_sink();
diagnostics_snapshot_map_t diagnostics_snapshot();

}}} // namespace caspar::protocol::ember
