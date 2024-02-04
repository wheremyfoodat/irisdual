
#pragma once

#include <chrono>
#include <dual/nds/nds.hpp>
#include <memory>

#include <SDL.h>

#include "emulator_thread.hpp"

class Application {
  public:
    Application();
   ~Application();

    int Run(int argc, char** argv);

  private:
    void CreateWindow(int scale, bool fullscreen);
    void LoadROM(const char* path);
    void LoadBootROM(const char* path, bool arm9);
    void MainLoop();
    void HandleEvent(const SDL_Event& event);
    void UpdateFPS();

    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
    SDL_Texture* m_textures[2];
    SDL_Rect m_screen_geometry[2];

    std::unique_ptr<dual::nds::NDS> m_nds{};
    EmulatorThread m_emu_thread{};
    int m_fps_counter{};
    std::chrono::time_point<std::chrono::system_clock> m_last_fps_update{};

    bool m_touch_pen_down{false};
};