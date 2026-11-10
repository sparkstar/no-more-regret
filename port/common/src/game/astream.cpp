/* ASTREAM.C (Code48, 1178:040f-0602): the animations in memory: the table
 * of a shape's animations, and the frames of each animation. */
#include "astream.h"

AnimInfo::AnimInfo()
{
    memset(this, 0, sizeof(*this));
}

void AnimAction::allocate()
{
    release();
    if (static_cast<u8>(info.frames) == 0) {
        halt();
    }
    frameList = new AnimFrame[size() / sizeof(AnimFrame)];
}

u16 AnimAction::size() const
{
    i16 directions;
    if (info.sixteenDirections) {
        directions = 16;
    } else {
        directions = 8;
    }
    return directions * sizeof(AnimFrame) * info.frames;
}

/* UNCALLED: nothing calls it. */
void AnimAction::setFrames(u8 frames)
{
    info.frames = frames;
    allocate();
}

AnimFrame *AnimAction::frameAt(char direction, u8 index) const
{
    i16 sixteen;
    if ((sixteen = info.sixteenDirections) == 0) {
        direction >>= 1;
    }
    if (direction <= 7 || (sixteen != 0)) {
        return frameList + static_cast<ui16>(direction * info.frames) + index;
    }
    return nullptr;
}

/* UNCALLED: nothing calls it. */
void AnimAction::setFrame(char direction, u8 index, AnimFrame *frame) const
{
    if (!info.sixteenDirections) {
        direction >>= 1;
    }
    if (static_cast<ui16>(index) >= info.frames) {
        halt();
    }
    (frameList + static_cast<ui16>(info.frames * direction) + index)->copy(frame);
}

void AnimAction::release()
{
    if (frameList != nullptr) {
        delete[] frameList;
        frameList = nullptr;
    }
}
