
#pragma once

#include <atom/integer.hpp>

namespace dual::nds {

  struct Header {
    u8 game_title[12];
    u8 game_code[4];
    u8 maker_code[2];
    u8 unit_code;
    u8 encryption_seed_select;
    u8 device_capacity;
    u8 reserved0[8];
    u8 nds_region;
    u8 version;
    u8 autostart;

    struct Binary {
      u32 file_address;
      u32 entrypoint;
      u32 load_address;
      u32 size;
    } arm9, arm7;
  } __attribute__((packed));

} // namespace dual::nds
