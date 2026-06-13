#define _CRT_SECURE_NO_WARNINGS // 인코딩 및 보안 경고 방지
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

// 대사 문자열 버퍼 크기
char messageLog[200] = "주변을 수색하여 탈출할 단서를 찾으십시오.";

// 몬스터가 등장할 문의 좌표
int lastExitX = -10; int lastExitY = -10;

// 한 방에서 보스를 이미 따돌렸는지 체크하는 플래그
bool bossDefeatedInRoom = false;

// 비밀번호 문이 한 번 열렸는지 기억하는 플래그
bool isDoorUnlocked = false;

// 맨 처음 시작한 복도 상태인지 체크하는 플래그
bool isFirstRoom = true;

// 쯔꾸루 스타일 대화창 출력 함수 (팝업 스타일)
void showDialog(const char* speaker, const char* text) {
    int startY = MAP_HEIGHT + 4; // 맵 바로 아래에 출력

    // 스페이스바가 이미 눌려있다면 뗄 때까지 대기
    while (GetAsyncKeyState(VK_SPACE) & 0x8000) { Sleep(10); }

    gotoxy(0, startY);
    setColor(COLOR_WHITE);
    // 쯔꾸루 감성의 두꺼운 테두리
    printf("┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
    printf("┃                                                                               ┃\n");
    printf("┃                                                                               ┃\n");
    printf("┃                                                                               ┃\n");
    printf("┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n");

    // 발화자 (이름) 출력
    gotoxy(4, startY + 1);
    setColor(COLOR_YELLOW);
    printf("▶ %s", speaker);

    // 대사 내용 출력
    gotoxy(4, startY + 2);
    setColor(COLOR_WHITE);
    printf("%-70s", text);

    // 다음 넘김 키 표시
    gotoxy(66, startY + 3);
    setColor(COLOR_GREEN);
    printf("(Space) ▼");

    // 유저가 스페이스바를 누를 때까지 대기 (여기서 게임이 일시정지됨)
    while (!(GetAsyncKeyState(VK_SPACE) & 0x8000)) { Sleep(30); }
    while (GetAsyncKeyState(VK_SPACE) & 0x8000) { Sleep(10); } // 뗄 때까지 대기
}

// 평상시 얇은 상태창 출력 함수
void printMessageLog() {
    int startY = MAP_HEIGHT + 4;
    gotoxy(0, startY);
    setColor(COLOR_YELLOW);
    printf("=================================================================================\n");
    setColor(COLOR_WHITE);
    printf("[알림] %-70s\n", messageLog);
    setColor(COLOR_YELLOW);
    printf("=================================================================================\n");

    // 대화창이 남긴 잔여 라인을 덮어써서 완벽하게 지워줌 (사라지는 효과)
    printf("                                                                                 \n");
    printf("                                                                                 \n");
    setColor(COLOR_WHITE);
}

// 특정 타일 하나만 다시 그리는 부분 렌더링 함수
void drawSingleTile(int currentRoom, int i, int j) {
    if (i < 0 || i >= MAP_HEIGHT || j < 0 || j >= MAP_WIDTH) return;
    gotoxy(j * 2, i + 4);
    int tile = world_maps[currentRoom][i][j];

    if (j >= room_limit_width[currentRoom] || i >= room_limit_height[currentRoom]) { printf("  "); }
    else if (currentRoom == itemRoom && !isItemPicked && i == itemY && j == itemX) { setColor(COLOR_GREEN); printf("* "); }
    else if ((tile >= 3 && tile <= 6) || tile == 10 || tile == TILE_EXIT) { setColor(COLOR_BROWN); printf("目 "); }
    else if (tile == TILE_STAIRS) { setColor(COLOR_PURPLE); printf("S "); }
    else if (tile == TILE_DESK) { setColor(COLOR_YELLOW); printf("T "); }
    else if (tile == TILE_WALL) {
        setColor(COLOR_DARKGRAY);
        if (i == 0 || i == room_limit_height[currentRoom] - 1 || j == 0 || j == room_limit_width[currentRoom] - 1) printf("# ");
        else printf("X ");
    }
    else if (tile == TILE_CLOSET) { setColor(COLOR_GREEN); printf("▩"); }
    else { printf("  "); }
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
    roomNames[6] = "이은석 교수실"; roomNames[7] = "[비밀공간]"; roomNames[8] = "2층 창고";
    roomNames[9] = "2층 강의실"; roomNames[10] = "2층 계단실";

    initWorldMaps(); initDoors();

    cursorInfo.bVisible = FALSE; cursorInfo.dwSize = 1;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);

    bool fullRedraw = true;
    int oldPx = -1, oldPy = -1;
    int oldMx = -1, oldMy = -1;
    bool oldIsHidden = false;
    char oldMessageLog[200] = "";

    while (!gameOver && !gameClear) {
        int (*currentMap)[MAP_WIDTH] = world_maps[currentRoom];

        if (fullRedraw) {
            system("cls");
            gotoxy(0, 0);
            setColor(COLOR_WHITE);
            printf("+-------------------------------------------------------------------------------+\n");
            printf("| 방 정보: %-26s | 상태: %-31s |\n", roomNames[currentRoom], isHidden ? "옷장에 숨음 (SAFE)" : "추격당하는 중...");
            printf("| 소지품: %-68s |\n", hasKey ? "[교수실 열쇠]" : "없음");
            printf("+-------------------------------------------------------------------------------+\n");

            for (i = 0; i < MAP_HEIGHT; i++) {
                for (j = 0; j < MAP_WIDTH; j++) {
                    drawSingleTile(currentRoom, i, j);
                }
            }

            strcpy_s(oldMessageLog, sizeof(oldMessageLog), "");
            fullRedraw = false;
            oldPx = -1; oldPy = -1; oldMx = -1; oldMy = -1;
            oldIsHidden = isHidden;
        }
        else {
            if (isHidden != oldIsHidden) {
                gotoxy(46, 1);
                setColor(COLOR_WHITE);
                printf("%-31s", isHidden ? "옷장에 숨음 (SAFE)" : "추격당하는 중...   ");
                oldIsHidden = isHidden;
            }
        }

        // 평상시 상태창 업데이트 (대화가 끝나면 이 창이 덮어쓰면서 팝업이 사라집니다)
        if (strcmp(messageLog, oldMessageLog) != 0) {
            printMessageLog();
            strcpy_s(oldMessageLog, sizeof(oldMessageLog), messageLog);
        }

        // 플레이어 렌더링
        if (oldPx != px || oldPy != py || oldIsHidden != isHidden) {
            if (oldPx != -1 && oldPy != -1) {
                drawSingleTile(currentRoom, oldPy, oldPx);
            }
            if (!isHidden) {
                gotoxy(px * 2, py + 4);
                setColor(COLOR_BLUE);
                printf("P ");
            }
            oldPx = px; oldPy = py;
        }

        // 몬스터 렌더링
        if (oldMx != mx || oldMy != my || !bossActive) {
            if (oldMx != -1 && oldMy != -1) {
                drawSingleTile(currentRoom, oldMy, oldMx);
                drawSingleTile(currentRoom, oldMy, oldMx + 1);
                drawSingleTile(currentRoom, oldMy, oldMx + 2);
            }
            if (bossActive && mx != -10 && my != -10) {
                gotoxy(mx * 2, my + 4);
                setColor(COLOR_RED);
                printf("⊙_⊙ ");
            }
            oldMx = mx; oldMy = my;
        }

        if (bossActive && !isHidden) {
            if (abs(px - mx) <= 2 && py == my) {
                gameOver = true;
                break;
            }
        }

        // 2. 스페이스바 상호작용
        if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
            if (!spacePressed) {
                if (currentRoom == 8 && abs(px - 10) <= 1 && abs(py - 4) <= 1) {
                    showDialog("나", "책상 서랍에서 낡은 노트를 발견했다.");
                    showDialog("낡은 노트", "'비밀번호는 [1111]이다.'");
                    showDialog("나", "비밀번호 단서를 확인했다. 잊지 말자.");

                    spacePressed = true; fullRedraw = true; continue;
                }

                if (currentRoom == itemRoom && !isItemPicked && abs(px - itemX) <= 1 && abs(py - itemY) <= 1) {
                    hasKey = true; isItemPicked = true;
                    showDialog("시스템", "강의실 바닥에서 반짝이는 것을 발견했습니다.");
                    showDialog("시스템", "[교수실 마스터 열쇠]를 획득했습니다!");

                    fullRedraw = true; spacePressed = true; continue;
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

                if (currentRoom == 1 && abs(px - 2) + abs(py - 1) <= 1) {
                    currentRoom = 10; px = 2; py = 2; lastExitX = 2; lastExitY = 1;
                    bossActive = false; bossFollowTimer = 20; bossDefeatedInRoom = false;
                    strcpy_s(messageLog, sizeof(messageLog), "2층 계단실로 내려왔습니다.");
                    fullRedraw = true; spacePressed = true; continue;
                }
                if (currentRoom == 10 && abs(px - 2) + abs(py - 1) <= 1) {
                    currentRoom = 1; px = 2; py = 2; lastExitX = 2; lastExitY = 1;
                    bossActive = false; bossFollowTimer = 20; bossDefeatedInRoom = false;
                    strcpy_s(messageLog, sizeof(messageLog), "1층 계단실로 올라왔습니다.");
                    fullRedraw = true; spacePressed = true; continue;
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

                // 이은석 교수실 비밀번호 문 상호작용
                if (currentRoom == 6 && targetDoorTile == 10) {
                    if (isDoorUnlocked) {
                        showDialog("시스템", "이미 잠금 해제된 비밀 통로를 통과합니다.");
                        px = 1; py = 5; currentRoom = 7; lastExitX = -10; lastExitY = -10;
                        bossActive = false; bossFollowTimer = -1; mx = -10; my = -10; bossDefeatedInRoom = true;
                        fullRedraw = true; spacePressed = true; continue;
                    }

                    int startY = MAP_HEIGHT + 4;
                    while (GetAsyncKeyState(VK_SPACE) & 0x8000) { Sleep(10); }

                    // 대화창 팝업
                    gotoxy(0, startY);
                    setColor(COLOR_WHITE);
                    printf("┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
                    printf("┃                                                                               ┃\n");
                    printf("┃                                                                               ┃\n");
                    printf("┃                                                                               ┃\n");
                    printf("┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n");
                    gotoxy(4, startY + 1); setColor(COLOR_YELLOW); printf("▶ 도어락");
                    gotoxy(4, startY + 2); setColor(COLOR_WHITE); printf("비밀번호 4자리를 입력하세요: ");

                    cursorInfo.bVisible = TRUE;
                    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);

                    int inputPassword = 0;
                    if (scanf_s("%d", &inputPassword) != 1) {
                        while (getchar() != '\n');
                        inputPassword = -1;
                    }

                    cursorInfo.bVisible = FALSE;
                    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);

                    if (inputPassword == 1111) {
                        showDialog("시스템", "철컥! 비밀번호가 일치하여 비밀 장치 문이 열렸습니다.");
                        px = 1; py = 5; currentRoom = 7; lastExitX = -10; lastExitY = -10;
                        bossActive = false; bossFollowTimer = -1; mx = -10; my = -10; bossDefeatedInRoom = true;
                        isDoorUnlocked = true;
                    }
                    else {
                        showDialog("시스템", "[경고] 비밀번호가 틀렸습니다!");
                        showDialog("나", "이런, 소리가 너무 컸어... 교수가 이쪽으로 오고 있다!");
                        bossActive = true;
                        mx = 1; my = 1; monsterMoveTurn = -10; bossDefeatedInRoom = false;
                    }

                    fullRedraw = true; // 대화창 제거 및 맵 원상복구
                    spacePressed = true;
                    continue;
                }

                if ((targetDoorTile >= 3 && targetDoorTile <= 6) || targetDoorTile == 10) {
                    bossActive = false; bossFollowTimer = 25; bossDefeatedInRoom = false; isFirstRoom = false;
                    if (currentRoom == 0) {
                        int doorIdx = targetDoorTile - 3; px = doors[doorIdx].nextPlayerX; py = doors[doorIdx].nextPlayerY; currentRoom = doors[doorIdx].targetRoom;
                        if (currentRoom == 1) { lastExitX = 10; lastExitY = 11; } if (currentRoom == 2) { lastExitX = 12; lastExitY = 13; }
                        if (currentRoom == 3) { lastExitX = 10; lastExitY = 0; } if (currentRoom == 4) { lastExitX = 10; lastExitY = 0; }
                    }
                    else if (currentRoom == 5) {
                        if (targetDoorTile == 3) { px = 10; py = 10; currentRoom = 10; lastExitX = 10; lastExitY = 11; }
                        if (targetDoorTile == 5) { px = 12; py = 12; currentRoom = 9;  lastExitX = 12; lastExitY = 13; }
                        if (targetDoorTile == 4) { px = 10; py = 1;  currentRoom = 6;  lastExitX = 10; lastExitY = 0; }
                        if (targetDoorTile == 6) { px = 10; py = 1;  currentRoom = 8;  lastExitX = 10; lastExitY = 0; }
                    }
                    else if (currentRoom == 7 && targetDoorTile == 10) { px = 18; py = 5; currentRoom = 6; lastExitX = 19; lastExitY = 5; }
                    else {
                        int prevRoom = currentRoom;
                        if (prevRoom == 1) { px = 5;  py = 1; currentRoom = 0; lastExitX = 5; lastExitY = 0; } if (prevRoom == 2) { px = 20; py = 1; currentRoom = 0; lastExitX = 20; lastExitY = 0; }
                        if (prevRoom == 3) { px = 10; py = MAP_HEIGHT - 2; currentRoom = 0; lastExitX = 10; lastExitY = MAP_HEIGHT - 1; } if (prevRoom == 4) { px = 30; py = MAP_HEIGHT - 2; currentRoom = 0; lastExitX = 30; lastExitY = MAP_HEIGHT - 1; }
                        if (prevRoom == 10) { px = 5;  py = 1; currentRoom = 5; lastExitX = 5; lastExitY = 0; } if (prevRoom == 9) { px = 25; py = 1; currentRoom = 5; lastExitX = 25; lastExitY = 0; }
                        if (prevRoom == 6) { px = 5;  py = MAP_HEIGHT - 2; currentRoom = 5; lastExitX = 5; lastExitY = MAP_HEIGHT - 1; } if (prevRoom == 8) { px = 30; py = MAP_HEIGHT - 2; currentRoom = 5; lastExitX = 30; lastExitY = MAP_HEIGHT - 1; }
                    }
                    fullRedraw = true; playerMoveTurn = 0; spacePressed = true; continue;
                }

                int nearExit = false;
                for (int d = 0; d < 4; d++) {
                    if (py + dy[d] >= 0 && py + dy[d] < MAP_HEIGHT && px + dx[d] >= 0 && px + dx[d] < MAP_WIDTH) {
                        if (currentMap[py + dy[d]][px + dx[d]] == TILE_EXIT) nearExit = true;
                    }
                }

                // 탈출구 메뉴를 대화창 팝업 시스템에 통합
                if (currentRoom == 4 && nearExit) {
                    if (!hasKey) {
                        showDialog("시스템", "[!] 교수실 비밀 통로가 잠겨있습니다.");
                        showDialog("나", "굳게 잠겨있어... 탈출하려면 [마스터 열쇠]가 필요해.");
                        fullRedraw = true; // 닫기
                    }
                    else {
                        int selection = 0; bool menuActive = true;
                        while (GetAsyncKeyState(VK_SPACE) & 0x8000) { Sleep(10); }
                        bool menuSpacePressed = false;

                        int startY = MAP_HEIGHT + 4;

                        while (menuActive) {
                            gotoxy(0, startY);
                            setColor(COLOR_WHITE);
                            printf("┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
                            printf("┃                                                                               ┃\n");
                            printf("┃                                                                               ┃\n");
                            printf("┃                                                                               ┃\n");
                            printf("┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n");
                            gotoxy(4, startY + 1); setColor(COLOR_YELLOW); printf("▶ 탈출구");
                            gotoxy(4, startY + 2); setColor(COLOR_WHITE); printf("열쇠를 사용해 학교 건물 밖으로 탈출하시겠습니까?");

                            gotoxy(6, startY + 3);
                            if (selection == 0) printf("▶ 예        아니오"); else printf("   예      ▶ 아니오");

                            if (GetAsyncKeyState(VK_LEFT) & 0x8000)  selection = 0;
                            if (GetAsyncKeyState(VK_RIGHT) & 0x8000) selection = 1;
                            if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
                                if (!menuSpacePressed) { if (selection == 0) gameClear = true; menuActive = false; menuSpacePressed = true; }
                            }
                            else { menuSpacePressed = false; }
                            Sleep(100);
                        }
                        if (gameClear) { system("cls"); break; }
                        else { fullRedraw = true; } // 대화창 닫기
                    }
                }
                spacePressed = true;
            }
        }
        else { spacePressed = false; }

        // 3. 플레이어 이동 로직
        static DWORD lastMoveTime = 0;

        if (GetTickCount() - lastMoveTime >= 130) {
            nextX = px; nextY = py;
            bool isMoved = false;

            if (GetAsyncKeyState(VK_UP) & 0x8000) { nextY--; isMoved = true; }
            else if (GetAsyncKeyState(VK_DOWN) & 0x8000) { nextY++; isMoved = true; }
            else if (GetAsyncKeyState(VK_LEFT) & 0x8000) { nextX--; isMoved = true; }
            else if (GetAsyncKeyState(VK_RIGHT) & 0x8000) { nextX++; isMoved = true; }

            if (isMoved && (nextX != px || nextY != py)) {
                if (nextX >= 0 && nextX < MAP_WIDTH && nextY >= 0 && nextY < MAP_HEIGHT) {
                    int nextTile = currentMap[nextY][nextX];
                    if (nextTile != TILE_WALL && !(nextTile >= 3 && nextTile <= 6) && nextTile != 10 && nextTile != TILE_EXIT && nextTile != TILE_DESK) {
                        px = nextX; py = nextY;
                        lastMoveTime = GetTickCount();
                    }
                }
            }
        }

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { gameOver = true; break; }

        // 4. 추격자 스폰 타이머 조정
        if (currentRoom != 7 && !bossActive && !bossDefeatedInRoom && bossFollowTimer > 0) {
            if (!(currentRoom == 0 && isFirstRoom)) {
                bossFollowTimer--;
                if (bossFollowTimer == 0) {
                    bossActive = true;
                    mx = lastExitX; my = lastExitY;
                }
            }
        }

        // 5. 아오오니 AI 추격
        if (currentRoom != 7 && bossActive && mx != -10 && my != -10) {
            monsterMoveTurn++;
            if (monsterMoveTurn >= 12) {
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
                        strcpy_s(messageLog, sizeof(messageLog), "이은석 교수가 문 밖으로 완전히 물러갔습니다. 안전합니다.");
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
        printf("\n\n\n\n\t[ STAGE CLEAR ]\n\t단서를 찾아 비밀번호를 풀고 대탈출에 완벽히 성공하셨습니다, 형님!\n\n\n");
    }
    else {
        printf("\n\n\n\n\t[ GAME OVER ]\n\t이은석교수에게 잡혔습니다...\n\n\n");
    }
    return 0;
}