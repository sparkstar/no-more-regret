/* FILE\FILEPARS.C (1248:0000-06c1): parsing a buffer in memory. See
 * filepars.h. */
#include <cstdlib>
#include "common.h"
#include "file.h"
#include "filepars.h"

/* UNCALLED: nothing calls it. */
Parser::Parser(void *source, u32 blockSize)
{
    if (source == nullptr) {
        halt();
    }
    /* The limit of the block's selector: one less than its size. */
    size = blockSize - 1;
    if (size > static_cast<s32>(0xfc00L)) {
        halt();
    }
    data = std::make_unique_for_overwrite<char[]>(size);
    if (data == nullptr) {
        allocationFailure();
    }
    memcpy(data.get(), source, size);
    cursor = data.get();
}

/* data frees itself. */
Parser::~Parser() = default;

void Parser::skipSpace()
{
    while (isSpace() && (atEnd() == 0U)) {
        cursor++;
    }
}

void Parser::readWord(char *word)
{
    u16 length = 0;

    skipSpace();
    while (!isSpace() && (atEnd() == 0U)) {
        word[length] = *cursor++;
        length++;
    }
    word[length] = 0;
}

void Parser::readUntil(char delimiter, char *text)
{
    u16 length = 0;

    while ((atEnd() == 0U) && *cursor != delimiter) {
        text[length] = *cursor++;
        length++;
    }
    text[length] = 0;
}

void Parser::skipPast(char delimiter)
{
    while (*cursor != delimiter && (atEnd() == 0U)) {
        cursor++;
    }
    if (atEnd() == 0U) {
        cursor++;
    }
}

/* UNCALLED: nothing calls it. */
void Parser::skipWord()
{
    skipSpace();
    while (!isSpace() && (atEnd() == 0U)) {
        cursor++;
    }
}

void Parser::skipLine()
{
    skipPast('\n');
}

/* UNCALLED: nothing calls it. */
s32 Parser::readNumber()
{
    u16 length = 0;
    char text[40];

    skipSpace();
    while (isDigit() && (atEnd() == 0U)) {
        text[length] = *cursor++;
        length++;
    }
    text[length] = 0;
    if (text[0] != 0) {
        return atoi(text);
    }
    return 0;
}

u8 Parser::readByte()
{
    if (atEnd() == 0U) {
        return *cursor++;
    }
    return 0;
}

/* UNCALLED: nothing calls it. */
u16 Parser::readShort()
{
    if (static_cast<u32>(static_cast<i16>(static_cast<i16>(cursor - data.get()) + 1)) < size) {
        return *shortCursor++;
    }
    return 0;
}

/* The original compares the whole far pointer, its selector too, with the
 * size: past any size, so it reads nothing and returns 0. Nothing calls
 * it. */
/* UNCALLED: nothing calls it. */
u32 Parser::readLong()
{
    return 0;
}

/* UNCALLED: nothing calls it. */
void Parser::readQuoted(char *text)
{
    u16 length;

    skipSpace();
    if (*cursor == '"' && (atEnd() == 0U)) {
        length = 0;
        cursor++;
        while (*cursor != '"' && (atEnd() == 0U)) {
            text[length] = *cursor++;
            length++;
        }
        text[length] = 0;
    }
}

FileParser::FileParser(const char *path) : file(path, FILE_READ, 0)
{
    size = file.size();
    if (size > static_cast<s32>(0xfc00L)) {
        halt();
    }
    data = std::make_unique_for_overwrite<char[]>(size + 1);
    if (data == nullptr) {
        allocationFailure();
    }
    file.read(data.get(), size, -1L);
    cursor = data.get();
}
