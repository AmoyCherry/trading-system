#pragma once

#include "protocol/messages.hpp"
#include <variant>

namespace ts::proto {

// What a client sends "into the stack"
using ClientMsg = std::variant<NewOrder, Cancel>;

} // namespace ts::proto
