/* ANIMCACH.C (Code50, 1188): the animations of the shapes (anim.dat), kept
 * in memory for ten shapes at a time; the Silencer's (shape 1) keeps the
 * first slot. */
#ifndef PORT_ANIMCACH_H
#define PORT_ANIMCACH_H
#include "common.h"
#include "animfrm.h"
#include "astream.h"
#include "animfile.h"

constexpr int ANIM_CACHE = 10;

/* Makes the cache (1188:0013) and frees it (0080). */
void initAnimCache();
void closeAnimCache();
/* The frame of anim of shape in direction at index (1188:042c). */
void getAnimFrame(u16 shape, u16 anim, u8 direction, u16 index, AnimFrame *frame);
/* The header of anim of shape (1188:04de). */
AnimInfo animInfo(u16 shape, u16 anim);
/* 1 when shape has anim, loading the shape's animations first (1188:0944). */
bool hasAnim(u16 shape, u16 anim);

/* Nothing calls these, in the original either. */
/* UNCALLED: nothing calls it. */
void unknown08d9();
/* UNCALLED: nothing calls it. */
bool hasAnims(u16 shape);
#endif
