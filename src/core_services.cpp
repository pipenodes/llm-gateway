#include "core_services.h"

namespace core {

EnqueueResult NullRequestQueue::enqueue(Priority, std::chrono::milliseconds,
    std::function<void()>, const std::string&, const std::string&) {
    std::promise<bool> p;
    p.set_value(false);
    return {false, 0, p.get_future()};
}

} // namespace core
