
#pragma once

#include <dual/nds/nds.hpp>
#include <memory>

#include <SDL.h>

class Application {
  public:
    Application();
   ~Application();

    int Run(int argc, char** argv);

  private:
    void CreateWindow();
    void LoadROM(const char* path);
    void MainLoop();

    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
    SDL_Texture* m_textures[2];

    std::unique_ptr<dual::nds::NDS> nds{};
};