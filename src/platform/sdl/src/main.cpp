
#include "application.hpp"

#undef main

int main(int argc, char** argv) {
  return Application{}.Run(argc, argv);
}