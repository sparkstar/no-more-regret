/* KEYIO.C (11d0:02e1-0512): the keyboard process, which keeps the last key
 * pressed for the code that waits for a key. */
#ifndef PORT_KEYIO_H
#define PORT_KEYIO_H
#include "common.h"
#include "kprocess.h"
#include "keyboard.h"

class KeyboardProcess : public EventProcess {
public:
    explicit KeyboardProcess(Loading loading) : EventProcess(loading) {}
    KeyboardProcess();
    void run() override {}
    void event(u32 event, EventData data) override;
    void cleanup() override;
    void load(File *file) override;
    /* Waits for the next key with a character. */
    /* UNCALLED: nothing calls it. */
    KeyEvent waitKey();
};
#endif
