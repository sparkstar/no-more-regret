/* The controls' other devices: the gamepad, the mouse's middle and side
 * buttons and wheel, which come on the keys' line (input.h's DeviceEvent),
 * and the world's gump's mouse (world.cpp), as inputs of the bindings
 * (bindings.h).
 *
 * An input pressed gives the action it is bound to in the world, or in the
 * world's selection of items while it is selecting (the selection's
 * actions alone, as the original's joystick did then): the controller's
 * action as the action's key (bindings.h's actionKey) at the time, which
 * the kernel hands the controller where it hands it the keyboard's keys
 * (deliverDeviceKeys, after deliverKeys), repeating while it is held as the
 * key daemon repeats the keys (deviceTick), let go when it goes up, no
 * longer held when the keys are flushed (flushDevices); the world's and
 * the selection's as their code for the world's gump at the dispatcher's
 * next turn (deviceCode, dropped when another gump is on top); a mode's
 * held while it is down; the run's lock turned round. While the keyboard is
 * paused (kernel.h) a press is dropped. A stick leans one way at a time (the
 * axis it leans most, by a quarter more to change), a direction and a
 * trigger on beyond half their travel and off below two fifths; a stick's
 * direction bound to a turn turns at each tick by its lean (the mouse's
 * turn, Controller's mouseTurnAmount). The gamepad taken away lets go of
 * all of it. */
#ifndef PORT_DEVICES_H
#define PORT_DEVICES_H
#include "bindings.h"

/* Whether a device's input is down now (a mode's held). */
bool deviceDown(Device device);
/* How many times the action's key has come from the input since it went
 * down (its repeats, as kernel.h's keyRepeats); 0 when it is up or gave
 * the action in another way. */
u16 deviceRepeats(Device device, Action action);
/* An input of the world's gump's mouse (world.cpp): down, with the action
 * it gives as a key or a code (Action::NONE when the mouse's way does the
 * action, or for none); up; once (its motion: the action's key down and up,
 * or its code). */
void devicePress(Device device, Action action);
void deviceRelease(Device device);
void devicePulse(Device device, Action action);
/* The key daemon's tick (kdaemon.cpp): the keys of the inputs held repeat
 * as a key's do, and the sticks bound to a turn turn. */
void deviceTick();
/* The keys for the controller to it, as the kernel's deliverKeys hands it
 * the keyboard's (when it listens and is at the priority or above); the
 * kernel's flushKeys: those keys dropped, the inputs held no longer held
 * (as the keys' counts cleared), until they come down again. */
void deliverDeviceKeys();
void flushDevices();
/* The next code of a world's or a selection's action an input gave, for
 * the world's gump (the dispatcher's poll); false when none waits. */
bool deviceCode(i16 *code);
#endif
