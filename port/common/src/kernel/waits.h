/* The game's waits as state (the port's). Where the original waits in a
 * loop of its own (a modal gump's, a movie's, a fade's) and then goes on,
 * the port opens a wait and returns; the game's steps (gamemain.cpp's
 * gameStep, runGameStep) run a pass of the wait at a time, and when it is
 * over what came after it, in the original's order.
 *
 * What came after is kept on the port's own stack of the calls, the rests:
 * the code that opens a wait gives its rest with it; each call above it
 * whose code goes on after the call that returned gives its rest too
 * (waitOpened, giveRest), and returns. The rests lie under the wait, the
 * innermost on top, and run in that order when it is over; a rest that
 * opens a wait again puts that wait and its own rests above the others. A
 * rest gets the result of the call it comes after (restResult), which the
 * wait or the rest before it set.
 *
 * Inside a loop of the original's that waits where it is (a modal gump
 * run by runModalGump rather than as a wait), a wait is run there to its end and its
 * rest after it (waitsBlocked, openWait): the calls above it go on as they
 * did.
 *
 * The waits and rests are the game's memory (gamememory.h), so that a
 * snapshot holds them; gameRelease frees them (releaseWaits). */
#ifndef PORT_WAITS_H
#define PORT_WAITS_H
#include "common.h"
#include "clock.h"

#include <memory>
#include <utility>

class GameWait;

/* What comes after a call: run once, when the waits above it are over. */
class GameRest {
public:
    GameRest() = default;
    virtual ~GameRest() = default;
    GameRest(const GameRest &) = delete;
    GameRest &operator=(const GameRest &) = delete;
    GameRest(GameRest &&) = delete;
    GameRest &operator=(GameRest &&) = delete;
    /* Runs the rest; true when it ended in a pass of the game's time (the
     * kernel's pass, a modal gump's), which ends the step. */
    [[nodiscard]] virtual bool run() = 0;
    /* The wait this is, or none. */
    [[nodiscard]] virtual GameWait *wait() { return nullptr; }
};

/* A loop of the original's that waits, a pass at a time; its run is what
 * the call that waited did after its loop, which sets the call's result. */
class GameWait : public GameRest {
public:
    /* A pass of the loop, ending in a pass of the game's time; over once the
     * original's loop would have ended. */
    virtual void pass() = 0;
    [[nodiscard]] bool over() const { return over_; }
    [[nodiscard]] GameWait *wait() override { return this; }

protected:
    void setOver() { over_ = true; }

private:
    bool over_ = false;
};

/* A rest written where its code is, as a lambda that returns whether it
 * ended in a pass of the game's time. */
template <typename Code> class CodeRest final : public GameRest {
public:
    explicit CodeRest(Code code) : code_(std::move(code)) {}
    [[nodiscard]] bool run() override { return code_(); }

private:
    Code code_;
};

/* A wait of the original's that waits for the clock's next tick or input
 * (clockWait), once: its pass is the clockWait. */
class ClockWait final : public GameWait {
public:
    void pass() override;
    [[nodiscard]] bool run() override { return false; }
};

/* A wait of the original's for some milliseconds (Borland's delay,
 * clockDelay), once: its pass is the delay. */
class DelayWait final : public GameWait {
public:
    explicit DelayWait(unsigned milliseconds) : milliseconds_(milliseconds) {}
    void pass() override
    {
        clockDelay(milliseconds_);
        setOver();
    }
    [[nodiscard]] bool run() override { return false; }

private:
    unsigned milliseconds_;
};

/* A loop of the original's that waits for the clock while a condition
 * holds (while (holds()) clockWait();): its pass is a clockWait, over when
 * the condition no longer holds. */
template <typename Holds> class WhileWait final : public GameWait {
public:
    explicit WhileWait(Holds holds) : holds_(std::move(holds)) {}
    void pass() override
    {
        clockWait();
        if (!holds_()) {
            setOver();
        }
    }
    [[nodiscard]] bool run() override { return false; }

private:
    Holds holds_;
};

/* A loop of the original's whose turn lets time pass (while (holds())
 * turn();): its pass is a turn, over when the condition no longer holds
 * after it. */
template <typename Holds, typename Turn> class LoopWait final : public GameWait {
public:
    LoopWait(Holds holds, Turn turn) : holds_(std::move(holds)), turn_(std::move(turn)) {}
    void pass() override
    {
        turn_();
        if (!holds_()) {
            setOver();
        }
    }
    [[nodiscard]] bool run() override { return false; }

private:
    Holds holds_;
    Turn turn_;
};

/* A wait was opened and the calls return to the step, each giving its rest
 * on the way. */
[[nodiscard]] bool waitOpened();
/* A wait or a rest is there, or one is opening, or a loop waits where it
 * is: the game is inside one of its calls (a modal gump, a delay, a usecode
 * thread inside an instruction), which a save cannot hold. */
[[nodiscard]] bool waitsOpen();
/* Opens wait, its caller's rest (or none) under it; the calls return from
 * here. Inside a loop that waits where it is (waitsBlocked), the wait runs
 * here to its end and the rest after it. */
void openWait(std::unique_ptr<GameWait> wait, std::unique_ptr<GameRest> rest);
/* While a wait was opened: the rest of a call above it, under the rests
 * given before. */
void giveRestFrame(std::unique_ptr<GameRest> rest);
template <typename Code> void giveRest(Code code)
{
    giveRestFrame(std::make_unique<CodeRest<Code>>(std::move(code)));
}
/* A clockWait as a wait, code (a lambda as giveRest's) after it; inside a
 * loop that waits where it is, the clockWait and code at once. */
void openClockWaitFrame(std::unique_ptr<GameRest> rest);
template <typename Code> void openClockWait(Code code)
{
    openClockWaitFrame(std::make_unique<CodeRest<Code>>(std::move(code)));
}
/* clockDelay as a wait, code (a lambda as giveRest's) after it. */
template <typename Code> void openClockDelay(unsigned milliseconds, Code code)
{
    openWait(std::make_unique<DelayWait>(milliseconds), std::make_unique<CodeRest<Code>>(std::move(code)));
}
/* The original's while (holds()) clockWait(); then the rest: a wait when
 * the condition holds, else the rest at once. */
template <typename Holds> void openWaitWhileFrame(Holds holds, std::unique_ptr<GameRest> rest)
{
    if (!holds()) {
        if (rest != nullptr) {
            static_cast<void>(rest->run());
        }
        return;
    }
    openWait(std::make_unique<WhileWait<Holds>>(std::move(holds)), std::move(rest));
}
/* The original's while (holds()) turn(); then the rest (or none): a wait
 * when the condition holds, else the rest at once. */
template <typename Holds, typename Turn> void openLoopFrame(Holds holds, Turn turn, std::unique_ptr<GameRest> rest)
{
    if (!holds()) {
        if (rest != nullptr) {
            static_cast<void>(rest->run());
        }
        return;
    }
    openWait(std::make_unique<LoopWait<Holds, Turn>>(std::move(holds), std::move(turn)), std::move(rest));
}
/* The same with code (a lambda as giveRest's) for the rest. */
template <typename Holds, typename Code> void openWaitWhile(Holds holds, Code code)
{
    openWaitWhileFrame(std::move(holds), std::make_unique<CodeRest<Code>>(std::move(code)));
}
/* The same with code (a lambda as giveRest's) for the rest. */
template <typename Holds, typename Turn, typename Code> void openLoop(Holds holds, Turn turn, Code code)
{
    openLoopFrame(std::move(holds), std::move(turn), std::make_unique<CodeRest<Code>>(std::move(code)));
}
/* After code that may have opened a wait: rest (or none) under the wait,
 * else at once. */
void afterWait(std::unique_ptr<GameRest> rest);
/* After a call whose waits the game's data rules out (the reason at the
 * call): halts if one was opened. */
void expectNoWait();
/* The same with code, a lambda that returns nothing. */
template <typename Code> void whenDone(Code code)
{
    if (waitOpened()) {
        giveRest([code = std::move(code)]() mutable {
            code();
            return false;
        });
        return;
    }
    code();
}
/* The result of the call a rest comes after. */
[[nodiscard]] s32 restResult();
void setRestResult(s32 value);
/* Inside a loop of the original's that waits where it is (blockWaits, ...
 * unblockWaits around it), where a wait runs to its end at once. */
[[nodiscard]] bool waitsBlocked();
void blockWaits();
void unblockWaits();
/* Runs wait to its end where it is: its passes, then its own end. The
 * calls name the wait's own functions (Wait, a final class), which the
 * static comparison of calls follows. */
template <typename Wait> void runWaitHere(Wait &wait)
{
    blockWaits();
    while (!wait.over()) {
        wait.Wait::pass();
    }
    unblockWaits();
    static_cast<void>(wait.Wait::run());
}

/* A step of the game: the waits' passes and the rests while there are any,
 * else base (the start's or the kernel's pass), until a pass of the game's
 * time has been run. */
using StepBase = void (*)();
void runGameStep(StepBase base);
/* Frees the waits and rests left at the run's end (gameRelease), the top
 * first. */
void releaseWaits();
#endif
