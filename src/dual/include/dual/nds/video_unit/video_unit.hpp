
#pragma once

#include <atom/bit.hpp>
#include <atom/integer.hpp>
#include <dual/common/scheduler.hpp>
#include <dual/nds/arm7/dma.hpp>
#include <dual/nds/arm9/dma.hpp>
#include <dual/nds/video_unit/gpu/gpu.hpp>
#include <dual/nds/video_unit/ppu/ppu.hpp>
#include <dual/nds/enums.hpp>
#include <dual/nds/irq.hpp>
#include <dual/nds/system_memory.hpp>
#include <functional>

namespace dual::nds {

  class VideoUnit {
    public:
      VideoUnit(
        Scheduler& scheduler,
        SystemMemory& memory,
        IRQ& irq9,
        IRQ& irq7,
        arm9::DMA& dma9,
        arm7::DMA& dma7
      );

      void Reset();
      void DirectBoot();

      void SetPresentationCallback(std::function<void(const u32*, const u32*)> present_callback) {
        m_present_callback = std::move(present_callback);
      }

      GPU& GetGPU() {
        return m_gpu;
      }

      PPU& GetPPU(int id) {
        return m_ppu[id];
      }

      u16   Read_DISPSTAT(CPU cpu);
      void Write_DISPSTAT(CPU cpu, u16 value, u16 mask);

      u16 Read_VCOUNT();

      u16   Read_POWCNT1();
      void Write_POWCNT(u16 value, u16 mask);

      u32   Read_DISPCAPCNT();
      void Write_DISPCAPCNT(u32 value, u32 mask);

    private:
      static constexpr int k_drawing_lines = 192;
      static constexpr int k_blanking_lines = 71;
      static constexpr int k_total_lines = k_drawing_lines + k_blanking_lines;

      void UpdateVerticalCounterMatchFlag(CPU cpu);
      void BeginHDraw(int late);
      void BeginHBlank(int late);
      void RunDisplayCapture();

      Scheduler& m_scheduler;
      VRAM& m_vram;

      GPU m_gpu;
      PPU m_ppu[2];

      union DISPSTAT {
        atom::Bits<0, 1, u16> vblank_flag;
        atom::Bits<1, 1, u16> hblank_flag;
        atom::Bits<2, 1, u16> vmatch_flag;
        atom::Bits<3, 1, u16> enable_vblank_irq;
        atom::Bits<4, 1, u16> enable_hblank_irq;
        atom::Bits<5, 1, u16> enable_vmatch_irq;
        atom::Bits<7, 1, u16> vmatch_setting_msb;
        atom::Bits<8, 8, u16> vmatch_setting;

        u16 half = 0u;
      } m_dispstat[2];

      u16 m_vcount{};

      union POWCNT1 {
        atom::Bits< 0, 1, u16> enable_lcds;
        atom::Bits< 1, 1, u16> enable_ppu_a;
        atom::Bits< 2, 1, u16> enable_gpu_render_engine;
        atom::Bits< 3, 1, u16> enable_gpu_geometry_engine;
        atom::Bits< 9, 1, u16> enable_ppu_b;
        atom::Bits<15, 1, u16> enable_display_swap;

        u16 half = 0u;
      } m_powcnt1{};

      union DISPCAPCNT {
        enum class SourceA {
          GPUAndPPU = 0,
          GPU = 1
        };

        enum class SourceB {
          VRAM = 0,
          MainMemoryDisplayFIFO = 1
        };

        enum class CaptureSource {
          A = 0,
          B = 1,
          BlendAB = 2
        };

        atom::Bits< 0, 5, u32> eva;
        atom::Bits< 8, 5, u32> evb;
        atom::Bits<16, 2, u32> vram_write_block;
        atom::Bits<18, 2, u32> vram_write_offset;
        atom::Bits<20, 2, u32> capture_size;
        atom::Bits<24, 1, u32> src_a;
        atom::Bits<25, 1, u32> src_b;
        atom::Bits<26, 2, u32> vram_read_offset;
        atom::Bits<29, 2, u32> capture_src;
        atom::Bits<31, 1, u32> capture_enable;

        u32 word = 0u;
      } m_dispcapcnt{};

      bool m_display_swap_latch{};
      bool m_display_capture_active{};

      IRQ* m_irq[2]{};
      arm9::DMA& m_dma9;
      arm7::DMA& m_dma7;
      std::function<void(const u32*, const u32*)> m_present_callback;
  };

} // namespace dual::nds
