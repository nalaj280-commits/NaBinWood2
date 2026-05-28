#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <stdbool.h>
#include <math.h>

#define MAP_WIDTH 40
#define MAP_HEIGHT 20
#define NUM_ROOMS 4

#define KEY_UP 72
#define KEY_DOWN 80

void gotoxy(int x, int y) {
    COORD pos;
    pos.X = x;
    pos.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

int world_maps[NUM_ROOMS][MAP_HEIGHT][MAP_WIDTH];

// 아이템 관련 전역 변수
int itemRoom = 3;
int itemX = 33;
int itemY = 10;
bool hasKey = false;
bool isItemPicked = false;

void initWorldMaps() {
    int r, i, j;

    for (r = 0; r < NUM_ROOMS; r++) {
        for (i = 0; i < MAP_HEIGHT; i++) {
            for (j = 0; j < MAP_WIDTH; j++) {
                if (i == 0 || i == MAP_HEIGHT - 1 || j == 0 || j == MAP_WIDTH - 1) {
                    world_maps[r][i][j] = 1;
                }
                else {
                    world_maps[r][i][j] = 0;
                }
            }
        }
    }

    // --- [Room 0: 대저택 현관 로비] ---
    for (j = 15; j <= 24; j++) { world_maps[0][9][j] = 1; world_maps[0][10][j] = 1; }
    world_maps[0][3][5] = 1; world_maps[0][3][34] = 1;
    world_maps[0][1][20] = 3;
    world_maps[0][10][MAP_WIDTH - 2] = 5;
    world_maps[0][2][2] = 6; world_maps[0][2][3] = 6;

    // --- [Room 1: Cozy Bedroom (침실)] ---
    for (i = 3; i <= 6; i++) { for (j = 4; j <= 8; j++) world_maps[1][i][j] = 1; }
    for (j = 25; j <= 35; j++) world_maps[1][15][j] = 1;
    world_maps[1][MAP_HEIGHT - 2][20] = 2;
    world_maps[1][10][1] = 4;
    world_maps[1][2][36] = 6; world_maps[1][2][37] = 6;

    // --- [Room 2: 비밀의 서재] ---
    for (i = 3; i <= 14; i += 3) {
        for (j = 5; j <= 34; j++) world_maps[2][i][j] = 1;
    }
    world_maps[2][10][MAP_WIDTH - 2] = 3;
    world_maps[2][10][1] = 7;
    world_maps[2][1][1] = 6; world_maps[2][1][2] = 6;

    // --- [Room 3: 어두운 긴 복도] ---
    for (i = 4; i <= 15; i += 4) {
        for (j = 10; j <= 15; j++) world_maps[3][i][j] = 1;
        for (j = 25; j <= 30; j++) world_maps[3][i + 2][j] = 1;
    }
    world_maps[3][10][1] = 2;
    world_maps[3][1][36] = 6; world_maps[3][1][37] = 6;
}

int main() {
    int currentRoom = 0;
    int prevRoom = 0;
    int px = 20, py = 14;

    int mx = -10, my = -10;

    bool bossActive = false;
    int bossFollowTimer = 0;
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
    int targetX, targetY;
    int tileValue;
    int targetRoom;

    char* roomNames[4];
    CONSOLE_CURSOR_INFO cursorInfo;

    roomNames[0] = "Main Lobby (Room 0)";
    roomNames[1] = "Cozy Bedroom (Room 1)";
    roomNames[2] = "Secret Library (Room 2)";
    roomNames[3] = "Dark Hallway (Room 3)";

    initWorldMaps();

    cursorInfo.bVisible = FALSE;
    cursorInfo.dwSize = 1;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);

    system("cls");

    while (!gameOver && !gameClear) {
        int (*currentMap)[MAP_WIDTH] = world_maps[currentRoom];

        // 1. 화면 렌더링
        gotoxy(0, 0);
        printf("+-------------------------------------------------------------------------------+\n");
        printf("| Room: %-30s | Status: %-27s |\n", roomNames[currentRoom], isHidden ? "HIDING IN CLOSET" : "SURVIVING...");
        printf("| Inventory: %-66s |\n", hasKey ? "[KEY]" : "EMPTY");
        printf("+-------------------------------------------------------------------------------+\n");

        for (i = 0; i < MAP_HEIGHT; i++) {
            for (j = 0; j < MAP_WIDTH; j++) {
                if (i == py && j == px && !isHidden) {
                    printf("P ");
                }
                else if (i == my && j == mx && bossActive) {
                    printf("M ");
                }
                else if (currentRoom == itemRoom && !isItemPicked && i == itemY && j == itemX) {
                    printf("* ");
                }
                else if ((currentMap[i][j] >= 2 && currentMap[i][j] <= 5) || currentMap[i][j] == 7) {
                    printf("D ");
                }
                else if (currentMap[i][j] == 1) {
                    if (i == 0 || i == MAP_HEIGHT - 1 || j == 0 || j == MAP_WIDTH - 1) printf("# ");
                    else printf("X ");
                }
                else if (currentMap[i][j] == 6) {
                    printf("H ");
                }
                else {
                    printf("  ");
                }
            }
            printf("\n");
        }

        // 2. 잡혔을 때 게임 오버 판정
        if (bossActive && !isHidden && px == mx && py == my) {
            gameOver = true;
            break;
        }

        // 3. 스페이스바 입력 처리 (평상시 맵 전용)
        if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
            if (!spacePressed) {
                if (currentRoom == itemRoom && !isItemPicked) {
                    if (abs(px - itemX) <= 1 && abs(py - itemY) <= 1) {
                        hasKey = true;
                        isItemPicked = true;
                        gotoxy(0, MAP_HEIGHT + 4);
                        printf("[!] 탈출 열쇠를 획득했습니다!                      \n");
                        Sleep(600);
                    }
                }

                if (currentMap[py][px] == 6) {
                    isHidden = !isHidden;
                }
                spacePressed = true;
            }
        }
        else {
            spacePressed = false;
        }

        // 4. 플레이어 이동 및 실시간 방 전환 판정
        if (!isHidden) {
            playerMoveTurn++;
            if (playerMoveTurn >= 2) {
                nextX = px;
                nextY = py;

                if (GetAsyncKeyState(VK_UP) & 0x8000)    nextY--;
                if (GetAsyncKeyState(VK_DOWN) & 0x8000)  nextY++;
                if (GetAsyncKeyState(VK_LEFT) & 0x8000)  nextX--;
                if (GetAsyncKeyState(VK_RIGHT) & 0x8000) nextX++;

                if (nextX >= 0 && nextX < MAP_WIDTH && nextY >= 0 && nextY < MAP_HEIGHT) {
                    if (currentMap[nextY][nextX] != 1) {
                        tileValue = currentMap[nextY][nextX];

                        // 서재 왼쪽 탈출문 상호작용
                        if (currentRoom == 2 && tileValue == 7) {
                            system("cls");
                            if (!hasKey) {
                                gotoxy(5, MAP_HEIGHT / 2);
                                printf("[!] 탈출구가 굳게 잠겨 있습니다. '열쇠가 없습니다.'");
                                gotoxy(5, (MAP_HEIGHT / 2) + 2);
                                printf("돌아가려면 스페이스바를 누르십시오...");

                                // 이전에 누르고 있던 스페이스바 떼기 대기
                                while (GetAsyncKeyState(VK_SPACE) & 0x8000) { Sleep(10); }
                                // 새로 스페이스바 누를 때까지 대기
                                while (!(GetAsyncKeyState(VK_SPACE) & 0x8000)) { Sleep(30); }
                                system("cls");
                            }
                            else {
                                int selection = 0;
                                bool menuActive = true;

                                // 이전에 문 앞으로 오면서 누른 스페이스바 떼기 대기 (연속 입력 방지)
                                while (GetAsyncKeyState(VK_SPACE) & 0x8000) { Sleep(10); }

                                bool menuSpacePressed = false;

                                while (menuActive) {
                                    gotoxy(5, MAP_HEIGHT / 2);
                                    printf("★ 탈출구 앞에서 열쇠를 사용하시겠습니까? ★");

                                    gotoxy(7, (MAP_HEIGHT / 2) + 3);
                                    if (selection == 0) printf("▶ 예      아니오");
                                    else printf("   예   ▶ 아니오");

                                    if (GetAsyncKeyState(VK_LEFT) & 0x8000)  selection = 0;
                                    if (GetAsyncKeyState(VK_RIGHT) & 0x8000) selection = 1;

                                    // [기믹 반영]: 엔터 대신 스페이스바로 고르기!
                                    if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
                                        if (!menuSpacePressed) {
                                            if (selection == 0) {
                                                gameClear = true;
                                            }
                                            menuActive = false;
                                            menuSpacePressed = true;
                                        }
                                    }
                                    else {
                                        menuSpacePressed = false;
                                    }
                                    Sleep(100);
                                }
                                system("cls");
                                if (gameClear) break;
                            }
                            playerMoveTurn = 0;
                            spacePressed = true; // 메인 루프로 복귀할 때 중복 입력 방지
                            continue;
                        }

                        // 일반 문 이동 처리
                        if (tileValue >= 2 && tileValue <= 5) {
                            prevRoom = currentRoom;
                            targetRoom = tileValue - 2;

                            if (bossWaitingAfterHide) {
                                bossReappearTimer = 25;
                                bossFollowTimer = 0;
                                bossActive = false;
                                bossWaitingAfterHide = false;
                            }
                            else {
                                bossFollowTimer = 25;
                                bossActive = false;
                            }

                            if (currentRoom == 0 && targetRoom == 1) { px = 20; py = MAP_HEIGHT - 3; }
                            else if (currentRoom == 1 && targetRoom == 0) { px = 20; py = 3; }
                            else if (currentRoom == 1 && targetRoom == 2) { px = MAP_WIDTH - 3; py = 10; }
                            else if (currentRoom == 2 && targetRoom == 1) { px = 3; py = 10; }
                            else if (currentRoom == 0 && targetRoom == 3) { px = 3; py = 10; }
                            else if (currentRoom == 3 && targetRoom == 0) { px = MAP_WIDTH - 3; py = 10; }

                            currentRoom = targetRoom;
                            system("cls");
                            playerMoveTurn = 0;
                            continue;
                        }
                        else {
                            px = nextX;
                            py = nextY;
                        }
                    }
                }
                playerMoveTurn = 0;
            }
        }

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            gameOver = true;
            break;
        }

        // 5-1. 문 역추격 리스폰 로직
        if (!bossActive && bossFollowTimer > 0) {
            bossFollowTimer--;
            if (bossFollowTimer == 0) {
                bossActive = true;
                if (currentRoom == 1 && prevRoom == 0) { mx = 20; my = MAP_HEIGHT - 2; }
                else if (currentRoom == 0 && prevRoom == 1) { mx = 20; my = 1; }
                else if (currentRoom == 2 && prevRoom == 1) { mx = MAP_WIDTH - 2; my = 10; }
                else if (currentRoom == 1 && prevRoom == 2) { mx = 1; my = 10; }
                else if (currentRoom == 3 && prevRoom == 0) { mx = 1; my = 10; }
                else if (currentRoom == 0 && prevRoom == 3) { mx = MAP_WIDTH - 2; my = 10; }
                else { bossActive = false; bossFollowTimer = 20; }
            }
        }

        // 5-2. 옷장 은신 성공 후 기습 등장 타이머
        if (!bossActive && bossReappearTimer > 0) {
            bossReappearTimer--;
            if (bossReappearTimer == 0) {
                bossActive = true;
                if (currentRoom == 1 && prevRoom == 0) { mx = 20; my = MAP_HEIGHT - 2; }
                else if (currentRoom == 0 && prevRoom == 1) { mx = 20; my = 1; }
                else if (currentRoom == 2 && prevRoom == 1) { mx = MAP_WIDTH - 2; my = 10; }
                else if (currentRoom == 1 && prevRoom == 2) { mx = 1; my = 10; }
                else if (currentRoom == 3 && prevRoom == 0) { mx = 1; my = 10; }
                else if (currentRoom == 0 && prevRoom == 3) { mx = MAP_WIDTH - 2; my = 10; }
                else { mx = MAP_WIDTH - 2; my = 2; }
            }
        }

        // 6. 아오오니 AI
        if (bossActive && mx != -10 && my != -10) {
            monsterMoveTurn++;
            if (monsterMoveTurn >= 2) {
                monsterSubTurn++;
                if (monsterSubTurn % 5 != 0) {
                    targetX = mx;
                    targetY = my;

                    if (!isHidden) {
                        if (mx < px) targetX++;
                        else if (mx > px) targetX--;
                        if (my < py) targetY++;
                        else if (my > py) targetY--;
                    }
                    else {
                        if (mx < px) targetX++;
                        else if (mx > px) targetX--;
                        if (my < py) targetY++;
                        else if (my > py) targetY--;

                        if (abs(mx - px) <= 1 && abs(my - py) <= 1) {
                            bossActive = false;
                            bossFollowTimer = 0;
                            bossWaitingAfterHide = true;
                            mx = -10; my = -10;
                        }
                    }

                    if (mx != -10 && my != -10) {
                        if (currentMap[targetY][targetX] != 1) {
                            mx = targetX;
                            my = targetY;
                        }
                        else {
                            if (currentMap[my][targetX] != 1) mx = targetX;
                            else if (currentMap[targetY][mx] != 1) my = targetY;
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
        printf("\t서재 왼쪽 숨겨진 문을 열고 저택에서 완벽하게 탈출했습니다!\n");
        printf("\t최종 생존을 축하드립니다, 형님!\n\n\n");
    }
    else {
        printf("\n\n\n\n\t[ GAME OVER ]\n");
        printf("\t대저택에서 탈출하지 못하고 잡혔습니다...\n\n\n");
    }

    return 0;
}