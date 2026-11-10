/* No Regret's values of the kernel (common/src/kernel/kernel.cpp). */
#ifndef PORT_KERNEL_GAME_H
#define PORT_KERNEL_GAME_H

/* The key queue's events. */
inline constexpr int KEY_QUEUE = 48;
/* The key queue's room: KEY_QUEUE here; the one program's build takes the larger of
 * the two games'. */
inline constexpr int KEY_QUEUE_CAPACITY = KEY_QUEUE;
/* The items the kernel keeps maps of processes and types for: all of them. */
inline constexpr int KERNEL_ITEMS = 0x2800; /* ITEMS */
#endif
