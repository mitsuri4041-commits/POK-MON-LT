
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
static int sprinting = 0;

static void vertex(float x,float y,float z,
                   float r,float g,float b)
{
    C3D_ImmSendAttrib(
        x-cameraX,y,z-cameraZ,1.0f
    );

    C3D_ImmSendAttrib(r,g,b,1.0f);
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

static void box(
    float x,float y,float z,
    float w,float h,float d,
    float r,float g,float b)
{
    float a=x-w/2;
    float c=x+w/2;
    float bottom=y;
    float top=y+h;
    float front=z+d/2;
    float back=z-d/2;

    /* Front */
    quad(a,bottom,front,c,bottom,front,
         c,top,front,a,top,front,r,g,b);

    /* Back */
    quad(c,bottom,back,a,bottom,back,
         a,top,back,c,top,back,r*0.75f,g*0.75f,b*0.75f);

    /* Left */
    quad(a,bottom,back,a,bottom,front,
         a,top,front,a,top,back,
         r*0.8f,g*0.8f,b*0.8f);

    /* Right */
    quad(c,bottom,front,c,bottom,back,
         c,top,back,c,top,front,
         r*0.9f,g*0.9f,b*0.9f);

    /* Top */
    quad(a,top,front,c,top,front,
         c,top,back,a,top,back,
         r*1.1f,g*1.1f,b*1.1f);

    /* Bottom */
    quad(a,bottom,back,c,bottom,back,
         c,bottom,front,a,bottom,front,
         r*0.6f,g*0.6f,b*0.6f);
}

static void drawGround(void)
{
    quad(-20,-1.3f,-3,20,-1.3f,-3,
         24,-1.3f,-35,-24,-1.3f,-35,
         0.18f,0.62f,0.20f);

    quad(-2.5f,-1.27f,-3,2.5f,-1.27f,-3,
         2,-1.27f,-35,-2,-1.27f,-35,
         0.62f,0.47f,0.30f);
}

static void drawTree(float x,float z,float s)
{
    float y=-1.25f;

    box(x,y,z,0.35f*s,1.0f*s,0.35f*s,
        0.38f,0.20f,0.08f);

    triangle(x-0.9f*s,y+0.5f*s,z,
             x+0.9f*s,y+0.5f*s,z,
             x,y+2.4f*s,z,
             0.04f,0.36f,0.10f);

    triangle(x-0.7f*s,y+1.2f*s,z-0.02f,
             x+0.7f*s,y+1.2f*s,z-0.02f,
             x,y+2.9f*s,z-0.02f,
             0.10f,0.55f,0.14f);
}

static void drawHouse(float x,float z,float s)
{
    float y=-1.25f;

    box(x,y,z,s*1.8f,s*1.5f,0.8f*s,
        0.88f,0.70f,0.48f);

    triangle(x-s,y+1.5f*s,z+0.42f*s,
             x+s,y+1.5f*s,z+0.42f*s,
             x,y+2.5f*s,z+0.42f*s,
             0.65f,0.12f,0.10f);

    box(x,y,z+0.43f*s,0.38f*s,0.85f*s,
        0.08f*s,0.28f,0.15f,0.08f);

    box(x+0.55f*s,y+0.65f*s,z+0.44f*s,
        0.35f*s,0.38f*s,0.06f*s,
        0.25f,0.70f,0.90f);
}

static void drawPlayer(void)
{
    float x=playerX;
    float z=playerZ;
    float y=-1.23f;

    float swing=sinf(animation)*0.16f*moveAmount;

    /* Shoes */
    box(x-0.12f,y,z,0.22f,0.12f,0.32f,
        0.12f,0.10f,0.08f);

    box(x+0.12f,y,z,0.22f,0.12f,0.32f,
        0.12f,0.10f,0.08f);

    /* Legs */
    box(x-0.12f,y+0.12f+swing,z,
        0.18f,0.40f,0.22f,
        0.12f,0.18f,0.52f);

    box(x+0.12f,y+0.12f-swing,z,
        0.18f,0.40f,0.22f,
        0.12f,0.18f,0.52f);

    /* Body */
    box(x,y+0.52f,z,
        0.52f,0.55f,0.30f,
        0.85f,0.18f,0.12f);

    /* Backpack */
    box(x,y+0.55f,z-0.20f,
        0.38f,0.42f,0.18f,
        0.10f,0.20f,0.65f);

    /* Arms */
    box(x-0.32f,y+0.48f+swing,z,
        0.16f,0.48f,0.20f,
        0.95f,0.68f,0.45f);

    box(x+0.32f,y+0.48f-swing,z,
        0.16f,0.48f,0.20f,
        0.95f,0.68f,0.45f);

    /* Neck */
    box(x,y+1.02f,z,0.18f,0.12f,0.18f,
        0.95f,0.68f,0.45f);

    /* Head */
    box(x,y+1.18f,z,0.38f,0.38f,0.32f,
        0.95f,0.68f,0.45f);

    /* Hair */
    box(x,y+1.37f,z-0.02f,
        0.40f,0.16f,0.34f,
        0.18f,0.10f,0.06f);

    /* Cap */
    box(x,y+1.48f,z,
        0.48f,0.10f,0.42f,
        0.10f,0.18f,0.68f);

    box(x,y+1.43f,z+0.22f,
        0.45f,0.04f,0.20f,
        0.10f,0.18f,0.68f);
}

static void initializeScene(void)
{
    shaderData=DVLB_ParseFile(
        (u32*)world_v_shbin,
        world_v_shbin_size
    );

    if(!shaderData)
        return;

    shaderProgramInit(&shader);

    shaderProgramSetVsh(
        &shader,&shaderData->DVLE[0]
    );

    C3D_BindProgram(&shader);

    projectionLocation=
        shaderInstanceGetUniformLocation(
            shader.vertexShader,"projection"
        );

    Mtx_PerspTilt(
        &projection,
        60.0f*PI/180.0f,
        400.0f/240.0f,
        0.1f,100.0f,false
    );

    C3D_AttrInfo *attributes=C3D_GetAttrInfo();

    AttrInfo_Init(attributes);
    AttrInfo_AddLoader(attributes,0,GPU_FLOAT,3);
    AttrInfo_AddLoader(attributes,1,GPU_FLOAT,3);

    C3D_TexEnv *environment=C3D_GetTexEnv(0);

    C3D_TexEnvInit(environment);
    C3D_TexEnvSrc(environment,C3D_Both,
                  GPU_PRIMARY_COLOR,0,0);
    C3D_TexEnvFunc(environment,C3D_Both,GPU_REPLACE);

    C3D_DepthTest(true,GPU_GEQUAL,GPU_WRITE_ALL);
}

static void updatePlayer(void)
{
    circlePosition circle;
    hidCircleRead(&circle);

    u32 keys=hidKeysHeld();

    float dx=circle.dx/128.0f;
    float dz=-circle.dy/128.0f;

    if(keys&KEY_LEFT)  dx-=1.0f;
    if(keys&KEY_RIGHT) dx+=1.0f;
    if(keys&KEY_UP)    dz-=1.0f;
    if(keys&KEY_DOWN)  dz+=1.0f;

    float length=sqrtf(dx*dx+dz*dz);

    moveAmount=length>0.1f?1.0f:0.0f;
    sprinting=(keys&KEY_B)?1:0;

    playerSpeed=sprinting?0.075f:0.045f;

    if(length>1.0f)
        length=1.0f;

    if(length>0.0f)
    {
        dx/=length;
        dz/=length;

        playerX+=dx*playerSpeed;
        playerZ+=dz*playerSpeed;

        animation+=sprinting?0.32f:0.20f;
    }

    if(playerX < -5.0f) playerX=-5.0f;
    if(playerX >  5.0f) playerX= 5.0f;

    if(playerZ < -25.0f) playerZ=-25.0f;
    if(playerZ > -3.0f) playerZ=-3.0f;

    cameraX=playerX;
    cameraZ=playerZ+4.0f;
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

    drawHouse(-4,-12,1.0f);
    drawHouse(4,-18,1.0f);

    drawPlayer();
}

int main(void)
{
    gfxInitDefault();
    consoleInit(GFX_BOTTOM,NULL);

    if(!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE))
    {
        gfxExit();
        return 1;
    }

    target=C3D_RenderTargetCreate(
        240,400,GPU_RB_RGBA8,
        GPU_RB_DEPTH24_STENCIL8
    );

    if(!target)
    {
        C3D_Fini();
        gfxExit();
        return 1;
    }

    C3D_RenderTargetSetOutput(
        target,GFX_TOP,GFX_LEFT,DISPLAY_FLAGS
    );

    initializeScene();

    while(aptMainLoop())
    {
        hidScanInput();

        u32 down=hidKeysDown();

        if(down&KEY_START)
            break;

        updatePlayer();

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        C3D_RenderTargetClear(
            target,C3D_CLEAR_ALL,
            0x87CEEBFF,0
        );

        C3D_FrameDrawOn(target);
        renderWorld();

        C3D_FrameEnd(0);

        consoleClear();

        printf("POKEMON WORLD\n");
        printf("====================\n");
        printf("3D CHARACTER SYSTEM\n\n");
        printf("CIRCLE PAD: MOVE\n");
        printf("D-PAD: MOVE\n");
        printf("B: RUN\n");
        printf("START: EXIT\n\n");
        printf("PLAYER X: %.2f\n",playerX);
        printf("PLAYER Z: %.2f\n",playerZ);
        printf("ANIMATION: %s\n",
               moveAmount>0?"MOVING":"IDLE");
    }

    shaderProgramFree(&shader);
    DVLB_Free(shaderData);

    C3D_RenderTargetDelete(target);
    C3D_Fini();
    gfxExit();

    return 0;
}
