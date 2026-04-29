#include <stdint.h>
#include "main.h"
#include <mrc_base.h>
#include <mrc_jgraphics.h>
#include <mrc_win.h>
#include <mrc_menu.h>
#include <mrc_text.h>
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define CHUNK_SIZE 8
#define PI 3.14159265f
#define STEP_SIZE 0.1f
#define EPSILON 0.001f
#define FOCAL_LENGTH 200.0f
mrc_jImageSt* testex;
mrc_jgraphics_func_table_t funk;
BlockType world[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
mrc_jgraphics_context_t *gContext; 
Camera cam;
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

float my_fmodf(float x, float y) {
    if (y == 0) return 0;
    return x - (float)((int)(x / y)) * y; 
}

float my_sinf(float x) {
    float res, term, angle;
    int i;
    angle = my_fmodf(x, 2.0f * PI);
    if (angle > PI) angle -= 2.0f * PI;
    else if (angle < -PI) angle += 2.0f * PI;
    res = angle;
    term = angle;
    for (i = 1; i <= 10; i++) {
        term = -angle * angle / ((2.0f * i) * (2.0f * i + 1.0f));
        res += term;
    }
    return res;
}

float my_cosf(float x) {
    float res, term, angle;
    int i;
    angle = my_fmodf(x, 2.0f * PI);
    if (angle > PI) angle -= 2.0f * PI;
    else if (angle < -PI) angle += 2.0f * PI;
    res = 1.0f;
    term = 1.0f;
    for (i = 1; i <= 10; i++) {
        term = -angle * angle / ((2.0f * i - 1.0f) * (2.0f * i));
        res += term;
    }
    return res;
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

/**
 * Проецирует точку из мировых координат в экранные координаты относительно камеры.
 * @param world_pt Точка в мире
 * @param cam Камера с позицией и углами
 * @param sx Выходной X на экране
 * @param sy Выходной Y на экране
 * @param sz Выходная глубина Z после трансформации (для сортировки или отсечения)
 * @return MR_SUCCESS если точка перед камерой, иначе ошибка (если за спиной)
 */
 
int32 projectPoint(Vec3 world_pt, Camera cam, int16 *sx, int16 *sy, int16 *sz) {
    // 1. Локальные координаты (Перенос центра мира в начало координат камеры)
    float x = world_pt.x - cam.pos.x;
    float y = world_pt.y - cam.pos.y;
    float z = world_pt.z - cam.pos.z;
    float projX, projY; // Промежуточные float для расчетов

    // Мы используем отрицательные углы камеры, чтобы перевести мир в пространство вида (View Space)
    //float yaw = -1*cam.yaw;
    //float pitch = -1*cam.pitch; // В твоем коде было так же, но важно соблюдать порядок

    // Но для чистоты возьмем значения напрямую из параметров трансформации
    float aY = -1*cam.yaw;
    float aP = -1*cam.pitch;

    float cosY = my_cosf(aY);
    float sinY = my_sinf(aY);
    float cosP = my_cosf(aP);
    float sinP = my_sinf(aP);

    // 2. Поворот вокруг оси Y (Yaw) — меняет X и Z
    // Это перемещает точку по горизонтали относительно взгляда
    float rx = x * cosY + z * sinY;
    float rz = -x * sinY + z * cosY;
    float ry = y; // Y не меняется при повороте вокруг вертикальной оси

    // 3. Поворот вокруг оси X (Pitch) — меняет Y и Z
    // Теперь мы вращаем полученные координаты вокруг новой горизонтальной оси
    float final_x = rx;
    float final_y = ry * cosP - rz * sinP;
    float final_z = ry * sinP + rz * cosP;

    // Сохраняем глубину для сортировки или отсечения
    *sz = (int16)final_z;

    // 4. Clipping: Проверка "Near Plane"
    // Если точка за камерой или слишком близко к ней (Z <= 0), она должна исчезнуть.
    // Увеличиваем порог до 0.5f-1.0f, чтобы избежать деления на околонулевые числа
    if (final_z < 0.7f) {
        return MR_FAILED;
    }

    // 5. Перспективная проекция
    // Формула: ScreenCoord = (CameraCoord / Z) * FocalLength + Center
    projX = (final_x / final_z) * FOCAL_LENGTH + (SCREEN_WIDTH / 2.0f);
    projY = (final_y / final_z) * FOCAL_LENGTH + (SCREEN_HEIGHT / 2.0f);

    // 6. Явное приведение типов к int16 и запись по указателю
    // Это самый безопасный способ для старого компилятора
    *sx = (int16)projX;
    *sy = (int16)projY;

    return MR_SUCCESS;
}

/* Главный обработчик событий ввода */
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
    cam.pos.y = 5.0f;
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
        

        // Проверяем пересечение горизонтальной линии с каждым из трех ребер
        // Ребро 0-1
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

        // Если нашли две точки пересечения, рисуем линию между ними
        if (count >= 2) {
            // Сортируем X для корректного рисования слева направо
            leftX = intersectX[0], rightX = intersectX[1];
            if (leftX > rightX) { float tmp = leftX; leftX = rightX; rightX = tmp; }
            mrc_drawLine((int16)leftX, y, (int16)rightX, y, r, g, b);
        }
    }
}
 
void drawCube(float fx, float fy, float fz, int type, mrc_jImageSt* tex) {
    Vec3 vertices[8];
    int16 px[8], py[8], pz[8];
    float s = 0.5f; // Размер половины куба
    int i, f;
    float edge1x, edge1y, edge2x, edge2y, normalZ;
    int v0, v1, v2, v3;

    // Определение граней куба через индексы вершин (обход по часовой стрелке для нормалей)
    int faces[6][4] = {
        {0, 3, 2, 1}, // Front
        {4, 5, 6, 7}, // Back
        {0, 1, 5, 4}, // Bottom
        {3, 2, 6, 7}, // Top
        {0, 4, 7, 3}, // Left
        {1, 2, 6, 5}  // Right
    };

    // Определение вершин куба
    vertices[0].x = fx-s; vertices[0].y = fy-s; vertices[0].z = fz-s;
    vertices[1].x = fx+s; vertices[1].y = fy-s; vertices[1].z = fz-s;
    vertices[2].x = fx+s; vertices[2].y = fy+s; vertices[2].z = fz-s;
    vertices[3].x = fx-s; vertices[3].y = fy+s; vertices[3].z = fz-s;
    vertices[4].x = fx-s; vertices[4].y = fy-s; vertices[4].z = fz+s;
    vertices[5].x = fx+s; vertices[5].y = fy-s; vertices[5].z = fz+s;
    vertices[6].x = fx+s; vertices[6].y = fy+s; vertices[6].z = fz+s;
    vertices[7].x = fx-s; vertices[7].y = fy+s; vertices[7].z = fz+s;

    // Проекция всех точек (с учетом clipping в projectPoint)
    for (i = 0; i < 8; i++) {
        if (projectPoint(vertices[i], cam, &px[i], &py[i], &pz[i]) != MR_SUCCESS) {
            // Если точка за камерой, ставим "защитное" значение
            //px[i] = -100; py[i] = -100; pz[i] = -100;
        }
    }

    // Рендеринг каждой грани
    for (f = 0; f < 6; f++) {
        v0 = faces[f][0];
        v1 = faces[f][1];
        v2 = faces[f][2];
        v3 = faces[f][3];

        // --- Back-face Culling ---
        edge1x = px[v1] - px[v0];
        edge1y = py[v1] - py[v0];
        edge2x = px[v2] - px[v0];
        edge2y = py[v2] - py[v0];
        normalZ = (edge1x * edge2y) - (edge1y * edge2x);

        // Если грань смотрит на нас
        if (normalZ > 0) {
            // 1. Заливка цветом (бледно-зеленый)
            fillTriangle(px[v0], py[v0], px[v1], py[v1], px[v2], py[v2], 150, 255, 150);
            fillTriangle(px[v0], py[v0], px[v2], py[v2], px[v3], py[v3], 150, 255, 150);

            // 2. Отрисовка границ ЭТОЙ конкретной видимой грани темными линиями
            // Мы рисуем линии только здесь, чтобы не рисовать задние ребра куба
            mrc_drawLine(px[v0], py[v0], px[v1], py[v1], 100, 100, 100);
            mrc_drawLine(px[v1], py[v1], px[v2], py[v2], 100, 100, 100);
            mrc_drawLine(px[v2], py[v2], px[v3], py[v3], 100, 100, 100);
            mrc_drawLine(px[v3], py[v3], px[v0], py[v0], 100, 100, 100);
        }
    }

    /* СТАРЫЙ ЦИКЛ ПО edges[12][2] УДАЛЕН ОТСЮДА */
}

void gameDraw() {
    drawCube(3.0f, 5.0f, 10.0f, 0, testex);
    drawCube(4.0f, 5.0f, 10.0f, 0, testex);
    drawCube(5.0f, 5.0f, 10.0f, 0, testex);
    drawCube(6.0f, 5.0f, 10.0f, 0, testex);
    drawCube(7.0f, 5.0f, 10.0f, 0, testex);
    drawCube(7.0f, 4.0f, 10.0f, 0, testex);
    drawCube(4.0f, 3.0f, 10.0f, 0, testex);
    drawCube(5.0f, 3.0f, 10.0f, 0, testex);
    drawCube(6.0f, 3.0f, 10.0f, 0, testex);
    drawCube(3.0f, 2.0f, 10.0f, 0, testex);
    drawCube(3.0f, 1.0f, 10.0f, 0, testex);
    drawCube(3.0f, 0.0f, 10.0f, 0, testex);
    drawCube(4.0f, 0.0f, 10.0f, 0, testex);
    drawCube(5.0f, 0.0f, 10.0f, 0, testex);
    drawCube(6.0f, 0.0f, 10.0f, 0, testex);
    drawCube(7.0f, 0.0f, 10.0f, 0, testex);
}
void mrc_draw(int32 data)
{
	mrc_clearScreen(147, 180, 255);
	gameUpdate();
	gameDraw();
    mrc_refreshScreen(0,0,SCREEN_WIDTH,SCREEN_HEIGHT);
}
int32 mrc_init(void)
{
    globalTimer = mrc_timerCreate();
    gameStart();
    mrc_timerStart(globalTimer, 50, 0, mrc_draw, 1);
	return MR_SUCCESS;
}