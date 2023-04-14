
#include "application.hpp"

Application::Application() {
  SDL_Init(SDL_INIT_VIDEO);
}

Application::~Application() {
  for(auto& texture : m_textures) SDL_DestroyTexture(texture);

  SDL_DestroyRenderer(m_renderer);
  SDL_DestroyWindow(m_window);
  SDL_Quit();
}

int Application::Run(int argc, char **argv) {
  CreateWindow();
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

void Application::MainLoop() {
  static const SDL_Rect rects[2] {
    {0,   0, 512, 384},
    {0, 384, 512, 384}
  };

  SDL_Event event;

  while(true) {
    while(SDL_PollEvent(&event)) {
      if(event.type == SDL_QUIT) {
        return;
      }
    }

    SDL_RenderClear(m_renderer);
    SDL_RenderCopy(m_renderer, m_textures[0], nullptr, &rects[0]);
    SDL_RenderCopy(m_renderer, m_textures[1], nullptr, &rects[1]);
    SDL_RenderPresent(m_renderer);
  }
}
