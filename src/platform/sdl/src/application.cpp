
#include <algorithm>
#include <atom/logger/logger.hpp>
#include <atom/arguments.hpp>
#include <dual/nds/backup/eeprom512b.hpp>
#include <dual/nds/backup/flash.hpp>
#include <fstream>

#include "application.hpp"
#include "sdl2_audio_driver.hpp"

Application::Application() {
  SDL_Init(SDL_INIT_VIDEO);

  m_nds = std::make_unique<dual::nds::NDS>();
  m_nds->GetAPU().SetAudioDriver(std::make_shared<SDL2AudioDriver>());
}

Application::~Application() {
  for(auto& texture : m_textures) SDL_DestroyTexture(texture);

  SDL_DestroyRenderer(m_renderer);
  SDL_DestroyWindow(m_window);
  SDL_Quit();
}

int Application::Run(int argc, char** argv) {
  std::vector<const char*> files{};
  std::string boot7_path = "boot7.bin";
  std::string boot9_path = "boot9.bin";

  atom::Arguments args{"irisdual", "A Nintendo DS emulator developed for fun, with performance and multicore CPUs in mind.", {0, 1, 0}};
  args.RegisterArgument(boot7_path, true, "boot7", "Path to the ARM7 Boot ROM", "path");
  args.RegisterArgument(boot9_path, true, "boot9", "Path to the ARM9 Boot ROM", "path");
  args.RegisterFile("nds_file", false);

  if(!args.Parse(argc, argv, &files)) {
    std::exit(-1);
  }

  CreateWindow();
  // ARM7 boot ROM must be loaded before the ROM when firmware booting.
  LoadBootROM(boot7_path.c_str(), false);
  LoadBootROM(boot9_path.c_str(), true);
  LoadROM(files[0]);
  MainLoop();
  return 0;
}

void Application::CreateWindow() {
  m_window = SDL_CreateWindow(
    "irisdual",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    512,
    768,
    SDL_WINDOW_ALLOW_HIGHDPI
  );

  m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);

  for(auto& texture : m_textures) {
    texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 192);
  }
}

void Application::LoadROM(const char* path) {
  u8* data;
  size_t size;
  std::ifstream file{path, std::ios::binary};

  if(!file.good()) {
    ATOM_PANIC("Failed to open NDS file: '{}'", path);
  }

  file.seekg(0, std::ios::end);
  size = file.tellg();
  file.seekg(0);

  data = new u8[size];
  file.read((char*)data, static_cast<std::streamsize>(size));

  if(!file.good()) {
    ATOM_PANIC("Failed to read NDS file: '{}'", path);
  }

  const auto save_path = std::filesystem::path{path}.replace_extension("sav").string();

  // TODO: decide the correct save type
  std::shared_ptr<dual::nds::arm7::SPI::Device> backup = std::make_shared<dual::nds::FLASH>(save_path, dual::nds::FLASH::Size::_512K);

  m_nds->LoadROM(std::make_shared<dual::nds::MemoryROM>(data, size), backup);
  m_nds->DirectBoot();
}

void Application::LoadBootROM(const char* path, bool arm9) {
  const size_t maximum_size = arm9 ? 0x8000 : 0x4000;

  std::ifstream file{path, std::ios::binary};

  if(!file.good()) {
    ATOM_PANIC("Failed to open boot ROM: '{}'", path);
  }

  size_t size;

  file.seekg(0, std::ios::end);
  size = file.tellg();
  file.seekg(0);

  if(size > maximum_size) {
    ATOM_PANIC("Boot ROM is too big, expected {} bytes but got {} bytes", maximum_size, size);
  }

  std::array<u8, 0x8000> boot_rom{};

  file.read((char*)boot_rom.data(), static_cast<std::streamsize>(size));

  if(!file.good()) {
    ATOM_PANIC("Failed to read Boot ROM: '{}'", path);
  }

  if(arm9) {
    m_nds->LoadBootROM9(boot_rom);
  } else {
    m_nds->LoadBootROM7(std::span<u8, 0x4000>{boot_rom.data(), 0x4000});
  }
}

void Application::MainLoop() {
  static const SDL_Rect rects[2] {
    {0,   0, 512, 384},
    {0, 384, 512, 384}
  };

  SDL_Event event;

  m_emu_thread.Start(std::move(m_nds));

  while(true) {
    while(SDL_PollEvent(&event)) {
      if(event.type == SDL_QUIT) {
        return;
      }
      HandleEvent(event);
    }

    const auto frame = m_emu_thread.AcquireFrame();

    if(frame.has_value()) {
      SDL_UpdateTexture(m_textures[0], nullptr, frame.value().first, 256 * sizeof(u32));
      SDL_UpdateTexture(m_textures[1], nullptr, frame.value().second, 256 * sizeof(u32));

      SDL_RenderClear(m_renderer);
      SDL_RenderCopy(m_renderer, m_textures[0], nullptr, &rects[0]);
      SDL_RenderCopy(m_renderer, m_textures[1], nullptr, &rects[1]);
      SDL_RenderPresent(m_renderer);

      m_emu_thread.ReleaseFrame();

      UpdateFPS();
    }

    // @todo: move this logic into HandleEvent()

    m_emu_thread.SetFastForward(SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_SPACE]);

    if(SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_F11]) {
      m_nds = m_emu_thread.Stop();
      m_nds->Reset();
      m_emu_thread.Start(std::move(m_nds));
    }

    if(SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_F12]) {
      m_nds = m_emu_thread.Stop();
      m_nds->DirectBoot();
      m_emu_thread.Start(std::move(m_nds));
    }
  }
}

void Application::HandleEvent(const SDL_Event& event) {
  const u32 type = event.type;

  // @todo: do not hardcode the window geometry
  const i32 window_x0 = 0;
  const i32 window_x1 = 512;
  const i32 window_y0 = 384;
  const i32 window_y1 = 768;

  const auto window_xy_in_range = [](i32 x, i32 y) {
    return x >= window_x0 && x < window_x1 && y >= window_y0 && y < window_y1;
  };

  const auto window_x_to_screen_x = [&](i32 window_x) -> u8 {
    return std::clamp((window_x - window_x0) * 256 / (window_x1 - window_x0), 0, 255);
  };

  const auto window_y_to_screen_y = [&](i32 window_y) -> u8 {
    return std::clamp((window_y - window_y0) * 192 / (window_y1 - window_y0), 0, 191);
  };

  const auto set_touch_state = [&](i32 window_x, i32 window_y) {
    m_emu_thread.SetTouchState(m_touch_pen_down, window_x_to_screen_x(window_x), window_y_to_screen_y(window_y));
  };

  if(type == SDL_MOUSEBUTTONUP || type == SDL_MOUSEBUTTONDOWN) {
    const SDL_MouseButtonEvent& mouse_event = (const SDL_MouseButtonEvent&)event;

    if(mouse_event.button == SDL_BUTTON_LEFT) {
      const i32 window_x = mouse_event.x;
      const i32 window_y = mouse_event.y;

      m_touch_pen_down = window_xy_in_range(window_x, window_y) && mouse_event.state == SDL_PRESSED;
      set_touch_state(window_x, window_y);
    }
  }

  if(type == SDL_MOUSEMOTION) {
    const SDL_MouseMotionEvent& mouse_event = (const SDL_MouseMotionEvent&)event;

    const i32 window_x = mouse_event.x;
    const i32 window_y = mouse_event.y;

    if(window_xy_in_range(window_x, window_y)) {
      m_touch_pen_down = mouse_event.state & SDL_BUTTON_LMASK;
    }

    set_touch_state(window_x, window_y);
  }

  if(type == SDL_KEYUP || type == SDL_KEYDOWN) {
    const SDL_KeyboardEvent& keyboard_event = (const SDL_KeyboardEvent&)event;

    const auto update_key = [&](dual::nds::Key key) {
      m_emu_thread.SetKeyState(key, type == SDL_KEYDOWN);
    };

    switch(keyboard_event.keysym.sym) {
      case SDLK_a: update_key(dual::nds::Key::A); break;
      case SDLK_s: update_key(dual::nds::Key::B); break;
      case SDLK_d: update_key(dual::nds::Key::L); break;
      case SDLK_f: update_key(dual::nds::Key::R); break;
      case SDLK_q: update_key(dual::nds::Key::X); break;
      case SDLK_w: update_key(dual::nds::Key::Y); break;
      case SDLK_BACKSPACE: update_key(dual::nds::Key::Select); break;
      case SDLK_RETURN:    update_key(dual::nds::Key::Start);  break;
      case SDLK_UP:    update_key(dual::nds::Key::Up);    break;
      case SDLK_DOWN:  update_key(dual::nds::Key::Down);  break;
      case SDLK_LEFT:  update_key(dual::nds::Key::Left);  break;
      case SDLK_RIGHT: update_key(dual::nds::Key::Right); break;
    }
  }
}

void Application::UpdateFPS() {
  const auto now = std::chrono::system_clock::now();
  const auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(
    now - m_last_fps_update).count();

  m_fps_counter++;

  if(elapsed_time >= 1000) {
    const f32 fps = (f32)m_fps_counter / (f32)elapsed_time * 1000.0f;
    SDL_SetWindowTitle(m_window, fmt::format("irisdual [{:.2f} fps]", fps).c_str());
    m_fps_counter = 0;
    m_last_fps_update = std::chrono::system_clock::now();
  }
}
