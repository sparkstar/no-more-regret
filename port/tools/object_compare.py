#!/usr/bin/env python3
"""The port's objects before and after a change that should not change
them: a change of the format, a mechanical fix of the compiler's warnings,
a rename.

usage: object_compare.py save [--config NAME ...]
       object_compare.py check [--config NAME ...] [--target OLD=NEW ...]

Builds the port's objects in three GCC configurations (build/objects/NAME:
linux, the window's build with the tests; nowindow; win, MinGW), each
without debugging information (it holds the lines) and
with each function and datum in a section of its own
(-ffunction-sections -fdata-sections), so that the objects compare by
function and not by the order a compiler puts the functions in, and
without the halts' places (PORT_PLACES=OFF: their lines would follow every
line that moves into the objects). save
writes the fingerprints (build/objects/NAME.json): for each object of the
port's sources, each section's size, contents and relocations. check
builds again and compares with them: an object whose file is the same
bytes as when saved is the same, and only the others are read again (a
change that touches a few files is checked in seconds). A function that is only renamed
(the same contents and relocations under another name, as a parameter
that becomes const char * gives) counts as the same and is listed; the
references to it are read under its new name. An object whose source moved
pairs with the one of the same target and file name; --target OLD=NEW names
a target renamed between the save and the check.

Prints, for each configuration, the objects that are the same, and names
what differs: the sections (functions and data) whose contents or
relocations differ, those only before or only after, and the objects only
before or only after. Exit 1 when anything differs. The sections that the
code's own sections decide (the unwinding tables, the section groups,
the compiler's notes) are not compared.
"""
import argparse, collections, concurrent.futures, hashlib, json, os, re, subprocess, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
BUILD = ROOT / 'build/objects'
SDL = ROOT / 'build/port/_deps/sdl3-src'
COMMON = ['-G', 'Ninja', '-DCMAKE_BUILD_TYPE=RelWithDebInfo',
          '-DCMAKE_CXX_FLAGS_RELWITHDEBINFO=-O2 -DNDEBUG',
          '-DCMAKE_CXX_FLAGS=-ffunction-sections -fdata-sections', '-DPORT_PLACES=OFF']
CONFIGS = {
    'linux': COMMON + ['-DCMAKE_CXX_COMPILER=g++', '-DPORT_WINDOW=ON'],
    'nowindow': COMMON + ['-DCMAKE_CXX_COMPILER=g++', '-DPORT_WINDOW=OFF'],
    'win': COMMON + [f'-DCMAKE_TOOLCHAIN_FILE={ROOT}/port/cmake/mingw-w64-x86_64.cmake', '-DPORT_WINDOW=ON'],
}
OBJDUMP = {'linux': 'objdump', 'nowindow': 'objdump', 'win': 'x86_64-w64-mingw32-objdump'}
# Sections that follow from the code's (unwinding), group it, or note the
# compiler: not compared.
IGNORED = re.compile(r'^(\.group|\.comment|\.note\..*|\.eh_frame|\.debug.*|\.pdata.*|\.xdata.*|\.rdata\$zzz|'
                     r'\.llvm_addrsig|\.gnu\.lto.*)$')
SECTION = re.compile(r'^\s*\d+\s+(\S+)\s+([0-9a-f]+)\s')
CONTENTS = re.compile(r'^Contents of section (.+):$')
RELOCATIONS = re.compile(r'^RELOCATION RECORDS FOR \[(.+)\]:$')
TEXT = re.compile(r'^\.text[.$]')


def build(name):
    """Configures (once) and builds a configuration; the directory."""
    directory = BUILD / name
    if not (directory / 'build.ninja').exists():
        sources = [] if name == 'nowindow' else [f'-DFETCHCONTENT_SOURCE_DIR_SDL3={SDL}']
        subprocess.run(['cmake', '-S', str(ROOT / 'port'), '-B', str(directory)] + CONFIGS[name] + sources,
                       check=True, stdout=subprocess.DEVNULL)
    # The configuration's settings again (one added since the directory was
    # made reaches it: PORT_PLACES), then the objects of sources the build no
    # longer has (moved or removed) go, so that only what the build makes is
    # compared.
    settings = [o for o in CONFIGS[name] if o.startswith('-D') and not o.startswith(('-DCMAKE_TOOLCHAIN_FILE',
                                                                                      '-DCMAKE_CXX_COMPILER'))]
    subprocess.run(['cmake', str(directory)] + settings, check=True, stdout=subprocess.DEVNULL)
    subprocess.run(['ninja', '-C', str(directory), '-t', 'cleandead'], check=True, stdout=subprocess.DEVNULL)
    result = subprocess.run(['cmake', '--build', str(directory)], capture_output=True, text=True)
    if result.returncode:
        sys.exit(f'{name}: the build failed\n{result.stdout[-4000:]}{result.stderr[-2000:]}')
    return directory


def objects(directory):
    """The objects of the port's sources in a build (not SDL's)."""
    found = []
    for path in sorted(directory.rglob('*.o*')):
        relative = path.relative_to(directory).as_posix()
        if path.suffix in ('.o', '.obj') and '.dir/' in relative and not relative.startswith('_deps/') \
                and '/CompilerId' not in relative:
            found.append(relative)
    return found


def fingerprint(objdump, path):
    """{section: [size, contents' hash, relocations]} of an object; a name
    that repeats gets #n after it."""
    headers = subprocess.run([objdump, '-h', '-w', path], capture_output=True, text=True, check=True).stdout
    sizes, seen = {}, {}
    for line in headers.splitlines():
        m = SECTION.match(line)
        if not m or IGNORED.match(m.group(1)):
            continue
        name = m.group(1)
        seen[name] = seen.get(name, 0) + 1
        key = name if seen[name] == 1 else f'{name}#{seen[name]}'
        sizes[key] = int(m.group(2), 16)
    dump = subprocess.run([objdump, '-s', '-r', '-w', path], capture_output=True, text=True, check=True).stdout
    contents, relocations, section, mode, counts = {}, {}, None, None, {}

    def keyed(name):
        counts[(mode, name)] = counts.get((mode, name), 0) + 1
        n = counts[(mode, name)]
        return name if n == 1 else f'{name}#{n}'
    for line in dump.splitlines():
        m = RELOCATIONS.match(line)
        if m:
            mode = 'r'
            section = keyed(m.group(1))
            relocations.setdefault(section, [])
            continue
        m = CONTENTS.match(line)
        if m:
            mode = 'c'
            section = keyed(m.group(1))
            contents.setdefault(section, [])
            continue
        if section is None or not line.strip():
            continue
        if mode == 'r':
            fields = line.split()
            if len(fields) >= 3 and re.fullmatch(r'[0-9a-f]+', fields[0]):
                relocations[section].append([int(fields[0], 16), fields[1], ' '.join(fields[2:])])
        elif mode == 'c':
            # " 0000 f30f1efa 660f6f05 00000000 c6476800  ....f.o......Gh."
            head = line[1:].split(' ', 1)
            if len(head) == 2 and re.fullmatch(r'[0-9a-f]+', head[0]):
                contents[section].append(head[1][:35].replace(' ', ''))
    out = {}
    for key, size in sizes.items():
        data = ''.join(contents.get(key, []))
        out[key] = [size, hashlib.sha1(data.encode()).hexdigest()[:16], relocations.get(key, [])]
    return out


def file_hash(path):
    return hashlib.sha1(Path(path).read_bytes()).hexdigest()


def fingerprints(name, directory, saved=None):
    """{object: fingerprint} of a build, {object: its file's hash}, and how
    many were read: an object whose file is as saved keeps its saved
    fingerprint."""
    objdump = OBJDUMP[name]
    paths = objects(directory)
    with concurrent.futures.ThreadPoolExecutor(12) as pool:
        hashes = dict(zip(paths, pool.map(lambda p: file_hash(directory / p), paths)))
    known = (saved or {}).get('hashes', {})
    prints = (saved or {}).get('prints', {})
    fresh = [p for p in paths if known.get(p) != hashes[p] or p not in prints]
    with concurrent.futures.ThreadPoolExecutor(12) as pool:
        read = dict(zip(fresh, pool.map(lambda p: fingerprint(objdump, str(directory / p)), fresh)))
    return {p: read[p] if p in read else prints[p] for p in paths}, hashes, len(fresh)


SECTION_OF = re.compile(r'^(\.(?:text|data|rodata|bss|rdata|tbss|tdata|gcc_except_table)(?:\.rel\.ro)?'
                        r'(?:\.(?:unlikely|hot|startup|exit))?[.$])(.+)$')


def symbol(section):
    """The symbol a section of its own holds (.text.NAME, .rodata.NAME,
    .gcc_except_table.NAME, .text$NAME)."""
    m = SECTION_OF.match(section.split('#')[0])
    return m.group(2) if m else None


def unqualified(name):
    """A mangled name without its member function's cv-qualifiers (a
    method made const is the same function), and with the anonymous
    namespace's names anywhere in it as a static one's
    (_ZN12_GLOBAL__N_13fooEv as _ZL3foov, a lambda's class in such a
    function the same): a static moved into the anonymous namespace is the
    same."""
    if not name:
        return name
    name = re.sub(r'^_ZN[KV]+', '_ZN', name)
    marker = 'N12_GLOBAL__N_1'
    out, at = [], 0
    while True:
        found = name.find(marker, at)
        if found < 0:
            out.append(name[at:])
            break
        # an internal variable's name has an L before its length
        local = name[found + len(marker):found + len(marker) + 1] == 'L'
        digits = re.match(r'\d+', name[found + len(marker) + local:])
        if not digits:
            out.append(name[at:found + len(marker)])
            at = found + len(marker)
            continue
        begin = found + len(marker) + local + len(digits.group(0))
        identifier = name[begin:begin + int(digits.group(0))]
        after = begin + int(digits.group(0))
        if name[after:after + 1] == 'E':
            out.append(name[at:found] + 'L' + digits.group(0) + identifier)
            at = after + 1
        else:
            out.append(name[at:after])
            at = after
    # inside a named namespace (the one program's regret_values::): the
    # anonymous one's component is a static's L
    return re.sub(r'(?<=[A-Za-z_0-9])12_GLOBAL__N_1L?(?=\d)', 'L', ''.join(out))


def demangle(names):
    if not names:
        return {}
    out = subprocess.run(['c++filt'], input='\n'.join(names), capture_output=True, text=True).stdout.splitlines()
    return dict(zip(names, out))


def target_and_name(path, targets):
    """(the target, under its new name, and the file name) of an object's
    path in a build."""
    parts = Path(path).parts
    target = next((p for p in parts if p.endswith('.dir')), '').removesuffix('.dir')
    return targets.get(target, target), parts[-1]


def paired(before, after, targets):
    """before with the objects whose sources moved under their paths now: an
    object only before that pairs with one only after by its target and file
    name, alone of that pair on each side."""
    gone = collections.defaultdict(list)
    new = collections.defaultdict(list)
    for path in set(before) - set(after):
        gone[target_and_name(path, targets)].append(path)
    for path in set(after) - set(before):
        new[target_and_name(path, targets)].append(path)
    out = dict(before)
    for key, paths in gone.items():
        if len(paths) == 1 and len(new.get(key, [])) == 1:
            out[new[key][0]] = out.pop(paths[0])
    return out


def compare(name, before, after, targets=None, unchanged=frozenset()):
    """Prints the differences of a configuration; how many there are.
    unchanged: the objects whose files are the bytes saved, the same."""
    before = paired(before, after, targets or {})
    only_before = sorted(set(before) - set(after))
    only_after = sorted(set(after) - set(before))
    # Renamed: a section only before and one only after, with the same size,
    # contents and relocations at the same places. Each object's own (a
    # local symbol, as GCC's CSWTCH.n of a switch's table, is the object's
    # alone), and all of them for the references between objects.
    renames = {}
    own_renames = collections.defaultdict(dict)
    for path in sorted(set(before) & set(after)):
        if path in unchanged:
            continue
        gone = {s: v for s, v in before[path].items() if s not in after[path]}
        new = {s: v for s, v in after[path].items() if s not in before[path]}
        # First by name: the same name but for the anonymous namespace or a
        # const (unqualified), one each.
        by_name = collections.defaultdict(list)
        for s in new:
            if symbol(s):
                by_name[(s.split(symbol(s))[0], unqualified(symbol(s)))].append(s)
        for s in list(gone):
            if not symbol(s):
                continue
            match = by_name.get((s.split(symbol(s))[0], unqualified(symbol(s))), [])
            if len(match) == 1 and match[0] in new:
                renames[symbol(s)] = symbol(match[0])
                own_renames[path][symbol(s)] = symbol(match[0])
                del gone[s]
                del new[match[0]]
        shapes = {}
        for s, (size, digest, relocations) in new.items():
            shapes.setdefault((size, digest, tuple((r[0], r[1]) for r in relocations)), []).append(s)
        for s, (size, digest, relocations) in gone.items():
            match = [m for m in shapes.get((size, digest, tuple((r[0], r[1]) for r in relocations)), [])
                     if symbol(m)]
            if len(match) > 1:
                # A member function made const: its name with the const
                # qualifier taken out (_ZNK -> _ZN) is the old one's.
                same_name = [m for m in match if unqualified(symbol(m)) == unqualified(symbol(s))]
                if len(same_name) == 1:
                    match = same_name
            if len(match) > 1:
                # Several of the same shape (a static and its guard): the one
                # whose name shares the longest start with the old name.
                shared = [len(os.path.commonprefix([s, m])) for m in match]
                if shared.count(max(shared)) == 1:
                    match = [match[shared.index(max(shared))]]
            if len(match) == 1 and symbol(s):
                renames[symbol(s)] = symbol(match[0])
                own_renames[path][symbol(s)] = symbol(match[0])

    # A constructor's or destructor's body is under its base name (C2, D2);
    # the calls name the complete one (C1, D1), an alias in the same section.
    for table in (renames, *own_renames.values()):
        for old_name, new_name in list(table.items()):
            for base, complete in (('C2E', 'C1E'), ('D2E', 'D1E')):
                if base in old_name and base in new_name:
                    table.setdefault(old_name.replace(base, complete, 1), new_name.replace(base, complete, 1))

    def renamed(value, path):
        """A relocation's target under the new names: a renamed symbol, or a
        section of a renamed function's own (its exception table). A symbol
        the object has a section of is looked up in its own renames only."""
        m = re.match(r'^([^+\-]+)(.*)$', value)
        if not m:
            return value
        base = m.group(1)
        own = SECTION_OF.match(base)
        name = own.group(2) if own else base
        local = own_renames[path]
        defined = defined_in[path]
        table = local if name in local or name in defined else renames
        if base in table:
            return table[base] + m.group(2)
        if own and own.group(2) in table:
            return own.group(1) + table[own.group(2)] + m.group(2)
        return value
    defined_in = {path: {symbol(s) for s in before[path]} for path in set(before) & set(after)}
    differing = []
    same = 0
    for path in sorted(set(before) & set(after)):
        b, a = before[path], after[path]
        if path in unchanged:
            same += 1
            continue
        problems = []
        local = own_renames[path]
        for s in sorted(set(b) | set(a)):
            target = s
            if s not in a and symbol(s) in local:
                target = s.replace(symbol(s), local[symbol(s)])
            if s not in b and symbol(s) in local.values():
                continue
            if s not in b:
                problems.append(('only after', s))
            elif target not in a:
                problems.append(('only before', s))
            else:
                size_b, digest_b, relocations_b = b[s]
                size_a, digest_a, relocations_a = a[target]
                moved = [[r[0], r[1], renamed(r[2], path)] for r in relocations_b]
                if (symbol(s) or '').startswith('_ZTS') and target != s:
                    # A type's name, its contents the name: renamed with it.
                    continue
                if size_b != size_a or digest_b != digest_a:
                    problems.append(('contents differ', s))
                elif moved != relocations_a:
                    problems.append(('relocations differ', s))
        if problems:
            differing.append((path, problems))
        else:
            same += 1
    names = demangle(sorted({symbol(s) for _, problems in differing for _, s in problems if symbol(s)} |
                            set(renames) | set(renames.values())))
    print(f'{name}: {len(set(before) & set(after))} objects, {same} the same, {len(differing)} differ'
          + (f', {len(only_before)} only before' if only_before else '')
          + (f', {len(only_after)} only after' if only_after else '')
          + (f', {len(renames)} renamed' if renames else ''))
    for old, new in sorted(renames.items()):
        print(f'  renamed: {names.get(old, old)} -> {names.get(new, new)}')
    for path, problems in differing:
        print(f'  {path}')
        for what, s in problems:
            print(f'    {what}: {names.get(symbol(s), s) if symbol(s) else s}')
    for path in only_before:
        print(f'  only before: {path}')
    for path in only_after:
        print(f'  only after: {path}')
    return len(differing) + len(only_before) + len(only_after)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('action', choices=('save', 'check'))
    ap.add_argument('--config', action='append', choices=sorted(CONFIGS), help='default: all three')
    ap.add_argument('--target', action='append', default=[], help='with check: OLD=NEW, a renamed target')
    args = ap.parse_args()
    targets = dict(t.split('=', 1) for t in args.target)
    differences = 0
    for name in args.config or list(CONFIGS):
        directory = build(name)
        saved = BUILD / f'{name}.json'
        before = json.loads(saved.read_text()) if saved.exists() else None
        if before is not None and 'prints' not in before:
            before = {'prints': before, 'hashes': {}}
        if args.action == 'save':
            prints, hashes, _ = fingerprints(name, directory, before)
            saved.write_text(json.dumps({'prints': prints, 'hashes': hashes}))
            print(f'{name}: {len(prints)} objects saved')
        else:
            if before is None:
                sys.exit(f'{name}: nothing saved (object_compare.py save)')
            prints, hashes, read = fingerprints(name, directory, before)
            print(f'{name}: {read} objects changed since the save')
            unchanged = frozenset(p for p, h in hashes.items() if before['hashes'].get(p) == h)
            differences += compare(name, before['prints'], prints, targets, unchanged)
    return 1 if differences else 0


if __name__ == '__main__':
    sys.exit(main())
