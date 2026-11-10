/* The replay's and the capture's pure parts (port/system/replay.h, capture.h, nut.h): NUT's numbers, checksum,
 * packets and frame headers; the capture's samples, its resampling to
 * 44100 a second (whole or in pieces, the same) and its picture of a
 * screen; inputs.tsv's lines both ways and their mistakes; the times of
 * --from and --to; the days; the debug features' line of enhanced.cfg and
 * the options that are debug features (commandline.h); the end's message
 * box (report.h). A replay of the game itself is replay.py's. */
#include "capture.h"
#include "commandline.h"
#include "nut.h"
#include "replay.h"
#include "report.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

namespace {

int checks = 0, failures = 0;

void check(bool ok, const std::string &what)
{
    checks++;
    if (!ok) {
        failures++;
        std::printf("MISMATCH replay: %s\n", what.c_str());
    }
}

std::string hex(const std::vector<std::uint8_t> &bytes)
{
    std::string text;
    char two[4];
    for (std::uint8_t byte : bytes) {
        std::snprintf(two, sizeof two, "%02x", byte);
        text += two;
    }
    return text;
}

void nutPieces()
{
    struct Case {
        std::uint64_t value;
        const char *bytes;
    };
    const Case varints[] = {{0, "00"},         {127, "7f"},       {128, "8100"},
                            {32767, "81ff7f"}, {44100, "82d844"}, {65536, "848000"}};
    for (const Case &c : varints) {
        check(hex(nut::varint(c.value)) == c.bytes,
              "varint " + std::to_string(c.value) + ": " + hex(nut::varint(c.value)));
    }
    /* CRC-32 of 04C11DB7h from 0, no final xor: the check value of
     * CRC-32/POSIX without its xor. */
    const char *digits = "123456789";
    check(nut::checksum(reinterpret_cast<const std::uint8_t *>(digits), 9, 0) == 0x89A1897Fu, "checksum of 1-9");
    std::vector<std::uint8_t> payload{1, 2, 3};
    std::vector<std::uint8_t> packet = nut::packet(0x4E4B000000000000ull, payload);
    check(hex(packet).substr(0, 16) == "4e4b000000000000" && packet[8] == 7, "packet's start code and size");
    check(nut::checksum(packet.data() + 9, packet.size() - 9, 0) == 0, "packet's checksum makes 0");
    std::vector<std::uint8_t> big(5000, 0x55);
    std::vector<std::uint8_t> bigPacket = nut::packet(0x4E4B000000000000ull, big);
    /* The start code, the size in two bytes and their checksum. */
    check(nut::checksum(bigPacket.data(), 14, 0) == 0, "a long packet's header checksum");
    std::vector<std::uint8_t> headers = nut::headers({640, 480, 60, 44100, 2});
    check(std::memcmp(headers.data(), "nut/multimedia container\0NM", 27) == 0, "the file's start");
    std::vector<std::uint8_t> frame = nut::frameHeader(nut::AUDIO, 735, 2940);
    /* Frame code 1, stream 1, the time 735 + 128, the size 2940, a checksum. */
    check(frame.size() == 10 && hex(frame).substr(0, 12) == "0101865f967c", "frame header " + hex(frame));
    check(nut::checksum(frame.data(), frame.size(), 0) == 0, "frame header's checksum");
}

void samples()
{
    const std::uint8_t eight[] = {0x80, 0xff, 0x00, 0x81};
    std::vector<i16> fromEight = captureSamples(eight, 4, false);
    check(fromEight == std::vector<i16>{0, 32512, -32768, 256}, "8-bit samples");
    const std::uint8_t sixteen[] = {0x34, 0x12, 0xff, 0xff};
    check(captureSamples(sixteen, 4, true) == std::vector<i16>{0x1234, -1}, "16-bit samples");

    /* 44100 a second as it is. */
    Resampler same;
    same.rate = 44100;
    std::vector<i16> input{1, 2, 3, 4, 5, 6};
    Resampled made = resample(same, input);
    check(made.output == input && made.state.out == 3, "44100 kept");

    /* 22050: every other one between two; the last waits for the next. */
    Resampler half;
    half.rate = 22050;
    made = resample(half, {0, 0, 100, 200, 200, 400});
    check(made.output == std::vector<i16>{0, 0, 50, 100, 100, 200, 150, 300, 200, 400}, "22050 doubled");
    made = resample(made.state, {300, 600});
    check(made.output == std::vector<i16>{250, 500, 300, 600}, "22050 on");

    /* In pieces or at once, the same; pieces of an odd size too, which end
     * between a pair's two samples as the output's do (audio.cpp). */
    for (u32 rate : {11025u, 22050u, 32000u, 48000u}) {
        std::vector<i16> all;
        for (int k = 0; k < 2000; k++) {
            all.push_back((i16)(k * 37 % 2000 - 1000));
        }
        Resampler whole;
        whole.rate = rate;
        Resampled atOnce = resample(whole, all);
        for (const std::size_t size : {6U, 5U, 1U}) {
            Resampler piece;
            piece.rate = rate;
            std::vector<i16> pieces;
            for (std::size_t k = 0; k < all.size(); k += size) {
                Resampled step =
                    resample(piece, std::vector<i16>(all.begin() + (std::ptrdiff_t)k,
                                                     all.begin() + (std::ptrdiff_t)std::min(k + size, all.size())));
                piece = step.state;
                pieces.insert(pieces.end(), step.output.begin(), step.output.end());
            }
            check(pieces == atOnce.output && piece.in == 1000,
                  "in pieces of " + std::to_string(size) + " at " + std::to_string(rate) + ": " +
                      std::to_string(pieces.size()) + " samples, " + std::to_string(piece.in) + " pairs in");
        }
        /* Up to the place of the last input: 999 x 44100 / rate, and the
         * one at 0. */
        std::uint64_t expected = (1000 - 1) * (std::uint64_t)CAPTURE_RATE / rate + 1;
        check(atOnce.state.out == expected,
              "samples out at " + std::to_string(rate) + ": " + std::to_string(atOnce.state.out));
        check(atOnce.state.input.size() <= 4, "input kept at " + std::to_string(rate));
    }
}

void pictures()
{
    std::vector<u8> black = capturePicture(nullptr, 640);
    check(black.size() == 640u * 480 * 3 && std::all_of(black.begin(), black.end(), [](u8 b) { return b == 0; }),
          "a text mode black");

    std::vector<u8> pixels(640 * 480);
    pixels[0] = 1;
    pixels[479 * 640 + 639] = 2;
    ScreenView view{pixels.data(), 640, 480, 640, 8, {}};
    view.palette[3] = 10;
    view.palette[4] = 20;
    view.palette[5] = 30;
    view.palette[6] = 255;
    std::vector<u8> rgb = capturePicture(&view, 640);
    check(rgb[0] == 10 && rgb[1] == 20 && rgb[2] == 30, "640x480's first pixel");
    check(rgb[rgb.size() - 3] == 255 && rgb[rgb.size() - 2] == 0, "640x480's last pixel");

    std::vector<u8> small(320 * 200);
    small[199 * 320 + 319] = 1;
    ScreenView mode13{small.data(), 320, 200, 320, 8, {}};
    mode13.palette[3] = 7;
    rgb = capturePicture(&mode13, 640);
    check(rgb[rgb.size() - 3] == 7 && rgb[(478 * 640 + 638) * 3] == 7 && rgb[(477 * 640 + 638) * 3] == 0,
          "320x200 stretched to 640x480");

    std::vector<u8> words(640 * 480 * 2);
    words[0] = 0x00;
    words[1] = 0xf8; /* red */
    ScreenView movie{words.data(), 640, 480, 1280, 16, {}};
    rgb = capturePicture(&movie, 640);
    check(rgb[0] == 255 && rgb[1] == 0 && rgb[2] == 0, "16-bit red");

    /* The wide screen (854 for 480 rows, 107 columns each side): with its
     * sides as it is, dimmed at half; a 4:3 screen in its middle, black
     * beside it; with the sides into a 640 frame, the middle alone. */
    const u16 wide = 854;
    const u16 side = (wide - 640) / 2;
    std::vector<u8> widePixels((std::size_t)wide * 480, 1);
    widePixels[side] = 2;
    ScreenView sides{widePixels.data(), wide, 480, wide, 8, {}};
    sides.palette[3] = 200;
    sides.palette[6] = 100;
    sides.frameWidth = wide;
    sides.sides = side;
    rgb = capturePicture(&sides, wide);
    check(rgb.size() == (std::size_t)wide * 480 * 3 && rgb[0] == 200 && rgb[side * 3] == 100,
          "the wide screen with its sides");
    sides.dimmed = true;
    rgb = capturePicture(&sides, wide);
    check(rgb[0] == 100 && rgb[(wide - 1) * 3] == 100 && rgb[side * 3] == 100 && rgb[(side + 1) * 3] == 200,
          "the wide screen's sides dimmed");
    sides.dimmed = false;
    rgb = capturePicture(&sides, 640);
    check(rgb.size() == 640u * 480 * 3 && rgb[0] == 100 && rgb[3] == 200, "the wide screen's middle in a 640 frame");
    rgb = capturePicture(&view, wide);
    check(rgb[0] == 0 && rgb[(side - 1) * 3] == 0 && rgb[side * 3] == 10 && rgb[(side + 640) * 3] == 0 &&
              rgb[((479 * wide) + side + 639) * 3] == 255,
          "a 4:3 screen in the wide frame's middle");
}

ReplayInput input(ReplayInput::Kind kind)
{
    ReplayInput made;
    made.frame = 1234;
    made.kind = kind;
    return made;
}

bool same(const ReplayInput &a, const ReplayInput &b)
{
    return a.frame == b.frame && a.kind == b.kind && a.scan == b.scan && a.x == b.x && a.y == b.y &&
           a.buttons == b.buttons && a.device.kind == b.device.kind && a.device.which == b.device.which &&
           a.device.value == b.device.value && a.date.year == b.date.year && a.date.month == b.date.month &&
           a.date.day == b.date.day && a.date.weekday == b.date.weekday && a.pass == b.pass && a.until == b.until;
}

void lines()
{
    std::vector<std::pair<ReplayInput, std::string>> cases;
    ReplayInput key = input(ReplayInput::KEY);
    key.scan = 0x1c;
    cases.push_back({key, "1234\tkey\t1c"});
    key.scan = 0x9c;
    cases.push_back({key, "1234\trelease\t1c"});
    key.scan = 0xe0;
    cases.push_back({key, "1234\tkey\te0"});
    ReplayInput motion = input(ReplayInput::MOTION);
    motion.x = -3;
    motion.y = 12;
    motion.buttons = 5;
    cases.push_back({motion, "1234\tmotion\t-3\t12\t5"});
    ReplayInput device = input(ReplayInput::DEVICE);
    device.device = {DeviceKind::PAD_AXIS, 1, -1200};
    cases.push_back({device, "1234\taxis\tlefty\t-1200"});
    device.device = {DeviceKind::PAD_BUTTON, 0, 1};
    cases.push_back({device, "1234\tpad\ta\t1"});
    device.device = {DeviceKind::WHEEL, 0, -2};
    cases.push_back({device, "1234\twheel\t-2"});
    device.device = {DeviceKind::MOUSE_BUTTON, 2, 0};
    cases.push_back({device, "1234\tmousebutton\tx2\t0"});
    device.device = {DeviceKind::PAD_GONE, 0, 0};
    cases.push_back({device, "1234\tpadgone"});
    cases.push_back({input(ReplayInput::CLOSE), "1234\tclose"});
    cases.push_back({input(ReplayInput::END), "1234\tend"});
    ReplayInput date = input(ReplayInput::DATE);
    date.date = *clockDateOf(2026, 10, 3);
    cases.push_back({date, "1234\tdate\t2026-10-03"});
    ReplayInput pass = input(ReplayInput::PASS);
    pass.pass = 304;
    pass.until = 14201653;
    cases.push_back({pass, "1234\tpass\t304\t14201653"});
    for (const auto &[made, text] : cases) {
        check(replayLine(made) == text, "line " + text + ": " + replayLine(made));
        ReplayLine read = readReplayLine(text);
        check(read.what == ReplayLine::INPUT && same(read.input, made), "read back " + text);
    }

    ReplayLine spaced = readReplayLine("  7  key 1c   # Enter");
    check(spaced.what == ReplayLine::INPUT && spaced.input.frame == 7 && spaced.input.scan == 0x1c,
          "spaces and a comment");
    check(readReplayLine("").what == ReplayLine::NOTHING &&
              readReplayLine("# frame\tinput").what == ReplayLine::NOTHING,
          "blank and comment lines");
    for (const char *wrong : {"x key 1c", "5", "5 key", "5 key 100", "5 key 1c 2a", "5 release 80", "5 motion 1 2",
                              "5 motion 1 2 8", "5 date 2026-02-30", "5 date 2026-1-1x", "5 pass 0 1", "5 end 1",
                              "5 pad z 1", "5 axis leftx 40000", "5 nonsense", "-5 key 1c"}) {
        check(readReplayLine(wrong).what == ReplayLine::MISTAKE, std::string("a mistake: ") + wrong);
    }
}

void times()
{
    struct Case {
        const char *text;
        std::optional<std::uint64_t> frame;
    };
    const Case cases[] = {
        {"90", 90},   {"0", 0},     {"1:30", 5400}, {"0:01.5", 90}, {"1:00:00", 216000}, {"2:03:04.25", 443055},
        {"1:60", {}}, {"", {}},     {"a", {}},      {"-1", {}},     {"1:2:3:4", {}},     {":30", {}},
        {"1:-3", {}}, {"1:1e1", {}}};
    for (const Case &c : cases) {
        check(replayFrameOf(c.text) == c.frame, std::string("time ") + c.text);
    }
}

void days()
{
    std::optional<ClockDate> day = clockDateOf(2026, 10, 3);
    check(day && day->weekday == 6, "2026-10-03 a Saturday");
    day = clockDateOf(1996, 1, 1);
    check(day && day->weekday == 1, "1996-01-01 a Monday");
    day = clockDateOf(2024, 2, 29);
    check(day && day->weekday == 4, "2024-02-29 a Thursday");
    check(!clockDateOf(2023, 2, 29) && !clockDateOf(2026, 13, 1) && !clockDateOf(2026, 0, 1), "no such days");
}

/* enhanced.cfg's debug_feature=: true or 1 in any case, spaces round it,
 * the last line's; another value, a comment, none: off. The options that
 * need the debug features, the first named; --game, --version and the
 * game's own do not. */
void debugFeatures()
{
    check(debugFeatureLine("debug_feature=true\r\n"), "debug_feature=true");
    check(debugFeatureLine("game=regret\r\n Debug_Feature = TRUE \r\nkeys=modern\r\n"), "its key in any case, spaces");
    check(debugFeatureLine("debug_feature=1"), "debug_feature=1, no line end");
    check(!debugFeatureLine("debug_feature=yes\r\n"), "debug_feature=yes: off");
    check(!debugFeatureLine("# debug_feature=true\r\n"), "a comment: off");
    check(!debugFeatureLine("debug_feature=true\r\ndebug_feature=false\r\n"), "the last line's");
    check(!debugFeatureLine(""), "no file's text: off");
    check(debugOption({"-warp", "3", "--replay", "x", "--record", "y"}) == "--replay", "the first, --replay");
    for (const char *option :
         {"--record", "--replay", "--continue", "--watch", "--capture", "--from", "--to", "--keep-files"}) {
        check(debugOption({option}) == option, std::string(option) + ": a debug feature");
    }
    check(debugOption({"--game", "regret", "--version", "-skill", "2", "--debug-feature"}).empty(),
          "--game, --version, the game's own: none");
}

/* The message box at the end: that the game stopped, and where the report
 * is; nothing of why (the report's). */
void endMessage()
{
    check(endMessageText(nullptr) == "The game stopped because of an error.", "no report");
    check(endMessageText("") == "The game stopped because of an error.", "an empty path: no report");
    check(endMessageText("/games/regret/regret-crash-1.log") ==
              "The game stopped because of an error.\n\nA report of it is in\n/games/regret/regret-crash-1.log",
          "the report's place: " + endMessageText("/games/regret/regret-crash-1.log"));
}

}

int main()
{
    nutPieces();
    samples();
    pictures();
    lines();
    times();
    days();
    debugFeatures();
    endMessage();
    std::printf("replay: %d checks, %d mismatches\n", checks, failures);
    return failures ? 1 : 0;
}
