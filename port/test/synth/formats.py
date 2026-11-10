"""The game's file formats as the port reads them, for data made from nothing
of the originals': each function names the
port's source that reads what it writes."""
import math, struct


def flex(objects, count=None, title=b'synthetic test data'):
    """A FLEX archive (file/flex.h): a 128-byte header (a title padded with
    1Ah, the count, the version 1, the checksum), an (offset, size) per
    object, offset 0 for none, and the objects. Missing objects up to count
    are empty."""
    count = len(objects) if count is None else count
    objects = list(objects) + [b''] * (count - len(objects))
    index, body = bytearray(), bytearray()
    start = 0x80 + 8 * count
    # Equal objects share their bytes: the readers go by each entry's
    # offset and size.
    placed = {}
    for data in objects:
        if data and data not in placed:
            placed[data] = start + len(body)
            body += data
        index += struct.pack('<II', placed[data], len(data)) if data else bytes(8)
    header = bytearray(title.ljust(82, b'\x1a')[:82])
    header += struct.pack('<HiIIi', 0, count, 1, 0, 0) + bytes(28)
    data = header + index + body
    # Flex::checksum: the signed bytes from the header's end to the size
    # less the header's.
    end = len(data) - 0x80
    total = sum(b - 256 if b > 127 else b for b in data[0x80:end]) if end > 0x80 else 0
    struct.pack_into('<i', data, 0x60, (total + 2**31) % 2**32 - 2**31)
    return bytes(data)


def frame(width, height, x, y, colour):
    """A frame of one colour (graph/drawframe.h): its header (flags,
    width, height, the origin), an offset per row from its own entry, and
    each row as runs (a skip, a length, the pixels) that end at the width."""
    row = bytearray()
    left = width
    while left > 0:
        n = min(left, 255)
        row += bytes([0, n]) + bytes([colour]) * n
        left -= n
    rows = 0x14 + 4 * height
    table = b''.join(struct.pack('<I', rows + k * len(row) - (0x14 + 4 * k)) for k in range(height))
    return struct.pack('<Iiiii', 0, width, height, x, y) + table + bytes(row) * height


def _shape(frames, entry):
    """A shape (graph/shape.h): the largest frame's size, the frame count,
    a table entry per frame (entry(offset, frame) gives its bytes and the
    data), then the frames' data; equal frames share theirs."""
    width = max(struct.unpack_from('<i', f, 4)[0] for f in frames)
    height = max(struct.unpack_from('<i', f, 8)[0] for f in frames)
    table, body = bytearray(), bytearray()
    start = 6 + 8 * len(frames)
    placed = {}
    for f in frames:
        if f not in placed:
            placed[f] = entry(start + len(body), f)
            body += placed[f][1]
        offset, data = placed[f]
        table += struct.pack('<Ii', offset, len(data))
    return struct.pack('<HHh', width, height, len(frames)) + bytes(table) + bytes(body)


def shape(frames):
    """A shape of the archives the cache reads whole (GUMPS.FLX, FONTS.FLX,
    MOUSE.SHP; cache/flexhand.cpp): the frames' offsets from the shape's
    start."""
    return _shape(frames, lambda offset, f: (offset, f))


def cached_shape(frames):
    """A shape of SHAPES.FLX (cache/shaphand.cpp, loadFrame): each table
    entry has FRAME_ON_DISK (80000000h) and points at an 8-byte FrameBlock
    (shape, frame, entry) before its frame, which the cache fills in its
    own copy (so the file's are 0); the entry's size counts both."""
    return _shape(frames, lambda offset, f: (0x80000000 | offset, bytes(8) + f))


def trig():
    """static\\trig.dat (system/pfloat.cpp): sine, cosine and tangent in
    16.16 for 256 steps of the circle, computed."""
    out = bytearray()
    for function in (math.sin, math.cos, math.tan):
        for i in range(256):
            value = max(min(function(2 * math.pi * i / 256), 32767), -32768)
            out += struct.pack('<i', round(value * 65536))
    return bytes(out)


def palette():
    """gamepal.pal: 256 colours of 6 bits each."""
    return b''.join(bytes([i >> 2, (i * 3 >> 2) & 63, (255 - i) >> 2]) for i in range(256))


def palette_tables():
    """xformpal.dat (graph/gamepal.cpp, PaletteTables::load): object 0 the
    translucency index (FFh: none) and eight remapping tables (here the
    identity), object 1 32 bytes."""
    return flex([b'\xff' * 256 + bytes(range(256)) * 8, bytes(32)])


def shape_info(fixed=0, solid=0, land=0, family=0, x=0, y=0, z=0, npc=0, draw=1, crate=0, roof=0, weight=0,
               volume=0, selectable=0, occludes=0, targetable=0, anim_type=0, anim_data=0, anim_speed=0, death_cry=0):
    """A shape's 9 bytes of typeflag.dat (item/type.h ShapeInfo): the bits
    from the lowest, then the weight and the volume."""
    bits = (fixed | solid << 1 | land << 3 | occludes << 4 | draw << 8 | roof << 10 | family << 12 | x << 21 |
            y << 26 | z << 31 | anim_type << 36 | anim_data << 40 | anim_speed << 44 | selectable << 49 |
            targetable << 52 | npc << 53 | crate << 54 | death_cry << 55)
    return bits.to_bytes(7, 'little') + bytes([weight, volume])


def item_record(x, y, z, shape, frame=0, flags=0, quality=0, npc=0, map=0):
    """An item of a map in fixed.dat (item/itemdata.h ItemRecord), 16
    bytes."""
    return struct.pack('<HHBHBHHBBH', x, y, z, shape, frame, flags, quality, npc, map, 0)


def npc_types(types):
    """npc.dat, object 0 of dtable.flx (game/dtable.h NpcType, 8Eh bytes,
    40h of them): {index: {field: value}} with shape, maxHp, weaponType,
    weaponType2, defaultActivity0-2 and weapons (three); the others have no
    shape."""
    places = {'maxHp': 2, 'weaponType': 0x18, 'weaponType2': 0x1a, 'defaultActivity0': 0x1e, 'shape': 0x3e,
              'defaultActivity1': 0x40, 'defaultActivity2': 0x42}
    out = bytearray()
    for i in range(0x40):
        entry = bytearray(0x8e)
        fields = dict(shape=0xffff) | types.get(i, {})
        for name, value in fields.items():
            if name == 'weapons':
                entry[0x47:0x4a] = bytes(value)
            else:
                struct.pack_into('<H', entry, places[name], value)
        out += entry
    return bytes(out)


def damage_table(entries, shapes=0x800):
    """damage.flx's object 0 (item/itemdam.h DamageEntry, 6 bytes a shape):
    {shape: dict(damageable, explosion, explosion_damages, durable,
    explosion_destroys, replace, absolute, sound, frames or (shape, frame),
    points)}."""
    out = bytearray(6 * shapes)
    for shape, e in entries.items():
        bits = (e.get('damageable', 0) | e.get('explosion', 0) << 1 | e.get('explosion_damages', 0) << 3 |
                e.get('durable', 0) << 4 | e.get('explosion_destroys', 0) << 5 | e.get('replace', 0) << 6 |
                e.get('absolute', 0) << 7)
        if e.get('replace'):
            middle = struct.pack('<HB', *e['becomes'])
        else:
            middle = bytes(e.get('frames', (0, 0, 0)))
        out[6 * shape:6 * shape + 6] = bytes([bits, e.get('sound', 0)]) + middle + bytes([e.get('points', 0)])
    return flex([bytes(out)])


# The combat tactics' commands (game/attack.cpp) and their words.
TACTICS = {
    'SET_UNKNOWN59': (0x81, 1), 'CLEAR_UNKNOWN59': (0x82, 0), 'SET_TARGET': (0x84, 1), 'WALK': (0x85, 0), 'RUN': (0x86, 0), 'RETREAT': (0x87, 0), 'TURN_LEFT': (0x88, 0),
    'TURN_RIGHT': (0x89, 0), 'FIRE_SMALL': (0x8a, 0), 'FIRE_LARGE': (0x8b, 0), 'STAND': (0x8c, 0),
    'WALK_HOME': (0x8d, 0), 'WALK_TO_TARGET': (0x8e, 0), 'WALK_HALFWAY': (0x8f, 0), 'KNEEL_FIRE': (0x92, 0),
    'WAIT': (0x93, 1), 'LOITER': (0x94, 1), 'FACE_TARGET': (0x95, 0), 'SET_ACTIVITY': (0x96, 1),
    'SET_TACTIC': (0x97, 1), 'MOVE_HOME': (0x98, 0), 'STOP': (0x99, 0), 'JUMP_IF_WITHIN_480': (0x9a, 1),
    'JUMP_IF_BEYOND_160': (0x9b, 1), 'JUMP_UNLESS_IN_RANGE': (0x9c, 1), 'JUMP_IF_IN_RANGE': (0x9d, 1),
    'JUMP_UNLESS_ONE_IN': (0x9e, 2), 'LOOP': (0x9f, 1), 'WALK_TO_WAYPOINT': (0xa6, 1), 'FACE_NORTH': (0xa7, 0),
    'FACE_SOUTH': (0xa8, 0), 'FACE_EAST': (0xa9, 0), 'FACE_WEST': (0xaa, 0), 'FACE_NORTHEAST': (0xab, 0),
    'FACE_NORTHWEST': (0xac, 0), 'FACE_SOUTHEAST': (0xad, 0), 'FACE_SOUTHWEST': (0xae, 0), 'SET_DATA': (0xaf, 2),
    'ADD_DATA': (0xb0, 2), 'SUB_DATA': (0xb1, 2), 'MUL_DATA': (0xb2, 2), 'DIV_DATA': (0xb3, 2),
    'GET_DIRECTION': (0xb4, 1), 'SET_DIRECTION': (0xb5, 1), 'GET_TARGET_DIRECTION': (0xb6, 1),
    'KNEELING_RETREAT': (0xb7, 0), 'KNEELING_ADVANCE': (0xb8, 0), 'KNEELING_SLIDE_LEFT': (0xb9, 0),
    'JUMP': (0xc0, 1), 'END_LOOP': (0xc1, 0), 'RESTART': (0xff, 0),
}


def tactic(name, blocks):
    """A combat tactic (game/attack.h CombatTactic): its name, the start of
    each of its four blocks from the entry's start, and the blocks' scripts.
    blocks: four texts of a command and its words a line ('label:' lines
    mark places a jump's word names; a word at or past 33000 names a data
    word, Attack::data)."""
    lines = [[line.split() for line in text.strip().splitlines() if line.strip()] for text in blocks]
    labels = {}
    for _ in range(2):
        code, starts, found = bytearray(), [], {}
        for block in lines:
            starts.append(0x18 + len(code))
            for words in block:
                if words[0].endswith(':'):
                    found[words[0][:-1]] = 0x18 + len(code)
                    continue
                command, count = TACTICS[words[0]]
                assert len(words) - 1 == count, words
                code.append(command)
                for word in words[1:]:
                    value = labels.get(word, 0) if not word.lstrip('-').isdigit() else int(word)
                    code += struct.pack('<H', value & 0xffff)
        labels = found
    return name.encode()[:15].ljust(16, b'\0') + struct.pack('<4H', *starts) + bytes(code)


def usecode_class(functions):
    """A class's unit (usecode/uprocess.h): ClassHeader (the functions
    beyond the 32 events, a long, codeBase from 1, 8 bytes), a FunctionEntry
    (size, offset from codeBase) for each function, the code. functions:
    {number: code}, 0-31 the events'."""
    beyond = max([number - 31 for number in functions if number >= 32], default=0)
    table, code = bytearray(), bytearray()
    for number in range(32 + beyond):
        if number in functions:
            table += struct.pack('<HI', len(functions[number]), 1 + len(code))
            code += functions[number]
        else:
            table += bytes(6)
    return struct.pack('<III', beyond, 0, 20 + 6 * (32 + beyond)) + bytes(8) + bytes(table) + bytes(code)


def usecode_file(classes, count=0xc00):
    """eusecode.flx (usecode/uprocess.cpp, usecode/unitfile.cpp): object 1
    a ClassInfo (events, name of 9 bytes) per class, class n object n+2.
    classes: {number: (name, {event: code})}."""
    info = bytearray(13 * count)
    objects = [b'', b''] + [b''] * count
    for number, (name, functions) in classes.items():
        struct.pack_into('<I9s', info, 13 * number, sum(1 << event for event in functions if event < 32),
                         name.encode())
        objects[number + 2] = usecode_class(functions)
    objects[1] = bytes(info)
    return flex(objects)


def amf_module():
    """A music module (sound/amfload.h): the header ("ASYLUM Music Format
    V1.0", speed, tempo, one sample, one pattern, one order), a pattern of
    64 empty rows of 8 channels, and the sample's 32 bytes of silence."""
    header = bytearray(0xa66)
    header[:24] = b'ASYLUM Music Format V1.0'
    header[32:38] = bytes([6, 125, 1, 1, 1, 0])
    sample = struct.pack('<22sBBbIII', b'silence', 0, 64, 0, 32, 0, 0)
    header[0x126:0x126 + len(sample)] = sample
    return bytes(header) + bytes(0x800) + bytes([0x80]) * 32


def amf_song():
    """A music module with notes (common/asylum/asyplay.cpp): two samples (a
    looped square, a falling saw once), three patterns in the orders 0, 1,
    2, 1 and every effect the player has on some channel: arpeggio, slides
    up and down, tone portamento (and with a volume slide), vibrato (and with
    a volume slide), sample offset, volume slide, volume, the E commands
    (cut, retrigger, fine volume up and down, pan), 1Bh's retrigger, speed
    and tempo, a pattern break and a position jump back to the start."""
    header = bytearray(0xa66)
    header[:24] = b'ASYLUM Music Format V1.0'
    orders = [0, 1, 2, 1]
    header[32:38] = bytes([6, 125, 2, 3, len(orders), 0])
    header[38:38 + len(orders)] = bytes(orders)
    square = bytes([0x30] * 16 + [0xd0] * 16) * 2
    saw = bytes((0x60 - k // 4) & 0xff for k in range(512))
    samples = [(b'square', 0, 64, 0, square, 0, len(square)), (b'saw', 0, 48, 2, saw, 0, 0)]
    for k, (name, finetune, volume, transpose, data, loop, loop_length) in enumerate(samples):
        struct.pack_into('<22sBBbIII', header, 0x126 + 0x25 * k, name, finetune, volume, transpose, len(data), loop,
                         loop_length)
    effects = [
        [(0, 0x37), (0xc, 0x30), (1, 0x04), (2, 0x04), (4, 0x48), (0xa, 0x0f), (0xa, 0xf0), (9, 0x01)],
        [(3, 0x08), (5, 0x02), (6, 0x20), (0xe, 0xc2), (0xe, 0x93), (0xe, 0xa4), (0xe, 0xb4), (0xe, 0x88)],
        [(0x1b, 0x13), (0x1b, 0), (3, 0), (4, 0), (9, 0), (0xf, 4), (0xf, 0x90), (0xc, 0x40)],
    ]
    patterns = bytearray()
    for number in range(3):
        pattern = bytearray(0x800)
        for row in range(64):
            for channel in range(8):
                at = (row * 8 + channel) * 4
                if (row + channel) % 4 == 0:
                    pattern[at] = 13 + (row * 5 + channel * 7) % 48
                    pattern[at + 1] = 1 + (row + channel) % 2
                command, param = effects[number][(row // 4 + channel) % 8]
                if row % 2 == 0 or command in (3, 4, 5, 6, 0, 1, 2, 0xa):
                    pattern[at + 2], pattern[at + 3] = command, param
        if number == 1:
            pattern[40 * 32 + 2:40 * 32 + 4] = bytes([0xd, 0x10])
        if number == 2:
            pattern[63 * 32 + 2:63 * 32 + 4] = bytes([0xb, 0])
        patterns += pattern
    return bytes(header) + bytes(patterns) + b''.join(sample[4] for sample in samples)


def anim_info(frames, two_step=0, attack=0, looping=0, unstoppable=0, end_loop=0, repeat=0, rotated=0,
              sixteen=0, destroy=0):
    """An animation's header (game/astream.h AnimInfo): 32 bits from the
    lowest."""
    bits = (frames | two_step << 8 | attack << 9 | looping << 10 | unstoppable << 11 | end_loop << 14 |
            repeat << 16 | rotated << 20 | sixteen << 30 | destroy << 31)
    return struct.pack('<I', bits)


def anim_frame(frame, delta_dir=0, delta_z=0, on_ground=1, attack_range=0, usecode=0, npc_sound=0, sound=0,
               flipped=0, dx=0, dy=0, dz=0):
    """A frame of an animation (game/animfrm.h AnimFrame): 64 bits from the
    lowest; the signed fields in two's complement of their widths. dx, dy
    and dz: the frame's point (a shot's start, explode.cpp fireOffset)."""
    bits = ((frame & 0xfff) | npc_sound << 12 | usecode << 14 | flipped << 15 | (delta_z & 0xff) << 16 |
            (sound & 0xff) << 24 | (delta_dir & 0x1ff) << 32 | on_ground << 41 | attack_range << 42 |
            (dx & 0x1f) << 46 | (dy & 0x1f) << 51 | (dz & 0xf) << 56 | (sound >> 8 & 3) << 62)
    return struct.pack('<Q', bits)


def anim_file(tables, shapes=0x800):
    """anim.dat (game/animfile.cpp): an offset per shape to its table of
    100h offsets, one per animation, each to the animation's header and its
    frames (each direction's in turn). tables: {shape: {anim: (info, [frames
    of every direction])}}; equal tables share their bytes."""
    head = bytearray(4 * shapes)
    body = bytearray()
    placed = {}
    for shape, actions in sorted(tables.items()):
        key = tuple((anim, info, tuple(frames)) for anim, (info, frames) in sorted(actions.items()))
        if key not in placed:
            table = bytearray(0x400)
            start = len(head) + len(body)
            data = bytearray()
            for anim, (info, frames) in sorted(actions.items()):
                struct.pack_into('<i', table, 4 * anim, start + 0x400 + len(data))
                data += info + b''.join(frames)
            placed[key] = start
            body += table + data
        struct.pack_into('<i', head, 4 * shape, placed[key])
    return bytes(head + body)


def bmp(width=640, height=480, colour=1):
    """An 8-bit BMP (graph/bmp.h): the file's header, the information
    header, 256 colours and the rows from the bottom, each padded to 4
    bytes; a frame of colour around colour 0."""
    pitch = (width + 3) // 4 * 4
    rows = bytearray()
    for y in range(height):
        edge = y in (0, height - 1)
        row = bytes([colour if edge or x in (0, width - 1) else 0 for x in range(width)])
        rows += row + bytes(pitch - width)
    colours = b''.join(bytes([i, i, i, 0]) for i in range(256))
    offset = 14 + 40 + len(colours)
    return (b'BM' + struct.pack('<IHHI', offset + len(rows), 0, 0, offset) +
            struct.pack('<IiiHHIIiiII', 40, width, height, 1, 8, 0, len(rows), 0, 0, 256, 0) + colours + bytes(rows))


def sound_effect(samples=256, rate=11025, loop=False):
    """A sound effect of SOUND.FLX (sound/sndcache.h SoundEffect): "ASFX",
    the samples' length, the loop's start and length (0: once), a word, the
    rate, then 8-bit samples (here a quiet saw)."""
    data = bytes((128 + (i % 32) - 16) & 0xff for i in range(samples))
    return b'ASFX' + struct.pack('<IIIHH', samples, 0, samples if loop else 0, 0, rate) + data


def tagged_flex(objects, tag=0x31e28309):
    """A FLEX of named objects (file/tagflex.cpp TagFlex, "LockTagFlex"):
    object 0 a record per other object, its tag (a long; the load would take
    the clock's time for a title other than LockTagFlex's) and its name
    with a 0, then the objects from 1."""
    names = b''.join(struct.pack('<I', tag) + f'SFX{n:04d}'.encode() + b'\0' for n in range(1, len(objects) + 1))
    return flex([names] + list(objects), title=b'LockTagFlex\0')


def glob(entries):
    """A glob of glob.flx (item/globs.h): its count, then each item's place
    in half steps from its chunk's corner, its shape and frame:
    [(x, y, z, shape, frame)]."""
    return struct.pack('<H', len(entries)) + b''.join(struct.pack('<BBBHB', *entry) for entry in entries)


# The movies (movie/avifile.h, rrv.cpp, xan/): AVI files of our own frames.

def riff_chunk(tag, data):
    """A RIFF chunk: its id, size and data, padded to an even size."""
    return tag + struct.pack('<I', len(data)) + data + (b'\0' if len(data) & 1 else b'')


def riff_list(kind, chunks):
    return riff_chunk(b'LIST', kind + b''.join(chunks))


def stream_header(kind, handler, length, rect=(0, 0, 0, 0)):
    """'strh': the type, the handler, flags, priority and language, the
    initial frames, scale and rate (15 a second), start, length, buffer
    size, quality, sample size and the frame's rectangle."""
    return (kind + handler + struct.pack('<IHHIIIIIIII', 0, 0, 0, 0, 1, 15, 0, length, 0, 0xffffffff, 0) +
            struct.pack('<4h', *rect))


def wave_format(tag, channels, rate, bits, align=None, per_second=None):
    """The 'strf' of a sound stream (sound/audstrm.h WaveFormat)."""
    align = align if align is not None else bits // 8 * channels
    per_second = per_second if per_second is not None else rate * align
    return struct.pack('<HhiihH', tag, channels, rate, per_second, align, bits)


def avi(width, height, handler, video_format, chunks, move=None, sound=None, microseconds=66667):
    """An AVI file: 'avih' (the frame's time, the frames, the streams, the
    size), the video stream ('strh' with handler, 'strf' video_format, Xan's
    MOVE list of chunks), the sound stream when sound is (handler, format),
    and the movi list of chunks [(id, data)] in order."""
    frames = sum(1 for tag, _ in chunks if tag in (b'00dc', b'00db'))
    streams = 2 if sound else 1
    avih = struct.pack('<10I', microseconds, 0, 0, 0, frames, 0, streams, 0, width, height) + bytes(16)
    video = [riff_chunk(b'strh', stream_header(b'vids', handler, frames, (0, 0, width, height))),
             riff_chunk(b'strf', video_format)]
    if move is not None:
        video.append(riff_list(b'MOVE', [riff_chunk(tag, data) for tag, data in move]))
    lists = [riff_chunk(b'avih', avih), riff_list(b'strl', video)]
    if sound is not None:
        sound_handler, sound_format = sound
        audio = sum(1 for tag, _ in chunks if tag == b'01wb')
        lists.append(riff_list(b'strl', [riff_chunk(b'strh', stream_header(b'auds', sound_handler, audio)),
                                         riff_chunk(b'strf', sound_format)]))
    body = b'AVI ' + riff_list(b'hdrl', lists) + riff_list(b'movi', [riff_chunk(tag, data) for tag, data in chunks])
    return b'RIFF' + struct.pack('<I', len(body)) + body


def bitmap_format(width, height, colours=64):
    """A video stream's 'strf' (rrv.cpp BitmapFormat): the bitmap's header,
    then the palette (blue, green, red, 0): colours of the movie's own."""
    header = struct.pack('<IiiHHIIiiII', 40, width, height, 1, 8, 0, width * height, 0, 0, colours, colours)
    first = (256 - colours) // 2
    palette = b''.join(bytes([(i * 4) & 0xff, (i * 2) & 0xff, (255 - i) & 0xff, 0]) if first <= i < first + colours
                       else bytes(4) for i in range(256))
    return header + palette


class Bits:
    """Bits from the top of each byte, in order (movie/bitfield.cpp)."""

    def __init__(self):
        self.out, self.count = bytearray(), 0

    def put(self, value, width):
        for k in range(width - 1, -1, -1):
            if self.count % 8 == 0:
                self.out.append(0)
            self.out[-1] |= ((value >> k) & 1) << (7 - self.count % 8)
            self.count += 1

    def data(self):
        return bytes(self.out) + bytes(8)


# RRV's run codes: an index whose extra bits are not 0 wide (3, 7, 11 and 15
# read none and take a stray word); 0 a run of 0.
RRV_CODES = [(0x100, 9, 8), (0, 5, 16)]


def rrv_run(bits, run):
    if run == 0:
        bits.put(0, 4)
        return
    for base, index, extra in RRV_CODES:
        if base <= run < base + (1 << extra) and (base or run >= 0x200):
            bits.put(index, 4)
            bits.put(run - base, extra)
            return
    bits.put(5, 4)
    bits.put(run, 16)


def rrv_frame(width, height, slices, pixels_of, pairs=False, unchanged=()):
    """An RRV frame (rrv.cpp): the offsets of the slices from the chunk's
    start, each slice the offset of its pixels from its codes, then the
    runs from a skip (skips and copies by turns), then the pixels.
    pixels_of(slice) gives the slice's runs [(skip, copy pixels)]; pairs
    halves the runs and the pixels (each pixel shown twice across);
    unchanged slices have no codes."""
    lines = height // slices
    bodies = []
    for index in range(slices):
        if index in unchanged:
            bodies.append(struct.pack('<i', 0))
            continue
        bits, pixels = Bits(), bytearray()
        for skip, copy in pixels_of(index, width * lines):
            rrv_run(bits, skip // 2 if pairs else skip)
            rrv_run(bits, len(copy) // 2 if pairs else len(copy))
            pixels += copy[::2] if pairs else copy
        codes = bits.data()
        bodies.append(struct.pack('<i', len(codes)) + codes + bytes(pixels) + bytes(4))
    offsets, at = [], 4 * slices
    for body in bodies:
        offsets.append(at)
        at += len(body)
    return struct.pack(f'<{slices}I', *offsets) + b''.join(bodies)


def lz_literal(data):
    """xan/xanunpack.cpp's packing of data as literal runs (E0h + n: 4n + 4
    bytes, up to 70h), then the end (FCh)."""
    out, at = bytearray(), 0
    while len(data) - at >= 4:
        n = min((len(data) - at) // 4, 28)
        out.append(0xe0 | (n - 1))
        out += data[at:at + 4 * n]
        at += 4 * n
    tail = data[at:]
    out.append(0xfc | len(tail))
    out += tail
    return bytes(out) + bytes(4)


def lz_repeat(head, total):
    """head, then copies of it from len(head) back (the C0h form, up to 1028
    bytes a copy) to total bytes; the end."""
    out = bytearray(lz_literal(head)[:-5])
    made, distance = len(head), len(head) - 1
    while made < total:
        count = min(1028, total - made)
        if count < 5:
            out.append(0xe0)
            out += head[:4]
            made += 4
            continue
        high, rest = (count - 5) >> 8, (count - 5) & 0xff
        out += bytes([0xc0 | ((distance >> 16) << 4) | (high << 2), (distance >> 8) & 0xff, distance & 0xff, rest])
        made += count
    return bytes(out) + bytes([0xfc]) + bytes(4)


def xan_huffman(symbols, alphabet=4):
    """xan/xanunpack.cpp huffman's input for symbols below alphabet (4): the
    pairs' count, the end's code (alphabet), the pairs (node eof + 1 first,
    the root last) and the codes from the top of each byte: 2 bits each,
    11 the end."""
    eof = alphabet
    # nodes eof+1: (0, 1), eof+2: (2, 3), eof+3 (the root): (eof+1, eof+4)
    # with eof+4: (alphabet... ) kept simple: the root's right is a node
    # (eof+2) whose right is the end
    pairs = [(0, 1), (2, eof), (eof + 1, eof + 2)]
    bits = Bits()
    for symbol in symbols:
        bits.put({0: 0b00, 1: 0b01, 2: 0b10}[symbol], 2)
    bits.put(0b11, 2)
    return bytes([len(pairs), eof]) + b''.join(bytes(pair) for pair in pairs) + bits.data()


def xan_frame(width, height, kind, steps, chroma=None, corrections=None, second=None):
    """A Xan frame (xan/xandecode.cpp): its type (0 a whole frame, 1 a
    change), the chroma's place from +4 (0 for none), then the luma: the
    corrections' offsets from +8 (type 1 two), the Huffman codes of the
    steps (width / 2 for each row, symbols 0-2), then the corrections (LZ)
    and the chroma (mode, the table's count, its words, LZ indexes)."""
    codes = xan_huffman(steps)
    heads = 8 if kind == 1 else 4
    luma = bytearray(bytes(heads) + codes)
    first = second_at = 0
    if corrections is not None:
        first = len(luma)
        luma += lz_literal(corrections)
    if second is not None:
        second_at = len(luma)
        luma += lz_literal(second)
    struct.pack_into('<I', luma, 0, first)
    if kind == 1:
        struct.pack_into('<I', luma, 4, second_at)
    chroma_at = 0
    body = bytearray(struct.pack('<II', kind, 0) + luma)
    if chroma is not None:
        mode, table, indexes = chroma
        chroma_at = len(body) - 4
        body += struct.pack('<HH', mode, len(table)) + struct.pack(f'<{len(table)}H', *table) + lz_literal(indexes)
    struct.pack_into('<I', body, 4, chroma_at)
    return bytes(body)


def xan_move(bits=4):
    """Xan's MOVE list: '_PC_' (the colour's bits), 'BEST' (the 8-bit
    colour table, LZ: chroma x luma to the palette: a ramp over the luma)
    and 'PAL8' (the palette)."""
    ramp = bytes(16 + (luma * 3) % 224 for luma in range(64))
    palette = b''.join(bytes([i >> 2, (i >> 1) & 0x3f, (255 - i) >> 2]) for i in range(256))
    return [(b'_PC_', struct.pack('<I', bits)), (b'BEST', lz_repeat(ramp, 0x10000)), (b'PAL8', palette)]


def iff_subtitles(lines):
    """No Regret's subtitles (movie/subtitles.cpp readIffSubtitles): FORM
    with an "ENGL" chunk (its size big-endian) of lines: the frame (a
    dword) and the text's length and text; a negative frame ends."""
    body = b''.join(struct.pack('<iB', frame, len(text)) + text for frame, text in lines) + struct.pack('<iB', -1, 0)
    chunk = b'ENGL' + struct.pack('>I', len(body)) + body
    return b'FORM' + struct.pack('>I', len(chunk) + 4) + b'SUBT' + chunk


def text_subtitles(lines):
    """No Remorse's subtitles (readTextSubtitles): a marker line "@" with the
    frame from its eighth character, then the line's text."""
    return b''.join(b'@LINE  %d\r\n' % frame + text + b'\r\n' for frame, text in lines)
