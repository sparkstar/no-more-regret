# Translations

The files that show the games' text in another language. There is Korean (`ko`): both games' terminals and movie
subtitles.

## Files

- `<language>/<game>/usecode.jsonl`: the usecode's text (the terminals, memos and e-mails)
- `<language>/<game>/subtitles.jsonl`: the movies' subtitles, a line for each subtitle
- `ko/GLOSSARY.md`, `ko/STORY.md`: the Korean translation's terms and each character's way of speaking, the two
  games' stories and who speaks in each movie (in Korean)

Each line is one JSON object:

```jsonl
{"hash": "8d0d60fca6497fe6", "text": "우선 레지스탕스에 온 걸 환영하네, 대위.", "state": "draft"}
```

| Field | Meaning |
|---|---|
| `hash` | The original text's FNV-1a 64-bit hash, 16 hexadecimal digits: the key. A text found in several places takes the one line everywhere |
| `text` | The translation, in UTF-8 |
| `state`, `note` | The translator's own; the generator does not read them |

The original texts come from the game's files, which are not in the repository. These write them to
`build/text/<game>_usecode_text.json` and `build/text/<game>_subtitles.json`, each text with its hash, its places
and where it shows:

```sh
python3 port/tools/usecode_text.py                           # No Regret's usecode text
CRUSADER_GAME=noremorse python3 port/tools/usecode_text.py   # No Remorse's
python3 port/tools/subtitle_text.py                          # the subtitles
```

Translate only the texts whose `use` is `terminal`. A text the screen never shows (a movie's name, say) breaks
the game when it changes.

## Writing a translation

- **Marks:** keep the original's. `^` is a new line, `*` a short pause (not shown).
- **Lines:** leave out the spaces the original added to fill its 20 columns; the terminal breaks the lines by the
  letters' widths.
- **Pieces:** where the original splits a text to put a number in it or to build a list, translate each piece.
  They join in the original's order, so put the pieces' ends where the language's word order needs them. A piece
  several texts share (`sameText`) must read right in all of them.
- **Numbers:** in Korean the particle after a number depends on the number (`4512가`, `4513이`), so no particle
  goes right after one.
- **Letters:** printable ASCII and the 2,350 Hangul syllables of KS X 1001 (`../common/fonts/ko.png`). The
  generator stops on any other letter (`똠`, curly quotes, `…`).
- **Subtitles:** each shows at the original's frame. Where the original splits a sentence over two subtitles, so
  does the translation. A line holds about 60 Hangul letters.

## Into the build

```sh
python3 port/tools/make_text.py           # writes the font and the translations as C++
python3 port/tools/make_text.py --check   # fails when they differ from what it would write (ctest's text)
```

The files it writes, `common/src/text/hangul_glyphs.cpp` and `<game>/src/text/translations.cpp`, are not edited by
hand: after a change to a translation, run the generator and commit its files too. To play in Korean, put
`language=ko` in the game folder's `enhanced.cfg`.
