#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <stdbool.h>
#include <math.h>

#define MAP_WIDTH 40
#define MAP_HEIGHT 20
#define NUM_ROOMS 4

// ≈∏¿œ ∞™ ¡§¿«
#define TILE_EMPTY 0
#define TILE_WALL 1
#define TILE_DESK 2     
#define TILE_EXIT 7
#define TILE_STAIRS 8  
#define TILE_CLOSET 15  

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

// ¥ÎªÁ √‚∑¬¿ª ¿ß«— ¿¸ø™ ∑Œ±◊ πÆ¿⁄ø≠
char messageLog[100] = "¡÷∫Ø¿ª ºˆªˆ«œø© ≈ª√‚«“ ¥‹º≠∏¶ √£¿∏Ω Ω√ø¿, «¸¥‘.";

// ∏ÛΩ∫≈Õ∞° µÓ¿Â«“ πÆ¿« ¡¬«•
int lastExitX = -10; int lastExitY = -10;

// «— πÊø°º≠ ∫∏Ω∫∏¶ ¿ÃπÃ µ˚µπ∑»¥¬¡ˆ √º≈©«œ¥¬ «√∑°±◊ (π´«— ∏ÆΩ∫∆˘ πÊ¡ˆ)
bool bossDefeatedInRoom = false;

// ¥ÎªÁ√¢ √‚∑¬ «‘ºˆ
void printMessageLog() {
    gotoxy(0, MAP_HEIGHT + 4);
    setColor(COLOR_YELLOW);
    printf("=================================================================================\n");
    printf("[æÀ∏≤] %-70s\n", messageLog);
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

    bool bossActive = false; int bossFollowTimer = 60;
    bool isHidden = false; bool spacePressed = false;
    bool gameOver = false; bool gameClear = false; // [ºˆ¡§ øœ∑·] æ’ø° ¿⁄∑·«¸ bool¿ª ¡§»Æ«œ∞‘ √ﬂ∞°«ﬂΩ¿¥œ¥Ÿ.

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
                else if (currentMap[i][j] == TILE_CLOSET) { setColor(COLOR_GREEN); printf("H "); }
                else { printf("  "); }
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

                if (currentMap[py][px] == TILE_CLOSET) {
                    isHidden = !isHidden;
                    if (isHidden) {
                        strcpy_s(messageLog, sizeof(messageLog), "ø ¿Â º”ø° º˚æ˙Ω¿¥œ¥Ÿ. º˚º“∏Æ∏¶ ¡◊¿ÃΩ Ω√ø¿...");
                    }
                    else {
                        strcpy_s(messageLog, sizeof(messageLog), "ø ¿Âø°º≠ ≥™ø‘Ω¿¥œ¥Ÿ.");
                    }
                }

                // πÊ ¿Ãµø(∞Ë¥‹) Ω√ º“∏Í «√∑°±◊ ∏Æº¬
                if (currentRoom == 1 && abs(px - 2) + abs(py - 1) <= 1) {
                    currentRoom = 10; px = 2; py = 2; lastExitX = 2; lastExitY = 1;
                    bossActive = false; bossFollowTimer = 20; bossDefeatedInRoom = false;
                    strcpy_s(messageLog, sizeof(messageLog), "2√˛ ∞Ë¥‹Ω«∑Œ ≥ª∑¡ø‘Ω¿¥œ¥Ÿ.");
                    system("cls"); spacePressed = true; continue;
                }
                if (currentRoom == 10 && abs(px - 2) + abs(py - 1) <= 1) {
                    currentRoom = 1; px = 2; py = 2; lastExitX = 2; lastExitY = 1;
                    bossActive = false; bossFollowTimer = 20; bossDefeatedInRoom = false;
                    strcpy_s(messageLog, sizeof(messageLog), "1√˛ ∞Ë¥‹Ω«∑Œ ø√∂Ûø‘Ω¿¥œ¥Ÿ.");
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

                if (currentRoom == 6 && targetDoorTile == 10) {
                    cursorInfo.bVisible = TRUE; SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);

                    gotoxy(0, MAP_HEIGHT + 7);
                    printf("¢∫ ∫Òπ–π¯»£ 4¿⁄∏Æ∏¶ ¿‘∑¬«œººø‰: ");
                    int inputPassword = 0;

                    if (scanf_s("%d", &inputPassword) == 1) {
                        if (inputPassword == 1111) {
                            px = 1; py = 5; currentRoom = 7; lastExitX = -10; lastExitY = -10;
                            bossActive = false; bossFollowTimer = -1; mx = -10; my = -10; bossDefeatedInRoom = true;
                            strcpy_s(messageLog, sizeof(messageLog), "√∂ƒ¿! ∫Òπ–π¯»£∞° ¿œƒ°«œø© ∫Òπ– ¿Âƒ° πÆ¿Ã ø≠∑»Ω¿¥œ¥Ÿ. (ø©±‚¥¬ æ»¿¸«’¥œ¥Ÿ.)");
                        }
                        else {
                            strcpy_s(messageLog, sizeof(messageLog), "ªﬂ∫Ú! ∞Ê∞Ì: ∫Òπ–π¯»£∞° ∆≤∑»Ω¿¥œ¥Ÿ! (æ∆ø¿ø¿¥œ∞° πÆø°º≠ √ﬂ∞›¿ª Ω√¿€«’¥œ¥Ÿ!)");
                            bossActive = true; bossFollowTimer = 0; mx = currentDoorX; my = currentDoorY; bossDefeatedInRoom = false;
                        }
                    }

        // 4. ?åÎ†à?¥Ïñ¥ ?¥Îèô Î∞??§ÏãúÍ∞?Î∞??ÑÌôò ?êÏ†ï
        if (!isHidden) {
            playerMoveTurn++;
            if (playerMoveTurn >= 2) {
                nextX = px;
                nextY = py;

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
                        gotoxy(5, MAP_HEIGHT / 2); printf("[!] ±≥ºˆΩ« ∫Òπ– ≈Î∑Œ∞° ¿·∞‹¿÷Ω¿¥œ¥Ÿ. ∏∂Ω∫≈Õ ø≠ºË∞° « ø‰«’¥œ¥Ÿ.");
                        gotoxy(5, (MAP_HEIGHT / 2) + 2); printf("µπæ∆∞°∑¡∏È Ω∫∆‰¿ÃΩ∫πŸ∏¶ ¥©∏£Ω Ω√ø¿...");
                        while (GetAsyncKeyState(VK_SPACE) & 0x8000) { Sleep(10); }
                        while (!(GetAsyncKeyState(VK_SPACE) & 0x8000)) { Sleep(30); }
                        system("cls");
                    }
                    else {
                        int selection = 0; bool menuActive = true;
                        while (GetAsyncKeyState(VK_SPACE) & 0x8000) { Sleep(10); }
                        bool menuSpacePressed = false;
                        while (menuActive) {
                            gotoxy(5, MAP_HEIGHT / 2); printf("°⁄ ø≠ºË∏¶ ªÁøÎ«ÿ «–±≥ ∞«π∞ π€¿∏∑Œ ≈ª√‚«œΩ√∞⁄Ω¿¥œ±Ó? °⁄");
                            gotoxy(7, (MAP_HEIGHT / 2) + 3);
                            if (selection == 0) printf("¢∫ øπ      æ∆¥œø¿"); else printf("   øπ   ¢∫ æ∆¥œø¿");
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

        // 4. √ﬂ∞›¿⁄ Ω∫∆˘ ≈∏¿Ã∏” ¡∂¡§
        if (currentRoom != 0 && currentRoom != 7 && !bossActive && !bossDefeatedInRoom && bossFollowTimer > 0) {
            bossFollowTimer--;
            if (bossFollowTimer == 0) {
                bossActive = true;
                mx = lastExitX; my = lastExitY;
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

                    if (isHidden && abs(mx - px) <= 1 && abs(my - py) <= 1) {
                        bossActive = false;
                        bossFollowTimer = -1;
                        mx = -10; my = -10;
                        bossDefeatedInRoom = true;
                        strcpy_s(messageLog, sizeof(messageLog), "æ∆ø¿ø¿¥œ∞° πÆ π€¿∏∑Œ øœ¿¸»˜ π∞∑Ø∞¨Ω¿¥œ¥Ÿ. æ»¿¸«’¥œ¥Ÿ.");
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
        printf("\n\n\n\n\t?? [ STAGE CLEAR ] ??\n");
        printf("\t?úÏû¨ ?ºÏ™Ω ?®Í≤®Ïß?Î¨∏ÏùÑ ?¥Í≥† ?Ä?ùÏóê???ÑÎ≤Ω?òÍ≤å ?àÏ∂ú?àÏäµ?àÎã§!\n");
        printf("\tÏµúÏ¢Ö ?ùÏ°¥??Ï∂ïÌïò?úÎ¶Ω?àÎã§, ?ïÎãò!\n\n\n");
    }
    else {
        printf("\n\n\n\n\t[ GAME OVER ]\n");
        printf("\t?Ä?Ä?ùÏóê???àÏ∂ú?òÏ? Î™ªÌïòÍ≥??°Ìòî?µÎãà??..\n\n\n");
    }

    return 0;
}