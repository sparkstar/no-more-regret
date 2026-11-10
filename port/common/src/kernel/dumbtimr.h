/* DumbTimer (1408:0383-04a0, file name unknown): a system process that calls
 * CONTROL.C every second (60 ticks of the system timer). */
#ifndef PORT_DUMBTIMR_H
#define PORT_DUMBTIMR_H
#include "common.h"
#include "kprocess.h"

class DumbTimer : public PeriodicProcess {
public:
    explicit DumbTimer(Loading loading) : PeriodicProcess(loading) {}
    u32 lastTick;

    DumbTimer();
    void run() override;
    void event(u32 event, EventData data) override;
};
ASSERT_SIZE(DumbTimer, 0x44);

#endif
