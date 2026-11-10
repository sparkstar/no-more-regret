/* AMFLOAD.C (12e0:02f0-0526): reads the music modules ASYLUM.DLL plays
 * (Asylum Music Format). */
#ifndef PORT_AMFLOAD_H
#define PORT_AMFLOAD_H
#include "common.h"

constexpr int AMF_SAMPLES = 64;
/* 64 rows of 8 channels of 4-byte notes. */
constexpr int AMF_PATTERN_SIZE = 0x800;

/* A sample header of an AMF file. */
struct __attribute__((packed)) AmfSample {
    char name[22];
    u8 finetune;
    u8 volume;
    char transpose;
    u32 length;
    u32 loopStart;
    u32 loopLength;
};
ASSERT_FILE_SIZE(AmfSample, 0x25);
/* The header of an AMF file. */
struct __attribute__((packed)) AmfHeader {
    char signature[32]; /* "ASYLUM Music Format V1.0" */
    u8 speed;
    u8 tempo;
    u8 samples;
    u8 patterns;
    u8 orders;
    u8 restart;
    u8 order[256];
    AmfSample sample[AMF_SAMPLES];
};
ASSERT_FILE_SIZE(AmfHeader, 0xa66);

/* A music module in memory. */
class AmfModule {
public:
    AmfHeader *header;
    u8 *patterns;
    u8 *samples[AMF_SAMPLES];
};
ASSERT_SIZE(AmfModule, 0x108);

/* Reads the module in path; halts when it is not AMF. */
bool loadAmf(AmfModule *module, const char *path);
/* Frees what loadAmf allocated. */
void freeAmf(AmfModule *module);
#endif
