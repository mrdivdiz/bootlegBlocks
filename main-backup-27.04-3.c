#include <stdint.h>
#include "main.h"
#include <mrc_base.h>
#include <mrc_jgraphics.h>
#include <mrc_win.h>
#include <mrc_menu.h>
#include <mrc_text.h>
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define CHUNK_SIZE 2
#define PI 3.14159265f
#define TWO_PI 6.283185307f
#define STEP_SIZE 0.1f
#define EPSILON 0.001f
#define FOCAL_LENGTH 200.0f
mrc_jgraphics_func_table_t funk;
BlockType world[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
mrc_jgraphics_context_t *gContext; 
Camera cam;
CBitmap handBmp;
int g_key_up, g_key_down, g_key_left, g_key_right;
int g_touch_active = 0;
char test1[8];
char test2[8];
int g_last_tx = 120;
int g_last_ty = 160;
int g_curr_tx = 120;
int g_curr_ty = 160;
int diff_x = 0;
int diff_y = 0;
float dx = 0;
float dz = 0;
float move_speed = 1.0f;
float rot_speed = 0.0004f;
int globalTimer;

__inline void initSprite(CBitmap* bmp, int w, int h, unsigned char* pixels)
{
	bmp->width = w;
	bmp->height = h;
	bmp->pixels = pixels;
}

void initSpriteData()
{
	initSprite(&handBmp, 64, 64, (unsigned char*)_sprHand);
}

float my_fmodf(float x, float y) {
    if (y == 0) return 0;
    return x - (float)((int)(x / y)) * y; 
}

float my_sinf(float x) {
    float res, term, angle, xopt;
    int i;
    angle = my_fmodf(x, TWO_PI);
    if (angle > PI) angle -= TWO_PI;
    else if (angle < -PI) angle += TWO_PI;
    res = angle;
    term = angle;
    xopt = -angle * angle;
    for (i = 1; i <= 4; i++) {
        term = xopt / ((2.0f * i) * (2.0f * i + 1.0f));
        res += term;
    }
    return res;
}

float my_cosf(float x) {
    float res, term, angle, xopt;
    int i;
    angle = my_fmodf(x, TWO_PI);
    if (angle > PI) angle -= TWO_PI;
    else if (angle < -PI) angle += TWO_PI;
    res = 1.0f;
    term = 1.0f;
    xopt = -angle * angle;
    for (i = 1; i <= 4; i++) {
        term = xopt / ((2.0f * i - 1.0f) * (2.0f * i));
        res += term;
    }
    return res;
}

void gDrawBitmap(CBitmap* bmp, int x, int y)
{
	mrc_bitmapShowEx((uint16*)bmp->pixels, x, y, bmp->width, bmp->width, bmp->height, BM_TRANSPARENT, 0, 0);
}

int32 mrc_extRecvAppEvent(int32 app, int32 code, int32 param0, int32 param1)
{
        return MR_SUCCESS;
}
int32 mrc_extRecvAppEventEx(int32 code, int32 p0, int32 p1, int32 p2, int32 p3, int32 p4,int32 p5)
{
        return MR_SUCCESS;
}


int32 mrc_pause(void)
{
	return MR_SUCCESS;
}

void reverse(char s[]) {
    int i, j;
    char c;
    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

void int_to_string(int n, char s[]) {
    int i = 0, sign;
    if ((sign = n) < 0) n = -n;
    
    do { 
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);
    
    if (sign < 0) s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}

int32 mrc_resume(void)
{
	return MR_SUCCESS;
}


int32 mrc_exitApp(void)
{
	
	return MR_SUCCESS;
}
int32 projectPoint(Vec3 world_pt, const Camera *cam, float cosY, float sinY, float cosP, float sinP, int16 *sx, int16 *sy, int16 *sz) {
    float x = world_pt.x - cam->pos.x;
    float y = world_pt.y - cam->pos.y;
    float z = world_pt.z - cam->pos.z;
    float rx = x * cosY + z * sinY;
    float rz = -x * sinY + z * cosY;
    float final_x = rx;
    float final_y = y * cosP - rz * sinP;
    float final_z = y * sinP + rz * cosP;
    float invZ;
    *sz = (int16)final_z;
    invZ = FOCAL_LENGTH / final_z;
    
    *sx = (int16)(final_x * invZ + (SCREEN_WIDTH * 0.5f));
    *sy = (int16)(final_y * invZ + (SCREEN_HEIGHT * 0.5f));

    return MR_SUCCESS;
}

int32 mrc_event(int32 ev, int32 p0, int32 p1) {
    if (ev == MR_KEY_PRESS) {
        if (p0 ==  MR_KEY_UP) g_key_up = 1;
        if (p0 == MR_KEY_DOWN) g_key_down = 1;  //нерабочие нейрокакашки
        if (p0 == MR_KEY_LEFT) g_key_left = 1;
        if (p0 == MR_KEY_RIGHT) g_key_right = 1;
        if (g_key_up) {
        dx += my_sinf(cam.yaw) * move_speed;
        dz += my_cosf(cam.yaw) * move_speed;
        g_key_up = 0;
        }
        if (g_key_down) {
        dx -= my_sinf(cam.yaw) * move_speed;
        dz -= my_cosf(cam.yaw) * move_speed;
         g_key_down = 0;
        }
        if (g_key_left) {
        dx -= my_cosf(cam.yaw) * move_speed;
        dz += my_sinf(cam.yaw) * move_speed;
         g_key_left = 0;
        }
        if (g_key_right) {
        dx += my_cosf(cam.yaw) * move_speed;
        dz -= my_sinf(cam.yaw) * move_speed;
         g_key_right = 0;
        }
        if (ev == MR_KEY_RELEASE){
            g_key_up = 0;
            g_key_down = 0;
            g_key_left = 0;
            g_key_right = 0;
        }

        cam.pos.x += dx;
        cam.pos.z += dz;
        
    }
    if (ev == MR_MOUSE_DOWN) {
        g_touch_active = 1;
        g_curr_tx = p0;
        g_curr_ty = p1;
        diff_x = g_curr_tx - 120;
        diff_y = g_curr_ty - 160;
        
    } else if (ev == MR_MOUSE_UP) {
        g_touch_active = 0;
        diff_x = 0;
        diff_y = 0;
    }
    
    return 0;
}

void gameStart() {
    int x, y, z;
    cam.pos.x = CHUNK_SIZE / 2.0f;
    cam.pos.y = 2.0f;
    cam.pos.z = CHUNK_SIZE / 2.0f;
    cam.yaw = 0.0f;
    cam.pitch = 0.0f;
    g_key_up = g_key_down = g_key_left = g_key_right = 0;
    g_touch_active = 0;
    g_last_tx = 120;
    g_last_ty = 160;
    g_curr_tx = 120;
    g_curr_ty = 160;

    for (x = 0; x < CHUNK_SIZE; x++) {
        for (y = 0; y < CHUNK_SIZE; y++) {
            for (z = 0; z < CHUNK_SIZE; z++) {
                if (y == 0) world[x][y][z] = BLOCK_BEDROCK;
                else if (y == 1) world[x][y][z] = BLOCK_STONE;
                else if (y == 2) world[x][y][z] = BLOCK_DIRT;
                else if (y == 3) world[x][y][z] = BLOCK_GRASS;
                else world[x][y][z] = BLOCK_AIR;
            }
        }
    }
}
void gameUpdate() {
    dx = 0.0f;
    dz = 0.0f;
    cam.yaw   += (float)diff_x * rot_speed;
    cam.pitch -= (float)diff_y * rot_speed;
    if (cam.pitch > 1.5f) cam.pitch = 1.5f;
    if (cam.pitch < -1.5f) cam.pitch = -1.5f;
}

void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8 r, uint8 g, uint8 b) {
    // Находим границы по Y
    int y;
    float intersectX[3];
    int count = 0;
    float leftX, rightX;
    int minY = y0,maxY = y0;
    if (y1 < minY) minY = y1; if (y2 < minY) minY = y2;
    if (y1 > maxY) maxY = y1; if (y2 > maxY) maxY = y2;

    for (y = minY; y <= maxY; y++) {
        count = 0;
        if ((y >= y0 && y <= y1) || (y >= y1 && y <= y0)) {
            if (y0 != y1) intersectX[count++] = x0 + (float)(x1 - x0) * (y - y0) / (y1 - y0);
        }
        // Ребро 1-2
        if ((y >= y1 && y <= y2) || (y >= y2 && y <= y1)) {
            if (y1 != y2) intersectX[count++] = x1 + (float)(x2 - x1) * (y - y1) / (y2 - y1);
        }
        // Ребро 2-0
        if ((y >= y2 && y <= y0) || (y >= y0 && y <= y2)) {
            if (y2 != y0) intersectX[count++] = x2 + (float)(x0 - x2) * (y - y2) / (y0 - y2);
        }

        if (count >= 2) {
            leftX = intersectX[0], rightX = intersectX[1];
            if (leftX > rightX) { float tmp = leftX; leftX = rightX; rightX = tmp; }
            mrc_drawLine((int16)leftX, y, (int16)rightX, y, r, g, b);
        }
    }
}
 
void drawCube(float fx, float fy, float fz, int type, float cosY, float sinY, float cosP, float sinP) {
    int16 px[8], py[8], pz[8];
    float s = 0.5f;
    float v_x[8], v_y[8], v_z[8];
    uint8 r, g, b;
    int i, f;
    int16 v0, v1, v2, v3, sx, sy, sz;
    int32 edge1x, edge1y, edge2x, edge2y, normalZ;
    const int8* face;
    static const int8 faces[6][4] = {
    {0, 3, 2, 1}, {4, 5, 6, 7}, {0, 1, 5, 4},
    {3, 2, 6, 7}, {0, 4, 7, 3}, {1, 2, 6, 5}
    };

    // 1. Предварительное определение цветов (выносим switch из цикла граней!)
    // Это экономит до 5 проветок условий за один вызов куба.
    if (type == BLOCK_DIRT)      { r = 139; g = 69;  b = 19; }
    else if (type == BLOCK_GRASS) { r = 150; g = 255; b = 150; }
    else                         { r = 128; g = 128; b = 128; }

    v_x[0] = fx - s; v_y[0] = fy - s; v_z[0] = fz - s;
    v_x[1] = fx + s; v_y[1] = fy - s; v_z[1] = fz - s;
    v_x[2] = fx + s; v_y[2] = fy + s; v_z[2] = fz - s;
    v_x[3] = fx - s; v_y[3] = fy + s; v_z[3] = fz - s;
    v_x[4] = fx - s; v_y[4] = fy - s; v_z[4] = fz + s;
    v_x[5] = fx + s; v_y[5] = fy - s; v_z[5] = fz + s;
    v_x[6] = fx + s; v_y[6] = fy + s; v_z[6] = fz + s;
    v_x[7] = fx - s; v_y[7] = fy + s; v_z[7] = fz + s;

    // 2. Проекция и СТРОГИЙ CLIPPING (Пункт 1)
    for (i = 0; i < 8; i++) {
        Vec3 world_pt;
        world_pt.x = v_x[i];
        world_pt.y = v_y[i];
        world_pt.z = v_z[i];
        
        if (projectPoint(world_pt, &cam,  cosY, sinY, cosP, sinP, &sx, &sy, &sz) != MR_SUCCESS) {
            return; // Если хотя бы одна точка не прошла проекцию/clipping — бросаем рендер блока
        }
        
        // Проверка границ экрана: если вершина за пределами [0...WIDTH-1] или [0...HEIGHT-1]
        if ((uint16)sx >= SCREEN_WIDTH || (uint16)sy >= SCREEN_HEIGHT) {
            return; // Бросаем рендер всего куба
        }
        
        px[i] = sx; py[i] = sy; pz[i] = sz;
    }

    for (f = 0; f < 6; f++) {
        face = faces[f];
        v0 = face[0]; 
        v1 = face[1]; 
        v2 = face[2]; 
        v3 = face[3];
        edge1x = px[v1] - px[v0];
        edge1y = py[v1] - py[v0];
        edge2x = px[v2] - px[v0];
        edge2y = py[v2] - py[v0];
        normalZ = (edge1x * edge2y) - (edge1y * edge2x);

        if (normalZ > 0) {
            fillTriangle(px[v0], py[v0], px[v1], py[v1], px[v2], py[v2], r, g, b);
            fillTriangle(px[v0], py[v0], px[v2], py[v2], px[v3], py[v3], r, g, b);

            // Отрисовка границ видимой грани
            //mrc_drawLine(px[v0], py[v0], px[v1], py[v1], 100, 100, 100);
           // mrc_drawLine(px[v1], py[v1], px[v2], py[v2], 100, 100, 100);
            //mrc_drawLine(px[v2], py[v2], px[v3], py[v3], 100, 100, 100);
            mrc_drawLine(px[v3], py[v3], px[v0], py[v0], 100, 100, 100);
        }
    }
}

void gameDraw(float cosY, float sinY, float cosP, float sinP) {
     drawCube(2.0f, 6.0f, 2.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(2.0f, 6.0f, 3.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(2.0f, 6.0f, 4.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(2.0f, 6.0f, 5.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(2.0f, 6.0f, 6.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(2.0f, 6.0f, 7.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(2.0f, 6.0f, 8.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(2.0f, 6.0f, 9.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(3.0f, 6.0f, 2.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(3.0f, 6.0f, 3.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(3.0f, 6.0f, 4.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(3.0f, 6.0f, 5.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(3.0f, 6.0f, 6.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(3.0f, 6.0f, 7.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(3.0f, 6.0f, 8.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(3.0f, 6.0f, 9.0f, 1,cosY,sinY,cosP,sinP);     
     drawCube(3.0f, 6.0f, 9.0f, 1,cosY,sinY,cosP,sinP);     
     drawCube(4.0f, 6.0f, 2.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(4.0f, 6.0f, 3.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(4.0f, 6.0f, 4.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(4.0f, 6.0f, 5.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(4.0f, 6.0f, 6.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(4.0f, 6.0f, 7.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(4.0f, 6.0f, 8.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(4.0f, 6.0f, 9.0f, 1,cosY,sinY,cosP,sinP);   
     drawCube(5.0f, 6.0f, 2.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(5.0f, 6.0f, 3.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(5.0f, 6.0f, 4.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(5.0f, 6.0f, 5.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(5.0f, 6.0f, 6.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(5.0f, 6.0f, 7.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(5.0f, 6.0f, 8.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(5.0f, 6.0f, 9.0f, 1,cosY,sinY,cosP,sinP);   
     drawCube(6.0f, 6.0f, 2.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(6.0f, 6.0f, 3.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(6.0f, 6.0f, 4.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(6.0f, 6.0f, 5.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(6.0f, 6.0f, 6.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(6.0f, 6.0f, 7.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(6.0f, 6.0f, 8.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(6.0f, 6.0f, 9.0f, 1,cosY,sinY,cosP,sinP);       
     drawCube(7.0f, 6.0f, 2.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(7.0f, 6.0f, 3.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(7.0f, 6.0f, 4.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(7.0f, 6.0f, 5.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(7.0f, 6.0f, 6.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(7.0f, 6.0f, 7.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(7.0f, 6.0f, 8.0f, 1,cosY,sinY,cosP,sinP);
     drawCube(7.0f, 6.0f, 9.0f, 1,cosY,sinY,cosP,sinP);
     
     
    drawCube(3.0f, 5.0f, 10.0f, 2,cosY,sinY,cosP,sinP);
    drawCube(4.0f, 5.0f, 10.0f, 2,cosY,sinY,cosP,sinP);
    drawCube(5.0f, 5.0f, 10.0f, 2,cosY,sinY,cosP,sinP);
    drawCube(6.0f, 5.0f, 10.0f, 2,cosY,sinY,cosP,sinP);
    drawCube(7.0f, 5.0f, 10.0f, 2,cosY,sinY,cosP,sinP);
    drawCube(7.0f, 4.0f, 10.0f, 2,cosY,sinY,cosP,sinP);
    drawCube(4.0f, 3.0f, 10.0f, 2,cosY,sinY,cosP,sinP);
    drawCube(5.0f, 3.0f, 10.0f, 2,cosY,sinY,cosP,sinP);
    drawCube(6.0f, 3.0f, 10.0f, 2,cosY,sinY,cosP,sinP);
    drawCube(3.0f, 2.0f, 10.0f, 2,cosY,sinY,cosP,sinP);
    drawCube(3.0f, 1.0f, 10.0f, 2,cosY,sinY,cosP,sinP); 
    
    drawCube(5.0f, 5.0f, 7.0f, 2,cosY,sinY,cosP,sinP);
    drawCube(6.0f, 5.0f, 7.0f, 2,cosY,sinY,cosP,sinP);
    drawCube(7.0f, 5.0f, 7.0f, 2,cosY,sinY,cosP,sinP);
    drawCube(8.0f, 5.0f, 7.0f, 2,cosY,sinY,cosP,sinP);
    drawCube(9.0f, 5.0f, 7.0f, 2,cosY,sinY,cosP,sinP);
    drawCube(9.0f, 4.0f, 7.0f, 2,cosY,sinY,cosP,sinP);
    drawCube(6.0f, 3.0f, 7.0f, 2,cosY,sinY,cosP,sinP);
    drawCube(7.0f, 3.0f, 7.0f, 2,cosY,sinY,cosP,sinP);
    drawCube(8.0f, 3.0f, 7.0f, 2,cosY,sinY,cosP,sinP);
    drawCube(5.0f, 2.0f, 7.0f, 2,cosY,sinY,cosP,sinP);
    drawCube(5.0f, 1.0f, 7.0f, 2,cosY,sinY,cosP,sinP); 
    
    }
    
void mrc_draw(int32 data)
{
    float aY = -cam.yaw;
    float aP = -cam.pitch;
    float cY = my_cosf(aY);
    float sY = my_sinf(aY);
    float cP = my_cosf(aP);
    float sP = my_sinf(aP);
	mrc_clearScreen(147, 180, 255);
	gameUpdate();
	gameDraw(cY, sY, cP, sP);
    gDrawBitmap(&handBmp, SCREEN_WIDTH - handBmp.width, SCREEN_HEIGHT - handBmp.height - 20);
    mrc_refreshScreen(0,0,SCREEN_WIDTH,SCREEN_HEIGHT);
}
int32 mrc_init(void)
{
    globalTimer = mrc_timerCreate();
    initSpriteData();
    gameStart();
    mrc_timerStart(globalTimer, 100, 0, mrc_draw, 1);
	return MR_SUCCESS;
}