
#pragma once

#include <fmt/format.h>

namespace dual::nds {

  enum class CPU {
    ARM7 = 0,
    ARM9 = 1
  };

  constexpr CPU operator~(CPU cpu) {
    return (CPU)((int)cpu ^ 1);
  }

} // namespace dual::nds

template<> struct fmt::formatter<dual::nds::CPU> : formatter<std::string_view> {
  template<typename FormatContext>
  auto format(dual::nds::CPU cpu, FormatContext& ctx) const {
    std::string_view name = "(unknown)";

    switch(cpu) {
      case dual::nds::CPU::ARM7: name = "arm7"; break;
      case dual::nds::CPU::ARM9: name = "arm9"; break;
    }

    return formatter<std::string_view>::format(name, ctx);
  }
};