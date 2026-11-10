/* The searches of the usecode's loops on its stack. OP_LOOP copies its
 * search onto the thread's stack, where OP_LOOP_NEXT moves it on and the
 * usecode drops it, as many bytes as BCC gave the search's class, when the
 * loop ends; a saved game holds it there when a thread stops in a loop. The
 * port keeps the stack's bytes as the original's (its vtable the DS offset
 * of the class's vtable that INTER.C's inline constructors made) and moves
 * the search on through a search of the port's read from them. */
#ifndef PORT_USESEARCH_H
#define PORT_USESEARCH_H
#include "common.h"
#include "itemfind.h"

/* The bytes BCC gave a search of OP_LOOP's kind (inter.cpp's LOOP_*). */
u16 searchImageSize(i16 kind);
/* Writes a search of the kind as BCC laid it out. */
void putSearchImage(u8 *image, const ItemSearch *search, i16 kind);
/* Reads a search into one of the port's of the image's class; false when
 * the image is of another. */
bool getSearchImage(const u8 *image, ItemSearch *search);
/* Moves the search at image on to its next item (ItemSearch::next) and
 * returns that item, 0 at the end. */
u16 nextSearchImage(u8 *image);
#endif
