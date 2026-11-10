/* Code51 (1190, file name unknown): the members of an animation frame. */
#include "animfrm.h"

/* UNCALLED: nothing calls it. */
void AnimFrame::clear()
{
    memset(this, 0, sizeof(*this));
}

void AnimFrame::copy(AnimFrame *from)
{
    memcpy(this, from, sizeof(*this));
}

bool AnimFrame::moves() const
{
    return dx != 0 || dy != 0 || dz != 0;
}

u16 AnimFrame::sound() const
{
    u16 low = soundLow;
    const u16 high = soundHigh << 8;
    low |= high;
    return low & FRAME_SOUND_MASK;
}

/* UNCALLED: nothing calls it. */
void AnimFrame::setSound(i16 sound)
{
    soundLow = sound & 0xff;
    soundHigh = (sound & 0x300) >> 8;
}
