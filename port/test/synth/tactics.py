"""The combat tactics of the synthetic world (static\\combat.dat,
game/attack.cpp): tactic 0 the generic attack (Attack::run runs no script
for it, but Attack::setTactic halts without its entry), 1 a chase of the
target that fires when it is near, 2 the other commands in turn but
KNEEL_FIRE, whose animation (1Ah) Npc::doAnim has not: it halts, in the
originals too (the games' tactics never have it); 3 the rest and each way of
the conditional jumps (npcs.py's tactics-rest)."""
import formats

CHASE = [
    """
    SET_TARGET 1
    top:
    FACE_TARGET
    JUMP_IF_WITHIN_480 near
    WALK_TO_TARGET
    JUMP top
    near:
    LOOP 3
    FIRE_SMALL
    WAIT 20
    END_LOOP
    WALK_HALFWAY
    JUMP_UNLESS_IN_RANGE away
    FIRE_SMALL
    away:
    RESTART
    """,
    """
    TURN_LEFT
    WALK
    TURN_RIGHT
    WALK
    FACE_EAST
    FACE_WEST
    RESTART
    """,
    'WAIT 10\nRESTART',
    'RESTART',
]

EVERYTHING = [
    """
    SET_DATA 5 33000
    ADD_DATA 33000 3
    SUB_DATA 33000 1
    MUL_DATA 33000 4
    DIV_DATA 33000 2
    GET_DIRECTION 33001
    SET_DIRECTION 4
    GET_TARGET_DIRECTION 33002
    FACE_NORTH
    FACE_SOUTH
    FACE_NORTHEAST
    FACE_NORTHWEST
    FACE_SOUTHEAST
    FACE_SOUTHWEST
    KNEELING_ADVANCE
    KNEELING_RETREAT
    KNEELING_SLIDE_LEFT
    WALK_TO_WAYPOINT 1
    WALK_HOME
    JUMP_UNLESS_ONE_IN 2 one
    RUN
    one:
    JUMP_IF_BEYOND_160 two
    RETREAT
    two:
    JUMP_IF_IN_RANGE three
    STAND
    three:
    LOITER 30
    FIRE_LARGE
    MOVE_HOME
    RESTART
    """,
    'WAIT 15\nFIRE_SMALL\nRESTART',
    'SET_TACTIC 1\nRESTART',
    'STOP',
]


# Tactic 3 (npcs.py's tactics-rest, an NPC near the Silencer and one far):
# the commands the others leave and each way of the conditional jumps; the
# far one stands at the end (SET_ACTIVITY), the near one stops.
REST = [
    """
    WALK_TO_WAYPOINT 2
    SET_UNKNOWN59 7
    CLEAR_UNKNOWN59
    WAIT 32001
    JUMP_IF_WITHIN_480 near
    WALK_TO_TARGET
    near:
    JUMP_IF_BEYOND_160 beyond
    STAND
    beyond:
    JUMP_IF_IN_RANGE inrange
    RETREAT
    inrange:
    JUMP_UNLESS_IN_RANGE outrange
    STAND
    outrange:
    JUMP_UNLESS_ONE_IN 1 one
    RUN
    one:
    JUMP last
    STAND
    last:
    RESTART
    """,
    """
    WAIT 30
    JUMP_IF_WITHIN_480 stop
    SET_ACTIVITY 1
    stop:
    STOP
    """,
    'RESTART',
    'RESTART',
]


def objects():
    """combat.dat's objects by tactic number."""
    return [formats.tactic('generic', ['RESTART'] * 4), formats.tactic('chase', CHASE),
            formats.tactic('everything', EVERYTHING), formats.tactic('rest', REST)]
