
#include <dual/nds/video_unit/gpu/command_processor.hpp>

namespace dual::nds::gpu {

  void CommandProcessor::cmdBeginVertices() {
    DequeueFIFO();

    // ...
  }

  void CommandProcessor::cmdEndVertices() {
    DequeueFIFO();

    // ...
  }

  void CommandProcessor::cmdSwapBuffers() {
    DequeueFIFO();

    // ...
  }

  void CommandProcessor::cmdViewport() {
    DequeueFIFO();

    // ...
  }

} // namespace dual::nds::gpu
