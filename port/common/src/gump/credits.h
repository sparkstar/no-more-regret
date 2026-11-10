/* Code123 (file name unknown): the credits. static\credits.dat
 * is a script of lines drawn with two fonts over static\cred.dat, with fades
 * of static\cred.pal. */
#ifndef PORT_CREDITS_H
#define PORT_CREDITS_H
#include "common.h"
#include "waits.h"

/* The credits until the script ends or a key is pressed; in No Remorse,
 * with the cheats on, the end plays the movie z03. The original's
 * showCredits as a wait: the credits run a turn a step; code (a lambda as
 * waits.h's giveRest's) after their end. */
void openCreditsFrame(std::unique_ptr<GameRest> rest);
template <typename Code> void openCredits(Code code)
{
    openCreditsFrame(std::make_unique<CodeRest<Code>>(std::move(code)));
}
#endif
