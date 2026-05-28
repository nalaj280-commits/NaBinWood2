#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <stdbool.h>
#include <math.h>

#define MAP_WIDTH 40
#define MAP_HEIGHT 20
#define NUM_ROOMS 4

void gotoxy(int x, int y) {
    COORD pos;
    pos.X = x;
    pos.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

int world_maps[NUM_ROOMS][MAP_HEIGHT][MAP_WIDTH];

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
        for (j = 5; j <= 30; j++) world_maps[2][i][j] = 1;
    }
    world_maps[2][10][MAP_WIDTH - 2] = 3;
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
    int mx = 30, my = 5;

    bool bossActive = true;
    int bossFollowTimer = 0;
    int bossReappearTimer = 0;
    bool bossWaitingAfterHide = false;

    bool isHidden = false;
    bool spacePressed = false;
    bool gameOver = false;

    int playerMoveTurn = 0;
    int monsterMoveTurn = 0;
    int monsterSubTurn = 0; // 미세 속도 조절을 위한 서브 변수

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

    while (!gameOver) {
        int (*currentMap)[MAP_WIDTH] = world_maps[currentRoom];

        // 1. 화면 렌더링
        gotoxy(0, 0);
        printf("+-------------------------------------------------------------------------------+\n");
        printf("| Room: %-30s | Status: %-27s |\n", roomNames[currentRoom], isHidden ? "HIDING IN CLOSET" : "SURVIVING...");
        printf("+-------------------------------------------------------------------------------+\n");

        for (i = 0; i < MAP_HEIGHT; i++) {
            for (j = 0; j < MAP_WIDTH; j++) {
                if (i == py && j == px && !isHidden) {
                    printf("P ");
                }
                else if (i == my && j == mx && bossActive) {
                    printf("M ");
                }
                else if (currentMap[i][j] >= 2 && currentMap[i][j] <= 5) {
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

        // 3. 숨기 기믹 처리 (Spacebar)
        if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
            if (!spacePressed) {
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

                        if (tileValue >= 2 && tileValue <= 5) {
                            prevRoom = currentRoom;
                            targetRoom = tileValue - 2;

                            if (bossActive) {
                                bossFollowTimer = 40;
                                bossActive = false;
                            }
                            else {
                                if (bossWaitingAfterHide) {
                                    bossReappearTimer = 25; // 0.8초 뒤 기습 등장
                                    bossWaitingAfterHide = false;
                                }
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

        // 5. 문으로 도망쳤을 때의 보스 역추격 리스폰 로직
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

        // 옷장 은신 성공 후, 다음 방 문(D) 위치에서 기습 등장 타이머 (0.8초)
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
                // [이속 미세 하향]: 플레이어(2루프당 1칸)보다 아주 살짝만 느리게 조절 (5루프당 2칸 이동 효과)
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
    printf("\n\n\n\n\t[ GAME OVER ]\n");
    printf("\t대저택에서 탈출하지 못하고 잡혔습니다...\n\n\n");

    return 0;
}