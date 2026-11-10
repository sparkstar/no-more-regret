#!/usr/bin/env python3
"""The scenes' documents: the function of each
scene in scenes.all_scenes has a docstring of fields, which this checks and
lists. The form:

    <What the scene does: a sentence or a few lines, ending with a stop.>

    Variant of: <scene>              (a variant: its tests are that scene's)
    Checks: <the engine's behaviour, the port's sources it goes through>
    Drives: <usecode or inputs, and the data made for it>
    Tests:                           (not in a variant)
        <test name>
            <what it shows, the values the games come to, and its
            footprint: what in the records shows it ran and, where the
            records keep it, its result>
    Original: <Both follow ...> or <None: why, and its ground>
    Guards: <the bugs and decisions it keeps>              (may be left out)
    Traps: <what a change of it must keep clear of>        (may be left out)

The fields come in that order, a field's text goes on in indented lines,
and a test name may hold <words> in angle brackets for any text, <n> for a
number (the tests made from a table's rows, or of a pattern). A test is
under the entry of its name, or else under the first entry whose pattern it
fits. The check: every field there
that must be, no other; each test of the scene (in each game) under one
entry of Tests, and each entry over a test in one game at least; no two
tests of a scene of the same name; Original "Both follow" for a scene the
original follows, "None" for one it does not.

usage: docs.py check
       docs.py list [--game regret|remorse] [--scene NAME]
"""
import argparse, inspect, re, sys
from dataclasses import dataclass, field
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import scenes  # noqa: E402

GAMES = ('regret', 'remorse')
FIELDS = ('Variant of', 'Checks', 'Drives', 'Tests', 'Original', 'Guards', 'Traps')
REQUIRED = ('Checks', 'Drives', 'Original')
FIELD = re.compile(r'([A-Z][a-z]+(?: of)?): ?(.*)')
INDENT = 4


@dataclass
class Doc:
    summary: str = ''
    fields: dict = field(default_factory=dict)
    # [(test name, what it shows)]
    tests: list = field(default_factory=list)


def parse(text):
    """A scene's docstring as a Doc, and what is wrong with its form."""
    problems = []
    lines = inspect.cleandoc(text or '').splitlines()
    first = []
    while lines and lines[0].strip():
        first.append(lines.pop(0).strip())
    doc = Doc(summary=' '.join(first))
    if not doc.summary:
        return doc, ['no first lines (what the scene does)']
    if not doc.summary.endswith('.'):
        problems.append('the first lines do not end with a stop')
    name = None
    for line in lines:
        if not line.strip():
            continue
        indent = len(line) - len(line.lstrip())
        if indent == 0:
            match = FIELD.fullmatch(line)
            if match is None or match.group(1) not in FIELDS:
                problems.append(f'not a field: {line.strip()[:60]!r}')
                name = None
                continue
            name = match.group(1)
            if name in doc.fields:
                problems.append(f'{name} twice')
            doc.fields[name] = match.group(2).strip()
            if name == 'Tests' and doc.fields[name]:
                problems.append('Tests: its entries go on the lines under it')
        elif name is None:
            problems.append(f'a line outside the fields: {line.strip()[:60]!r}')
        elif name == 'Tests':
            if indent == INDENT:
                doc.tests.append((line.strip(), ''))
            elif indent > INDENT and doc.tests:
                test, said = doc.tests[-1]
                doc.tests[-1] = (test, f'{said} {line.strip()}'.strip())
            else:
                problems.append(f'Tests: an entry\'s name {INDENT} spaces in, its text more: {line.strip()[:60]!r}')
        else:
            doc.fields[name] = f'{doc.fields[name]} {line.strip()}'.strip()
    order = [name for name in FIELDS if name in doc.fields]
    if order != list(doc.fields):
        problems.append(f'the fields out of order: {", ".join(doc.fields)} (the order: {", ".join(FIELDS)})')
    for name in REQUIRED + (() if 'Variant of' in doc.fields else ('Tests',)):
        if not doc.fields.get(name) and not (name == 'Tests' and doc.tests):
            problems.append(f'no {name}')
    if 'Variant of' in doc.fields and 'Tests' in doc.fields:
        problems.append('a variant has its base\'s Tests')
    for name, text in doc.fields.items():
        if name != 'Tests' and not text:
            problems.append(f'{name} empty')
    for test, said in doc.tests:
        if not said:
            problems.append(f'Tests: {test!r} says nothing')
    return doc, problems


NUMBER = r'-?(?:0x[0-9a-fA-F]+|[0-9]+)'


def pattern(test):
    """The expression of an entry's name: its <n> a number, its other
    <words> any text."""
    out = []
    for k, part in enumerate(re.split(r'(<[^<>]+>)', test)):
        if k % 2 == 0:
            out.append(re.escape(part))
        else:
            out.append(NUMBER if part == '<n>' else '.+')
    return re.compile(''.join(out))


def entry(doc, test):
    """The index of the entry of doc's Tests over test, None for none (the
    first there is: an exact name before a pattern)."""
    for k, (name, _) in enumerate(doc.tests):
        if name == test:
            return k
    for k, (name, _) in enumerate(doc.tests):
        if pattern(name).fullmatch(test):
            return k
    return None


def documents(makers):
    """{scene: Doc} of the makers' docstrings, and what is wrong with them."""
    docs, problems = {}, []
    for name, make in makers.items():
        doc, wrong = parse(make.__doc__)
        docs[name] = doc
        problems += [f'{name}: {said}' for said in wrong]
    return docs, problems


def owner(docs, name):
    """The scene whose Tests the scene's tests are under: its own, or its
    base's for a variant."""
    return docs[name].fields.get('Variant of', name)


def check(makers=None):
    """What is wrong with the scenes' documents: their form, and their
    Tests against the tests the scenes have in each game."""
    makers = makers or scenes.all_scenes()
    docs, problems = documents(makers)
    used = {name: set() for name in docs}
    for game in GAMES:
        for name, make in makers.items():
            scene = make(game)
            if scene is None:
                continue
            if scene.name != name:
                problems.append(f'{name}: its scene is called {scene.name!r}')
            base = owner(docs, name)
            if base not in docs:
                problems.append(f'{name}: a variant of {base!r}, which is no scene')
                continue
            doubled = sorted({test for test in scene.tests if scene.tests.count(test) > 1})
            if doubled:
                problems.append(f'{name} ({game}): tests of the same name: {", ".join(doubled[:4])}')
            for test in scene.tests:
                k = entry(docs[base], test)
                if k is None:
                    problems.append(f'{name} ({game}): the test {test!r} is under no entry of {base}\'s Tests')
                else:
                    used[base].add(k)
            original = docs[name].fields.get('Original', '')
            if scene.oracle and not original.startswith('Both follow'):
                problems.append(f'{name}: the original follows it, its Original does not begin "Both follow"')
            if not scene.oracle and not original.startswith('None'):
                problems.append(f'{name}: the original does not follow it, its Original does not begin "None"')
    for name, doc in docs.items():
        for k, (test, _) in enumerate(doc.tests):
            if k not in used[name]:
                problems.append(f'{name}: the entry {test!r} of its Tests is over no test')
    return problems


def describe(name, test, makers=None, docs=None):
    """What the scene's test shows, from its document ('' for nothing)."""
    if docs is None:
        docs, _ = documents(makers or scenes.all_scenes())
    if name not in docs:
        return ''
    base = owner(docs, name)
    if base not in docs:
        return ''
    k = entry(docs[base], test)
    return docs[base].tests[k][1] if k is not None else ''


def listing(games=GAMES, chosen=''):
    """The scenes and their tests, as their documents say, in text."""
    makers = scenes.all_scenes()
    docs, _ = documents(makers)
    out = []
    for name, make in makers.items():
        if chosen and chosen not in name:
            continue
        made = {game: make(game) for game in games}
        counts = ', '.join(f'{game} {len(scene.tests)}' for game, scene in made.items() if scene is not None)
        if not counts:
            continue
        doc = docs[name]
        out.append(f'{name} ({counts} tests)')
        out.append(f'  {doc.summary}')
        for field_name in FIELDS:
            if field_name in doc.fields and field_name != 'Tests':
                out.append(f'  {field_name}: {doc.fields[field_name]}')
        base = owner(docs, name)
        entries = docs[base].tests if base in docs else []
        for k, (test, said) in enumerate(entries):
            found = {game: [t for t in scene.tests if entry(docs[base], t) == k]
                     for game, scene in made.items() if scene is not None}
            shown = {game: tests for game, tests in found.items() if tests}
            if not shown:
                continue
            many = max(len(tests) for tests in shown.values())
            where = '' if len(shown) == len(found) else f' [{", ".join(shown)} only]'
            out.append(f'  - {test}' + (f' ({many} tests)' if many > 1 else '') + f'{where}: {said}')
            if chosen and '<' in test:
                names = list(dict.fromkeys(t for tests in shown.values() for t in tests))
                out.append('      ' + '; '.join(names))
        out.append('')
    return '\n'.join(out)


def main():
    ap = argparse.ArgumentParser(description='The scenes\' documents: checked or listed.')
    ap.add_argument('action', choices=('check', 'list'))
    ap.add_argument('--game', choices=GAMES)
    ap.add_argument('--scene', default='', help='the scenes whose name holds this (their tests\' names too)')
    args = ap.parse_args()
    if args.action == 'list':
        print(listing((args.game,) if args.game else GAMES, args.scene))
        return 0
    problems = check()
    for problem in problems:
        print(f'docs: {problem}')
    count = len(scenes.all_scenes())
    print(f'docs: {count} scenes, {len(problems)} problems')
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
