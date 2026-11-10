/* The module after UNITFILE.C (1408:0299-0382, file name unknown): one
 * system process that never runs. Code that runs a process directly
 * (ANIMPRIM.C and 1118) first makes it wait on this one, then reads this
 * one's result. */
#ifndef PORT_ANCHOR_H
#define PORT_ANCHOR_H
#include "common.h"
#include "kprocess.h"

class AnchorProcess : public Process {
public:
    explicit AnchorProcess(Loading loading) : Process(loading) {}
    AnchorProcess();
    void run() override {}
    void cleanup() override;
    void afterLoad() override;
};
ASSERT_SIZE(AnchorProcess, 0x32);

extern AnchorProcess *&anchorProcess;

/* No Remorse's (ANIMPRIM.C's own class): the process each test move of
 * AnimPrimitive::init waits on, which ends as soon as it runs; init ends it
 * when the moves do not, else it lives to its run at the next pass (a
 * save then holds it). Vtable 1478:2A33. */
class TestAnchor final : public Process {
public:
    explicit TestAnchor(Loading loading) : Process(loading) {}
    TestAnchor() = default;
    void run() override { terminate(0); }
};
ASSERT_SIZE(TestAnchor, 0x32);
#endif
