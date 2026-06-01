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
#define KEY_ENTER 13

void gotoxy(int x, int y) {
    COORD pos;
    pos.X = x;
    pos.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

int world_maps[NUM_ROOMS][MAP_HEIGHT][MAP_WIDTH];

// ?ÑÏù¥??Í¥Ä???ÑÏó≠ Î≥Ä??
int itemRoom = 3;
int itemX = 35;
int itemY = 2;
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

    // --- [Room 0: ?Ä?Ä???ÑÍ? Î°úÎπÑ] ---
    for (j = 15; j <= 24; j++) { world_maps[0][9][j] = 1; world_maps[0][10][j] = 1; }
    world_maps[0][3][5] = 1; world_maps[0][3][34] = 1;
    world_maps[0][1][20] = 3;
    world_maps[0][10][MAP_WIDTH - 2] = 5;
    world_maps[0][2][2] = 6; world_maps[0][2][3] = 6;

    // --- [Room 1: Cozy Bedroom (Ïπ®Ïã§)] ---
    for (i = 3; i <= 6; i++) { for (j = 4; j <= 8; j++) world_maps[1][i][j] = 1; }
    for (j = 25; j <= 35; j++) world_maps[1][15][j] = 1;
    world_maps[1][MAP_HEIGHT - 2][20] = 2;
    world_maps[1][10][1] = 4;
    world_maps[1][2][36] = 6; world_maps[1][2][37] = 6;

    // --- [Room 2: ÎπÑÎ????úÏû¨] ---
    for (i = 3; i <= 14; i += 3) {
        for (j = 5; j <= 34; j++) world_maps[2][i][j] = 1; // Ï±ÖÏû• Î∞∞Ïπò ?¥Ïßù Ï°∞Ï†ï
    }
    // [Íµ¨Ï°∞ Î≥ÄÍ≤? ?§Î•∏Ï™?Î¨?3)?Ä Ïπ®Ïã§Í≥??∞Í≤∞?òÎäî ?ºÎ∞ò Î¨?/ ?ºÏ™Ω Î¨?7)???àÏ∂ú ?ÑÏö© Î¨∏ÏúºÎ°??àÎ°ú ?†ÏÑ§!
    world_maps[2][10][MAP_WIDTH - 2] = 3;
    world_maps[2][10][1] = 7; // ?àÏ∂ú ?ÑÏö© ?πÏàò ?Ä???òÎ≤Ñ '7' ÏßÄ??
    world_maps[2][1][1] = 6; world_maps[2][1][2] = 6;

    // --- [Room 3: ?¥Îëê??Í∏?Î≥µÎèÑ] ---
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

        // 1. ?îÎ©¥ ?åÎçîÎß?
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
                    printf("D "); // ?àÎ°ú ÎßåÎì† ?ºÏ™Ω ?àÏ∂ú Î¨∏ÎèÑ ?îÎ©¥?êÎäî 'D'Î°?Ï∂úÎ†•?òÍ≤å ?§Ï†ï
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

        // 2. ?°Ìòî????Í≤åÏûÑ ?§Î≤Ñ ?êÏ†ï
        if (bossActive && !isHidden && px == mx && py == my) {
            gameOver = true;
            break;
        }

        // 3. ?§Ìéò?¥Ïä§Î∞??ÖÎ†• Ï≤òÎ¶¨ (?®Í∏∞Í∏?OR ?ÑÏù¥??Ï§çÍ∏∞)
        if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
            if (!spacePressed) {
                if (currentRoom == itemRoom && !isItemPicked) {
                    if (abs(px - itemX) <= 1 && abs(py - itemY) <= 1) {
                        hasKey = true;
                        isItemPicked = true;
                        gotoxy(0, MAP_HEIGHT + 4);
                        printf("[!] ?àÏ∂ú ?¥Ïá†Î•??çÎìù?àÏäµ?àÎã§!                      \n");
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

        // 4. ?åÎ†à?¥Ïñ¥ ?¥Îèô Î∞??§ÏãúÍ∞?Î∞??ÑÌôò ?êÏ†ï
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

                        // [Í∏∞Î? ?òÏ†ï] ÎπÑÎ????úÏû¨(Room 2)??'?àÎ°ú???ºÏ™Ω ?àÏ∂ú ?ÑÏö© Î¨?7)' ?ÅÌò∏?ëÏö©
                        if (currentRoom == 2 && tileValue == 7) {
                            system("cls");
                            if (!hasKey) {
                                gotoxy(5, MAP_HEIGHT / 2);
                                printf("[!] ?àÏ∂úÍµ¨Í? Íµ≥Í≤å ?†Í≤® ?àÏäµ?àÎã§. '?¥Ïá†Í∞Ä ?ÜÏäµ?àÎã§.'");
                                gotoxy(5, (MAP_HEIGHT / 2) + 2);
                                printf("?åÏïÑÍ∞Ä?§Î©¥ ?ÑÎ¨¥ ?§ÎÇò ?ÑÎ•¥??ãú??..");
                                while (!(GetAsyncKeyState(VK_SPACE) & 0x8000) && !(GetAsyncKeyState(VK_RETURN) & 0x8000)) { Sleep(30); }
                                system("cls");
                            }
                            else {
                                int selection = 0;
                                bool menuActive = true;

                                while (menuActive) {
                                    gotoxy(5, MAP_HEIGHT / 2);
                                    printf("???àÏ∂úÍµ??ûÏóê???¥Ïá†Î•??¨Ïö©?òÏãúÍ≤†Ïäµ?àÍπå? ??);

                                    gotoxy(7, (MAP_HEIGHT / 2) + 3);
                                    if (selection == 0) printf("????     ?ÑÎãà??);
                                    else printf("   ??  ???ÑÎãà??);

                                    if (GetAsyncKeyState(VK_LEFT) & 0x8000)  selection = 0;
                                    if (GetAsyncKeyState(VK_RIGHT) & 0x8000) selection = 1;
                                    if (GetAsyncKeyState(VK_RETURN) & 0x8000) {
                                        if (selection == 0) {
                                            gameClear = true;
                                        }
                                        menuActive = false;
                                    }
                                    Sleep(100);
                                }
                                system("cls");
                                if (gameClear) break;
                            }
                            playerMoveTurn = 0;
                            continue;
                        }

                        // ?ºÎ∞ò?ÅÏù∏ Î¨??¥Îèô Ï≤òÎ¶¨ (?§Î•∏Ï™?Î¨??¨Ìï®)
                        if (tileValue >= 2 && tileValue <= 5) {
                            prevRoom = currentRoom;
                            targetRoom = tileValue - 2;

                            if (bossActive) {
                                bossFollowTimer = 25;
                                bossActive = false;
                            }
                            else {
                                if (bossWaitingAfterHide) {
                                    bossReappearTimer = 25;
                                    bossWaitingAfterHide = false;
                                }
                            }

                            // Î∞??ÑÌôò ???åÎ†à?¥Ïñ¥ Ï¥àÍ∏∞ ?ÑÏπò ?§Ï†ï
                            if (currentRoom == 0 && targetRoom == 1) { px = 20; py = MAP_HEIGHT - 3; }
                            else if (currentRoom == 1 && targetRoom == 0) { px = 20; py = 3; }
                            else if (currentRoom == 1 && targetRoom == 2) { px = MAP_WIDTH - 3; py = 10; } // ?úÏû¨ ?§Ïñ¥?????∞Ï∏°?ºÎ°ú ÏßÑÏûÖ
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

        // 5. Î¨?Î¶¨Ïä§??Î∞?Î≥µÍ? ?úÏñ¥ ?Ä?¥Î®∏ Î°úÏßÅ
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

        // 6. ?ÑÏò§?§Îãà AI
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
        printf("\n\n\n\n\t?éâ [ STAGE CLEAR ] ?éâ\n");
        printf("\t?úÏû¨ ?ºÏ™Ω ?®Í≤®Ïß?Î¨∏ÏùÑ ?¥Í≥† ?Ä?ùÏóê???ÑÎ≤Ω?òÍ≤å ?àÏ∂ú?àÏäµ?àÎã§!\n");
        printf("\tÏµúÏ¢Ö ?ùÏ°¥??Ï∂ïÌïò?úÎ¶Ω?àÎã§, ?ïÎãò!\n\n\n");
    }
    else {
        printf("\n\n\n\n\t[ GAME OVER ]\n");
        printf("\t?Ä?Ä?ùÏóê???àÏ∂ú?òÏ? Î™ªÌïòÍ≥??°Ìòî?µÎãà??..\n\n\n");
    }

    return 0;
}