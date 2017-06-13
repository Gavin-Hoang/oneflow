#include "oneflow/core/actor/copy_comm_net_actor.h"
#include "oneflow/core/actor/actor_registry.h"
#include "oneflow/core/register/local_register_warpper.h"

namespace oneflow {

// need review
void CopyCommNetActor::ProcessMsg(const ActorMsg& msg,
                                  const ThreadContext&) {
  CpuKernelCtx kernel_ctx(nullptr);
  ProcessMsgWithKernelCtx(msg, kernel_ctx);
}

REGISTER_ACTOR(kCopyCommNetTask, true, CopyCommNetActor);
REGISTER_ACTOR(kCopyCommNetTask, false, CopyCommNetActor);

}  // namespace oneflow
