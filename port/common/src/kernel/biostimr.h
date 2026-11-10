/* 11e0:02d1-044a, file name unknown: BiosTimer, the process that passes the
 * timer interrupt on to the real-mode BIOS handler at 18.2 Hz. A module of its
 * own after KEYBOARD.C: MOD2AMF.EXE starts a segment with it, and its data
 * starts on an even address. */
#ifndef PORT_BIOSTIMR_H
#define PORT_BIOSTIMR_H
#include <phapi.h>
#include "common.h"
#include "kprocess.h"

/* The real-mode address of the BIOS's timer handler (INT 8) on the
 * original's machine, which setVectors takes and a save holds. */
const REALPTR BIOS_TIMER_HANDLER = 0xf000fea5;

class BiosTimer : public PeriodicProcess {
public:
    explicit BiosTimer(Loading loading) : PeriodicProcess(loading) {}
    REALPTR realVector;
    void *protectedVector;

    BiosTimer();
    /* The handlers the tick goes on to: the BIOS's, the clock's own in the
     * port. The original gave it the vectors the kernel's hookTimer took;
     * the real-mode one (BIOS_TIMER_HANDLER) is in a save. */
    void setVectors();
    void run() override {}
    void event(u32 event, EventData data) override;
};
ASSERT_SIZE(BiosTimer, 0x48);
#endif
