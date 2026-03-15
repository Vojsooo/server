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

#include "ember_registry.h"

namespace caspar { namespace protocol { namespace ember {

void ember_registry::register_module_root(module_root root)
{
    std::lock_guard<std::mutex> lock(mutex_);
    module_roots_.push_back(std::move(root));
}

std::vector<ember_registry::module_root> ember_registry::module_roots() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return module_roots_;
}

}}} // namespace caspar::protocol::ember
