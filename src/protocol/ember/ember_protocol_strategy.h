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

#include "../util/protocol_strategy.h"

#include <memory>
#include <string>

namespace caspar { namespace protocol { namespace ember {

class ember_provider;

IO::protocol_strategy_factory<char>::ptr create_ember_plus_strategy_factory(const std::wstring&                  name,
                                                                            const std::shared_ptr<ember_provider>& provider);

}}} // namespace caspar::protocol::ember
