/* ASYLUM.DLL segment 1, the player: the music's tick (1809h), its rows
 * (14E3h) and effects (129Ch), and the sound effects' starts (199Fh), as
 * the hand-written original does them. See asylib.h.
 *
 * The periods' divisions take EBX whole where the code has set only BX
 * (the portamentos, 1329h and 135Fh): their high word is what the code
 * before left there, from a row, a sound effect's start, or the fill's
 * caller. playerEbxHigh carries it. */
#include "asylib.h"
#include "gamestate.h"

#include <cstdio>

namespace asylum {

u16 &playerEbxHigh = gameGlobal<u16>(0);

namespace {

const u32 CLOCK = 0xda7a68;

/* The step of a period: (CLOCK / period << 16) / the DSP's rate, both
 * divisions by EBX, whose high word is high. */
u32 stepOf(u16 period, u16 high)
{
    const u32 step = divide32(CLOCK, static_cast<u32>(high) << 16 | period) << 16;
    return divide32(step, static_cast<u32>(high) << 16 | data.dspRate);
}

/* A dword of the tables (the periods' table by a note past it, 1:1566h,
 * reads past it into the config). */
u32 dwordAt(u16 offset)
{
    return data.tables.dword(offset);
}

/* 1:1452 the volume slide of dl: the low nibble down, the high one up. */
void volumeSlide(Channel &c, u8 param)
{
    const u8 down = param & 0xf;
    const u8 up = param >> 4;
    u8 volume = static_cast<s8>(c.volume) >= static_cast<s8>(down) ? static_cast<u8>(c.volume - down) : 0;
    volume = static_cast<u8>(volume + up);
    c.volume = volume > 0x40 ? 0x40 : volume;
}

/* 1:138B the tone portamento towards +21h by speed * 4. */
void tonePortamento(Channel &c, u8 speed)
{
    const u16 step = static_cast<u16>(speed << 2);
    const u16 target = c.portaTarget;
    u16 period = c.period;
    playerEbxHigh = 0;
    if (period == target) {
        return;
    }
    if (static_cast<s16>(period) > static_cast<s16>(target)) {
        period = static_cast<u16>(period - step);
        if (static_cast<s16>(period) < static_cast<s16>(target)) {
            period = target;
        }
    } else {
        period = static_cast<u16>(period + step);
        if (static_cast<s16>(period) > static_cast<s16>(target)) {
            period = target;
        }
    }
    c.period = period;
    c.step = stepOf(period, 0);
}

/* 1:13D8 the vibrato of param (speed, depth) on the step, not the period. */
void vibrato(Channel &c, u8 param)
{
    const u8 depth = param & 0xf;
    c.vibratoPosition = static_cast<u8>(c.vibratoPosition + ((param >> 4) << 2));
    const u8 position = c.vibratoPosition;
    const u16 product = static_cast<u16>(VIBRATO_SINE[(position >> 2) & 0x1f] * depth);
    const u16 rolled = static_cast<u16>(product << 1 | product >> 15);
    u16 offset = static_cast<u16>((rolled >> 8 & 0xff) | (rolled & 1) << 8);
    if (static_cast<s8>(position) < 0) {
        offset = static_cast<u16>(-offset);
    }
    u16 period = static_cast<u16>(offset + c.period);
    playerEbxHigh = 0;
    if (static_cast<s16>(period) < 0x71) {
        period = 0x71;
    }
    if (static_cast<s16>(period) > 0x6b00) {
        period = 0x6b00;
    }
    c.step = stepOf(period, 0);
}

/* 1:12EC the retrigger: counts down +3Dh, then plays from the start. */
void retrigger(Channel &c)
{
    if (c.counter != 0 && --c.counter != 0) {
        return;
    }
    c.position = 0;
    c.counter = c.counterReload;
}

/* 1:129C the effects of a tick. */
void tickEffects(Channel &c)
{
    const u16 effect = c.effect;
    if (effect == 0U) {
        return;
    }
    const u8 command = static_cast<u8>(effect >> 8);
    const u8 param = static_cast<u8>(effect);
    switch (command) {
    case 0: { /* 130Bh arpeggio */
        u16 index = static_cast<u16>(c.arpeggioIndex);
        playerEbxHigh = static_cast<u16>(c.arpeggioIndex >> 16);
        c.step = c.arpeggio[index >> 2];
        index = static_cast<u16>(index + 4);
        if (index < 0xc) {
            c.arpeggioIndex = static_cast<u32>(playerEbxHigh) << 16 | index;
        } else {
            c.arpeggioIndex = 0;
            playerEbxHigh = 0;
        }
        return;
    }
    case 1: { /* 1329h portamento up */
        u16 period = static_cast<u16>(c.period - static_cast<u16>(param << 2));
        if (static_cast<s16>(period) < 0x71) {
            period = 0x71;
        }
        c.period = period;
        c.step = stepOf(period, playerEbxHigh);
        return;
    }
    case 2: { /* 135Fh portamento down */
        const u16 period = static_cast<u16>(c.period + static_cast<u16>(param << 2));
        c.period = period;
        c.step = stepOf(period, playerEbxHigh);
        return;
    }
    case 3:
        tonePortamento(c, param);
        return;
    case 4:
        vibrato(c, param);
        return;
    case 5: /* 1441h */
        volumeSlide(c, param);
        tonePortamento(c, c.portaSpeed);
        return;
    case 6: /* 144Ah */
        volumeSlide(c, param);
        vibrato(c, c.vibrato);
        return;
    case 0xa:
        volumeSlide(c, param);
        return;
    case 0xe: { /* 1473h */
        const u16 shifted = static_cast<u16>(effect << 4);
        const u8 sub = static_cast<u8>(shifted >> 8) & 0xf;
        const u8 value = static_cast<u8>(shifted);
        switch (sub) {
        case 0xc: /* 1496h cut */
            if (c.counter != 0 && --c.counter != 0) {
                return;
            }
            c.volume = 0;
            return;
        case 9:
            retrigger(c);
            return;
        case 0xa: { /* 14C6h fine volume up, once */
            const u8 volume = static_cast<u8>(c.volume + ((value >> 4) & 0xf));
            c.volume = volume > 0x40 ? 0x40 : volume;
            c.effect = 0xeff;
            return;
        }
        case 0xb: { /* 14ABh fine volume down, once */
            const u8 down = (value >> 4) & 0xf;
            c.volume = static_cast<s8>(c.volume) >= static_cast<s8>(down) ? static_cast<u8>(c.volume - down) : 0;
            c.effect = 0xeff;
            return;
        }
        }
        return;
    }
    case 0x1b:
        retrigger(c);
        return;
    }
}

/* 1:1726 the arpeggio of a row: the steps of the period and of the notes
 * hi and lo semitones up, from the period table where the period falls. */
void arpeggio(Channel &c, u8 param)
{
    const u16 hi = param >> 4;
    const u16 lo = param & 0xf;
    const u32 product = c.period * c.c2spd;
    u16 at = 0;
    for (u16 k = 0; k < 0x60; k++, at = static_cast<u16>(at + 4)) {
        if (product >= dwordAt(static_cast<u16>(PERIODS_AT + at))) {
            break;
        }
    }
    u16 high = static_cast<u16>(c.c2spd >> 16);
    auto stepAt = [&](u16 offset) {
        const u16 period = static_cast<u16>(divide32(dwordAt(static_cast<u16>(PERIODS_AT + offset)), c.c2spd));
        return stepOf(period, high);
    };
    c.arpeggio[0] = stepAt(at);
    c.arpeggio[1] = stepAt(static_cast<u16>((hi << 2) + at));
    c.arpeggio[2] = stepAt(static_cast<u16>((lo << 2) + at));
    playerEbxHigh = high;
    c.arpeggioIndex = 0;
}

/* 1:14E3 a row's note of the channel at pattern: its instrument, its note
 * (the period from the table and the instrument's C2 speed; the position,
 * not the fraction, back to 0) and its effect. */
void readRow(Channel &c, const u8 *pattern)
{
    const u8 note = pattern[0];
    const u8 instrument = pattern[1];
    const u16 effect = static_cast<u16>(pattern[2] << 8 | pattern[3]);
    c.effect = effect;
    u16 cx = note;
    if (instrument > INSTRUMENTS) {
        /* Past the song's 31 the original reads the other tables, which
         * the game's songs never make it do: the channel quiet. */
        char text[80];
        std::snprintf(text, sizeof(text), "ASYLUM.DLL's instrument %u, past its %u: the channel quiet", instrument,
                      INSTRUMENTS);
        passOver(Trouble::INSTRUMENT, text);
        c.active = 0;
    } else if (instrument != 0U) {
        const u16 i = static_cast<u16>(instrument - 1);
        c.volume = static_cast<u8>(data.volumes[i]);
        c.data = Samples{nullptr, nullptr, data.sampleData[i], data.sampleSizes[i], 0};
        c.length = data.sampleLengths[i];
        c.loopStart = data.loopStarts[i];
        c.loopLength = data.loopLengths[i];
        c.transpose = data.transposes[i];
        c.c2spd = data.instrumentC2spds[i];
        c.active = 1;
    }
    if (cx != 0U) {
        cx = static_cast<u16>(cx + c.transpose);
        cx = static_cast<u16>(divide32(dwordAt(static_cast<u16>(PERIODS_AT + static_cast<u16>(cx << 2))), c.c2spd));
        playerEbxHigh = static_cast<u16>(c.c2spd >> 16);
        c.portaTarget = cx;
        if ((effect >> 8) != 3) {
            c.period = cx;
            c.step = stepOf(cx, 0);
            playerEbxHigh = 0;
            c.position = 0;
        }
    }
    if (effect == 0U) {
        return;
    }
    const u8 command = static_cast<u8>(effect >> 8);
    u8 param = static_cast<u8>(effect);
    switch (command) {
    case 0:
        arpeggio(c, param);
        return;
    case 3: /* 163Dh */
        if (param == 0U) {
            param = c.portaSpeed;
        }
        c.portaSpeed = param;
        c.effect = static_cast<u16>((effect & 0xff00) | param);
        return;
    case 4: { /* 164Dh */
        const u8 old = c.vibrato;
        if ((param & 0xf) == 0) {
            param |= old & 0xf;
        }
        if ((param & 0xf0) == 0) {
            param |= old & 0xf0;
        }
        c.vibrato = param;
        c.effect = static_cast<u16>((effect & 0xff00) | param);
        if (cx != 0U) {
            c.vibratoPosition = 0;
        }
        return;
    }
    case 9: { /* 167Ah */
        if (param == 0U) {
            param = c.sampleOffset;
        }
        c.sampleOffset = param;
        s32 offset = static_cast<s32>(param) << 8;
        if (offset >= static_cast<s32>(c.length)) {
            offset = static_cast<s32>(c.length) - 1;
        }
        c.position = static_cast<u32>(offset);
        return;
    }
    case 0xb: /* 16A5h */
        data.order = param;
        data.row = 0x40;
        return;
    case 0xc: /* 16D1h */
        c.volume = param > 0x40 ? 0x40 : param;
        return;
    case 0xd: /* 16DEh */
        data.breakRow = static_cast<u8>((param & 0xf) + ((param >> 4) * 10));
        data.row = 0x40;
        return;
    case 0xf: /* 16F9h */
        if (param == 0U) {
            return;
        }
        if (param <= 0x1f) {
            data.speed = param;
            data.speedCount = param;
            return;
        }
        data.tempo = param;
        data.tickSamples = divide16(data.tickRate, static_cast<u16>((0x66 * param) >> 8));
        return;
    case 0xe: { /* 1613h */
        const u16 shifted = static_cast<u16>(effect << 4);
        const u8 sub = static_cast<u8>(shifted >> 8) & 0xf;
        const u8 value = static_cast<u8>(static_cast<u8>(shifted) >> 4);
        switch (sub) {
        case 0xc:
            c.counter = value;
            return;
        case 9:
            c.counterReload = value;
            c.counter = value;
            return;
        case 8: /* 16AFh the pan */
            c.mixer = panMixer(value);
            setPan(value);
            c.panRight = data.panRight;
            c.panLeft = data.panLeft;
            c.musicPanRight = c.panRight;
            c.musicPanLeft = c.panLeft;
            return;
        }
        return;
    }
    case 0x1b: /* 15FBh */
        if (param == 0U) {
            c.counter = c.counterReload;
            return;
        }
        param &= 0xf;
        c.counterReload = param;
        c.counter = param;
        return;
    }
}

/* 1:1921 a sound effect channel's request at its tick: state 1 starts it
 * (the pans the volume for this tick, the fraction left as it was), 2 goes
 * on at the record's volume, centred, or ends with the channel. */
void startSound(SfxRecord &record, Channel &c)
{
    if (record.state == 0) {
        c.length = 0;
        return;
    }
    if (record.state == 1) {
        record.state = 2;
        c.length = record.length;
        c.loopStart = record.loopStart;
        c.loopLength = record.loopLength;
        c.step = record.step;
        playerEbxHigh = static_cast<u16>(record.step >> 16);
        c.active = 1;
        c.position = 0;
        c.volume = record.volume;
        c.panRight = record.volume;
        c.panLeft = record.volume;
        c.data = record.data;
        return;
    }
    c.volume = record.volume;
    c.panRight = 0x40;
    c.panLeft = 0x40;
    if (c.active == 0U) {
        record.state = 0;
    }
}

}

/* 1:1809 the music's tick: the effects, and at the speed's end a row. */
void musicTick()
{
    if (--data.speedCount != 0) {
        for (u16 i = 0; i < MUSIC_CHANNELS; i++) {
            tickEffects(data.channels[i]);
        }
        return;
    }
    data.speedCount = data.speed;
    u32 at = data.patternOffset;
    const u8 *block = data.patternBlock;
    if (data.row >= 0x40) {
        at = 0;
        block = data.patterns;
        u8 order = data.order;
        if (order >= data.songLength) {
            order = data.restart;
            data.order = order;
            if (order >= data.songLength) {
                playerEbxHigh = 0;
                return;
            }
        }
        at += static_cast<u32>(data.orders[order]) << 11;
        data.row = data.breakRow;
        data.breakRow = 0;
        playerEbxHigh = 0;
        data.patternOffset = at;
        data.patternBlock = block;
        data.order++;
    }
    data.row++;
    for (u16 i = 0; i < MUSIC_CHANNELS; i++, at += 4) {
        readRow(data.channels[i],
                Samples{nullptr, nullptr, block, (block != nullptr) ? data.patternsSize : 0, 0}.at(at, 4));
    }
    data.patternOffset = at;
    for (u16 i = 0; i < MUSIC_CHANNELS; i++) {
        tickEffects(data.channels[i]);
    }
}

/* 1:199F */
void startSounds()
{
    for (u16 i = 0; i < SFX_CHANNELS; i++) {
        startSound(data.sfx[i], data.channels[MUSIC_CHANNELS + i]);
    }
}

}
