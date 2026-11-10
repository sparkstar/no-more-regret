/* FILE\FILEPARS.C (Code74, 1248): reads text and binary values from a whole
 * file (or a copy of a memory block) held in memory. */
#ifndef PORT_FILEPARS_H
#define PORT_FILEPARS_H
#include <dos.h>
#include "common.h"
#include "file.h"

#include <memory>

class Parser {
public:
    /* The copy of the text, its own. */
    std::unique_ptr<char[]> data;
    /* The read position; readShort and readLong step it as a pointer to
     * their type. */
    union {
        char *cursor;
        u16 *shortCursor;
        u32 *longCursor;
    };
    u32 size;
    Parser()
    {
        cursor = nullptr;
        size = 0;
    }
    /* Copies the whole block of source's selector (at most FC00h bytes): the
     * port's pointers carry no selector, and blockSize is the block's. */
    /* UNCALLED: nothing calls it. */
    Parser(void *source, u32 blockSize); /* 1248:0000 */
    ~Parser();                           /* 1248:00e6 */
    [[nodiscard]] bool isSpace() const
    {
        return *cursor == ' ' || *cursor == '\n' || *cursor == '\r' || *cursor == '\t';
    }
    /* UNCALLED: nothing calls it. */
    [[nodiscard]] bool isDigit() const { return *cursor >= '0' && *cursor <= '9'; }
    /* The original compares the cursor's offset, which counts from data: a
     * block has a selector of its own. */
    [[nodiscard]] u8 atEnd() const { return static_cast<u32>(cursor - data.get()) >= size ? 1 : 0; }
    void skipSpace(); /* 1248:012e */
    /* Copies the next word, up to a space, into word. */
    void readWord(char *word);                  /* 1248:019a */
    void readUntil(char delimiter, char *text); /* 1248:0240 */
    /* Moves past the next delimiter. */
    void skipPast(char delimiter); /* 1248:02a7 */
    /* UNCALLED: nothing calls it. */
    void skipWord(); /* 1248:0309 */
    void skipLine(); /* 1248:0384 */
    /* UNCALLED: nothing calls it. */
    s32 readNumber(); /* 1248:0398 */
    u8 readByte();    /* 1248:044c */
    /* UNCALLED: nothing calls it. */
    u16 readShort(); /* 1248:048e */
    /* UNCALLED: nothing calls it. */
    u32 readLong(); /* 1248:04c8 */
    /* Copies a "quoted" string without the quotes; nothing when the next
     * word is not quoted. */
    /* UNCALLED: nothing calls it. */
    void readQuoted(char *text); /* 1248:0509 */
};
class FileParser : public Parser {
public:
    File file;
    /* Reads the whole file (at most FC00h bytes). */
    explicit FileParser(const char *path); /* 1248:05b1 */
};
ASSERT_SIZE(Parser, 0xc);
ASSERT_SIZE(FileParser, 0x14);
#endif
