/* See pcstate.h. */
#include "pcstate.h"

PcState pcState()
{
    return {timerState(), interruptState(),   keyboardState(), mouseState(),
            audioState(), videoMemoryState(), filesState()};
}

void putPcState(const PcState &state)
{
    putTimerState(state.timer);
    putInterruptState(state.interrupts);
    putKeyboardState(state.keyboard);
    putMouseState(state.mouse);
    putAudioState(state.audio);
    putVideoMemoryState(state.video);
    putFilesState(state.files);
}
