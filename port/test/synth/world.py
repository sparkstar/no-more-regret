"""The synthetic world: a game's install,
CD and a recording that starts a scene, all made from nothing of the
originals'. The pictures are boxes of one colour; what the engine reads as
numbers (frames, boxes, NPC types) is set for what the scenes do."""
import re, shutil, struct
from pathlib import Path

import deaths
import formats
import scenes
import tactics
import ucasm

# The shapes' type flags (item/type.h): the Silencer, the floor, the test
# egg, an item, a container and an actor.
TYPE_FLAGS = {
    scenes.SILENCER: dict(solid=1, x=2, y=2, z=5, npc=1, weight=80),
    scenes.FLOOR: dict(fixed=1, solid=1, land=1, x=4, y=4, z=0),
    scenes.EGG: dict(family=4, x=1, y=1, z=1, draw=0),
    scenes.ITEM: dict(solid=1, x=1, y=1, z=2, weight=5),
    scenes.CONTAINER: dict(solid=1, family=6, x=2, y=2, z=2),
    # the actors occlude and are targets (the reticle's, control.cpp)
    scenes.ACTOR: dict(solid=1, x=2, y=2, z=5, npc=1, weight=80, occludes=1, targetable=1),
    scenes.BARREL: dict(solid=1, x=1, y=1, z=2, weight=10),
    scenes.WAYPOINT: dict(x=1, y=1, z=1, draw=0),
    scenes.TELEPORT_EGG: dict(family=8, x=1, y=1, z=1, draw=0),
    scenes.GLOB_EGG: dict(family=3, draw=0),
    scenes.SNAP_EGG: dict(x=1, y=1, z=1, draw=0),
    scenes.CRATE: dict(solid=1, x=2, y=2, z=2, crate=1, weight=20),
    scenes.PLATFORM: dict(fixed=1, solid=1, land=1, x=4, y=4, z=6),
    scenes.WALL: dict(fixed=1, solid=1, x=1, y=4, z=8),
    scenes.WALL_ALONG: dict(fixed=1, solid=1, x=4, y=1, z=8),
    scenes.SWITCH: dict(fixed=1, solid=1, x=1, y=1, z=2, selectable=1),
    # the shots (game/shapes.h SHAPE_SHOT, sprite.cpp SuperSprite): a solid
    # box of 1 by 1 by 1, as both games' are; a shot without one meets
    # nothing on its way
    scenes.named_shapes()['SHAPE_SHOT']: dict(solid=1, x=1, y=1, z=1),
}
# The NPC types (npc.dat). The Silencer's must be there: the status bar
# divides by its hit points. The test actor's carries the second weapon of
# the tables (the BA-41 in both games) or the AR-7 when armed.
NPC_TYPES = {0: dict(shape=scenes.SILENCER, maxHp=150),
             1: dict(shape=scenes.ACTOR, maxHp=50, weaponType=2, weaponType2=8, defaultActivity0=1,
                     defaultActivity1=2, defaultActivity2=3, weapons=(2, 8, 3))}
# the shapes the program knows by number, as the test actor
NPC_TYPES |= {2 + k: dict(NPC_TYPES[1], shape=shape) for k, shape in enumerate(scenes.NPC_SHAPES)}
# deaths.py's NPCs, as the test actor
NPC_TYPES |= {2 + len(scenes.NPC_SHAPES) + k: dict(NPC_TYPES[1], shape=shape)
              for k, shape in enumerate(deaths.ACTORS + deaths.STILL)}
# npcs.py's attackers, as the test actor
NPC_TYPES |= {2 + len(scenes.NPC_SHAPES) + len(deaths.ACTORS + deaths.STILL) + k: dict(NPC_TYPES[1], shape=shape)
              for k, shape in enumerate(scenes.SPEAKER_SHAPES)}
# hits.py's targets: their hit points, no weapon, no activity (they stand)
NPC_TYPES |= {len(NPC_TYPES) + k: dict(shape=shape, maxHp=scenes.TARGET_HP) for k, shape in enumerate(scenes.TARGETS)}
# How the shapes take damage (damage.flx): the test item takes several hits
# and turns frames, the container explodes and goes, the barrel becomes a
# test item.
DAMAGE = {scenes.ITEM: dict(damageable=1, durable=1, points=10, frames=(1, 2, 0), sound=3),
          scenes.CONTAINER: dict(damageable=1, explosion=1, explosion_damages=1, explosion_destroys=1, points=5),
          scenes.BARREL: dict(damageable=1, replace=1, becomes=(scenes.ITEM, 3), points=1),
          # breakables.py's: at the first hit with a step of frame; durable
          # with absolute frames; durable to nothing (destroyed); the
          # explosions 2 (with damage) and 3 (destroying); replaced when
          # worn down; and one whose hits go to its usecode
          310: dict(damageable=1, frames=(1, 0, 0), sound=5),
          311: dict(damageable=1, durable=1, absolute=1, frames=(4, 5, 6), points=20),
          312: dict(damageable=1, durable=1, points=10),
          313: dict(damageable=1, explosion=2, explosion_damages=1, points=1),
          314: dict(damageable=1, explosion=3, explosion_destroys=1, points=1),
          315: dict(damageable=1, durable=1, replace=1, becomes=(scenes.BARREL, 2), points=5),
          316: dict(damageable=1, durable=1, frames=(2, 1, 0), points=8)}
SHAPES = 0x800
# Frames of every shape: the sprites of the intrinsics' suite run past 32,
# fire types 11h and 14h's shots to direction * 6 + 78h and direction * 3 +
# DCh (crusader.cpp fireWeapon).
SHAPE_FRAMES = 512
# The usecode's globals (unkds.dat), where the scenes write their results.
GLOBALS = 1024
# crusader.cfg as INSTALL.EXE writes it for the port's sound card
# (tools/install.py --sound: a Sound Blaster 16, the sound and the music
# on), without the movies.
SOUND_CARD = ['soundcard=3', 'sound=on', 'music=on', 'port=0x220', 'irq=7', 'dma=5']
CONFIG = {
    'regret': ['// CRUSADER NO REMORSE Configuration File.', *SOUND_CARD, 'moviehicolor=false', 'control=0',
               'cdletter=D', 'flicpath=flics'],
    'remorse': ['// CRUSADER NO REMORSE Configuration File.', *SOUND_CARD, 'control=0', 'cdletter=D',
                'flicpath=flics', 'fullinstall=true'],
}
# The sound effects (SOUND.FLX, a TagFlex as the games' are: No Remorse reads
# its names): 1-3FFh, every fourth loops.
SOUNDS = 1024


def song_names(game):
    """The game's songs' file names (<game>/src/sound/music_game.h's
    SONG_NAMES, CHRISTMAS_SONGS and SONG_TURNS): the CD has a file for each,
    as the game's does (No Remorse's original loads a missing one, No
    Regret's and the port's do not)."""
    text = (scenes.PORT / game / 'src/sound/music_game.h').read_text()
    names = set()
    for table in ('SONG_NAMES', 'CHRISTMAS_SONGS', 'SONG_TURNS'):
        declaration = re.search(table + r'[^=;{]*(?:=\s*)?(\{.*?\});', text, re.S)
        if declaration:
            names.update(re.findall(r'"(\w+)"', declaration.group(1)))
    return sorted(names)


# The animations of the actors' shapes (game/animprim.h's numbers, 40h of
# them): {anim: (frames a direction, move along the direction a frame in
# half units, flags)}; the rest stand still for 4 frames. Moves keep to the
# floor; rotated ones go sideways; attacks hit at their middle frame.
MOVES = {
    0x01: (8, 8, {'looping': 1, 'two_step': 1}), 0x03: (8, 16, {'looping': 1, 'two_step': 1}), 0x05: (6, 12, {}),
    0x22: (4, 8, {}), 0x23: (4, 8, {}), 0x26: (4, 8, {}), 0x24: (6, 8, {}), 0x2c: (6, 8, {}),
    0x30: (8, 16, {'looping': 1, 'two_step': 1}), 0x31: (8, 16, {'looping': 1, 'two_step': 1}), 0x34: (6, 6, {}),
    0x38: (8, 16, {}), 0x1e: (4, 0, {'repeat': 2}), 0x1f: (4, 0, {'repeat': 2}),
    0x3a: (6, 12, {}), 0x02: (6, -8, {}), 0x2d: (6, -8, {}), 0x33: (6, -6, {}), 0x1c: (6, -8, {'rotated': 1}),
    0x1d: (6, 8, {'rotated': 1}), 0x1b: (6, 6, {'rotated': 1}), 0x35: (6, -6, {'rotated': 1}),
    0x17: (6, -12, {'rotated': 1, 'unstoppable': 1}), 0x18: (6, 12, {'rotated': 1, 'unstoppable': 1}),
    0x3d: (6, -12, {'rotated': 1, 'unstoppable': 1}), 0x3e: (6, 12, {'rotated': 1, 'unstoppable': 1}),
    0x12: (6, -4, {}), 0x14: (6, 4, {}),
    0x00: (1, 0, {}), 0x04: (1, 0, {}), 0x25: (1, 0, {}), 0x2e: (1, 0, {}), 0x2f: (1, 0, {}), 0x3c: (1, 0, {}),
    0x08: (4, 0, {'attack': 1}), 0x0d: (4, 0, {'attack': 1}), 0x1a: (4, 0, {'attack': 1}),
    0x2a: (4, 0, {'attack': 1}), 0x2b: (4, 0, {'attack': 1}), 0x32: (4, 0, {'attack': 1}),
    0x36: (4, 0, {'attack': 1}), 0x37: (4, 0, {'looping': 1}),
}
# The frames' own flags (formats.anim_frame) by animation, of frame k of
# frames: the steps' sounds (a walk's footstep, a run's NPC sound), the
# rolls' flipped frames, the jump's frames off the ground, and the attacks'
# frames that call the usecode (as the game's animations have them).
def frame_flags(anim, k, frames):
    if anim in (0x01, 0x30) and k == frames // 4:
        return {'sound': 0x40}
    if anim in (0x03, 0x31) and k == frames // 2:
        return {'npc_sound': 1}
    if anim in (0x17, 0x18, 0x3d, 0x3e) and k >= frames // 2:
        return {'flipped': 1}
    if anim == 0x38 and 0 < k < frames - 1:
        return {'on_ground': 0}
    if anim in (0x08, 0x0d) and k == frames - 1:
        return {'usecode': 1}
    return {}


# The shapes that move with them: the Silencer, the test actor, the shapes
# the program knows by number, deaths.py's NPCs and the shapes they die in,
# npcs.py's attackers and hits.py's targets.
ACTORS = (scenes.SILENCER, scenes.ACTOR) + scenes.NPC_SHAPES + deaths.ACTORS + deaths.DEAD + scenes.SPEAKER_SHAPES + scenes.TARGETS


# An attack's point in each of the 8 directions (item/distance.cpp
# directionX, directionY: 0 north, clockwise), in 16s across and 8s up: out
# of the actor's box (32 by 32, its place its far corner) by a shot's box
# (16 by 16), so that a shot starts in the open (crusader.cpp fireWeapon's
# shapeFits), 16 up.
ATTACK_STEPS = {1: 1, 0: 0, -1: -2}
ATTACK_POINTS = [(ATTACK_STEPS[x], ATTACK_STEPS[y], 2)
                 for x, y in zip((0, 1, 1, 1, 0, -1, -1, -1), (-1, -1, 0, 1, 1, 1, 0, -1))]


def animations(moves=None):
    """Each actor's animations (formats.anim_file's tables); moves: a
    scene's own in place of MOVES's."""
    table = MOVES | (moves or {})
    actions = {}
    for anim in range(0x40):
        frames, step, flags = table.get(anim, (4, 0, {}))
        attack = flags.get('attack', 0)
        info = formats.anim_info(frames, **flags)
        # The jump goes up then down; an attack reaches and fires at its
        # middle frame, whose point (ATTACK_POINTS) is the shot's start (as
        # the game's attacks have: fireOffset leaves its caller's point
        # unset without one, and placeReticle's height is then the
        # original's stack's).
        lift = (lambda k: 4 if k < frames // 2 else -4) if anim == 0x38 else (lambda k: 0)
        actions[anim] = (info, [formats.anim_frame((direction * frames + k) % SHAPE_FRAMES, delta_dir=step,
                                                   delta_z=lift(k), **(dict(attack_range=3, dx=ATTACK_POINTS[direction][0],
                                                                            dy=ATTACK_POINTS[direction][1],
                                                                            dz=ATTACK_POINTS[direction][2])
                                                                       if attack and k == frames // 2 else {}),
                                                   **frame_flags(anim, k, frames))
                                for direction in range(8) for k in range(frames)])
    # deaths.py's still shapes cannot fall forwards (ANIM_FALL_FORWARDS)
    still = {anim: action for anim, action in actions.items() if anim != 0x14}
    return {shape: actions for shape in ACTORS} | {shape: still for shape in deaths.STILL}


def game_shapes(game):
    """{name: shape} of the game's named shapes (common/src/game/dtable.h,
    <game>/src/game/dtable_game.h)."""
    shapes = {}
    for path in (scenes.PORT / 'common/src/game/dtable.h', scenes.PORT / game / 'src/game/dtable_game.h'):
        text = path.read_text()
        for name, value in re.findall(r'#define (SHAPE_\w+) (0x[0-9a-fA-F]+|\d+)', text):
            shapes[name] = int(value, 0)
        for name, value in re.findall(r'constexpr int (SHAPE_\w+) = (0x[0-9a-fA-F]+|\d+);', text):
            shapes[name] = int(value, 0)
    return shapes


def table_shapes(game):
    """{family: [shapes]} of the game's tables of weapons, ammunition,
    inventory items and bombs (<game>/src/game/dtable_game.cpp): each
    entry's first field."""
    names = game_shapes(game)
    text = (scenes.PORT / game / 'src/game/dtable_game.cpp').read_text()
    out = {}
    for table, family in (('weaponTypes', 10), ('ammoTypes', 11), ('bombTypes', 12), ('inventoryTypes', 13)):
        body = re.search(table + r'\)\[\w+\] =\s*gameGlobal<[^>]+>\(\{(.*?)\}\);', text, re.S)
        if body is None:
            continue
        found = []
        for first in re.findall(r'\{\s*([A-Z_0-9x]+)\s*,', body.group(1)):
            shape = names.get(first) if first.startswith('SHAPE_') else int(first, 0)
            if shape:
                found.append(shape)
        out[family] = found
    return out


def overlay_weapons(game):
    """The weapons with overlays (<game>/src/game/wpncache_game.cpp's
    overlayWeapons) and their overlay types: [(shape, type)]."""
    names = game_shapes(game)
    text = (scenes.PORT / game / 'src/game/wpncache_game.cpp').read_text()
    return [(names[name], int(kind, 0)) for name, kind in re.findall(r'\{(SHAPE_\w+),\s*(\w+)\}', text)]


def weapon_overlays(game):
    """wpnovlay.dat (game/wpncache.cpp loadOverlays): for each attack
    animation of the Silencer's, an object of each overlay type's frames
    (OverlayFrame: dx, dy, frame) for every direction in turn."""
    types = max(kind for _, kind in overlay_weapons(game)) + 1
    objects = [b''] * 0x100
    for anim in range(0x40):
        frames, _, flags = MOVES.get(anim, (4, 0, {}))
        if flags.get('attack'):
            objects[anim] = b''.join(struct.pack('<bbH', kind - 2, 1 - kind, (k + kind) % SHAPE_FRAMES)
                                     for kind in range(types) for k in range(frames * 8))
    return formats.flex(objects)


def crate(game):
    """The crates' shape: No Regret's shape information marks its crates
    (the crate bit, our CRATE); No Remorse's marks none, and its program
    knows them by their shapes (<game>/src/item/item_game.h CRATE_SHAPES):
    the first of them."""
    listed = re.search(r'CRATE_SHAPES\s*=\s*\{([^}]*)\}', (scenes.PORT / game / 'src/item/item_game.h').read_text())
    return int(listed.group(1).split(',')[0], 0) if listed else scenes.CRATE


# The files every scene has (make): the globs, the credits and the shop.
def globs(game):
    """The globs (glob.flx) by number: their items in half steps from the
    chunk's corner. 5 is empty: its egg makes nothing."""
    return {
        1: [(10, 10, 0, scenes.ITEM, 0), (20, 10, 0, scenes.ITEM, 1), (30, 10, 0, scenes.BARREL, 0),
            (10, 30, 0, scenes.ITEM, 2), (10, 30, 16, scenes.ITEM, 3)],
        2: [(100, 100, 0, scenes.BARREL, 0), (110, 100, 0, scenes.BARREL, 1), (120, 100, 0, scenes.ITEM, 4)],
        3: [(50, 50, 0, scenes.ITEM, k) for k in range(6)] + [(60, 60, 0, crate(game), 0)],
        4: [(120, 120, 0, scenes.ITEM, 7), (130, 120, 0, scenes.BARREL, 2)],
    }


def glob_file(game):
    """glob.flx: the globs by number."""
    objects = [b''] * 16
    for number, entries in globs(game).items():
        objects[number] = formats.glob(entries)
    return formats.flex(objects, 3072)


def credits_script():
    """static\\credits.dat (gump/credits.cpp): ^ fades in, ! a row's y, @ a
    title, $ a line, * a block's end, & a page's fade, # the end."""
    return (b'^!' + struct.pack('<h', 80) + b'@SYNTHETIC\0$THE TEST DATA\0$NOTHING OF THE ORIGINALS\0*&' +
            b'@SECOND PAGE\0$MORE\0*#')


def stuff():
    """static\\stuff.dat (gump/wezzy.h): for each of 16 levels a count and 20
    wares (an id, two words, the shape, the cost, the number, a word)."""
    out = bytearray()
    for level in range(16):
        wares = [(b'W\0\0\0', 0x32f, 100), (b'W\0\0\0', 0x332, 300), (b'I\0\0\0', 0x351, 50),
                 (b'I\0\0\0', 0x33d, 20)]
        record = struct.pack('<H', len(wares))
        for k in range(20):
            if k < len(wares):
                kind, shape, cost = wares[k]
                record += kind + struct.pack('<HHHhHH', 0, 0, shape, cost + level, 1, 0)
            else:
                record += bytes(16)
        out += record
    return bytes(out)


def type_flags(game):
    out = bytearray(SHAPES * 9)
    for family, shapes in table_shapes(game).items():
        for shape in shapes:
            out[shape * 9:shape * 9 + 9] = formats.shape_info(family=family, x=1, y=1, z=1)
    # the keycard and the credits are in no table: an inventory item, and
    # the credits by their quality (Npc::credits reads familyQuality)
    names = game_shapes(game)
    for name, family in (('SHAPE_KEYCARD', 13), ('SHAPE_CREDITS', 1)):
        shape = names[name]
        out[shape * 9:shape * 9 + 9] = formats.shape_info(family=family, x=1, y=1, z=1)
    flags_of = dict(TYPE_FLAGS)
    for shape in scenes.NPC_SHAPES:
        flags_of[shape] = TYPE_FLAGS[scenes.ACTOR]
    for shape in deaths.ACTORS + deaths.DEAD + deaths.STILL + (deaths.WOMAN,):
        flags_of[shape] = dict(TYPE_FLAGS[scenes.ACTOR], death_cry=1)
    for shape in scenes.SPEAKER_SHAPES:
        flags_of[shape] = TYPE_FLAGS[scenes.ACTOR]
    for shape in scenes.TARGETS:
        flags_of[shape] = TYPE_FLAGS[scenes.ACTOR]
    flags_of[scenes.CRAWLING_SPIDER] = dict(solid=1, x=1, y=1, z=1, targetable=1)
    for shape in deaths.WIDE:
        flags_of[shape] = dict(TYPE_FLAGS[scenes.ACTOR], x=4, y=4, death_cry=1)
    for shape in scenes.BREAKABLES:
        flags_of[shape] = dict(solid=1, x=1, y=1, z=2, weight=5)
    for shape in scenes.BIG_SHAPES:
        flags_of[shape] = dict(x=1, y=1, z=1)
    for shape, (kind, data, speed) in scenes.ANIMATED.items():
        flags_of[shape] = dict(solid=1, x=1, y=1, z=2, anim_type=kind, anim_data=data, anim_speed=speed)
    if crate(game) != scenes.CRATE:
        # no crate bit where the game's program does not read it
        flags_of[scenes.CRATE] = {k: v for k, v in TYPE_FLAGS[scenes.CRATE].items() if k != 'crate'}
        flags_of[crate(game)] = flags_of[scenes.CRATE]
    for shape, flags in flags_of.items():
        out[shape * 9:shape * 9 + 9] = formats.shape_info(**flags)
    return bytes(out)


def intrinsic_count(game):
    """The game's intrinsics (tables/intrinsics.tsv): unkcoff.dat has a far
    address for each."""
    return max(max(numbers) for numbers in scenes.intrinsic_numbers(game).values()) + 1


def unkcoff(game, symbols):
    """unkcoff.dat for the original's program: each intrinsic's function
    (tables/intrinsics.tsv) at its address in symbols, the table of the
    original's addresses (name, segment and offset as the link map has them,
    in hexadecimal: the build's symbols_<game>.tsv, which only a tree with
    the recovered sources makes). The original reads a far address a number
    (uprocess.cpp loadCSymbols): the offset, then the program's segment
    number, the map's segment less 1000h over 8 plus 1 (Phar Lap's
    selectors, 8 apart from 1000h). The players' own files are these bytes
    (both games)."""
    addresses = {}
    for line in Path(symbols).read_text().splitlines():
        words = line.split('\t')
        if len(words) >= 3:
            addresses.setdefault(words[0], (int(words[1], 16), int(words[2], 16)))
    rows = [line.split('\t') for line in (scenes.PORT / game / 'tables/intrinsics.tsv').read_text().splitlines()[1:]]
    # every number, _main's last: the original moves the others by its
    # segment
    data = bytearray(4 * (max(int(row[0], 16) for row in rows) + 1))
    for row in rows:
        segment, offset = addresses[row[1]]
        struct.pack_into('<HH', data, 4 * int(row[0], 16), offset, ((segment - 0x1000) >> 3) + 1)
    return bytes(data)


def camera_egg(game, scene):
    """The start's map, an egg at the Silencer's start and its class, for a
    scene begun by -warp; None for one begun otherwise. The original's warp
    teleports him to a new map, whose loading pops him before the view has
    moved there (itemcach.cpp popToCoords's scroll, from the view of the
    map left): the camera's map point is left off the map (32808), and when
    he first walks to the screen's edge the scroll from it puts the fast
    area off the map, where every move of his fails (Sweep::test, his
    start out of the area). The egg's entry (event 7) moves the camera to
    him, on the map loaded (0), as the games' usecode does at a mission's
    start."""
    if '-warp' not in scene.arguments:
        return None
    at = scene.arguments.index('-warp')
    number, x, y, z = (int(word) for word in scene.arguments[at + 1:at + 5])
    if '-mapoff' in scene.arguments:
        number += int(scene.arguments[scene.arguments.index('-mapoff') + 1])
    table = scenes.intrinsics_table(game)
    code = ['ENTER 0'] + scenes.call(table, '@moveCamera$qususucus', f'{x} {y} {z} 0', local_offsets={}) + ['RETURN']
    egg = formats.item_record(x, y, z, scenes.EGG, quality=scenes.CAMERA_EGG_TYPE, npc=0x22)
    return number, egg, ('SYNTCAM', {7: ucasm.assemble('\n'.join(code))})


def make(directory, game, scene, unkcoff=None):
    """directory/install (the hard drive), directory/cd and
    directory/recording for scene of game. unkcoff: the intrinsics' far
    addresses in the original's program, which only the original reads
    (the port's intrinsics go by number): unkcoff()'s bytes for a
    comparison with the original, else zeros."""
    install, cd, recording = directory / 'install', directory / 'cd', directory / 'recording'
    for path in (install / 'STATIC', install / 'USECODE', install / 'SAVEGAME', cd / 'STATIC', recording / 'files'):
        path.mkdir(parents=True, exist_ok=True)
    static, usecode = install / 'STATIC', install / 'USECODE'
    box = formats.frame(6, 6, 3, 3, 100)
    # most shapes a small box; the big ones' frames (of nine sizes, so that
    # the holes they leave fit badly) fill the shape cache (about 11 MB) when
    # sprites go through them (crowd.py)
    small = formats.cached_shape([box] * SHAPE_FRAMES)
    sizes = [formats.frame(120 + 15 * k, 120 + 15 * k, 60, 100, 90) for k in range(9)]
    big = formats.cached_shape([sizes[k % 9] for k in range(SHAPE_FRAMES)])
    (static / 'SHAPES.FLX').write_bytes(formats.flex([big if shape in scenes.BIG_SHAPES else small
                                                      for shape in range(SHAPES)]))
    (static / 'TYPEFLAG.DAT').write_bytes(type_flags(game))
    (static / 'ANIM.DAT').write_bytes(formats.anim_file(animations(scene.moves)))
    (static / 'GLOB.FLX').write_bytes(glob_file(game))
    # Gumps' frames wide enough for their texts: a save slot takes a name
    # only while it fits the slot's width less 5 (loadsave.cpp).
    (static / 'GUMPS.FLX').write_bytes(formats.flex([formats.shape([formats.frame(160, 24, 0, 0, 40)] * 64)] * 150))
    (static / 'FONTS.FLX').write_bytes(formats.flex([formats.shape([formats.frame(6, 8, 0, 0, 15)] * 256)] * 256))
    (static / 'DAMAGE.FLX').write_bytes(formats.damage_table(DAMAGE))
    (static / 'DTABLE.FLX').write_bytes(formats.flex([formats.npc_types(NPC_TYPES)], 32))
    (static / 'COMBAT.DAT').write_bytes(formats.flex(tactics.objects(), 64))
    (static / 'WPNOVLAY.DAT').write_bytes(weapon_overlays(game))
    (static / 'XFORMPAL.DAT').write_bytes(formats.palette_tables())
    (static / 'GAMEPAL.PAL').write_bytes(formats.palette())
    # The screens' pictures and palettes (the menus, the help, the loads
    # and saves, the credits): on the hard drive and on the CD, as the game
    # reads some from each.
    picture = formats.bmp()
    for place in (static, cd / 'STATIC'):
        for name in ('MISC.PAL', 'MISC2.PAL', 'DIFF.PAL', 'STAR.PAL', 'CRED.PAL'):
            (place / name).write_bytes(formats.palette())
        # (No Remorse's help pages are BMPs named HELP1-3.DAT: a page
        # missing is made empty by File::open and read as garbage)
        for name in ('HELP1.BMP', 'HELP2.BMP', 'HELP3.BMP', 'HELP4.BMP', 'HELP5.BMP', 'HELP1.DAT', 'HELP2.DAT',
                     'HELP3.DAT', 'LOAD.BMP', 'SAVE.BMP', 'CRED.DAT'):
            (place / name).write_bytes(picture)
        (place / 'CREDITS.DAT').write_bytes(credits_script())
        (place / 'STUFF.DAT').write_bytes(stuff())
    (static / 'MOUSE.SHP').write_bytes(formats.shape([formats.frame(8, 8, 4, 4, 15)] * 40))
    (static / 'TRIG.DAT').write_bytes(formats.trig())
    classes, scene_maps = dict(scene.classes), dict(scene.maps)
    camera = camera_egg(game, scene)
    if camera is not None:
        number, egg, camera_class = camera
        scene_maps[number] = list(scene_maps[number]) + [egg]
        classes[0x900 + scenes.CAMERA_EGG_TYPE] = camera_class
    (usecode / 'EUSECODE.FLX').write_bytes(formats.usecode_file(classes))
    (usecode / 'UNKCOFF.DAT').write_bytes(unkcoff if unkcoff else bytes(4 * intrinsic_count(game)))
    (usecode / 'UNKDS.DAT').write_bytes(bytes(GLOBALS))
    maps = [b''] * 256
    for number, records in scene_maps.items():
        maps[number] = b''.join(records)
    (cd / 'STATIC' / 'FIXED.DAT').write_bytes(formats.flex(maps))
    (install / 'SOUND').mkdir(exist_ok=True)
    # sizes of 256 to 7424 samples, so that the sound cache (about 300 KB)
    # fills when many are played (sndcache.cpp freeBlocks, compactBlocks)
    sizes = [256 + 1024 * k for k in range(8)]
    once = [formats.sound_effect(size) for size in sizes]
    looped = [formats.sound_effect(size, loop=True) for size in sizes]
    (install / 'SOUND' / 'SOUND.FLX').write_bytes(formats.tagged_flex([looped[n % 8] if n % 4 == 1 else once[n % 8]
                                                                        for n in range(1, SOUNDS)]))
    (cd / 'SOUND').mkdir(exist_ok=True)
    song = formats.amf_song()
    for name in song_names(game):
        (cd / 'SOUND' / f'{name.upper()}.AMF').write_bytes(song)
    config = [line for line in CONFIG[game]
              if line.split('=')[0] not in {extra.split('=')[0] for extra in scene.config}] + scene.config
    (install / 'crusader.cfg').write_bytes(''.join(line + '\r\n' for line in config).encode())
    if scene.movies:
        (cd / 'FLICS').mkdir(exist_ok=True)
        for name, data in scene.movies.items():
            (cd / 'FLICS' / name).write_bytes(data)
    # The recording (system/replay.h): the game and its arguments, the
    # inputs (only the date and the end) and the player's files.
    (recording / 'run.tsv').write_text(f'game\t{game}\narguments\t' + '\t'.join(scene.arguments) + '\n')
    inputs = sorted(scene.inputs, key=lambda line: int(line.split('\t')[0]))
    (recording / 'inputs.tsv').write_text(f'# frame\tinput\tvalues ({scene.name})\n0\tdate\t1996-01-01\n' +
                                          ''.join(line + '\n' for line in inputs) + f'{scene.frames}\tend\n')
    shutil.copytree(install / 'SAVEGAME', recording / 'files' / 'SAVEGAME', dirs_exist_ok=True)
    shutil.copy2(install / 'crusader.cfg', recording / 'files' / 'crusader.cfg')
    if scene.enhanced:
        text = ''.join(line + '\n' for line in scene.enhanced)
        for place in (install, recording / 'files'):
            (place / 'enhanced.cfg').write_text(text)
    return install, cd, recording
