/* AMFLOAD.C (12e0:02f0-0526): music modules. See amfload.h. */
#include "amfload.h"
#include "file.h"

void freeAmf(AmfModule *module)
{
    i16 i;

    if (module->header != nullptr) {
        delete module->header;
        module->header = nullptr;
    }
    if (module->patterns != nullptr) {
        delete[] module->patterns;
        module->patterns = nullptr;
    }
    for (i = 0; i < AMF_SAMPLES; i++) {
        if (module->samples[i] != nullptr) {
            delete[] module->samples[i];
            module->samples[i] = nullptr;
        }
    }
}

bool loadAmf(AmfModule *module, const char *path)
{
    u8 *sample;
    i16 i;
    File file(path, FILE_READ, 0);

    module->header = new AmfHeader;
    memset(module->header, 0, sizeof(AmfHeader));
    file.read(module->header, sizeof(AmfHeader));
    if (memcmp(module->header->signature, "ASYLUM", 6) != 0) {
        haltMessage("Invalid AMF format.");
    }
    module->patterns = new u8[static_cast<u32>(module->header->patterns) * AMF_PATTERN_SIZE];
    u8 *patterns = module->patterns;
    file.read(patterns, module->header->patterns * static_cast<s32>(AMF_PATTERN_SIZE));
    for (i = 0; i < AMF_SAMPLES; i++) {
        if (module->header->sample[i].length != 0U) {
            sample = new u8[module->header->sample[i].length];
            module->samples[i] = sample;
            file.read(sample, module->header->sample[i].length);
        }
    }
    file.close();
    return true;
}
