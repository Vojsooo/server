#pragma once

#include <mutex>

namespace caspar { namespace protocol { namespace ember {

struct system_snapshot
{
    double process_cpu_pct          = 0.0;
    double system_cpu_pct           = 0.0;
    double process_resident_mb      = 0.0;
    double system_total_memory_mb   = 0.0;
    double system_available_memory_mb = 0.0;
    double system_used_memory_pct   = 0.0;
};

class ember_system_info_sampler final
{
  public:
    ember_system_info_sampler();

    system_snapshot sample() const;

  private:
    mutable std::mutex mutex_;
    mutable bool       initialized_ = false;

#ifdef _WIN32
    mutable unsigned long long previous_system_idle_   = 0;
    mutable unsigned long long previous_system_kernel_ = 0;
    mutable unsigned long long previous_system_user_   = 0;
    mutable unsigned long long previous_process_total_ = 0;
#else
    mutable unsigned long long previous_system_total_  = 0;
    mutable unsigned long long previous_system_busy_   = 0;
    mutable unsigned long long previous_process_total_ = 0;
    mutable long               page_size_              = 0;
#endif
};

}}} // namespace caspar::protocol::ember
