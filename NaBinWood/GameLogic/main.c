#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <stdbool.h>
#include <math.h>

#define MAP_WIDTH 40
#define MAP_HEIGHT 20
#define NUM_ROOMS 5

// 타일 값 정의
#define TILE_EMPTY 0
#define TILE_WALL 1
#define TILE_CLOSET 6
#define TILE_EXIT 7

void gotoxy(int x, int y) {
    COORD pos;
    pos.X = x;
    pos.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

// 문 정보를 담는 구조체
typedef struct {
    int targetRoom;
    int nextPlayerX;
    int nextPlayerY;
    int nextMonsterX;
    int nextMonsterY;
} DoorInfo;

int world_maps[NUM_ROOMS][MAP_HEIGHT][MAP_WIDTH];
DoorInfo doors[4];

// 방별 실제 활성화될 가로/세로 한계 크기 정의 (검은색 칠하기 필터용)
int room_limit_width[5] = { 40, 20, 24, 20, 20 };
int room_limit_height[5] = { 20, 12, 14, 10, 10 };

int itemRoom = 2;
int itemX = 18;
int itemY = 5;
bool hasKey = false;
bool isItemPicked = false;

void initDoors() {
    // [타일 2] 복도 ↔ 계단실(Room 1)
    doors[0].targetRoom = 1;
    doors[0].nextPlayerX = 10;          doors[0].nextPlayerY = 10;
    doors[0].nextMonsterX = 10;         doors[0].nextMonsterY = 9;

    // [타일 3] 복도 ↔ 강의실(Room 2)
    doors[1].targetRoom = 2;
    doors[1].nextPlayerX = 12;          doors[1].nextPlayerY = 12;
    doors[1].nextMonsterX = 12;         doors[1].nextMonsterY = 11;

    // [타일 4] 복도 ↔ 과방(Room 3)
    doors[2].targetRoom = 3;
    doors[2].nextPlayerX = 10;          doors[2].nextPlayerY = 2;
    doors[2].nextMonsterX = 10;         doors[2].nextMonsterY = 3;

    // [타일 5] 복도 ↔ 교수실(Room 4)
    doors[3].targetRoom = 4;
    doors[3].nextPlayerX = 10;          doors[3].nextPlayerY = 2;
    doors[3].nextMonsterX = 10;         doors[3].nextMonsterY = 3;
}

void initWorldMaps() {
    int r, i, j;

    // 1. 도화지 초기화
    for (r = 0; r < NUM_ROOMS; r++) {
        for (i = 0; i < MAP_HEIGHT; i++) {
            for (j = 0; j < MAP_WIDTH; j++) {
                if (i == 0 || i == room_limit_height[r] - 1 || j == 0 || j == room_limit_width[r] - 1) {
                    world_maps[r][i][j] = TILE_WALL;
                }
                else if (j >= room_limit_width[r] || i >= room_limit_height[r]) {
                    world_maps[r][i][j] = TILE_WALL;
                }
                else {
                    world_maps[r][i][j] = TILE_EMPTY;
                }
            }
        }
    }

    // --- [Room 0: 중앙 긴 복도 (40x20)] ---
    world_maps[0][1][5] = 2;
    world_maps[0][1][20] = 3;
    world_maps[0][MAP_HEIGHT - 2][10] = 4;
    world_maps[0][MAP_HEIGHT - 2][30] = 5;

    // --- [Room 1: 계단실 (20x12)] ---
    for (i = 3; i <= 8; i++) { for (j = 5; j <= 15; j++) world_maps[1][i][j] = TILE_WALL; }
    world_maps[1][11][10] = 2;

    // --- [Room 2: 강의실 (24x14)] ---
    for (j = 4; j <= 20; j += 4) { world_maps[2][4][j] = TILE_WALL; world_maps[2][8][j] = TILE_WALL; }
    world_maps[2][13][12] = 3;

    // --- [Room 3: 학생 과방 (20x10)] ---
    for (j = 4; j <= 15; j++) world_maps[3][6][j] = TILE_WALL;
    world_maps[3][2][2] = TILE_CLOSET; world_maps[3][2][3] = TILE_CLOSET;
    world_maps[3][1][10] = 4;

    // --- [Room 4: 교수실 (20x10)] ---
    for (i = 4; i <= 8; i++) world_maps[4][i][8] = TILE_WALL;
    world_maps[4][5][18] = TILE_EXIT;
    world_maps[4][1][10] = 5;
}

int main() {
    int currentRoom = 0;
    int prevRoom = 0;
    int px = 5, py = 10;

    int mx = -10, my = -10;

    bool bossActive = false;
    int bossFollowTimer = 60;
    int bossReappearTimer = 0;
    bool bossWaitingAfterHide = false;

    bool isHidden = false;
    bool spacePressed = false;
    bool gameOver = false;
    bool gameClear = false;

    int playerMoveTurn = 0;
    int monsterMoveTurn = 0;
    int monsterSubTurn = 0;

    int i, j;
    int nextX, nextY;
    int tileValue;

    char* roomNames[5];
    CONSOLE_CURSOR_INFO cursorInfo;

    roomNames[0] = "중앙 복도 (Hallway)";
    roomNames[1] = "계단실 (Stairs)";
    roomNames[2] = "강의실 (Lecture Room)";
    roomNames[3] = "학생 과방 (Student Lounge)";
    roomNames[4] = "교수실 (Professor's Office)";

    initWorldMaps();
    initDoors();

    cursorInfo.bVisible = FALSE;
    cursorInfo.dwSize = 1;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);

    system("cls");

    while (!gameOver && !gameClear) {
        int (*currentMap)[MAP_WIDTH] = world_maps[currentRoom];

        // 1. 화면 렌더링
        gotoxy(0, 0);
        printf("+-------------------------------------------------------------------------------+\n");
        printf("| 카테고리: %-26s | 상태: %-31s |\n", roomNames[currentRoom], isHidden ? "옷장에 숨음 (SAFE)" : "추격당하는 중...");
        printf("| 소지품: %-68s |\n", hasKey ? "[교수실 열쇠]" : "없음");
        printf("+-------------------------------------------------------------------------------+\n");

        for (i = 0; i < MAP_HEIGHT; i++) {
            for (j = 0; j < MAP_WIDTH; j++) {
                if (j >= room_limit_width[currentRoom] || i >= room_limit_height[currentRoom]) {
                    printf("  ");
                }
                else if (i == py && j == px && !isHidden) {
                    printf("P ");
                }
                else if (i == my && j == mx && bossActive) {
                    printf("M ");
                }
                else if (currentRoom == itemRoom && !isItemPicked && i == itemY && j == itemX) {
                    printf("* ");
                }
                else if ((currentMap[i][j] >= 2 && currentMap[i][j] <= 5) || currentMap[i][j] == TILE_EXIT) {
                    printf("D ");
                }
                else if (currentMap[i][j] == TILE_WALL) {
                    if (i == 0 || i == room_limit_height[currentRoom] - 1 || j == 0 || j == room_limit_width[currentRoom] - 1) {
                        printf("# ");
                    }
                    else {
                        printf("X ");
                    }
                }
                else if (currentMap[i][j] == TILE_CLOSET) {
                    printf("H ");
                }
                else {
                    printf("  ");
                }
            }
            printf("\n");
        }

        // 2. 피격 판정
        if (bossActive && !isHidden && px == mx && py == my) {
            gameOver = true;
            break;
        }

        // 3. 스페이스바 상호작용
        if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
            if (!spacePressed) {
                if (currentRoom == itemRoom && !isItemPicked) {
                    if (abs(px - itemX) <= 1 && abs(py - itemY) <= 1) {
                        hasKey = true;
                        isItemPicked = true;
                        gotoxy(0, MAP_HEIGHT + 4);
                        printf("[!] 강의실 바닥에서 교수실 마스터 열쇠를 획득했습니다! \n");
                        Sleep(800);
                    }
                }

                if (currentMap[py][px] == TILE_CLOSET) {
                    isHidden = !isHidden;
                }

                int currentTile = currentMap[py][px];
                if (currentTile >= 2 && currentTile <= 5) {
                    int doorIdx = currentTile - 2;

                    if (bossWaitingAfterHide) {
                        bossReappearTimer = 25; bossFollowTimer = 0;
                        bossActive = false; bossWaitingAfterHide = false;
                    }
                    else {
                        bossFollowTimer = 25; bossActive = false;
                    }

                    if (currentRoom == 0) {
                        px = doors[doorIdx].nextPlayerX;
                        py = doors[doorIdx].nextPlayerY;
                        currentRoom = doors[doorIdx].targetRoom;
                    }
                    else {
                        if (currentRoom == 1) { px = 5;  py = 2; }
                        if (currentRoom == 2) { px = 20; py = 2; }
                        if (currentRoom == 3) { px = 10; py = MAP_HEIGHT - 3; }
                        if (currentRoom == 4) { px = 30; py = MAP_HEIGHT - 3; }

                        prevRoom = currentRoom;
                        currentRoom = 0;
                    }

                    system("cls");
                    playerMoveTurn = 0;
                    spacePressed = true;
                    continue;
                }
                spacePressed = true;
            }
        }
        else {
            spacePressed = false;
        }

        // 4. 플레이어 이동 (2턴에 1칸 이동)
        if (!isHidden) {
            playerMoveTurn++;
            if (playerMoveTurn >= 2) {
                nextX = px; nextY = py;

                if (GetAsyncKeyState(VK_UP) & 0x8000)    nextY--;
                if (GetAsyncKeyState(VK_DOWN) & 0x8000)  nextY++;
                if (GetAsyncKeyState(VK_LEFT) & 0x8000)  nextX--;
                if (GetAsyncKeyState(VK_RIGHT) & 0x8000) nextX++;

                if (nextX >= 0 && nextX < MAP_WIDTH && nextY >= 0 && nextY < MAP_HEIGHT) {
                    if (currentMap[nextY][nextX] != TILE_WALL) {
                        tileValue = currentMap[nextY][nextX];

                        if (currentRoom == 4 && tileValue == TILE_EXIT) {
                            system("cls");
                            if (!hasKey) {
                                gotoxy(5, MAP_HEIGHT / 2);
                                printf("[!] 교수실 비밀 통로가 잠겨있습니다. 열쇠가 필요합니다.");
                                gotoxy(5, (MAP_HEIGHT / 2) + 2);
                                printf("돌아가려면 스페이스바를 누르십시오...");

                                while (GetAsyncKeyState(VK_SPACE) & 0x8000) { Sleep(10); }
                                while (!(GetAsyncKeyState(VK_SPACE) & 0x8000)) { Sleep(30); }
                                system("cls");
                            }
                            else {
                                int selection = 0;
                                bool menuActive = true;

                                while (GetAsyncKeyState(VK_SPACE) & 0x8000) { Sleep(10); }
                                bool menuSpacePressed = false;

                                while (menuActive) {
                                    gotoxy(5, MAP_HEIGHT / 2);
                                    printf("★ 열쇠를 사용해 학교 건물 밖으로 탈출하시겠습니까? ★");
                                    gotoxy(7, (MAP_HEIGHT / 2) + 3);
                                    if (selection == 0) printf("▶ 예      아니오");
                                    else printf("   예   ▶ 아니오");

                                    if (GetAsyncKeyState(VK_LEFT) & 0x8000)  selection = 0;
                                    if (GetAsyncKeyState(VK_RIGHT) & 0x8000) selection = 1;

                                    if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
                                        if (!menuSpacePressed) {
                                            if (selection == 0) gameClear = true;
                                            menuActive = false;
                                            menuSpacePressed = true;
                                        }
                                    }
                                    else { menuSpacePressed = false; }
                                    Sleep(100);
                                }
                                system("cls");
                                if (gameClear) break;
                            }
                            playerMoveTurn = 0;
                            spacePressed = true;
                            continue;
                        }

                        px = nextX;
                        py = nextY;
                    }
                }
                playerMoveTurn = 0;
            }
        }

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            gameOver = true;
            break;
        }

        // 5-1. 문 역추격 시스템
        if (!bossActive && bossFollowTimer > 0) {
            bossFollowTimer--;
            if (bossFollowTimer == 0) {
                bossActive = true;

                if (currentRoom == 0) {
                    if (prevRoom == 1) { mx = 5;  my = 1; }
                    if (prevRoom == 2) { mx = 20; my = 1; }
                    if (prevRoom == 3) { mx = 10; my = MAP_HEIGHT - 2; }
                    if (prevRoom == 4) { mx = 30; my = MAP_HEIGHT - 2; }
                }
                else {
                    int doorIdx = currentRoom - 1;
                    mx = doors[doorIdx].nextMonsterX;
                    my = doors[doorIdx].nextMonsterY;
                }
            }
        }

        // 5-2. 은신 후 리스폰 시스템
        if (!bossActive && bossReappearTimer > 0) {
            bossReappearTimer--;
            if (bossReappearTimer == 0) {
                bossActive = true;
                if (currentRoom == 0) {
                    if (prevRoom == 1) { mx = 5; my = 1; }
                    if (prevRoom == 2) { mx = 20; my = 1; }
                    if (prevRoom == 3) { mx = 10; my = MAP_HEIGHT - 2; }
                    if (prevRoom == 4) { mx = 30; my = MAP_HEIGHT - 2; }
                }
                else {
                    int doorIdx = currentRoom - 1;
                    mx = doors[doorIdx].nextMonsterX;
                    my = doors[doorIdx].nextMonsterY;
                }
            }
        }

        // 6. 아오오니 AI 추격 (★ 속도를 4턴에 1칸으로 하향 조정)
        if (bossActive && mx != -10 && my != -10) {
            monsterMoveTurn++;
            if (monsterMoveTurn >= 4) { // 기존 2에서 4로 변경 (더 느리게 반응)
                monsterSubTurn++;
                if (monsterSubTurn % 5 != 0) {
                    int targetX = mx; int targetY = my;

                    if (mx < px) targetX++;
                    else if (mx > px) targetX--;
                    if (my < py) targetY++;
                    else if (my > py) targetY--;

                    if (isHidden && abs(mx - px) <= 1 && abs(my - py) <= 1) {
                        bossActive = false; bossFollowTimer = 0;
                        bossWaitingAfterHide = true;
                        mx = -10; my = -10;
                    }

                    if (mx != -10 && my != -10) {
                        if (currentMap[targetY][targetX] != TILE_WALL) {
                            mx = targetX; my = targetY;
                        }
                        else {
                            if (currentMap[my][targetX] != TILE_WALL) mx = targetX;
                            else if (currentMap[targetY][mx] != TILE_WALL) my = targetY;
                        }
                    }
                }
                monsterMoveTurn = 0;
            }
        }

        Sleep(30);
    }

    system("cls");
    if (gameClear) {
        printf("\n\n\n\n\t🎉 [ STAGE CLEAR ] 🎉\n");
        printf("\t아오오니의 추격을 뚫고 건물 밖으로 무사히 탈출했습니다!\n");
        printf("\t최종 생존을 축하드립니다, 형님!\n\n\n");
    }
    else {
        printf("\n\n\n\n\t[ GAME OVER ]\n");
        printf("\t탈출하지 못하고 아오오니에게 잡혔습니다...\n\n\n");
    }

    return 0;
}