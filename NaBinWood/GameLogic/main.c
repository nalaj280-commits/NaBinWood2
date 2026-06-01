#define _CRT_SECURE_NO_WARNINGS // 인코딩 안 틀어지게 안전장치 추가
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <stdbool.h>
#include <math.h>
#include <string.h> 

#define MAP_WIDTH 40
#define MAP_HEIGHT 20
#define NUM_ROOMS 11  

// 타일 값 정의
#define TILE_EMPTY 0
#define TILE_WALL 1
#define TILE_DESK 2     
#define TILE_EXIT 7
#define TILE_STAIRS 8  
#define TILE_CLOSET 15  

// 콘솔 색상 코드 정의
#define COLOR_BLUE      9
#define COLOR_GREEN     10
#define COLOR_RED       12
#define COLOR_BROWN     6
#define COLOR_DARKGRAY  8
#define COLOR_WHITE     15
#define COLOR_PURPLE    13 
#define COLOR_YELLOW    14

void gotoxy(int x, int y) {
    COORD pos;
    pos.X = x;
    pos.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

void setColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

// 문 및 이동 정보를 담는 구조체
typedef struct {
    int targetRoom;
    int nextPlayerX;
    int nextPlayerY;
} DoorInfo;

int world_maps[NUM_ROOMS][MAP_HEIGHT][MAP_WIDTH];
DoorInfo doors[4];

int room_limit_width[NUM_ROOMS] = { 40, 20, 24, 20, 20,   40, 20, 16, 20, 24, 20 };
int room_limit_height[NUM_ROOMS] = { 20, 12, 14, 10, 10,   20, 12,  8, 10, 14, 12 };

// 아이템 및 암호 시스템 관련 변수
int itemRoom = 2;
int itemX = 18; int itemY = 5;
bool hasKey = false;
bool isItemPicked = false;

// 대사 출력을 위한 전역 로그 문자열
char messageLog[100] = "주변을 수색하여 탈출할 단서를 찾으십시오, 형님.";

// 몬스터가 등장할 문의 좌표
int lastExitX = -10; int lastExitY = -10;

// 한 방에서 보스를 이미 따돌렸는지 체크하는 플래그 (무한 리스폰 방지)
bool bossDefeatedInRoom = false;

// [신설] 비밀번호 문이 한 번 열렸는지 기억하는 플래그 (프리패스 기능)
bool isDoorUnlocked = false;

// 대사창 출력 함수
void printMessageLog() {
    gotoxy(0, MAP_HEIGHT + 4);
    setColor(COLOR_YELLOW);
    printf("=================================================================================\n");
    printf("[알림] %-70s\n", messageLog);
    printf("=================================================================================\n");
    setColor(COLOR_WHITE);
}

void initDoors() {
    doors[0].targetRoom = 1; doors[0].nextPlayerX = 10; doors[0].nextPlayerY = 10;
    doors[1].targetRoom = 2; doors[1].nextPlayerX = 12; doors[1].nextPlayerY = 12;
    doors[2].targetRoom = 3; doors[2].nextPlayerX = 10; doors[2].nextPlayerY = 2;
    doors[3].targetRoom = 4; doors[3].nextPlayerX = 10; doors[3].nextPlayerY = 2;
}

void initWorldMaps() {
    int r, i, j;
    for (r = 0; r < NUM_ROOMS; r++) {
        for (i = 0; i < MAP_HEIGHT; i++) {
            for (j = 0; j < MAP_WIDTH; j++) {
                if (i == 0 || i == room_limit_height[r] - 1 || j == 0 || j == room_limit_width[r] - 1) world_maps[r][i][j] = TILE_WALL;
                else if (j >= room_limit_width[r] || i >= room_limit_height[r]) world_maps[r][i][j] = TILE_WALL;
                else world_maps[r][i][j] = TILE_EMPTY;
            }
        }
    }

    // 1층 배정
    world_maps[0][0][5] = 3;   world_maps[0][0][20] = 4;
    world_maps[0][MAP_HEIGHT - 1][10] = 5;   world_maps[0][MAP_HEIGHT - 1][30] = 6;

    for (i = 3; i <= 8; i++) { for (j = 5; j <= 15; j++) world_maps[1][i][j] = TILE_WALL; }
    world_maps[1][11][10] = 3; world_maps[1][1][2] = TILE_STAIRS;

    for (j = 4; j <= 20; j += 4) { world_maps[2][4][j] = TILE_WALL; world_maps[2][8][j] = TILE_WALL; }
    world_maps[2][13][12] = 4;

    for (j = 4; j <= 15; j++) world_maps[3][6][j] = TILE_WALL;
    world_maps[3][2][2] = TILE_CLOSET; world_maps[3][2][3] = TILE_CLOSET; world_maps[3][0][10] = 5;

    for (i = 4; i <= 8; i++) world_maps[4][i][8] = TILE_WALL;
    world_maps[4][5][19] = TILE_EXIT; world_maps[4][0][10] = 6;

    // 2층 배정
    world_maps[5][0][5] = 3; world_maps[5][0][25] = 5;
    world_maps[5][MAP_HEIGHT - 1][5] = 4; world_maps[5][MAP_HEIGHT - 1][30] = 6;

    world_maps[10][11][10] = 3; world_maps[10][1][2] = TILE_STAIRS;

    for (j = 4; j <= 20; j += 4) { world_maps[9][4][j] = TILE_WALL; world_maps[9][8][j] = TILE_WALL; }
    world_maps[9][13][12] = 5;

    world_maps[6][0][10] = 4; world_maps[6][5][19] = 10;

    world_maps[7][5][0] = 10;

    world_maps[8][0][10] = 6;
    world_maps[8][4][10] = TILE_DESK;
}

int main() {
    int currentRoom = 0;
    int px = 5, py = 10; int mx = -10, my = -10;

    bool bossActive = false; int bossFollowTimer = 60;
    bool isHidden = false; bool spacePressed = false;
    bool gameOver = false; bool gameClear = false;

    int playerMoveTurn = 0; int monsterMoveTurn = 0; int monsterSubTurn = 0;
    int i, j, nextX, nextY;

    char* roomNames[NUM_ROOMS];
    CONSOLE_CURSOR_INFO cursorInfo;

    roomNames[0] = "1층 중앙 복도"; roomNames[1] = "1층 계단실"; roomNames[2] = "1층 강의실";
    roomNames[3] = "학생 과방"; roomNames[4] = "1층 교수실 (탈출구)"; roomNames[5] = "2층 아래층 복도";
    roomNames[6] = "이은석 교수실"; roomNames[7] = "★ 비밀공간 ★"; roomNames[8] = "2층 창고";
    roomNames[9] = "2층 강의실"; roomNames[10] = "2층 계단실";

    initWorldMaps(); initDoors();

    cursorInfo.bVisible = FALSE; cursorInfo.dwSize = 1;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    system("cls");

    while (!gameOver && !gameClear) {
        int (*currentMap)[MAP_WIDTH] = world_maps[currentRoom];

        // 1. 화면 렌더링
        gotoxy(0, 0);
        setColor(COLOR_WHITE);
        printf("+-------------------------------------------------------------------------------+\n");
        printf("| 방 정보: %-26s | 상태: %-31s |\n", roomNames[currentRoom], isHidden ? "옷장에 숨음 (SAFE)" : "추격당하는 중...");
        printf("| 소지품: %-68s |\n", hasKey ? "[교수실 열쇠]" : "없음");
        printf("+-------------------------------------------------------------------------------+\n");

        for (i = 0; i < MAP_HEIGHT; i++) {
            for (j = 0; j < MAP_WIDTH; j++) {
                if (j >= room_limit_width[currentRoom] || i >= room_limit_height[currentRoom]) { printf("  "); }
                else if (i == py && j == px && !isHidden) { setColor(COLOR_BLUE); printf("P "); }
                else if (i == my && j == mx && bossActive) { setColor(COLOR_RED); printf("M "); }
                else if (currentRoom == itemRoom && !isItemPicked && i == itemY && j == itemX) { setColor(COLOR_GREEN); printf("* "); }
                else if ((currentMap[i][j] >= 3 && currentMap[i][j] <= 6) || currentMap[i][j] == 10 || currentMap[i][j] == TILE_EXIT) { setColor(COLOR_BROWN); printf("D "); }
                else if (currentMap[i][j] == TILE_STAIRS) { setColor(COLOR_PURPLE); printf("S "); }
                else if (currentMap[i][j] == TILE_DESK) { setColor(COLOR_YELLOW); printf("T "); }
                else if (currentMap[i][j] == TILE_WALL) {
                    setColor(COLOR_DARKGRAY);
                    if (i == 0 || i == room_limit_height[currentRoom] - 1 || j == 0 || j == room_limit_width[currentRoom] - 1) printf("# ");
                    else printf("X ");
                }
                else if (currentMap[i][j] == TILE_CLOSET) { setColor(COLOR_GREEN); printf("H "); }
                else { printf("  "); }
            }
            printf("\n");
        }

        printMessageLog();

        if (bossActive && !isHidden && px == mx && py == my) { gameOver = true; break; }

        // 2. 스페이스바 상호작용
        if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
            if (!spacePressed) {
                if (currentRoom == 8 && abs(px - 10) <= 1 && abs(py - 4) <= 1) {
                    strcpy_s(messageLog, sizeof(messageLog), "책상 서랍에서 노트를 발견했다: '비밀번호는 [1111]이다.' (넘어가려면 Space)");
                    printMessageLog();
                    while (GetAsyncKeyState(VK_SPACE) & 0x8000) { Sleep(10); }
                    while (!(GetAsyncKeyState(VK_SPACE) & 0x8000)) { Sleep(30); }
                    strcpy_s(messageLog, sizeof(messageLog), "비밀번호 단서를 확인했습니다.");
                    spacePressed = true; system("cls"); continue;
                }

                if (currentRoom == itemRoom && !isItemPicked && abs(px - itemX) <= 1 && abs(py - itemY) <= 1) {
                    hasKey = true; isItemPicked = true;
                    strcpy_s(messageLog, sizeof(messageLog), "강의실 바닥에서 [교수실 마스터 열쇠]를 획득했습니다!");
                    spacePressed = true; continue;
                }

                if (currentMap[py][px] == TILE_CLOSET) {
                    isHidden = !isHidden;
                    if (isHidden) {
                        strcpy_s(messageLog, sizeof(messageLog), "옷장 속에 숨었습니다. 숨소리를 죽이십시오...");
                    }
                    else {
                        strcpy_s(messageLog, sizeof(messageLog), "옷장에서 나왔습니다.");
                    }
                }

                // 방 이동(계단) 시 소멸 플래그 리셋
                if (currentRoom == 1 && abs(px - 2) + abs(py - 1) <= 1) {
                    currentRoom = 10; px = 2; py = 2; lastExitX = 2; lastExitY = 1;
                    bossActive = false; bossFollowTimer = 20; bossDefeatedInRoom = false;
                    strcpy_s(messageLog, sizeof(messageLog), "2층 계단실로 내려왔습니다.");
                    system("cls"); spacePressed = true; continue;
                }
                if (currentRoom == 10 && abs(px - 2) + abs(py - 1) <= 1) {
                    currentRoom = 1; px = 2; py = 2; lastExitX = 2; lastExitY = 1;
                    bossActive = false; bossFollowTimer = 20; bossDefeatedInRoom = false;
                    strcpy_s(messageLog, sizeof(messageLog), "1층 계단실로 올라왔습니다.");
                    system("cls"); spacePressed = true; continue;
                }

                int dx[4] = { 0, 0, -1, 1 }; int dy[4] = { -1, 1, 0, 0 };
                int targetDoorTile = 0;
                int currentDoorX = px, currentDoorY = py;

                for (int d = 0; d < 4; d++) {
                    int checkX = px + dx[d]; int checkY = py + dy[d];
                    if (checkX >= 0 && checkX < MAP_WIDTH && checkY >= 0 && checkY < MAP_HEIGHT) {
                        int tile = currentMap[checkY][checkX];
                        if ((tile >= 3 && tile <= 6) || tile == 10) {
                            targetDoorTile = tile;
                            currentDoorX = checkX; currentDoorY = checkY;
                            break;
                        }
                    }
                }

                // [수정 기믹] 이은석 교수실 비밀번호 문 상호작용
                if (currentRoom == 6 && targetDoorTile == 10) {
                    // 이미 비밀번호를 한 번 성공했다면 입력 없이 다이렉트 통과!
                    if (isDoorUnlocked) {
                        px = 1; py = 5; currentRoom = 7; lastExitX = -10; lastExitY = -10;
                        bossActive = false; bossFollowTimer = -1; mx = -10; my = -10; bossDefeatedInRoom = true;
                        strcpy_s(messageLog, sizeof(messageLog), "이미 잠금 해제된 비밀 통로를 통과합니다.");
                        system("cls"); spacePressed = true; continue;
                    }

                    cursorInfo.bVisible = TRUE; SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);

                    gotoxy(0, MAP_HEIGHT + 7);
                    printf("▶ 비밀번호 4자리를 입력하세요: ");
                    int inputPassword = 0;

                    if (scanf_s("%d", &inputPassword) == 1) {
                        if (inputPassword == 1111) {
                            px = 1; py = 5; currentRoom = 7; lastExitX = -10; lastExitY = -10;
                            bossActive = false; bossFollowTimer = -1; mx = -10; my = -10; bossDefeatedInRoom = true;
                            isDoorUnlocked = true; // [해제] 다음부터는 입력 안 받게 고정!
                            strcpy_s(messageLog, sizeof(messageLog), "철컥! 비밀번호가 일치하여 비밀 장치 문이 열렸습니다. (이제 그냥 다닐 수 있습니다.)");
                        }
                        else {
                            strcpy_s(messageLog, sizeof(messageLog), "삐빅! 경고: 비밀번호가 틀렸습니다! (아오오니가 문에서 추격을 시작합니다!)");
                            bossActive = true; bossFollowTimer = 0; mx = currentDoorX; my = currentDoorY; bossDefeatedInRoom = false;
                        }
                    }

                    cursorInfo.bVisible = FALSE; SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
                    system("cls"); spacePressed = true; continue;
                }

                if ((targetDoorTile >= 3 && targetDoorTile <= 6) || targetDoorTile == 10) {
                    bossActive = false;
                    bossFollowTimer = 25;
                    bossDefeatedInRoom = false;

                    if (currentRoom == 0) {
                        int doorIdx = targetDoorTile - 3;
                        px = doors[doorIdx].nextPlayerX; py = doors[doorIdx].nextPlayerY;
                        currentRoom = doors[doorIdx].targetRoom;

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
                        int prevRoom = currentRoom;
                        if (prevRoom == 1) { px = 5;  py = 1; currentRoom = 0; lastExitX = 5; lastExitY = 0; }
                        if (prevRoom == 2) { px = 20; py = 1; currentRoom = 0; lastExitX = 20; lastExitY = 0; }
                        if (prevRoom == 3) { px = 10; py = MAP_HEIGHT - 2; currentRoom = 0; lastExitX = 10; lastExitY = MAP_HEIGHT - 1; }
                        if (prevRoom == 4) { px = 30; py = MAP_HEIGHT - 2; currentRoom = 0; lastExitX = 30; lastExitY = MAP_HEIGHT - 1; }
                        if (prevRoom == 10) { px = 5;  py = 1; currentRoom = 5; lastExitX = 5; lastExitY = 0; }
                        if (prevRoom == 9) { px = 25; py = 1; currentRoom = 5; lastExitX = 25; lastExitY = 0; }
                        if (prevRoom == 6) { px = 5;  py = MAP_HEIGHT - 2; currentRoom = 5; lastExitX = 5; lastExitY = MAP_HEIGHT - 1; }
                        if (prevRoom == 8) { px = 30; py = MAP_HEIGHT - 2; currentRoom = 5; lastExitX = 30; lastExitY = MAP_HEIGHT - 1; }
                    }
                    system("cls"); playerMoveTurn = 0; spacePressed = true; continue;
                }

                int nearExit = false;
                for (int d = 0; d < 4; d++) {
                    if (py + dy[d] >= 0 && py + dy[d] < MAP_HEIGHT && px + dx[d] >= 0 && px + dx[d] < MAP_WIDTH) {
                        if (currentMap[py + dy[d]][px + dx[d]] == TILE_EXIT) nearExit = true;
                    }
                }
                if (currentRoom == 4 && nearExit) {
                    system("cls");
                    if (!hasKey) {
                        gotoxy(5, MAP_HEIGHT / 2); printf("[!] 교수실 비밀 통로가 잠겨있습니다. 마스터 열쇠가 필요합니다.");
                        gotoxy(5, (MAP_HEIGHT / 2) + 2); printf("돌아가려면 스페이스바를 누르십시오...");
                        while (GetAsyncKeyState(VK_SPACE) & 0x8000) { Sleep(10); }
                        while (!(GetAsyncKeyState(VK_SPACE) & 0x8000)) { Sleep(30); }
                        system("cls");
                    }
                    else {
                        int selection = 0; bool menuActive = true;
                        while (GetAsyncKeyState(VK_SPACE) & 0x8000) { Sleep(10); }
                        bool menuSpacePressed = false;
                        while (menuActive) {
                            gotoxy(5, MAP_HEIGHT / 2); printf("★ 열쇠를 사용해 학교 건물 밖으로 탈출하시겠습니까? ★");
                            gotoxy(7, (MAP_HEIGHT / 2) + 3);
                            if (selection == 0) printf("▶ 예      아니오"); else printf("   예   ▶ 아니오");
                            if (GetAsyncKeyState(VK_LEFT) & 0x8000)  selection = 0;
                            if (GetAsyncKeyState(VK_RIGHT) & 0x8000) selection = 1;
                            if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
                                if (!menuSpacePressed) { if (selection == 0) gameClear = true; menuActive = false; menuSpacePressed = true; }
                            }
                            else { menuSpacePressed = false; }
                            Sleep(100);
                        }
                        system("cls"); if (gameClear) break;
                    }
                }
                spacePressed = true;
            }
        }
        else { spacePressed = false; }

        // 3. 플레이어 이동
        if (!isHidden) {
            playerMoveTurn++;
            if (playerMoveTurn >= 2) {
                nextX = px; nextY = py;
                if (GetAsyncKeyState(VK_UP) & 0x8000)    nextY--;
                if (GetAsyncKeyState(VK_DOWN) & 0x8000)  nextY++;
                if (GetAsyncKeyState(VK_LEFT) & 0x8000)  nextX--;
                if (GetAsyncKeyState(VK_RIGHT) & 0x8000) nextX++;

                if (nextX >= 0 && nextX < MAP_WIDTH && nextY >= 0 && nextY < MAP_HEIGHT) {
                    int nextTile = currentMap[nextY][nextX];
                    if (nextTile != TILE_WALL && !(nextTile >= 3 && nextTile <= 6) && nextTile != 10 && nextTile != TILE_EXIT && nextTile != TILE_DESK) {
                        px = nextX; py = nextY;
                    }
                }
                playerMoveTurn = 0;
            }
        }

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { gameOver = true; break; }

        // 4. 추격자 스폰 타이머 조정
        if (currentRoom != 0 && currentRoom != 7 && !bossActive && !bossDefeatedInRoom && bossFollowTimer > 0) {
            bossFollowTimer--;
            if (bossFollowTimer == 0) {
                bossActive = true;
                mx = lastExitX; my = lastExitY;
            }
        }

        // 5. 아오오니 AI 추격
        if (currentRoom != 7 && bossActive && mx != -10 && my != -10) {
            monsterMoveTurn++;
            if (monsterMoveTurn >= 4) {
                monsterSubTurn++;
                if (monsterSubTurn % 5 != 0) {
                    int targetX = mx; int targetY = my;
                    if (mx < px) targetX++; else if (mx > px) targetX--;
                    if (my < py) targetY++; else if (my > py) targetY--;

                    if (isHidden && abs(mx - px) <= 1 && abs(my - py) <= 1) {
                        bossActive = false;
                        bossFollowTimer = -1;   
                        mx = -10; my = -10;
                        bossDefeatedInRoom = true;
                        strcpy_s(messageLog, sizeof(messageLog), "아오오니가 문 밖으로 완전히 물러갔습니다. 안전합니다.");
                    }
                    if (mx != -10 && my != -10) {
                        int monsterNextTile = currentMap[targetY][targetX];
                        if (monsterNextTile != TILE_WALL && !(monsterNextTile >= 3 && monsterNextTile <= 6) && monsterNextTile != 10 && monsterNextTile != TILE_EXIT && monsterNextTile != TILE_DESK) {
                            mx = targetX; my = targetY;
                        }
                    }
                }
                monsterMoveTurn = 0;
            }
        }
        Sleep(30);
    }

    system("cls");
    setColor(COLOR_WHITE);
    if (gameClear) {
        printf("\n\n\n\n\t🎉 [ STAGE CLEAR ] 🎉\n\t단서를 찾아 비밀번호를 풀고 대탈출에 완벽히 성공하셨습니다, 형님!\n\n\n");
    }
    else {
        printf("\n\n\n\n\t[ GAME OVER ]\n\t아오오니에게 잡혔습니다...\n\n\n");
    }
    return 0;
}