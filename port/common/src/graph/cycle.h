/* CycleProcess (file name unknown): cycles the game palette. On each
 * timer event it rotates entries 1-7 and brightens the seven colours it keeps
 * for entries 8-14 (the channels of cycleChannels, by 2 each, wrapping from 63
 * to 0), then writes entries 1-14 to the VGA DAC. */
#ifndef PORT_CYCLE_H
#define PORT_CYCLE_H
#include "common.h"
#include "palette.h"
#include "kprocess.h"

/* CycleProcess's process type. */
constexpr int CYCLE_TYPE = 0x248;

class CycleProcess;
/* No Remorse's PLAYER.C reads it far. */
extern CycleProcess *&cycleProcess;

class CycleProcess : public PeriodicProcess {
public:
    explicit CycleProcess(Loading loading) : PeriodicProcess(loading) {}
    u8 paused;
    u8 stopped;

    CycleProcess();
    void run() override;
    void event(u32 event, EventData data) override;
    /* Writes one DAC entry. */
    void setColor(RGB color, i16 index);
    /* Adds 2 to the channels of color that are set in channels, wrapping
     * above 63 to 0; 1 when one wrapped. */
    bool brighten(RGB *color, RGB *channels);
    /* Subtracts 2, wrapping below 0 to 63. */
    /* UNCALLED: nothing calls it. */
    bool darken(RGB *color, RGB *channels);
    void cleanup() override;
    void afterLoad() override { cycleProcess = this; }
};
ASSERT_SIZE(CycleProcess, 0x42);
#endif
