/* No Remorse's values of the kernel (common/src/kernel/kernel.cpp). */
#ifndef PORT_KERNEL_GAME_H
#define PORT_KERNEL_GAME_H

/* The key queue's events (its kernel compares with 20h and 1Fh, KeyDaemon::event
 * with 10h). */
inline constexpr int KEY_QUEUE = 32;
/* The key queue's room: KEY_QUEUE here; the one program's build takes the larger of
 * the two games'. */
inline constexpr int KEY_QUEUE_CAPACITY = KEY_QUEUE;
/* The items the kernel keeps maps of processes and types for: the first 0x2400
 * (480h bytes of bits at 11d0's initKernel), while the item data has ITEMS. */
inline constexpr int KERNEL_ITEMS = 0x2400;
#endif
