
#include <atom/logger/sink/console.hpp>
#include <atom/logger/logger.hpp>

#include "application.hpp"

#undef main

int main(int argc, char** argv) {
  atom::get_logger().InstallSink(std::make_shared<atom::LoggerConsoleSink>());

  return Application{}.Run(argc, argv);
}