#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

/* =========================================
   POKEMON WORLD
   Full Game Engine Foundation
   ========================================= */

#define GAME_TITLE "POKEMON WORLD"
#define MAX_PARTY 6
#define MAX_POKEMON 1025
#define MAX_NPCS 128
#define MAX_QUESTS 256
#define MAX_REGIONS 7

typedef enum
{
    REGION_KANTO,
    REGION_JOHTO,
    REGION_HOENN,
    REGION_SINNOH,
    REGION_UNOVA,
    REGION_KALOS,
    REGION_ALOLA
} RegionID;

typedef enum
{
    WEATHER_CLEAR,
    WEATHER_RAIN,
    WEATHER_SNOW,
    WEATHER_THUNDER
} WeatherType;

typedef enum
{
    SCENE_WORLD,
    SCENE_BATTLE,
    SCENE_MENU,
    SCENE_DIALOGUE
} SceneType;

typedef struct
{
    float x;
    float y;
    float z;
} Vector3;

typedef struct
{
    unsigned short species;
    unsigned short level;
    unsigned int experience;
    unsigned int personality;
    bool shiny;
    bool active;
} Pokemon;

typedef struct
{
    Vector3 position;
    float rotation;
    float speed;
} Player;

typedef struct
{
    unsigned int id;
    Vector3 position;
    char name[32];
    char dialogue[256];
    bool active;
} NPC;

typedef struct
{
    unsigned int id;
    char title[64];
    char description[256];
    bool completed;
} Quest;

typedef struct
{
    RegionID region;
    WeatherType weather;
    SceneType scene;

    unsigned int money;
    unsigned int playTime;

    unsigned char hour;
    unsigned char minute;

    Player player;

    Pokemon party[MAX_PARTY];
    NPC npcs[MAX_NPCS];
    Quest quests[MAX_QUESTS];

    unsigned int questCount;
    unsigned int npcCount;

    bool menuOpen;
    bool gameRunning;
} GameState;

/* =========================================
   GLOBAL GAME DATA
   ========================================= */

static GameState game;

static const char *regionNames[MAX_REGIONS] =
{
    "KANTO",
    "JOHTO",
    "HOENN",
    "SINNOH",
    "UNOVA",
    "KALOS",
    "ALOLA"
};

/* =========================================
   GAME INITIALIZATION
   ========================================= */

static void initializeGame(void)
{
    memset(&game, 0, sizeof(game));

    game.region = REGION_KANTO;
    game.weather = WEATHER_CLEAR;
    game.scene = SCENE_WORLD;

    game.hour = 12;
    game.minute = 0;

    game.money = 3000;

    game.player.position.x = 0.0f;
    game.player.position.y = 0.0f;
    game.player.position.z = 0.0f;

    game.player.speed = 0.08f;

    game.gameRunning = true;

    /* EXP Share enabled from beginning */
}

/* =========================================
   PLAYER MOVEMENT
   ========================================= */

static void updatePlayer(void)
{
    circlePosition circle;
    hidCircleRead(&circle);

    u32 held = hidKeysHeld();

    float dx = 0.0f;
    float dz = 0.0f;

    if (held & KEY_UP)
        dz -= 1.0f;

    if (held & KEY_DOWN)
        dz += 1.0f;

    if (held & KEY_LEFT)
        dx -= 1.0f;

    if (held & KEY_RIGHT)
        dx += 1.0f;

    dx += circle.dx / 128.0f;
    dz -= circle.dy / 128.0f;

    float length = sqrtf(dx * dx + dz * dz);

    if (length > 0.0f)
    {
        dx /= length;
        dz /= length;

        game.player.position.x += dx * game.player.speed;
        game.player.position.z += dz * game.player.speed;

        game.player.rotation = atan2f(dx, -dz);
    }
}

/* =========================================
   POKEMON PARTY
   ========================================= */

static void initializeParty(void)
{
    for (int i = 0; i < MAX_PARTY; i++)
    {
        game.party[i].species = 0;
        game.party[i].level = 1;
        game.party[i].experience = 0;
        game.party[i].shiny = false;
        game.party[i].active = false;
    }
}

/* =========================================
   NPC SYSTEM
   ========================================= */

static void initializeNPCSystem(void)
{
    game.npcCount = 0;

    memset(game.npcs, 0, sizeof(game.npcs));
}

/* =========================================
   QUEST SYSTEM
   ========================================= */

static void initializeQuestSystem(void)
{
    game.questCount = 0;

    memset(game.quests, 0, sizeof(game.quests));
}

/* =========================================
   TIME AND WEATHER
   ========================================= */

static void updateWorldTime(void)
{
    game.minute++;

    if (game.minute >= 60)
    {
        game.minute = 0;
        game.hour++;
    }

    if (game.hour >= 24)
        game.hour = 0;
}

/* =========================================
   REGION SYSTEM
   ========================================= */

static void changeRegion(RegionID region)
{
    if (region >= REGION_KANTO &&
        region <= REGION_ALOLA)
    {
        game.region = region;

        game.player.position.x = 0.0f;
        game.player.position.y = 0.0f;
        game.player.position.z = 0.0f;
    }
}

/* =========================================
   GAME INPUT
   ========================================= */

static void updateInput(void)
{
    hidScanInput();

    u32 down = hidKeysDown();

    if (down & KEY_START)
    {
        game.menuOpen = !game.menuOpen;

        game.scene = game.menuOpen
            ? SCENE_MENU
            : SCENE_WORLD;
    }

    if (down & KEY_B)
    {
        if (game.menuOpen)
        {
            game.menuOpen = false;
            game.scene = SCENE_WORLD;
        }
    }

    if (down & KEY_SELECT)
    {
        changeRegion(
            (RegionID)((game.region + 1) % MAX_REGIONS)
        );
    }

    if (!game.menuOpen)
        updatePlayer();
}

/* =========================================
   GAME UPDATE
   ========================================= */

static void updateGame(void)
{
    updateInput();

    updateWorldTime();
}

/* =========================================
   GRAPHICS INITIALIZATION
   ========================================= */

static void initializeGraphics(void)
{
    gfxInitDefault();

    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

    consoleInit(GFX_BOTTOM, NULL);
}

/* =========================================
   WORLD RENDERER
   ========================================= */

static void renderWorld(void)
{
    printf("POKEMON WORLD\n");
    printf("REGION: %s\n", regionNames[game.region]);

    printf("PLAYER POSITION\n");

    printf("X: %.2f\n",
        game.player.position.x);

    printf("Z: %.2f\n",
        game.player.position.z);

    printf("\nTIME: %02u:%02u\n",
        game.hour,
        game.minute);

    printf("\nWEATHER: %u\n",
        game.weather);

    printf("\nPARTY\n");

    for (int i = 0; i < MAX_PARTY; i++)
    {
        if (game.party[i].active)
        {
            printf("POKEMON %u LV.%u\n",
                game.party[i].species,
                game.party[i].level);
        }
    }

    printf("\n[CIRCLE PAD] MOVE\n");
    printf("[START] MENU\n");
    printf("[SELECT] REGION\n");
}

/* =========================================
   MAIN MENU
   ========================================= */

static void renderMenu(void)
{
    printf("\n===== MAIN MENU =====\n");

    printf("POKEMON\n");
    printf("BAG\n");
    printf("POKEDEX\n");
    printf("QUESTS\n");
    printf("MAP\n");
    printf("SAVE GAME\n");

    printf("\n[B] BACK\n");
}

/* =========================================
   MAIN RENDER
   ========================================= */

static void renderGame(void)
{
    consoleClear();

    if (game.scene == SCENE_MENU)
        renderMenu();
    else
        renderWorld();
}

/* =========================================
   GAME SHUTDOWN
   ========================================= */

static void shutdownGame(void)
{
    C3D_Fini();

    gfxExit();
}

/* =========================================
   MAIN
   ========================================= */

int main(void)
{
    initializeGame();

    initializeParty();
    initializeNPCSystem();
    initializeQuestSystem();

    initializeGraphics();

    while (aptMainLoop() && game.gameRunning)
    {
        updateGame();

        renderGame();

        gfxFlushBuffers();
        gfxSwapBuffers();

        gspWaitForVBlank();
    }

    shutdownGame();

    return 0;
}
