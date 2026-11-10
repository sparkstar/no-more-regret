#!/usr/bin/env python3
"""The game's boundary: what the game's libraries (game, asm, asylum, xan)
take from outside themselves, held to its rules. The build runs it before
the program links (CMakeLists.txt's target boundary, PORT_BOUNDARY_CHECK),
on every build: GCC's and Clang's, the sanitizers' and the coverage's,
MinGW-w64's and macOS's.

usage: boundary.py [--build DIR] [--nm NM] [--cxxfilt CXXFILT]
                   [--leading-underscore] [--list]

A symbol an object of the libraries' (and of the program's choice of the
game's values and links of the games' data) leaves undefined and none of
them defines is the boundary's. Each must be one of:
- platform: a function port/platform's headers declare, which the system
  layer gives: the game talks to the host through these alone;
- library: one of port/compat's functions the port keeps (KEPT: Borland's
  string functions and rand, which the original's library had, and the
  crash report);
- runtime: a function of the C and C++ runtime without side effects on the
  host (PURE: memory, strings, characters, formatting into a buffer,
  parsing, new and delete; PURE_STD: the C++ library's strings and standard
  exceptions, which libc++ keeps out of line);
- the compiler's own (TOOLCHAIN): exceptions, RTTI, the stack's protection
  and probes, thread-local storage, the sanitizers' and the coverage's
  instrumentation, the C++ library's internals.
Any other fails (exit 1), named with the game's objects that use it: a
symbol of the system layer that port/platform does not declare (the game
reaching into the virtual PC), a function of port/compat not kept, or a
runtime function not known to be pure (a file, the standard output, the
environment, the clock, the process): the game gets a platform function for
it instead, or a function without side effects goes into PURE.

The names are compared as one toolchain writes them, made the same for all:
C++'s demangled (c++filt), Mach-O's leading underscore dropped
(--leading-underscore), the inline namespaces of the C++ libraries (libc++'s
std::__1, libstdc++'s std::__cxx11) left out, MinGW-w64's prefixes of a DLL's
import (__imp_) and of its own stdio (__mingw_) dropped, a C function's
checked or C23 variant (__memcpy_chk, __isoc23_sscanf) taken as the function
and the calls a compiler makes in a function's place (puts and putchar for
printf) as that function.

--list prints every symbol of the boundary with its state and its users.
"""
import argparse, collections, re, subprocess, sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import overlay  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
LIBRARIES = tuple(f'lib{target}.a' for target in overlay.targets())
# The program's objects of the game's sources besides the libraries
# (CMakeLists.txt): its choice of the game's values and the links of the
# games' data files.
GAME_OBJECTS = ('unified_values', 'unified_data')
# The targets whose objects are the system layer's (CMakeLists.txt): its
# objects and the arena's. The program's own (main, the window, new and
# delete) are not built yet when the check runs; what they give is a
# platform function or new and delete, which are known by their names.
SYSTEM_TARGETS = ('port_system_objects', 'game_memory')
OBJECT_SUFFIXES = ('.o', '.obj')
# port/compat's library functions the port keeps (fnsplit) and its crash
# report, which the game's stops call (compat/crash.h).
KEPT = {'stricmp', 'strnicmp', 'strlwr', 'itoa', 'borlandRand', 'fnsplit', 'crashReport', 'fatalError'}
# The C and C++ runtime's functions without side effects on the host, by
# kind; a toolchain's own names for one beside it (glibc's and macOS's
# character tables, macOS's memset of a pattern, Clang's bcmp for a memcmp
# compared with 0).
PURE = {
    # memory
    'memchr', 'memcmp', 'bcmp', 'memcpy', 'memmove', 'memset', 'bzero', 'memset_pattern16',
    # strings
    'strcat', 'strchr', 'strcmp', 'strcpy', 'strcspn', 'strlen', 'strncmp', 'strncpy', 'strspn', 'strstr',
    # characters
    'isalnum', 'tolower', 'toupper', '__ctype_b_loc', '__ctype_tolower_loc', '__ctype_toupper_loc',
    '_DefaultRuneLocale', '__maskrune', '__tolower', '__toupper',
    # formatting into a buffer
    'snprintf', 'vsnprintf',
    # parsing
    'sscanf', 'vsscanf', 'strtol', 'atoi', 'atol',
    # the program's new and delete (system/newdelete.cpp)
    'operator new', 'operator new[]', 'operator delete', 'operator delete[]',
}
# The C++ library's parts without side effects on the host that a library
# keeps out of line (libc++ does, where libstdc++ names its own internal
# functions): the strings' members and std::to_string, and the standard
# exceptions' members, type information and virtual tables, which the
# strings, the containers, new[] and std::function throw.
STANDARD_EXCEPTIONS = ('exception|bad_alloc|bad_array_new_length|bad_function_call|bad_cast|bad_typeid|logic_error|'
                       'length_error|out_of_range|invalid_argument|domain_error|runtime_error|range_error|'
                       'overflow_error|underflow_error')
PURE_STD = re.compile(r'^(std::basic_string<.*>::\w+\(|std::to_string\(|'
                      rf'((typeinfo|typeinfo name|vtable) for )?std::({STANDARD_EXCEPTIONS})(::|$))')
DEFINED = set('TDBRWVCGSIiu')
# The compiler's and its library's own symbols, by their names made one.
TOOLCHAIN = re.compile(
    r'^(_GLOBAL_OFFSET_TABLE_|__dso_handle|_Unwind_\w+|__cxa_\w+|__gxx_personality_\w+|__stack_chk_(fail|guard)|'
    r'__dynamic_cast|__chkstk\w*|___chkstk\w*|__tls_get_addr|_tlv_bootstrap|__emutls_get_address|'
    r'vtable for __cxxabiv1::\w+|std::type_info::operator==\(std::type_info const&\) const|std::terminate\(\))$|'
    r'^__(asan|ubsan|sanitizer|lsan|msan|tsan|gcov)_|^__llvm_(gcov|profile|prf)')
# The calls GCC makes in printf's place (a format of text alone).
IN_PRINTF_PLACE = {'puts', 'putchar'}


def plain(name):
    """The name made one: the inline namespaces left out, MinGW-w64's
    prefixes dropped, a C function for its checked or C23 variant, printf
    for the calls GCC makes in its place."""
    name = name.replace('std::__1::', 'std::').replace('std::__cxx11::', 'std::')
    name = name.removeprefix('__imp_')
    if name.startswith('__mingw_'):
        name = name.removeprefix('__mingw_')
    if name in IN_PRINTF_PLACE:
        return 'printf'
    m = re.fullmatch(r'__(\w+)_chk', name) or re.fullmatch(r'__isoc\d+_(\w+)', name)
    return m.group(1) if m else name


def without_templates(name):
    """The name without its template arguments: <...> at any depth."""
    out, depth = [], 0
    for c in name:
        if c == '<':
            depth += 1
        elif c == '>':
            depth = max(depth - 1, 0)
        elif depth == 0:
            out.append(c)
    return ''.join(out)


def base_name(name):
    """A function's name without its arguments, its scopes and its template
    arguments (operator new[] as it is)."""
    head = without_templates(name.split('(')[0])
    return head.split('::')[-1].strip()


def internal(name):
    """A C++ library's internal function: in std, its own name reserved
    (beginning with _: std::__throw_length_error, a class's _M_create)."""
    return name.startswith('std::') and base_name(name).startswith('_')


def platform():
    """The names port/platform's headers declare."""
    names = set()
    for header in (ROOT / 'port/platform').glob('*.h'):
        text = re.sub(r'/\*.*?\*/', '', header.read_text(), flags=re.S)
        names.update(re.findall(r'^(?:\[\[[^\]]*\]\] )?[\w:<> ]+?[ *&](\w+)\(', text, re.M))
    return names


def symbols(nm, path, member=None):
    """{member: (defined, undefined)} of an archive or an object, by nm."""
    out = collections.defaultdict(lambda: (set(), set()))
    text = subprocess.run([nm, '-A', str(path)], capture_output=True, text=True, check=True).stdout
    for line in text.splitlines():
        fields = line.split()
        if len(fields) < 2:
            continue
        name, kind = fields[-1], fields[-2]
        where = member or line.split(':')[1]
        defined, undefined = out[where]
        if kind == 'U':
            undefined.add(name)
        elif kind in DEFINED:
            defined.add(name)
    return out


def demangle(cxxfilt, names, leading_underscore):
    """{raw name: its name made one}."""
    names = list(names)
    text = subprocess.run([cxxfilt], input='\n'.join(names), capture_output=True, text=True, check=True).stdout
    out = {}
    for raw, shown in zip(names, text.splitlines()):
        # Mach-O's own underscore before a C name, which c++filt leaves.
        if leading_underscore and shown == raw and shown.startswith('_'):
            shown = shown[1:]
        out[raw] = plain(shown)
    return out


def objects(directory):
    """The object files under a target's directory."""
    return sorted(path for suffix in OBJECT_SUFFIXES for path in directory.rglob(f'*{suffix}'))


def object_name(member):
    """A member's file's name without its suffixes, as the users show it."""
    for suffix in OBJECT_SUFFIXES:
        member = member.removesuffix(suffix)
    return member.removesuffix('.cpp')


def boundary(build, nm, cxxfilt, leading_underscore):
    """{symbol: (state, where from, users)}; a state of three: platform,
    library, runtime, else what is wrong with it."""
    defined, users = set(), collections.defaultdict(set)
    found = {}
    for library in LIBRARIES:
        found.update(symbols(nm, build / library))
    for target in GAME_OBJECTS:
        for obj in objects(build / 'CMakeFiles' / f'{target}.dir'):
            found.update(symbols(nm, obj, obj.name))
    for member, (d, u) in found.items():
        defined |= d
        for name in u:
            users[name].add(object_name(member))
    outside = {name for name in users if name not in defined}
    providers = {}
    for target in SYSTEM_TARGETS:
        for obj in objects(build / 'CMakeFiles' / f'{target}.dir'):
            for d, _ in symbols(nm, obj, obj.name).values():
                for name in d & outside:
                    providers.setdefault(name, f'system/{object_name(obj.name)}.cpp')
    for member, (d, _) in symbols(nm, build / 'libcompat.a').items():
        for name in d & outside:
            providers.setdefault(name, f'compat/{object_name(member)}.cpp')
    names = demangle(cxxfilt, outside, leading_underscore)
    interfaces = platform()
    rows = {}
    for name in outside:
        symbol = names[name]
        if TOOLCHAIN.search(symbol) or internal(symbol):
            continue
        where = providers.get(name, 'runtime')
        base = base_name(symbol)
        if base in interfaces:
            state = 'platform'
        elif where.startswith('compat/'):
            state = 'library' if base in KEPT else f"port/compat's ({where}), not in KEPT"
        elif where.startswith('system/'):
            state = f"the system layer's ({where}), not declared in port/platform"
        elif base in PURE or PURE_STD.search(symbol):
            state = 'runtime'
        else:
            state = "the runtime's, not in PURE (the functions without side effects on the host)"
        used = users[name] | (rows[symbol][2] if symbol in rows else set())
        rows[symbol] = (state, where, used)
    return rows


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('--build', type=Path, default=ROOT / 'build/port')
    ap.add_argument('--nm', default='nm')
    ap.add_argument('--cxxfilt', default='c++filt')
    ap.add_argument('--leading-underscore', action='store_true')
    ap.add_argument('--list', action='store_true')
    args = ap.parse_args()
    rows = boundary(args.build.resolve(), args.nm, args.cxxfilt, args.leading_underscore)
    allowed = ('platform', 'library', 'runtime')
    if args.list:
        for symbol, (state, where, used) in sorted(rows.items(), key=lambda r: (r[1][0], r[1][1], r[0])):
            print(f'{state}\t{where}\t{symbol}\t{",".join(sorted(used))}')
    states = collections.Counter(state for state, _, _ in rows.values() if state in allowed)
    wrong = {symbol: row for symbol, row in rows.items() if row[0] not in allowed}
    for symbol, (state, _, used) in sorted(wrong.items()):
        print(f'BOUNDARY {symbol} (used by {",".join(sorted(used))}): {state}')
    print(f'boundary: {len(rows)} symbols, ' + ', '.join(f'{states[s]} {s}' for s in allowed)
          + (f'; {len(wrong)} break its rules: the game reaches the host through port/platform alone '
             '(port/tools/boundary.py)' if wrong else ''))
    sys.exit(1 if wrong else 0)


if __name__ == '__main__':
    main()
