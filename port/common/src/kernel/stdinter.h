/* STDINT.C (1208:0000-0340): the standard interrupt handler, the process
 * that polls the event processes subscribed to it (Process::subscribe) and
 * hands each event they report to them. */
#ifndef PORT_STDINT_H
#define PORT_STDINT_H
#include "common.h"
#include "kprocess.h"
#include "sublist.h"

class StdIntHandler;
extern StdIntHandler *&stdIntHandler;

class StdIntHandler : public Process {
public:
    explicit StdIntHandler(Loading loading) : Process(loading) {}
    SubscriberList subscribers;
    SubscriberList spare;

    StdIntHandler();
    void run() override;
    void cleanup() override;
    void load(File *file) override;
    void save(File *file) override;
    void afterLoad() override { stdIntHandler = this; }
};
ASSERT_SIZE(StdIntHandler, 0x50);
#endif
