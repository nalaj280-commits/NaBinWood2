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

    // [기믹 추가]: 옷장에서 보스가 완전히 사라진 후, 다음 방으로 넘어갔을 때 작동할 타이머 변수
    int bossReappearTimer = 0;
    bool bossWaitingAfterHide = false; // 옷장 은신으로 보스를 완전히 따돌렸는지 여부 플래그

    bool isHidden = false;
    bool spacePressed = false;
    bool gameOver = false;

    int playerMoveTurn = 0;
    int monsterMoveTurn = 0;

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
                                // 추격 도중 문으로 도망친 경우 (기존 역추격 타이머)
                                bossFollowTimer = 40;
                                bossActive = false;
                            }
                            else {
                                // [형님 요청 반영]: 옷장에 숨어서 보스가 완전히 퇴장한 상태(mx == -10)였다면,
                                // 다음 방으로 넘어가는 순간 재등장 카운트다운 타이머를 시동합니다!
                                if (bossWaitingAfterHide && mx == -10) {
                                    bossReappearTimer = 150; // 약 4.5초 뒤 재등장 (수치 조절 가능)
                                    bossWaitingAfterHide = false; // 플래그 해제
                                }
                            }

                            // 방 이동 순간 텔레포트 좌표 셋팅
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

        // [형님 요청 기믹]: 옷장 은신 성공 후, 다음 방으로 넘어갔을 때 실시간 타이머 계산
        if (!bossActive && bossReappearTimer > 0) {
            bossReappearTimer--;
            if (bossReappearTimer == 0) {
                bossActive = true;
                // 플레이어가 있는 현재 방의 우측 상단 구석탱이에서 기습 리스폰!
                mx = MAP_WIDTH - 2;
                my = 2;
            }
        }

        // 6. 아오오니 AI
        if (bossActive) {
            monsterMoveTurn++;
            if (monsterMoveTurn >= 3) {
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

                    // 옷장 바로 앞(거리 1 이하)까지 도달하면 타임아웃 퇴장
                    if (abs(mx - px) <= 1 && abs(my - py) <= 1) {
                        bossActive = false;
                        bossFollowTimer = 0;
                        bossWaitingAfterHide = true; // 대기 플래그 ON (이제 다음 방 넘어가면 타이머 시작)
                        mx = -10; my = -10;
                    }
                }

                if (currentMap[targetY][targetX] != 1) {
                    mx = targetX;
                    my = targetY;
                }
                else {
                    if (currentMap[my][targetX] != 1) mx = targetX;
                    else if (currentMap[targetY][mx] != 1) my = targetY;
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