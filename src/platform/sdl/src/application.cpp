
#include <atom/logger/logger.hpp>
#include <fstream>

#include "application.hpp"

Application::Application() {
  SDL_Init(SDL_INIT_VIDEO);

  m_nds = std::make_unique<dual::nds::NDS>();
}

Application::~Application() {
  for(auto& texture : m_textures) SDL_DestroyTexture(texture);

  SDL_DestroyRenderer(m_renderer);
  SDL_DestroyWindow(m_window);
  SDL_Quit();
}

int Application::Run(int argc, char **argv) {
  CreateWindow();
  if(argc < 2) {
    LoadROM("armwrestler.nds");
  } else {
    LoadROM(argv[1]);
  }
  MainLoop();
  return 0;
}

void Application::CreateWindow() {
  m_window = SDL_CreateWindow(
    "ndsemu",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    512,
    768,
    SDL_WINDOW_ALLOW_HIGHDPI
  );

  m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

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

  m_nds->LoadROM(std::make_shared<dual::nds::MemoryROM>(data, size));
  m_nds->DirectBoot();

  ATOM_TRACE("Successfully loaded '{}'!", path);
}

void Application::MainLoop() {
  static const SDL_Rect rects[2] {
    {0,   0, 512, 384},
    {0, 384, 512, 384}
  };

  SDL_Event event;

  u16* vram = (u16*)m_nds->GetSystemMemory().lcdc_vram_hack.data();

  u32 framebuffer[256 * 192];

  while(true) {
    while(SDL_PollEvent(&event)) {
      if(event.type == SDL_QUIT) {
        return;
      }
    }

    m_nds->Step(559241);

    for(int i = 0; i < 256 * 192; i++) {
      const u16 rgb555 = vram[i];

      const int r = (rgb555 >>  0) & 31;
      const int g = (rgb555 >>  5) & 31;
      const int b = (rgb555 >> 10) & 31;

      framebuffer[i] = 0xFF000000 | r << 19 | g << 11 | b << 3;
    }

    SDL_UpdateTexture(m_textures[0], nullptr, framebuffer, 256 * sizeof(u32));

    SDL_RenderClear(m_renderer);
    SDL_RenderCopy(m_renderer, m_textures[0], nullptr, &rects[0]);
    SDL_RenderCopy(m_renderer, m_textures[1], nullptr, &rects[1]);
    SDL_RenderPresent(m_renderer);
  }
}
