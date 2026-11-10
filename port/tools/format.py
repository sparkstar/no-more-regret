#!/usr/bin/env python3
"""The port's format: clang-tidy 20's fixes with port/.clang-tidy
(tidy.py), then clang-format 20 with port/.clang-format, on the port's C++
files.

usage: format.py [--check] [--no-tidy] [--all | FILE ...]
       format.py --staged

Without files, the C++ files of port/ that differ from git's HEAD (staged
or not, and new ones); --all, every C++ file of the port (common, regret,
remorse, system, platform, compat, test). Formats them in place: first
the fixes of the clang-tidy rules (tidy.py --fix: the files clang-tidy checks,
not the tests yet), which names what the rules find and cannot fix, then
clang-format; --no-tidy formats only. --check changes nothing, names the
files whose format differs and exits 1 if there are any (ctest's format:
--check --all; ctest's tidy checks the rules). --staged checks what is
staged for the next commit (the hook before a commit,
port/tools/hooks/pre-commit): its format, and what the clang-tidy rules find
in it (as the files are in the working tree). Without clang-format 20
--check exits 77, which ctest counts as skipped, and --staged lets the commit
through with a warning; without clang-tidy 20 the rules are left out with a
warning.

The generators write their files through formatted(), so that what they write
is in the format already and their --check compares the same text.
"""
import argparse, re, shutil, subprocess, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PORT = ROOT / 'port'
STYLE = PORT / '.clang-format'
DIRECTORIES = ('common', 'regret', 'remorse', 'unified', 'system', 'platform', 'compat', 'test')
VERSION = 20
MISSING = f'no clang-format {VERSION} (clang-format-{VERSION}, or clang-format of that version, on the PATH)'


def binary():
    """clang-format of major version 20 (the format differs between versions), or None."""
    for name in (f'clang-format-{VERSION}', 'clang-format'):
        path = shutil.which(name)
        if path:
            out = subprocess.run([path, '--version'], capture_output=True, text=True).stdout
            m = re.search(r'version (\d+)\.', out)
            if m and int(m.group(1)) == VERSION:
                return path
    return None


def formatted(text, name):
    """The text of a C++ file (name: its path, for the language) in the port's format."""
    tool = binary() or sys.exit(MISSING)
    result = subprocess.run([tool, f'--style=file:{STYLE}', f'--assume-filename={name}'], input=text,
                            capture_output=True, text=True, check=True)
    return result.stdout


def all_files():
    return [p for d in DIRECTORIES for p in sorted((PORT / d).rglob('*')) if p.suffix in ('.cpp', '.h')]


def changed_files():
    """The C++ files of port/ that differ from HEAD, and the new ones."""
    names = subprocess.run(['git', 'diff', '--name-only', 'HEAD', '--', 'port'], cwd=ROOT, capture_output=True,
                           text=True, check=True).stdout.split()
    names += subprocess.run(['git', 'ls-files', '--others', '--exclude-standard', '--', 'port'], cwd=ROOT,
                            capture_output=True, text=True, check=True).stdout.split()
    paths = [ROOT / n for n in dict.fromkeys(names)]
    return [p for p in paths if p.suffix in ('.cpp', '.h') and p.exists()
            and p.relative_to(PORT).parts[0] in DIRECTORIES]


def staged():
    """The staged C++ files of port/ whose staged text is not in the format; 1 if any."""
    names = subprocess.run(['git', 'diff', '--cached', '--name-only', '--diff-filter=ACMR', '--', 'port'], cwd=ROOT,
                           capture_output=True, text=True, check=True).stdout.split()
    differ = []
    for name in names:
        path = ROOT / name
        if path.suffix not in ('.cpp', '.h') or path.relative_to(PORT).parts[0] not in DIRECTORIES:
            continue
        text = subprocess.run(['git', 'show', f':{name}'], cwd=ROOT, capture_output=True, text=True,
                              check=True).stdout
        if formatted(text, name) != text:
            differ.append(name)
    for name in differ:
        print(f'not in the format: {name} (port/tools/format.py formats it; then git add)')
    found = rules([ROOT / n for n in names if (ROOT / n).suffix in ('.cpp', '.h')], fix=False)
    return 1 if differ or found else 0


def rules(files, fix):
    """The clang-tidy rules on files (tidy.py), with fix their fixes first;
    what is left, named. Without clang-tidy 20, a warning and nothing."""
    import tidy  # tidy.py imports this file
    files = [p for p in files if p.relative_to(PORT).parts[0] in tidy.DIRECTORIES
             and 'test' not in p.relative_to(PORT).parts]
    if not files:
        return []
    if not tidy.available():
        print(f'warning: {tidy.MISSING}: the clang-tidy rules are not applied')
        return []
    found = tidy.tidy(files, ROOT / 'build/tidy', fix)
    tidy.report(found)
    if found:
        print(f'tidy: {len(found)} findings the rules\' fixes leave')
    return found


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('--check', action='store_true', help='change nothing; name the files not in the format')
    ap.add_argument('--all', action='store_true', help='every C++ file of the port')
    ap.add_argument('--staged', action='store_true', help='check what is staged for the next commit')
    ap.add_argument('--no-tidy', action='store_true', help='clang-format only, without the clang-tidy fixes')
    ap.add_argument('files', nargs='*', type=Path)
    args = ap.parse_args()
    if args.staged:
        if binary() is None:
            print(f'warning: {MISSING}: the format is not checked')
            return 0
        return staged()
    files = all_files() if args.all else [p.resolve() for p in args.files] if args.files else changed_files()
    tool = binary()
    if tool is None:
        print(MISSING)
        return 77 if args.check else 1
    found = [] if args.check or args.no_tidy else rules(files, fix=True)
    differ = []
    # clang-format takes many files at once: --dry-run --Werror names the ones that differ.
    for i in range(0, len(files), 100):
        batch = [str(p) for p in files[i:i + 100]]
        if args.check:
            result = subprocess.run([tool, f'--style=file:{STYLE}', '--dry-run', '--Werror'] + batch,
                                    capture_output=True, text=True)
            differ += sorted({line.split(':')[0] for line in result.stderr.splitlines()
                              if ': error: code should be clang-formatted' in line})
        else:
            subprocess.run([tool, f'--style=file:{STYLE}', '-i'] + batch, check=True)
    if args.check:
        for path in differ:
            print(f'not in the format: {Path(path).relative_to(ROOT)}')
        print(f'format: {len(files)} files, {len(differ)} not in the format')
        return 1 if differ else 0
    print(f'format: {len(files)} files formatted')
    return 1 if found else 0


if __name__ == '__main__':
    sys.exit(main())
