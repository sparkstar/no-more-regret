/* See waits.h. */
#include "waits.h"
#include "gamestate.h"

#include <vector>

namespace {

/* The waits and the rests, the top last. */
std::vector<std::unique_ptr<GameRest>> &frames = gameGlobal<std::vector<std::unique_ptr<GameRest>>>();
/* A wait was opened and the calls return to the step; the rests they give
 * go in at frames[restPlace], under those given before. */
bool &opening = gameGlobal<bool>(false);
std::size_t &restPlace = gameGlobal<std::size_t>(0);
/* The result of the call the next rest comes after. */
s32 &result = gameGlobal<s32>(0);
/* How many loops of the original's that wait where they are hold the
 * thread (waitsBlocked). */
u16 &blocked = gameGlobal<u16>(0);

/* Whether a wait was opened since the step last looked, which it then
 * forgets. */
bool takeOpening()
{
    const bool was = opening;
    opening = false;
    return was;
}

}

bool waitOpened()
{
    return opening;
}

bool waitsOpen()
{
    return opening || !frames.empty() || blocked != 0U;
}

void openWait(std::unique_ptr<GameWait> wait, std::unique_ptr<GameRest> rest)
{
    if (opening) {
        halt();
    }
    if (blocked != 0U) {
        while (!wait->over()) {
            wait->pass();
        }
        static_cast<void>(wait->run());
        wait.reset();
        if (rest != nullptr) {
            static_cast<void>(rest->run());
        }
        return;
    }
    restPlace = frames.size();
    if (rest != nullptr) {
        frames.push_back(std::move(rest));
    }
    frames.push_back(std::move(wait));
    opening = true;
}

void giveRestFrame(std::unique_ptr<GameRest> rest)
{
    if (!opening) {
        halt();
    }
    frames.insert(frames.begin() + static_cast<std::ptrdiff_t>(restPlace), std::move(rest));
}

void ClockWait::pass()
{
    clockWait();
    setOver();
}

void openClockWaitFrame(std::unique_ptr<GameRest> rest)
{
    openWait(std::make_unique<ClockWait>(), std::move(rest));
}

void afterWait(std::unique_ptr<GameRest> rest)
{
    if (rest == nullptr) {
        return;
    }
    if (opening) {
        giveRestFrame(std::move(rest));
        return;
    }
    static_cast<void>(rest->run());
}

void expectNoWait()
{
    if (opening) {
        halt();
    }
}

s32 restResult()
{
    return result;
}

void setRestResult(s32 value)
{
    result = value;
}

bool waitsBlocked()
{
    return blocked != 0U;
}

void blockWaits()
{
    blocked++;
}

void unblockWaits()
{
    blocked--;
}

void runGameStep(StepBase base)
{
    for (;;) {
        if (frames.empty()) {
            base();
            if (!takeOpening()) {
                return;
            }
            continue;
        }
        GameWait *wait = frames.back()->wait();
        if (wait != nullptr && !wait->over()) {
            wait->pass();
            if (!takeOpening()) {
                return;
            }
            continue;
        }
        const std::unique_ptr<GameRest> rest = std::move(frames.back());
        frames.pop_back();
        const bool passed = rest->run();
        if (takeOpening()) {
            continue;
        }
        if (passed) {
            return;
        }
    }
}

void releaseWaits()
{
    /* What the frames' ends open waits for (a movie's sound) runs where it
     * is. */
    blocked = 1;
    while (!frames.empty()) {
        frames.pop_back();
    }
    frames.shrink_to_fit();
    opening = false;
    restPlace = 0;
    result = 0;
    blocked = 0;
}
