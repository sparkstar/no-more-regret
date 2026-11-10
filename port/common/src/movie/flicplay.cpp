/* FLICPLAY.C: plays the flics. See flicplay.h.
 *
 * The fades and the read-ahead's blocks are waited for in spins that the
 * timer's interrupt ends; the port's interrupts come as time passes, so each
 * turn lets it pass (clockWait), as fade.cpp's waits. */
#include "gamestate.h"
#include <cstdio>
#include <cstring>
#include "flicplay.h"
#include "asylum.h"
#include "basecam.h"
#include "camera.h"
#include "clock.h"
#include "cycle.h"
#include "dsfxman.h"
#include "fade.h"
#include "filespec.h"
#include "gamefile.h"
#include "gamepal.h"
#include "gametime.h"
#include "gray.h"
#include "gumpshap.h"
#include "hyperdsk.h"
#include "kernel.h"
#include "mousectl.h"
#include "music.h"
#include "npc.h"
#include "sndcache.h"
#include "system.h"
#include "systimer.h"
#include "utility.h"
#include "vframe.h"
#include "files.h"
#include "clock.h"
#include "host.h"

/* FlicPlayer::state */
constexpr int STATE_EVENTS = 1; /* the events of eventFrame */
constexpr int STATE_FRAME = 2;  /* the next object */
constexpr int STATE_REPEAT = 3; /* after the last frame: again, or stop */
constexpr int STATE_STOP = 5;

/* The sound of 'AVI' flics: a frame's worth of 22,050 Hz samples at the
 * default 15 frames a second, through a 64K ring behind a sound effect head. */
constexpr int FRAME_RATE = 15;
constexpr int SOUND_RATE = 22050;
constexpr int FRAME_SOUND = (SOUND_RATE / FRAME_RATE);
constexpr s32 SOUND_RING = (static_cast<s32>(0x10000L));

u8 &flicPlaying = gameGlobal<u8>(0);
/* The mouse was shown before the flic. */
u8 &flicMouseShown = gameGlobal<u8>(0);
u32 &flicVideoMode = gameSetting<u32>(VESA_640X480);
u8 &flicAnimationsHeld = gameGlobal<u8>(0);
u8 &flicGreyScreen = gameGlobal<u8>(0);

namespace {
/* UNCALLED: nothing calls it. */
void hideFlicMouse()
{
    if (mouseCtl != nullptr) {
        flicMouseShown = mouseCtl->visible;
        if (flicMouseShown != 0U) {
            mouseCtl->hide();
        }
    }
}

void showFlicMouse()
{
    if ((mouseCtl != nullptr) && (flicMouseShown != 0U)) {
        mouseCtl->show();
    }
}
}

FlicWait::FlicWait(u16 ticks, u16 newRate)
{
    nameProcess(pid, "FlicWait");
    count = ticks;
    setRate(newRate);
    eventMask |= EVENT_TIMER;
    startTimer(1);
}

void FlicWait::event(u32 event, EventData /*data*/)
{
    if (event == EVENT_TIMER) {
        if ((count == 0U) || pendingKeys > 0) {
            terminate(0);
        }
        count--;
    }
}

FlicRestoreProcess::FlicRestoreProcess(u8 held, u8 grey)
{
    animationsHeld = held;
    greyScreen = grey;
    setType(FLIC_RESTORE_TYPE);
}

void FlicRestoreProcess::run()
{
    if (flicVideoMode != MODE_13H && (greyScreen == 0U)) {
        Palette black(RGB(0, 0, 0));
        FadeProcess *fade = new FadeProcess(&black, 60, 1, 0, 1);
        u32 events;

        do {
            events = fade->eventMask;
            if ((events & EVENT_TIMER) != 0U) {
                clockWait();
            }
        } while ((events & EVENT_TIMER) != 0U);
    }
    if (gameCamera != nullptr) {
        gameCamera->flags &= ~PROCESS_ACTIVE;
        gameCamera->repaint();
        screenViewport->blit(drawViewport);
    }
    if (flicVideoMode == MODE_13H || (greyScreen == 0U)) {
        FadeProcess *fade = new FadeProcess(gamePalette, 20, 1, 0, 1);
        u32 events;

        do {
            events = fade->eventMask;
            if ((events & EVENT_TIMER) != 0U) {
                clockWait();
            }
        } while ((events & EVENT_TIMER) != 0U);
    }
    showFlicMouse();
    if (animationProcess != nullptr) {
        ::animationsHeld = animationsHeld;
    }
    releaseControlledNpc();
    if (cycleProcess != nullptr) {
        cycleProcess->paused = 0;
    }
    terminate(0);
}

/* UNCALLED: nothing calls it. */
FlicPlayer::FlicPlayer(char *name, i16 screenX, i16 screenY)
{
    flicPlaying = 1;
    setType(FLIC_TYPE);
    suspend();
    nameProcess(pid, "FlicPlayer");
    pause();
    FileSpec spec;
    /* Read only while there is an animationProcess, which the game has from
     * its start to its end. */
    u8 held = 0;

    spec.set(name, 0);
    holdControlledNpc();
    if (animationProcess != nullptr) {
        held = animationsHeld;
        animationsHeld = 1;
    }
    if (gameCamera != nullptr) {
        gameCamera->flags |= PROCESS_ACTIVE;
    }
    flicAnimationsHeld = held;
    flicGreyScreen = greyScreen;
    eventMask |= EVENT_RELATED_DIED;
    listenTo(pid);
    /* Only the file name is kept, in the caller's string. */
    strcpy(name, spec.name);
    if (fileExists(makePath(gameDrive, flicPath, name, ".skf")) == 0) {
        flags |= PROCESS_ACTIVE;
        hostTextAt(1, 3);
        hostPrint("Flic %s not found.", makePath(gameDrive, flicPath, name, ".skf"));
        flex = nullptr;
        /* Borland's sleep: two seconds, the clock's, whose ticks come
         * meanwhile as the original's did. */
        clockDelay(2000);
        return;
    }
    flex = new FastFlex(makePath(gameDrive, flicPath, name, ".skf"), FILE_READWRITE | FILE_SHARED, -1);
    this->x = screenX;
    this->y = screenY;
    held = 0;
    unknown4a = 0;
    unknown4c = 0;
    hasAudio = 0;
    soundBuffer = nullptr;
    soundWrite = nullptr;
    backdrop = nullptr;
    bandsPerTick = 2;
    frameBands = 0;
    readAhead = 1;
    applyPalette = 1;
    restoreScreen = 1;
    screenSaved = 0;
    band = 0;
    nextPreload = 0;
    eventIndex = 0;
    showBorder = 0;
    drawIndex = 0;
    background = screenViewport;
    fading = 0;
    fadeRate = 60;
    startTime = 0;
    unknown93 = 0;
    eventFrame = 0;
    frameTicks = 4;
    greyPalette = nullptr;
    greyScreen = 0;
    memset(preloads, 0, sizeof(preloads));
    readIndex = 0;
    loops = 0;
    frameCount = flex->count;
    flex->readObject(0, events, -1L, 0L);
    savedMusic = musicStatus();
    savedPalette = nullptr;
    savedMode = getVideoMode();
    palette = nullptr;
    pauseCycling = 1;
    frameData = nullptr;
    raisePriority = 0;
    soundStarted = 0;
    frame = 1;
    frameLoaded = 0;
    restore = 0;
    seekNeeded = 1;
    state = STATE_EVENTS;
    restoreMusic = 1;
    stopEffects = 1;
    framesDrawn = 0;
    videoMode = flicVideoMode;
    skipTicks = 0;
    keepMouse = 0;
    stopHyperDisk();
}

void FlicPlayer::onKill()
{
    i16 i;

    flicPlaying = 0;
    eventMask &= ~EVENT_TIMER;
    startHyperDisk();
    if (effectsStarted != 0U) {
        ASS_StopSFX(0);
    }
    if (flex == nullptr) {
        if (gameCamera != nullptr) {
            gameCamera->flags &= ~PROCESS_ACTIVE;
        }
        if (animationProcess != nullptr) {
            animationsHeld = flicAnimationsHeld;
        }
        releaseControlledNpc();
        if ((pauseCycling != 0U) && (cycleProcess != nullptr)) {
            cycleProcess->paused = 0;
        }
        return;
    }
    Process *other = findItemProcess(item, FLIC_TYPE);

    if ((effectsStarted != 0U) && (stopEffects != 0U)) {
        stopAllEffects();
    }
    if ((readAhead == 0U) && (frameData != nullptr)) {
        if (frameData != nullptr) {
            delete[] frameData;
            frameData = nullptr;
        }
    }
    if (readAhead != 0U) {
        for (i = 0; i < FLIC_BLOCKS; i++) {
            if (blocks[i].data != nullptr) {
                delete[] blocks[i].data;
                blocks[i].data = nullptr;
            }
        }
    }
    if ((other == nullptr) && static_cast<u16>(videoMode) != static_cast<u16>(savedMode)) {
        Palette black(RGB(0, 0, 0));
        FadeProcess *fade = new FadeProcess(&black, fadeRate, 1, 0, 1);
        u32 fadeEvents;

        do {
            fadeEvents = fade->eventMask;
            if ((fadeEvents & EVENT_TIMER) != 0U) {
                clockWait();
            }
        } while ((fadeEvents & EVENT_TIMER) != 0U);
        currentSystem->setMode(savedMode);
        black.apply();
    }
    if ((other == nullptr) && (greyScreen != 0U)) {
        if (backdrop != nullptr) {
            background->blit(backdrop, 0, 0, 319, 219, x - 20, y - 20);
        }
        if (greyPalette != nullptr) {
            greyPalette->apply();
        }
        File *file = new File("vidbg.vp", 0, 0);

        file->read(drawViewport->memory(),
                   static_cast<s32>(static_cast<i16>(drawViewport->right() - drawViewport->left() + 1)) *
                       static_cast<i16>(drawViewport->bottom() - drawViewport->top() + 1));
        if (file != nullptr) {
            delete file;
            file = nullptr;
        }
        fileRemove("vidbg.vp");
        background->blit(drawViewport);
        FadeProcess *fade = new FadeProcess(gamePalette, fadeRate, 1, 0, 1);
        u32 fadeEvents;

        do {
            fadeEvents = fade->eventMask;
            if ((fadeEvents & EVENT_TIMER) != 0U) {
                clockWait();
            }
        } while ((fadeEvents & EVENT_TIMER) != 0U);
    }
    if (greyPalette != nullptr) {
        delete greyPalette;
        greyPalette = nullptr;
    }
    if (soundBuffer != nullptr) {
        delete[] soundBuffer;
        soundBuffer = nullptr;
    }
    if (palette != nullptr) {
        delete palette;
        palette = nullptr;
    }
    if (flex != nullptr) {
        delete flex;
        flex = nullptr;
    }
    if (backdrop != nullptr) {
        delete backdrop;
        backdrop = nullptr;
    }
    if (savedPalette != nullptr) {
        delete savedPalette;
        savedPalette = nullptr;
    }
    if ((restoreMusic != 0U) && (other == nullptr)) {
        playSong(savedMusic);
    }
    if ((restoreScreen != 0U) && (restore == 0U)) {
        if (gameCamera != nullptr) {
            gameCamera->flags &= ~PROCESS_ACTIVE;
        }
        showFlicMouse();
        if (animationProcess != nullptr) {
            animationsHeld = flicAnimationsHeld;
        }
        releaseControlledNpc();
        if ((pauseCycling != 0U) && (cycleProcess != nullptr) && (other == nullptr)) {
            cycleProcess->paused = 0;
        }
        if (applyPalette == 0U) {
            gameCamera->repaint();
        }
    }
}

/* UNCALLED: nothing calls it. */
void FlicPlayer::start()
{
    if (flex == nullptr) {
        terminate(0);
        return;
    }
    if (keepMouse == 0U) {
        hideFlicMouse();
    } else {
        flicMouseShown = 0;
    }
    if ((pauseCycling != 0U) && (cycleProcess != nullptr)) {
        cycleProcess->paused = 1;
    }
    if (raisePriority != 0U) {
        setPriority(currentPriority + 1);
        setCurrentPriority(level());
    }
    if (static_cast<u16>(videoMode) != static_cast<u16>(savedMode)) {
        Palette black(RGB(0, 0, 0));
        FadeProcess *fade = new FadeProcess(&black, fadeRate, 0, 0, 1);
        u32 fadeEvents;

        do {
            fadeEvents = fade->eventMask;
            if ((fadeEvents & EVENT_TIMER) != 0U) {
                clockWait();
            }
        } while ((fadeEvents & EVENT_TIMER) != 0U);
        currentSystem->setMode(videoMode);
        if (videoMode == MODE_13H) {
            background = vgaScreen;
        } else {
            background = svgaScreen;
        }
        x = 40;
        y = 10;
        greyScreen = 0;
    }
    savedPalette = new Palette(0, 256);
    savedPalette->capture();
    FlexHeader header;

    flex->readHeader(&header);
    if (strcmp(header.format, "AVI") == 0) {
        hasAudio = 1;
    }
    if (greyScreen != 0U) {
        i16 i;
        u8 *pixel;

        clearDirtyRects();
        if (cycleProcess != nullptr) {
            cycleProcess->paused = 1;
        }
        u8 nearest[16];

        /* The colours the GUI uses keep their nearest in the flic's range. */
        for (i = 0; i < 8; i++) {
            nearest[i] = savedPalette->closestTo(i, 8, 240);
        }
        for (i = 248; i < 256; i++) {
            nearest[static_cast<i16>(i - 240)] = savedPalette->closestTo(i, 8, 240);
        }
        for (i16 row = 0; row < static_cast<i16>(drawViewport->bottom() - drawViewport->top() + 1); row++) {
            pixel = drawViewport->address(0, row);
            for (i16 column = 0; column < static_cast<i16>(drawViewport->right() - drawViewport->left() + 1);
                 column++, pixel++) {
                if (*pixel < 8) {
                    *pixel = nearest[*pixel];
                } else if (*pixel >= 248) {
                    *pixel = nearest[static_cast<i16>(*pixel - 240)];
                }
            }
        }
        greyPalette = new Palette(*savedPalette);
        greyPalette->toYiq();
        for (i = 0; i < 256; i++) {
            greyPalette->data[i].blue = greyPalette->data[i].green = greyPalette->data[i].red;
        }
        i16 greyEntry;

        for (greyEntry = 0; greyEntry < 8; greyEntry++) {
            greyPalette->data[greyEntry].red = greyPalette->data[greyEntry].green = greyPalette->data[greyEntry].blue =
                greyEntry << 2;
        }
        for (greyEntry = 248; greyEntry < 256; greyEntry++) {
            greyPalette->data[greyEntry].red = greyPalette->data[greyEntry].green = greyPalette->data[greyEntry].blue =
                (greyEntry + 16) << 2;
        }
        FadeProcess *fade = new FadeProcess(greyPalette, fadeRate, 0, 0, 1);

        fade->block(this);
        background->blit(drawViewport);
        File *file = new File("vidbg.vp", FILE_READWRITE, 1);

        file->write(drawViewport->memory(),
                    static_cast<s32>(static_cast<i16>(drawViewport->right() - drawViewport->left() + 1)) *
                        static_cast<i16>(drawViewport->bottom() - drawViewport->top() + 1));
        if (file != nullptr) {
            delete file;
            file = nullptr;
        }
        backdrop = new Viewport(VIEWPORT_LARGE, 0, 0, 320, 220);
        backdrop->blit(drawViewport, x - 20, y - 20, x + 300, y + 200);
        toFixedGray(backdrop, savedPalette);
        if (showBorder != 0U) {
            BandFrame first;

            if (hasAudio != 0U) {
                flex->readObject(2, &first, sizeof(first), 4L);
            } else {
                flex->readObject(2, &first, sizeof(first), 2L);
            }
            if (first.height <= 121) {
                if (first.width <= 161) {
                    const GumpShape border(0x34);

                    border.draw(x, y);
                } else {
                    const GumpShape border(0x35);

                    border.draw(x, y);
                }
            } else if (first.height <= 181) {
                const GumpShape border(0x33);

                border.draw(x, y);
            }
        }
        toFixedGray(drawViewport, savedPalette);
    } else if (showBorder != 0U) {
        if (videoMode == MODE_13H || (applyPalette != 0U)) {
            drawViewport->clear(0);
        }
        BandFrame first;

        if (hasAudio != 0U) {
            flex->readObject(2, &first, sizeof(first), 4L);
        } else {
            flex->readObject(2, &first, sizeof(first), 2L);
        }
        if (first.height <= 121) {
            if (first.width <= 161) {
                const GumpShape border(0x34);

                border.draw(x, y);
            } else {
                const GumpShape border(0x35);

                border.draw(x, y);
            }
        } else if (first.height <= 181) {
            const GumpShape border(0x33);

            border.draw(x, y);
        }
        if (applyPalette != 0U) {
            toFixedGray(drawViewport, gamePalette);
        } else {
            background->blit(drawViewport);
        }
    }
    /* At least a palette frame: its type and 256 colours. */
    s32 largest = 0x302;

    if (readAhead != 0U) {
        i16 i;
        for (i = 2; i < frameCount; i++) {
            const s32 size = flex->objectSize(i);

            if (size > largest) {
                largest = size;
            }
        }
        /* The i of the loop above: BCC 3.1 keeps a for statement's
         * declaration to the end of the enclosing block. */
        for (i = 0; i < FLIC_BLOCKS; i++) {
            memset(&blocks[i], 0, sizeof(FlicBlock));
            blocks[i].data = new u8[largest];
            memset(blocks[i].data, 0, largest);
            blocks[i].kind = BLOCK_FREE;
        }
    }
    if (hasAudio != 0U) {
        if (effectsStarted != 0U) {
            soundBuffer = new u8[SOUND_RING];
            soundWrite = soundBuffer + sizeof(SoundEffect);
            SoundEffect *effect = reinterpret_cast<SoundEffect *>(soundBuffer);

            effect->length = SOUND_RING - static_cast<ui16>(sizeof(SoundEffect));
            effect->loopStart = 0;
            effect->unknown10 = 0;
            effect->loopLength = effect->length;
            effect->rate = SOUND_RATE;
        }
    } else if (frame != 1) {
        eventFrame = frame + 1;
    }
    i16 event;
    i16 preload;

    for (event = 0, preload = 0; events[event].frame != 0xffff; event++) {
        if (events[event].type != FLIC_PRELOAD) {
            continue;
        }
        preloads[preload].frame = events[event].frame;
        if (preloads[preload].frame > flex->count) {
            break;
        }
        s32 size = flex->objectSize(events[event].frame);

        if (readAhead != 0U) {
            size = largest;
        }
        preloads[preload].data = new u8[size];
        flex->readObject(events[event].frame, preloads[preload].data, -1L, 0L);
        preload++;
    }
    playStart = systemTimer->ticks;
}

/* UNCALLED: nothing calls it. */
void FlicPlayer::stop()
{
    eventMask &= ~EVENT_TIMER;
    flags |= PROCESS_ACTIVE;
    terminate(0);
}

void FlicPlayer::run()
{
    /* An empty test the original keeps. */
    if ((musicEnabled != 0U) && (raisePriority != 0U)) {
        /* empty in the original */
    }
    if (readAhead != 0U) {
        s32 size;
        u16 room;
        i16 i;

        if ((hasFlags(PROCESS_TIMED) == 0U) && frame == 6 && (soundStarted != 0U)) {
            startTimer(1);
            startTime = systemTimer->ticks;
            eventMask |= EVENT_TIMER;
            event(EVENT_TIMER, 0);
            if ((effectsStarted != 0U) && (soundBuffer != nullptr)) {
                ASS_PlaySFX(soundBuffer, 0, 64, 0, 1);
            }
        }
        while ((size = flex->objectSize(frame)) == 0) {
            frame++;
        }
        if (frame == static_cast<ui16>(preloads[nextPreload].frame)) {
            if (blocks[static_cast<int>(readIndex)].data != nullptr) {
                delete[] blocks[static_cast<int>(readIndex)].data;
                blocks[static_cast<int>(readIndex)].data = nullptr;
            }
            blocks[static_cast<int>(readIndex)].data = preloads[nextPreload++].data;
            seekNeeded = 1;
        } else {
            if ((hasAudio == 0U) || (seekNeeded != 0U)) {
                flex->seekObject(frame);
                seekNeeded = 0;
            }
            s32 piece = 0x1000;
            s32 offset;

            for (offset = 0; offset < size; offset += piece) {
                if (piece + offset > size) {
                    piece = size - offset;
                }
                flex->read(blocks[static_cast<int>(readIndex)].data + offset, piece);
            }
        }
        if ((soundStarted == 0U) && *reinterpret_cast<i16 *>(blocks[static_cast<int>(readIndex)].data) == CHUNK_FRAME) {
            u8 *picture;

            if (hasAudio != 0U) {
                picture = blocks[static_cast<int>(readIndex)].data + 4;
            } else {
                picture = blocks[static_cast<int>(readIndex)].data + 2;
            }
            soundStarted = 1;
            frameBands = (reinterpret_cast<BandFrame *>(picture))->height / BAND_LINES;
            if (frameBands < 6) {
                frameBands = 6;
            }
            if (frameBands % bandsPerTick == 0) {
                setRate(frameBands / bandsPerTick * FRAME_RATE);
            } else {
                setRate((frameBands / bandsPerTick + 1) * FRAME_RATE);
            }
        }
        if (*reinterpret_cast<i16 *>(blocks[static_cast<int>(readIndex)].data) == CHUNK_FRAME && (hasAudio != 0U) &&
            (effectsStarted != 0U)) {
            const u16 length = (reinterpret_cast<u16 *>(blocks[static_cast<int>(readIndex)].data))[1];

            room = SOUND_RING - static_cast<u16>(soundWrite - soundBuffer);
            u8 *samples = blocks[static_cast<int>(readIndex)].data + length + 4;

            if (room < static_cast<ui16>(FRAME_SOUND)) {
                memcpy(soundWrite, samples, room);
                soundWrite = soundBuffer + sizeof(SoundEffect);
                memcpy(soundWrite, samples + room, FRAME_SOUND - room);
                soundWrite += FRAME_SOUND - room;
            } else {
                memcpy(soundWrite, samples, FRAME_SOUND);
                soundWrite += FRAME_SOUND;
            }
        }
        frameData = blocks[static_cast<int>(readIndex)].data;
        if (*reinterpret_cast<i16 *>(frameData) == CHUNK_FRAME) {
            blocks[static_cast<int>(readIndex)].kind = BLOCK_READY;
        } else {
            blocks[static_cast<int>(readIndex)].kind = BLOCK_FREE;
        }
        readIndex = static_cast<i16>(readIndex + 1) % FLIC_BLOCKS;
        i = 0;
        while (i++ < FLIC_BLOCKS && blocks[static_cast<int>(drawIndex)].kind == BLOCK_FREE) {
            drawIndex = static_cast<i16>(drawIndex + 1) % FLIC_BLOCKS;
        }
        while (blocks[static_cast<int>(readIndex)].kind != BLOCK_FREE) {
            clockWait();
        }
        blocks[static_cast<int>(readIndex)].kind = BLOCK_READING;
    }
    if ((readAhead != 0U) && pendingKeys > 0 && (hasFlags(PROCESS_TIMED) != 0U)) {
        state = STATE_STOP;
        if ((effectsStarted != 0U) && (stopEffects != 0U)) {
            stopAllEffects();
        }
    }
    switch (state) {
    case STATE_EVENTS:
        while (events[eventIndex].frame == static_cast<ui16>(eventFrame)) {
            eventValue = events[eventIndex].value;
            /* wait, black and white are declared between the cases, without
             * braces: they take the switch body's frame slots in this order.
             * BCC 3.1 accepts the jumps past their initializations; a
             * standard C++ compiler does not. */
            switch (events[eventIndex].type) {
            case FLIC_PLAY_MUSIC:
                if (musicEnabled != 0U) {
                    playSong(eventValue);
                }
                break;
            case FLIC_STOP_MUSIC:
                break;
            case FLIC_PLAY_EFFECT:
                if (effectsStarted != 0U) {
                    playEffect(eventValue);
                }
                break;
            case FLIC_STOP_EFFECT:
                if (effectsStarted != 0U) {
                    stopEffect(eventValue);
                }
                break;
            case FLIC_SET_SPEED:
                if (eventValue > 60 || eventValue < 1) {
                    break;
                }
                frameTicks = 60 / eventValue;
                playStart = systemTimer->ticks;
                if ((readAhead != 0U) && (frameBands != 0)) {
                    setRate(eventValue * (frameBands / bandsPerTick));
                }
                break;
            case FLIC_WAIT:
                FlicWait *wait;
                wait = new FlicWait(eventValue, 10);

                eventMask &= ~EVENT_TIMER;
                wait->listenTo(pid);
                eventIndex++;
                return;
            case FLIC_PLAY_SOUND:
                if (effectsStarted != 0U) {
                    flex->readEntry(eventValue, &entry);
                    u8 *sound = new u8[entry.size - 2];

                    flex->readObject(eventValue, sound, entry.size - 2, 2L);
                }
                break;
            case FLIC_FADE_OUT: {
                Palette black(RGB(0, 0, 0));

                new FadeProcess(&black, eventValue, 0, 0, 1);
                fading = 1;
                break;
            }
            case FLIC_FADE_IN:
                new FadeProcess(palette, eventValue, 0, 0, 1);
                fading = 0;
                break;
            case FLIC_FADE_WHITE:
                Palette white(RGB(63, 63, 63));

                new FadeProcess(&white, eventValue, 0, 0, 1);
                fading = 1;
                break;
            }
            eventIndex++;
        }
        state = STATE_FRAME;
        [[fallthrough]];
    case STATE_FRAME:
        if ((readAhead == 0U) && (frameLoaded == 0U)) {
            if (frameData != nullptr) {
                delete[] frameData;
                frameData = nullptr;
            }
            flex->readEntry(frame, &entry);
            if (entry.size != 0) {
                frameData = new u8[entry.size + 1000];
                flex->readObject(frame, frameData, entry.size, 0L);
                frameLoaded = 1;
            }
        }
        if (frameData != nullptr) {
            switch (*reinterpret_cast<i16 *>(frameData)) {
            case CHUNK_PALETTE:
                if (palette != nullptr) {
                    delete palette;
                    palette = nullptr;
                }
                palette = new Palette(reinterpret_cast<RGB *>(frameData + 2));
                if (fading == 0U) {
                    palette->data[0].red = 0;
                    palette->data[0].green = 0;
                    palette->data[0].blue = 0;
                    if ((greyScreen != 0U) || (flicGreyScreen != 0U) || (hasAudio != 0U)) {
                        i16 i;

                        for (i = 0; i < 8; i++) {
                            palette->data[i].red = palette->data[i].green = palette->data[i].blue = i << 2;
                        }
                        for (i = 248; i < 256; i++) {
                            palette->data[i].red = palette->data[i].green = palette->data[i].blue = (i + 16) << 2;
                        }
                        palette->apply(0, 8);
                        palette->apply(248, 8);
                        if (screenSaved == 0U) {
                            background->blit(drawViewport);
                            screenSaved = 1;
                        }
                    }
                    if (applyPalette != 0U) {
                        palette->apply();
                    }
                }
                frameLoaded = 0;
                break;
            case CHUNK_FRAME:
                eventFrame++;
                if (readAhead != 0U) {
                    break;
                }
                if (startTime == 0U) {
                    startTime = systemTimer->ticks;
                }
                if (frameTicks * framesDrawn > systemTimer->ticks - startTime) {
                    return;
                }
                framesDrawn++;
                frameLoaded = 0;
                drawBands(background, reinterpret_cast<BandFrame *>(frameData + 2), x, y, 0, -1);
                break;
            case 13:
            case 16:
            case 20:
                frameLoaded = 0;
                state = STATE_REPEAT;
                return;
            default:
                frameLoaded = 0;
                break;
            }
        }
        frame++;
        if (frame < frameCount) {
            state = STATE_EVENTS;
            return;
        }
        [[fallthrough]];
    case STATE_REPEAT:
        if (loops != 0) {
            if (loops > 0) {
                loops--;
            }
            if (hasAudio != 0U) {
                frame = 3;
            } else {
                frame = 1;
            }
            eventFrame = 0;
            eventIndex = 0;
            state = STATE_EVENTS;
            return;
        }
        [[fallthrough]];
    case STATE_STOP:
        if (readAhead != 0U) {
            i16 busy = 2;

            while (busy > 1 && (pendingKeys == 0U)) {
                busy = 0;
                for (i16 i = 0; i < FLIC_BLOCKS; i++) {
                    if (blocks[i].kind != BLOCK_FREE) {
                        busy++;
                    }
                }
                if (busy > 1) {
                    clockWait();
                }
            }
        }
        eventMask &= ~EVENT_TIMER;
        terminate(0);
        break;
    }
}

void FlicPlayer::event(u32 event, EventData /*data*/)
{
    if ((event & EVENT_RELATED_DIED) != 0U) {
        Process *other = findItemProcess(item, FLIC_TYPE);

        if ((other == nullptr) && (restore != 0U)) {
            Process *restorer = new FlicRestoreProcess(flicAnimationsHeld, greyScreen);

            block(restorer);
        }
    } else if (skipTicks != 0U) {
        skipTicks--;
    } else {
        if (blocks[static_cast<int>(drawIndex)].kind == BLOCK_FREE) {
            drawIndex = drawIndex == FLIC_BLOCKS - 1 ? 0 : drawIndex + 1;
        }
        if (blocks[static_cast<int>(drawIndex)].kind == BLOCK_READY ||
            blocks[static_cast<int>(drawIndex)].kind == BLOCK_DRAWING) {
            BandFrame *picture;

            if (hasAudio != 0U) {
                picture = reinterpret_cast<BandFrame *>(blocks[static_cast<int>(drawIndex)].data + 4);
            } else {
                picture = reinterpret_cast<BandFrame *>(blocks[static_cast<int>(drawIndex)].data + 2);
            }
            if (blocks[static_cast<int>(drawIndex)].kind == BLOCK_READY) {
                blocks[static_cast<int>(drawIndex)].kind = BLOCK_DRAWING;
                band = 0;
            }
            const i16 bands = static_cast<i16>(band + bandsPerTick) > frameBands ? frameBands - band : bandsPerTick;

            if (static_cast<u16>(videoMode) == static_cast<u16>(MODE_13H)) {
                drawBands(background, picture, x, y, band, bands);
            } else {
                drawBandsBanked(screenViewport, picture, x, y, band, bands);
            }
            band += bands;
            if (band < frameBands && static_cast<ui16>(band * BAND_LINES) < picture->height) {
                return;
            }
            if (static_cast<ui16>(band * BAND_LINES) >= picture->height) {
                while (band < frameBands) {
                    skipTicks++;
                    band += bandsPerTick;
                }
            }
            framesDrawn++;
            blocks[static_cast<int>(drawIndex)].kind = BLOCK_FREE;
            drawIndex = drawIndex == FLIC_BLOCKS - 1 ? 0 : drawIndex + 1;
        }
    }
}
