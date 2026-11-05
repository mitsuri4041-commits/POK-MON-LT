#include <3ds.h>
#include <citro3d.h>
#include <tex3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SCREEN_W 400
#define SCREEN_H 240

typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    Vec3 position;
    float rotation;
} Player;

static C3D_RenderTarget *top;
static Player player = {{0.0f, 0.0f, 0.0f}, 0.0f};

static void initGraphics(void)
{
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

    top = C3D_RenderTargetCreate(
        240, 400,
        GPU_RB_RGBA8,
        GPU_RB_DEPTH24_STENCIL8
    );

    C3D_RenderTargetSetOutput(
        top, GFX_TOP, GFX_LEFT,
        GX_TRANSFER_FLIP_VERT(0) |
        GX_TRANSFER_OUT_TILED(0) |
        GX_TRANSFER_RAW_COPY(0) |
        GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) |
        GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) |
        GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO)
    );
}

static void updatePlayer(void)
{
    hidScanInput();

    u32 keys = hidKeysHeld();

    circlePosition circle;
    hidCircleRead(&circle);

    float dx = circle.dx / 128.0f;
    float dz = -circle.dy / 128.0f;

    if (keys & KEY_UP) dz -= 1.0f;
    if (keys & KEY_DOWN) dz += 1.0f;
    if (keys & KEY_LEFT) dx -= 1.0f;
    if (keys & KEY_RIGHT) dx += 1.0f;

    float length = sqrtf(dx * dx + dz * dz);

    if (length > 0.0f) {
        dx /= length;
        dz /= length;

        player.position.x += dx * 0.08f;
        player.position.z += dz * 0.08f;

        player.rotation = atan2f(dx, -dz);
    }
}

static void renderWorld(void)
{
    C3D_RenderTargetClear(
        top,
        C3D_CLEAR_ALL,
        0x87CEEBFF,
        0
    );

    C3D_FrameDrawOn(top);

    /*
       3D MODEL PIPELINE:
       Terrain
       Player Model
       Camera
       Lighting
       Pokemon Models

       These require Citro3D shader,
       vertex buffers and actual model assets.
    */
}

static void renderUI(void)
{
    consoleClear();

    printf("POKEMON WORLD\n");
    printf("========================\n");
    printf("3D WORLD ENGINE\n\n");

    printf("PLAYER X: %.2f\n", player.position.x);
    printf("PLAYER Z: %.2f\n", player.position.z);

    printf("\nCIRCLE PAD: MOVE\n");
    printf("D-PAD: MOVE\n");
    printf("START: MENU\n");
}

int main(void)
{
    initGraphics();
    consoleInit(GFX_BOTTOM, NULL);

    while (aptMainLoop())
    {
        updatePlayer();

        if (hidKeysDown() & KEY_B)
            break;

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        renderWorld();

        C3D_FrameEnd(0);

        renderUI();
    }

    C3D_RenderTargetDelete(top);
    C3D_Fini();
    gfxExit();

    return 0;
}
