/* Code7 (No Regret's 1030, No Remorse's 1020, file name unknown; the
 * addresses are No Regret's): moving the game to another map and starting a
 * new game, the games' differences as each game's values (mapjump_game.h). */
#include "mapjump.h"
#include "gamestate.h"
#include "waits.h"
#include "mapjump_game.h"
#include "alert.h"
#include "asylum.h"
#include "camera.h"
#include "control.h"
#include "cprotmem.h"
#include "crusys.h"
#include "cycle.h"
#include "dispatch.h"
#include "display.h"
#include "dlitem.h"
#include "dtable.h"
#include "egg.h"
#include "fade.h"
#include "flag.h"
#include "gamepal.h"
#include "gravity.h"
#include "gumpshap.h"
#include "item.h"
#include "itemcach.h"
#include "itemdata.h"
#include "itemfind.h"
#include "kdaemon.h"
#include "kernel.h"
#include "mainmenu.h"
#include "music.h"
#include "npc.h"
#include "savegame.h"
#include "scrit.h"
#include "snap.h"
#include "sndcache.h"
#include "sndmgr.h"
#include "sndsrc.h"
#include "spmgr.h"
#include "status.h"
#include "system.h"
#include "systimer.h"
#include "tracker.h"
#include "uprocess.h"
#include "viewport.h"
#include "widesides.h"
#include "world.h"
#include "movie.h"
#include "screen.h"

#include <memory>

/* What keepCarried keeps: the controlled NPC's record and NPC data, its
 * equipment and its items of shape 476h. */
struct CarriedItems {
    ItemRecord npc;
    NpcData npcData;
    ItemRecord equipment[EQUIPMENT_SLOTS_CAPACITY];
    i16 count;
    std::unique_ptr<ItemRecord[]> others;
};
/* BCC's: the NPC's record (10h), its NPC data, a record for each equipment
 * slot, the count and the far pointer (No Regret 37Dh, No Remorse 26Fh). */
ASSERT_SIZE(CarriedItems, 0x10 + NPC_DATA_SIZE + (EQUIPMENT_SLOTS * 0x10) + 2 + 4);

namespace {
/* Its own between the maps. */
std::unique_ptr<CarriedItems> &carried = gameGlobal<std::unique_ptr<CarriedItems>>();

void arriveAtEgg(u16 map, i16 egg, u16 x, u16 y, u8 z, u16 oldMap, u8 sameMap);
}

void moveToEgg(u16 map, i16 egg, u8 /*unused*/)
{
    const u16 x = itemX[controlledNpc.id];
    const u16 y = itemY[controlledNpc.id];
    const u8 z = itemZ[controlledNpc.id];
    const u16 oldMap = npcMap(&controlledNpc.id);
    u8 sameMap = 1;
    if (map != currentMap) {
        sameMap = 0;
        if (worldActive != 0U) {
            controlledNpc.teleport(0, 0, 0, map);
        } else {
            moveCamera(0, 0, 0, map);
        }
        if (waitOpened()) {
            /* The egg after the map's loading's wait. */
            giveRest([map, egg, x, y, z, oldMap, sameMap]() {
                arriveAtEgg(map, egg, x, y, z, oldMap, sameMap);
                return false;
            });
            return;
        }
    }
    arriveAtEgg(map, egg, x, y, z, oldMap, sameMap);
}

namespace {
/* moveToEgg's on the map: the egg, or back where it was. */
void arriveAtEgg(u16 map, i16 egg, u16 x, u16 y, u8 z, u16 oldMap, u8 sameMap)
{
    const AreaSearch search(0, 0, -1, -1,
                            LoopScript(SCRIPT_FAMILY, SCRIPT_INT, FAMILY_TELEPORT_EGG, SCRIPT_EQUAL, SCRIPT_FRAME,
                                       SCRIPT_INT, 1, SCRIPT_EQUAL, SCRIPT_AND, SCRIPT_Q, SCRIPT_INT, 0xff, SCRIPT_AND,
                                       SCRIPT_INT, egg, SCRIPT_EQUAL, SCRIPT_AND, SCRIPT_END),
                            -1);
    if (search.isValid()) {
        Support support;

        if (worldActive != 0U) {
            controlledNpc.teleport(itemX[search.id], itemY[search.id], itemZ[search.id], map);
        } else {
            moveCamera(itemX[search.id], itemY[search.id], itemZ[search.id], map);
        }
        const ItemPoint point(itemX[search.id], itemY[search.id], itemZ[search.id]);
        testSupport(&support, 1, point, 1, controlledNpc.id);
        if (support.standing() == 0U) {
            fall(&controlledNpc);
        }
    } else if (sameMap == 0U) {
        if (worldActive != 0U) {
            controlledNpc.teleport(x, y, z, oldMap);
        } else {
            moveCamera(x, y, z, oldMap);
        }
    }
}
}

void showLoading()
{
    jumpToBlack();
    cycleProcess->paused = 0;
    drawViewport->clear(0);
    hideStatus();
    screenViewport->blit(drawViewport);
    gamePalette->apply();
    const GumpShape logo(GumpShape(7));
    const Rect area = gumpShapes->dimensions(logo.shape, logo.frame);
    const i16 width = SCREEN_WIDTH - RECT_WIDTH(area);
    const i16 height = SCREEN_HEIGHT - RECT_HEIGHT(area);
    const i16 x = width / 2;
    const i16 y = height / 2;
    gumpShapes->draw(x, y, logo.shape, logo.frame, logo.paintMode);
    screenViewport->blit(drawViewport);
}

/* The first map of each mission (-warp). */
i16 (&missionMaps)[17] = gameGlobal<i16[17]>({0, 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 40});

namespace {
void startMenu();
void playDemoIntro();

/* The movies before the start menu from the movie at i: the game's
 * INTRO_MOVIES, INTRO_PAUSE ticks apart, waits of the steps; after the
 * last, the start menu, or with -demo the intro again (playDemoIntro). */
void playIntroFrom(u16 i, bool demo)
{
    if (i == INTRO_MOVIES.size()) {
        if (demo) {
            playDemoIntro();
        } else {
            startMenu();
        }
        return;
    }
    auto next = [i, demo](bool /*cancelled*/) { playIntroFrom(i + 1, demo); };
    if (i != 0 && INTRO_PAUSE != 0) {
        openTimerDelay(INTRO_PAUSE, [i, next]() {
            openMovie(INTRO_MOVIES[i], next);
            return false;
        });
        return;
    }
    openMovie(INTRO_MOVIES[i], next);
}

void startChosen(i16 result);

/* -demo's: the intro again until a key comes, then the start menu. */
void playDemoIntro()
{
    if (static_cast<unsigned int>(gumpKeys.isEmpty()) == 0U) {
        startMenu();
        return;
    }
    playIntroFrom(0, true);
}
}

void startGame()
{
    unknown1047();
    jumpToBlack();
    snapProcess->clearEggs();
    setControlledNpc(1);
    keyDaemon->setRate(45);
    hideStatus();
    freezeWorld(1);
    /* The port's wide screen: the intro and the start menu without the
     * world's sides (widesides.h), drawn again when a game goes on. */
    coverWideSides();
    jumpToBlack();
    cycleProcess->eventMask &= ~EVENT_TIMER;
    if (warpMission == -1) {
        if (demoMode != 0U) {
            gumpKeys.clear();
            playDemoIntro();
            return;
        }
        playIntroFrom(0, false);
        return;
    }
    startMenu();
}

namespace {
/* startGame's after the intro: the start menu. */
void startMenu()
{
    if (START_HOLDS_SILENCER) {
        drawViewport->clear(0);
        screenViewport->blit(drawViewport);
        freezeWorld(1);
    }
    initTracker();
    atStartMenu = 1;
    if (START_HOLDS_SILENCER) {
        controlledNpc.held = 1;
        setTeleporting();
    }
    if (warpMission == -1) {
        /* The start menu's choice is done after it has closed. */
        openModalGump(new MainMenuGump, static_cast<u32>(-1),
                      [](s32 chosen) { startChosen(static_cast<i16>(chosen)); });
        return;
    }
    startChosen(0);
}

/* startChosen's new game's end, after its moves: the screen, the
 * controlled NPC and the world. */
void newGameStarted()
{
    drawViewport->clear(0);
    screenViewport->blit(drawViewport);
    setFollowedItem(1);
    releaseControlledNpc();
    thawWorld(1);
    showStatus();
}

/* startChosen's -warp's after the nowhere map: to the mission's map (map). */
void warpMoved(i16 map)
{
    setFollowedItem(1);
    unknown1453 = 0;
    if (JUMP_MOVES_TO_EGG) {
        showLoading();
        if (warpX == -1) {
            if (destinationEgg < 0 || destinationEgg == 30) {
                moveToEgg(map, 30, 1);
                whenDone([]() {
                    jumpToBlack();
                    newGameStarted();
                });
                return;
            }
            moveToEgg(map, destinationEgg, 1);
            whenDone([]() {
                playSong(0x45);
                gameCamera->repaint();
                gamePalette->apply();
                cycleProcess->paused = 0;
                newGameStarted();
            });
            return;
        }
        if (destinationEgg < 0) {
            controlledNpc.teleport(warpX, warpY, warpZ, map);
        } else {
            moveToEgg(map, destinationEgg, 1);
        }
        whenDone([]() {
            playSong(0x45);
            gameCamera->repaint();
            gamePalette->apply();
            cycleProcess->paused = 0;
            newGameStarted();
        });
        return;
    }
    if (warpX == -1) {
        if (destinationEgg < 0) {
            teleportToEgg(map, 30, 1);
        } else {
            teleportToEgg(map, destinationEgg, 1);
        }
    } else if (destinationEgg < 0) {
        controlledNpc.teleport(warpX, warpY, warpZ, map);
    } else {
        teleportToEgg(map, destinationEgg, 1);
    }
    whenDone([]() {
        gameCamera->repaint();
        gamePalette->apply();
        cycleProcess->paused = 0;
        newGameStarted();
    });
}

/* startGame's after the start menu: its choice, result (0 with -warp). */
void startChosen(i16 result)
{
    /* The port's: a saved game loadGame cannot take, said over a black
     * screen in the game's palette (the menu has left its own black), and
     * the start menu again (savegame.h). */
    if (result == MAINMENU_LOAD && !savedGameLoadable()) {
        drawViewport->clear(0);
        screenViewport->blit(drawViewport);
        gamePalette->apply();
        refuseSavedGame([]() { startMenu(); });
        return;
    }
    atStartMenu = 0;
    stopMusic();
    cycleProcess->eventMask |= EVENT_TIMER;
    allowMovieRestore();
    if (result == MAINMENU_NEW_GAME) {
        killAll();
        worldActive = 1;
        alertOn = 0;
        skipIgnored = 1;
        setFollowedItem(0);
        closeItemCache();
        freeNpcs();
        initItemCache();
        allocateNpcs();
        initTracker();
        freeFlags();
        loadFlags(nullptr, 0);
        unknown1453 = 1;
        controlledNpc.reset();
        if (skill > 0) {
            controlledNpc.difficulty = skill;
        }
        Item dataLink;
        if (dataLink.create(SHAPE_DATA_LINK, 0)) {
            dataLink.popToCoords(controlledNpc.x(), controlledNpc.y(), controlledNpc.z());
            pickUp(dataLink.id, 0);
        }
        Item extra;
        if (extra.create(0x598, 0)) {
            extra.popToContainer(controlledNpc.id);
        }
        cycleProcess->paused = 1;
        /* The moves to the first map, each after the wait of the map's
         * loading before it. */
        if (warpMission == -1) {
            moveCamera(0, 0, 0, 0);
            unknown1453 = 0;
            setFollowedItem(1);
            if (JUMP_MOVES_TO_EGG) {
                showLoading();
                moveToEgg(1, 30, 1);
                whenDone([]() {
                    jumpToBlack();
                    newGameStarted();
                });
                return;
            }
            teleportToEgg(1, 30, 1);
            newGameStarted();
            return;
        }
        const i16 map = missionMaps[warpMission] + mapOffset;
        warpMission = -1;
        controlledNpc.teleport(0, 0, 0, 0xff);
        whenDone([map]() {
            moveCamera(0, 0, 0, 0xff);
            whenDone([map]() { warpMoved(map); });
        });
        return;
    }
    if (result == MAINMENU_LOAD) {
        loadGame();
        showStatus();
        gameCamera->repaint();
        gameCamera->run();
        gamePalette->apply();
        thawWorld(1);
        cycleProcess->paused = 0;
    } else if (result == MAINMENU_SAVE) {
        startMapJump(0); /* never: the start menu refuses MAINMENU_SAVE */
    } else {
        cycleProcess->paused = 1;
        systemShutdown(1);
    }
    showStatus();
}
}

void unknown07d2() {}

MapJumpProcess::MapJumpProcess(i16 targetMap)
{
    this->map = targetMap;
}

void startMapJump(u16 map)
{
    new MapJumpProcess(map);
}

void keepCarried()
{
    NpcData *data;
    i16 i;

    if (carried == nullptr) {
        carried = std::make_unique_for_overwrite<CarriedItems>();
        readItem(1, &carried->npc);
        carried->npcData = npcs[1];
        data = &carried->npcData;
        for (i16 slot = 0; slot < EQUIPMENT_SLOTS; slot++) {
            if (data->equipment[slot] != 0U) {
                readItem(data->equipment[slot], &carried->equipment[slot]);
            }
        }
        {
            ContainerSearch search(1, LoopScript(SCRIPT_SHAPE, SCRIPT_INT, 0x476, SCRIPT_EQUAL, SCRIPT_END), -1);
            for (carried->count = 0; search.isValid(); search.next()) {
                carried->count++;
            }
        }
        i = 0;
        carried->others = std::make_unique_for_overwrite<ItemRecord[]>(carried->count);
        {
            ContainerSearch search(1, LoopScript(SCRIPT_SHAPE, SCRIPT_INT, 0x476, SCRIPT_EQUAL, SCRIPT_END), -1);
            while (search.isValid()) {
                Item item(search.id);
                search.next();
                item.push();
                item.popToCoords(0, 0, 0);
                readItem(item.id, &carried->others[i++]);
            }
        }
    }
}

void restoreCarried()
{
    NpcData *data;
    i16 i;

    if (carried != nullptr) {
        resetItem(1, &carried->npc);
        data = &carried->npcData;
        for (i = 0; i < EQUIPMENT_SLOTS; i++) {
            if (data->equipment[i] != 0U) {
                Item item = createItem(&carried->equipment[i]);
                item.popToContainer(controlledNpc.id);
                item.setFlags(item.flags() | ITEM_EQUIPPED);
                data->equipment[i] = item.id;
            }
        }
        npcs[1] = carried->npcData;
        for (i = 0; i < carried->count; i++) {
            Item item = createItem(&carried->others[i]);
            item.popToContainer(controlledNpc.id);
            item.setFlags(item.flags() | ITEM_EQUIPPED);
        }
        carried->others.reset();
        carried.reset();
    }
}

namespace {
/* Stops what belongs to the map before a jump (1030:0b6f). */
void leaveMap()
{
    if (JUMP_SHOWS_LOADING) {
        showLoading();
    }
    killAll();
    resetTargeting(controller);
    ASS_StopAllSFX();
    if (JUMP_RESETS_SOUND_CACHE) {
        resetSoundCache();
    }
    resetChannels();
    resetSoundSources();
    if (JUMP_COMPACTS_MEMORY) {
        compactBlocks();
        compactMemory(0);
    }
    resetUsecode();
    closeTracker();
    initTracker();
    snapProcess->clearEggs();
    clearSprites();
    clearPickups();
}
}

void MapJumpProcess::run()
{
    cycleProcess->paused = 1;
    leaveMap();
    keepCarried();
    closeItemCache();
    freeNpcs();
    initItemCache();
    allocateNpcs();
    restoreCarried();
    if (JUMP_MOVES_TO_EGG) {
        moveToEgg(map, 30, 1);
        /* After the map's loading's wait. */
        whenDone([this]() {
            if (map <= 200) {
                jumpToBlack();
            }
            showStatus();
        });
    } else {
        teleportToEgg(map, 30, 1);
    }
}
