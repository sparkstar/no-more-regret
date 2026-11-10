/* ..\\ITEM\\TYPE.C (Code35, 1110:0000-0408): the shapes' type flags and
 * names. */
#include <cstring>
#include "gamestate.h"
#include "type.h"
#include "file.h"
#include "filespec.h"
#include "gamefile.h"
#include "utility.h"

TypeFlags &typeFlags = gameGlobal<TypeFlags>();

void ShapeInfo::reset()
{
    family = 0;
    x = 1;
    y = 1;
    z = 0;
    fixed = 0;
    solid = 1;
    sea = 0;
    land = 0;
    damaging = 0;
    noisy = 0;
    draw = 0;
    ignore = 0;
    occludes = 0;
    bag = 0;
    roof = 0;
    translucent = 0;
    editor = 0;
    selectable = 0;
    preload = 0;
    sound = 0;
    targetable = 0;
    npc = 0;
    animType = 0;
    animData = 0;
    animSpeed = 0;
    weight = 0;
    volume = 0;
    equipType = 0;
}

void TypeFlags::init()
{
    infos = nullptr;
    names = nullptr;
    loadInfos();
    loadNames();
}

void TypeFlags::close()
{
    if (infos != nullptr) {
        delete[] infos;
        infos = nullptr;
    }
    if (names != nullptr) {
        delete[] names;
        names = nullptr;
    }
}

void TypeFlags::loadInfos()
{
    char path[80];

    if (infos == nullptr && (infos = new ShapeInfo[SHAPES]) == nullptr) {
        allocationFailure();
    }
    strcpy(path, makePath(nullptr, staticDirectory, gameFiles[GAME_TYPEFLAG], nullptr));
    const File file(path, FILE_READWRITE, 0);
    file.read(infos, static_cast<ui16>(SHAPES * sizeof(ShapeInfo)), 0);
}

ShapeInfo TypeFlags::info(i16 shape) const
{
    return infos[shape];
}

void TypeFlags::loadNames()
{
    char path[80];

    if (names == nullptr && (names = new char[SHAPES - 1][32]) == nullptr) {
        allocationFailure();
    }
    strcpy(path, makePath(nullptr, editorDirectory, "typename.dat", nullptr));
    if (fileExists(path) != 0) {
        const File file(path, FILE_READWRITE, 0);
        /* The file starts with the name of shape 0. */
        file.read(names, static_cast<ui16>((SHAPES - 1) * sizeof(*names)), sizeof(*names));
    }
}

/* UNCALLED: nothing calls it. */
char *TypeFlags::name(i16 shape) const
{
    return (shape != 0) ? names[static_cast<i16>(shape - 1)] : nullptr;
}

/* UNCALLED: nothing calls it. */
i16 TypeFlags::find(char *text, u8 lower, i16 start) const
{
    if (lower != 0U) {
        strlwr(text);
    }
    for (i16 shape = start; shape < SHAPES; shape++) {
        if (lower != 0U) {
            char buffer[80];
            strcpy(buffer, names[static_cast<i16>(shape - 1)]);
            strlwr(buffer);
            if (strstr(buffer, text) != nullptr) {
                return shape;
            }
        } else if (strcmp(text, names[static_cast<i16>(shape - 1)]) == 0) {
            return shape;
        }
    }
    return -1;
}

/* The family of a shape (1110:03ec, intrinsic 147). */
i16 familyOfType(i16 shape)
{
    return typeFlags.infos[shape].family;
}
