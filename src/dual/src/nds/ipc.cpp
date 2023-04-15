
#include <atom/logger/logger.hpp>
#include <dual/nds/ipc.hpp>

namespace dual::nds {

  IPC::IPC(IRQ& irq9, IRQ& irq7) {
    irq[(int)CPU::ARM9] = &irq9;
    irq[(int)CPU::ARM7] = &irq7;
  }

  void IPC::Reset() {
  }

  u32 IPC::IPCSYNC_ReadWord(CPU cpu) {
    return 0u;
  }

  void IPC::IPCSYNC_WriteWord(CPU cpu, u32 value, u32 mask) {
    ATOM_INFO("{}: IPC: write sync: 0x{:08X}", (cpu == CPU::ARM9) ? "arm9" : "arm7", value & mask);
  }

} // namespace dual::nds
