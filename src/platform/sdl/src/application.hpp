
#pragma once

#include <SDL.h>

class Application {
  public:
    Application();
   ~Application();

    int Run(int argc, char** argv);

  private:
    void CreateWindow();
    void MainLoop();

    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* textures[2];
};