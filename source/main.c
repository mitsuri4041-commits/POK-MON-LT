
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <math.h>
#include "world_v_shbin.h"

#define PI 3.14159265f

#define DISPLAY_FLAGS ( \
    GX_TRANSFER_FLIP_VERT(0) | \
    GX_TRANSFER_OUT_TILED(0) | \
    GX_TRANSFER_RAW_COPY(0) | \
    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | \
    GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
    GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO) \
)

static C3D_RenderTarget *target;
static DVLB_s *shaderData;
static shaderProgram_s shader;
static C3D_Mtx projection;
static int projectionLocation;

static float playerX = 0.0f;
static float playerZ = -4.0f;
static float playerSpeed = 0.045f;

static float cameraX = 0.0f;
static float cameraZ = 0.0f;

static float animation = 0.0f;
static float moveAmount = 0.0f;

static int facing = 0;
static int sprinting = 0;

static void vertex(
    float x, float y, float z,
    float r, float g, float b)
{
    C3D_ImmSendAttrib(
        x - cameraX,
        y,
        z - cameraZ,
        1.0f
    );

    C3D_ImmSendAttrib(r, g, b, 1.0f);
}

static void triangle(
    float x1,float y1,float z1,
    float x2,float y2,float z2,
    float x3,float y3,float z3,
    float r,float g,float b)
{
    C3D_ImmDrawBegin(GPU_TRIANGLES);

    vertex(x1,y1,z1,r,g,b);
    vertex(x2,y2,z2,r,g,b);
    vertex(x3,y3,z3,r,g,b);

    C3D_ImmDrawEnd();
}

static void quad(
    float x1,float y1,float z1,
    float x2,float y2,float z2,
    float x3,float y3,float z3,
    float x4,float y4,float z4,
    float r,float g,float b)
{
    C3D_ImmDrawBegin(GPU_TRIANGLES);

    vertex(x1,y1,z1,r,g,b);
    vertex(x2,y2,z2,r,g,b);
    vertex(x3,y3,z3,r,g,b);

    vertex(x1,y1,z1,r,g,b);
    vertex(x3,y3,z3,r,g,b);
    vertex(x4,y4,z4,r,g,b);

    C3D_ImmDrawEnd();
}

static void drawGround(void)
{
    quad(
        -18,-1.3f,-3,
         18,-1.3f,-3,
         24,-1.3f,-35,
        -24,-1.3f,-35,
        0.18f,0.62f,0.20f
    );

    quad(
        -2.4f,-1.27f,-3,
         2.4f,-1.27f,-3,
         2.0f,-1.27f,-35,
        -2.0f,-1.27f,-35,
        0.62f,0.47f,0.30f
    );

    /* Grass patches */
    for (int i=0; i<12; i++)
    {
        float z = -5.0f - i*2.3f;

        quad(
            -7,-1.26f,z,
            -5,-1.26f,z,
            -5,-1.26f,z-0.7f,
            -7,-1.26f,z-0.7f,
            0.12f,0.52f,0.16f
        );

        quad(
            5,-1.26f,z,
            7,-1.26f,z,
            7,-1.26f,z-0.7f,
            5,-1.26f,z-0.7f,
            0.12f,0.52f,0.16f
        );
    }
}

static void drawTree(float x,float z,float s)
{
    float y = -1.25f;

    /* Trunk */
    quad(
        x-0.16f*s,y,z,
        x+0.16f*s,y,z,
        x+0.16f*s,y+0.9f*s,z,
        x-0.16f*s,y+0.9f*s,z,
        0.38f,0.20f,0.08f
    );

    /* Leaves */
    triangle(
        x-0.9f*s,y+0.4f*s,z,
        x+0.9f*s,y+0.4f*s,z,
        x,y+2.2f*s,z,
        0.04f,0.34f,0.10f
    );

    triangle(
        x-0.7f*s,y+1.1f*s,z-0.02f,
        x+0.7f*s,y+1.1f*s,z-0.02f,
        x,y+2.7f*s,z-0.02f,
        0.10f,0.52f,0.14f
    );
}

static void drawHouse(float x,float z,float s)
{
    float y = -1.25f;

    /* Wall */
    quad(
        x-s,y,z,
        x+s,y,z,
        x+s,y+1.5f*s,z,
        x-s,y+1.5f*s,z,
        0.88f,0.72f,0.48f
    );

    /* Roof */
    triangle(
        x-s*1.2f,y+1.5f*s,z-0.03f,
        x+s*1.2f,y+1.5f*s,z-0.03f,
        x,y+2.5f*s,z-0.03f,
        0.65f,0.12f,0.10f
    );

    /* Door */
    quad(
        x-0.22f*s,y,z-0.04f,
        x+0.22f*s,y,z-0.04f,
        x+0.22f*s,y+0.8f*s,z-0.04f,
        x-0.22f*s,y+0.8f*s,z-0.04f,
        0.30f,0.16f,0.08f
    );

    /* Window */
    quad(
        x+0.45f*s,y+0.65f*s,z-0.05f,
        x+0.75f*s,y+0.65f*s,z-0.05f,
        x+0.75f*s,y+1.0f*s,z-0.05f,
        x+0.45f*s,y+1.0f*s,z-0.05f,
        0.30f,0.72f,0.90f
    );
}

static void drawPlayer(void)
{
    float x = playerX;
    float z = playerZ;
    float y = -1.23f;

    float legSwing = sinf(animation) * 0.08f * moveAmount;

    /* Left leg */
    quad(
        x-0.20f,y,z,
        x-0.02f,y,z,
        x-0.02f,y+0.35f+legSwing,z,
        x-0.20f,y+0.35f+legSwing,z,
        0.10f,0.15f,0.50f
    );

    /* Right leg */
    quad(
        x+0.02f,y,z,
        x+0.20f,y,z,
        x+0.20f,y+0.35f-legSwing,z,
        x+0.02f,y+0.35f-legSwing,z,
        0.10f,0.15f,0.50f
    );

    /* Body */
    quad(
        x-0.25f,y+0.35f,z,
        x+0.25f,y+0.35f,z,
        x+0.25f,y+0.85f,z,
        x-0.25f,y+0.85f,z,
        0.85f,0.18f,0.12f
    );

    /* Head */
    quad(
        x-0.16f,y+0.85f,z,
        x+0.16f,y+0.85f,z,
        x+0.16f,y+1.15f,z,
        x-0.16f,y+1.15f,z,
        0.95f,0.68f,0.45f
    );

    /* Hat */
    quad(
        x-0.22f,y+1.12f,z-0.02f,
        x+0.22f,y+1.12f,z-0.02f,
        x+0.22f,y+1.19f,z-0.02f,
        x-0.22f,y+1.19f,z-0.02f,
        0.12f,0.18f,0.65f
    );
}

static void initializeScene(void)
{
    shaderData = DVLB_ParseFile(
        (u32*)world_v_shbin,
        world_v_shbin_size
    );

    if (!shaderData)
        return;

    shaderProgramInit(&shader);

    shaderProgramSetVsh(
        &shader,
        &shaderData->DVLE[0]
    );

    C3D_BindProgram(&shader);

    projectionLocation =
        shaderInstanceGetUniformLocation(
            shader.vertexShader,
            "projection"
        );

    Mtx_PerspTilt(
        &projection,
        60.0f * PI / 180.0f,
        400.0f / 240.0f,
        0.1f,
        100.0f,
        false
    );

    C3D_AttrInfo *attributes = C3D_GetAttrInfo();

    AttrInfo_Init(attributes);
    AttrInfo_AddLoader(attributes,0,GPU_FLOAT,3);
    AttrInfo_AddLoader(attributes,1,GPU_FLOAT,3);

    C3D_TexEnv *environment = C3D_GetTexEnv(0);

    C3D_TexEnvInit(environment);

    C3D_TexEnvSrc(
        environment,
        C3D_Both,
        GPU_PRIMARY_COLOR,
        0,
        0
    );

    C3D_TexEnvFunc(
        environment,
        C3D_Both,
        GPU_REPLACE
    );

    C3D_DepthTest(
        true,
        GPU_GEQUAL,
        GPU_WRITE_ALL
    );
}

static void updatePlayer(void)
{
    circlePosition circle;
    hidCircleRead(&circle);

    u32 keys = hidKeysHeld();

    float dx = circle.dx / 128.0f;
    float dz = -circle.dy / 128.0f;

    if (keys & KEY_LEFT)  dx -= 1.0f;
    if (keys & KEY_RIGHT) dx += 1.0f;
    if (keys & KEY_UP)    dz -= 1.0f;
    if (keys & KEY_DOWN)  dz += 1.0f;

    float length = sqrtf(dx*dx + dz*dz);

    moveAmount = length > 0.1f ? 1.0f : 0.0f;

    sprinting = (keys & KEY_B) ? 1 : 0;

    playerSpeed = sprinting ? 0.075f : 0.045f;

    if (length > 1.0f)
        length = 1.0f;

    if (length > 0.0f)
    {
        dx /= length;
        dz /= length;

        playerX += dx * playerSpeed;
        playerZ += dz * playerSpeed;

        if (fabsf(dx) > fabsf(dz))
            facing = dx > 0 ? 1 : 3;
        else
            facing = dz < 0 ? 0 : 2;

        animation += sprinting ? 0.32f : 0.20f;
    }

    if (playerX < -5.0f) playerX = -5.0f;
    if (playerX >  5.0f) playerX =  5.0f;

    if (playerZ < -25.0f) playerZ = -25.0f;
    if (playerZ > -3.0f) playerZ = -3.0f;

    /* Camera follows player */
    cameraX = playerX;
    cameraZ = playerZ + 4.0f;
}

static void renderWorld(void)
{
    C3D_FVUnifMtx4x4(
        GPU_VERTEX_SHADER,
        projectionLocation,
        &projection
    );

    drawGround();

    drawTree(-4,-6,1.0f);
    drawTree(4,-6,1.0f);

    drawTree(-6,-10,1.3f);
    drawTree(6,-10,1.3f);

    drawTree(-8,-16,1.6f);
    drawTree(8,-16,1.6f);

    drawTree(-5,-23,1.8f);
    drawTree(5,-23,1.8f);

    drawHouse(-4,-12,1.0f);
    drawHouse(4,-18,1.0f);

    drawPlayer();
}

int main(void)
{
    gfxInitDefault();
    consoleInit(GFX_BOTTOM,NULL);

    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE))
    {
        gfxExit();
        return 1;
    }

    target = C3D_RenderTargetCreate(
        240,
        400,
        GPU_RB_RGBA8,
        GPU_RB_DEPTH24_STENCIL8
    );

    if (!target)
    {
        C3D_Fini();
        gfxExit();
        return 1;
    }

    C3D_RenderTargetSetOutput(
        target,
        GFX_TOP,
        GFX_LEFT,
        DISPLAY_FLAGS
    );

    initializeScene();

    while (aptMainLoop())
    {
        hidScanInput();

        u32 down = hidKeysDown();

        if (down & KEY_START)
            break;

        updatePlayer();

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        C3D_RenderTargetClear(
            target,
            C3D_CLEAR_ALL,
            0x87CEEBFF,
            0
        );

        C3D_FrameDrawOn(target);

        renderWorld();

        C3D_FrameEnd(0);

        consoleClear();

        printf("POKEMON WORLD\n");
        printf("====================\n");
        printf("3D WORLD ENGINE\n\n");

        printf("PLAYER X: %.2f\n",playerX);
        printf("PLAYER Z: %.2f\n",playerZ);

        printf("\nCIRCLE PAD: MOVE\n");
        printf("D-PAD: MOVE\n");
        printf("B: RUN\n");
        printf("START: EXIT\n");

        printf("\nCAMERA: FOLLOW PLAYER\n");
        printf("ANIMATION: %s\n",
            moveAmount > 0 ? "MOVING" : "IDLE");
    }

    shaderProgramFree(&shader);
    DVLB_Free(shaderData);

    C3D_RenderTargetDelete(target);
    C3D_Fini();
    gfxExit();

    return 0;
}
