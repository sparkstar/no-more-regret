/* FLICPLAY.C: plays the game's flics (<drive><dir>\name.skf). A flic is
 * a FLEX archive: object 0 is a table of events, each tied to a frame; the
 * other objects start with a chunk type. A frame (CHUNK_FRAME) is a band
 * frame of VFRAME.C; in 'AVI' flics the type is followed by the frame's
 * length, and a frame's worth of sound follows the frame. With the frames
 * read ahead (readAhead), the timer event draws them a band at a time while
 * the sound plays. */
#ifndef PORT_FLICPLAY_H
#define PORT_FLICPLAY_H
#include "common.h"
#include "fastflex.h"
#include "palette.h"
#include "kprocess.h"
#include "viewport.h"

/* The process types. */
constexpr int FLIC_TYPE = 0x240;
constexpr int FLIC_RESTORE_TYPE = 0x25d;

/* The frames read ahead, the frames kept by FLIC_PRELOAD and the events. */
constexpr int FLIC_BLOCKS = 5;
constexpr int FLIC_PRELOADS = 20;
constexpr int FLIC_EVENTS = 256;

/* The chunk types. 13, 16 and 20 end the flic's pass. */
constexpr int CHUNK_PALETTE = 1; /* 256 colours */
constexpr int CHUNK_FRAME = 2;

/* FlicBlock::kind */
constexpr int BLOCK_READING = 1;
constexpr int BLOCK_READY = 2; /* a frame to draw */
constexpr int BLOCK_DRAWING = 3;
constexpr int BLOCK_FREE = 4;

/* FlicEvent::type */
constexpr int FLIC_PLAY_MUSIC = 3; /* song value */
constexpr int FLIC_STOP_MUSIC = 4; /* not played here */
constexpr int FLIC_PLAY_EFFECT = 5;
constexpr int FLIC_STOP_EFFECT = 6;
constexpr int FLIC_SET_SPEED = 7;   /* value frames a second */
constexpr int FLIC_FADE_OUT = 8;    /* to black at rate value */
constexpr int FLIC_FADE_IN = 9;     /* to the flic's palette */
constexpr int FLIC_WAIT = 12;       /* value ticks, or until a key */
constexpr int FLIC_PLAY_SOUND = 14; /* reads the object value; nothing plays it */
constexpr int FLIC_FADE_WHITE = 15;
constexpr int FLIC_PRELOAD = 24; /* keeps the event's frame in memory */

/* A frame read ahead. */
struct FlicBlock {
    i16 kind;
    u8 *data;
};
/* A frame FLIC_PRELOAD keeps in memory. */
struct FlicPreload {
    i16 frame;
    u8 *data;
};
/* The event table: FFFFh ends it. */
struct FlicEvent {
    u16 frame;
    u16 type;
    u16 value;
};
ASSERT_SIZE(FlicBlock, 0x6);
ASSERT_SIZE(FlicPreload, 0x6);
ASSERT_SIZE(FlicEvent, 0x6);

class FlicPlayer : public PeriodicProcess {
public:
    explicit FlicPlayer(Loading loading) : PeriodicProcess(loading) {}
    i16 x;
    i16 y;
    u8 unknown44;
    u8 hasAudio;     /* the flic's header says "AVI" */
    u8 *soundBuffer; /* a 64K ring behind a sound effect head */
    u16 unknown4a;
    u16 unknown4c;
    u8 *soundWrite;
    Viewport *backdrop; /* what was under the flic, in grey */
    i16 bandsPerTick;
    i16 frameBands; /* of the frame being drawn */
    FlicBlock blocks[FLIC_BLOCKS];
    u8 readAhead;
    u8 applyPalette;
    u8 restoreScreen;
    u8 screenSaved;
    i16 band;
    u16 nextPreload;
    i16 eventIndex;
    u8 showBorder;
    char drawIndex;
    Viewport *background;
    u8 fading;
    u16 fadeRate;
    FastFlex *flex;
    u32 startTime;
    u32 unknown93;
    i16 eventFrame; /* the frame the events are at */
    u16 frameTicks;
    Palette *greyPalette;
    u8 greyScreen;
    FlexEntry entry;
    FlicPreload preloads[FLIC_PRELOADS];
    char readIndex;
    i16 loops;
    s32 frameCount;
    u16 savedMusic;
    Palette *savedPalette;
    u32 savedMode;
    u16 eventValue;
    FlicEvent events[FLIC_EVENTS];
    Palette *palette;
    u8 pauseCycling;
    u8 *frameData;
    u8 raisePriority;
    u8 soundStarted;
    u16 frame;
    u8 frameLoaded;
    u8 restore;
    u8 seekNeeded;
    u32 playStart;
    i16 state;
    u8 restoreMusic;
    u8 stopEffects;
    u32 framesDrawn;
    u32 videoMode;
    u16 skipTicks;
    u8 unknown755;
    u8 keepMouse;

    /* UNCALLED: nothing calls it. */
    FlicPlayer(char *name, i16 screenX, i16 screenY);
    void run() override;
    void onKill() override; /* the original's destructor */
    void event(u32 event, EventData data) override;
    /* Sets up the screen, the palettes and the frames read ahead. */
    /* UNCALLED: nothing calls it. */
    void start();
    /* UNCALLED: nothing calls it. */
    void stop();
};
ASSERT_SIZE(FlicPlayer, 0x757);

/* Terminates after ticks timer events at rate, or when a key is pressed. */
class FlicWait : public PeriodicProcess {
public:
    explicit FlicWait(Loading loading) : PeriodicProcess(loading) {}
    u16 count; /* the ticks left */

    FlicWait(u16 ticks, u16 newRate);
    void run() override { terminate(0); }
    void event(u32 event, EventData data) override;
};
ASSERT_SIZE(FlicWait, 0x42);

/* Brings the game screen back after a flic. */
class FlicRestoreProcess : public Process {
public:
    explicit FlicRestoreProcess(Loading loading) : Process(loading) {}
    u8 animationsHeld; /* to restore */
    u8 greyScreen;

    FlicRestoreProcess(u8 held, u8 grey);
    void run() override;
};
ASSERT_SIZE(FlicRestoreProcess, 0x34);

/* getVideoMode() of VGA mode 13h: the mode in the low word, the 8-line font
 * in the high word. */
constexpr s32 MODE_13H = (static_cast<s32>(0x80013L));

/* A flic runs. */
extern u8 &flicPlaying;
/* The video mode the flics play in (getVideoMode()): in No Remorse
 * crusader.cfg's videosize big or large and bigvideo pick mode 13h; in No
 * Regret nothing writes it, so the mode 13h paths are not taken. No
 * Remorse's PLAYER.C reads it far. */
extern u32 &flicVideoMode;
/* The screen is greyed before a flic; in No Remorse the usecode's movie
 * overlay clears it (ITEM.C). */
extern u8 &flicGreyScreen;
#endif
