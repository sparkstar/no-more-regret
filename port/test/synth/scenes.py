"""The scenes of the synthetic data: each a
map of its own, the usecode classes it runs, the game's arguments that
start it there and its length. A test of a scene is one pass: a pass oracle
that stops names the test by its pass. Each leaves a footprint in the
records of a replay (Scene.ran, Record), or is counted as shown by none."""
import bisect, csv
from dataclasses import dataclass, field
from pathlib import Path

import formats
import ucasm

PORT = Path(__file__).resolve().parents[2]

# The shapes of the synthetic world (world.py gives them their type flags).
SILENCER, FLOOR, EGG, ITEM, CONTAINER, ACTOR, BARREL = 1, 100, 200, 300, 301, 302, 303
# The tactics' waypoints (game/attack.cpp TACTIC_WALK_TO_WAYPOINT): the
# frame its number, the low quality its group.
WAYPOINT = 0x33a
# The teleport eggs (family 8).
TELEPORT_EGG = 0x1d0
# The glob eggs (family 3: their quality the glob), the camera's snap eggs
# (display/snap.h SNAP_EGG), a crate (what lies on it falls when it goes).
GLOB_EGG, SNAP_EGG, CRATE = 16, 0x4fe, 304
# A raised platform's tile (64 by 64 by 48), a wall (16 by 64 by 64) and one
# along the x axis (64 by 16 by 64).
PLATFORM, WALL, WALL_ALONG = 101, 102, 103
# A switch the selection (S) picks and uses.
SWITCH = 105
# Shapes that break in the ways damage.flx has (breakables.py): their
# entries in world.DAMAGE.
BREAKABLES = tuple(range(310, 317))
# NPC shapes the program knows by number: a woman (FEMALE_SHAPES), a
# speaker of ready lines (2FDh) and robots, some of which feint when hit
# (ROBOT_SHAPES; crusader.cpp takeHit, attack.cpp).
NPC_SHAPES = (0x597, 0x2fd, 0x383, 0x4e6, 0x338, 0x385, 0x4c8)
# NPC shapes that speak before they fire (attack.cpp, npcs.py's attackers)
# besides those above and deaths.py's: 319h, 384h, 1B4h, 2DFh; and No
# Regret's crawling spider (584h), which the NPCs shoot at.
SPEAKER_SHAPES = (0x319, 0x384, 0x1b4, 0x2df)
CRAWLING_SPIDER = 0x584
# Targets of the hits (hits.py): NPCs of hit points enough for every
# weapon's shots, standing; an organic one (a shape the program does not
# know) and a robot (of ROBOT_SHAPES, whose bullets' damage is a third).
TARGET, ROBOT_TARGET = 0x5e0, 0x45d
TARGETS = (TARGET, ROBOT_TARGET)
TARGET_HP = 20000
# Shapes whose frames are big (120 to 240 square): the sprites of crowd.py fill the
# shape cache with them.
BIG_SHAPES = (120, 121, 122, 123)
# Shapes that animate on the screen (display/dlist.cpp animateItems): their
# animation's type, data and speed.
ANIMATED = {110 + k: kind for k, kind in enumerate([(1, 0, 1), (1, 2, 2), (2, 0, 1), (3, 1, 1), (4, 2, 1),
                                                    (5, 0, 2), (6, 2, 1), (6, 0, 1)])}


def named_shapes():
    """{name: shape} of common/src/game/shapes.h (the same in both games)."""
    import re
    text = (PORT / 'common/src/game/shapes.h').read_text()
    return {name: int(value, 0) for name, value in re.findall(r'constexpr int (SHAPE_\w+) = (0x[0-9a-fA-F]+|\d+);', text)}


def shape_name(shape):
    """A shape's name for a test: shapes.h's in words, or its number."""
    names = {value: name for name, value in named_shapes().items()}
    return names[shape][len('SHAPE_'):].lower().replace('_', ' ') if shape in names else f'shape {shape:#x}'


def animated_items():
    """One of each animated shape near the start."""
    return [formats.item_record(X0 - 120 + 40 * k, Y0 - 150, 0, shape, frame=1)
            for k, shape in enumerate(sorted(ANIMATED))]
# The test egg's type: its class is 900h + the type (usecode/uprocess.cpp
# classOf), its event 7 runs when the Silencer comes into its range.
TEST_EGG_TYPE = 0x1c
# The camera's egg (world.py camera_egg), at the start of each scene begun
# by -warp: its event 7 moves the camera to the Silencer.
CAMERA_EGG_TYPE = 0x1e
TEST_MAP = 100
X0, Y0 = 16384, 16384


@dataclass
class Scene:
    name: str
    # {map: [ItemRecord bytes]}
    maps: dict
    # {class: (name, {event: code})}
    classes: dict
    arguments: list
    frames: int
    # the tests the scene runs, for the report
    tests: list = field(default_factory=list)
    # which of them ran, from a replay's Record: for each test True when
    # its footprint is there, False when it is not, None when nothing the
    # records keep shows it (an input whose work is in no record)
    ran: object = None
    # the recording's inputs after its date (system/replay.cpp): lines of
    # frame, verb and its words
    inputs: list = field(default_factory=list)
    # the port's own settings (enhanced.cfg's lines), and whether the
    # original can follow the scene: not with the port's own text, keys or
    # screen
    enhanced: list = field(default_factory=list)
    oracle: bool = True
    # the pass the original follows the scene to, 0 for its end: where the
    # port fixes an original's bug the scene runs into (its document says)
    followed_to: int = 0
    # the CD's movies and subtitles ({file name in FLICS: bytes}) and
    # crusader.cfg's lines in place of the same keys'
    movies: dict = field(default_factory=dict)
    config: list = field(default_factory=list)
    # the scene whose records (states and trace) this one's must be: the
    # same scene in English or the 4:3 screen, which the original follows
    same_as: str = ''
    # the actors' animations this scene has in place of world.MOVES's
    # ({anim: (frames, move, flags)}), as speed.py's walk that waits its
    # frames' ticks
    moves: dict = field(default_factory=dict)


# The interval timer's ticks a second, by which the state log keeps the
# virtual time, and a recording's frames a second.
TIMER_HZ = 1193182
FRAME_HZ = 60
# The test class's entry, which drives a scene (the test egg's event 7).
DRIVER = '091C:0007'


class Record:
    """What a replay of a scene left (run.py replay): the state log, a line
    a pass (system/statelog.cpp: the pass, the virtual time, the seed, the
    processes alive, the Silencer's item, place and direction, ...), each
    pass at its frame by its virtual time, and the usecode's trace, a line
    each with its pass (platform/trace.h)."""

    def __init__(self, states, trace, frames=''):
        self.states = [line.split() for line in states.splitlines()]
        self.trace = [line.split() for line in trace.splitlines()]
        # Each movie's frames shown, in the order played (movies_of).
        self.movies = movies_of(frames)
        # Each pass's frame by its virtual time, one a pass where the time
        # goes back (a load gives back its save's).
        self.frames = []
        for state in self.states:
            frame = round(int(state[1]) * FRAME_HZ / TIMER_HZ)
            self.frames.append(frame if not self.frames or frame > self.frames[-1] else self.frames[-1] + 1)
        # The processes the trace tells of: the pass each was made in (its
        # + line, None before the trace) and their kill's line.
        self.processes = []
        made = {}
        for line in self.trace:
            if line[1] == '+':
                made[line[2]] = int(line[0])
            elif line[1] == '-':
                self.processes.append((made.pop(line[2], None), int(line[0]), int(line[4]), ' '.join(line[5:])))

    def between(self, first, last):
        """The state lines of the passes from frame first to before frame
        last, after the one before them (what they change starts there)."""
        low, high = bisect.bisect_left(self.frames, first), bisect.bisect_left(self.frames, last)
        return self.states[max(0, low - 1):high]

    def moved(self, first, last):
        """The Silencer's place or direction changes from frame first to
        before frame last."""
        return len({tuple(state[5:9]) for state in self.between(first, last)}) > 1

    def starters(self, name, first, last):
        """The items of the processes of that name made from frame first to
        before frame last (and ended)."""
        low, high = bisect.bisect_left(self.frames, first), bisect.bisect_left(self.frames, last)
        if low >= high:
            return set()
        passes = (int(self.states[low][0]), int(self.states[high - 1][0]))
        return {of for at, _, of, named in self.processes if at is not None and passes[0] <= at <= passes[1] and
                named == name}

    def made(self, name, first, last):
        """How many processes of that name were made from frame first to
        before frame last (of those that have ended: keep the scene a while
        after the frames counted)."""
        low, high = bisect.bisect_left(self.frames, first), bisect.bisect_left(self.frames, last)
        if low >= high:
            return 0
        passes = (int(self.states[low][0]), int(self.states[high - 1][0]))
        return sum(1 for at, _, _, named in self.processes if at is not None and passes[0] <= at <= passes[1] and
                   named == name)

    def started(self, name, first, last, item=None):
        """A process of that name (of item) is made from frame first to
        before frame last (and has ended)."""
        items = self.starters(name, first, last)
        return bool(items) if item is None else item in items

    def calls(self, number=None, function=DRIVER):
        """The trace's lines of the intrinsic calls from function, of one
        intrinsic's number or of all, in order."""
        return [line for line in self.trace
                if line[1] == 'I' and line[3] == function and (number is None or int(line[4], 16) == number)]

    def state(self, number):
        """The state line of pass number."""
        if not hasattr(self, 'by_pass'):
            self.by_pass = {int(state[0]): state for state in self.states}
        return self.by_pass[number]

    def global_value(self, offset, size=2):
        """A value of the usecode's globals at the run's end (the trace's G
        lines: the bytes each pass changed), size bytes from offset."""
        if not hasattr(self, 'globals_at_end'):
            self.globals_at_end = {}
            for line in self.trace:
                if line[1] == 'G':
                    at = int(line[2], 16)
                    for k, byte in enumerate(line[3:]):
                        self.globals_at_end[at + k] = int(byte, 16)
        return sum(self.globals_at_end.get(offset + k, 0) << 8 * k for k in range(size))


def movies_of(frames):
    """The movies of a frames log (REGRET_FRAMES, system/movieframes.cpp: a
    line for each frame shown, its number and a hash of the screen; a
    "movie" line before a script's movie): [[frame lines]] each, a movie
    beginning at a "movie" line or where the frames start again."""
    movies, last = [], None
    for line in frames.splitlines():
        words = line.split()
        if not words:
            continue
        if words[0] == 'movie':
            movies.append([])
            last = None
            continue
        number = int(words[0])
        if not movies or (last is not None and number <= last):
            movies.append([])
        movies[-1].append(line)
        last = number
    return movies


# The fields of a state line (system/statelog.cpp): the Silencer's hit
# points, the hit points of all the NPCs (the Silencer's among them).
STATE_HP, STATE_ALL_HP = 9, 10


# A test's end in a driver: the family of shape k, k the test's number, an
# intrinsic call that changes nothing and that the trace shows with its
# argument (a load gives the driver back the ends of tests it had passed).
MARKER = '@familyOfType$qi'


def marker(table, k):
    """The lines of the end of a driver's test k."""
    return call(table, MARKER, str(k))


def marks(game, record):
    """{test: the pass of its first marker} of the record's trace."""
    out = {}
    for line in record.calls(intrinsics_table(game)[MARKER][0]):
        out.setdefault(int(line[5], 16) | int(line[6], 16) << 8, int(line[0]))
    return out


class marked:
    """The ran of a scene whose driver ends each test k with marker(k):
    those whose marker is in the trace ran."""

    def __init__(self, game):
        self.game = game

    def __call__(self, record, tests):
        seen = marks(self.game, record)
        return [k in seen for k in range(len(tests))]

    def at(self, record, tests, low, high):
        """The tests the driver was in from pass low to high: those whose
        marker came from low on, up to the first from high on."""
        ends = marks(self.game, record)
        out = []
        for k in sorted(ends):
            if ends[k] >= low and k < len(tests):
                out.append(tests[k])
                if ends[k] >= high:
                    break
        return out


class checked(marked):
    """The ran of a marked scene whose tests show their results in the
    records: test k ran when its marker is in the trace and check(record,
    marks, k) holds (marks: {test: the pass of its marker})."""

    def __init__(self, game, check):
        super().__init__(game)
        self.check = check

    def __call__(self, record, tests):
        seen = marks(self.game, record)
        return [k in seen and bool(self.check(record, seen, k)) for k in range(len(tests))]


def reached(game, k):
    """A footprint (seen's kind): the driver's marker k in the trace, where
    the driver went on after a screen that the keys closed."""
    return lambda record, first, last: k in marks(game, record)


# The keys whose actions move or turn the Silencer, and those of the
# actions he does where he stands (kneel, stand, fire): the footprints of a
# controls scene's actions.
MOVE_KEYS = {'up', 'down', 'left', 'right', 'home', 'end', 'pageup', 'pagedown', 'insert', 'delete', 'kpslash', 'j',
             'kp1', 'kp2', 'kp3', 'kp4', 'kp6', 'kp7', 'kp8', 'kp9'}
ACT_KEYS = {'kp5', 'space'}


def footprint(keys):
    """The footprint of an action of keys held together (Record's): moved,
    acted (an animation of the Silencer started) or None (none that the
    records keep: the weapon's and the items' keys, the game's own). An
    action may have idle instead (no animation of his started: a key
    that does nothing there)."""
    if MOVE_KEYS & set(keys):
        return 'moved'
    if ACT_KEYS & set(keys):
        return 'acted'
    return None


def seen(record, kind, first, last):
    """Whether the footprint kind (one of footprint's, or a function of the
    record and the frames) is in the record from frame first to before
    frame last; None for no kind."""
    if callable(kind):
        return kind(record, first, last)
    if kind == 'moved':
        return record.moved(first, last)
    if kind == 'acted':
        return record.started('AnimPrimitive', first, last, item=1)
    if kind == 'idle':
        return not record.started('AnimPrimitive', first, last, item=1)
    return None


class windows_ran:
    """The ran of a scene of inputs: windows, (first frame, last frame,
    footprint kind) for each test."""

    def __init__(self, windows):
        self.windows = windows

    def __call__(self, record, tests):
        return [seen(record, kind, first, last) for first, last, kind in self.windows]

    def at(self, record, tests, low, high):
        """The tests whose frames meet those of the passes low to high."""
        frames = record.frames[max(0, low - 1):high]
        if not frames:
            return []
        return [test for test, (first, last, _) in zip(tests, self.windows) if first <= frames[-1] and frames[0] < last]


def tests_at(scene, record, low, high):
    """The names of the scene's tests from pass low to high, None when its
    ran cannot tell."""
    at = getattr(scene.ran, 'at', None)
    return None if at is None else at(record, scene.tests, low, high)


def floor_and_egg(size=8):
    """A floor of size by size tiles of 64 around (X0, Y0) and the test egg
    there, its range over the Silencer's start."""
    records = [formats.item_record(X0 - 32 * size + 64 * i + 63, Y0 - 32 * size + 64 * j + 63, 0, FLOOR)
               for i in range(size) for j in range(size)]
    return records + [formats.item_record(X0, Y0, 0, EGG, quality=TEST_EGG_TYPE, npc=0x22)]


def intrinsics_table(game):
    """{name: (number, argument bytes, return bytes)} of the game's
    intrinsics (tables/intrinsics.tsv), the first number of each name."""
    out = {}
    with open(PORT / game / 'tables/intrinsics.tsv') as table:
        for row in csv.DictReader(table, delimiter='\t'):
            if row['name'] != '_main' and row['name'] not in out:
                out[row['name']] = (int(row['number'], 16), int(row['argument bytes']), int(row['return bytes'] or 0))
    return out


def intrinsic_numbers(game):
    """{name: [numbers]} of the game's intrinsics: the usecode calls the
    same function by several numbers (tables/intrinsics.tsv)."""
    out = {}
    with open(PORT / game / 'tables/intrinsics.tsv') as table:
        for row in csv.DictReader(table, delimiter='\t'):
            if row['name'] != '_main':
                out.setdefault(row['name'], []).append(int(row['number'], 16))
    return out


# The intrinsics' suite: the driver's locals, from its frame's base.
LOCALS = {'I': -2, 'S': -4, 'C': -6, 'J': -8, 'W': -10, 'R1': -12, 'R2': -14, 'R3': -16, 'N': -18, 'P': -24}
FRAME = 24
# Each test: an intrinsic's name, its arguments in C's order and 'wait' to
# wait for the process it returns. A pointer to a local: I the test item, J
# the second, C the container, S the Silencer, N a new item, W a word (1),
# R1-R3 ints, P an ItemPoint; E the egg (this). A value: a local's name in
# lower case (i, j, c: the items' numbers); a number a word; 'L:n' a long.
INTRINSIC_TESTS = [
    # items' getters
    ('@Item@frame$qv', 'I'), ('@Item@map$qv', 'I'), ('@Item@flags$qv', 'I'), ('@Item@isOnScreen$qv', 'I'),
    ('@Item@isNpc$qv', 'I'), ('@Item@z$qv', 'I'), ('@Item@qualityLow$qv', 'I'), ('@Item@x$qv', 'I'),
    ('@Item@y$qv', 'I'), ('@Item@shape$qv', 'I'), ('@Item@qualityHigh$qv', 'I'), ('@Item@centreY$qv', 'I'),
    ('@Item@centreX$qv', 'I'), ('@Item@family$qv', 'I'), ('@Item@eggId$qv', 'E'), ('@Item@isInNpc$qv', 'I'),
    ('@Item@npcNumber$qv', 'I'), ('@Item@nothing$qv', 'I'), ('@Item@familyQuality$qv', 'I'),
    ('@Item@eggType$qv', 'E'), ('@Item@inFastArea$qv', 'I'), ('@Item@surfaceWeight$qv', 'I'), ('@Item@q$qv', 'I'),
    ('@Item@centreZ$qv', 'I'), ('@Item@eggXRange$qv', 'E'), ('@Item@overlaps$qus', 'I j'), ('@Item@isOn$qus', 'I j'),
    ('@Item@isCompletelyOn$qus', 'I j'), ('@Item@isCentreOn$qus', 'I j'), ('@Item@directionToItem$qus', 'I j'),
    ('@Item@directionTo$qusus', 'I 16500 16300'), ('@Item@typeFlag$qi', 'I 1'), ('@Item@getPoint$qn9ItemPoint', 'J P'),
    ('@Item@footpad$qnit1t1', 'I R1 R2 R3'),
    # the Silencer's getters
    ('@Npc@direction$qv', 'S'), ('@Npc@lastAnimSet$qv', 'S'), ('@Npc@waypointGroup$qv', 'S'), ('@Npc@hp$qv', 'S'),
    ('@Npc@mana$qv', 'S'), ('@Npc@activity$qv', 'S'), ('@Npc@isDead$qv', 'S'), ('@Npc@isBusy$qv', 'S'),
    ('@Npc@isKneeling$qv', 'S'), ('@Npc@maxEnergy$qv', 'S'), ('@Npc@isInCombat$qv', 'S'),
    ('@Npc@lastActivity$qv', 'S'), ('@Npc@equipment$qi', 'S 0'), ('@Npc@defaultActivity0$qv', 'S'),
    ('@Npc@defaultActivity1$qv', 'S'), ('@Npc@defaultActivity2$qv', 'S'), ('@Npc@isFalling$qv', 'S'),
    ('@Npc@maxHp$qv', 'S'),
    # the world's
    ('@isAlertActive$qv', ''), ('@gameDifficulty$qv', ''), ('@controlledNpcNumber$qv', ''), ('@topEthereal$qv', ''),
    ('@releaseBuild$qv', ''), ('@violenceEnabled$qv', ''), ('@animationsOff$qv', ''), ('@isTeleporting$qv', ''),
    ('@cameraX$qv', ''), ('@cameraY$qv', ''), ('@cameraZ$qv', ''), ('@moveKeyDownRecently$qv', ''),
    ('@currentTimerTick$qv', ''), ('@isHeld$qv', ''), ('@hasKeycard$quc', '1'), ('@isNpcType$qus', f'{ACTOR}'),
    ('@familyOfType$qi', f'{ITEM}'), ('@randomBetween$qii', '1 100'),
    ('@closestDirection$qiiiiiii', f'{X0} {Y0} 16448 16448 16 0 15'), ('@fineDirection$qiiii', f'{X0} {Y0} 16448 16300'),
    ('@canExistAt$qusususus', f'{ITEM} 16500 16500 0'), ('@countItemProcesses$qusi', 'i 0'), ('@npcMap$qnus', 'W'),
    ('@numberString$qus', '42'), ('@isEffectPlaying$qi', '1'), ('@rangeIfVisible$qn4Itemus', 'I j'),
    ('@fireDistance$qn4Itemusuciii', 'S i 0 10 10 0'),
    # making and moving items
    ('@Item@create$qusus', f'N {ITEM} 2'), ('@Item@legalCreateAtCoords$qususususuc', f'N {ITEM} 0 16320 16320 0'),
    ('@Item@legalCreateAtPoint$qususn9ItemPoint', f'N {ITEM} 0 P'), ('@Item@legalMoveToPoint$qn9ItemPointusus', 'I P 0 0'),
    # the test item's setters
    ('@Item@setFrame$qus', 'I 2'), ('@Item@orFlags$qus', 'I 32'), ('@Item@andFlags$qus', 'I 65503'),
    ('@Item@setQualityLow$qus', 'I 5'), ('@Item@setQualityHigh$qus', 'I 6'), ('@Item@setNpcNumber$quc', 'I 3'),
    ('@Item@setShape$qus', f'I {ITEM}'), ('@Item@setQuality$qus', 'I 4660'), ('@Item@setFamilyQuality$qus', 'I 7'),
    ('@Item@setEggXRange$quc', 'I 2'), ('@Item@setQ$qus', 'I 9'), ('@Item@setMap$qi', f'I {TEST_MAP}'),
    ('@Item@setBroken$qv', 'I'), ('@Item@touch$qv', 'I'), ('@Item@push$qv', 'I'), ('@Item@pop$qv', 'I'),
    ('@Item@move$qususuc', 'I 16460 16390 0'), ('@Item@popToCoords$qususuc', 'I 16448 16384 0'),
    ('@Item@enterFastArea$qv', 'I'), ('@Item@cast$qus', 'I 1'), ('@Item@avatarStoleSomething$qus', 'I 0'),
    ('@Item@use$qv', 'I'), ('@Item@unequip$qi', 'I 0'), ('@Item@equip$qi', 'I 0'), ('@fall$qn4Item', 'J'),
    # sounds (no sound card)
    ('@Item@playSound$qi', 'I 1'), ('@Item@playLoop$qi', 'I 1'), ('@Item@stopSounds$qv', 'I'),
    ('@Item@setSoundVolume$qizc', 'I 1 100'), ('@Item@isSoundPlaying$qi', 'I 1'), ('@Item@restartSound$qi', 'I 1'),
    ('@Item@stopSound$qi', 'I 1'), ('@playEffect$qi', '1'),
    # the Silencer's setters
    ('@Npc@setDefaultActivity0$qus', 'S 0'), ('@Npc@setDefaultActivity1$qus', 'S 0'),
    ('@Npc@setDefaultActivity2$qus', 'S 0'), ('@Npc@setMana$qus', 'S 50'), ('@Npc@clearImmortal$qv', 'S'),
    ('@Npc@setImmortal$qv', 'S'), ('@Npc@clrInCombat$qv', 'S'), ('@Npc@setCombatTactic$qus', 'S 0'),
    ('@Npc@setWaypointGroup$qus', 'S 1'), ('@Npc@setHp$qus', 'S 120'), ('@Npc@addHp$qi', 'S 10'),
    ('@Npc@turnToward$qucuc', 'S 2 0'), ('@Npc@setActivity$qus', 'S 0'), ('@Npc@doAnim$qusususuc', 'S 0 0 0 0'),
    # the world's setters and processes
    ('@repaintCamera$qv', ''), ('@resetKeyboardState$qv', ''), ('@pauseMusic$qv', ''), ('@resumeMusic$qv', ''),
    ('@stopMusic$qv', ''), ('@playSong$qi', '1'), ('@setStasis$qv', ''), ('@clearStasis$qv', ''),
    ('@setTeleporting$qv', ''), ('@clearTeleporting$qv', ''), ('@clearKeycards$qv', ''), ('@jumpToBlack$qv', ''),
    ('@jumpToGreyScale$qv', ''), ('@jumpToGamePalette$qv', ''), ('@jumpToWhite$qv', ''),
    ('@jumpToColor$qiii', '10 20 30'), ('@showStatusGump$qv', ''), ('@hideStatusGump$qv', ''),
    ('@updateInventoryDisplay$qv', ''), ('@removeInventoryItem$qus', '849'), ('@suppressMovieRestore$qv', ''),
    ('@allowMovieRestore$qv', ''), ('@clearAlertActive$qv', ''), ('@setAlertActive$qv', ''),
    ('@setFollowedItem$qus', '1'), ('@moveCamera$qususucus', f'{X0} {Y0} 0 {TEST_MAP}'), ('@setHeld$qus', '0'),
    # each fade waited for, as the game's usecode does (No Regret's
    # original's stop)
    ('@fadeOut$qv', '', 'wait'), ('@fadeIn$qv', '', 'wait'), ('@fadeToGamePalette$qus', '5', 'wait'),
    ('@fadeTowardColor$qiiiusus', '0 0 0 5 10', 'wait'),
    ('@createHealer$qv', ''), ('@createBatteryCharger$qv', ''),
    ('@createSpriteProcess$qususususususuc', f'{ITEM} 0 3 10 {X0} {Y0} 0'), ('@killItem$qusi', 'i 0'),
    ('@pickUp$qusuc', 'j 0'), ('@Npc@setEquip$qusus', 'S 0 i'),
    # physics, combat, actors
    ('@hurl$qn4Itemiiii', 'J 10 0 5 4'), ('@Item@shoot$qn9ItemPointusus', 'J P 10 1'),
    ('@receiveHitIntrinsic$qn4Itemusucius', 'J 0 4 255 3'), ('@explodeIntrinsic$qn4Itemiuc', 'J 0 0'),
    ('@fireWeapon$qn4Itemiiizciuc', 'S 48 0 16 4 2 0'), ('@Npc@createActor$qusus', f'N {ACTOR} 0'),
    ('@Npc@teleport$qususucuc', f'S 16400 16400 0 {TEST_MAP}'),
    # the end: containers, destruction
    ('@Item@popToContainer$qus', 'I c'), ('@Item@destroyContents$qv', 'C'), ('@Item@destroy$qv', 'J'),
]
# Left out, for scenes of their own: what ends or leaves the scene or waits
# for the player (playMovie, showComputerText, showKeypad, startMapJump,
# teleportToEgg, playCredits, setControlledNpcNumber, createNpc, setDead).


def argument_code(argument, local_offsets):
    """The tokens that push one argument, and its bytes."""
    if argument == 'E':
        return ['PUSH_LOCAL_LONG 6'], 4
    if argument in local_offsets:
        return [f'PUSH_LOCAL_ADDRESS {local_offsets[argument]}'], 4
    if argument.islower() and argument.upper() in local_offsets:
        return [f'PUSH_LOCAL_WORD {local_offsets[argument.upper()]}'], 2
    if argument.startswith('L:'):
        return [f'PUSH_LONG {int(argument[2:], 0)}'], 4
    return [f'PUSH_WORD {int(argument, 0)}'], 2


def call(table, name, arguments, result=None, local_offsets=None):
    """A call of the intrinsic name with arguments (pushed last first), and
    with result its return into the globals at that offset. local_offsets:
    the names of the caller's locals (LOCALS by default)."""
    local_offsets = LOCALS if local_offsets is None else local_offsets
    number, argument_bytes, returned = table[name]
    lines, size = [], 0
    for argument in reversed(arguments.split()):
        code, n = argument_code(argument, local_offsets)
        lines += code
        size += n
    if size != argument_bytes:
        raise ValueError(f'{name}: {size} bytes of arguments, the table has {argument_bytes}')
    lines += [f'CALL_INTRINSIC {size} {number}', f'MOVE_TOP {-size}']
    if result is not None and returned:
        push = {1: 'PUSH_RETURNED_BYTE', 2: 'PUSH_RETURNED_WORD', 4: 'PUSH_RETURNED_LONG'}[returned]
        lines += [push, f'POP_GLOBAL {result} {returned}']
    return lines


def intrinsics_driver(game):
    """The egg's event 7: the test items, then each test of the game's
    intrinsics a pass apart, its result in the globals at 4 * its index.
    Returns the code and the tests' names."""
    table = intrinsics_table(game)
    lines = [f'ENTER {FRAME}', 'PUSH_WORD 1', f'POP_LOCAL_WORD {LOCALS["S"]}', 'PUSH_WORD 1',
             f'POP_LOCAL_WORD {LOCALS["W"]}']
    for local, shape, frame, x, y in (('I', ITEM, 0, 16448, 16384), ('C', CONTAINER, 0, 16384, 16448),
                                      ('J', ITEM, 1, 16448, 16448)):
        lines += call(table, '@Item@create$qusus', f'{local} {shape} {frame}')
        lines += call(table, '@Item@popToCoords$qususuc', f'{local} {x} {y} 0')
    lines += call(table, '@Item@getPoint$qn9ItemPoint', 'J P')
    lines += ['SUSPEND']
    tests = [test for test in INTRINSIC_TESTS if test[0] in table]
    numbers = intrinsic_numbers(game)
    names = []
    for k, (name, arguments, *how) in enumerate(tests):
        # each of the name's numbers in turn, a pass each
        for number in numbers[name]:
            _, size, returned = table[name]
            each = dict(table, **{name: (number, size, returned)})
            if how == ['wait']:
                # Its process's end: our pid under its pid, IMPLIES 1 1 (as
                # the game's usecode waits for a process it spawns).
                lines += ['PUSH_PID'] + call(each, name, arguments, 4 * k)
                lines += ['PUSH_RETURNED_WORD', 'IMPLIES 1 1', 'POP_RETURNED_WORD']
            else:
                lines += call(each, name, arguments, 4 * k)
            lines += ['SUSPEND']
            names.append(name if number == numbers[name][0] else f'{name}#{number:03X}')
    lines += ['RETURN']
    return '\n'.join(lines), names


def intrinsics(game):
    """Each intrinsic the usecode calls, once by each of its numbers, a pass
    apart.

    Checks: the intrinsics (usecode/intrinsics.cpp and the functions they
        call; <game>/tables/intrinsics.tsv): the items' and the Silencer's
        getters and setters, the world's, the sounds (no sound card),
        making and moving items, the fades, the healer and the charger,
        sprites, physics, combat, actors, containers and destruction.
    Drives: the test egg's event 7 (intrinsics_driver): three test items
        and a container made, then INTRINSIC_TESTS' calls in turn, each
        result into the globals at 4 times its index (the trace's G lines).
    Tests:
        <intrinsic>
            The intrinsic called with its row's arguments, its result in the
            globals; <intrinsic>#<number> the same by another of its
            numbers. Its footprint: its call in the trace.
    Original: Both follow.
    Guards: each fade is waited for, as the games' usecode does. Left out for
        scenes of their own: what ends or leaves the
        scene or waits for the player (playMovie, showComputerText,
        showKeypad, startMapJump, teleportToEgg, playCredits,
        setControlledNpcNumber, createNpc, setDead).
    Traps: a result of 0 leaves no G line (the globals start at 0): pick
        arguments whose result is not 0.
    """
    code, tests = intrinsics_driver(game)
    return Scene(name='intrinsics', maps={TEST_MAP: floor_and_egg()},
                 classes={0x900 + TEST_EGG_TYPE: ('SYNTEGG', {7: ucasm.assemble(code)})},
                 arguments=START, frames=300 + 2 * len(tests), tests=tests,
                 ran=lambda record, tests: [k < len(record.calls()) - SETUP_CALLS for k in range(len(tests))])


# The intrinsics' calls before the tests: three Item::create and
# popToCoords, one getPoint (intrinsics_driver).
SETUP_CALLS = 7
# The game's arguments that start a scene: at (X0, Y0) on the test map.
START = ['-warp', '0', str(X0), str(Y0), '0', '-mapoff', str(TEST_MAP)]


def all_scenes():
    """{name: scene of a game}."""
    import anims
    import autofire
    import breakables
    import cheats
    import controls
    import deaths
    import crowd
    import eggrace
    import hits
    import hover
    import menus
    import movies
    import moves
    import npcs
    import physics
    import pickups
    import ported
    import saves
    import target
    import specials
    import speed
    import tokens
    return {'intrinsics': intrinsics, 'tokens': tokens.scene, 'controls': controls.scene, 'controls-config': controls.configured, 'npcs': npcs.scene, 'npcs-saved': npcs.saved, 'npcs-skill1': npcs.skill1, 'npcs-skill2': npcs.skill2, 'npcs-shapes': npcs.shapes, 'attackers': npcs.attackers, 'tactics-rest': npcs.tactics_rest, 'point-blank': npcs.point_blank,
            'saves': saves.scene} | specials.SCENES | physics.SCENES | pickups.SCENES | cheats.SCENES | anims.SCENES | breakables.SCENES | deaths.SCENES | crowd.SCENES | eggrace.SCENES | hover.SCENES | moves.SCENES | target.SCENES | menus.SCENES | movies.SCENES | ported.SCENES | autofire.SCENES | speed.SCENES | hits.SCENES
