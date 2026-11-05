
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <math.h>
#include "world_v_shbin.h"

#define PI 3.14159265f
#define RAIN_COUNT 20

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

static float waterTime = 0.0f;
static float weatherTime = 0.0f;

static int dayMode = 1;
static int weatherMode = 0;
static int frameCount = 0;

static float rainX[RAIN_COUNT];
static float rainY[RAIN_COUNT];
static float rainZ[RAIN_COUNT];

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

    quad(a,bottom,front,c,bottom,front,
         c,top,front,a,top,front,r,g,b);

    quad(c,bottom,back,a,bottom,back,
         a,top,back,c,top,back,
         r*0.75f,g*0.75f,b*0.75f);

    quad(a,bottom,back,a,bottom,front,
         a,top,front,a,top,back,
         r*0.8f,g*0.8f,b*0.8f);

    quad(c,bottom,front,c,bottom,back,
         c,top,back,c,top,front,
         r*0.9f,g*0.9f,b*0.9f);

    quad(a,top,front,c,top,front,
         c,top,back,a,top,back,
         r,g,b);
}

static void drawGround(void)
{
    quad(
        -22,-1.3f,-3,
         22,-1.3f,-3,
         25,-1.3f,-35,
        -25,-1.3f,-35,
        0.16f,0.52f,0.17f
    );

    quad(
        -2.5f,-1.27f,-3,
         2.5f,-1.27f,-3,
         2.0f,-1.27f,-35,
        -2.0f,-1.27f,-35,
        0.58f,0.43f,0.29f
    );
}

static void drawTree(float x,float z,float s)
{
    float y=-1.25f;

    box(x,y,z,0.35f*s,1.0f*s,0.35f*s,
        0.38f,0.20f,0.08f);

    triangle(
        x-0.9f*s,y+0.5f*s,z,
        x+0.9f*s,y+0.5f*s,z,
        x,y+2.4f*s,z,
        0.04f,0.34f,0.10f
    );

    triangle(
        x-0.7f*s,y+1.2f*s,z-0.02f,
        x+0.7f*s,y+1.2f*s,z-0.02f,
        x,y+2.9f*s,z-0.02f,
        0.10f,0.52f,0.14f
    );
}

static void drawHouse(float x,float z,float s)
{
    float y=-1.25f;

    box(x,y,z,s*1.8f,s*1.5f,s*0.8f,
        0.85f,0.70f,0.50f);

    triangle(
        x-s,y+1.5f*s,z+0.42f*s,
        x+s,y+1.5f*s,z+0.42f*s,
        x,y+2.5f*s,z+0.42f*s,
        0.62f,0.12f,0.10f
    );

    box(
        x,y,z+0.43f*s,
        0.38f*s,0.85f*s,0.08f*s,
        0.28f,0.15f,0.08f
    );

    box(
        x+0.55f*s,y+0.65f*s,z+0.44f*s,
        0.35f*s,0.38f*s,0.06f*s,
        dayMode ? 0.25f : 1.0f,
        dayMode ? 0.70f : 0.65f,
        dayMode ? 0.90f : 0.20f
    );
}

static void drawWater(void)
{
    quad(
        6,-1.28f,-5,
        12,-1.28f,-5,
        12,-1.28f,-25,
        6,-1.28f,-25,
        0.05f,0.35f,0.78f
    );

    for(int i=0;i<7;i++)
    {
        float z=-7.0f-i*2.3f;
        float x=8.0f+sinf(waterTime+i)*0.6f;

        quad(
            x,-1.25f,z,
            x+1.0f,-1.25f,z,
            x+1.0f,-1.25f,z-0.08f,
            x,-1.25f,z-0.08f,
            0.55f,0.85f,1.0f
        );
    }
}

static void drawRain(void)
{
    if(weatherMode!=1)
        return;

    for(int i=0;i<RAIN_COUNT;i++)
    {
        float x=rainX[i];
        float y=rainY[i];
        float z=rainZ[i];

        quad(
            x,y,z,
            x+0.025f,y,z,
            x+0.08f,y-0.45f,z,
            x+0.055f,y-0.45f,z,
            0.65f,0.80f,1.0f
        );
    }
}

static void drawPlayer(void)
{
    float x=playerX;
    float z=playerZ;
    float y=-1.23f;

    float swing=sinf(animation)*0.14f*moveAmount;

    box(x-0.12f,y,z,0.22f,0.12f,0.32f,
        0.12f,0.10f,0.08f);

    box(x+0.12f,y,z,0.22f,0.12f,0.32f,
        0.12f,0.10f,0.08f);

    box(x-0.12f,y+0.12f+swing,z,
        0.18f,0.40f,0.22f,
        0.12f,0.18f,0.52f);

    box(x+0.12f,y+0.12f-swing,z,
        0.18f,0.40f,0.22f,
        0.12f,0.18f,0.52f);

    box(x,y+0.52f,z,0.52f,0.55f,0.30f,
        0.85f,0.18f,0.12f);

    box(x,y+0.55f,z-0.20f,
        0.38f,0.42f,0.18f,
        0.10f,0.20f,0.65f);

    box(x-0.32f,y+0.48f+swing,z,
        0.16f,0.48f,0.20f,
        0.95f,0.68f,0.45f);

    box(x+0.32f,y+0.48f-swing,z,
        0.16f,0.48f,0.20f,
        0.95f,0.68f,0.45f);

    box(x,y+1.02f,z,
        0.18f,0.12f,0.18f,
        0.95f,0.68f,0.45f);

    box(x,y+1.18f,z,
        0.38f,0.38f,0.32f,
        0.95f,0.68f,0.45f);

    box(x,y+1.37f,z-0.02f,
        0.40f,0.16f,0.34f,
        0.18f,0.10f,0.06f);

    box(x,y+1.48f,z,
        0.48f,0.10f,0.42f,
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
    C3D_TexEnvSrc(
        environment,C3D_Both,
        GPU_PRIMARY_COLOR,0,0
    );
    C3D_TexEnvFunc(
        environment,C3D_Both,GPU_REPLACE
    );

    C3D_DepthTest(true,GPU_GEQUAL,GPU_WRITE_ALL);

    for(int i=0;i<RAIN_COUNT;i++)
    {
        rainX[i]=(float)((i*37)%14)-7.0f;
        rainY[i]=(float)((i*19)%7);
        rainZ[i]=-5.0f-(float)((i*13)%20);
    }
}

static void updatePlayer(void)
{
    circlePosition circle;
    hidCircleRead(&circle);

    u32 keys=hidKeysHeld();

    float dx=circle.dx/128.0f;
    float dz=-circle.dy/128.0f;

    if(keys&KEY_LEFT) dx-=1.0f;
    if(keys&KEY_RIGHT) dx+=1.0f;
    if(keys&KEY_UP) dz-=1.0f;
    if(keys&KEY_DOWN) dz+=1.0f;

    float length=sqrtf(dx*dx+dz*dz);

    moveAmount=length>0.1f?1.0f:0.0f;

    playerSpeed=(keys&KEY_B)?0.075f:0.045f;

    if(length>1.0f)
        length=1.0f;

    if(length>0.0f)
    {
        dx/=length;
        dz/=length;

        playerX+=dx*playerSpeed;
        playerZ+=dz*playerSpeed;

        animation+=(keys&KEY_B)?0.32f:0.20f;
    }

    if(playerX < -5.0f) playerX=-5.0f;
    if(playerX >  5.0f) playerX= 5.0f;

    if(playerZ < -25.0f) playerZ=-25.0f;
    if(playerZ > -3.0f) playerZ=-3.0f;

    cameraX=playerX;
    cameraZ=playerZ+4.0f;
}

static void updateEnvironment(void)
{
    waterTime+=0.08f;
    weatherTime+=0.05f;
    frameCount++;

    if(weatherMode==1)
    {
        for(int i=0;i<RAIN_COUNT;i++)
        {
            rainY[i]-=0.18f;

            if(rainY[i]<-1.0f)
            {
                rainY[i]=5.0f;
                rainX[i]=(float)((frameCount+i*17)%14)-7.0f;
            }
        }
    }
}

static u32 getSkyColor(void)
{
    if(!dayMode)
        return weatherMode==1 ? 0x25334FFF : 0x101B3FFF;

    if(weatherMode==1)
        return 0x586C83FF;

    return 0x87CEEBFF;
}

static void renderWorld(void)
{
    C3D_FVUnifMtx4x4(
        GPU_VERTEX_SHADER,
        projectionLocation,
        &projection
    );

    drawGround();

    drawWater();

    drawTree(-4,-6,1.0f);
    drawTree(4,-6,1.0f);
    drawTree(-6,-10,1.3f);
    drawTree(6,-10,1.3f);
    drawTree(-8,-16,1.6f);
    drawTree(8,-16,1.6f);

    drawHouse(-4,-12,1.0f);
    drawHouse(4,-18,1.0f);

    drawRain();

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

        if(down&KEY_X)
            weatherMode=(weatherMode+1)%2;

        if(down&KEY_Y)
            dayMode=!dayMode;

        updatePlayer();
        updateEnvironment();

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        C3D_RenderTargetClear(
            target,C3D_CLEAR_ALL,
            getSkyColor(),0
        );

        C3D_FrameDrawOn(target);

        renderWorld();

        C3D_FrameEnd(0);

        consoleClear();

        printf("POKEMON WORLD\n");
        printf("====================\n");
        printf("ENVIRONMENT SYSTEM\n\n");

        printf("CIRCLE PAD: MOVE\n");
        printf("D-PAD: MOVE\n");
        printf("B: RUN\n");
        printf("X: WEATHER\n");
        printf("Y: DAY / NIGHT\n");
        printf("START: EXIT\n\n");

        printf("TIME: %s\n",dayMode?"DAY":"NIGHT");
        printf("WEATHER: %s\n",
               weatherMode==1?"RAIN":"CLEAR");

        printf("PLAYER X: %.2f\n",playerX);
        printf("PLAYER Z: %.2f\n",playerZ);
    }

    shaderProgramFree(&shader);
    DVLB_Free(shaderData);

    C3D_RenderTargetDelete(target);
    C3D_Fini();
    gfxExit();

    return 0;
}
