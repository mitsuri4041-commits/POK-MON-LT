
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <math.h>
#include "world_v_shbin.h"

#define PI 3.14159265f
#define PARTY_SIZE 6

#define DISPLAY_FLAGS ( \
    GX_TRANSFER_FLIP_VERT(0) | \
    GX_TRANSFER_OUT_TILED(0) | \
    GX_TRANSFER_RAW_COPY(0) | \
    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | \
    GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
    GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO) \
)

typedef struct {
    float x,z;
    float r,g,b;
    float phase;
    const char *name;
    const char *greeting;
    int level;
    int friendship;
    int mood;
} Pokemon;

static C3D_RenderTarget *target;
static DVLB_s *shaderData;
static shaderProgram_s shader;
static C3D_Mtx projection;
static int projectionLocation;

static float playerX=0.0f;
static float playerZ=-4.0f;
static float cameraX=0.0f;
static float cameraZ=0.0f;
static float animation=0.0f;
static float moveAmount=0.0f;

static int frameCount=0;
static int messageTimer=0;
static int selectedPokemon=-1;

static char message[128]="Your Pokemon are happy!";

static Pokemon party[PARTY_SIZE]={
    {-1,-5,.95f,.25f,.15f,0,"CHARMANDER","Char!",12,30,0},
    {1,-5,.20f,.55f,.95f,1,"SQUIRTLE","Squirtle!",12,35,0},
    {-1.5f,-6,.25f,.80f,.25f,2,"BULBASAUR","Bulba!",12,40,0},
    {1.5f,-6,.95f,.80f,.15f,3,"PIKACHU","Pika Pika!",12,50,0},
    {-2,-7,.75f,.35f,.85f,4,"EEVEE","Eevee!",12,45,0},
    {2,-7,.95f,.45f,.65f,5,"JIGGLYPUFF","Jiggly!",12,40,0}
};

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

static void box(float x,float y,float z,
                float w,float h,float d,
                float r,float g,float b)
{
    float a=x-w/2,c=x+w/2;
    float bottom=y,top=y+h;
    float front=z+d/2,back=z-d/2;

    quad(a,bottom,front,c,bottom,front,
         c,top,front,a,top,front,r,g,b);

    quad(c,bottom,back,a,bottom,back,
         a,top,back,c,top,back,
         r*.75f,g*.75f,b*.75f);

    quad(a,bottom,back,a,bottom,front,
         a,top,front,a,top,back,
         r*.8f,g*.8f,b*.8f);

    quad(c,bottom,front,c,bottom,back,
         c,top,back,c,top,front,
         r*.9f,g*.9f,b*.9f);

    quad(a,top,front,c,top,front,
         c,top,back,a,top,back,r,g,b);
}

static void drawGround(void)
{
    quad(-22,-1.3f,-3,22,-1.3f,-3,
         25,-1.3f,-35,-25,-1.3f,-35,
         .16f,.52f,.17f);

    quad(-2.5f,-1.27f,-3,2.5f,-1.27f,-3,
         2,-1.27f,-35,-2,-1.27f,-35,
         .58f,.43f,.29f);
}

static void drawTree(float x,float z)
{
    box(x,-1.25f,z,.35f,1,.35f,.38f,.20f,.08f);

    triangle(x-.9f,-.75f,z,
             x+.9f,-.75f,z,
             x,1.15f,z,.04f,.34f,.10f);

    triangle(x-.7f,-.05f,z-.02f,
             x+.7f,-.05f,z-.02f,
             x,1.65f,z-.02f,.10f,.52f,.14f);
}

static void drawPlayer(void)
{
    float y=-1.23f;
    float swing=sinf(animation)*.14f*moveAmount;

    box(playerX-.12f,y,playerZ,.22f,.12f,.32f,
        .12f,.10f,.08f);

    box(playerX+.12f,y,playerZ,.22f,.12f,.32f,
        .12f,.10f,.08f);

    box(playerX,y+.52f,playerZ,.52f,.55f,.30f,
        .85f,.18f,.12f);

    box(playerX,y+.55f,playerZ-.20f,
        .38f,.42f,.18f,.10f,.20f,.65f);

    box(playerX-.32f,y+.48f+swing,playerZ,
        .16f,.48f,.20f,.95f,.68f,.45f);

    box(playerX+.32f,y+.48f-swing,playerZ,
        .16f,.48f,.20f,.95f,.68f,.45f);

    box(playerX,y+1.18f,playerZ,
        .38f,.38f,.32f,.95f,.68f,.45f);

    box(playerX,y+1.48f,playerZ,
        .48f,.10f,.42f,.10f,.18f,.68f);
}

static void drawPokemon(Pokemon *p,int index)
{
    float bob=sinf(frameCount*.06f+p->phase)*.07f;
    float x=p->x;
    float y=-1.23f+bob;
    float z=p->z;

    if(p->mood==1)
        y+=fabsf(sinf(frameCount*.12f+p->phase))*.12f;

    box(x,y,z,.62f,.58f,.52f,p->r,p->g,p->b);

    box(x,y+.48f,z-.04f,.48f,.42f,.42f,
        p->r,p->g,p->b);

    box(x-.12f,y+.68f,z+.18f,.07f,.09f,.04f,
        .04f,.04f,.04f);

    box(x+.12f,y+.68f,z+.18f,.07f,.09f,.04f,
        .04f,.04f,.04f);

    box(x-.18f,y-.10f,z+.08f,.18f,.12f,.22f,
        p->r*.8f,p->g*.8f,p->b*.8f);

    box(x+.18f,y-.10f,z+.08f,.18f,.12f,.22f,
        p->r*.8f,p->g*.8f,p->b*.8f);

    if(selectedPokemon==index && messageTimer>0)
    {
        float sparkle=sinf(frameCount*.2f)*.08f;

        box(x,y+.98f+sparkle,z,
            .12f,.12f,.12f,1.0f,1.0f,.25f);
    }
}

static void updateParty(void)
{
    for(int i=0;i<PARTY_SIZE;i++)
    {
        float angle=(float)i*1.05f;
        float distance=1.1f+(float)(i/2)*.55f;

        float targetX=playerX+sinf(angle)*distance;
        float targetZ=playerZ+1.3f+(float)(i/2)*.8f;

        party[i].x+=(targetX-party[i].x)*.035f;
        party[i].z+=(targetZ-party[i].z)*.035f;

        if(moveAmount>.1f)
            party[i].mood=0;
        else if((frameCount+i*30)%300<35)
            party[i].mood=1;
        else
            party[i].mood=0;
    }
}

static void interactPokemon(void)
{
    int nearest=-1;
    float nearestDistance=999;

    for(int i=0;i<PARTY_SIZE;i++)
    {
        float dx=party[i].x-playerX;
        float dz=party[i].z-playerZ;
        float distance=sqrtf(dx*dx+dz*dz);

        if(distance<nearestDistance)
        {
            nearestDistance=distance;
            nearest=i;
        }
    }

    if(nearest<0) return;

    selectedPokemon=nearest;

    Pokemon *p=&party[nearest];

    if(p->friendship<255)
        p->friendship+=1;

    snprintf(message,sizeof(message),
        "%s: %s Friendship: %d",
        p->name,p->greeting,p->friendship);

    messageTimer=240;
}

static void updatePlayer(void)
{
    circlePosition circle;
    hidCircleRead(&circle);

    u32 keys=hidKeysHeld();

    float dx=circle.dx/128.0f;
    float dz=-circle.dy/128.0f;

    if(keys&KEY_LEFT) dx-=1;
    if(keys&KEY_RIGHT) dx+=1;
    if(keys&KEY_UP) dz-=1;
    if(keys&KEY_DOWN) dz+=1;

    float length=sqrtf(dx*dx+dz*dz);

    moveAmount=length>.1f?1:0;

    float speed=(keys&KEY_B)?.075f:.045f;

    if(length>1) length=1;

    if(length>0)
    {
        dx/=length;
        dz/=length;

        playerX+=dx*speed;
        playerZ+=dz*speed;
        animation+=.20f;
    }

    if(playerX < -5) playerX=-5;
    if(playerX > 5) playerX=5;
    if(playerZ < -25) playerZ=-25;
    if(playerZ > -3) playerZ=-3;

    cameraX=playerX;
    cameraZ=playerZ+4;
}

static void initializeScene(void)
{
    shaderData=DVLB_ParseFile(
        (u32*)world_v_shbin,
        world_v_shbin_size
    );

    if(!shaderData) return;

    shaderProgramInit(&shader);
    shaderProgramSetVsh(&shader,&shaderData->DVLE[0]);
    C3D_BindProgram(&shader);

    projectionLocation=shaderInstanceGetUniformLocation(
        shader.vertexShader,"projection"
    );

    Mtx_PerspTilt(
        &projection,60.0f*PI/180.0f,
        400.0f/240.0f,.1f,100.0f,false
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

static void renderWorld(void)
{
    C3D_FVUnifMtx4x4(
        GPU_VERTEX_SHADER,projectionLocation,&projection
    );

    drawGround();

    drawTree(-4,-6);
    drawTree(4,-6);
    drawTree(-6,-12);
    drawTree(6,-15);

    for(int i=0;i<PARTY_SIZE;i++)
        drawPokemon(&party[i],i);

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

        if(down&KEY_A)
            interactPokemon();

        updatePlayer();
        updateParty();

        frameCount++;

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        C3D_RenderTargetClear(
            target,C3D_CLEAR_ALL,0x87CEEBFF,0
        );

        C3D_FrameDrawOn(target);
        renderWorld();
        C3D_FrameEnd(0);

        consoleClear();

        printf("POKEMON WORLD\n");
        printf("====================\n");
        printf("FRIENDSHIP SYSTEM\n\n");

        printf("CIRCLE PAD: MOVE\n");
        printf("D-PAD: MOVE\n");
        printf("B: RUN\n");
        printf("A: TALK / FRIENDSHIP\n");
        printf("START: EXIT\n\n");

        printf("YOUR PARTY:\n");

        for(int i=0;i<PARTY_SIZE;i++)
        {
            printf("%d. %-12s Lv.%d\n",
                i+1,party[i].name,party[i].level);
        }

        printf("\n");

        if(messageTimer>0)
        {
            printf("%s\n",message);
            messageTimer--;
        }
        else
        {
            printf("Your Pokemon are following you!");
        }
    }

    shaderProgramFree(&shader);
    DVLB_Free(shaderData);

    C3D_RenderTargetDelete(target);
    C3D_Fini();
    gfxExit();

    return 0;
}
