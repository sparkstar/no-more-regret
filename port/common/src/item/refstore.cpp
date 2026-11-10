/* ..\\ITEM\\REFSTORE.C (Code34, 1108:0000-0414): put-off hit events. */
#include <cstdlib>
#include "refstore.h"
#include "item.h"
#include "kernel.h"

StorageData::StorageData(RefStore *newStore, i16 newKind, u16 processItem, u16 newHitter, i16 newForce)
{
    nameProcess(pid, "StorageData");
    setType(STORAGE_TYPE);
    setItem(processItem);
    this->kind = newKind;
    index = -1;
    this->store = newStore;
    this->hitter = newHitter;
    this->force = newForce;
    removed = 0;
}

void StorageData::run()
{
    Item target(item);

    if (currentPid() == pid) {
        halt();
    }
    if (kind == STORE_GOT_HIT) {
        target.gotHit(hitter, force, nullptr);
        return;
    }
    if (kind == STORE_HIT) {
        target.hit(hitter, force);
        return;
    }
    halt();
}

void StorageData::onKill()
{
    if (removed == 0U) {
        store->remove(item);
    }
}

RefStore::RefStore(i16 storeSize)
{
    this->size = storeSize;
    count = 0;
    if (storeSize != 0) {
        entries = std::make_unique_for_overwrite<StorageData *[]>(storeSize);
        if (entries == nullptr) {
            halt();
        }
        memset(static_cast<void *>(entries.get()), 0, storeSize * sizeof(StorageData *));
    }
}

void RefStore::add(StorageData *data)
{
    entries[count] = data;
    data->setIndex(count++);
}

void RefStore::remove(u16 item) const
{
    i16 i;

    for (i = 0; i < size; i++) {
        StorageData *data = entries[i];
        if ((data != nullptr) && data->item == item) {
            entries[i] = nullptr;
            if (data->hasFlags(PROCESS_KILLED) == 0U) {
                data->removed = 1;
                data->terminate(0);
            }
        }
    }
    for (i = 0; i < size; i++) {
        StorageData *data = entries[i];
        if ((data != nullptr) && data->hitter == item) {
            entries[i] = nullptr;
            data->removed = 1;
            data->terminate(0);
        }
    }
}

void RefStore::flush() const
{
    for (i16 i = 0; i < size; i++) {
        StorageData *data = entries[i];
        if (data != nullptr) {
            data->run();
            if (entries[i] != nullptr) {
                entries[i] = nullptr;
                data->removed = 1;
                data->terminate(0);
            }
        }
    }
}

/* entries frees itself. */
RefStore::~RefStore() = default;
