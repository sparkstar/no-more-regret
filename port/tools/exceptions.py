#!/usr/bin/env python3
"""The exceptions to the port's rules, against port/tools/exceptions.tsv.

usage: exceptions.py [--write]

An exception is made in its place, with the rule's name and why:
- a compiler's warning: #pragma GCC diagnostic ignored "-W..." between a
  push and a pop, with a comment between the push and it (the why);
- the format: /* clang-format off */ after a comment (the why), closed by
  /* clang-format on */;
- clang-tidy: NOLINT, NOLINTNEXTLINE or NOLINTBEGIN with the rule in
  parentheses and the why after a colon, NOLINTBEGIN closed by NOLINTEND.
What turns a rule off for a whole file or directory is not allowed: an
ignored pragma outside a push and a pop, -Wno-..., -w or
COMPILE_WARNING_AS_ERROR OFF in the CMake files, a .clang-format or
.clang-tidy below port/.

Reads the C++ files format.py formats (common, regret, remorse, system,
platform, compat, test).
Prints the exceptions and their count by rule. Without --write the
exceptions (file, rule, why, in their order) must be the file's: a new one,
or one gone, fails (exit 1) and is named, and the file is written again with
--write in the change that makes the difference. A place without its rule's
name or why, or a rule turned off more widely, fails either way.
"""
import argparse, collections, re, sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from format import PORT, ROOT, all_files

OUT = PORT / 'tools/exceptions.tsv'
CMAKE_FILES = [PORT / 'CMakeLists.txt', PORT / 'test/CMakeLists.txt'] + sorted((PORT / 'cmake').glob('*'))
PRAGMA = re.compile(r'^\s*#\s*pragma\s+(?:GCC|clang)\s+diagnostic\s+(push|pop|ignored\s+"([^"]*)")')
FORMAT = re.compile(r'(?:/\*|//)\s*clang-format\s+(off|on)\b')
NOLINT = re.compile(r'\bNOLINT(NEXTLINE|BEGIN|END)?\b(\(([^)]*)\))?(\s*:\s*(\S.*?))?\s*(?:\*/|$)')
COMMENT = re.compile(r'/\*(.*?)\*/|//([^\n]*)', re.S)
WIDE = re.compile(r'(?<![\w-])(-Wno-[\w-]+|-w)(?![\w=-])|\bCOMPILE_WARNING_AS_ERROR\s+OFF\b')


def why(text):
    """The last comment of text, as one line; '' if none."""
    comments = [m.group(1) if m.group(1) is not None else m.group(2) for m in COMMENT.finditer(text)]
    if not comments:
        return ''
    lines = [re.sub(r'^\s*\*?\s?', '', line) for line in comments[-1].split('\n')]
    return ' '.join(' '.join(lines).split())


def scan(path):
    """([(line, rule, why)], [problems]) of a C++ file."""
    name = path.relative_to(PORT).as_posix()
    lines = path.read_text(errors='replace').split('\n')
    found, problems = [], []
    pushes = []  # line numbers (0-based) of the open pushes
    formatting = None
    tidy = []  # open NOLINTBEGIN: (line, rules)
    for i, line in enumerate(lines):
        m = PRAGMA.match(line)
        if m and m.group(1) == 'push':
            pushes.append(i)
        elif m and m.group(1) == 'pop':
            if not pushes:
                problems.append(f'{name}:{i + 1}: a pop without its push')
            else:
                pushes.pop()
        elif m:
            if not pushes:
                problems.append(f'{name}:{i + 1}: {m.group(2)} ignored outside a push and a pop (the rest of the file)')
                continue
            reason = why('\n'.join(line for line in lines[pushes[-1] + 1:i] if not PRAGMA.match(line)))
            if not reason:
                problems.append(f'{name}:{i + 1}: {m.group(2)} without a comment between the push and it (why)')
            found.append((i + 1, m.group(2), reason))
        m = FORMAT.search(line)
        if m and m.group(1) == 'off':
            before = '\n'.join(lines[:i])
            reason = why(before) if re.search(r'(\*/|//[^\n]*)\s*$', before) else ''
            if not reason:
                problems.append(f'{name}:{i + 1}: clang-format off without a comment on the line before it (why)')
            if formatting is not None:
                problems.append(f'{name}:{i + 1}: clang-format off again before on')
            formatting = i
            found.append((i + 1, 'clang-format', reason))
        elif m:
            if formatting is None:
                problems.append(f'{name}:{i + 1}: clang-format on without off')
            formatting = None
        m = NOLINT.search(line)
        if m:
            kind, rules, reason = m.group(1) or '', m.group(3) or '', m.group(5)
            if not rules:
                problems.append(f'{name}:{i + 1}: NOLINT{kind} without the rule in parentheses')
            if kind == 'END':
                if not tidy or tidy[-1][1] != rules:
                    problems.append(f'{name}:{i + 1}: NOLINTEND({rules}) without its NOLINTBEGIN')
                else:
                    tidy.pop()
                continue
            if not reason:
                problems.append(f'{name}:{i + 1}: NOLINT{kind}({rules}) without the why after a colon')
            if kind == 'BEGIN':
                tidy.append((i, rules))
            found.append((i + 1, f'NOLINT({rules})', reason or ''))
    for i in pushes:
        problems.append(f'{name}:{i + 1}: a push without its pop')
    if formatting is not None:
        problems.append(f'{name}:{formatting + 1}: clang-format off without on')
    for i, rules in tidy:
        problems.append(f'{name}:{i + 1}: NOLINTBEGIN({rules}) without NOLINTEND')
    return found, problems


def wide():
    """The places that turn a rule off for a whole file or directory."""
    problems = []
    for path in CMAKE_FILES:
        for i, line in enumerate(path.read_text().split('\n')):
            code = line.split('#', 1)[0]
            for m in WIDE.finditer(code):
                problems.append(f'{path.relative_to(PORT).as_posix()}:{i + 1}: {m.group(0)} turns warnings off widely')
    for name in ('.clang-format', '.clang-tidy'):
        for path in PORT.rglob(name):
            if path.parent != PORT:
                problems.append(f'{path.relative_to(PORT).as_posix()}: a setting of its own for a directory')
    return problems


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('--write', action='store_true')
    args = ap.parse_args()
    rows, problems = [], wide()
    for path in all_files():
        found, wrong = scan(path)
        problems += wrong
        rows += [(path.relative_to(PORT).as_posix(), line, rule, reason) for line, rule, reason in found]
    for name, line, rule, reason in rows:
        print(f'{name}:{line}: {rule}: {reason}')
    counts = collections.Counter(rule for _, _, rule, _ in rows)
    print(f'{len(rows)} exceptions' + (': ' + ', '.join(f'{rule} {n}' for rule, n in counts.most_common())
                                       if rows else ''))
    for problem in problems:
        print(f'RULE {problem}')
    table = [(name, rule, reason) for name, _, rule, reason in rows]
    if args.write:
        OUT.write_text(''.join('\t'.join(row) + '\n' for row in table))
        print(f'written {OUT.relative_to(ROOT)}')
        sys.exit(1 if problems else 0)
    kept = [tuple(line.split('\t')) for line in OUT.read_text().split('\n') if line] if OUT.exists() else []
    wrong = []
    left = collections.Counter(kept)
    for row in table:
        if left[row]:
            left[row] -= 1
        else:
            wrong.append(f'new: {" | ".join(row)}')
    wrong += [f'gone: {" | ".join(row)}' for row, n in left.items() for _ in range(n)]
    if table != kept and not wrong:
        wrong.append('the same exceptions in another order')
    for w in wrong:
        print(f'EXCEPTION {w}')
    if wrong:
        print(f'exceptions: {len(wrong)} differences from {OUT.relative_to(ROOT)} (exceptions.py --write after a '
              'change)')
    sys.exit(1 if problems or wrong else 0)


if __name__ == '__main__':
    main()
