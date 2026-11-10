/* KDAEMON.C (11d0:0000-02e0): the key daemon, a process at 45 Hz that
 * repeats the keys held down, and in the port the other devices' inputs
 * held (devices.h). */
#ifndef PORT_KDAEMON_H
#define PORT_KDAEMON_H
#include "common.h"
#include "kprocess.h"

class KeyDaemon : public PeriodicProcess {
public:
    explicit KeyDaemon(Loading loading) : PeriodicProcess(loading) {}
    KeyDaemon();
    void run() override;
    void event(u32 event, EventData data) override;
    void cleanup() override;
    void load(File *file) override;
};

extern KeyDaemon *&keyDaemon;
/* Ticks before a held key repeats. */
extern u16 &repeatDelay;
inline void setRepeatDelay(u16 delay)
{
    repeatDelay = delay;
}
#endif
