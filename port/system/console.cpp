/* Borland's conio for the port, the host's text screen (host.h): the text
 * screen of the start (CRUSYS.C's title) on the standard output. The current
 * row is kept as 80 columns, which hostTextAt moves along; a new row writes it
 * out without its trailing spaces, and empty rows are left out. Colours have
 * no place on the output and are dropped; the mode goes to video.h. The
 * game's printf (hostPrint) goes to the same output: the game writes with it
 * only after a hostTextAt, which writes the row out first. */
#include "conio.h"
#include "gamememory.h"
#include "host.h"
#include "video.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>

namespace {

const int COLUMNS = 80;

std::string row(COLUMNS, ' ');
int rowNumber = 1;
int column = 0;
bool written = false;

void writeRow()
{
    if (written) {
        std::string text = row.substr(0, row.find_last_not_of(' ') + 1);
        std::fwrite(text.data(), 1, text.size(), stdout);
        std::fputc('\n', stdout);
    }
    row.assign(COLUMNS, ' ');
    written = false;
}

}

void hostClearText()
{
    const SystemSide systemSide;
    writeRow();
    rowNumber = 1;
    column = 0;
}

void hostTextAt(int x, int y)
{
    const SystemSide systemSide;
    if (y != rowNumber) {
        writeRow();
        rowNumber = y;
    }
    column = x - 1;
}

int hostPrintText(const char *format, ...)
{
    const SystemSide systemSide;
    char text[1024];
    va_list arguments;
    va_start(arguments, format);
    const int length = std::vsnprintf(text, sizeof(text), format, arguments);
    va_end(arguments);
    for (const char *c = text; *c != 0; c++) {
        if (*c == '\r') {
            column = 0;
        } else if (*c == '\n') {
            writeRow();
            rowNumber++;
        } else {
            if (column >= COLUMNS) {
                writeRow();
                rowNumber++;
                column = 0;
            }
            row[column++] = *c;
            written = true;
        }
    }
    return length;
}

int hostPrint(const char *format, ...)
{
    const SystemSide systemSide;
    va_list arguments;
    va_start(arguments, format);
    const int length = std::vprintf(format, arguments);
    va_end(arguments);
    return length;
}

void hostTextColor(int /*unused*/)
{
    const SystemSide systemSide;
}

void hostTextBackground(int /*unused*/)
{
    const SystemSide systemSide;
}

/* C4350 is 80x50, with the 8-line font; the others 80x25. */
void hostTextMode(int mode)
{
    const SystemSide systemSide;
    setTextMode(mode == C4350);
    hostClearText();
}
