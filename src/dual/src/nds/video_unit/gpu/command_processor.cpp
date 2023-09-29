
#include <dual/nds/video_unit/gpu/command_processor.hpp>

namespace dual::nds::gpu {

  void CommandProcessor::cmdMtxPush() {
    DequeueFIFO();
  }

} // namespace dual::nds::gpu
