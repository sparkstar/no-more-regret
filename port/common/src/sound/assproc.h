/* 12e0:0000-02ef, file name unknown: starts and stops ASYLUM.DLL and runs
 * its process. The module before AMFLOAD.C: its data (1480:5570-5612) ends
 * with the ASSProcess vtable, and AMFLOAD.C's starts on the next word. */
#ifndef PORT_ASSPROC_H
#define PORT_ASSPROC_H
#include "common.h"
#include "kprocess.h"

/* Updates the card on each timer event while ASYLUM.DLL runs. */
class ASSProcess : public PeriodicProcess {
public:
    explicit ASSProcess(Loading loading) : PeriodicProcess(loading) {}
    ASSProcess();
    void run() override;
    void cleanup() override;
    void load(File *file) override;
    void event(u32 event, EventData data) override;
};
ASSERT_SIZE(ASSProcess, 0x40);

/* The sound card setup: the mixing rate, the volumes and the oversampling. */
extern u16 &sampleRate;
extern u16 &soundVolume;
extern u16 &musicVolume;
extern u8 &oversampling;
/* Whether the configuration turned sound on (DSFXMAN.C). */
extern u8 &soundEnabled;
/* Whether the configuration turned music on (1480:954d); only Code5 reads
 * and writes it. */
extern u8 &musicOn;

/* Starts an ASSProcess when the effects run and there is none. */
void startASSProcess();
/* Starts ASYLUM.DLL on the card; the rate argument is not used. */
void initAsylum(u16 port, u16 irq, u16 dma, u16 card, u16 rate);
void closeAsylum();

/* Nothing calls these, in the original either. */
/* UNCALLED: nothing calls it. */
void unusedAmf();
#endif
