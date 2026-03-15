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

#include "ember_provider.h"

#include <common/env.h>
#include <common/filesystem.h>
#include <common/tweener.h>
#include <common/utf.h>

#include <core/frame/frame_transform.h>
#include <core/mixer/image/blend_modes.h>

#include <ember/Ember.hpp>
#include <s101/CommandType.hpp>
#include <s101/Dtd.hpp>
#include <s101/MessageType.hpp>
#include <s101/PackageFlag.hpp>
#include <s101/StreamEncoder.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <chrono>
#include <cctype>
#include <cmath>
#include <fstream>
#include <limits>
#include <sstream>
#include <set>

namespace caspar { namespace protocol { namespace ember {
namespace {

using access_t = libember::glow::Access;
using parameter_type_t = libember::glow::ParameterType;

constexpr int channels_root_number      = 100;
constexpr int runtime_root_number       = 2;
constexpr int runtime_channel_count_param_number = 1;
constexpr int runtime_state_update_interval_param_number = 2;
constexpr int play_node_number          = 10;
constexpr int loadbg_node_number        = 11;
constexpr int pause_node_number         = 12;
constexpr int resume_node_number        = 13;
constexpr int stop_node_number          = 14;
constexpr int clear_node_number         = 15;
constexpr int refresh_node_number       = 16;
constexpr int load_node_number          = 17;
constexpr int call_node_number          = 18;
constexpr int callbg_node_number        = 19;
constexpr int layers_root_node_number   = 40;
constexpr int layer_state_node_number   = 1;
constexpr int layer_mixer_node_number   = 2;

constexpr int clip_layer_param_number       = 1;
constexpr int clip_loop_param_number        = 2;
constexpr int clip_name_param_number        = 3;
constexpr int clip_seek_param_number        = 4;
constexpr int clip_length_param_number      = 5;
constexpr int clip_filter_param_number      = 6;
constexpr int clip_clear404_param_number    = 7;
constexpr int clip_transition_param_number  = 8;
constexpr int clip_duration_param_number    = 9;
constexpr int clip_tween_param_number       = 10;
constexpr int clip_direction_param_number   = 11;
constexpr int clip_execute_param_number     = 12;
constexpr int clip_last_reply_param_number  = 13;
constexpr int clip_last_success_param_number = 14;
constexpr int clip_auto_param_number        = 15;
constexpr int clip_sting_node_number        = 20;

constexpr int sting_enabled_param_number             = 1;
constexpr int sting_mask_param_number                = 2;
constexpr int sting_trigger_param_number             = 3;
constexpr int sting_overlay_param_number             = 4;
constexpr int sting_audio_fade_start_param_number    = 5;
constexpr int sting_audio_fade_duration_param_number = 6;

constexpr int load_layer_param_number        = 1;
constexpr int load_name_param_number         = 2;
constexpr int load_loop_param_number         = 3;
constexpr int load_seek_param_number         = 4;
constexpr int load_length_param_number       = 5;
constexpr int load_filter_param_number       = 6;
constexpr int load_clear404_param_number     = 7;
constexpr int load_execute_param_number      = 8;
constexpr int load_last_reply_param_number   = 9;
constexpr int load_last_success_param_number = 10;

constexpr int action_layer_param_number      = 1;
constexpr int action_execute_param_number    = 2;
constexpr int action_last_reply_param_number = 3;
constexpr int action_last_success_param_number = 4;

constexpr int call_layer_param_number        = 1;
constexpr int call_arguments_param_number    = 2;
constexpr int call_execute_param_number      = 3;
constexpr int call_last_reply_param_number   = 4;
constexpr int call_last_success_param_number = 5;

constexpr int refresh_execute_param_number   = 1;
constexpr int refresh_count_param_number     = 2;
constexpr int refresh_last_reply_param_number = 3;
constexpr int refresh_last_success_param_number = 4;

constexpr int layer_state_foreground_param_number = 1;
constexpr int layer_state_background_param_number = 2;
constexpr int layer_state_paused_param_number     = 3;
constexpr int layer_state_frames_left_param_number = 4;

constexpr int layer_mixer_keyer_param_number      = 1;
constexpr int layer_mixer_invert_param_number     = 2;
constexpr int layer_mixer_opacity_param_number    = 3;
constexpr int layer_mixer_brightness_param_number = 4;
constexpr int layer_mixer_saturation_param_number = 5;
constexpr int layer_mixer_contrast_param_number   = 6;
constexpr int layer_mixer_rotation_param_number   = 7;
constexpr int layer_mixer_volume_param_number     = 8;
constexpr int layer_mixer_blend_param_number      = 9;
constexpr int layer_mixer_fill_x_param_number     = 10;
constexpr int layer_mixer_fill_y_param_number     = 11;
constexpr int layer_mixer_fill_w_param_number     = 12;
constexpr int layer_mixer_fill_h_param_number     = 13;
constexpr int layer_mixer_clip_x_param_number     = 14;
constexpr int layer_mixer_clip_y_param_number     = 15;
constexpr int layer_mixer_clip_w_param_number     = 16;
constexpr int layer_mixer_clip_h_param_number     = 17;
constexpr int layer_mixer_anchor_x_param_number   = 18;
constexpr int layer_mixer_anchor_y_param_number   = 19;
constexpr int layer_mixer_crop_l_param_number     = 20;
constexpr int layer_mixer_crop_t_param_number     = 21;
constexpr int layer_mixer_crop_r_param_number     = 22;
constexpr int layer_mixer_crop_b_param_number     = 23;
constexpr int layer_mixer_opacity_pct_param_number    = 24;
constexpr int layer_mixer_brightness_pct_param_number = 25;
constexpr int layer_mixer_saturation_pct_param_number = 26;
constexpr int layer_mixer_contrast_pct_param_number   = 27;
constexpr int layer_mixer_volume_pct_param_number     = 28;
constexpr int layer_mixer_fill_x_pct_param_number     = 29;
constexpr int layer_mixer_fill_y_pct_param_number     = 30;
constexpr int layer_mixer_fill_w_pct_param_number     = 31;
constexpr int layer_mixer_fill_h_pct_param_number     = 32;
constexpr int layer_mixer_clip_x_pct_param_number     = 33;
constexpr int layer_mixer_clip_y_pct_param_number     = 34;
constexpr int layer_mixer_clip_w_pct_param_number     = 35;
constexpr int layer_mixer_clip_h_pct_param_number     = 36;
constexpr int layer_mixer_anchor_x_pct_param_number   = 37;
constexpr int layer_mixer_anchor_y_pct_param_number   = 38;
constexpr int layer_mixer_crop_l_pct_param_number     = 39;
constexpr int layer_mixer_crop_t_pct_param_number     = 40;
constexpr int layer_mixer_crop_r_pct_param_number     = 41;
constexpr int layer_mixer_crop_b_pct_param_number     = 42;
constexpr int layer_mixer_last_param_number           = layer_mixer_crop_b_pct_param_number;
constexpr int scaled_mixer_factor                 = 1000;
constexpr long minimum_state_update_interval_ms   = 1;
constexpr long maximum_state_update_interval_ms   = 60000;

struct live_layer_state
{
    int          layer_index = 0;
    std::wstring foreground;
    std::wstring background;
    bool         paused      = false;
    std::int64_t frames_left = -1;
    bool         keyer       = false;
    bool         invert      = false;
    double       opacity     = 1.0;
    double       brightness  = 1.0;
    double       saturation  = 1.0;
    double       contrast    = 1.0;
    double       rotation    = 0.0;
    double       volume      = 1.0;
    int          blend       = 0;
    double       fill_x      = 0.0;
    double       fill_y      = 0.0;
    double       fill_w      = 1.0;
    double       fill_h      = 1.0;
    double       clip_x      = 0.0;
    double       clip_y      = 0.0;
    double       clip_w      = 1.0;
    double       clip_h      = 1.0;
    double       anchor_x    = 0.0;
    double       anchor_y    = 0.0;
    double       crop_l      = 0.0;
    double       crop_t      = 0.0;
    double       crop_r      = 1.0;
    double       crop_b      = 1.0;
    std::map<std::string, core::monitor::vector_t> monitor_entries;
};

using live_layer_map_t      = std::map<int, live_layer_state>;
using live_snapshot_map_t   = std::map<int, live_layer_map_t>;

struct function_invocation_request
{
    std::vector<int>                      path;
    const libember::glow::GlowInvocation* invocation = nullptr;
};

struct parameter_write_request
{
    std::vector<int>        path;
    libember::glow::Value value;
};

void send_qualified_parameter_update(const IO::client_connection<char>::ptr& client,
                                     const std::vector<int>&                 path,
                                     const std::function<void(libember::glow::GlowQualifiedParameter*)>& setter);

std::string to_packet_string(const libs101::StreamEncoder<unsigned char>& encoder)
{
    return std::string(encoder.begin(), encoder.end());
}

class ember_message_builder final
{
  public:
    static std::string encode(const libember::dom::Node& node)
    {
        ember_message_builder builder;
        stream                output(&builder);
        node.encode(output);
        output.finish();
        return std::move(builder.payload_);
    }

  private:
    class stream final : public libember::util::OctetStream
    {
      public:
        explicit stream(ember_message_builder* owner)
            : libember::util::OctetStream(1024)
            , owner_(owner)
        {
        }

        void finish() { owner_->finish_packet(begin(), end(), true); }

      private:
        void flush(iterator first, iterator last) override { owner_->finish_packet(first, last, false); }

        ember_message_builder* owner_;
    };

    template <typename InputIterator>
    void finish_packet(InputIterator first, InputIterator last, bool is_last_packet)
    {
        libs101::StreamEncoder<unsigned char> encoder;
        const auto                            version  = libember::glow::GlowDtd::version();
        const auto                            is_empty = first == last;
        const auto flags = static_cast<unsigned char>(
            (is_first_packet_ ? libs101::PackageFlag::FirstPackage : 0) |
            (is_last_packet ? libs101::PackageFlag::LastPackage : 0) | (is_empty ? libs101::PackageFlag::EmptyPackage : 0));

        encoder.encode(0x00);
        encoder.encode(libs101::MessageType::EmBER);
        encoder.encode(libs101::CommandType::EmBER);
        encoder.encode(0x01);
        encoder.encode(flags);
        encoder.encode(libs101::Dtd::Glow);
        encoder.encode(0x02);
        encoder.encode((version >> 0) & 0xFF);
        encoder.encode((version >> 8) & 0xFF);
        encoder.encode(first, last);
        encoder.finish();

        payload_.append(encoder.begin(), encoder.end());
        is_first_packet_ = false;
    }

    bool        is_first_packet_ = true;
    std::string payload_;
};

std::string make_provider_state_packet(bool online)
{
    libs101::StreamEncoder<unsigned char> encoder;
    encoder.encode(0x00);
    encoder.encode(libs101::MessageType::EmBER);
    encoder.encode(libs101::CommandType::ProviderState);
    encoder.encode(0x01);
    encoder.encode(online ? 0x01 : 0x00);
    encoder.finish();
    return to_packet_string(encoder);
}

void add_string_parameter(libember::glow::GlowNodeBase* parent,
                          int                           number,
                          const std::string&           identifier,
                          const std::string&           value,
                          const std::string&           description,
                          access_t                      access = access_t::ReadOnly)
{
    auto* parameter = new libember::glow::GlowParameter(parent, number);
    parameter->setIdentifier(identifier);
    if (!description.empty())
        parameter->setDescription(description);
    parameter->setAccess(access);
    parameter->setType(parameter_type_t::String);
    parameter->setValue(value);
}

void add_integer_parameter(libember::glow::GlowNodeBase* parent,
                           int                           number,
                           const std::string&           identifier,
                           long                          value,
                           const std::string&           description,
                           access_t                      access = access_t::ReadOnly)
{
    auto* parameter = new libember::glow::GlowParameter(parent, number);
    parameter->setIdentifier(identifier);
    if (!description.empty())
        parameter->setDescription(description);
    parameter->setAccess(access);
    parameter->setType(parameter_type_t::Integer);
    parameter->setValue(value);
}

void add_ranged_integer_parameter(libember::glow::GlowNodeBase* parent,
                                  int                           number,
                                  const std::string&           identifier,
                                  long                          value,
                                  long                          minimum,
                                  long                          maximum,
                                  const std::string&           description,
                                  access_t                      access = access_t::ReadOnly)
{
    auto* parameter = new libember::glow::GlowParameter(parent, number);
    parameter->setIdentifier(identifier);
    if (!description.empty())
        parameter->setDescription(description);
    parameter->setAccess(access);
    parameter->setType(parameter_type_t::Integer);
    parameter->setMinimum(minimum);
    parameter->setMaximum(maximum);
    parameter->setValue(std::max(minimum, std::min(value, maximum)));
}

void add_boolean_parameter(libember::glow::GlowNodeBase* parent,
                           int                           number,
                           const std::string&           identifier,
                           bool                          value,
                           const std::string&           description,
                           access_t                      access = access_t::ReadOnly)
{
    auto* parameter = new libember::glow::GlowParameter(parent, number);
    parameter->setIdentifier(identifier);
    if (!description.empty())
        parameter->setDescription(description);
    parameter->setAccess(access);
    parameter->setType(parameter_type_t::Boolean);
    parameter->setValue(value);
}

void add_real_parameter(libember::glow::GlowNodeBase* parent,
                        int                           number,
                        const std::string&           identifier,
                        double                        value,
                        const std::string&           description,
                        access_t                      access = access_t::ReadOnly)
{
    auto* parameter = new libember::glow::GlowParameter(parent, number);
    parameter->setIdentifier(identifier);
    if (!description.empty())
        parameter->setDescription(description);
    parameter->setAccess(access);
    parameter->setType(parameter_type_t::Real);
    parameter->setValue(value);
}

long scaled_mixer_value(double value, int factor = scaled_mixer_factor)
{
    return static_cast<long>(std::llround(value * static_cast<double>(factor)));
}

double unscaled_mixer_value(const libember::glow::Value& value, double default_value, int factor = scaled_mixer_factor)
{
    switch (value.type().value()) {
        case libember::glow::ParameterType::Integer:
            return static_cast<double>(value.toInteger()) / static_cast<double>(factor);
        case libember::glow::ParameterType::Real:
            return value.toReal();
        case libember::glow::ParameterType::Boolean:
            return value.toBoolean() ? 1.0 : 0.0;
        case libember::glow::ParameterType::String:
            try {
                return std::stod(value.toString());
            } catch (...) {
                return default_value;
            }
        default:
            return default_value;
    }
}

double clamp_double(double value, double minimum, double maximum)
{
    return std::max(minimum, std::min(value, maximum));
}

bool mixer_parameter_range(int parameter_number, double& minimum, double& maximum)
{
    switch (parameter_number) {
        case layer_mixer_opacity_param_number:
        case layer_mixer_brightness_param_number:
        case layer_mixer_saturation_param_number:
        case layer_mixer_contrast_param_number:
        case layer_mixer_volume_param_number:
        case layer_mixer_fill_w_param_number:
        case layer_mixer_fill_h_param_number:
        case layer_mixer_clip_w_param_number:
        case layer_mixer_clip_h_param_number:
        case layer_mixer_crop_l_param_number:
        case layer_mixer_crop_t_param_number:
        case layer_mixer_crop_r_param_number:
        case layer_mixer_crop_b_param_number:
            minimum = 0.0;
            maximum = 1.0;
            return true;
        case layer_mixer_fill_x_param_number:
        case layer_mixer_fill_y_param_number:
        case layer_mixer_clip_x_param_number:
        case layer_mixer_clip_y_param_number:
        case layer_mixer_anchor_x_param_number:
        case layer_mixer_anchor_y_param_number:
            minimum = -1.0;
            maximum = 1.0;
            return true;
        case layer_mixer_rotation_param_number:
            minimum = -360.0;
            maximum = 360.0;
            return true;
        default:
            return false;
    }
}

bool mixer_percent_parameter_info(int parameter_number, int& raw_parameter_number, long& minimum, long& maximum)
{
    switch (parameter_number) {
        case layer_mixer_opacity_pct_param_number:
            raw_parameter_number = layer_mixer_opacity_param_number;
            minimum              = 0;
            maximum              = 100;
            return true;
        case layer_mixer_brightness_pct_param_number:
            raw_parameter_number = layer_mixer_brightness_param_number;
            minimum              = 0;
            maximum              = 100;
            return true;
        case layer_mixer_saturation_pct_param_number:
            raw_parameter_number = layer_mixer_saturation_param_number;
            minimum              = 0;
            maximum              = 100;
            return true;
        case layer_mixer_contrast_pct_param_number:
            raw_parameter_number = layer_mixer_contrast_param_number;
            minimum              = 0;
            maximum              = 100;
            return true;
        case layer_mixer_volume_pct_param_number:
            raw_parameter_number = layer_mixer_volume_param_number;
            minimum              = 0;
            maximum              = 100;
            return true;
        case layer_mixer_fill_x_pct_param_number:
            raw_parameter_number = layer_mixer_fill_x_param_number;
            minimum              = -100;
            maximum              = 100;
            return true;
        case layer_mixer_fill_y_pct_param_number:
            raw_parameter_number = layer_mixer_fill_y_param_number;
            minimum              = -100;
            maximum              = 100;
            return true;
        case layer_mixer_fill_w_pct_param_number:
            raw_parameter_number = layer_mixer_fill_w_param_number;
            minimum              = 0;
            maximum              = 100;
            return true;
        case layer_mixer_fill_h_pct_param_number:
            raw_parameter_number = layer_mixer_fill_h_param_number;
            minimum              = 0;
            maximum              = 100;
            return true;
        case layer_mixer_clip_x_pct_param_number:
            raw_parameter_number = layer_mixer_clip_x_param_number;
            minimum              = -100;
            maximum              = 100;
            return true;
        case layer_mixer_clip_y_pct_param_number:
            raw_parameter_number = layer_mixer_clip_y_param_number;
            minimum              = -100;
            maximum              = 100;
            return true;
        case layer_mixer_clip_w_pct_param_number:
            raw_parameter_number = layer_mixer_clip_w_param_number;
            minimum              = 0;
            maximum              = 100;
            return true;
        case layer_mixer_clip_h_pct_param_number:
            raw_parameter_number = layer_mixer_clip_h_param_number;
            minimum              = 0;
            maximum              = 100;
            return true;
        case layer_mixer_anchor_x_pct_param_number:
            raw_parameter_number = layer_mixer_anchor_x_param_number;
            minimum              = -100;
            maximum              = 100;
            return true;
        case layer_mixer_anchor_y_pct_param_number:
            raw_parameter_number = layer_mixer_anchor_y_param_number;
            minimum              = -100;
            maximum              = 100;
            return true;
        case layer_mixer_crop_l_pct_param_number:
            raw_parameter_number = layer_mixer_crop_l_param_number;
            minimum              = 0;
            maximum              = 100;
            return true;
        case layer_mixer_crop_t_pct_param_number:
            raw_parameter_number = layer_mixer_crop_t_param_number;
            minimum              = 0;
            maximum              = 100;
            return true;
        case layer_mixer_crop_r_pct_param_number:
            raw_parameter_number = layer_mixer_crop_r_param_number;
            minimum              = 0;
            maximum              = 100;
            return true;
        case layer_mixer_crop_b_pct_param_number:
            raw_parameter_number = layer_mixer_crop_b_param_number;
            minimum              = 0;
            maximum              = 100;
            return true;
        default:
            return false;
    }
}

double clamp_mixer_parameter_value(int parameter_number, double value)
{
    double minimum = 0.0;
    double maximum = 0.0;
    if (!mixer_parameter_range(parameter_number, minimum, maximum))
        return value;

    return clamp_double(value, minimum, maximum);
}

double raw_value_from_percent_parameter(int parameter_number, const libember::glow::Value& value)
{
    int  raw_parameter_number = 0;
    long minimum             = 0;
    long maximum             = 0;
    if (!mixer_percent_parameter_info(parameter_number, raw_parameter_number, minimum, maximum))
        return 0.0;

    double percent = 0.0;
    switch (value.type().value()) {
        case libember::glow::ParameterType::Integer:
            percent = static_cast<double>(value.toInteger());
            break;
        case libember::glow::ParameterType::Real:
            percent = value.toReal();
            break;
        case libember::glow::ParameterType::Boolean:
            percent = value.toBoolean() ? 100.0 : 0.0;
            break;
        case libember::glow::ParameterType::String:
            try {
                percent = std::stod(value.toString());
            } catch (...) {
                percent = 0.0;
            }
            break;
        default:
            break;
    }

    percent = clamp_double(percent, static_cast<double>(minimum), static_cast<double>(maximum));
    return clamp_mixer_parameter_value(raw_parameter_number, percent / 100.0);
}

void add_scaled_parameter(libember::glow::GlowNodeBase* parent,
                          int                           number,
                          const std::string&           identifier,
                          double                        value,
                          const std::string&           description,
                          access_t                      access = access_t::ReadOnly,
                          int                           factor = scaled_mixer_factor,
                          const std::string&           format = "%.3f")
{
    auto* parameter = new libember::glow::GlowParameter(parent, number);
    parameter->setIdentifier(identifier);
    if (!description.empty())
        parameter->setDescription(description);
    parameter->setAccess(access);
    parameter->setType(parameter_type_t::Integer);
    parameter->setFactor(factor);
    if (!format.empty())
        parameter->setFormat(format);
    double minimum = 0.0;
    double maximum = 0.0;
    if (mixer_parameter_range(number, minimum, maximum)) {
        parameter->setMinimum(scaled_mixer_value(minimum, factor));
        parameter->setMaximum(scaled_mixer_value(maximum, factor));
        value = clamp_double(value, minimum, maximum);
    }
    parameter->setValue(scaled_mixer_value(value, factor));
}

void add_percent_parameter(libember::glow::GlowNodeBase* parent,
                           int                           number,
                           const std::string&           identifier,
                           double                        value,
                           const std::string&           description,
                           access_t                      access = access_t::ReadOnly)
{
    auto* parameter = new libember::glow::GlowParameter(parent, number);
    parameter->setIdentifier(identifier);
    if (!description.empty())
        parameter->setDescription(description);
    parameter->setAccess(access);
    parameter->setType(parameter_type_t::Integer);
    parameter->setFormat("%d");

    int  raw_parameter_number = 0;
    long minimum             = 0;
    long maximum             = 0;
    if (mixer_percent_parameter_info(number, raw_parameter_number, minimum, maximum)) {
        parameter->setMinimum(minimum);
        parameter->setMaximum(maximum);
        value = clamp_mixer_parameter_value(raw_parameter_number, value);
        parameter->setValue(static_cast<long>(std::llround(value * 100.0)));
    } else {
        parameter->setValue(static_cast<long>(std::llround(value)));
    }
}

void add_enum_parameter(libember::glow::GlowNodeBase*     parent,
                        int                               number,
                        const std::string&               identifier,
                        long                              value,
                        const std::vector<std::string>&  enumeration,
                        const std::string&               description,
                        access_t                          access = access_t::ReadOnly)
{
    auto* parameter = new libember::glow::GlowParameter(parent, number);
    parameter->setIdentifier(identifier);
    if (!description.empty())
        parameter->setDescription(description);
    parameter->setAccess(access);
    parameter->setType(parameter_type_t::Integer);
    if (!enumeration.empty()) {
        parameter->setEnumeration(enumeration.begin(), enumeration.end());
        parameter->setMinimum(0L);
        parameter->setMaximum(static_cast<long>(enumeration.size() - 1));
    }
    parameter->setValue(value);
}

const std::vector<std::string>& play_transition_labels()
{
    static const std::vector<std::string> labels = {"None", "Cut", "Mix", "Push", "Slide", "Wipe", "FadeCut",
                                                    "CutFade", "VFade"};
    return labels;
}

const std::vector<std::string>& play_direction_labels()
{
    static const std::vector<std::string> labels = {"Default", "FromLeft", "FromRight", "FromTop", "FromBottom"};
    return labels;
}

const std::vector<std::string>& mixer_blend_labels()
{
    static const std::vector<std::string> labels = {"normal",
                                                    "lighten",
                                                    "darken",
                                                    "multiply",
                                                    "average",
                                                    "add",
                                                    "subtract",
                                                    "difference",
                                                    "negation",
                                                    "exclusion",
                                                    "screen",
                                                    "overlay",
                                                    "soft_light",
                                                    "hard_light",
                                                    "color_dodge",
                                                    "color_burn",
                                                    "linear_dodge",
                                                    "linear_burn",
                                                    "linear_light",
                                                    "vivid_light",
                                                    "pin_light",
                                                    "hard_mix",
                                                    "reflect",
                                                    "glow",
                                                    "phoenix",
                                                    "contrast",
                                                    "saturation",
                                                    "color",
                                                    "luminosity"};
    return labels;
}

template <typename T>
const T* monitor_value_as(const core::monitor::vector_t& values)
{
    if (values.empty())
        return nullptr;

    return boost::get<T>(&values.front());
}

std::wstring monitor_to_wstring(const core::monitor::vector_t& values)
{
    if (const auto* wvalue = monitor_value_as<std::wstring>(values))
        return *wvalue;
    if (const auto* value = monitor_value_as<std::string>(values))
        return u16(*value);
    return L"";
}

std::string monitor_value_to_string(const core::monitor::data_t& value)
{
    struct visitor : boost::static_visitor<std::string>
    {
        std::string operator()(bool value) const { return value ? "true" : "false"; }
        std::string operator()(std::int32_t value) const { return std::to_string(value); }
        std::string operator()(std::int64_t value) const { return std::to_string(value); }
        std::string operator()(std::uint32_t value) const { return std::to_string(value); }
        std::string operator()(std::uint64_t value) const { return std::to_string(value); }
        std::string operator()(float value) const
        {
            std::ostringstream stream;
            stream << value;
            return stream.str();
        }
        std::string operator()(double value) const
        {
            std::ostringstream stream;
            stream << value;
            return stream.str();
        }
        std::string operator()(const std::string& value) const { return value; }
        std::string operator()(const std::wstring& value) const { return u8(value); }
    };

    return boost::apply_visitor(visitor(), value);
}

std::string ember_label_from_key(const std::string& key)
{
    std::string label;
    bool        capitalize = true;

    for (auto ch : key) {
        if (ch == '_' || ch == '-' || ch == ' ') {
            capitalize = true;
            continue;
        }

        label += capitalize ? static_cast<char>(std::toupper(static_cast<unsigned char>(ch))) : ch;
        capitalize = false;
    }

    return label.empty() ? "Value" : label;
}

bool monitor_to_bool(const core::monitor::vector_t& values, bool default_value = false)
{
    if (const auto* value = monitor_value_as<bool>(values))
        return *value;
    if (const auto* value = monitor_value_as<std::int32_t>(values))
        return *value != 0;
    if (const auto* value = monitor_value_as<std::int64_t>(values))
        return *value != 0;
    return default_value;
}

std::int64_t monitor_to_int64(const core::monitor::vector_t& values, std::int64_t default_value = -1)
{
    if (const auto* value = monitor_value_as<std::int32_t>(values))
        return *value;
    if (const auto* value = monitor_value_as<std::int64_t>(values))
        return *value;
    if (const auto* value = monitor_value_as<std::uint32_t>(values))
        return *value;
    if (const auto* value = monitor_value_as<std::uint64_t>(values))
        return static_cast<std::int64_t>(*value);
    return default_value;
}

int blend_index_from_mode(core::blend_mode mode)
{
    const auto text = u8(core::get_blend_mode(mode));
    const auto& labels = mixer_blend_labels();

    for (std::size_t i = 0; i < labels.size(); ++i) {
        if (boost::iequals(labels[i], text))
            return static_cast<int>(i);
    }

    return 0;
}

std::wstring blend_token_from_index(long value)
{
    const auto& labels = mixer_blend_labels();
    const auto  index  = static_cast<std::size_t>(std::max(0L, std::min(value, static_cast<long>(labels.size() - 1))));
    return u16(labels.at(index));
}

bool nearly_equal(double left, double right, double epsilon = 0.0001)
{
    return std::fabs(left - right) <= epsilon;
}

bool operator==(const live_layer_state& left, const live_layer_state& right)
{
    return left.layer_index == right.layer_index && left.foreground == right.foreground &&
           left.background == right.background && left.paused == right.paused && left.frames_left == right.frames_left &&
           left.keyer == right.keyer && left.invert == right.invert && nearly_equal(left.opacity, right.opacity) &&
           nearly_equal(left.brightness, right.brightness) && nearly_equal(left.saturation, right.saturation) &&
           nearly_equal(left.contrast, right.contrast) && nearly_equal(left.rotation, right.rotation) &&
           nearly_equal(left.volume, right.volume) && left.blend == right.blend &&
           nearly_equal(left.fill_x, right.fill_x) && nearly_equal(left.fill_y, right.fill_y) &&
           nearly_equal(left.fill_w, right.fill_w) && nearly_equal(left.fill_h, right.fill_h) &&
           nearly_equal(left.clip_x, right.clip_x) && nearly_equal(left.clip_y, right.clip_y) &&
           nearly_equal(left.clip_w, right.clip_w) && nearly_equal(left.clip_h, right.clip_h) &&
           nearly_equal(left.anchor_x, right.anchor_x) && nearly_equal(left.anchor_y, right.anchor_y) &&
           nearly_equal(left.crop_l, right.crop_l) && nearly_equal(left.crop_t, right.crop_t) &&
           nearly_equal(left.crop_r, right.crop_r) && nearly_equal(left.crop_b, right.crop_b) &&
           left.monitor_entries == right.monitor_entries;
}

bool operator!=(const live_layer_state& left, const live_layer_state& right) { return !(left == right); }

bool value_to_bool(const libember::glow::Value& value, bool default_value = false)
{
    switch (value.type().value()) {
        case libember::glow::ParameterType::Boolean:
            return value.toBoolean();
        case libember::glow::ParameterType::Integer:
            return value.toInteger() != 0;
        case libember::glow::ParameterType::Real:
            return !nearly_equal(value.toReal(), 0.0);
        case libember::glow::ParameterType::String: {
            const auto text = boost::to_lower_copy(value.toString());
            if (text.empty())
                return default_value;
            return text != "0" && text != "false" && text != "off";
        }
        default:
            return default_value;
    }
}

double value_to_double(const libember::glow::Value& value, double default_value = 0.0)
{
    switch (value.type().value()) {
        case libember::glow::ParameterType::Real:
            return value.toReal();
        case libember::glow::ParameterType::Integer:
            return static_cast<double>(value.toInteger());
        case libember::glow::ParameterType::Boolean:
            return value.toBoolean() ? 1.0 : 0.0;
        case libember::glow::ParameterType::String:
            try {
                return std::stod(value.toString());
            } catch (...) {
                return default_value;
            }
        default:
            return default_value;
    }
}

live_layer_map_t collect_live_layers(const amcp::channel_context& channel)
{
    live_layer_map_t               layers;
    const auto                     stage_state = channel.raw_channel->stage()->state();
    static const std::string       layer_prefix = "layer/";

    for (const auto& entry : stage_state) {
        if (!boost::starts_with(entry.first, layer_prefix))
            continue;

        const auto remainder = entry.first.substr(layer_prefix.size());
        const auto slash     = remainder.find('/');
        if (slash == std::string::npos)
            continue;

        const auto layer_text = remainder.substr(0, slash);
        const auto tail       = remainder.substr(slash + 1);

        int layer_index = 0;
        try {
            layer_index = std::stoi(layer_text);
        } catch (...) {
            continue;
        }

        auto& layer = layers[layer_index];
        layer.layer_index = layer_index;
        layer.monitor_entries[tail] = entry.second;

        if (tail == "foreground/producer")
            layer.foreground = monitor_to_wstring(entry.second);
        else if (tail == "background/producer")
            layer.background = monitor_to_wstring(entry.second);
        else if (tail == "foreground/paused")
            layer.paused = monitor_to_bool(entry.second);
        else if (tail == "foreground/frames_left")
            layer.frames_left = monitor_to_int64(entry.second);
    }

    static const double pi = 3.14159265358979323846;
    for (auto& layer_entry : layers) {
        const auto transform = channel.raw_channel->stage()->get_current_transform(layer_entry.first).get();
        auto&      layer     = layer_entry.second;
        layer.keyer          = transform.image_transform.is_key;
        layer.invert         = transform.image_transform.invert;
        layer.opacity        = transform.image_transform.opacity;
        layer.brightness     = transform.image_transform.brightness;
        layer.saturation     = transform.image_transform.saturation;
        layer.contrast       = transform.image_transform.contrast;
        layer.rotation       = transform.image_transform.angle / pi * 180.0;
        layer.volume         = transform.audio_transform.volume;
        layer.blend          = blend_index_from_mode(transform.image_transform.blend_mode);
        layer.fill_x         = transform.image_transform.fill_translation[0];
        layer.fill_y         = transform.image_transform.fill_translation[1];
        layer.fill_w         = transform.image_transform.fill_scale[0];
        layer.fill_h         = transform.image_transform.fill_scale[1];
        layer.clip_x         = transform.image_transform.clip_translation[0];
        layer.clip_y         = transform.image_transform.clip_translation[1];
        layer.clip_w         = transform.image_transform.clip_scale[0];
        layer.clip_h         = transform.image_transform.clip_scale[1];
        layer.anchor_x       = transform.image_transform.anchor[0];
        layer.anchor_y       = transform.image_transform.anchor[1];
        layer.crop_l         = transform.image_transform.crop.ul[0];
        layer.crop_t         = transform.image_transform.crop.ul[1];
        layer.crop_r         = transform.image_transform.crop.lr[0];
        layer.crop_b         = transform.image_transform.crop.lr[1];
    }

    return layers;
}

live_snapshot_map_t collect_live_snapshot(const spl::shared_ptr<std::vector<amcp::channel_context>>& channels)
{
    live_snapshot_map_t snapshot;

    for (const auto& channel : *channels)
        snapshot.emplace(channel.raw_channel->index(), collect_live_layers(channel));

    return snapshot;
}

bool topology_changed(const live_snapshot_map_t& previous, const live_snapshot_map_t& current)
{
    if (previous.size() != current.size())
        return true;

    auto prev_it = previous.begin();
    auto curr_it = current.begin();
    while (prev_it != previous.end() && curr_it != current.end()) {
        if (prev_it->first != curr_it->first)
            return true;
        if (prev_it->second.size() != curr_it->second.size())
            return true;

        auto prev_layer_it = prev_it->second.begin();
        auto curr_layer_it = curr_it->second.begin();
        while (prev_layer_it != prev_it->second.end() && curr_layer_it != curr_it->second.end()) {
            if (prev_layer_it->first != curr_layer_it->first)
                return true;
            ++prev_layer_it;
            ++curr_layer_it;
        }

        ++prev_it;
        ++curr_it;
    }

    return false;
}

std::string ember_identifier_from_monitor_path(const std::string& path)
{
    std::string identifier;
    bool        capitalize = true;

    for (auto ch : path) {
        const auto uch = static_cast<unsigned char>(ch);
        if (ch == '/' || ch == '_' || ch == '-' || ch == ' ' || ch == '.') {
            capitalize = true;
            continue;
        }

        if (!std::isalnum(uch)) {
            capitalize = true;
            continue;
        }

        identifier += capitalize ? static_cast<char>(std::toupper(uch)) : ch;
        capitalize = false;
    }

    if (identifier.empty())
        identifier = "Value";

    if (std::isdigit(static_cast<unsigned char>(identifier.front())))
        identifier.insert(identifier.begin(), 'V');

    return identifier;
}

std::string ember_description_from_monitor_path(const std::string& path)
{
    std::stringstream stream(path);
    std::string       segment;
    std::string       description;

    while (std::getline(stream, segment, '/')) {
        if (segment.empty())
            continue;

        if (!description.empty())
            description += "/";

        description += ember_label_from_key(segment);
    }

    return description.empty() ? "Value" : description;
}

std::string monitor_component_label(const std::string& path, std::size_t index, std::size_t count)
{
    const auto slash      = path.find_last_of('/');
    const auto leaf       = boost::to_lower_copy(slash == std::string::npos ? path : path.substr(slash + 1));
    const auto value_name = "Value" + std::to_string(index + 1);

    if (count == 2) {
        if (leaf == "time")
            return index == 0 ? "Current" : "Duration";
        if (leaf == "clip")
            return index == 0 ? "Start" : "Duration";
        if (leaf == "fps")
            return index == 0 ? "Numerator" : "Denominator";
        if (leaf == "frame")
            return index == 0 ? "Current" : "Duration";
    }

    return value_name;
}

template <typename Callback>
void enumerate_flat_monitor_parameters(const std::map<std::string, core::monitor::vector_t>& entries,
                                       int                                                   first_child_number,
                                       Callback&&                                            callback)
{
    int                   child_number = first_child_number;
    std::set<std::string> used_identifiers;

    for (const auto& entry : entries) {
        if (entry.first.empty() || entry.second.empty())
            continue;

        const auto base_identifier  = ember_identifier_from_monitor_path(entry.first);
        const auto base_description = ember_description_from_monitor_path(entry.first);
        const auto component_count  = entry.second.size();

        for (std::size_t index = 0; index < component_count; ++index) {
            auto identifier  = base_identifier;
            auto description = base_description;

            if (component_count > 1) {
                const auto component_label = monitor_component_label(entry.first, index, component_count);
                identifier += component_label;
                description += " " + component_label;
            }

            if (!used_identifiers.insert(identifier).second) {
                const auto base = identifier;
                int        suffix = 2;
                do {
                    identifier = base + std::to_string(suffix++);
                } while (!used_identifiers.insert(identifier).second);
            }

            callback(child_number++, identifier, description, entry.second.at(index));
        }
    }
}

void append_flat_monitor_parameters(libember::glow::GlowNodeBase*                     parent,
                                    const std::map<std::string, core::monitor::vector_t>& entries,
                                    int                                                   first_child_number = 100)
{
    enumerate_flat_monitor_parameters(entries,
                                      first_child_number,
                                      [&](int child_number,
                                          const std::string& identifier,
                                          const std::string& description,
                                          const core::monitor::data_t& value) {
                                          add_string_parameter(
                                              parent, child_number, identifier, monitor_value_to_string(value), description);
                                      });
}

const amcp::channel_context* find_channel_context(const spl::shared_ptr<std::vector<amcp::channel_context>>& channels,
                                                  int                                                         channel_index)
{
    for (const auto& channel : *channels) {
        if (channel.raw_channel->index() == channel_index)
            return &channel;
    }

    return nullptr;
}

std::vector<int> layer_state_parameter_path(int channel_index, int layer_index, int parameter_number)
{
    return {channels_root_number, channel_index, layers_root_node_number, layer_index, layer_state_node_number, parameter_number};
}

std::vector<int> layer_mixer_parameter_path(int channel_index, int layer_index, int parameter_number)
{
    return {channels_root_number, channel_index, layers_root_node_number, layer_index, layer_mixer_node_number, parameter_number};
}

void send_live_layer_state_parameter(const IO::client_connection<char>::ptr& client,
                                     int                                      channel_index,
                                     const live_layer_state&                  layer,
                                     int                                      parameter_number)
{
    const auto path = layer_state_parameter_path(channel_index, layer.layer_index, parameter_number);

    switch (parameter_number) {
        case layer_state_foreground_param_number:
            send_qualified_parameter_update(client, path, [&](auto* p) { p->setValue(u8(layer.foreground)); });
            return;
        case layer_state_background_param_number:
            send_qualified_parameter_update(client, path, [&](auto* p) { p->setValue(u8(layer.background)); });
            return;
        case layer_state_paused_param_number:
            send_qualified_parameter_update(client, path, [&](auto* p) { p->setValue(layer.paused); });
            return;
        case layer_state_frames_left_param_number:
            if (layer.frames_left < 0)
                return;
            send_qualified_parameter_update(client, path, [&](auto* p) { p->setValue(static_cast<long>(layer.frames_left)); });
            return;
        default:
            return;
    }
}

void send_live_layer_mixer_parameter(const IO::client_connection<char>::ptr& client,
                                     int                                      channel_index,
                                     const live_layer_state&                  layer,
                                     int                                      parameter_number)
{
    const auto path = layer_mixer_parameter_path(channel_index, layer.layer_index, parameter_number);

    switch (parameter_number) {
        case layer_mixer_keyer_param_number:
            send_qualified_parameter_update(client, path, [&](auto* p) { p->setValue(layer.keyer); });
            return;
        case layer_mixer_invert_param_number:
            send_qualified_parameter_update(client, path, [&](auto* p) { p->setValue(layer.invert); });
            return;
        case layer_mixer_opacity_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(scaled_mixer_value(layer.opacity)); });
            return;
        case layer_mixer_brightness_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(scaled_mixer_value(layer.brightness)); });
            return;
        case layer_mixer_saturation_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(scaled_mixer_value(layer.saturation)); });
            return;
        case layer_mixer_contrast_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(scaled_mixer_value(layer.contrast)); });
            return;
        case layer_mixer_rotation_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(scaled_mixer_value(layer.rotation)); });
            return;
        case layer_mixer_volume_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(scaled_mixer_value(layer.volume)); });
            return;
        case layer_mixer_blend_param_number:
            send_qualified_parameter_update(client,
                                            path,
                                            [&](auto* p) { p->setValue(static_cast<long>(layer.blend)); });
            return;
        case layer_mixer_fill_x_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(scaled_mixer_value(layer.fill_x)); });
            return;
        case layer_mixer_fill_y_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(scaled_mixer_value(layer.fill_y)); });
            return;
        case layer_mixer_fill_w_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(scaled_mixer_value(layer.fill_w)); });
            return;
        case layer_mixer_fill_h_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(scaled_mixer_value(layer.fill_h)); });
            return;
        case layer_mixer_clip_x_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(scaled_mixer_value(layer.clip_x)); });
            return;
        case layer_mixer_clip_y_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(scaled_mixer_value(layer.clip_y)); });
            return;
        case layer_mixer_clip_w_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(scaled_mixer_value(layer.clip_w)); });
            return;
        case layer_mixer_clip_h_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(scaled_mixer_value(layer.clip_h)); });
            return;
        case layer_mixer_anchor_x_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(scaled_mixer_value(layer.anchor_x)); });
            return;
        case layer_mixer_anchor_y_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(scaled_mixer_value(layer.anchor_y)); });
            return;
        case layer_mixer_crop_l_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(scaled_mixer_value(layer.crop_l)); });
            return;
        case layer_mixer_crop_t_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(scaled_mixer_value(layer.crop_t)); });
            return;
        case layer_mixer_crop_r_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(scaled_mixer_value(layer.crop_r)); });
            return;
        case layer_mixer_crop_b_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(scaled_mixer_value(layer.crop_b)); });
            return;
        case layer_mixer_opacity_pct_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(static_cast<long>(std::llround(layer.opacity * 100.0))); });
            return;
        case layer_mixer_brightness_pct_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(static_cast<long>(std::llround(layer.brightness * 100.0))); });
            return;
        case layer_mixer_saturation_pct_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(static_cast<long>(std::llround(layer.saturation * 100.0))); });
            return;
        case layer_mixer_contrast_pct_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(static_cast<long>(std::llround(layer.contrast * 100.0))); });
            return;
        case layer_mixer_volume_pct_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(static_cast<long>(std::llround(layer.volume * 100.0))); });
            return;
        case layer_mixer_fill_x_pct_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(static_cast<long>(std::llround(layer.fill_x * 100.0))); });
            return;
        case layer_mixer_fill_y_pct_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(static_cast<long>(std::llround(layer.fill_y * 100.0))); });
            return;
        case layer_mixer_fill_w_pct_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(static_cast<long>(std::llround(layer.fill_w * 100.0))); });
            return;
        case layer_mixer_fill_h_pct_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(static_cast<long>(std::llround(layer.fill_h * 100.0))); });
            return;
        case layer_mixer_clip_x_pct_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(static_cast<long>(std::llround(layer.clip_x * 100.0))); });
            return;
        case layer_mixer_clip_y_pct_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(static_cast<long>(std::llround(layer.clip_y * 100.0))); });
            return;
        case layer_mixer_clip_w_pct_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(static_cast<long>(std::llround(layer.clip_w * 100.0))); });
            return;
        case layer_mixer_clip_h_pct_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(static_cast<long>(std::llround(layer.clip_h * 100.0))); });
            return;
        case layer_mixer_anchor_x_pct_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(static_cast<long>(std::llround(layer.anchor_x * 100.0))); });
            return;
        case layer_mixer_anchor_y_pct_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(static_cast<long>(std::llround(layer.anchor_y * 100.0))); });
            return;
        case layer_mixer_crop_l_pct_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(static_cast<long>(std::llround(layer.crop_l * 100.0))); });
            return;
        case layer_mixer_crop_t_pct_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(static_cast<long>(std::llround(layer.crop_t * 100.0))); });
            return;
        case layer_mixer_crop_r_pct_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(static_cast<long>(std::llround(layer.crop_r * 100.0))); });
            return;
        case layer_mixer_crop_b_pct_param_number:
            send_qualified_parameter_update(
                client, path, [&](auto* p) { p->setValue(static_cast<long>(std::llround(layer.crop_b * 100.0))); });
            return;
        default:
            return;
    }
}

void send_live_layer_snapshot(const IO::client_connection<char>::ptr& client,
                              int                                      channel_index,
                              const live_layer_state&                  layer)
{
    send_live_layer_state_parameter(client, channel_index, layer, layer_state_foreground_param_number);
    send_live_layer_state_parameter(client, channel_index, layer, layer_state_background_param_number);
    send_live_layer_state_parameter(client, channel_index, layer, layer_state_paused_param_number);
    send_live_layer_state_parameter(client, channel_index, layer, layer_state_frames_left_param_number);

    for (auto parameter_number = layer_mixer_keyer_param_number; parameter_number <= layer_mixer_last_param_number;
         ++parameter_number) {
        send_live_layer_mixer_parameter(client, channel_index, layer, parameter_number);
    }
}

void send_live_layer_diff(const IO::client_connection<char>::ptr& client,
                          int                                      channel_index,
                          const live_layer_state&                  previous,
                          const live_layer_state&                  current)
{
    if (previous.foreground != current.foreground)
        send_live_layer_state_parameter(client, channel_index, current, layer_state_foreground_param_number);
    if (previous.background != current.background)
        send_live_layer_state_parameter(client, channel_index, current, layer_state_background_param_number);
    if (previous.paused != current.paused)
        send_live_layer_state_parameter(client, channel_index, current, layer_state_paused_param_number);
    if (previous.frames_left != current.frames_left)
        send_live_layer_state_parameter(client, channel_index, current, layer_state_frames_left_param_number);

    if (previous.keyer != current.keyer)
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_keyer_param_number);
    if (previous.invert != current.invert)
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_invert_param_number);
    if (!nearly_equal(previous.opacity, current.opacity)) {
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_opacity_param_number);
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_opacity_pct_param_number);
    }
    if (!nearly_equal(previous.brightness, current.brightness)) {
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_brightness_param_number);
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_brightness_pct_param_number);
    }
    if (!nearly_equal(previous.saturation, current.saturation)) {
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_saturation_param_number);
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_saturation_pct_param_number);
    }
    if (!nearly_equal(previous.contrast, current.contrast)) {
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_contrast_param_number);
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_contrast_pct_param_number);
    }
    if (!nearly_equal(previous.rotation, current.rotation))
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_rotation_param_number);
    if (!nearly_equal(previous.volume, current.volume)) {
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_volume_param_number);
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_volume_pct_param_number);
    }
    if (previous.blend != current.blend)
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_blend_param_number);
    if (!nearly_equal(previous.fill_x, current.fill_x)) {
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_fill_x_param_number);
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_fill_x_pct_param_number);
    }
    if (!nearly_equal(previous.fill_y, current.fill_y)) {
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_fill_y_param_number);
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_fill_y_pct_param_number);
    }
    if (!nearly_equal(previous.fill_w, current.fill_w)) {
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_fill_w_param_number);
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_fill_w_pct_param_number);
    }
    if (!nearly_equal(previous.fill_h, current.fill_h)) {
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_fill_h_param_number);
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_fill_h_pct_param_number);
    }
    if (!nearly_equal(previous.clip_x, current.clip_x)) {
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_clip_x_param_number);
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_clip_x_pct_param_number);
    }
    if (!nearly_equal(previous.clip_y, current.clip_y)) {
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_clip_y_param_number);
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_clip_y_pct_param_number);
    }
    if (!nearly_equal(previous.clip_w, current.clip_w)) {
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_clip_w_param_number);
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_clip_w_pct_param_number);
    }
    if (!nearly_equal(previous.clip_h, current.clip_h)) {
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_clip_h_param_number);
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_clip_h_pct_param_number);
    }
    if (!nearly_equal(previous.anchor_x, current.anchor_x)) {
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_anchor_x_param_number);
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_anchor_x_pct_param_number);
    }
    if (!nearly_equal(previous.anchor_y, current.anchor_y)) {
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_anchor_y_param_number);
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_anchor_y_pct_param_number);
    }
    if (!nearly_equal(previous.crop_l, current.crop_l)) {
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_crop_l_param_number);
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_crop_l_pct_param_number);
    }
    if (!nearly_equal(previous.crop_t, current.crop_t)) {
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_crop_t_param_number);
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_crop_t_pct_param_number);
    }
    if (!nearly_equal(previous.crop_r, current.crop_r)) {
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_crop_r_param_number);
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_crop_r_pct_param_number);
    }
    if (!nearly_equal(previous.crop_b, current.crop_b)) {
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_crop_b_param_number);
        send_live_layer_mixer_parameter(client, channel_index, current, layer_mixer_crop_b_pct_param_number);
    }
}

std::vector<std::wstring> available_play_clips()
{
    std::vector<std::wstring> clips = {L""};
    std::set<std::wstring>    unique_clips;

    try {
        const boost::filesystem::path media_root(env::media_folder());
        if (!boost::filesystem::exists(media_root))
            return clips;

        for (boost::filesystem::recursive_directory_iterator it(media_root), end; it != end; ++it) {
            if (!boost::filesystem::is_regular_file(it->path()))
                continue;

            const auto clip_path = get_relative_without_extension(it->path(), media_root).generic_wstring();
            if (clip_path.empty())
                continue;

            unique_clips.insert(clip_path);
        }
    } catch (...) {
        CASPAR_LOG_CURRENT_EXCEPTION();
        return clips;
    }

    clips.insert(clips.end(), unique_clips.begin(), unique_clips.end());
    return clips;
}

std::vector<std::string> play_clip_labels(const std::vector<std::wstring>& clips)
{
    std::vector<std::string> labels;
    labels.reserve(clips.size());

    for (std::size_t i = 0; i < clips.size(); ++i)
        labels.push_back(i == 0 ? "None" : u8(clips[i]));

    return labels;
}

long find_clip_index(const std::wstring& file_name, const std::vector<std::wstring>& clips)
{
    if (file_name.empty())
        return 0;

    for (std::size_t i = 1; i < clips.size(); ++i) {
        if (boost::iequals(clips[i], file_name))
            return static_cast<long>(i);
    }

    return 0;
}

std::wstring clip_from_value(const libember::glow::Value& value, const std::vector<std::wstring>& clips)
{
    if (value.type().value() == libember::glow::ParameterType::Integer) {
        const auto index = std::max(0L, std::min(value.toInteger(), static_cast<long>(clips.size() - 1)));
        return clips.at(static_cast<std::size_t>(index));
    }

    auto clip = u16(value.toString());
    if (clip.empty() || boost::iequals(clip, L"None"))
        return L"";

    for (std::size_t i = 1; i < clips.size(); ++i) {
        if (boost::iequals(clips[i], clip))
            return clips[i];
    }

    return clip;
}

long enum_index_from_value(const libember::glow::Value& value, const std::vector<std::string>& labels)
{
    if (labels.empty())
        return 0;

    if (value.type().value() == libember::glow::ParameterType::Integer)
        return std::max(0L, std::min(value.toInteger(), static_cast<long>(labels.size() - 1)));

    const auto text = boost::to_lower_copy(value.toString());
    for (std::size_t i = 0; i < labels.size(); ++i) {
        if (boost::iequals(labels[i], text))
            return static_cast<long>(i);
    }

    return 0;
}

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

std::wstring compose_layer_action_command(const std::wstring& command_name, int channel_index, int layer);

std::wstring trim_copy(const std::wstring& value)
{
    return boost::trim_copy(value);
}

std::wstring compose_sting_arguments(const ember_provider::clip_command_state& state)
{
    if (!state.sting_enabled || state.sting_mask.empty())
        return L"";

    std::wstring arguments = L"STING (MASK=" + quote_amcp_token(state.sting_mask);

    if (state.sting_trigger_point > 0)
        arguments += L" trigger_point=" + std::to_wstring(state.sting_trigger_point);

    if (!state.sting_overlay.empty())
        arguments += L" overlay=" + quote_amcp_token(state.sting_overlay);

    if (state.sting_audio_fade_start > 0)
        arguments += L" audio_fade_start=" + std::to_wstring(state.sting_audio_fade_start);

    if (state.sting_audio_fade_duration > 0)
        arguments += L" audio_fade_duration=" + std::to_wstring(state.sting_audio_fade_duration);

    arguments += L")";
    return arguments;
}

std::wstring compose_clip_source_arguments(const ember_provider::clip_command_state& state,
                                           bool                                       include_transition,
                                           bool                                       include_auto,
                                           bool                                       include_sting)
{
    if (state.file_name.empty())
        return L"";

    std::wstring command = L" ";
    command += quote_amcp_token(state.file_name);

    if (state.loop)
        command += L" LOOP";

    if (state.seek > 0)
        command += L" SEEK " + std::to_wstring(state.seek);

    if (state.length > 0)
        command += L" LENGTH " + std::to_wstring(state.length);

    if (!state.filter.empty())
        command += L" FILTER " + quote_amcp_token(state.filter);

    if (state.clear_on_404)
        command += L" CLEAR_ON_404";

    if (include_auto && state.auto_play)
        command += L" AUTO";

    if (include_sting && state.sting_enabled) {
        const auto sting = compose_sting_arguments(state);
        if (!sting.empty())
            command += L" " + sting;
        return command;
    }

    if (include_transition) {
        const auto transition = transition_token(state.transition);
        if (!transition.empty() && state.transition_duration > 0) {
            command += L" " + transition + L" " + std::to_wstring(state.transition_duration);

            if (!state.tween.empty())
                command += L" " + state.tween;

            const auto direction = direction_token(state.direction);
            if (!direction.empty())
                command += L" " + direction;
        }
    }

    return command;
}

std::wstring compose_clip_command(const std::wstring&                       command_name,
                                  int                                       channel_index,
                                  const ember_provider::clip_command_state& state)
{
    std::wstring command = command_name + L" " + std::to_wstring(channel_index);
    if (state.layer > 0)
        command += L"-" + std::to_wstring(state.layer);

    command += compose_clip_source_arguments(state, true, false, false);

    return command;
}

std::wstring compose_loadbg_command(int channel_index, const ember_provider::clip_command_state& state)
{
    std::wstring command = L"LOADBG " + std::to_wstring(channel_index);
    if (state.layer > 0)
        command += L"-" + std::to_wstring(state.layer);

    command += compose_clip_source_arguments(state, true, true, true);
    return command;
}

std::wstring compose_load_command(int channel_index, const ember_provider::clip_command_state& state)
{
    std::wstring command = L"LOAD " + std::to_wstring(channel_index);
    if (state.layer > 0)
        command += L"-" + std::to_wstring(state.layer);

    command += compose_clip_source_arguments(state, false, false, false);
    return command;
}

std::wstring compose_call_command(const std::wstring&                       command_name,
                                  int                                       channel_index,
                                  const ember_provider::call_command_state& state)
{
    auto command = compose_layer_action_command(command_name, channel_index, state.layer);
    const auto arguments = trim_copy(state.arguments);
    if (!arguments.empty())
        command += L" " + arguments;

    return command;
}

std::wstring compose_layer_action_command(const std::wstring& command_name, int channel_index, int layer)
{
    std::wstring command = command_name + L" " + std::to_wstring(channel_index);
    if (layer > 0)
        command += L"-" + std::to_wstring(layer);
    return command;
}

std::wstring compose_clear_command(int channel_index, int layer)
{
    std::wstring command = L"CLEAR " + std::to_wstring(channel_index);
    if (layer > 0)
        command += L"-" + std::to_wstring(layer);
    return command;
}

void send_qualified_parameter_update(const IO::client_connection<char>::ptr& client,
                                     const std::vector<int>&                 path,
                                     const std::function<void(libember::glow::GlowQualifiedParameter*)>& setter)
{
    libember::glow::GlowRootElementCollection root;
    auto* qualified_parameter =
        new libember::glow::GlowQualifiedParameter(libember::ber::ObjectIdentifier(path.begin(), path.end()));
    setter(qualified_parameter);
    root.insert(root.end(), qualified_parameter);
    client->send(ember_message_builder::encode(root), true);
}

bool is_execute_request(const libember::glow::Value& value)
{
    switch (value.type().value()) {
        case libember::glow::ParameterType::Boolean:
            return value.toBoolean();
        case libember::glow::ParameterType::Integer:
            return value.toInteger() != 0;
        case libember::glow::ParameterType::String: {
            auto text = boost::to_lower_copy(value.toString());
            return !text.empty() && text != "0" && text != "false" && text != "off";
        }
        case libember::glow::ParameterType::None:
            return true;
        default:
            return false;
    }
}

long interval_ms_from_value(const libember::glow::Value& value, long default_value)
{
    switch (value.type().value()) {
        case libember::glow::ParameterType::Integer:
            return static_cast<long>(value.toInteger());
        case libember::glow::ParameterType::Real:
            return static_cast<long>(std::llround(value.toReal()));
        case libember::glow::ParameterType::Boolean:
            return value.toBoolean() ? default_value : minimum_state_update_interval_ms;
        case libember::glow::ParameterType::String:
            try {
                return static_cast<long>(std::stol(value.toString()));
            } catch (...) {
                return default_value;
            }
        default:
            return default_value;
    }
}

long clamp_state_update_interval_ms(long interval_ms)
{
    return std::max(minimum_state_update_interval_ms, std::min(interval_ms, maximum_state_update_interval_ms));
}

bool patch_ember_controller_interval(std::string& xml, long interval_ms)
{
    const auto xml_lower = boost::to_lower_copy(xml);

    std::size_t search_offset = 0;
    while (true) {
        const auto tcp_start = xml_lower.find("<tcp", search_offset);
        if (tcp_start == std::string::npos)
            return false;

        const auto tcp_open_end = xml_lower.find('>', tcp_start);
        const auto tcp_end      = xml_lower.find("</tcp>", tcp_open_end == std::string::npos ? tcp_start : tcp_open_end);
        if (tcp_open_end == std::string::npos || tcp_end == std::string::npos)
            return false;

        const auto protocol_start = xml_lower.find("<protocol>", tcp_open_end);
        if (protocol_start != std::string::npos && protocol_start < tcp_end) {
            const auto protocol_value_start = protocol_start + std::string("<protocol>").size();
            const auto protocol_value_end   = xml_lower.find("</protocol>", protocol_value_start);
            if (protocol_value_end != std::string::npos && protocol_value_end < tcp_end) {
                const auto protocol_value =
                    boost::trim_copy(xml.substr(protocol_value_start, protocol_value_end - protocol_value_start));
                if (boost::iequals(protocol_value, "EMBER_PLUS") || boost::iequals(protocol_value, "EMBER+") ||
                    boost::iequals(protocol_value, "EMBERPLUS")) {
                    const auto interval_tag_start = xml_lower.find("<state-update-interval-ms>", protocol_value_end);
                    if (interval_tag_start != std::string::npos && interval_tag_start < tcp_end) {
                        const auto interval_value_start =
                            interval_tag_start + std::string("<state-update-interval-ms>").size();
                        const auto interval_value_end =
                            xml_lower.find("</state-update-interval-ms>", interval_value_start);
                        if (interval_value_end == std::string::npos || interval_value_end > tcp_end)
                            return false;

                        xml.replace(interval_value_start,
                                    interval_value_end - interval_value_start,
                                    std::to_string(interval_ms));
                        return true;
                    }

                    const auto protocol_close_end = protocol_value_end + std::string("</protocol>").size();
                    std::string newline           = "\n";
                    const auto  line_start        = xml.rfind('\n', protocol_start);
                    if (line_start != std::string::npos && line_start > 0 && xml[line_start - 1] == '\r')
                        newline = "\r\n";

                    std::string indent = "            ";
                    if (line_start != std::string::npos)
                        indent = xml.substr(line_start + 1, protocol_start - (line_start + 1));

                    xml.insert(protocol_close_end,
                               newline + indent + "<state-update-interval-ms>" + std::to_string(interval_ms) +
                                   "</state-update-interval-ms>");
                    return true;
                }
            }
        }

        search_offset = tcp_end + std::string("</tcp>").size();
    }
}

bool persist_state_update_interval_to_config(long interval_ms, std::wstring& error_message)
{
    try {
        const auto config_path = boost::filesystem::path(env::configuration_file());
        boost::filesystem::ifstream input(config_path, std::ios::in | std::ios::binary);
        if (!input) {
            error_message = L"Unable to open configuration file for reading.";
            return false;
        }

        const auto xml = std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
        input.close();

        auto patched_xml = xml;
        if (!patch_ember_controller_interval(patched_xml, interval_ms)) {
            error_message = L"Unable to locate EMBER_PLUS controller entry in configuration file.";
            return false;
        }

        boost::filesystem::ofstream output(config_path, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!output) {
            error_message = L"Unable to open configuration file for writing.";
            return false;
        }

        output.write(patched_xml.data(), static_cast<std::streamsize>(patched_xml.size()));
        if (!output.good()) {
            error_message = L"Failed while writing configuration file.";
            return false;
        }

        return true;
    } catch (const std::exception& ex) {
        error_message = u16(ex.what());
        return false;
    } catch (...) {
        error_message = L"Unknown error while persisting configuration file.";
        return false;
    }
}

bool contains_directory_request(const libember::dom::Node& node)
{
    const auto type = libember::ber::Type::fromTag(node.typeTag());
    if (type.isApplicationDefined() && type.value() == libember::glow::GlowType::Command) {
        const auto& command = dynamic_cast<const libember::glow::GlowCommand&>(node);
        if (command.number().value() == libember::glow::CommandType::GetDirectory)
            return true;
    }

    const auto* container = dynamic_cast<const libember::dom::Container*>(&node);
    if (container == nullptr)
        return false;

    for (auto it = container->begin(); it != container->end(); ++it) {
        if (contains_directory_request(*it))
            return true;
    }

    return false;
}

std::vector<int> to_path(const libember::ber::ObjectIdentifier& oid)
{
    return std::vector<int>(oid.begin(), oid.end());
}

void collect_function_invocations(const libember::glow::GlowFunctionBase&      function,
                                  const std::vector<int>&                       path,
                                  std::vector<function_invocation_request>& requests)
{
    auto* children = function.children();
    if (children == nullptr)
        return;

    for (auto it = children->begin(); it != children->end(); ++it) {
        const auto type = libember::ber::Type::fromTag(it->typeTag());
        if (!type.isApplicationDefined() || type.value() != libember::glow::GlowType::Command)
            continue;

        const auto& command = dynamic_cast<const libember::glow::GlowCommand&>(*it);
        if (command.number().value() != libember::glow::CommandType::Invoke || command.invocation() == nullptr)
            continue;

        requests.push_back({path, command.invocation()});
    }
}

void collect_requests(const libember::dom::Node&                 node,
                      const std::vector<int>&                    current_path,
                      std::vector<function_invocation_request>& requests)
{
    const auto type = libember::ber::Type::fromTag(node.typeTag());
    if (!type.isApplicationDefined())
        return;

    switch (type.value()) {
        case libember::glow::GlowType::RootElementCollection:
        case libember::glow::GlowType::ElementCollection: {
            const auto* container = dynamic_cast<const libember::dom::Container*>(&node);
            if (container == nullptr)
                return;

            for (auto it = container->begin(); it != container->end(); ++it)
                collect_requests(*it, current_path, requests);
            return;
        }
        case libember::glow::GlowType::QualifiedNode: {
            const auto& qualified_node = dynamic_cast<const libember::glow::GlowQualifiedNode&>(node);
            const auto  path           = to_path(qualified_node.path());
            auto*       children       = qualified_node.children();
            if (children == nullptr)
                return;

            for (auto it = children->begin(); it != children->end(); ++it)
                collect_requests(*it, path, requests);
            return;
        }
        case libember::glow::GlowType::Node: {
            const auto& node_ref = dynamic_cast<const libember::glow::GlowNode&>(node);
            auto        path     = current_path;
            path.push_back(node_ref.number());

            auto* children = node_ref.children();
            if (children == nullptr)
                return;

            for (auto it = children->begin(); it != children->end(); ++it)
                collect_requests(*it, path, requests);
            return;
        }
        case libember::glow::GlowType::QualifiedFunction: {
            const auto& function = dynamic_cast<const libember::glow::GlowQualifiedFunction&>(node);
            collect_function_invocations(function, to_path(function.path()), requests);
            return;
        }
        case libember::glow::GlowType::Function: {
            const auto& function = dynamic_cast<const libember::glow::GlowFunction&>(node);
            auto        path     = current_path;
            path.push_back(function.number());
            collect_function_invocations(function, path, requests);
            return;
        }
        default:
            return;
    }
}

void collect_parameter_writes(const libember::dom::Node&              node,
                              const std::vector<int>&                 current_path,
                              std::vector<parameter_write_request>& writes)
{
    const auto type = libember::ber::Type::fromTag(node.typeTag());
    if (!type.isApplicationDefined())
        return;

    switch (type.value()) {
        case libember::glow::GlowType::RootElementCollection:
        case libember::glow::GlowType::ElementCollection: {
            const auto* container = dynamic_cast<const libember::dom::Container*>(&node);
            if (container == nullptr)
                return;

            for (auto it = container->begin(); it != container->end(); ++it)
                collect_parameter_writes(*it, current_path, writes);
            return;
        }
        case libember::glow::GlowType::QualifiedNode: {
            const auto& qualified_node = dynamic_cast<const libember::glow::GlowQualifiedNode&>(node);
            const auto  path           = to_path(qualified_node.path());
            auto*       children       = qualified_node.children();
            if (children == nullptr)
                return;

            for (auto it = children->begin(); it != children->end(); ++it)
                collect_parameter_writes(*it, path, writes);
            return;
        }
        case libember::glow::GlowType::Node: {
            const auto& node_ref = dynamic_cast<const libember::glow::GlowNode&>(node);
            auto        path     = current_path;
            path.push_back(node_ref.number());

            auto* children = node_ref.children();
            if (children == nullptr)
                return;

            for (auto it = children->begin(); it != children->end(); ++it)
                collect_parameter_writes(*it, path, writes);
            return;
        }
        case libember::glow::GlowType::QualifiedParameter: {
            const auto& parameter = dynamic_cast<const libember::glow::GlowQualifiedParameter&>(node);
            if (parameter.contains(libember::glow::ParameterProperty::Value))
                writes.push_back({to_path(parameter.path()), parameter.value()});
            return;
        }
        case libember::glow::GlowType::Parameter: {
            const auto& parameter = dynamic_cast<const libember::glow::GlowParameter&>(node);
            if (!parameter.contains(libember::glow::ParameterProperty::Value))
                return;

            auto path = current_path;
            path.push_back(parameter.number());
            writes.push_back({path, parameter.value()});
            return;
        }
        default:
            return;
    }
}

void send_invocation_result(const IO::client_connection<char>::ptr&      client,
                            int                                          invocation_id,
                            const ember_command_bridge::invocation_result& invocation_result)
{
    libember::glow::GlowInvocationResult response;
    if (invocation_id >= 0)
        response.setInvocationId(invocation_id);

    response.setSuccess(invocation_result.success);

    const auto encoded_message = u8(invocation_result.message);
    const std::vector<libember::glow::Value> result_values = {libember::glow::Value(encoded_message)};
    response.setTypedResult(result_values.begin(), result_values.end());

    client->send(ember_message_builder::encode(response), true);
}

std::string build_directory_response(const spl::shared_ptr<std::vector<amcp::channel_context>>& channels,
                                     const std::shared_ptr<ember_registry>&                     registry,
                                     const ember_command_bridge&                                command_bridge,
                                     long                                                       state_update_interval_ms,
                                     const live_snapshot_map_t&                                 live_snapshot,
                                     const std::vector<std::wstring>&                           media_clips,
                                     const std::map<int, ember_provider::clip_command_state>&   play_controls,
                                     const std::map<int, ember_provider::clip_command_state>&   loadbg_controls,
                                     const std::map<int, ember_provider::clip_command_state>&   load_controls,
                                     const std::map<int, ember_provider::layer_command_state>&  pause_controls,
                                     const std::map<int, ember_provider::layer_command_state>&  resume_controls,
                                     const std::map<int, ember_provider::layer_command_state>&  stop_controls,
                                     const std::map<int, ember_provider::clear_command_state>&  clear_controls,
                                     const std::map<int, ember_provider::refresh_command_state>& refresh_controls,
                                     const std::map<int, ember_provider::call_command_state>&   call_controls,
                                     const std::map<int, ember_provider::call_command_state>&   callbg_controls)
{
    const auto play_clip_enums  = play_clip_labels(media_clips);
    const auto transition_enums = play_transition_labels();
    const auto direction_enums  = play_direction_labels();

    auto root = std::unique_ptr<libember::glow::GlowRootElementCollection>(new libember::glow::GlowRootElementCollection());

    auto* identity = new libember::glow::GlowNode(root.get(), 1);
    identity->setIdentifier("identity");
    identity->setDescription("CasparCG identity");
    identity->setIsOnline(true);
    add_string_parameter(identity, 1, "product", "CasparCG Server", "CasparCG product name");
    add_string_parameter(identity, 2, "protocol", "Ember+", "Protocol endpoint type");

    auto* runtime = new libember::glow::GlowNode(root.get(), 2);
    runtime->setIdentifier("runtime");
    runtime->setDescription("CasparCG runtime");
    runtime->setIsOnline(true);
    add_integer_parameter(
        runtime,
        runtime_channel_count_param_number,
        "channel_count",
        static_cast<long>(channels->size()),
        "Number of configured video channels");
    add_ranged_integer_parameter(runtime,
                                 runtime_state_update_interval_param_number,
                                 "state_update_interval_ms",
                                 state_update_interval_ms,
                                 minimum_state_update_interval_ms,
                                 maximum_state_update_interval_ms,
                                 "StateUpdateIntervalMs",
                                 access_t::ReadWrite);

    auto* compatibility = new libember::glow::GlowNode(root.get(), 5);
    compatibility->setIdentifier("compatibility");
    compatibility->setDescription("Compatibility surfaces backed by existing CasparCG protocols");
    compatibility->setIsOnline(true);

    auto* amcp_execute = new libember::glow::GlowFunction(compatibility, 1);
    command_bridge.describe_amcp_execute_function(amcp_execute);

    auto* channel_root = new libember::glow::GlowNode(root.get(), 100);
    channel_root->setIdentifier("channels");
    channel_root->setDescription("Configured video channels");
    channel_root->setIsOnline(true);

    for (auto& channel : *channels) {
        const auto index       = channel.raw_channel->index();
        const auto format_name = u8(channel.raw_channel->stage()->video_format_desc().name);
        auto*      channel_node = new libember::glow::GlowNode(channel_root, index);
        channel_node->setIdentifier("channel_" + std::to_string(index));
        channel_node->setDescription("CasparCG channel " + std::to_string(index));
        channel_node->setIsOnline(true);

        add_integer_parameter(channel_node, 1, "index", static_cast<long>(index), "Channel index");
        add_string_parameter(channel_node, 2, "format", format_name, "Active video format");

        const auto add_clip_node =
            [&](int number,
                const std::string& name,
                const ember_provider::clip_command_state& state,
                bool include_loadbg_features) {
                auto* node = new libember::glow::GlowNode(channel_node, number);
                node->setIdentifier(name);
                node->setDescription(name);
                node->setIsOnline(true);

                add_integer_parameter(node, clip_layer_param_number, "Layer", state.layer, "Layer", access_t::ReadWrite);
                add_boolean_parameter(node, clip_loop_param_number, "Loop", state.loop, "Loop", access_t::ReadWrite);
                add_enum_parameter(node,
                                   clip_name_param_number,
                                   "Clip",
                                   find_clip_index(state.file_name, media_clips),
                                   play_clip_enums,
                                   "Clip",
                                   access_t::ReadWrite);
                add_integer_parameter(node, clip_seek_param_number, "Seek", state.seek, "Seek", access_t::ReadWrite);
                add_integer_parameter(node, clip_length_param_number, "Length", state.length, "Length", access_t::ReadWrite);
                add_string_parameter(node, clip_filter_param_number, "Filter", u8(state.filter), "Filter", access_t::ReadWrite);
                add_boolean_parameter(
                    node, clip_clear404_param_number, "Clear404", state.clear_on_404, "Clear404", access_t::ReadWrite);
                if (include_loadbg_features)
                    add_boolean_parameter(node, clip_auto_param_number, "Auto", state.auto_play, "Auto", access_t::ReadWrite);
                add_enum_parameter(node,
                                   clip_transition_param_number,
                                   "Transition",
                                   state.transition,
                                   transition_enums,
                                   "Transition",
                                   access_t::ReadWrite);
                add_integer_parameter(
                    node, clip_duration_param_number, "Duration", state.transition_duration, "Duration", access_t::ReadWrite);
                add_string_parameter(node, clip_tween_param_number, "Tween", u8(state.tween), "Tween", access_t::ReadWrite);
                add_enum_parameter(node,
                                   clip_direction_param_number,
                                   "Direction",
                                   state.direction,
                                   direction_enums,
                                   "Direction",
                                   access_t::ReadWrite);
                add_boolean_parameter(node, clip_execute_param_number, "Execute", false, "Execute", access_t::ReadWrite);
                add_string_parameter(node, clip_last_reply_param_number, "Reply", u8(state.last_reply), "Reply");
                add_boolean_parameter(node, clip_last_success_param_number, "Success", state.last_success, "Success");

                if (include_loadbg_features) {
                    auto* sting_node = new libember::glow::GlowNode(node, clip_sting_node_number);
                    sting_node->setIdentifier("Sting");
                    sting_node->setDescription("Sting");
                    sting_node->setIsOnline(true);

                    add_boolean_parameter(
                        sting_node, sting_enabled_param_number, "Enable", state.sting_enabled, "Enable", access_t::ReadWrite);
                    add_enum_parameter(sting_node,
                                       sting_mask_param_number,
                                       "Mask",
                                       find_clip_index(state.sting_mask, media_clips),
                                       play_clip_enums,
                                       "Mask",
                                       access_t::ReadWrite);
                    add_integer_parameter(
                        sting_node, sting_trigger_param_number, "Trigger", state.sting_trigger_point, "Trigger", access_t::ReadWrite);
                    add_enum_parameter(sting_node,
                                       sting_overlay_param_number,
                                       "Overlay",
                                       find_clip_index(state.sting_overlay, media_clips),
                                       play_clip_enums,
                                       "Overlay",
                                       access_t::ReadWrite);
                    add_integer_parameter(sting_node,
                                          sting_audio_fade_start_param_number,
                                          "AudioFadeStart",
                                          state.sting_audio_fade_start,
                                          "AudioFadeStart",
                                          access_t::ReadWrite);
                    add_integer_parameter(sting_node,
                                          sting_audio_fade_duration_param_number,
                                          "AudioFadeDuration",
                                          state.sting_audio_fade_duration,
                                          "AudioFadeDuration",
                                          access_t::ReadWrite);
                }
            };

        const auto add_load_node = [&](const ember_provider::clip_command_state& state) {
            auto* node = new libember::glow::GlowNode(channel_node, load_node_number);
            node->setIdentifier("Load");
            node->setDescription("Load");
            node->setIsOnline(true);

            add_integer_parameter(node, load_layer_param_number, "Layer", state.layer, "Layer", access_t::ReadWrite);
            add_enum_parameter(node,
                               load_name_param_number,
                               "Clip",
                               find_clip_index(state.file_name, media_clips),
                               play_clip_enums,
                               "Clip",
                               access_t::ReadWrite);
            add_boolean_parameter(node, load_loop_param_number, "Loop", state.loop, "Loop", access_t::ReadWrite);
            add_integer_parameter(node, load_seek_param_number, "Seek", state.seek, "Seek", access_t::ReadWrite);
            add_integer_parameter(node, load_length_param_number, "Length", state.length, "Length", access_t::ReadWrite);
            add_string_parameter(node, load_filter_param_number, "Filter", u8(state.filter), "Filter", access_t::ReadWrite);
            add_boolean_parameter(
                node, load_clear404_param_number, "Clear404", state.clear_on_404, "Clear404", access_t::ReadWrite);
            add_boolean_parameter(node, load_execute_param_number, "Execute", false, "Execute", access_t::ReadWrite);
            add_string_parameter(node, load_last_reply_param_number, "Reply", u8(state.last_reply), "Reply");
            add_boolean_parameter(node, load_last_success_param_number, "Success", state.last_success, "Success");
        };

        const auto add_action_node =
            [&](int number, const std::string& name, const ember_provider::layer_command_state& state) {
                auto* node = new libember::glow::GlowNode(channel_node, number);
                node->setIdentifier(name);
                node->setDescription(name);
                node->setIsOnline(true);

                add_integer_parameter(
                    node, action_layer_param_number, "Layer", state.layer, "Layer", access_t::ReadWrite);
                add_boolean_parameter(
                    node, action_execute_param_number, "Execute", false, "Execute", access_t::ReadWrite);
                add_string_parameter(node, action_last_reply_param_number, "Reply", u8(state.last_reply), "Reply");
                add_boolean_parameter(node, action_last_success_param_number, "Success", state.last_success, "Success");
            };

        const auto add_call_node =
            [&](int number, const std::string& name, const ember_provider::call_command_state& state) {
                auto* node = new libember::glow::GlowNode(channel_node, number);
                node->setIdentifier(name);
                node->setDescription(name);
                node->setIsOnline(true);

                add_integer_parameter(node, call_layer_param_number, "Layer", state.layer, "Layer", access_t::ReadWrite);
                add_string_parameter(
                    node, call_arguments_param_number, "Args", u8(state.arguments), "Args", access_t::ReadWrite);
                add_boolean_parameter(node, call_execute_param_number, "Execute", false, "Execute", access_t::ReadWrite);
                add_string_parameter(node, call_last_reply_param_number, "Reply", u8(state.last_reply), "Reply");
                add_boolean_parameter(node, call_last_success_param_number, "Success", state.last_success, "Success");
            };

        const auto add_clear_node = [&](const ember_provider::clear_command_state& state) {
            auto* node = new libember::glow::GlowNode(channel_node, clear_node_number);
            node->setIdentifier("Clear");
            node->setDescription("Clear");
            node->setIsOnline(true);

            add_integer_parameter(node, action_layer_param_number, "Layer", state.layer, "Layer", access_t::ReadWrite);
            add_boolean_parameter(
                node, action_execute_param_number, "Execute", false, "Execute", access_t::ReadWrite);
            add_string_parameter(node, action_last_reply_param_number, "Reply", u8(state.last_reply), "Reply");
            add_boolean_parameter(node, action_last_success_param_number, "Success", state.last_success, "Success");
        };

        const auto add_refresh_node = [&](const ember_provider::refresh_command_state& state) {
            auto* node = new libember::glow::GlowNode(channel_node, refresh_node_number);
            node->setIdentifier("Refresh");
            node->setDescription("Refresh");
            node->setIsOnline(true);

            add_boolean_parameter(
                node, refresh_execute_param_number, "Execute", false, "Execute", access_t::ReadWrite);
            add_integer_parameter(node,
                                  refresh_count_param_number,
                                  "Count",
                                  static_cast<long>(media_clips.size() > 0 ? media_clips.size() - 1 : 0),
                                  "Count");
            add_string_parameter(node, refresh_last_reply_param_number, "Reply", u8(state.last_reply), "Reply");
            add_boolean_parameter(node, refresh_last_success_param_number, "Success", state.last_success, "Success");
        };

        const auto play_it   = play_controls.find(index);
        const auto loadbg_it = loadbg_controls.find(index);
        const auto load_it   = load_controls.find(index);
        add_clip_node(play_node_number,
                      "Play",
                      play_it != play_controls.end() ? play_it->second : ember_provider::clip_command_state(),
                      false);
        add_clip_node(loadbg_node_number,
                      "LoadBg",
                      loadbg_it != loadbg_controls.end() ? loadbg_it->second : ember_provider::clip_command_state(),
                      true);
        add_load_node(load_it != load_controls.end() ? load_it->second : ember_provider::clip_command_state());

        const auto pause_it   = pause_controls.find(index);
        const auto resume_it  = resume_controls.find(index);
        const auto stop_it    = stop_controls.find(index);
        const auto clear_it   = clear_controls.find(index);
        const auto refresh_it = refresh_controls.find(index);
        const auto call_it    = call_controls.find(index);
        const auto callbg_it  = callbg_controls.find(index);

        add_action_node(
            pause_node_number,
            "Pause",
            pause_it != pause_controls.end() ? pause_it->second : ember_provider::layer_command_state());
        add_action_node(
            resume_node_number,
            "Resume",
            resume_it != resume_controls.end() ? resume_it->second : ember_provider::layer_command_state());
        add_action_node(
            stop_node_number,
            "Stop",
            stop_it != stop_controls.end() ? stop_it->second : ember_provider::layer_command_state());
        add_clear_node(clear_it != clear_controls.end() ? clear_it->second : ember_provider::clear_command_state());
        add_refresh_node(
            refresh_it != refresh_controls.end() ? refresh_it->second : ember_provider::refresh_command_state());
        add_call_node(call_node_number,
                      "Call",
                      call_it != call_controls.end() ? call_it->second : ember_provider::call_command_state());
        add_call_node(callbg_node_number,
                      "CallBg",
                      callbg_it != callbg_controls.end() ? callbg_it->second : ember_provider::call_command_state());

        auto* layers_node = new libember::glow::GlowNode(channel_node, layers_root_node_number);
        layers_node->setIdentifier("Layers");
        layers_node->setDescription("Layers");
        layers_node->setIsOnline(true);

        const auto live_channel_it = live_snapshot.find(index);
        if (live_channel_it != live_snapshot.end()) {
            for (const auto& live_layer_entry : live_channel_it->second) {
                const auto& live_layer = live_layer_entry.second;

                auto* layer_node = new libember::glow::GlowNode(layers_node, live_layer.layer_index);
                layer_node->setIdentifier("Layer_" + std::to_string(live_layer.layer_index));
                layer_node->setDescription("Layer " + std::to_string(live_layer.layer_index));
                layer_node->setIsOnline(true);

                auto* state_node = new libember::glow::GlowNode(layer_node, layer_state_node_number);
                state_node->setIdentifier("State");
                state_node->setDescription("State");
                state_node->setIsOnline(true);
                add_string_parameter(
                    state_node, layer_state_foreground_param_number, "Foreground", u8(live_layer.foreground), "Foreground");
                add_string_parameter(
                    state_node, layer_state_background_param_number, "Background", u8(live_layer.background), "Background");
                add_boolean_parameter(
                    state_node, layer_state_paused_param_number, "Paused", live_layer.paused, "Paused");
                if (live_layer.frames_left >= 0) {
                    add_integer_parameter(state_node,
                                          layer_state_frames_left_param_number,
                                          "FramesLeft",
                                          static_cast<long>(live_layer.frames_left),
                                          "FramesLeft");
                }
                auto monitor_entries = live_layer.monitor_entries;
                monitor_entries.erase("foreground/producer");
                monitor_entries.erase("background/producer");
                monitor_entries.erase("foreground/paused");
                monitor_entries.erase("foreground/frames_left");
                if (!monitor_entries.empty())
                    append_flat_monitor_parameters(state_node, monitor_entries);

                auto* mixer_node = new libember::glow::GlowNode(layer_node, layer_mixer_node_number);
                mixer_node->setIdentifier("Mixer");
                mixer_node->setDescription("Mixer");
                mixer_node->setIsOnline(true);
                add_boolean_parameter(
                    mixer_node, layer_mixer_keyer_param_number, "Keyer", live_layer.keyer, "Keyer", access_t::ReadWrite);
                add_boolean_parameter(
                    mixer_node, layer_mixer_invert_param_number, "Invert", live_layer.invert, "Invert", access_t::ReadWrite);
                add_scaled_parameter(
                    mixer_node, layer_mixer_opacity_param_number, "Opacity", live_layer.opacity, "Opacity", access_t::ReadWrite);
                add_percent_parameter(
                    mixer_node, layer_mixer_opacity_pct_param_number, "Opacity%", live_layer.opacity, "Opacity%", access_t::ReadWrite);
                add_scaled_parameter(mixer_node,
                                     layer_mixer_brightness_param_number,
                                     "Brightness",
                                     live_layer.brightness,
                                     "Brightness",
                                     access_t::ReadWrite);
                add_percent_parameter(mixer_node,
                                      layer_mixer_brightness_pct_param_number,
                                      "Brightness%",
                                      live_layer.brightness,
                                      "Brightness%",
                                      access_t::ReadWrite);
                add_scaled_parameter(mixer_node,
                                     layer_mixer_saturation_param_number,
                                     "Saturation",
                                     live_layer.saturation,
                                     "Saturation",
                                     access_t::ReadWrite);
                add_percent_parameter(mixer_node,
                                      layer_mixer_saturation_pct_param_number,
                                      "Saturation%",
                                      live_layer.saturation,
                                      "Saturation%",
                                      access_t::ReadWrite);
                add_scaled_parameter(
                    mixer_node, layer_mixer_contrast_param_number, "Contrast", live_layer.contrast, "Contrast", access_t::ReadWrite);
                add_percent_parameter(
                    mixer_node, layer_mixer_contrast_pct_param_number, "Contrast%", live_layer.contrast, "Contrast%", access_t::ReadWrite);
                add_scaled_parameter(
                    mixer_node, layer_mixer_rotation_param_number, "Rotation", live_layer.rotation, "Rotation", access_t::ReadWrite);
                add_scaled_parameter(
                    mixer_node, layer_mixer_volume_param_number, "Volume", live_layer.volume, "Volume", access_t::ReadWrite);
                add_percent_parameter(
                    mixer_node, layer_mixer_volume_pct_param_number, "Volume%", live_layer.volume, "Volume%", access_t::ReadWrite);
                add_enum_parameter(mixer_node,
                                   layer_mixer_blend_param_number,
                                   "Blend",
                                   live_layer.blend,
                                   mixer_blend_labels(),
                                   "Blend",
                                   access_t::ReadWrite);
                add_scaled_parameter(
                    mixer_node, layer_mixer_fill_x_param_number, "FillX", live_layer.fill_x, "FillX", access_t::ReadWrite);
                add_percent_parameter(
                    mixer_node, layer_mixer_fill_x_pct_param_number, "FillX%", live_layer.fill_x, "FillX%", access_t::ReadWrite);
                add_scaled_parameter(
                    mixer_node, layer_mixer_fill_y_param_number, "FillY", live_layer.fill_y, "FillY", access_t::ReadWrite);
                add_percent_parameter(
                    mixer_node, layer_mixer_fill_y_pct_param_number, "FillY%", live_layer.fill_y, "FillY%", access_t::ReadWrite);
                add_scaled_parameter(
                    mixer_node, layer_mixer_fill_w_param_number, "FillW", live_layer.fill_w, "FillW", access_t::ReadWrite);
                add_percent_parameter(
                    mixer_node, layer_mixer_fill_w_pct_param_number, "FillW%", live_layer.fill_w, "FillW%", access_t::ReadWrite);
                add_scaled_parameter(
                    mixer_node, layer_mixer_fill_h_param_number, "FillH", live_layer.fill_h, "FillH", access_t::ReadWrite);
                add_percent_parameter(
                    mixer_node, layer_mixer_fill_h_pct_param_number, "FillH%", live_layer.fill_h, "FillH%", access_t::ReadWrite);
                add_scaled_parameter(
                    mixer_node, layer_mixer_clip_x_param_number, "ClipX", live_layer.clip_x, "ClipX", access_t::ReadWrite);
                add_percent_parameter(
                    mixer_node, layer_mixer_clip_x_pct_param_number, "ClipX%", live_layer.clip_x, "ClipX%", access_t::ReadWrite);
                add_scaled_parameter(
                    mixer_node, layer_mixer_clip_y_param_number, "ClipY", live_layer.clip_y, "ClipY", access_t::ReadWrite);
                add_percent_parameter(
                    mixer_node, layer_mixer_clip_y_pct_param_number, "ClipY%", live_layer.clip_y, "ClipY%", access_t::ReadWrite);
                add_scaled_parameter(
                    mixer_node, layer_mixer_clip_w_param_number, "ClipW", live_layer.clip_w, "ClipW", access_t::ReadWrite);
                add_percent_parameter(
                    mixer_node, layer_mixer_clip_w_pct_param_number, "ClipW%", live_layer.clip_w, "ClipW%", access_t::ReadWrite);
                add_scaled_parameter(
                    mixer_node, layer_mixer_clip_h_param_number, "ClipH", live_layer.clip_h, "ClipH", access_t::ReadWrite);
                add_percent_parameter(
                    mixer_node, layer_mixer_clip_h_pct_param_number, "ClipH%", live_layer.clip_h, "ClipH%", access_t::ReadWrite);
                add_scaled_parameter(
                    mixer_node, layer_mixer_anchor_x_param_number, "AnchorX", live_layer.anchor_x, "AnchorX", access_t::ReadWrite);
                add_percent_parameter(mixer_node,
                                      layer_mixer_anchor_x_pct_param_number,
                                      "AnchorX%",
                                      live_layer.anchor_x,
                                      "AnchorX%",
                                      access_t::ReadWrite);
                add_scaled_parameter(
                    mixer_node, layer_mixer_anchor_y_param_number, "AnchorY", live_layer.anchor_y, "AnchorY", access_t::ReadWrite);
                add_percent_parameter(mixer_node,
                                      layer_mixer_anchor_y_pct_param_number,
                                      "AnchorY%",
                                      live_layer.anchor_y,
                                      "AnchorY%",
                                      access_t::ReadWrite);
                add_scaled_parameter(
                    mixer_node, layer_mixer_crop_l_param_number, "CropL", live_layer.crop_l, "CropL", access_t::ReadWrite);
                add_percent_parameter(
                    mixer_node, layer_mixer_crop_l_pct_param_number, "CropL%", live_layer.crop_l, "CropL%", access_t::ReadWrite);
                add_scaled_parameter(
                    mixer_node, layer_mixer_crop_t_param_number, "CropT", live_layer.crop_t, "CropT", access_t::ReadWrite);
                add_percent_parameter(
                    mixer_node, layer_mixer_crop_t_pct_param_number, "CropT%", live_layer.crop_t, "CropT%", access_t::ReadWrite);
                add_scaled_parameter(
                    mixer_node, layer_mixer_crop_r_param_number, "CropR", live_layer.crop_r, "CropR", access_t::ReadWrite);
                add_percent_parameter(
                    mixer_node, layer_mixer_crop_r_pct_param_number, "CropR%", live_layer.crop_r, "CropR%", access_t::ReadWrite);
                add_scaled_parameter(
                    mixer_node, layer_mixer_crop_b_param_number, "CropB", live_layer.crop_b, "CropB", access_t::ReadWrite);
                add_percent_parameter(
                    mixer_node, layer_mixer_crop_b_pct_param_number, "CropB%", live_layer.crop_b, "CropB%", access_t::ReadWrite);
            }
        }
    }

    auto* modules = new libember::glow::GlowNode(root.get(), 200);
    modules->setIdentifier("modules");
    modules->setDescription("Registered Ember+ module roots");
    modules->setIsOnline(true);

    for (const auto& module_root : registry->module_roots()) {
        auto* module_node = new libember::glow::GlowNode(modules, module_root.number);
        module_node->setIdentifier(module_root.identifier);
        if (!module_root.description.empty())
            module_node->setDescription(module_root.description);
        module_node->setIsOnline(true);
    }

    return ember_message_builder::encode(*root);
}

} // namespace

ember_provider::ember_provider(const spl::shared_ptr<std::vector<amcp::channel_context>>& channels,
                               const std::shared_ptr<amcp::amcp_command_repository>&      amcp_command_repository,
                               std::chrono::milliseconds                                   monitor_interval,
                               std::shared_ptr<ember_registry>                              registry)
    : channels_(channels)
    , registry_(std::move(registry))
    , command_bridge_(amcp_command_repository)
    , media_clips_(available_play_clips())
    , monitor_interval_(monitor_interval.count() > 0 ? monitor_interval : std::chrono::milliseconds(1000))
{
    for (auto& channel : *channels_) {
        const auto index = channel.raw_channel->index();
        play_controls_.emplace(index, clip_command_state());
        loadbg_controls_.emplace(index, clip_command_state());
        load_controls_.emplace(index, clip_command_state());
        pause_controls_.emplace(index, layer_command_state());
        resume_controls_.emplace(index, layer_command_state());
        stop_controls_.emplace(index, layer_command_state());
        clear_controls_.emplace(index, clear_command_state());
        refresh_controls_.emplace(index, refresh_command_state());
        call_controls_.emplace(index, call_command_state());
        callbg_controls_.emplace(index, call_command_state());
    }

    monitor_thread_ = std::thread([this] { monitor_layer_changes(); });
}

ember_provider::~ember_provider()
{
    stop_monitor_ = true;
    monitor_interval_cv_.notify_all();
    if (monitor_thread_.joinable())
        monitor_thread_.join();
}

void ember_provider::send_provider_state(const IO::client_connection<char>::ptr& client, bool online) const
{
    client->send(make_provider_state_packet(online), true);
}

void ember_provider::register_session(const std::shared_ptr<ember_session>& session)
{
    if (!session)
        return;

    std::lock_guard<std::mutex> lock(sessions_mutex_);

    auto it = sessions_.begin();
    while (it != sessions_.end()) {
        auto existing = it->lock();
        if (!existing) {
            it = sessions_.erase(it);
            continue;
        }

        if (existing.get() == session.get() || existing->client().get() == session->client().get())
            return;

        ++it;
    }

    sessions_.push_back(session);
}

std::vector<IO::client_connection<char>::ptr> ember_provider::active_clients() const
{
    std::vector<IO::client_connection<char>::ptr> clients;
    std::set<const void*>                         seen_clients;

    std::lock_guard<std::mutex> lock(sessions_mutex_);

    auto it = sessions_.begin();
    while (it != sessions_.end()) {
        auto session = it->lock();
        if (!session) {
            it = sessions_.erase(it);
            continue;
        }

        const auto& client = session->client();
        if (client.get() != nullptr && seen_clients.insert(client.get()).second)
            clients.push_back(client);

        ++it;
    }

    return clients;
}

long ember_provider::monitor_interval_ms() const
{
    std::lock_guard<std::mutex> lock(monitor_interval_mutex_);
    return static_cast<long>(monitor_interval_.count());
}

void ember_provider::set_monitor_interval(std::chrono::milliseconds interval)
{
    const auto normalized_interval = interval.count() > 0 ? interval : std::chrono::milliseconds(1000);
    {
        std::lock_guard<std::mutex> lock(monitor_interval_mutex_);
        monitor_interval_         = normalized_interval;
        monitor_interval_updated_ = true;
    }

    monitor_interval_cv_.notify_all();
}

void ember_provider::broadcast_monitor_interval_update(long interval_ms) const
{
    const auto path = std::vector<int>{runtime_root_number, runtime_state_update_interval_param_number};
    for (const auto& client : active_clients()) {
        send_qualified_parameter_update(client, path, [&](auto* p) {
            p->setValue(interval_ms);
            p->setMinimum(minimum_state_update_interval_ms);
            p->setMaximum(maximum_state_update_interval_ms);
        });
    }
}

void ember_provider::monitor_layer_changes()
{
    live_snapshot_map_t previous_snapshot;
    try {
        previous_snapshot = collect_live_snapshot(channels_);
    } catch (...) {
        CASPAR_LOG_CURRENT_EXCEPTION();
    }

    while (!stop_monitor_) {
        {
            std::unique_lock<std::mutex> lock(monitor_interval_mutex_);
            monitor_interval_updated_ = false;
            const auto wait_interval  = monitor_interval_;
            if (monitor_interval_cv_.wait_for(lock, wait_interval, [this] {
                    return stop_monitor_.load() || monitor_interval_updated_;
                })) {
                if (stop_monitor_)
                    break;

                continue;
            }
        }

        const auto clients = active_clients();
        if (clients.empty())
            continue;

        try {
            const auto current_snapshot = collect_live_snapshot(channels_);
            if (current_snapshot != previous_snapshot) {
                broadcast_directory_response();
                previous_snapshot = current_snapshot;
            }
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
        }
    }
}

void ember_provider::broadcast_directory_response() const
{
    const auto clients = active_clients();
    for (const auto& client : clients)
        send_directory_response(client);
}

void ember_provider::handle_request(libember::dom::Node* request_root, const std::shared_ptr<ember_session>& session)
{
    if (request_root == nullptr)
        return;

    register_session(session);

    bool sent_response = false;

    if (contains_directory_request(*request_root)) {
        send_directory_response(session->client());
        sent_response = true;
    }

    std::vector<function_invocation_request> invocation_requests;
    collect_requests(*request_root, {}, invocation_requests);

    for (const auto& request : invocation_requests) {
        auto result = command_bridge_.invoke(request.path, *request.invocation, session);
        send_invocation_result(session->client(), request.invocation->invocationId(), result);
        sent_response = true;
    }

    std::vector<parameter_write_request> parameter_writes;
    collect_parameter_writes(*request_root, {}, parameter_writes);
    for (const auto& write : parameter_writes)
        sent_response = handle_parameter_write(write.path, write.value, session) || sent_response;

    if (!sent_response)
        CASPAR_LOG(debug) << L"[ember] Ignoring unsupported Ember+ request from " << session->client()->address();
}

const std::shared_ptr<ember_registry>& ember_provider::registry() const { return registry_; }

void ember_provider::send_directory_response(const IO::client_connection<char>::ptr& client) const
{
    try {
        std::map<int, clip_command_state>    play_controls;
        std::map<int, clip_command_state>    loadbg_controls;
        std::map<int, clip_command_state>    load_controls;
        std::map<int, layer_command_state>   pause_controls;
        std::map<int, layer_command_state>   resume_controls;
        std::map<int, layer_command_state>   stop_controls;
        std::map<int, clear_command_state>   clear_controls;
        std::map<int, refresh_command_state> refresh_controls;
        std::map<int, call_command_state>    call_controls;
        std::map<int, call_command_state>    callbg_controls;
        std::vector<std::wstring>            media_clips;
        {
            std::lock_guard<std::mutex> lock(clip_commands_mutex_);
            play_controls = play_controls_;
            loadbg_controls = loadbg_controls_;
            load_controls = load_controls_;
            pause_controls = pause_controls_;
            resume_controls = resume_controls_;
            stop_controls = stop_controls_;
            clear_controls = clear_controls_;
            refresh_controls = refresh_controls_;
            call_controls = call_controls_;
            callbg_controls = callbg_controls_;
        }
        {
            std::lock_guard<std::mutex> lock(media_clips_mutex_);
            media_clips = media_clips_;
        }

        const auto live_snapshot = collect_live_snapshot(channels_);
        const auto state_update_interval_ms = monitor_interval_ms();

        client->send(build_directory_response(channels_,
                                              registry_,
                                              command_bridge_,
                                              state_update_interval_ms,
                                              live_snapshot,
                                              media_clips,
                                              play_controls,
                                              loadbg_controls,
                                              load_controls,
                                              pause_controls,
                                              resume_controls,
                                              stop_controls,
                                              clear_controls,
                                              refresh_controls,
                                              call_controls,
                                              callbg_controls),
                     true);
    } catch (...) {
        CASPAR_LOG_CURRENT_EXCEPTION();
    }
}

bool ember_provider::handle_live_layer_mixer_write(const std::vector<int>&              path,
                                                   const libember::glow::Value&         value,
                                                   const std::shared_ptr<ember_session>& session)
{
    if (path.size() != 6 || path[0] != channels_root_number || path[2] != layers_root_node_number ||
        path[4] != layer_mixer_node_number) {
        return false;
    }

    const auto channel_index = path[1];
    const auto layer_index   = path[3];
    const auto parameter     = path[5];
    const auto* channel      = find_channel_context(channels_, channel_index);
    if (channel == nullptr)
        return false;

    const auto current_layers = collect_live_layers(*channel);
    const auto current_layer_it = current_layers.find(layer_index);
    if (current_layer_it == current_layers.end())
        return false;

    const auto& state  = current_layer_it->second;
    const auto  target = std::to_wstring(channel_index) + L"-" + std::to_wstring(layer_index);

    std::wstring command;
    switch (parameter) {
        case layer_mixer_keyer_param_number:
            command = L"MIXER " + target + L" KEYER " + std::wstring(value_to_bool(value, state.keyer) ? L"1" : L"0");
            break;
        case layer_mixer_invert_param_number:
            command = L"MIXER " + target + L" INVERT " + std::wstring(value_to_bool(value, state.invert) ? L"1" : L"0");
            break;
        case layer_mixer_opacity_param_number:
        case layer_mixer_opacity_pct_param_number:
            command = L"MIXER " + target + L" OPACITY " +
                      std::to_wstring(parameter == layer_mixer_opacity_pct_param_number
                                          ? raw_value_from_percent_parameter(parameter, value)
                                          : clamp_mixer_parameter_value(
                                                layer_mixer_opacity_param_number, unscaled_mixer_value(value, state.opacity)));
            break;
        case layer_mixer_brightness_param_number:
        case layer_mixer_brightness_pct_param_number:
            command =
                L"MIXER " + target + L" BRIGHTNESS " +
                std::to_wstring(parameter == layer_mixer_brightness_pct_param_number
                                    ? raw_value_from_percent_parameter(parameter, value)
                                    : clamp_mixer_parameter_value(layer_mixer_brightness_param_number,
                                                                  unscaled_mixer_value(value, state.brightness)));
            break;
        case layer_mixer_saturation_param_number:
        case layer_mixer_saturation_pct_param_number:
            command =
                L"MIXER " + target + L" SATURATION " +
                std::to_wstring(parameter == layer_mixer_saturation_pct_param_number
                                    ? raw_value_from_percent_parameter(parameter, value)
                                    : clamp_mixer_parameter_value(layer_mixer_saturation_param_number,
                                                                  unscaled_mixer_value(value, state.saturation)));
            break;
        case layer_mixer_contrast_param_number:
        case layer_mixer_contrast_pct_param_number:
            command = L"MIXER " + target + L" CONTRAST " +
                      std::to_wstring(parameter == layer_mixer_contrast_pct_param_number
                                          ? raw_value_from_percent_parameter(parameter, value)
                                          : clamp_mixer_parameter_value(
                                                layer_mixer_contrast_param_number, unscaled_mixer_value(value, state.contrast)));
            break;
        case layer_mixer_rotation_param_number:
            command = L"MIXER " + target + L" ROTATION " +
                      std::to_wstring(
                          clamp_mixer_parameter_value(layer_mixer_rotation_param_number, unscaled_mixer_value(value, state.rotation)));
            break;
        case layer_mixer_volume_param_number:
        case layer_mixer_volume_pct_param_number:
            command = L"MIXER " + target + L" VOLUME " +
                      std::to_wstring(parameter == layer_mixer_volume_pct_param_number
                                          ? raw_value_from_percent_parameter(parameter, value)
                                          : clamp_mixer_parameter_value(
                                                layer_mixer_volume_param_number, unscaled_mixer_value(value, state.volume)));
            break;
        case layer_mixer_blend_param_number:
            command =
                L"MIXER " + target + L" BLEND " + blend_token_from_index(enum_index_from_value(value, mixer_blend_labels()));
            break;
        case layer_mixer_fill_x_param_number:
        case layer_mixer_fill_x_pct_param_number:
        case layer_mixer_fill_y_param_number:
        case layer_mixer_fill_y_pct_param_number:
        case layer_mixer_fill_w_param_number:
        case layer_mixer_fill_w_pct_param_number:
        case layer_mixer_fill_h_param_number:
        case layer_mixer_fill_h_pct_param_number: {
            auto fill_x = state.fill_x;
            auto fill_y = state.fill_y;
            auto fill_w = state.fill_w;
            auto fill_h = state.fill_h;

            if (parameter == layer_mixer_fill_x_param_number || parameter == layer_mixer_fill_x_pct_param_number)
                fill_x = parameter == layer_mixer_fill_x_pct_param_number
                             ? raw_value_from_percent_parameter(parameter, value)
                             : clamp_mixer_parameter_value(layer_mixer_fill_x_param_number, unscaled_mixer_value(value, fill_x));
            else if (parameter == layer_mixer_fill_y_param_number || parameter == layer_mixer_fill_y_pct_param_number)
                fill_y = parameter == layer_mixer_fill_y_pct_param_number
                             ? raw_value_from_percent_parameter(parameter, value)
                             : clamp_mixer_parameter_value(layer_mixer_fill_y_param_number, unscaled_mixer_value(value, fill_y));
            else if (parameter == layer_mixer_fill_w_param_number || parameter == layer_mixer_fill_w_pct_param_number)
                fill_w = parameter == layer_mixer_fill_w_pct_param_number
                             ? raw_value_from_percent_parameter(parameter, value)
                             : clamp_mixer_parameter_value(layer_mixer_fill_w_param_number, unscaled_mixer_value(value, fill_w));
            else
                fill_h = parameter == layer_mixer_fill_h_pct_param_number
                             ? raw_value_from_percent_parameter(parameter, value)
                             : clamp_mixer_parameter_value(layer_mixer_fill_h_param_number, unscaled_mixer_value(value, fill_h));

            command = L"MIXER " + target + L" FILL " + std::to_wstring(fill_x) + L" " + std::to_wstring(fill_y) + L" " +
                      std::to_wstring(fill_w) + L" " + std::to_wstring(fill_h);
            break;
        }
        case layer_mixer_clip_x_param_number:
        case layer_mixer_clip_x_pct_param_number:
        case layer_mixer_clip_y_param_number:
        case layer_mixer_clip_y_pct_param_number:
        case layer_mixer_clip_w_param_number:
        case layer_mixer_clip_w_pct_param_number:
        case layer_mixer_clip_h_param_number:
        case layer_mixer_clip_h_pct_param_number: {
            auto clip_x = state.clip_x;
            auto clip_y = state.clip_y;
            auto clip_w = state.clip_w;
            auto clip_h = state.clip_h;

            if (parameter == layer_mixer_clip_x_param_number || parameter == layer_mixer_clip_x_pct_param_number)
                clip_x = parameter == layer_mixer_clip_x_pct_param_number
                             ? raw_value_from_percent_parameter(parameter, value)
                             : clamp_mixer_parameter_value(layer_mixer_clip_x_param_number, unscaled_mixer_value(value, clip_x));
            else if (parameter == layer_mixer_clip_y_param_number || parameter == layer_mixer_clip_y_pct_param_number)
                clip_y = parameter == layer_mixer_clip_y_pct_param_number
                             ? raw_value_from_percent_parameter(parameter, value)
                             : clamp_mixer_parameter_value(layer_mixer_clip_y_param_number, unscaled_mixer_value(value, clip_y));
            else if (parameter == layer_mixer_clip_w_param_number || parameter == layer_mixer_clip_w_pct_param_number)
                clip_w = parameter == layer_mixer_clip_w_pct_param_number
                             ? raw_value_from_percent_parameter(parameter, value)
                             : clamp_mixer_parameter_value(layer_mixer_clip_w_param_number, unscaled_mixer_value(value, clip_w));
            else
                clip_h = parameter == layer_mixer_clip_h_pct_param_number
                             ? raw_value_from_percent_parameter(parameter, value)
                             : clamp_mixer_parameter_value(layer_mixer_clip_h_param_number, unscaled_mixer_value(value, clip_h));

            command = L"MIXER " + target + L" CLIP " + std::to_wstring(clip_x) + L" " + std::to_wstring(clip_y) + L" " +
                      std::to_wstring(clip_w) + L" " + std::to_wstring(clip_h);
            break;
        }
        case layer_mixer_anchor_x_param_number:
        case layer_mixer_anchor_x_pct_param_number:
        case layer_mixer_anchor_y_param_number:
        case layer_mixer_anchor_y_pct_param_number: {
            auto anchor_x = state.anchor_x;
            auto anchor_y = state.anchor_y;

            if (parameter == layer_mixer_anchor_x_param_number || parameter == layer_mixer_anchor_x_pct_param_number)
                anchor_x = parameter == layer_mixer_anchor_x_pct_param_number
                               ? raw_value_from_percent_parameter(parameter, value)
                               : clamp_mixer_parameter_value(layer_mixer_anchor_x_param_number,
                                                             unscaled_mixer_value(value, anchor_x));
            else
                anchor_y = parameter == layer_mixer_anchor_y_pct_param_number
                               ? raw_value_from_percent_parameter(parameter, value)
                               : clamp_mixer_parameter_value(layer_mixer_anchor_y_param_number,
                                                             unscaled_mixer_value(value, anchor_y));

            command = L"MIXER " + target + L" ANCHOR " + std::to_wstring(anchor_x) + L" " + std::to_wstring(anchor_y);
            break;
        }
        case layer_mixer_crop_l_param_number:
        case layer_mixer_crop_l_pct_param_number:
        case layer_mixer_crop_t_param_number:
        case layer_mixer_crop_t_pct_param_number:
        case layer_mixer_crop_r_param_number:
        case layer_mixer_crop_r_pct_param_number:
        case layer_mixer_crop_b_param_number:
        case layer_mixer_crop_b_pct_param_number: {
            auto crop_l = state.crop_l;
            auto crop_t = state.crop_t;
            auto crop_r = state.crop_r;
            auto crop_b = state.crop_b;

            if (parameter == layer_mixer_crop_l_param_number || parameter == layer_mixer_crop_l_pct_param_number)
                crop_l = parameter == layer_mixer_crop_l_pct_param_number
                             ? raw_value_from_percent_parameter(parameter, value)
                             : clamp_mixer_parameter_value(layer_mixer_crop_l_param_number, unscaled_mixer_value(value, crop_l));
            else if (parameter == layer_mixer_crop_t_param_number || parameter == layer_mixer_crop_t_pct_param_number)
                crop_t = parameter == layer_mixer_crop_t_pct_param_number
                             ? raw_value_from_percent_parameter(parameter, value)
                             : clamp_mixer_parameter_value(layer_mixer_crop_t_param_number, unscaled_mixer_value(value, crop_t));
            else if (parameter == layer_mixer_crop_r_param_number || parameter == layer_mixer_crop_r_pct_param_number)
                crop_r = parameter == layer_mixer_crop_r_pct_param_number
                             ? raw_value_from_percent_parameter(parameter, value)
                             : clamp_mixer_parameter_value(layer_mixer_crop_r_param_number, unscaled_mixer_value(value, crop_r));
            else
                crop_b = parameter == layer_mixer_crop_b_pct_param_number
                             ? raw_value_from_percent_parameter(parameter, value)
                             : clamp_mixer_parameter_value(layer_mixer_crop_b_param_number, unscaled_mixer_value(value, crop_b));

            command = L"MIXER " + target + L" CROP " + std::to_wstring(crop_l) + L" " + std::to_wstring(crop_t) + L" " +
                      std::to_wstring(crop_r) + L" " + std::to_wstring(crop_b);
            break;
        }
        default:
            return false;
    }

    const auto result = command_bridge_.execute_native_command_line(command, session);
    if (!result.success) {
        CASPAR_LOG(warning) << L"[ember] Mixer write failed for " << command << L": " << result.message;
        send_live_layer_snapshot(session->client(), channel_index, state);
        return true;
    }

    const auto updated_layers = collect_live_layers(*channel);
    const auto updated_layer_it = updated_layers.find(layer_index);
    if (updated_layer_it == updated_layers.end())
        return true;

    const auto clients = active_clients();
    for (const auto& client : clients)
        send_live_layer_snapshot(client, channel_index, updated_layer_it->second);

    return true;
}

bool ember_provider::handle_parameter_write(const std::vector<int>&              path,
                                            const libember::glow::Value&         value,
                                            const std::shared_ptr<ember_session>& session)
{
    if (handle_live_layer_mixer_write(path, value, session))
        return true;

    if (path.size() == 2 && path[0] == runtime_root_number && path[1] == runtime_state_update_interval_param_number) {
        const auto current_interval_ms = monitor_interval_ms();
        const auto requested_interval_ms =
            clamp_state_update_interval_ms(interval_ms_from_value(value, current_interval_ms));

        std::wstring error_message;
        {
            std::lock_guard<std::mutex> lock(configuration_mutex_);
            if (!persist_state_update_interval_to_config(requested_interval_ms, error_message)) {
                CASPAR_LOG(error) << L"[ember] Failed to persist Ember+ state update interval to "
                                  << env::configuration_file() << L": " << error_message;

                send_qualified_parameter_update(session->client(),
                                                path,
                                                [&](auto* p) { p->setValue(current_interval_ms); });
                return true;
            }
        }

        set_monitor_interval(std::chrono::milliseconds(requested_interval_ms));
        broadcast_monitor_interval_update(requested_interval_ms);
        CASPAR_LOG(info) << L"[ember] Updated Ember+ state update interval to " << requested_interval_ms
                         << L" ms and persisted it to " << env::configuration_file();
        return true;
    }

    if (path.size() < 4 || path[0] != channels_root_number)
        return false;

    const auto channel_index = path[1];
    std::vector<std::wstring> media_clips;
    {
        std::lock_guard<std::mutex> lock(media_clips_mutex_);
        media_clips = media_clips_;
    }
    const auto clip_count = static_cast<long>(media_clips.size() > 0 ? media_clips.size() - 1 : 0);

    const auto send_clip_value_update =
        [&](int target_node, const clip_command_state& state, int target_parameter, const std::vector<std::wstring>& clips) {
            const auto path = std::vector<int>{channels_root_number, channel_index, target_node, target_parameter};

            switch (target_parameter) {
                case clip_layer_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(static_cast<long>(state.layer)); });
                    return;
                case clip_loop_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(state.loop); });
                    return;
                case clip_name_param_number: {
                    const auto clip_labels = play_clip_labels(clips);
                    send_qualified_parameter_update(session->client(), path, [&](auto* p) {
                        p->setEnumeration(clip_labels.begin(), clip_labels.end());
                        p->setMinimum(0L);
                        p->setMaximum(static_cast<long>(clip_labels.size() - 1));
                        p->setValue(find_clip_index(state.file_name, clips));
                    });
                    return;
                }
                case clip_seek_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(static_cast<long>(state.seek)); });
                    return;
                case clip_length_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(static_cast<long>(state.length)); });
                    return;
                case clip_filter_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(u8(state.filter)); });
                    return;
                case clip_clear404_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(state.clear_on_404); });
                    return;
                case clip_auto_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(state.auto_play); });
                    return;
                case clip_transition_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(static_cast<long>(state.transition)); });
                    return;
                case clip_duration_param_number:
                    send_qualified_parameter_update(session->client(),
                                                    path,
                                                    [&](auto* p) { p->setValue(static_cast<long>(state.transition_duration)); });
                    return;
                case clip_tween_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(u8(state.tween)); });
                    return;
                case clip_direction_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(static_cast<long>(state.direction)); });
                    return;
                case clip_execute_param_number:
                    send_qualified_parameter_update(session->client(), path, [&](auto* p) { p->setValue(false); });
                    return;
                case clip_last_reply_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(u8(state.last_reply)); });
                    return;
                case clip_last_success_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(state.last_success); });
                    return;
                default:
                    return;
            }
        };

    const auto send_sting_value_update =
        [&](const clip_command_state& state, int target_parameter, const std::vector<std::wstring>& clips) {
            const auto path = std::vector<int>{
                channels_root_number, channel_index, loadbg_node_number, clip_sting_node_number, target_parameter};

            switch (target_parameter) {
                case sting_enabled_param_number:
                    send_qualified_parameter_update(session->client(), path, [&](auto* p) { p->setValue(state.sting_enabled); });
                    return;
                case sting_mask_param_number: {
                    const auto clip_labels = play_clip_labels(clips);
                    send_qualified_parameter_update(session->client(), path, [&](auto* p) {
                        p->setEnumeration(clip_labels.begin(), clip_labels.end());
                        p->setMinimum(0L);
                        p->setMaximum(static_cast<long>(clip_labels.size() - 1));
                        p->setValue(find_clip_index(state.sting_mask, clips));
                    });
                    return;
                }
                case sting_trigger_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(static_cast<long>(state.sting_trigger_point)); });
                    return;
                case sting_overlay_param_number: {
                    const auto clip_labels = play_clip_labels(clips);
                    send_qualified_parameter_update(session->client(), path, [&](auto* p) {
                        p->setEnumeration(clip_labels.begin(), clip_labels.end());
                        p->setMinimum(0L);
                        p->setMaximum(static_cast<long>(clip_labels.size() - 1));
                        p->setValue(find_clip_index(state.sting_overlay, clips));
                    });
                    return;
                }
                case sting_audio_fade_start_param_number:
                    send_qualified_parameter_update(session->client(),
                                                    path,
                                                    [&](auto* p) { p->setValue(static_cast<long>(state.sting_audio_fade_start)); });
                    return;
                case sting_audio_fade_duration_param_number:
                    send_qualified_parameter_update(
                        session->client(),
                        path,
                        [&](auto* p) { p->setValue(static_cast<long>(state.sting_audio_fade_duration)); });
                    return;
                default:
                    return;
            }
        };

    const auto send_load_value_update =
        [&](const clip_command_state& state, int target_parameter, const std::vector<std::wstring>& clips) {
            const auto path = std::vector<int>{channels_root_number, channel_index, load_node_number, target_parameter};

            switch (target_parameter) {
                case load_layer_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(static_cast<long>(state.layer)); });
                    return;
                case load_name_param_number: {
                    const auto clip_labels = play_clip_labels(clips);
                    send_qualified_parameter_update(session->client(), path, [&](auto* p) {
                        p->setEnumeration(clip_labels.begin(), clip_labels.end());
                        p->setMinimum(0L);
                        p->setMaximum(static_cast<long>(clip_labels.size() - 1));
                        p->setValue(find_clip_index(state.file_name, clips));
                    });
                    return;
                }
                case load_loop_param_number:
                    send_qualified_parameter_update(session->client(), path, [&](auto* p) { p->setValue(state.loop); });
                    return;
                case load_seek_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(static_cast<long>(state.seek)); });
                    return;
                case load_length_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(static_cast<long>(state.length)); });
                    return;
                case load_filter_param_number:
                    send_qualified_parameter_update(session->client(), path, [&](auto* p) { p->setValue(u8(state.filter)); });
                    return;
                case load_clear404_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(state.clear_on_404); });
                    return;
                case load_execute_param_number:
                    send_qualified_parameter_update(session->client(), path, [&](auto* p) { p->setValue(false); });
                    return;
                case load_last_reply_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(u8(state.last_reply)); });
                    return;
                case load_last_success_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(state.last_success); });
                    return;
                default:
                    return;
            }
        };

    const auto send_action_value_update =
        [&](int target_node, const layer_command_state& state, int target_parameter) {
            const auto path = std::vector<int>{channels_root_number, channel_index, target_node, target_parameter};
            switch (target_parameter) {
                case action_layer_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(static_cast<long>(state.layer)); });
                    return;
                case action_execute_param_number:
                    send_qualified_parameter_update(session->client(), path, [&](auto* p) { p->setValue(false); });
                    return;
                case action_last_reply_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(u8(state.last_reply)); });
                    return;
                case action_last_success_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(state.last_success); });
                    return;
                default:
                    return;
            }
        };

    const auto send_call_value_update =
        [&](int target_node, const call_command_state& state, int target_parameter) {
            const auto path = std::vector<int>{channels_root_number, channel_index, target_node, target_parameter};
            switch (target_parameter) {
                case call_layer_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(static_cast<long>(state.layer)); });
                    return;
                case call_arguments_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(u8(state.arguments)); });
                    return;
                case call_execute_param_number:
                    send_qualified_parameter_update(session->client(), path, [&](auto* p) { p->setValue(false); });
                    return;
                case call_last_reply_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(u8(state.last_reply)); });
                    return;
                case call_last_success_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(state.last_success); });
                    return;
                default:
                    return;
            }
        };

    const auto send_clear_value_update =
        [&](const clear_command_state& state, int target_parameter) {
            const auto path = std::vector<int>{channels_root_number, channel_index, clear_node_number, target_parameter};
            switch (target_parameter) {
                case action_layer_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(static_cast<long>(state.layer)); });
                    return;
                case action_execute_param_number:
                    send_qualified_parameter_update(session->client(), path, [&](auto* p) { p->setValue(false); });
                    return;
                case action_last_reply_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(u8(state.last_reply)); });
                    return;
                case action_last_success_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(state.last_success); });
                    return;
                default:
                    return;
            }
        };

    const auto send_refresh_value_update =
        [&](const refresh_command_state& state, int target_parameter, long count) {
            const auto path = std::vector<int>{channels_root_number, channel_index, refresh_node_number, target_parameter};
            switch (target_parameter) {
                case refresh_execute_param_number:
                    send_qualified_parameter_update(session->client(), path, [&](auto* p) { p->setValue(false); });
                    return;
                case refresh_count_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(count); });
                    return;
                case refresh_last_reply_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(u8(state.last_reply)); });
                    return;
                case refresh_last_success_param_number:
                    send_qualified_parameter_update(
                        session->client(), path, [&](auto* p) { p->setValue(state.last_success); });
                    return;
                default:
                    return;
            }
        };

    if (path.size() == 5) {
        if (path[2] != loadbg_node_number || path[3] != clip_sting_node_number)
            return false;

        const auto parameter = path[4];
        clip_command_state state_snapshot;
        {
            std::lock_guard<std::mutex> lock(clip_commands_mutex_);
            auto                        it = loadbg_controls_.find(channel_index);
            if (it == loadbg_controls_.end())
                return false;

            switch (parameter) {
                case sting_enabled_param_number:
                    it->second.sting_enabled = value_to_bool(value, it->second.sting_enabled);
                    break;
                case sting_mask_param_number:
                    it->second.sting_mask = clip_from_value(value, media_clips);
                    break;
                case sting_trigger_param_number:
                    it->second.sting_trigger_point = std::max(0L, value.toInteger());
                    break;
                case sting_overlay_param_number:
                    it->second.sting_overlay = clip_from_value(value, media_clips);
                    break;
                case sting_audio_fade_start_param_number:
                    it->second.sting_audio_fade_start = std::max(0L, value.toInteger());
                    break;
                case sting_audio_fade_duration_param_number:
                    it->second.sting_audio_fade_duration = std::max(0L, value.toInteger());
                    break;
                default:
                    return false;
            }

            state_snapshot = it->second;
        }

        send_sting_value_update(state_snapshot, parameter, media_clips);
        return true;
    }

    if (path.size() != 4)
        return false;

    const auto node_number = path[2];
    const auto parameter   = path[3];

    if (node_number == play_node_number || node_number == loadbg_node_number) {
        clip_command_state state_snapshot;
        {
            std::lock_guard<std::mutex> lock(clip_commands_mutex_);
            auto&                       controls = node_number == play_node_number ? play_controls_ : loadbg_controls_;
            auto                        it       = controls.find(channel_index);
            if (it == controls.end())
                return false;

            switch (parameter) {
                case clip_layer_param_number:
                    it->second.layer = std::max(0L, value.toInteger());
                    break;
                case clip_loop_param_number:
                    it->second.loop = value.toBoolean();
                    break;
                case clip_name_param_number:
                    it->second.file_name = clip_from_value(value, media_clips);
                    break;
                case clip_seek_param_number:
                    it->second.seek = std::max(0L, value.toInteger());
                    break;
                case clip_length_param_number:
                    it->second.length = std::max(0L, value.toInteger());
                    break;
                case clip_filter_param_number:
                    it->second.filter = u16(value.toString());
                    break;
                case clip_clear404_param_number:
                    it->second.clear_on_404 = value.toBoolean();
                    break;
                case clip_auto_param_number:
                    if (node_number != loadbg_node_number)
                        return false;
                    it->second.auto_play = value_to_bool(value, it->second.auto_play);
                    break;
                case clip_transition_param_number:
                    it->second.transition = static_cast<int>(enum_index_from_value(value, play_transition_labels()));
                    break;
                case clip_duration_param_number:
                    it->second.transition_duration = std::max(0L, value.toInteger());
                    break;
                case clip_tween_param_number:
                    it->second.tween = u16(value.toString());
                    break;
                case clip_direction_param_number:
                    it->second.direction = static_cast<int>(enum_index_from_value(value, play_direction_labels()));
                    break;
                case clip_execute_param_number:
                case clip_last_reply_param_number:
                case clip_last_success_param_number:
                    break;
                default:
                    return false;
            }

            state_snapshot = it->second;
        }

        if (parameter != clip_execute_param_number) {
            send_clip_value_update(node_number, state_snapshot, parameter, media_clips);
            return true;
        }

        if (!is_execute_request(value)) {
            send_clip_value_update(node_number, state_snapshot, clip_execute_param_number, media_clips);
            return true;
        }

        ember_command_bridge::invocation_result result;
        if (node_number == loadbg_node_number && state_snapshot.sting_enabled && state_snapshot.sting_mask.empty()) {
            result = {false, L"400 LOADBG STING MASK REQUIRED\r\n"};
        } else {
            result = command_bridge_.execute_native_command_line(node_number == play_node_number
                                                                     ? compose_clip_command(L"PLAY", channel_index, state_snapshot)
                                                                     : compose_loadbg_command(channel_index, state_snapshot),
                                                                 session);
        }

        {
            std::lock_guard<std::mutex> lock(clip_commands_mutex_);
            auto&                       controls = node_number == play_node_number ? play_controls_ : loadbg_controls_;
            auto&                       state    = controls.at(channel_index);
            state.last_reply                     = result.message;
            state.last_success                   = result.success;
            state_snapshot                       = state;
        }

        send_clip_value_update(node_number, state_snapshot, clip_execute_param_number, media_clips);
        send_clip_value_update(node_number, state_snapshot, clip_last_reply_param_number, media_clips);
        send_clip_value_update(node_number, state_snapshot, clip_last_success_param_number, media_clips);
        return true;
    }

    if (node_number == load_node_number) {
        clip_command_state state_snapshot;
        {
            std::lock_guard<std::mutex> lock(clip_commands_mutex_);
            auto                        it = load_controls_.find(channel_index);
            if (it == load_controls_.end())
                return false;

            switch (parameter) {
                case load_layer_param_number:
                    it->second.layer = std::max(0L, value.toInteger());
                    break;
                case load_name_param_number:
                    it->second.file_name = clip_from_value(value, media_clips);
                    break;
                case load_loop_param_number:
                    it->second.loop = value_to_bool(value, it->second.loop);
                    break;
                case load_seek_param_number:
                    it->second.seek = std::max(0L, value.toInteger());
                    break;
                case load_length_param_number:
                    it->second.length = std::max(0L, value.toInteger());
                    break;
                case load_filter_param_number:
                    it->second.filter = u16(value.toString());
                    break;
                case load_clear404_param_number:
                    it->second.clear_on_404 = value_to_bool(value, it->second.clear_on_404);
                    break;
                case load_execute_param_number:
                case load_last_reply_param_number:
                case load_last_success_param_number:
                    break;
                default:
                    return false;
            }

            state_snapshot = it->second;
        }

        if (parameter != load_execute_param_number) {
            send_load_value_update(state_snapshot, parameter, media_clips);
            return true;
        }

        if (!is_execute_request(value)) {
            send_load_value_update(state_snapshot, load_execute_param_number, media_clips);
            return true;
        }

        const auto result =
            command_bridge_.execute_native_command_line(compose_load_command(channel_index, state_snapshot), session);

        {
            std::lock_guard<std::mutex> lock(clip_commands_mutex_);
            auto&                       state = load_controls_.at(channel_index);
            state.last_reply                   = result.message;
            state.last_success                 = result.success;
            state_snapshot                     = state;
        }

        send_load_value_update(state_snapshot, load_execute_param_number, media_clips);
        send_load_value_update(state_snapshot, load_last_reply_param_number, media_clips);
        send_load_value_update(state_snapshot, load_last_success_param_number, media_clips);
        return true;
    }

    if (node_number == pause_node_number || node_number == resume_node_number || node_number == stop_node_number) {
        layer_command_state state_snapshot;
        std::map<int, layer_command_state>* controls = nullptr;
        std::wstring                        command_name;
        if (node_number == pause_node_number) {
            controls = &pause_controls_;
            command_name = L"PAUSE";
        } else if (node_number == resume_node_number) {
            controls = &resume_controls_;
            command_name = L"RESUME";
        } else {
            controls = &stop_controls_;
            command_name = L"STOP";
        }

        {
            std::lock_guard<std::mutex> lock(clip_commands_mutex_);
            auto                        it = controls->find(channel_index);
            if (it == controls->end())
                return false;

            switch (parameter) {
                case action_layer_param_number:
                    it->second.layer = std::max(0L, value.toInteger());
                    break;
                case action_execute_param_number:
                case action_last_reply_param_number:
                case action_last_success_param_number:
                    break;
                default:
                    return false;
            }

            state_snapshot = it->second;
        }

        if (parameter != action_execute_param_number) {
            send_action_value_update(node_number, state_snapshot, parameter);
            return true;
        }

        if (!is_execute_request(value)) {
            send_action_value_update(node_number, state_snapshot, action_execute_param_number);
            return true;
        }

        const auto result = command_bridge_.execute_native_command_line(
            compose_layer_action_command(command_name, channel_index, state_snapshot.layer), session);

        {
            std::lock_guard<std::mutex> lock(clip_commands_mutex_);
            auto&                       state = controls->at(channel_index);
            state.last_reply                   = result.message;
            state.last_success                 = result.success;
            state_snapshot                     = state;
        }

        send_action_value_update(node_number, state_snapshot, action_execute_param_number);
        send_action_value_update(node_number, state_snapshot, action_last_reply_param_number);
        send_action_value_update(node_number, state_snapshot, action_last_success_param_number);
        return true;
    }

    if (node_number == call_node_number || node_number == callbg_node_number) {
        call_command_state state_snapshot;
        std::map<int, call_command_state>* controls = nullptr;
        std::wstring                       command_name;
        if (node_number == call_node_number) {
            controls = &call_controls_;
            command_name = L"CALL";
        } else {
            controls = &callbg_controls_;
            command_name = L"CALLBG";
        }

        {
            std::lock_guard<std::mutex> lock(clip_commands_mutex_);
            auto                        it = controls->find(channel_index);
            if (it == controls->end())
                return false;

            switch (parameter) {
                case call_layer_param_number:
                    it->second.layer = std::max(0L, value.toInteger());
                    break;
                case call_arguments_param_number:
                    it->second.arguments = u16(value.toString());
                    break;
                case call_execute_param_number:
                case call_last_reply_param_number:
                case call_last_success_param_number:
                    break;
                default:
                    return false;
            }

            state_snapshot = it->second;
        }

        if (parameter != call_execute_param_number) {
            send_call_value_update(node_number, state_snapshot, parameter);
            return true;
        }

        if (!is_execute_request(value)) {
            send_call_value_update(node_number, state_snapshot, call_execute_param_number);
            return true;
        }

        const auto result =
            command_bridge_.execute_native_command_line(compose_call_command(command_name, channel_index, state_snapshot),
                                                        session);

        {
            std::lock_guard<std::mutex> lock(clip_commands_mutex_);
            auto&                       state = controls->at(channel_index);
            state.last_reply                   = result.message;
            state.last_success                 = result.success;
            state_snapshot                     = state;
        }

        send_call_value_update(node_number, state_snapshot, call_execute_param_number);
        send_call_value_update(node_number, state_snapshot, call_last_reply_param_number);
        send_call_value_update(node_number, state_snapshot, call_last_success_param_number);
        return true;
    }

    if (node_number == clear_node_number) {
        clear_command_state state_snapshot;
        {
            std::lock_guard<std::mutex> lock(clip_commands_mutex_);
            auto                        it = clear_controls_.find(channel_index);
            if (it == clear_controls_.end())
                return false;

            switch (parameter) {
                case action_layer_param_number:
                    it->second.layer = std::max(0L, value.toInteger());
                    break;
                case action_execute_param_number:
                case action_last_reply_param_number:
                case action_last_success_param_number:
                    break;
                default:
                    return false;
            }

            state_snapshot = it->second;
        }

        if (parameter != action_execute_param_number) {
            send_clear_value_update(state_snapshot, parameter);
            return true;
        }

        if (!is_execute_request(value)) {
            send_clear_value_update(state_snapshot, action_execute_param_number);
            return true;
        }

        const auto result =
            command_bridge_.execute_native_command_line(compose_clear_command(channel_index, state_snapshot.layer), session);

        {
            std::lock_guard<std::mutex> lock(clip_commands_mutex_);
            auto&                       state = clear_controls_.at(channel_index);
            state.last_reply                   = result.message;
            state.last_success                 = result.success;
            state_snapshot                     = state;
        }

        send_clear_value_update(state_snapshot, action_execute_param_number);
        send_clear_value_update(state_snapshot, action_last_reply_param_number);
        send_clear_value_update(state_snapshot, action_last_success_param_number);
        return true;
    }

    if (node_number == refresh_node_number) {
        refresh_command_state refresh_snapshot;
        clip_command_state    play_snapshot;
        clip_command_state    loadbg_snapshot;
        clip_command_state    load_snapshot;

        {
            std::lock_guard<std::mutex> lock(clip_commands_mutex_);
            auto                        it = refresh_controls_.find(channel_index);
            if (it == refresh_controls_.end())
                return false;
            refresh_snapshot = it->second;
        }

        if (parameter == refresh_count_param_number) {
            send_refresh_value_update(refresh_snapshot, refresh_count_param_number, clip_count);
            return true;
        }

        if (parameter == refresh_last_reply_param_number || parameter == refresh_last_success_param_number) {
            send_refresh_value_update(refresh_snapshot, parameter, clip_count);
            return true;
        }

        if (parameter != refresh_execute_param_number)
            return false;

        if (!is_execute_request(value)) {
            send_refresh_value_update(refresh_snapshot, refresh_execute_param_number, clip_count);
            return true;
        }

        media_clips = available_play_clips();
        const auto refreshed_clip_count = static_cast<long>(media_clips.size() > 0 ? media_clips.size() - 1 : 0);
        {
            std::lock_guard<std::mutex> lock(media_clips_mutex_);
            media_clips_ = media_clips;
        }
        {
            std::lock_guard<std::mutex> lock(clip_commands_mutex_);
            auto&                       refresh_state = refresh_controls_.at(channel_index);
            refresh_state.last_reply                 = L"202 REFRESH OK\r\n";
            refresh_state.last_success               = true;
            refresh_snapshot                         = refresh_state;
            play_snapshot                            = play_controls_.at(channel_index);
            loadbg_snapshot                          = loadbg_controls_.at(channel_index);
            load_snapshot                            = load_controls_.at(channel_index);
        }

        send_refresh_value_update(refresh_snapshot, refresh_execute_param_number, refreshed_clip_count);
        send_refresh_value_update(refresh_snapshot, refresh_count_param_number, refreshed_clip_count);
        send_refresh_value_update(refresh_snapshot, refresh_last_reply_param_number, refreshed_clip_count);
        send_refresh_value_update(refresh_snapshot, refresh_last_success_param_number, refreshed_clip_count);
        send_clip_value_update(play_node_number, play_snapshot, clip_name_param_number, media_clips);
        send_clip_value_update(loadbg_node_number, loadbg_snapshot, clip_name_param_number, media_clips);
        send_sting_value_update(loadbg_snapshot, sting_mask_param_number, media_clips);
        send_sting_value_update(loadbg_snapshot, sting_overlay_param_number, media_clips);
        send_load_value_update(load_snapshot, load_name_param_number, media_clips);
        return true;
    }

    return false;
}

}}} // namespace caspar::protocol::ember
