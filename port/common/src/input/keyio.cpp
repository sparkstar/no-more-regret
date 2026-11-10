/* KEYIO.C (11d0:02e1-0512): the keyboard process and the wait for a key. See
 * keyio.h. */
#include "keyio.h"
#include "gamestate.h"
#include "kernel.h"
#include "clock.h"

KeyboardProcess *&keyboardProcess = gameGlobal<KeyboardProcess *>(nullptr);
/* The last key with a character. Volatile: waitKey spins on it while the
 * keyboard interrupt fills it in. */
KeyEvent &lastKey = gameGlobal<KeyEvent>();

KeyboardProcess::KeyboardProcess()
{
    nameProcess(pid, "KeyboardProcess");
    if (keyboardProcess != nullptr) {
        halt();
    }
    keyboardProcess = this;
    setPriority(SYSTEM_LEVEL);
    setType(KEYBOARD_TYPE);
    listenKeyboard();
    flags |= PROCESS_ACTIVE;
    makeSystem();
    hold();
}

void KeyboardProcess::event(u32 event, EventData data)
{
    switch (event) {
    case EVENT_KEYBOARD: {
        KeyEvent *key;

        key = data.pointer<KeyEvent>();
        if ((key->ascii != 0U) && (key->pressed != 0U)) {
            lastKey = *key;
            eventMask &= ~EVENT_KEYBOARD;
        }
    }
    }
}

void KeyboardProcess::cleanup()
{
    keyboardProcess = nullptr;
}

void KeyboardProcess::load(File *file)
{
    Process::load(file);
    keyboardProcess = this;
}

/* UNCALLED: nothing calls it. */
KeyEvent KeyboardProcess::waitKey()
{
    lastKey.clear();
    eventMask |= EVENT_KEYBOARD;
    while (lastKey.ascii == 0U) {
        clockWait();
    }
    return lastKey;
}
