
#include <atom/logger/logger.hpp>
#include <atom/panic.hpp>
#include <dual/nds/arm9/cp15.hpp>

#define ID(opc1, cn, cm, opc2) ((opc1) << 11 | (cn) << 7 | (cm) << 3 | (opc2))

namespace dual::nds::arm9 {

  CP15::CP15(arm::CPU* cpu, arm::Memory* bus) : m_cpu{cpu}, m_bus{bus} {
  }

  void CP15::Reset() {
    // @todo: refer to ARM9 manual to figure out correct initialization values
  }

  void CP15::DirectBoot() {
    // Reset control register (enable DTCM and ITCM, exception base = 0xFFFF0000)
    MCR(0, 1, 0, 0, 0x0005707D);

    // Reset DTCM and ITCM configuration
    MCR(0, 9, 1, 0, 0x0300000A);
    MCR(0, 9, 1, 1, 0x00000020);
  }

  u32 CP15::MRC(int opc1, int cn, int cm, int opc2) {
    switch(ID(opc1, cn, cm, opc2)) {
      case ID(0, 0, 0, 0): { // Main ID
        return 0x41059461u;
      }
      case ID(0, 0, 0, 1): { // Cache type
        return 0x0F0D2112u;
      }
      case ID(0, 1, 0, 0): { // Control register
        return m_control.word;
      }
      default: {
        ATOM_WARN("arm9: CP15: unhandled MRC #{}, C{}, C{}, #{}", opc1, cn, cm, opc2);
      }
    }

    return 0u;
  }

  void CP15::MCR(int opc1, int cn, int cm, int opc2, u32 value) {
    switch(ID(opc1, cn, cm, opc2)) {
      case ID(0, 1, 0, 0): { // Control register
        m_control.word = (value & 0x000FF085u) | 0x78u;

        m_cpu->SetExceptionBase(m_control.alternate_vector_select ? 0xFFFF0000u : 0u);

        if(m_control.enable_big_endian_mode) {
          ATOM_PANIC("arm9: CP15: enabled unemulated big-endian mode");
        }

        if(m_control.disable_loading_tbit) {
          ATOM_PANIC("arm9: CP15: enabled unemulated ARMv4T compatibility mode");
        }
        break;
      }
      case ID(0, 7, 0, 4): { // Wait for IRQ
        m_cpu->SetWaitingForIRQ(true);
        break;
      }
      default: {
        ATOM_WARN("arm9: CP15: unhandled MCR #{}, C{}, C{}, #{} = 0x{:08X}", opc1, cn, cm, opc2, value);
      }
    }
  }

} // namespace dual::nds::arm9
