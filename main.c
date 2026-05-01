#include "main.h"
#include <mrc_base.h>
#include <mrc_exb.h>
#include <mrc_jgraphics.h>
#include <mrc_win.h>
#include <mrc_text.h>
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define SCREEN_WIDTH2 260
#define SCREEN_HEIGHT2 290
#define SCREEN_WIDTH2F 130.0f
#define SCREEN_HEIGHT2F 140.0f
#define WORLD_SX 16
#define WORLD_SY 8
#define WORLD_SZ 16
#define HALFPI 1.57079637f
#define PI 3.14159265f
#define TWO_PI 6.283185307f
#define FOCAL_LENGTH 200.0f
#define NEAR_Z 0.5f
#define SIN_LUT_SIZE 256
#define SIN_LUT_SIZEF 256.0F
static float sin_lut[SIN_LUT_SIZE];
uint8 world[WORLD_SX][WORLD_SY][WORLD_SZ];
uint8 visibleFaces[WORLD_SX][WORLD_SY][WORLD_SZ];

/* Face index -> neighbour offset.
   Order matches `faces[6][4]` in drawCube:
     0 = -Z, 1 = +Z, 2 = -Y, 3 = +Y, 4 = -X, 5 = +X */
static const int8 face_dx[6] = {  0,  0,  0,  0, -1,  1 };
static const int8 face_dy[6] = {  0,  0, -1,  1,  0,  0 };
static const int8 face_dz[6] = { -1,  1,  0,  0,  0,  0 };
mrc_jgraphics_context_t *gContext; 
Camera cam;
CBitmap handBmp;
CBitmap GUIBmp;
int g_curr_tx = 120;
int g_curr_ty = 160;
int diff_x = 0;
int diff_y = 0;
float dx = 0;
float dz = 0;
float move_speed = 1.0f;
float rot_speed = 0.0004f;
int globalTimer;


float my_fmodf(float x, float y) {
    if (y == 0) return 0;
    return x - (float)((int)(x / y)) * y; 
}

float my_sinf_legacy(float x) {
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

void initSinLUT(void) {
    int i;
    float angle;
    for (i = 0; i < SIN_LUT_SIZE; i++) {
        angle = ((float)i / (float)SIN_LUT_SIZE) * TWO_PI;
        sin_lut[i] = (float)my_sinf_legacy(angle); 
    }
}

__inline void initSprite(CBitmap* bmp, int w, int h, unsigned char* pixels)
{
	bmp->width = w;
	bmp->height = h;
	bmp->pixels = pixels;
}

void initSpriteData()
{
	initSprite(&handBmp, 64, 64, (unsigned char*)_sprHand);
    initSprite(&GUIBmp, 240, 64, (unsigned char*)_sprGUIPH);
}

float my_sinf(float x) {
    int index;
    float phase;
    phase = x;
    while (phase < 0) phase += TWO_PI;
    while (phase >= TWO_PI) phase -= TWO_PI;
    index = (int)((phase / TWO_PI) * SIN_LUT_SIZEF);
    return sin_lut[index];
}

float my_cosf(float x) {
    return my_sinf(x + HALFPI);
}

void gDrawBitmap(CBitmap* bmp, int x, int y)
{
	mrc_bitmapShowEx((uint16*)bmp->pixels, x, y, bmp->width, bmp->width, bmp->height, BM_TRANSPARENT, 0, 0);
}
void gDrawBitmapNt(CBitmap* bmp, int x, int y)
{
	mrc_bitmapShowEx((uint16*)bmp->pixels, x, y, bmp->width, bmp->width, bmp->height, BM_COPY, 0, 0);
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

    if (final_z < NEAR_Z) {
        return MR_FAILED;
    }

    *sz = (int16)final_z;
    invZ = FOCAL_LENGTH / final_z;

    *sx = (int16)(final_x * invZ + SCREEN_WIDTH2F);
    *sy = (int16)(final_y * invZ + SCREEN_HEIGHT2F);

    return MR_SUCCESS;
}

int32 mrc_event(int32 ev, int32 p0, int32 p1) {
    if (ev == MR_KEY_PRESS) {
        dx = 0.0f;
        dz = 0.0f;
        if (p0 ==  MR_KEY_UP) {
        dx += my_sinf(cam.yaw) * move_speed;
        dz += my_cosf(cam.yaw) * move_speed;
        }
        if (p0 == MR_KEY_DOWN) {
        dx -= my_sinf(cam.yaw) * move_speed;
        dz -= my_cosf(cam.yaw) * move_speed;
        }
        if (p0 == MR_KEY_LEFT) {
        dx -= my_cosf(cam.yaw) * move_speed;
        dz += my_sinf(cam.yaw) * move_speed;
        }
        if (p0 == MR_KEY_RIGHT) {
        dx += my_cosf(cam.yaw) * move_speed;
        dz -= my_sinf(cam.yaw) * move_speed;
        }

        cam.pos.x += dx;
        cam.pos.z += dz;
        
    } else if (ev == MR_MOUSE_DOWN) {
        g_curr_tx = p0;
        g_curr_ty = p1;
        diff_x = g_curr_tx - 120;
        diff_y = g_curr_ty - 160;
        
    } else if (ev == MR_MOUSE_UP) {
        diff_x = 0;
        diff_y = 0;
    }
    
    return MR_SUCCESS;
}

uint8 getBlock(int x, int y, int z) {
    if (x < 0 || x >= WORLD_SX) return (uint8)BLOCK_AIR;
    if (y < 0 || y >= WORLD_SY) return (uint8)BLOCK_AIR;
    if (z < 0 || z >= WORLD_SZ) return (uint8)BLOCK_AIR;
    return world[x][y][z];
}

uint8 computeFaceMask(int x, int y, int z) {
    int f;
    uint8 mask;
    if (world[x][y][z] == (uint8)BLOCK_AIR) return 0;
    mask = 0;
    for (f = 0; f < 6; f++) {
        if (getBlock(x + face_dx[f], y + face_dy[f], z + face_dz[f]) == (uint8)BLOCK_AIR) {
            mask = (uint8)(mask | (1 << f));
        }
    }
    return mask;
}

void rebuildAllFaceMasks(void) {
    int x, y, z;
    for (x = 0; x < WORLD_SX; x++) {
        for (y = 0; y < WORLD_SY; y++) {
            for (z = 0; z < WORLD_SZ; z++) {
                visibleFaces[x][y][z] = computeFaceMask(x, y, z);
            }
        }
    }
}

/* Recompute the face mask for (x,y,z) and its 6 axis neighbours.
   Call this after a single block changes (break / place). */
void updateFaceMaskRegion(int x, int y, int z) {
    int f, nx, ny, nz;
    if (x >= 0 && x < WORLD_SX && y >= 0 && y < WORLD_SY && z >= 0 && z < WORLD_SZ) {
        visibleFaces[x][y][z] = computeFaceMask(x, y, z);
    }
    for (f = 0; f < 6; f++) {
        nx = x + face_dx[f];
        ny = y + face_dy[f];
        nz = z + face_dz[f];
        if (nx >= 0 && nx < WORLD_SX && ny >= 0 && ny < WORLD_SY && nz >= 0 && nz < WORLD_SZ) {
            visibleFaces[nx][ny][nz] = computeFaceMask(nx, ny, nz);
        }
    }
}

void gameStart() {
    int x, y, z;
    cam.pos.x = (float)WORLD_SX / 2.0f;
    cam.pos.y = 6.0f;
    cam.pos.z = -2.0f;
    cam.yaw = 0.0f;
    cam.pitch = 0.0f;

    for (x = 0; x < WORLD_SX; x++) {
        for (y = 0; y < WORLD_SY; y++) {
            for (z = 0; z < WORLD_SZ; z++) {
                if (y == 0) world[x][y][z] = (uint8)BLOCK_BEDROCK;
                else if (y == 1 || y == 2) world[x][y][z] = (uint8)BLOCK_STONE;
                else if (y == 3) world[x][y][z] = (uint8)BLOCK_DIRT;
                else if (y == 4) world[x][y][z] = (uint8)BLOCK_GRASS;
                else world[x][y][z] = (uint8)BLOCK_AIR;
            }
        }
    }
}

void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8 r, uint8 g, uint8 b) {
    /* Объявление всех переменных в начале метода (C89) */
    int32 minY, maxY, y;
    int32 i, count;
    int32 ex_fp[3];      /* Текущие X координаты ребер в формате Fixed-point (Q16.16) */
    int32 edx_step[3];   /* Шаг изменения X на одну строку Y (Fixed-point) */
    int32 ey_start[3];   /* Начальная координата Y ребра */
    int32 ey_end[3];     /* Конечная координата Y ребра */
    int32 e_active[3];    /* Флаг: является ли ребро активным (не горизонтальным) */
    int32 ix[2];         /* Найденные пересечения X для текущей строки scanline */
    uint8 final_r, final_g, final_b; /* Локальные копии цветов */
    int32 joker;
    int32 temp_dy;
    int32 p0x, p0y, p1x, p1y, p2x, p2y;

    /* Подготовка данных */
    p0x = x0; p0y = y0;
    p1x = x1; p1y = y1;
    p2x = x2; p2y = y2;

    /* 1. Определяем границы отрисовки */
    minY = y0; if (y1 < minY) minY = y1; if (y2 < minY) minY = y2;
    maxY = y0; if (y1 > maxY) maxY = y1; if (y2 > maxY) maxY = y2;

    /* Если треугольник слишком плоский по вертикали, выходим */
    if (minY == maxY) return;

    /* Инициализация переменных ребер */
    for (i = 0; i < 3; i++) {
        e_active[i] = 0;
    }

    /* 2. Настройка трех ребер: (P0-P1), (P1-P2), (P2-P0) */
    /* Ребро 0: P0 -> P1 */
    temp_dy = p1y - p0y;
    if (temp_dy != 0) {
        e_active[0] = 1;
        if (p0y < p1y) {
            ey_start[0] = p0y; ey_end[0] = p1y;
            ex_fp[0] = p0x << 16;
            edx_step[0] = ((p1x - p0x) << 16) / temp_dy;
        } else {
            ey_start[0] = p1y; ey_end[0] = p0y;
            ex_fp[0] = p1x << 16;
            edx_step[0] = ((p0x - p1x) << 16) / (-temp_dy);
        }
    }

    /* Ребро 1: P1 -> P2 */
    temp_dy = p2y - p1y;
    if (temp_dy != 0) {
        e_active[1] = 1;
        if (p1y < p2y) {
            ey_start[1] = p1y; ey_end[1] = p2y;
            ex_fp[1] = p1x << 16;
            edx_step[1] = ((p2x - p1x) << 16) / temp_dy;
        } else {
            ey_start[1] = p2y; ey_end[1] = p1y;
            ex_fp[1] = p2x << 16;
            edx_step[1] = ((p1x - p2x) << 16) / (-temp_dy);
        }
    }

    /* Ребро 2: P2 -> P0 */
    temp_dy = p0y - p2y;
    if (temp_dy != 0) {
        e_active[2] = 1;
        if (p2y < p0y) {
            ey_start[2] = p2y; ey_end[2] = p0y;
            ex_fp[2] = p2x << 16;
            edx_step[2] = ((p0x - p2x) << 16) / temp_dy;
        } else {
            ey_start[2] = p0y; ey_end[2] = p2y;
            ex_fp[2] = p0x << 16;
            edx_step[2] = ((p2x - p0x) << 16) / (-temp_dy);
        }
    }

    /* 3. Основной цикл Scanline Renderer */
    for (y = minY; y <= maxY; y++) {
        count = 0;
        
        /* Ищем пересечения текущей строки Y с активными ребрами */
        for (i = 0; i < 3; i++) {
            if (e_active[i]) {
                /* Используем полуоткрытый интервал [start, end), чтобы избежать двойного учета вершин */
                if (y >= ey_start[i] && y < ey_end[i]) {
                    ix[count++] = ex_fp[i] >> 16; /* Перевод из Fixed-point обратно в int */
                    
                    /* Обновляем X для следующей строки сразу здесь (инкрементально) */
                    ex_fp[i] += edx_step[i];
                }
            }
        }

        /* Если нашли ровно два пересечения — это полноценная горизонтальная линия треугольника */
        if (count == 2) {
            /* Сортируем ix[0] и ix[1], чтобы leftX был меньше rightX */
            if (ix[0] > ix[1]) {
                long temp_x;
                temp_x = ix[0]; ix[0] = ix[1]; ix[1] = temp_x;
            }

            joker = (y % 16);
            final_r = r + joker; final_g = g + joker; final_b = b + joker;

            /* Рисуем линию через API библиотеки */
            mrc_drawLine((int16)ix[0], (int16)y, (int16)ix[1], (int16)y, final_r, final_g, final_b);
        }
    }
}
 
void drawCube(int32 fx, int32 fy, int32 fz, int type, uint8 faceMask, float cosY, float sinY, float cosP, float sinP) {
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
    if (type == BLOCK_DIRT) { r = 139; g = 69; b = 19; }
    else if (type == BLOCK_GRASS) { r = 150; g = 210; b = 150; }
    else if (type == BLOCK_STONE) { r = 128; g = 128; b = 128; }
    else if (type == BLOCK_BEDROCK) { r = 64; g = 64; b = 64; }
    else { r = 128; g = 128; b = 128; }
    v_x[0] = (float)fx - s; v_y[0] = (float)fy - s; v_z[0] = (float)fz - s;
    v_x[1] = (float)fx + s; v_y[1] = (float)fy - s; v_z[1] = (float)fz - s;
    v_x[2] = (float)fx + s; v_y[2] = (float)fy + s; v_z[2] = (float)fz - s;
    v_x[3] = (float)fx - s; v_y[3] = (float)fy + s; v_z[3] = (float)fz - s;
    v_x[4] = (float)fx - s; v_y[4] = (float)fy - s; v_z[4] = (float)fz + s;
    v_x[5] = (float)fx + s; v_y[5] = (float)fy - s; v_z[5] = (float)fz + s;
    v_x[6] = (float)fx + s; v_y[6] = (float)fy + s; v_z[6] = (float)fz + s;
    v_x[7] = (float)fx - s; v_y[7] = (float)fy + s; v_z[7] = (float)fz + s;

    for (i = 0; i < 8; i++) {
        Vec3 world_pt;
        world_pt.x = v_x[i];
        world_pt.y = v_y[i];
        world_pt.z = v_z[i];
        if (projectPoint(world_pt, &cam, cosY, sinY, cosP, sinP, &sx, &sy, &sz) != MR_SUCCESS) {
            return;
        }

        px[i] = sx; py[i] = sy; pz[i] = sz;
    }

    for (f = 0; f < 6; f++) {
        if (!(faceMask & (1 << f))) continue;
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
            //mrc_drawLine(px[v3], py[v3], px[v0], py[v0], 100, 100, 100);
        }
    }
}

void gameDraw(float cosY, float sinY, float cosP, float sinP) {
    int x, y, z;
    uint8 type;
    uint8 mask;
    for (x = 0; x < WORLD_SX; x++) {
        for (y = 0; y < WORLD_SY; y++) {
            for (z = 0; z < WORLD_SZ; z++) {
                type = world[x][y][z];
                if (type == BLOCK_AIR) continue;
                mask = visibleFaces[x][y][z];
                if (mask == 0) continue;
                drawCube((int32)x, (int32)y, (int32)z, (int)type, mask, cosY, sinY, cosP, sinP);
            }
        }
    }
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
	cam.yaw   += (float)diff_x * rot_speed;
    cam.pitch -= (float)diff_y * rot_speed;

	gameDraw(cY, sY, cP, sP);
    gDrawBitmap(&handBmp, SCREEN_WIDTH - handBmp.width, SCREEN_HEIGHT - handBmp.height - 70);
    gDrawBitmapNt(&GUIBmp, SCREEN_WIDTH - GUIBmp.width, SCREEN_HEIGHT - GUIBmp.height);
    mrc_refreshScreen(0,0,SCREEN_WIDTH,SCREEN_HEIGHT);
}
int32 mrc_init(void)
{
    globalTimer = mrc_timerCreate();
    initSinLUT();
    initSpriteData();
    gameStart();
    rebuildAllFaceMasks();
    mrc_timerStart(globalTimer, 100, 0, mrc_draw, 1);
	return MR_SUCCESS;
}