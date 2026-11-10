/* ..\\ITEM\\REFSTORE.C (Code34, 1108:0000-0414): the hit events a move puts
 * off until it is done (Code31). Each is a StorageData process kept on a
 * RefStore; the store runs them, or drops those of an item that goes. */
#ifndef PORT_REFSTORE_H
#define PORT_REFSTORE_H
#include "common.h"
#include "kprocess.h"

#include <memory>

class RefStore;

/* The type of the StorageData processes. */
constexpr int STORAGE_TYPE = 0x236;
/* StorageData::kind: the usecode event it calls on its item. */
constexpr int STORE_HIT = 0x20b;     /* Item::hit */
constexpr int STORE_GOT_HIT = 0x20c; /* Item::gotHit */

class StorageData : public Process {
public:
    explicit StorageData(Loading loading) : Process(loading) {}
    RefStore *store;
    u16 hitter;
    i16 force;
    /* The slot in store->entries. */
    i16 index;
    i16 kind;
    /* Taken off the store already. */
    u8 removed;

    StorageData(RefStore *newStore, i16 newKind, u16 processItem, u16 newHitter, i16 newForce);
    void setIndex(i16 slot) { index = slot; }
    void run() override;
    void onKill() override; /* the original's destructor */
};
ASSERT_SIZE(StorageData, 0x3f);

class RefStore {
public:
    /* Its own. */
    std::unique_ptr<StorageData *[]> entries;
    i16 size;
    i16 count;

    explicit RefStore(i16 storeSize);
    ~RefStore();
    void add(StorageData *data);
    /* Terminates the events of item and those item was to hit. */
    void remove(u16 item) const;
    /* Runs every event now and terminates it. */
    void flush() const;
};
ASSERT_SIZE(RefStore, 8);
#endif
