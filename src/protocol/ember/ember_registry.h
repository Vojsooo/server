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

#include <mutex>
#include <string>
#include <vector>

namespace caspar { namespace protocol { namespace ember {

class ember_registry final
{
  public:
    struct module_root
    {
        int         number      = 0;
        std::string identifier;
        std::string description;
    };

    void register_module_root(module_root root);
    std::vector<module_root> module_roots() const;

  private:
    mutable std::mutex          mutex_;
    std::vector<module_root> module_roots_;
};

}}} // namespace caspar::protocol::ember
