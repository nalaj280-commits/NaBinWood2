#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <wchar.h>
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <conio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#pragma warning(disable:4996)

#define MAP_WIDTH  40
#define MAP_HEIGHT 20
#define NUM_ROOMS  11

#define TILE_EMPTY  0
#define TILE_WALL   1
#define TILE_DESK   2
#define TILE_EXIT   7
#define TILE_STAIRS 8
#define TILE_CLOSET 15
#define TILE_MONITOR 16 
#define TILE_FIGURE 17 // 교수님의 한정판 애니 피규어가 숨겨진 액자
#define TILE_DROP_ZONE 18 // 피규어를 두는 특정 구역 추가
#define TILE_PURIFIER 19 // 2층 복도 정수기 추가
#define TILE_BLACKBOARD 20 // 2층 계단실 칠판 추가

#define COLOR_BLUE     9
#define COLOR_GREEN    10
#define COLOR_RED      12
#define COLOR_BROWN    6
#define COLOR_DARKGRAY 8
#define COLOR_WHITE    15
#define COLOR_PURPLE   13
#define COLOR_YELLOW   14
#define COLOR_CYAN     11

HANDLE hBuffer[2];
int    screenIndex = 0;
int    menu = 1;
int    isRunning = 1;

// ============================================================
//  더블 버퍼 기본 함수
// ============================================================
void init_double_buffer()
{
    // 커서 정보를 설정하기 위한 구조체. (크기 1, 표시 여부 FALSE: 숨김)
    CONSOLE_CURSOR_INFO ci = { 1, FALSE };

    for (int i = 0; i < 2; i++) {
        // 화면에 출력할 수 있는 콘솔 스크린 버퍼를 2개 생성합니다.
        hBuffer[i] = CreateConsoleScreenBuffer(
            GENERIC_READ | GENERIC_WRITE,       // 읽기/쓰기 권한 부여
            FILE_SHARE_READ | FILE_SHARE_WRITE, // 다른 프로세스와 공유 가능하게 설정
            NULL, CONSOLE_TEXTMODE_BUFFER, NULL);

        // 생성된 버퍼에 커서 숨김 설정을 적용합니다.
        SetConsoleCursorInfo(hBuffer[i], &ci);

        // 버퍼의 가로 170, 세로 60 크기를 설정합니다.
        COORD sz = { 170, 60 };
        SetConsoleScreenBufferSize(hBuffer[i], sz);
    }
}

void flip_buffer()
{
    // 현재 그리고 있던 백그라운드 버퍼를 실제 활성화된 화면으로 설정하여 사용자에게 보여줍니다.
    SetConsoleActiveScreenBuffer(hBuffer[screenIndex]);

    // 다음 번에는 다른 버퍼에 그림을 그리도록 인덱스를 반전시킵니다. (0 -> 1, 1 -> 0)
    screenIndex = !screenIndex;
}

void clear_buffer()
{
    COORD coord = { 0, 0 }; // 화면의 좌측 상단(0,0) 좌표
    DWORD dw;

    // 현재 그림을 그릴 백그라운드 버퍼(screenIndex)의 170*60 크기만큼을 넓은 문자 공백(L' ')으로 채웁니다.
    FillConsoleOutputCharacterW(hBuffer[screenIndex], L' ', 170 * 60, coord, &dw);

    // 같은 영역의 글자 속성(색상)을 7(기본 흰색 텍스트, 검은색 배경)로 초기화합니다.
    FillConsoleOutputAttribute(hBuffer[screenIndex], 7, 170 * 60, coord, &dw);

    // 지우기 작업이 끝난 후 커서를 다시 (0,0) 위치로 돌려놓습니다.
    SetConsoleCursorPosition(hBuffer[screenIndex], coord);
}

void clear_both_buffers()
{
    int temp = screenIndex; // 원래 인덱스를 임시로 저장해 둡니다.

    screenIndex = 0; clear_buffer(); // 0번 버퍼 비우기
    screenIndex = 1; clear_buffer(); // 1번 버퍼 비우기

    screenIndex = temp; // 작업이 끝난 후 원래 그리던 버퍼 인덱스로 복구합니다.
}

void flush_keyboard_buffer()
{
    // 키보드 버퍼에 읽지 않은 키 입력이 남아있는 동안 반복합니다.
    while (_kbhit()) {
        _getch(); // 남아있는 키보드 입력값을 허공으로 날려버립니다.
    }
}

void set_color_buf(int color)
{
    // 백그라운드 버퍼에 앞으로 출력할 텍스트의 속성(색상)을 지정합니다.
    SetConsoleTextAttribute(hBuffer[screenIndex], color);
}

void move_cursor_buf(int x, int y)
{
    // 인자로 받은 x, y 좌표를 COORD 구조체로 만들어 백그라운드 버퍼의 커서 위치를 이동시킵니다.
    COORD pos = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(hBuffer[screenIndex], pos);
}

void print_buf(const wchar_t* fmt, ...)
{
    wchar_t buf[2048]; // 조합된 문자열을 저장할 넉넉한 크기의 임시 버퍼입니다.

    // 가변 인자(..., %d, %s 등)를 처리하여 하나의 완성된 문자열로 만듭니다.
    va_list args;
    va_start(args, fmt);
    vswprintf(buf, sizeof(buf) / sizeof(wchar_t), fmt, args); // 유니코드 기반 포맷팅
    va_end(args);

    DWORD written;
    // 완성된 문자열(buf)을 현재 백그라운드 버퍼(hBuffer[screenIndex])에 출력(기록)합니다.
    WriteConsoleW(hBuffer[screenIndex], buf, lstrlenW(buf), &written, NULL);
}

// 문자열이 콘솔 화면에서 차지하는 실제 너비(칸 수)를 계산하는 함수입니다.
int get_width(const wchar_t* str)
{
    int w = 0; // 전체 너비를 저장할 변수

    // 문자열의 끝(NULL 문자)을 만날 때까지 한 글자씩 검사하며 반복합니다.
    while (*str) {
        // 현재 문자가 한글 유니코드 범위(가~힣: 0xAC00 ~ 0xD7A3)에 포함되는지 확인합니다.
        // 한글이면 2칸, 아니면(영어, 숫자, 기호 등) 1칸을 너비(w)에 더해줍니다.
        w += (*str >= 0xAC00 && *str <= 0xD7A3) ? 2 : 1;

        str++; // 다음 글자로 포인터를 이동합니다.
    }
    return w; // 계산된 총 너비를 반환합니다.
}

// 지정된 전체 너비(total_width) 안에서 텍스트를 왼쪽으로 정렬하여 출력하는 함수입니다.
void print_align_left(const wchar_t* text, int total_width)
{
    // 한글과 영어를 고려하여 텍스트의 실제 화면 너비를 구합니다.
    int w = get_width(text);

    // 텍스트를 백그라운드 버퍼에 먼저 출력합니다.
    print_buf(L"%ls", text);

    // 지정된 전체 넓이에서 글자가 차지한 너비를 뺀 나머지 빈 공간만큼 반복합니다.
    for (int i = w; i < total_width; i++)
        print_buf(L" "); // 빈 공간을 공백 문자로 채워 넣어 오른쪽 정렬 틀을 맞춰줍니다.
}

// 콘솔 창의 깜빡이는 커서를 보이거나 숨기는 함수입니다.
void set_cursor_visible(bool visible)
{
    // 커서 두께(1)와 표시 여부(visible이 참이면 TRUE, 거짓이면 FALSE)를 구조체로 설정합니다.
    CONSOLE_CURSOR_INFO ci = { 1, visible ? TRUE : FALSE };

    // 더블 버퍼링에 사용되는 0번 버퍼와 1번 버퍼 양쪽 모두에 커서 설정을 똑같이 적용합니다.
    SetConsoleCursorInfo(hBuffer[0], &ci);
    SetConsoleCursorInfo(hBuffer[1], &ci);
}
// ============================================================
//  맵 데이터 및 아이템 전역 변수
// ============================================================
int world_maps[NUM_ROOMS][MAP_HEIGHT][MAP_WIDTH];

typedef struct { int targetRoom, nextPlayerX, nextPlayerY; } DoorInfo;
DoorInfo doors[4];

int room_limit_width[NUM_ROOMS] = { 40,20,24,20,20, 40,20,16,20,24,20 };
int room_limit_height[NUM_ROOMS] = { 20,12,14,10,10, 20,12, 8,10,14,12 };

// 1층 탈출구방 열쇠 (비밀의방 Room 7에 위치)
int  itemRoom = 7, itemX = 8, itemY = 5;
bool hasExitRoomKey = false, isExitRoomKeyPicked = false;

// 계단실 열쇠
int itemStairsX = 12, itemStairsY = 6;
bool hasStairsKey = false, isStairsKeyPicked = false;

// 드라이버 (탈출구 Room 4에 위치)
int itemScrewdriverX = 2, itemScrewdriverY = 8;
bool hasScrewdriver = false, isScrewdriverPicked = false;

// 한정판 피규어 & 최종 탈출 열쇠
bool hasFigure = false, isFigurePicked = false, isFigurePlaced = false;
bool hasFinalKey = false, isFinalKeyPicked = false;
int finalKeyX = -10, finalKeyY = -10;

wchar_t messageLog[200] = L"주변을 수색하여 탈출할 단서를 찾으십시오.";
int  lastExitX = -10, lastExitY = -10;
bool bossDefeatedInRoom = false, isDoorUnlocked = false, isFirstRoom = true;

// ============================================================
//  몬스터 AI 길찾기 알고리즘 (BFS)
// ============================================================
void getMonsterNextStep(int map[][MAP_WIDTH], int mx, int my, int px, int py, int* nx, int* ny) {
    int dist[MAP_HEIGHT][MAP_WIDTH];
    for (int i = 0; i < MAP_HEIGHT; i++) {
        for (int j = 0; j < MAP_WIDTH; j++) {
            dist[i][j] = 9999;
        }
    }

    int qx[1000], qy[1000];
    int head = 0, tail = 0;

    qx[tail] = px; qy[tail] = py; tail++;
    dist[py][px] = 0;

    int dx[4] = { 0, 0, -1, 1 };
    int dy[4] = { -1, 1, 0, 0 };

    while (head < tail) {
        int cx = qx[head]; int cy = qy[head]; head++;
        for (int i = 0; i < 4; i++) {
            int nx_ = cx + dx[i];
            int ny_ = cy + dy[i];

            if (nx_ >= 0 && nx_ < MAP_WIDTH && ny_ >= 0 && ny_ < MAP_HEIGHT) {
                int t = map[ny_][nx_];
                if (t != TILE_WALL && !(t >= 3 && t <= 6) && t != 10 && t != TILE_EXIT && t != TILE_DESK && t != TILE_MONITOR && t != TILE_FIGURE && t != TILE_PURIFIER && t != TILE_BLACKBOARD) {
                    if (dist[ny_][nx_] == 9999) {
                        dist[ny_][nx_] = dist[cy][cx] + 1;
                        qx[tail] = nx_; qy[tail] = ny_; tail++;
                    }
                }
            }
        }
    }

    int minDist = 9999;
    *nx = mx; *ny = my;

    for (int i = 0; i < 4; i++) {
        int tx = mx + dx[i], ty = my + dy[i];
        if (tx >= 0 && tx < MAP_WIDTH && ty >= 0 && ty < MAP_HEIGHT) {
            int t = map[ty][tx];
            if (t != TILE_WALL && !(t >= 3 && t <= 6) && t != 10 && t != TILE_EXIT && t != TILE_DESK && t != TILE_MONITOR && t != TILE_FIGURE && t != TILE_PURIFIER && t != TILE_BLACKBOARD) {
                if (dist[ty][tx] < minDist) {
                    minDist = dist[ty][tx];
                    *nx = tx; *ny = ty;
                }
            }
        }
    }
}

// ============================================================
//  drawSingleTile — 버퍼에 타일 1개 출력
// ============================================================
void drawSingleTile(int room, int row, int col)
{
    if (row < 0 || row >= MAP_HEIGHT || col < 0 || col >= MAP_WIDTH) return;
    move_cursor_buf(col * 2, row + 5);
    int tile = world_maps[room][row][col];

    if (col >= room_limit_width[room] || row >= room_limit_height[room]) {
        print_buf(L"  ");
    }
    else if (room == itemRoom && !isExitRoomKeyPicked && row == itemY && col == itemX) {
        set_color_buf(COLOR_YELLOW);  print_buf(L"⚷ "); set_color_buf(COLOR_WHITE);
    }
    else if (room == 2 && !isStairsKeyPicked && row == itemStairsY && col == itemStairsX) {
        set_color_buf(COLOR_GREEN);  print_buf(L"⚷ "); set_color_buf(COLOR_WHITE);
    }
    else if (room == 4 && !isScrewdriverPicked && row == itemScrewdriverY && col == itemScrewdriverX) {
        set_color_buf(COLOR_DARKGRAY);  print_buf(L"┼ "); set_color_buf(COLOR_WHITE); // 드라이버
    }
    else if (room == 0 && !isFinalKeyPicked && finalKeyX != -10 && row == finalKeyY && col == finalKeyX) {
        set_color_buf(COLOR_RED);  print_buf(L"⚷ "); set_color_buf(COLOR_WHITE); // 최종 열쇠 (교수가 떨군 것)
    }
    else if ((tile >= 3 && tile <= 6) || tile == 10 || tile == TILE_EXIT) {
        set_color_buf(COLOR_BROWN);  print_buf(L"目 "); set_color_buf(COLOR_WHITE);
    }
    else if (tile == TILE_STAIRS) {
        set_color_buf(COLOR_PURPLE); print_buf(L"↕ "); set_color_buf(COLOR_WHITE);
    }
    else if (tile == TILE_DESK) {
        set_color_buf(COLOR_CYAN); print_buf(L"✉ "); set_color_buf(COLOR_WHITE);
    }
    else if (tile == TILE_MONITOR) {
        set_color_buf(COLOR_CYAN); print_buf(L"▣ "); set_color_buf(COLOR_WHITE);
    }
    else if (tile == TILE_FIGURE) {
        set_color_buf(COLOR_YELLOW); print_buf(L"♙ "); set_color_buf(COLOR_WHITE); // 피규어 액자
    }
    else if (tile == TILE_DROP_ZONE) {
        set_color_buf(COLOR_PURPLE); print_buf(L"◈ "); set_color_buf(COLOR_WHITE); // 피규어 두는 곳
    }
    else if (tile == TILE_PURIFIER) {
        set_color_buf(COLOR_BLUE); print_buf(L"♒ "); set_color_buf(COLOR_WHITE); // 정수기 (파란색으로 변경)
    }
    else if (tile == TILE_BLACKBOARD) {
        set_color_buf(COLOR_GREEN); print_buf(L"▒ "); set_color_buf(COLOR_WHITE); // 칠판
    }
    else if (tile == TILE_WALL) {
        set_color_buf(COLOR_DARKGRAY);

        int H = room_limit_height[room] - 1;
        int W = room_limit_width[room] - 1;

        bool isOuter = (row == 0 || row == H || col == 0 || col == W);

        if (isOuter) {
            if (row == 0 && col == 0) print_buf(L"╔═");
            else if (row == 0 && col == W) print_buf(L"╗ ");
            else if (row == H && col == 0) print_buf(L"╚═");
            else if (row == H && col == W) print_buf(L"╝ ");
            else if (row == 0 || row == H) print_buf(L"══");
            else                           print_buf(L"║ ");
        }
        else {
            print_buf(L"▓ ");
        }
        set_color_buf(COLOR_WHITE);
    }
    else if (tile == TILE_CLOSET) {
        set_color_buf(COLOR_GREEN);  print_buf(L"▩ "); set_color_buf(COLOR_WHITE);
    }
    else { print_buf(L"  "); }
}

// ============================================================
//  drawFullFrame
// ============================================================
void drawFullFrame(int room, int px, int py, int mx, int my,
    bool isHidden, bool bossActive,
    const wchar_t* roomName)
{
    clear_buffer();

    set_color_buf(COLOR_WHITE);
    move_cursor_buf(0, 0);
    print_buf(L"+-------------------------------------------------------------------------------+");
    move_cursor_buf(0, 1);
    print_buf(L"| ");
    set_color_buf(COLOR_CYAN);
    wchar_t tmp[80];
    swprintf(tmp, 80, L"%-26ls", roomName);
    print_buf(L"%ls", tmp);
    set_color_buf(COLOR_WHITE);
    print_buf(L" | ");
    set_color_buf(isHidden ? COLOR_GREEN : COLOR_RED);
    swprintf(tmp, 80, L"%-31ls", isHidden ? L"옷장에 숨음 (SAFE)" : L"추격당하는 중...");
    print_buf(L"%ls", tmp);
    set_color_buf(COLOR_WHITE);
    print_buf(L" |");
    move_cursor_buf(0, 2);
    print_buf(L"| ");

    // 인벤토리 동적 구성
    set_color_buf(COLOR_YELLOW);
    wchar_t invStr[200] = L"";
    if (hasExitRoomKey) wcscat(invStr, L"[탈출구 열쇠] ");
    if (hasStairsKey) wcscat(invStr, L"[계단실 열쇠] ");
    if (hasScrewdriver) wcscat(invStr, L"[드라이버] ");
    if (hasFigure) wcscat(invStr, L"[애니 피규어] ");
    if (hasFinalKey) wcscat(invStr, L"[최종 탈출 열쇠] ");
    if (wcslen(invStr) == 0) wcscpy(invStr, L"없음");

    swprintf(tmp, 80, L"%-68ls", invStr);
    print_buf(L"%ls", tmp);
    set_color_buf(COLOR_WHITE);
    print_buf(L" |");

    move_cursor_buf(0, 3);
    print_buf(L"+-------------------------------------------------------------------------------+");

    for (int i = 0; i < MAP_HEIGHT; i++)
        for (int j = 0; j < MAP_WIDTH; j++)
            drawSingleTile(room, i, j);

    if (!isHidden) {
        move_cursor_buf(px * 2, py + 5);
        set_color_buf(COLOR_BLUE); print_buf(L"P "); set_color_buf(COLOR_WHITE);
    }

    if (bossActive && mx != -10 && my != -10) {
        move_cursor_buf(mx * 2, my + 5);
        set_color_buf(COLOR_RED); print_buf(L"⊙_⊙ "); set_color_buf(COLOR_WHITE);
    }

    int logY = MAP_HEIGHT + 5;
    set_color_buf(COLOR_YELLOW);
    move_cursor_buf(0, logY);
    print_buf(L"=================================================================================");
    move_cursor_buf(0, logY + 1);
    set_color_buf(COLOR_WHITE);
    wchar_t padded[160];
    swprintf(padded, 160, L"[알림] %-70ls", messageLog);
    print_buf(L"%ls", padded);
    move_cursor_buf(0, logY + 2);
    set_color_buf(COLOR_YELLOW);
    print_buf(L"=================================================================================");
    set_color_buf(COLOR_WHITE);
}

// ============================================================
//  showDialog — 게임 화면을 백버퍼에 먼저 그린 뒤 팝업 덮기
// ============================================================
void showDialog_on_frame(int room, int px, int py, int mx, int my,
    bool isHidden, bool bossActive,
    const wchar_t* roomName,
    const wchar_t* speaker, const wchar_t* text)
{
    while (GetAsyncKeyState(VK_SPACE) & 0x8000) Sleep(10);

    drawFullFrame(room, px, py, mx, my, isHidden, bossActive, roomName);

    int startY = MAP_HEIGHT + 5;
    set_color_buf(COLOR_WHITE);
    move_cursor_buf(0, startY);
    print_buf(L"┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓");
    move_cursor_buf(0, startY + 1); print_buf(L"┃                                                                               ┃");
    move_cursor_buf(0, startY + 2); print_buf(L"┃                                                                               ┃");
    move_cursor_buf(0, startY + 3); print_buf(L"┃                                                                               ┃");
    move_cursor_buf(0, startY + 4);
    print_buf(L"┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛");
    move_cursor_buf(4, startY + 1); set_color_buf(COLOR_YELLOW); print_buf(L"▶ %ls", speaker);
    move_cursor_buf(4, startY + 2); set_color_buf(COLOR_WHITE);
    print_buf(L"%ls", text); // 공간 채우기 제거하고 텍스트만 출력
    move_cursor_buf(66, startY + 3); set_color_buf(COLOR_GREEN); print_buf(L"(Space) ▼");

    flip_buffer();

    while (!(GetAsyncKeyState(VK_SPACE) & 0x8000)) Sleep(30);
    while (GetAsyncKeyState(VK_SPACE) & 0x8000)  Sleep(10);
}

// ============================================================
//  initDoors / initWorldMaps
// ============================================================
void initDoors()
{
    doors[0].targetRoom = 1; doors[0].nextPlayerX = 10; doors[0].nextPlayerY = 10;
    doors[1].targetRoom = 2; doors[1].nextPlayerX = 12; doors[1].nextPlayerY = 12;
    doors[2].targetRoom = 3; doors[2].nextPlayerX = 10; doors[2].nextPlayerY = 2;
    doors[3].targetRoom = 4; doors[3].nextPlayerX = 10; doors[3].nextPlayerY = 2;
}

void initWorldMaps()
{
    int r, i, j;
    for (r = 0; r < NUM_ROOMS; r++)
        for (i = 0; i < MAP_HEIGHT; i++)
            for (j = 0; j < MAP_WIDTH; j++) {
                if (i == 0 || i == room_limit_height[r] - 1 || j == 0 || j == room_limit_width[r] - 1)
                    world_maps[r][i][j] = TILE_WALL;
                else if (j >= room_limit_width[r] || i >= room_limit_height[r])
                    world_maps[r][i][j] = TILE_WALL;
                else world_maps[r][i][j] = TILE_EMPTY;
            }

    world_maps[0][0][5] = 3; world_maps[0][0][20] = 4;
    world_maps[0][MAP_HEIGHT - 1][10] = 5; world_maps[0][MAP_HEIGHT - 1][30] = 6;

    // 1층 복도(Room 0) 중앙 피규어 두는 곳
    world_maps[0][10][20] = TILE_DROP_ZONE;

    for (i = 3; i <= 8; i++) for (j = 5; j <= 15; j++) world_maps[1][i][j] = TILE_WALL;
    world_maps[1][11][10] = 3; world_maps[1][1][2] = TILE_STAIRS;

    for (j = 4; j <= 20; j += 4) { world_maps[2][4][j] = TILE_WALL; world_maps[2][8][j] = TILE_WALL; }
    world_maps[2][13][12] = 4;

    for (j = 4; j <= 15; j++) world_maps[3][6][j] = TILE_WALL;
    world_maps[3][2][2] = TILE_CLOSET; world_maps[3][2][3] = TILE_CLOSET;
    world_maps[3][0][10] = 5;
    world_maps[3][4][15] = TILE_DESK;

    for (i = 4; i <= 8; i++) world_maps[4][i][8] = TILE_WALL;
    world_maps[4][5][19] = TILE_EXIT; world_maps[4][0][10] = 6;

    world_maps[5][0][5] = 3; world_maps[5][0][25] = 5;
    world_maps[5][MAP_HEIGHT - 1][5] = 4; world_maps[5][MAP_HEIGHT - 1][30] = 6;

    // 2층 복도(Room 5) 정수기 배치
    world_maps[5][2][20] = TILE_PURIFIER;

    world_maps[6][0][10] = 4; world_maps[6][5][19] = 10;

    // 비밀공간(Room 7) 위쪽 벽면에 교수님의 애니 피규어가 숨겨진 액자 배치
    world_maps[7][5][0] = 10;
    world_maps[7][1][8] = TILE_FIGURE;

    world_maps[8][0][10] = 6; world_maps[8][4][10] = TILE_DESK;

    for (j = 4; j <= 20; j += 4) { world_maps[9][4][j] = TILE_WALL; world_maps[9][8][j] = TILE_WALL; }
    world_maps[9][13][12] = 5;
    world_maps[9][2][10] = TILE_MONITOR;

    world_maps[10][11][10] = 3; world_maps[10][1][2] = TILE_STAIRS;

    // [수정] 2층 계단실(Room 10) 칠판을 위쪽 벽면으로 배치 (y좌표 1)
    world_maps[10][5][18] = TILE_BLACKBOARD;
}

// ============================================================
//  인트로 연출 함수들
// ============================================================
void draw_phone() {
    int x = 40, y = 3;
    move_cursor_buf(x, y); print_buf(L"┌─────────────────────────────────────────┐");
    for (int i = 1; i < 40; i++) { move_cursor_buf(x, y + i); print_buf(L"│                                         │"); }
    move_cursor_buf(x, y + 40); print_buf(L"└─────────────────────────────────────────┘");
}

void draw_lock_screen() {
    move_cursor_buf(56, 6);  print_buf(L"6월 22일 (월)");
    move_cursor_buf(47, 9);  print_buf(L" __   ___        ______ ______");
    move_cursor_buf(47, 10); print_buf(L"/_ | |__ \\   _  | ____| | ____|");
    move_cursor_buf(47, 11); print_buf(L" | |    ) |  (_) | |__   | |__");
    move_cursor_buf(47, 12); print_buf(L" | |  / /       |___ \\  |___ \\ ");
    move_cursor_buf(47, 13); print_buf(L" | | / /_    _   ___) |  ___) |");
    move_cursor_buf(47, 14); print_buf(L" |_| |____| (_) |____/  |____/");
}

void draw_notification(const wchar_t* name, const wchar_t* msg) {
    int x = 45, y = 32;
    for (int i = 0; i < 4; i++) { move_cursor_buf(x, y + i); print_buf(L"                                "); }
    move_cursor_buf(x, y);     print_buf(L"┌──────────────────────────────┐");
    move_cursor_buf(x, y + 1); print_buf(L"│ ♥ "); print_align_left(name, 24); print_buf(L"  │");
    move_cursor_buf(x, y + 2); print_buf(L"│ ");   print_align_left(msg, 29);  print_buf(L"│");
    move_cursor_buf(x, y + 3); print_buf(L"└──────────────────────────────┘");
}

void draw_art() {
    FILE* fp = fopen("art_title.txt", "rb");
    if (!fp) return;
    char utf8Buf[4096]; wchar_t wideBuf[4096]; int line = 0;
    while (fgets(utf8Buf, sizeof(utf8Buf), fp)) {
        MultiByteToWideChar(CP_UTF8, 0, utf8Buf, -1, wideBuf, 4096);
        move_cursor_buf(0, line);
        int x = 0; wchar_t* p = wideBuf;
        while (*p) {
            wchar_t ch = *p; set_color_buf(8);
            if (line >= 14 && line <= 24 && x >= 56 && x <= 121 && ch != L' ') set_color_buf(6);
            DWORD w; WriteConsoleW(hBuffer[screenIndex], &ch, 1, &w, NULL);
            p++; x++;
        }
        line++;
    }
    fclose(fp); set_color_buf(7);
}

void draw_lab_art() {
    FILE* fp = fopen("art_lab.txt", "rb");
    if (!fp) return;
    char utf8Buf[4096]; wchar_t wideBuf[4096]; int line = 0;
    while (fgets(utf8Buf, sizeof(utf8Buf), fp)) {
        MultiByteToWideChar(CP_UTF8, 0, utf8Buf, -1, wideBuf, 4096);
        move_cursor_buf(0, line);
        wchar_t* p = wideBuf;
        set_color_buf(COLOR_DARKGRAY);
        while (*p) {
            DWORD w;
            WriteConsoleW(hBuffer[screenIndex], p, 1, &w, NULL);
            p++;
        }
        line++;
    }
    fclose(fp);
    set_color_buf(COLOR_WHITE);
}
void draw_original_art() {
    FILE* fp = fopen("art_original.txt", "rb");
    if (!fp) return;
    char utf8Buf[4096]; wchar_t wideBuf[4096]; int line = 0;
    while (fgets(utf8Buf, sizeof(utf8Buf), fp)) {
        MultiByteToWideChar(CP_UTF8, 0, utf8Buf, -1, wideBuf, 4096);
        move_cursor_buf(0, line);
        wchar_t* p = wideBuf;
        set_color_buf(COLOR_RED); // 게임 오버 느낌을 위해 붉은색 지정
        while (*p) {
            DWORD w;
            WriteConsoleW(hBuffer[screenIndex], p, 1, &w, NULL);
            p++;
        }
        line++;
    }
    fclose(fp);
    set_color_buf(COLOR_WHITE);
}
void draw_ending_art() {
    FILE* fp = fopen("art_ending.txt", "rb");
    if (!fp) return;
    char utf8Buf[4096]; wchar_t wideBuf[4096]; int line = 0;
    while (fgets(utf8Buf, sizeof(utf8Buf), fp)) {
        MultiByteToWideChar(CP_UTF8, 0, utf8Buf, -1, wideBuf, 4096);
        move_cursor_buf(0, line);
        wchar_t* p = wideBuf;
        set_color_buf(COLOR_WHITE); // 게임 오버 느낌을 위해 붉은색 지정
        while (*p) {
            DWORD w;
            WriteConsoleW(hBuffer[screenIndex], p, 1, &w, NULL);
            p++;
        }
        line++;
    }
    fclose(fp);
    set_color_buf(COLOR_WHITE);
}
void draw_cut_art() {
    FILE* fp = fopen("art_cut.txt", "rb");
    if (!fp) return;
    char utf8Buf[4096]; wchar_t wideBuf[4096]; int line = 0;
    while (fgets(utf8Buf, sizeof(utf8Buf), fp)) {
        MultiByteToWideChar(CP_UTF8, 0, utf8Buf, -1, wideBuf, 4096);
        move_cursor_buf(0, line);
        wchar_t* p = wideBuf;
        set_color_buf(COLOR_DARKGRAY);
        while (*p) {
            DWORD w;
            WriteConsoleW(hBuffer[screenIndex], p, 1, &w, NULL);
            p++;
        }
        line++;
    }
    fclose(fp);
    set_color_buf(COLOR_WHITE);
}


void draw_rule_art() {
    FILE* fp = fopen("game_rule.txt", "rb");
    if (!fp) return;
    char utf8Buf[4096]; wchar_t wideBuf[4096]; int line = 0;
    while (fgets(utf8Buf, sizeof(utf8Buf), fp)) {
        MultiByteToWideChar(CP_UTF8, 0, utf8Buf, -1, wideBuf, 4096);
        move_cursor_buf(0, line);
        wchar_t* p = wideBuf;
        set_color_buf(COLOR_WHITE);
        while (*p) {
            DWORD w;
            WriteConsoleW(hBuffer[screenIndex], p, 1, &w, NULL);
            p++;
        }
        line++;
    }
    fclose(fp);
    set_color_buf(COLOR_WHITE);
}


void draw_menu() {
    if (menu == 1) set_color_buf(COLOR_YELLOW); else set_color_buf(COLOR_WHITE);
    move_cursor_buf(122, 23); print_buf(L"1. 게임 시작");
    if (menu == 2) set_color_buf(COLOR_YELLOW); else set_color_buf(COLOR_WHITE);
    move_cursor_buf(122, 27); print_buf(L"2. 게임 설명");
    if (menu == 3) set_color_buf(COLOR_YELLOW); else set_color_buf(COLOR_WHITE);
    move_cursor_buf(122, 31); print_buf(L"3. 제작자");
    if (menu == 4) set_color_buf(COLOR_YELLOW); else set_color_buf(COLOR_WHITE);
    move_cursor_buf(122, 35); print_buf(L"4. 종료");
    set_color_buf(7);
}

// ======== 타이틀 화면 ========
int RenderTitle() {
    clear_buffer();
    draw_art(); draw_menu(); flip_buffer();

    int ch = _getch();
    if (ch == 0 || ch == 224) {
        ch = _getch();
        if (ch == 72 && menu > 1) menu--;
        if (ch == 80 && menu < 4) menu++;
    }
    else if (ch == 32) {
        if (menu == 1) { flush_keyboard_buffer(); return 2; }
        if (menu == 2) { flush_keyboard_buffer(); return 3; }
        if (menu == 3) { flush_keyboard_buffer(); return 4; }
        if (menu == 4) isRunning = 0;
    }
    return 0;
}

// ============================================================
//  MainGame
// ============================================================
int MainGame()
{
    clear_both_buffers();
    draw_phone(); draw_lock_screen(); flip_buffer();
    Sleep(1000); Beep(1200, 200);

    clear_buffer(); draw_phone(); draw_lock_screen();
    draw_notification(L"여자친구", L"자기야 오늘 무슨 날인지 알지?"); flip_buffer();
    Sleep(2500); Beep(1000, 200);

    clear_buffer(); draw_phone(); draw_lock_screen();
    draw_notification(L"여자친구", L"자기야 오늘 무슨 날인지 알지?");
    draw_notification(L"여자친구", L"나 오늘 이쁘게 입고 갈게!!"); flip_buffer();
    Sleep(2500); Beep(700, 400);

    clear_buffer(); draw_phone(); draw_lock_screen();
    draw_notification(L"여자친구", L"나 오늘 이쁘게 입고 갈게!!");
    draw_notification(L"여자친구", L"터미널에 2시까지 와야해!!"); flip_buffer();
    Sleep(3000); Beep(1200, 200);

    clear_buffer(); draw_phone(); draw_lock_screen();
    draw_notification(L"여자친구", L"터미널에 2시까지 와야해!!");
    move_cursor_buf(100, 20); print_buf(L"아... 오늘.. 1주년 기념 여행가기로 했었지..."); flip_buffer();
    Sleep(3000); Beep(1200, 200);

    clear_buffer(); draw_phone(); draw_lock_screen();
    draw_notification(L"여자친구", L"터미널에 2시까지 와야해!!");
    move_cursor_buf(100, 20); print_buf(L"아... 오늘.. 1주년 기념 여행가기로 했었지...");
    move_cursor_buf(100, 22); print_buf(L"이은석 교수님 수업인데.. 흠..."); flip_buffer();
    Sleep(3000); Beep(1200, 200);

    clear_buffer(); draw_phone(); draw_lock_screen();
    draw_notification(L"여자친구", L"터미널에 2시까지 와야해!!");
    move_cursor_buf(100, 20); print_buf(L"아... 오늘.. 1주년 기념 여행가기로 했었지...");
    move_cursor_buf(100, 22); print_buf(L"이은석 교수님 수업인데.. 흠...");
    set_color_buf(COLOR_YELLOW);
    move_cursor_buf(100, 40); print_buf(L"[스페이스바 누르면 진행]"); flip_buffer();
    _getch();

    // ── ★ 실험실 아스키아트 컷씬 ──
    clear_both_buffers();
    draw_lab_art(); flip_buffer();
    Sleep(1500); Beep(800, 150);

    clear_buffer(); draw_lab_art();
    set_color_buf(COLOR_WHITE);
    move_cursor_buf(100, 20); print_buf(L"- 실습실 -");
    flip_buffer();
    Sleep(3000); Beep(1200, 200);

    clear_buffer(); draw_lab_art();
    set_color_buf(COLOR_WHITE);
    move_cursor_buf(100, 20); print_buf(L"- 실습실 -");
    move_cursor_buf(100, 23); print_buf(L"교수님 아직 안 오셨네...");
    flip_buffer();
    Sleep(3000); Beep(1200, 200);

    clear_buffer(); draw_lab_art();
    set_color_buf(COLOR_WHITE);
    move_cursor_buf(100, 20); print_buf(L"- 실습실 -");
    move_cursor_buf(100, 23); print_buf(L"교수님 아직 안 오셨네...");
    move_cursor_buf(100, 25); print_buf(L"빨리 챙겨서 나가야겠다.");
    flip_buffer();
    Sleep(3000); Beep(1200, 200);

    clear_buffer(); draw_lab_art();
    set_color_buf(COLOR_WHITE);
    move_cursor_buf(100, 20); print_buf(L"- 실습실 -");
    move_cursor_buf(100, 23); print_buf(L"교수님 아직 안 오셨네...");
    move_cursor_buf(100, 25); print_buf(L"빨리 챙겨서 나가야겠다.");
    set_color_buf(COLOR_YELLOW);
    move_cursor_buf(100, 40); print_buf(L"[스페이스바 누르면 진행]");
    set_color_buf(COLOR_WHITE);
    flip_buffer();
    _getch();


    // ─── 변수 초기화 ─────────────────────────────────────────
    initWorldMaps(); initDoors();
    isDoorUnlocked = false;
    hasStairsKey = false; isStairsKeyPicked = false;
    bool isStairsUnlocked = false;
    hasScrewdriver = false; isScrewdriverPicked = false;
    hasExitRoomKey = false; isExitRoomKeyPicked = false;
    bool isExitRoomUnlocked = false;

    hasFigure = false; isFigurePicked = false; isFigurePlaced = false;
    hasFinalKey = false; isFinalKeyPicked = false;
    finalKeyX = -10; finalKeyY = -10;

    isFirstRoom = true; bossDefeatedInRoom = false;
    lastExitX = -10; lastExitY = -10;
    wcscpy(messageLog, L"주변을 수색하여 탈출할 단서를 찾으십시오.");
    int currentRoom = 0, px = 5, py = 10, mx = -10, my = -10;
    bool bossActive = false; int bossFollowTimer = 60;
    bool isHidden = false, spacePressed = false, gameOver = false, gameClear = false;
    int monsterMoveTurn = 0;
    int nextX, nextY, i, j;

    const wchar_t* roomNames[NUM_ROOMS] = {
        L"1층 중앙 복도",        L"1층 계단실",         L"1층 강의실",
        L"학생 과방",            L"1층 교수실(탈출구)",  L"2층 복도",
        L"이은석 교수실",        L"[비밀공간]",         L"2층 창고",
        L"2층 강의실",           L"2층 계단실"
    };

    set_cursor_visible(false);
    static DWORD lastMoveTime = 0;

    while (!gameOver && !gameClear) {
        int (*currentMap)[MAP_WIDTH] = world_maps[currentRoom];

        drawFullFrame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom]);
        flip_buffer();

        // ── 게임오버 판정 ────────────────────────────────────
        if (bossActive && !isHidden && mx != -10)
            if (abs(px - mx) <= 1 && abs(py - my) <= 1) { gameOver = true; break; }

        // ── 스페이스바 상호작용 ──────────────────────────────
        if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
            if (!spacePressed) {

                // 2층 계단실(Room 10) 칠판 상호작용 (오른쪽 벽면 x=18, y=5)
                if (currentRoom == 10 && abs(px - 18) <= 1 && abs(py - 5) <= 1) {
                    showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                        L"시스템", L"칠판에 무언가 적혀있다.");
                    showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                        L"칠판", L"*0**");
                    showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                        L"나", L"두 번째 자리가 0이라는 뜻인가...?");
                    spacePressed = true; continue;
                }

                // 2층 복도(Room 5) 정수기 상호작용
                if (currentRoom == 5 && abs(px - 20) <= 1 && abs(py - 2) <= 1) {
                    showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                        L"나", L"목마른데 물이나 마셔야겠다.");
                    showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                        L"나", L"어 물컵안에 쪽지가 있잖아?");
                    showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                        L"쪽지", L"'3 == 0 true'");
                    showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                        L"나", L"이게 뭔뜻이지?");
                    spacePressed = true; continue;
                }

                // 1층 복도(Room 0) 최종 열쇠 획득
                if (currentRoom == 0 && !isFinalKeyPicked && finalKeyX != -10 && abs(px - finalKeyX) <= 1 && abs(py - finalKeyY) <= 1) {
                    hasFinalKey = true; isFinalKeyPicked = true;
                    showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                        L"시스템", L"교수가 떨어뜨린 [최종 탈출 열쇠]를 획득했습니다!");
                    showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                        L"나", L"좋아! 이제 탈출구 방(1층 교수실)으로 가서 밖으로 나가자!");
                    spacePressed = true; continue;
                }

                // 1층 복도(Room 0) 피규어 두는 곳 상호작용
                if (currentRoom == 0 && !isFigurePlaced && abs(px - 20) <= 1 && abs(py - 10) <= 1) {
                    if (currentMap[10][20] == TILE_DROP_ZONE) {
                        if (!hasFigure) {
                            showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                                L"나", L"여기에 무언가 시선을 끌 만한 것을 두면 좋을 것 같다.");
                        }
                        else {
                            isFigurePlaced = true;
                            hasFigure = false;
                            currentMap[10][20] = TILE_EMPTY; // 맵에서 피규어 두는 곳 완벽히 삭제

                            showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                                L"나", L"이곳에 [애니 피규어]를 내려놓았다.");

                            // 교수 컷신
                            bossActive = true;
                            mx = 20; my = 10;
                            drawFullFrame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom]);
                            flip_buffer();
                            Sleep(500);

                            showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                                L"이은석 교수", L"아, 아니...! 내 초특급 한정판 마법소녀 피규어가 왜 여기에!!!");

                            // ──────────────────────────────────────────────────────────
                            // 🔥 [추가된 부분] 컷신 아스키 아트 출력 연출
                            // ──────────────────────────────────────────────────────────
                            clear_both_buffers();
                            draw_cut_art();
                            set_color_buf(COLOR_YELLOW);
                            move_cursor_buf(70, 45); // 아트 높이에 따라 y좌표(45)는 적절히 조절해 주십시오.
                            print_buf(L"[스페이스바를 눌러 계속...]");
                            flip_buffer();

                            // 스페이스바를 누를 때까지 대기
                            while (!(GetAsyncKeyState(VK_SPACE) & 0x8000)) Sleep(30);
                            while (GetAsyncKeyState(VK_SPACE) & 0x8000) Sleep(10);
                            // ──────────────────────────────────────────────────────────

                            showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                                L"시스템", L"이은석 교수가 피규어를 지키기 위해 몸을 날리다 바닥에 무언가를 떨어뜨렸습니다.");
                            showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                                L"시스템", L"교수가 피규어를 소중하게 품에 안고 황급히 도망칩니다...");

                            bossActive = false; bossFollowTimer = -1; mx = -10; my = -10;
                            bossDefeatedInRoom = true;

                            // 열쇠 드롭
                            finalKeyX = 20; finalKeyY = 10;
                        }
                        spacePressed = true; continue;
                    }
                }

                // 탈출구(Room 4) 드라이버 획득
                if (currentRoom == 4 && !isScrewdriverPicked && abs(px - itemScrewdriverX) <= 1 && abs(py - itemScrewdriverY) <= 1) {
                    hasScrewdriver = true; isScrewdriverPicked = true;
                    showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                        L"시스템", L"구석에서 공구함에 떨어져 있던 [드라이버]를 발견하여 획득했습니다!");
                    spacePressed = true; continue;
                }

                // 비밀의방(Room 7) 숨겨진 피규어 상호작용 및 획득
                if (currentRoom == 7 && abs(px - 8) <= 1 && abs(py - 1) <= 1 && !isFigurePicked) {
                    if (!hasScrewdriver) {
                        showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                            L"나", L"벽에 수상할 정도로 낡은 [풍경화 액자]가 단단히 걸려있다.");
                        showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                            L"나", L"네 모서리가 나사로 고정되어 있어 [드라이버]가 없으면 뗄 수 없다...");
                    }
                    else {
                        hasFigure = true;
                        isFigurePicked = true;
                        currentMap[1][8] = TILE_WALL; // 벽에서 액자 제거

                        showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                            L"시스템", L"[드라이버]를 사용해 나사를 모두 풀고 액자를 떼어냈습니다.");
                        showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                            L"나", L"액자 뒤에... 숨겨진 금고가 있어! 안에는...");
                        showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                            L"시스템", L"금고 안에서 [한정판 애니 피규어]를 획득했습니다!");
                        showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                            L"나", L"교수님의 은밀한 취향이... 좋아, 이 피규어로 교수를 유인해야겠어.");
                    }
                    spacePressed = true; continue;
                }

                // 비밀의방(Room 7) 탈출구 열쇠 획득
                if (currentRoom == itemRoom && !isExitRoomKeyPicked &&
                    abs(px - itemX) <= 1 && abs(py - itemY) <= 1) {
                    hasExitRoomKey = true; isExitRoomKeyPicked = true;
                    showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                        L"시스템", L"바닥에서 반짝이는 열쇠를 발견했습니다.");
                    showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                        L"시스템", L"[1층 탈출구 방 열쇠]를 획득했습니다!");
                    spacePressed = true; continue;
                }

                // 2층 강의실(Room 9) 모니터 상호작용
                if (currentRoom == 9 && abs(px - 10) <= 1 && abs(py - 2) <= 1) {
                    int startY = MAP_HEIGHT + 5;
                    while (GetAsyncKeyState(VK_SPACE) & 0x8000) Sleep(10);

                    drawFullFrame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom]);
                    set_color_buf(COLOR_WHITE);
                    move_cursor_buf(0, startY);
                    print_buf(L"┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓");
                    move_cursor_buf(0, startY + 1); print_buf(L"┃                                                                               ┃");
                    move_cursor_buf(0, startY + 2); print_buf(L"┃                                                                               ┃");
                    move_cursor_buf(0, startY + 3); print_buf(L"┃                                                                               ┃");
                    move_cursor_buf(0, startY + 4); print_buf(L"┃                                                                               ┃");
                    move_cursor_buf(0, startY + 5);
                    print_buf(L"┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛");
                    move_cursor_buf(4, startY + 1); set_color_buf(COLOR_YELLOW); print_buf(L"▶ 켜져 있는 모니터 (▣ )");
                    move_cursor_buf(4, startY + 2); set_color_buf(COLOR_WHITE);  print_buf(L"암호를 입력하세요: ");
                    move_cursor_buf(4, startY + 3); set_color_buf(COLOR_DARKGRAY);  print_buf(L"-암호 입력후 enter-");
                    move_cursor_buf(4, startY + 4); set_color_buf(COLOR_CYAN);  print_buf(L"-힌트: 교수님-");

                    set_color_buf(COLOR_WHITE);

                    CONSOLE_CURSOR_INFO ci2 = { 1, TRUE };
                    SetConsoleCursorInfo(hBuffer[screenIndex], &ci2);
                    flip_buffer();

                    COORD inputPos = { (SHORT)28, (SHORT)(startY + 2) };
                    SetConsoleCursorPosition(hBuffer[!screenIndex], inputPos);

                    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
                    FlushConsoleInputBuffer(hIn);

                    wchar_t inputBuf[32] = { 0 }; DWORD inputRead = 0;
                    ReadConsoleW(hIn, inputBuf, 20, &inputRead, NULL);

                    for (int k = 0; k < (int)inputRead; k++)
                        if (inputBuf[k] == L'\r' || inputBuf[k] == L'\n') { inputBuf[k] = 0; break; }

                    ci2.bVisible = FALSE;
                    SetConsoleCursorInfo(hBuffer[0], &ci2);
                    SetConsoleCursorInfo(hBuffer[1], &ci2);

                    if (wcscmp(inputBuf, L"이빨빠진은석") == 0 || wcscmp(inputBuf, L"3") == 0) {
                        showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                            L"모니터", L"[접근 허가]");
                        showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                            L"모니터", L"[족보]에 접속합니다");
                        showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                            L"모니터", L"(로딩중.....)");
                        showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                            L"모니터", L"비밀번호 마지막자리는 ...4");
                        showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                            L"나", L"마지막 자리가 4라... 잊지 말고 기억해두자.");
                    }
                    else {
                        showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                            L"모니터", L"[경고] 비밀번호가 틀렸습니다.");
                    }
                    spacePressed = true; continue;
                }

                if (currentRoom == 3 && abs(px - 15) <= 1 && abs(py - 4) <= 1) {
                    showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                        L"나", L"낡은 책상 위에 [어느 대학원생의 일지]가 놓여있다.");
                    showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                        L"낡은 일지", L"'우연히 교수님의 은밀한 취향이 담긴 물건을 봐버렸다.'");
                    showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                        L"낡은 일지", L"'그 피규어는 세상에 공개되어서는 안 되는 한정판이야...'");
                    showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                        L"낡은 일지", L"'교수님이 나를 부르신다..'");
                    showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                        L"낡은 일지", L"'교수실에 비밀공간이 있다 비밀번호는 1.....'");
                    showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                        L"나", L"글씨가 다급하게 휘갈겨져 있어 뒷부분은 알아볼 수 없다.");
                    showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                        L"나", L"비밀번호의 시작이 '1'이군...");

                    spacePressed = true; continue;
                }

                if (currentRoom == 8 && abs(px - 10) <= 1 && abs(py - 4) <= 1) {
                    showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                        L"나", L"책상 서랍에서 쪽지를 발견했다.");
                    showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                        L"쪽지", L"'이은석에서 이를 빼면?'");
                    showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                        L"쪽지", L"1. 은석,   2. -2은석");
                    showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                        L"쪽지", L"'정답은??'");
                    showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                        L"쪽지", L"'3. 이빨빠진은석'");
                    showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                        L"나", L"'이게 무슨 헛소리야...'");

                    spacePressed = true; continue;
                }

                if (currentRoom == 2 && !isStairsKeyPicked &&
                    abs(px - itemStairsX) <= 1 && abs(py - itemStairsY) <= 1) {
                    hasStairsKey = true; isStairsKeyPicked = true;
                    showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                        L"시스템", L"강의실 책상 위에서 [계단실 열쇠]를 획득했습니다!");
                    spacePressed = true; continue;
                }

                // [수정된 부분] 옷장 상호작용
                if (currentMap[py][px] == TILE_CLOSET) {
                    isHidden = !isHidden;

                    if (isHidden) {
                        wcscpy(messageLog, L"옷장 속에 숨었습니다. 숨소리를 죽이십시오...");
                        // 만약 교수가 추격 중(bossActive)일 때 숨었다면 즉시 이벤트 발생
                        if (bossActive) {
                            // 숨은 상태의 UI를 화면에 먼저 한 번 그려줍니다.
                            drawFullFrame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom]);
                            flip_buffer();

                            // 형님 요청대로 슬립(1.5초) 대기
                            Sleep(1500);

                            // 교수 대사 팝업
                            showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                                L"이은석 교수", L"이자식 어디간거야?");
                            showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                                L"이은석 교수", L"잡히면 가만두지 않을테다!!!");

                            // 교수 퇴장 및 상태 초기화
                            bossActive = false; bossFollowTimer = -1; mx = -10; my = -10;
                            bossDefeatedInRoom = true;
                            wcscpy(messageLog, L"이은석 교수가 포기하고 돌아갔습니다. 안전합니다.");
                        }
                    }
                    else {
                        wcscpy(messageLog, L"옷장에서 나왔습니다.");
                    }
                    spacePressed = true; continue;
                }

                if (currentRoom == 1 && abs(px - 2) + abs(py - 1) <= 1) {
                    currentRoom = 10; px = 2; py = 2; lastExitX = 2; lastExitY = 1;
                    bossActive = false; bossFollowTimer = 20; bossDefeatedInRoom = false;
                    wcscpy(messageLog, L"2층 계단실로 올라갔습니다.");
                    spacePressed = true; continue;
                }
                if (currentRoom == 10 && abs(px - 2) + abs(py - 1) <= 1) {
                    currentRoom = 1; px = 2; py = 2; lastExitX = 2; lastExitY = 1;
                    bossActive = false; bossFollowTimer = 20; bossDefeatedInRoom = false;
                    wcscpy(messageLog, L"1층 계단실로 내려왔습니다.");
                    spacePressed = true; continue;
                }

                int dx4[4] = { 0,0,-1,1 }, dy4[4] = { -1,1,0,0 };
                int targetDoorTile = 0;
                for (int d = 0; d < 4; d++) {
                    int cx = px + dx4[d], cy = py + dy4[d];
                    if (cx >= 0 && cx < MAP_WIDTH && cy >= 0 && cy < MAP_HEIGHT) {
                        int t = currentMap[cy][cx];
                        if ((t >= 3 && t <= 6) || t == 10) { targetDoorTile = t; break; }
                    }
                }

                if (currentRoom == 6 && targetDoorTile == 10) {
                    if (isDoorUnlocked) {
                        showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                            L"시스템", L"이미 잠금 해제된 비밀 통로를 통과합니다.");
                        px = 1; py = 5; currentRoom = 7;
                        lastExitX = -10; lastExitY = -10;
                        bossActive = false; bossFollowTimer = -1; mx = -10; my = -10;
                        bossDefeatedInRoom = true;
                        spacePressed = true; continue;
                    }

                    int startY = MAP_HEIGHT + 5;
                    while (GetAsyncKeyState(VK_SPACE) & 0x8000) Sleep(10);

                    drawFullFrame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom]);
                    set_color_buf(COLOR_WHITE);
                    move_cursor_buf(0, startY);
                    print_buf(L"┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓");
                    move_cursor_buf(0, startY + 1); print_buf(L"┃                                                                               ┃");
                    move_cursor_buf(0, startY + 2); print_buf(L"┃                                                                               ┃");
                    move_cursor_buf(0, startY + 3); print_buf(L"┃                                                                               ┃");
                    move_cursor_buf(0, startY + 4);
                    print_buf(L"┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛");
                    move_cursor_buf(4, startY + 1); set_color_buf(COLOR_YELLOW); print_buf(L"▶ 도어락");
                    move_cursor_buf(4, startY + 2); set_color_buf(COLOR_WHITE);  print_buf(L"비밀번호 4자리를 입력하세요: ");
                    move_cursor_buf(4, startY + 3); set_color_buf(COLOR_DARKGRAY);  print_buf(L"-비밀번호 입력후 enter-");
                    set_color_buf(COLOR_WHITE);
                    CONSOLE_CURSOR_INFO ci2 = { 1, TRUE };
                    SetConsoleCursorInfo(hBuffer[screenIndex], &ci2);
                    flip_buffer();

                    COORD inputPos = { (SHORT)34, (SHORT)(startY + 2) };
                    SetConsoleCursorPosition(hBuffer[!screenIndex], inputPos);

                    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
                    FlushConsoleInputBuffer(hIn);

                    wchar_t inputBuf[16] = { 0 }; DWORD inputRead = 0;
                    ReadConsoleW(hIn, inputBuf, 10, &inputRead, NULL);

                    for (int k = 0; k < (int)inputRead; k++)
                        if (inputBuf[k] == L'\r' || inputBuf[k] == L'\n') { inputBuf[k] = 0; break; }
                    int inputPw = _wtoi(inputBuf);

                    ci2.bVisible = FALSE;
                    SetConsoleCursorInfo(hBuffer[0], &ci2);
                    SetConsoleCursorInfo(hBuffer[1], &ci2);

                    // TODO: 나중에 비밀번호 변경 필요 (현재 1004)
                    if (inputPw == 1004) {
                        showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                            L"시스템", L"철컥! 비밀번호가 일치하여 비밀 장치 문이 열렸습니다.");
                        px = 1; py = 5; currentRoom = 7;
                        lastExitX = -10; lastExitY = -10;
                        bossActive = false; bossFollowTimer = -1; mx = -10; my = -10;
                        bossDefeatedInRoom = true; isDoorUnlocked = true;
                    }
                    else {
                        showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                            L"시스템", L"[경고] 비밀번호가 틀렸습니다!");
                        showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                            L"나", L"이런, 소리가 너무 컸어... 교수가 이쪽으로 오고 있다!");
                        bossActive = true; mx = 10; my = 5;
                        monsterMoveTurn = -10; bossDefeatedInRoom = false;
                    }
                    spacePressed = true; continue;
                }

                if (currentRoom == 0 && targetDoorTile == 4) {
                    int sY = MAP_HEIGHT + 5;
                    while (GetAsyncKeyState(VK_SPACE) & 0x8000) Sleep(10);

                    drawFullFrame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom]);
                    set_color_buf(COLOR_WHITE);
                    move_cursor_buf(0, sY);
                    print_buf(L"┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓");
                    move_cursor_buf(0, sY + 1); print_buf(L"┃                                                                               ┃");
                    move_cursor_buf(0, sY + 2); print_buf(L"┃                                                                               ┃");
                    move_cursor_buf(0, sY + 3); print_buf(L"┃                                                                               ┃");
                    move_cursor_buf(0, sY + 4); print_buf(L"┃                                                                               ┃");
                    move_cursor_buf(0, sY + 5); print_buf(L"┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛");

                    move_cursor_buf(4, sY + 1); set_color_buf(COLOR_YELLOW); print_buf(L"▶ 문이 잠겨있다: i++ 의 의미는?");
                    move_cursor_buf(4, sY + 2); set_color_buf(COLOR_WHITE);  print_buf(L"1. 1만큼 증가");
                    move_cursor_buf(4, sY + 3); print_buf(L"2. i만큼 증가   3. 1만큼 곱하기");
                    move_cursor_buf(55, sY + 4); set_color_buf(COLOR_GREEN); print_buf(L"(숫자 1~3 입력)");
                    flip_buffer();

                    flush_keyboard_buffer();
                    int answer = 0;

                    while (1) {
                        int ch = _getch();
                        if (ch >= '1' && ch <= '3') {
                            answer = ch - '0';
                            break;
                        }
                    }

                    if (answer == 1) {
                        showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                            L"시스템", L"정답입니다! 1층 강의실 문이 열립니다.");
                    }
                    else {
                        showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                            L"시스템", L"오답입니다. 문이 굳게 닫혀 열리지 않습니다.");
                        spacePressed = true;
                        continue;
                    }
                }

                if (currentRoom == 0 && targetDoorTile == 3) {
                    if (!isStairsUnlocked) {
                        if (!hasStairsKey) {
                            showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                                L"시스템", L"[!] 1층 계단실 문이 굳게 잠겨있습니다. [계단실 열쇠]가 필요합니다.");
                            spacePressed = true; continue;
                        }
                        else {
                            int selection = 0; bool mActive = true, mSp = false;
                            int sY = MAP_HEIGHT + 5;
                            while (GetAsyncKeyState(VK_SPACE) & 0x8000) Sleep(10);
                            while (mActive) {
                                drawFullFrame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom]);
                                set_color_buf(COLOR_WHITE);
                                move_cursor_buf(0, sY);
                                print_buf(L"┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓");
                                move_cursor_buf(0, sY + 1); print_buf(L"┃                                                                               ┃");
                                move_cursor_buf(0, sY + 2); print_buf(L"┃                                                                               ┃");
                                move_cursor_buf(0, sY + 3); print_buf(L"┃                                                                               ┃");
                                move_cursor_buf(0, sY + 4);
                                print_buf(L"┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛");
                                move_cursor_buf(4, sY + 1); set_color_buf(COLOR_YELLOW); print_buf(L"▶ 계단실 문");
                                move_cursor_buf(4, sY + 2); set_color_buf(COLOR_WHITE);  print_buf(L"[계단실 열쇠]를 사용하여 문을 여시겠습니까?");
                                move_cursor_buf(6, sY + 3);
                                if (selection == 0) print_buf(L"▶ 예        아니오");
                                else                print_buf(L"   예        ▶ 아니오");
                                flip_buffer();

                                if (GetAsyncKeyState(VK_LEFT) & 0x8000) selection = 0;
                                if (GetAsyncKeyState(VK_RIGHT) & 0x8000) selection = 1;
                                if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
                                    if (!mSp) { mActive = false; mSp = true; }
                                }
                                else { mSp = false; }
                                Sleep(80);
                            }

                            if (selection == 0) {
                                isStairsUnlocked = true;
                                showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                                    L"시스템", L"철컥! [계단실 열쇠]를 사용하여 문을 열었습니다.");
                            }
                            else {
                                spacePressed = true; continue;
                            }
                        }
                    }
                }

                // 탈출구 방(Room 4) 입장 로직 변경 (열쇠 필요)
                if (currentRoom == 0 && targetDoorTile == 6) {
                    if (!isExitRoomUnlocked) {
                        if (!hasExitRoomKey) {
                            showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                                L"시스템", L"[!] 1층 탈출구 방 문이 굳게 잠겨있습니다. [탈출구 열쇠]가 필요합니다.");
                            spacePressed = true; continue;
                        }
                        else {
                            int selection = 0; bool mActive = true, mSp = false;
                            int sY = MAP_HEIGHT + 5;
                            while (GetAsyncKeyState(VK_SPACE) & 0x8000) Sleep(10);
                            while (mActive) {
                                drawFullFrame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom]);
                                set_color_buf(COLOR_WHITE);
                                move_cursor_buf(0, sY);
                                print_buf(L"┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓");
                                move_cursor_buf(0, sY + 1); print_buf(L"┃                                                                               ┃");
                                move_cursor_buf(0, sY + 2); print_buf(L"┃                                                                               ┃");
                                move_cursor_buf(0, sY + 3); print_buf(L"┃                                                                               ┃");
                                move_cursor_buf(0, sY + 4);
                                print_buf(L"┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛");
                                move_cursor_buf(4, sY + 1); set_color_buf(COLOR_YELLOW); print_buf(L"▶ 탈출구 방 문");
                                move_cursor_buf(4, sY + 2); set_color_buf(COLOR_WHITE);  print_buf(L"[탈출구 열쇠]를 사용하여 문을 여시겠습니까?");
                                move_cursor_buf(6, sY + 3);
                                if (selection == 0) print_buf(L"▶ 예        아니오");
                                else                print_buf(L"   예        ▶ 아니오");
                                flip_buffer();

                                if (GetAsyncKeyState(VK_LEFT) & 0x8000) selection = 0;
                                if (GetAsyncKeyState(VK_RIGHT) & 0x8000) selection = 1;
                                if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
                                    if (!mSp) { mActive = false; mSp = true; }
                                }
                                else { mSp = false; }
                                Sleep(80);
                            }

                            if (selection == 0) {
                                isExitRoomUnlocked = true;
                                showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                                    L"시스템", L"철컥! [탈출구 열쇠]를 사용하여 문을 열었습니다.");
                            }
                            else {
                                spacePressed = true; continue;
                            }
                        }
                    }
                }

                if ((targetDoorTile >= 3 && targetDoorTile <= 6) || targetDoorTile == 10) {
                    bossActive = false; bossFollowTimer = 25;
                    bossDefeatedInRoom = false; isFirstRoom = false;
                    if (currentRoom == 0) {
                        int idx = targetDoorTile - 3;
                        px = doors[idx].nextPlayerX; py = doors[idx].nextPlayerY;
                        currentRoom = doors[idx].targetRoom;
                        if (currentRoom == 1) { lastExitX = 10; lastExitY = 11; }
                        if (currentRoom == 2) { lastExitX = 12; lastExitY = 13; }
                        if (currentRoom == 3) { lastExitX = 10; lastExitY = 0; }
                        if (currentRoom == 4) { lastExitX = 10; lastExitY = 0; }
                    }
                    else if (currentRoom == 5) {
                        if (targetDoorTile == 3) { px = 10; py = 10; currentRoom = 10; lastExitX = 10; lastExitY = 11; }
                        if (targetDoorTile == 5) { px = 12; py = 12; currentRoom = 9;  lastExitX = 12; lastExitY = 13; }
                        if (targetDoorTile == 4) { px = 10; py = 1;  currentRoom = 6;  lastExitX = 10; lastExitY = 0; }
                        if (targetDoorTile == 6) { px = 10; py = 1;  currentRoom = 8;  lastExitX = 10; lastExitY = 0; }
                    }
                    else if (currentRoom == 7 && targetDoorTile == 10) {
                        px = 18; py = 5; currentRoom = 6; lastExitX = 19; lastExitY = 5;
                    }
                    else {
                        int prev = currentRoom;
                        if (prev == 1) { px = 5;  py = 1;            currentRoom = 0; lastExitX = 5;  lastExitY = 0; }
                        if (prev == 2) { px = 20; py = 1;            currentRoom = 0; lastExitX = 20; lastExitY = 0; }
                        if (prev == 3) { px = 10; py = MAP_HEIGHT - 2; currentRoom = 0; lastExitX = 10; lastExitY = MAP_HEIGHT - 1; }
                        if (prev == 4) { px = 30; py = MAP_HEIGHT - 2; currentRoom = 0; lastExitX = 30; lastExitY = MAP_HEIGHT - 1; }
                        if (prev == 10) { px = 5;  py = 1;            currentRoom = 5; lastExitX = 5;  lastExitY = 0; }
                        if (prev == 9) { px = 25; py = 1;            currentRoom = 5; lastExitX = 25; lastExitY = 0; }
                        if (prev == 6) { px = 5;  py = MAP_HEIGHT - 2; currentRoom = 5; lastExitX = 5;  lastExitY = MAP_HEIGHT - 1; }
                        if (prev == 8) { px = 30; py = MAP_HEIGHT - 2; currentRoom = 5; lastExitX = 30; lastExitY = MAP_HEIGHT - 1; }
                    }
                    spacePressed = true; continue;
                }

                bool nearExit = false;
                for (int d = 0; d < 4; d++) {
                    int cy = py + dy4[d], cx = px + dx4[d];
                    if (cy >= 0 && cy < MAP_HEIGHT && cx >= 0 && cx < MAP_WIDTH)
                        if (currentMap[cy][cx] == TILE_EXIT) nearExit = true;
                }

                // 탈출구 방(Room 4) 최종 탈출 해금!
                if (currentRoom == 4 && nearExit) {
                    if (!hasFinalKey) {
                        showDialog_on_frame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom],
                            L"시스템", L"[!] 최종 탈출을 위한 [최종 탈출 열쇠]가 필요합니다.");
                    }
                    else {
                        int selection = 0; bool mActive = true, mSp = false;
                        int sY = MAP_HEIGHT + 5;
                        while (GetAsyncKeyState(VK_SPACE) & 0x8000) Sleep(10);
                        while (mActive) {
                            drawFullFrame(currentRoom, px, py, mx, my, isHidden, bossActive, roomNames[currentRoom]);
                            set_color_buf(COLOR_WHITE);
                            move_cursor_buf(0, sY);
                            print_buf(L"┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓");
                            move_cursor_buf(0, sY + 1); print_buf(L"┃                                                                               ┃");
                            move_cursor_buf(0, sY + 2); print_buf(L"┃                                                                               ┃");
                            move_cursor_buf(0, sY + 3); print_buf(L"┃                                                                               ┃");
                            move_cursor_buf(0, sY + 4);
                            print_buf(L"┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛");
                            move_cursor_buf(4, sY + 1); set_color_buf(COLOR_YELLOW); print_buf(L"▶ 탈출구");
                            move_cursor_buf(4, sY + 2); set_color_buf(COLOR_WHITE);  print_buf(L"모든 것을 끝내고 학교 밖으로 탈출하시겠습니까?");
                            move_cursor_buf(6, sY + 3);
                            if (selection == 0) print_buf(L"▶ 예        아니오");
                            else                print_buf(L"   예        ▶ 아니오");
                            flip_buffer();
                            if (GetAsyncKeyState(VK_LEFT) & 0x8000) selection = 0;
                            if (GetAsyncKeyState(VK_RIGHT) & 0x8000) selection = 1;
                            if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
                                if (!mSp) { if (selection == 0) gameClear = true; mActive = false; mSp = true; }
                            }
                            else { mSp = false; }
                            Sleep(80);
                        }
                        if (gameClear) break;
                    }
                }

                spacePressed = true;
            }
        }
        else { spacePressed = false; }

        // 옷장에 숨어있지 않을 때(!isHidden)만 방향키 입력을 받도록 수정
        if (!isHidden && GetTickCount() - lastMoveTime >= 90) {
            nextX = px; nextY = py; bool moved = false;
            if (GetAsyncKeyState(VK_UP) & 0x8000) { nextY--; moved = true; }
            else if (GetAsyncKeyState(VK_DOWN) & 0x8000) { nextY++; moved = true; }
            else if (GetAsyncKeyState(VK_LEFT) & 0x8000) { nextX--; moved = true; }
            else if (GetAsyncKeyState(VK_RIGHT) & 0x8000) { nextX++; moved = true; }

            if (moved && (nextX != px || nextY != py)) {
                if (nextX >= 0 && nextX < MAP_WIDTH && nextY >= 0 && nextY < MAP_HEIGHT) {
                    int nt = currentMap[nextY][nextX];
                    // 갈 수 있는 길인지 검사
                    if (nt != TILE_WALL && !(nt >= 3 && nt <= 6) && nt != 10 && nt != TILE_EXIT && nt != TILE_DESK && nt != TILE_MONITOR && nt != TILE_FIGURE && nt != TILE_PURIFIER && nt != TILE_BLACKBOARD)
                    {
                        px = nextX; py = nextY; lastMoveTime = GetTickCount();
                    }
                }
            }
        }

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { gameOver = true; break; }

        if (currentRoom != 7 && !bossActive && !bossDefeatedInRoom && bossFollowTimer > 0) {
            if (!(currentRoom == 0 && isFirstRoom)) {
                bossFollowTimer--;
                if (bossFollowTimer == 0) { bossActive = true; mx = lastExitX; my = lastExitY; }
            }
        }

        if (currentRoom != 7 && bossActive && mx != -10 && my != -10) {
            monsterMoveTurn++;
            if (monsterMoveTurn >= 8) {
                if (isHidden && abs(mx - px) <= 1 && abs(my - py) <= 1) {
                    bossActive = false; bossFollowTimer = -1; mx = -10; my = -10;
                    bossDefeatedInRoom = true;
                    wcscpy(messageLog, L"이은석 교수가 물러갔습니다. 안전합니다.");
                }
                else if (!isHidden) {
                    int nextMx, nextMy;
                    getMonsterNextStep(currentMap, mx, my, px, py, &nextMx, &nextMy);
                    mx = nextMx; my = nextMy;
                }
                monsterMoveTurn = 0;
            }
        }
        Sleep(30);
    }

    clear_both_buffers();

    if (gameClear) {
        // 1. 엔딩 아트 출력 후 정적 (여운의 시작)
        clear_buffer();
        draw_ending_art();
        flip_buffer();
        Sleep(2000); // 2초간 묵직하게 대기

        // 2. 첫 번째 대사 출력
        clear_buffer();
        draw_ending_art();
        move_cursor_buf(70, 25);
        set_color_buf(COLOR_WHITE);
        print_buf(L"무사히 탈출했다...");
        flip_buffer();
        Sleep(1500); // 1.5초 대기

        // 3. 두 번째 대사 출력
        clear_buffer();
        draw_ending_art();
        move_cursor_buf(70, 25);
        set_color_buf(COLOR_WHITE);
        print_buf(L"무사히 탈출했다...");

        move_cursor_buf(70, 27);
        print_buf(L"여친과 즐거운 시간...");
        flip_buffer();
        Sleep(1500); // 1.5초 대기

        // 4. END 로고 출력
        clear_buffer();
        draw_ending_art();
        move_cursor_buf(70, 25);
        set_color_buf(COLOR_WHITE);
        print_buf(L"무사히 탈출했다...");

        move_cursor_buf(70, 27);
        print_buf(L"여친과 즐거운 시간...");

        move_cursor_buf(75, 31); // 대사 아래쪽으로 Y좌표 수정
        set_color_buf(COLOR_YELLOW);
        print_buf(L"[ E N D ]");
        flip_buffer();
        Sleep(4000); // 엔딩 쾅 찍고 2초 대기

        // 5. 마지막 복귀 안내 문구 출력
        clear_buffer();
        draw_ending_art();
        move_cursor_buf(70, 25);
        set_color_buf(COLOR_WHITE);
        print_buf(L"무사히 탈출했다...");

        move_cursor_buf(70, 27);
        print_buf(L"여친과 즐거운 시간...");

        move_cursor_buf(75, 31);
        set_color_buf(COLOR_YELLOW);
        print_buf(L"[ E N D ]");


        flip_buffer();
    }
    else {
        draw_original_art(); // 교수가 잡았을 때 아스키 아트 출력

        // 아스키 아트 아래쪽에 텍스트가 뜨도록 좌표 조정
        move_cursor_buf(75, 45); set_color_buf(COLOR_RED);
        print_buf(L"[ GAME OVER ]");
        move_cursor_buf(70, 47); set_color_buf(COLOR_WHITE);
        print_buf(L"이은석 교수에게 잡혔습니다...");
        move_cursor_buf(65, 50); set_color_buf(COLOR_DARKGRAY);
        print_buf(L"아무 키나 누르면 메인 화면으로 돌아갑니다.");
    }

    flip_buffer();

    flush_keyboard_buffer();
    _getch();

    menu = 1;
    return 0;
}
// ============================================================
//  GameEX / Team / start_game / main
// ============================================================
int GameEX() {
    clear_both_buffers();
    draw_rule_art();
    set_color_buf(COLOR_YELLOW);
    move_cursor_buf(25, 30); print_buf(L"아무 키나 누르면 타이틀로 돌아갑니다.");
    set_color_buf(COLOR_WHITE);
    flip_buffer();

    flush_keyboard_buffer();
    _getch();

    return 0;
}


int Team() {
    clear_both_buffers();
    move_cursor_buf(52, 8);  print_buf(L"=================== 제작팀 소개 ===================");
    move_cursor_buf(52, 10); print_buf(L"  조건우  - 조장 / PROJECT 설계 총괄 및 일정 제어");
    move_cursor_buf(52, 12); print_buf(L"   이경빈  - 조원 / PPT 제작 및 발표 대본 작성");
    move_cursor_buf(52, 14); print_buf(L"   정나라  - 조원 / ASCII ART 제작 및 UI 디자인");
    move_cursor_buf(52, 18); set_color_buf(COLOR_YELLOW); print_buf(L"아무 키나 누르면 타이틀로 돌아갑니다.");
    set_color_buf(COLOR_WHITE); flip_buffer();

    flush_keyboard_buffer();
    _getch();

    return 0;
}

int start_game() {
    clear_both_buffers();
    move_cursor_buf(40, 10);
    print_buf(L"Alt + Enter 를 눌러 전체화면으로 플레이하세요.");
    flip_buffer(); Sleep(3000); return 0;
}

int main()
{
    _setmode(_fileno(stdout), _O_U16TEXT);
    system("mode con cols=170 lines=60");
    system("chcp 65001");
    system("color 00");
    init_double_buffer();
    start_game();
    int gameStatus = 0;
    while (isRunning) {
        switch (gameStatus) {
        case 0: gameStatus = RenderTitle(); break;
        case 2: gameStatus = MainGame();    break;
        case 3: gameStatus = GameEX();      break;
        case 4: gameStatus = Team();        break;
        default: gameStatus = 0;            break;
        }
        if (!isRunning) break;
    }
    return 0;
}