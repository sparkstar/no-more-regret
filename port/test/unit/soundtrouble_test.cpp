/* The sound's trouble, which the original ended with, passed over: the
 * sound never ends the game. Without the games' data: every card of
 * crusader.cfg plays the port's output; the update before a card was
 * picked puts out nothing; a slot's sound that the cache replaces by a
 * shorter one while a channel plays it reads silence past its end, and
 * the cache's stop of the slot's effects (ASS_StopSlotSFX) stops it and
 * no other; the data outside the tables reads 0 and keeps nothing
 * written; a division by 0 gives 0; a channel without a mixer is not
 * mixed; a song whose tempo gives a tick of no samples (a divide error),
 * with an instrument's finetune past 15 and a row's instrument past 31,
 * plays: the tick that of no song, the instrument untuned, the channel
 * quiet. */
#include "asylib.h"
#include "asylum.h"
#include "amfload.h"
#include "host.h"
#include "pc.h"
#include "window.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

int checks = 0;
int failures = 0;
std::vector<u8> outBytes;

void check(bool ok, const std::string &what)
{
    checks++;
    if (!ok) {
        failures++;
        std::printf("MISMATCH soundtrouble: %s\n", what.c_str());
    }
}

/* A sound in SOUND.FLX's form (signed bytes at 14h), length samples: a
 * square wave between size and -size, a half period every 25 samples (0:
 * silence). */
std::vector<u8> squareSound(u32 length, u8 size)
{
    std::vector<u8> sound(0x14 + length);
    auto put32 = [&](u32 at, u32 value) {
        for (u32 k = 0; k < 4; k++) {
            sound[at + k] = (u8)(value >> (8 * k));
        }
    };
    put32(4, length);
    put32(8, 0);
    put32(0xc, length);
    sound[0x12] = (u8)22050;
    sound[0x13] = (u8)(22050 >> 8);
    for (u32 k = 0; k < length; k++) {
        sound[0x14 + k] = (u8)((k / 25) % 2 == 0 ? size : -size);
    }
    return sound;
}

void start(u16 card)
{
    asylum::loadData(asylum::data);
    ASS_SetDepth(16);
    AssConfig config{card, 0x220, 7, 5, 22050, 1, 1};
    ASS_Init(&config);
    outBytes.clear();
}

void play(double seconds)
{
    const std::uint64_t period = PIT_HZ / 60;
    const std::uint64_t end = pcTime() + (std::uint64_t)(seconds * PIT_HZ);
    while (pcTime() < end) {
        passTime(pcTime() + period);
        ASS_UpdateDAC();
    }
}

/* The largest sample of the output's last quarter of a second (words). */
int loudestOfLast()
{
    const std::size_t last = 22050 / 4 * 4;
    int loudest = 0;
    const std::size_t from = outBytes.size() > last ? outBytes.size() - last : 0;
    for (std::size_t k = from & ~(std::size_t)1; k + 1 < outBytes.size(); k += 2) {
        loudest = std::max(loudest, std::abs((int)(i16)(outBytes[k] | (outBytes[k + 1] << 8))));
    }
    return loudest;
}

void cards()
{
    std::vector<u8> sound = squareSound(22050, 0x60);
    for (const u16 card : {0, 1, 2, 3, 4, 7}) {
        start(card);
        ASS_PlaySFX(sound.data(), 0, 64, 0, 1);
        play(0.5);
        check(ASS_GetOutputBits() == 16 && !outBytes.empty() && loudestOfLast() > 1000,
              "card " + std::to_string(card) + ": the port's output plays");
        ASS_Uninit();
    }
    asylum::loadData(asylum::data);
    outBytes.clear();
    ASS_UpdateDAC();
    check(outBytes.empty(), "the update before a card was picked: nothing");
}

void slots()
{
    /* The cache's slot: a second's sound, and the shorter one the slot
     * gets while a channel plays the first, where the original faulted). */
    std::vector<u8> first = squareSound(22050, 0x60);
    std::vector<u8> shorter = squareSound(200, 0);
    void *slot = first.data();
    u32 size = (u32)first.size();
    start(3);
    ASS_PlaySlotSFX(&slot, &size, 0, 64, 0, 0);
    play(0.1);
    check(ASS_IsChannelBusy(0) == 1 && loudestOfLast() > 1000, "the slot's sound plays");
    slot = shorter.data();
    size = (u32)shorter.size();
    play(0.5);
    check(loudestOfLast() == 0, "past the slot's shorter sound: silence");
    play(1);
    check(ASS_IsChannelBusy(0) == 0, "the channel ends at its sound's length");
    ASS_Uninit();

    std::vector<u8> other = squareSound(22050, 0x60);
    void *otherSlot = other.data();
    u32 otherSize = (u32)other.size();
    slot = first.data();
    size = (u32)first.size();
    start(3);
    ASS_PlaySlotSFX(&slot, &size, 0, 64, 0, 0);
    ASS_PlaySlotSFX(&otherSlot, &otherSize, 1, 64, 0, 0);
    play(0.1);
    ASS_StopSlotSFX(&slot);
    play(0.1);
    check(ASS_IsChannelBusy(0) == 0 && ASS_IsChannelBusy(1) == 1,
          "the slot's stop: its channel stopped, the other slot's playing");
    ASS_StopSlotSFX(&otherSlot);
    play(0.5);
    check(ASS_IsChannelBusy(1) == 0 && loudestOfLast() == 0, "the other slot's stop: silence");
    ASS_Uninit();
}

void tables()
{
    start(3);
    asylum::Tables &t = asylum::data.tables;
    t.byte(0x10) = 5;
    check(t.byte(0x10) == 0, "a byte below the tables: 0, nothing kept");
    t.setWord(static_cast<u16>(asylum::TABLES_END - 1), 0x1234);
    check(t.word(static_cast<u16>(asylum::TABLES_END - 1)) == 0, "a word across the tables' end: 0");
    check(asylum::divide32(7, 0) == 0 && asylum::divide16(7, 0) == 0 && asylum::divide32(7, 2) == 3,
          "a division by 0: 0");
    asylum::Channel channel = asylum::data.channels[0];
    channel.active = 1;
    channel.length = 100;
    asylum::mixChannel(asylum::MIX_NONE, channel, asylum::SUM_AT, 10);
    check(channel.position == asylum::data.channels[0].position, "a channel without a mixer: not mixed");
    ASS_Uninit();
}

void song()
{
    /* One pattern: channel 0 instrument 1 (its finetune 16), channel 1 the
     * instrument 40; the tempo 1, whose tick is 1 * 24 / 60 = 0. */
    std::vector<u8> bytes(sizeof(AmfHeader) + AMF_PATTERN_SIZE + 2000);
    AmfModule module{};
    module.header = reinterpret_cast<AmfHeader *>(bytes.data());
    module.patterns = bytes.data() + sizeof(AmfHeader);
    AmfHeader &header = *module.header;
    header.speed = 6;
    header.tempo = 1;
    header.samples = 2;
    header.patterns = 1;
    header.orders = 1;
    for (int i = 0; i < 2; i++) {
        header.sample[i].finetune = i == 0 ? 16 : 0;
        header.sample[i].volume = 64;
        header.sample[i].length = 1000;
        module.samples[i] = bytes.data() + sizeof(AmfHeader) + AMF_PATTERN_SIZE + (1000 * i);
        for (int k = 0; k < 1000; k++) {
            module.samples[i][k] = (u8)((k / 25) % 2 == 0 ? 0x60 : -0x60);
        }
    }
    module.patterns[0] = 40;
    module.patterns[1] = 1;
    module.patterns[4] = 40;
    module.patterns[5] = 40;
    start(3);
    ASS_PlayMusic(&module);
    ASS_StartMusic();
    check(asylum::data.tickSamples == 0, "the song's tempo: a tick of no samples");
    play(0.5);
    check(!outBytes.empty(), "the song plays (the tick of no song)");
    check(asylum::data.instrumentC2spds[0] == asylum::data.c2spds[0], "a finetune past 15: untuned");
    check(asylum::data.channels[0].length == 1000 && asylum::data.channels[1].active == 0 &&
              asylum::data.channels[1].length == 0,
          "an instrument past 31: the channel quiet, the other's instrument taken");
    ASS_Uninit();
}

}

void hostCloseAction(void (*)()) {}
bool windowRun()
{
    return false;
}
bool windowFrame()
{
    return false;
}

bool windowFastForwarding()
{
    return false;
}
bool windowFrameUnshown()
{
    return false;
}
bool windowPicture(std::vector<u8> &, int &, int &)
{
    return false;
}
void windowAudio(const u8 *bytes, u32 size, u32 /*rate*/, bool /*sixteen*/)
{
    outBytes.insert(outBytes.end(), bytes, bytes + size);
}
bool windowAudioState(WindowAudio &)
{
    return false;
}
void windowMessage(const char *) {}
void windowWarning(const char *) {}
void windowTitleNote(const std::string &) {}
void windowExit(int code)
{
    std::exit(code);
}

int main()
{
    cards();
    slots();
    tables();
    song();
    std::printf("soundtrouble: %d checks, %d mismatches\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
