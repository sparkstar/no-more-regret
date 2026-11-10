/* 1408:0299-0382, file name unknown: the anchor process. See anchor.h. */
#include "anchor.h"
#include "gamestate.h"

AnchorProcess *&anchorProcess = gameGlobal<AnchorProcess *>(nullptr);

AnchorProcess::AnchorProcess()
{
    anchorProcess = this;
    flags |= PROCESS_ACTIVE;
    makeSystem();
}

void AnchorProcess::cleanup()
{
    Process::cleanup();
    anchorProcess = nullptr;
}

void AnchorProcess::afterLoad()
{
    anchorProcess = this;
}
