
#include "application.hpp"

Application::Application() {
  SDL_Init(SDL_INIT_VIDEO);
}

Application::~Application() {
  for(auto& texture : textures) SDL_DestroyTexture(texture);

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

int Application::Run(int argc, char **argv) {
  CreateWindow();
  MainLoop();
  return 0;
}

void Application::CreateWindow() {
  window = SDL_CreateWindow(
    "heterochromia",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    512,
    768,
    SDL_WINDOW_ALLOW_HIGHDPI
  );

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

  for(auto& texture : textures) {
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 192);
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

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, textures[0], nullptr, &rects[0]);
    SDL_RenderCopy(renderer, textures[1], nullptr, &rects[1]);
    SDL_RenderPresent(renderer);
  }
}
