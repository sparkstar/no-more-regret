/* The TASM module after SubscriberList (1208:0438-0504, intctl.asm):
 * interrupt vectors and the 8259 interrupt controllers. */
#ifndef PORT_INTCTL_H
#define PORT_INTCTL_H
#include "common.h"

extern "C" {
/* INT 21h AH=25h and 35h. */
void setVector(u8 vector, void *handler);
void *getVector(u8 vector);
/* Masks and unmasks interrupt line irq (0-15) at its controller. */
void maskIrq(u8 irq);
void unmaskIrq(u8 irq);
/* The end of interrupt for irq. */
void endInterrupt(u8 irq);
/* The unmasked lines, the slave controller's in the high byte. */
u16 irqMask();
}
#endif
