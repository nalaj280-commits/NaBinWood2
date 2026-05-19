#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <conio.h>
#include <windows.h>
#include <time.h>
#include <stdlib.h>


int floorNum = 1;
int love = 50;
int stamina = 100;
int timeStat = 90;

int gameOver = 0;
int clearGame = 0;

char map[15][31];

int playerX;
int playerY;

int teacherX;
int teacherY;

void loadMap();
void drawMap();
void movePlayer(char key);
void moveTeacher();
void girlfriendEvent();
void randomEvent();
void floorEffect();

void loadMap()
{
    if (floorNum == 1)
    {
        char temp[15][31] =
        {
            "############################",
            "#P     #       #         D #",
            "# #### # ##### # ####### # #",
            "#    # #     # #       # # #",
            "#### # ##### # ####### # # #",
            "#    #     # #       # #   #",
            "# ######## # ####### # ### #",
            "#        # #       # #     #",
            "###### # # ####### # ##### #",
            "#      # #       # #     # #",
            "# ###### ####### # ##### # #",
            "#        #     #    T    # #",
            "# ######## ### # ####### # #",
            "#                      #   #",
            "############################"

        };

        for (int i = 0; i < 15; i++)
        {
            for (int j = 0; j < 30; j++)
            {
                map[i][j] = temp[i][j];
            }
        }

        playerX = 1;
        playerY = 1;

        teacherX = 20;
        teacherY = 11;
    }

    else if (floorNum == 2)
    {
        char temp[15][31] =
        {
            "############################",
            "#P      ######            D#",
            "# ##### ###### ########## ##",
            "#     #        #          ##",
            "##### ######## # ######## ##",
            "#   #        # #        # ##",
            "# # ######## # ######## # ##",
            "# #      T # #      ## #  ##",
            "# ####### # ###### ## ######",
            "#       # #      # ##      #",
            "####### # ###### # ###### ##",
            "#       #        #        ##",
            "# ######################## #",
            "#                          #",
            "############################"
        };

        for (int i = 0; i < 15; i++)
        {
            for (int j = 0; j < 30; j++)
            {
                map[i][j] = temp[i][j];
            }
        }

        playerX = 1;
        playerY = 1;

        teacherX = 10;
        teacherY = 7;
    }

    else if (floorNum == 3)
    {
        char temp[15][31] =
        {
            "#############################",
            "#P         ######         D#",
            "# ######## ###### ##########",
            "#        #      #        ###",
            "######## # #### # ###### ###",
            "#      # # #### #      # ###",
            "# #### # #      ###### # ###",
            "# #### # ########## ## # ###",
            "#    # #        T## ## # ###",
            "#### # ########## ## # # ###",
            "#    #            ## # # ###",
            "# ################## # # ###",
            "#                    #     #",
            "# ##########################",
            "############################"
        };

        for (int i = 0; i < 15; i++)
        {
            for (int j = 0; j < 30; j++)
            {
                map[i][j] = temp[i][j];
            }
        }

        playerX = 1;
        playerY = 1;

        teacherX = 18;
        teacherY = 8;
    }
}

void drawMap()
{
    system("cls");

    printf("===== 교수님 몰래 탈출하기 =====\n\n");

    printf("현재 층 : %d층\n", floorNum);
    printf("남은 시간 : %d\n", timeStat);
    printf("여자친구 호감도 : %d\n", love);
    printf("스태미나 : %d\n\n", stamina);

    printf("WASD : 이동\n\n");

    for (int i = 0; i < 10; i++)
    {
        printf("%s\n", map[i]);
    }
}

void movePlayer(char key)
{
    int nextX = playerX;
    int nextY = playerY;

    if (key == 'w')
    {
        nextY--;
    }

    else if (key == 's')
    {
        nextY++;
    }

    else if (key == 'a')
    {
        nextX--;
    }

    else if (key == 'd')
    {
        nextX++;
    }

    if (map[nextY][nextX] != '#')
    {
        map[playerY][playerX] = ' ';

        playerX = nextX;
        playerY = nextY;

        if (map[playerY][playerX] == 'D')
        {
            floorNum++;

            if (floorNum > 3)
            {
                clearGame = 1;
                return;
            }

            loadMap();
            floorEffect();
            return;
        }

        map[playerY][playerX] = 'P';
    }
}

void moveTeacher()
{
    map[teacherY][teacherX] = ' ';

    if (teacherX < playerX)
    {
        teacherX++;
    }

    else if (teacherX > playerX)
    {
        teacherX--;
    }

    if (teacherY < playerY)
    {
        teacherY++;
    }

    else if (teacherY > playerY)
    {
        teacherY--;
    }

    if (playerX == teacherX && playerY == teacherY)
    {
        gameOver = 1;
    }

    map[teacherY][teacherX] = 'T';
}

void girlfriendEvent()
{
    int choice;

    system("cls");

    printf("==============================\n");
    printf("        여자친구 문자\n");
    printf("==============================\n\n");

    printf("오늘 진짜 오는 거 맞지...?\n\n");

    printf("1. 거의 다 왔어!\n");
    printf("2. 교수님이 안 보내줘...\n");
    printf("3. 읽씹한다\n\n");

    printf("선택 : ");
    scanf("%d", &choice);

    if (choice == 1)
    {
        love += 15;
        printf("\n호감도 상승!\n");
    }

    else if (choice == 2)
    {
        love -= 5;
        printf("\n여자친구가 실망했다...\n");
    }

    else
    {
        love -= 20;
        printf("\n여자친구가 삐졌다...\n");
    }

    Sleep(2000);
}

void randomEvent()
{
    int random = rand() % 3;

    system("cls");

    printf("==============================\n");
    printf("         랜덤 이벤트\n");
    printf("==============================\n\n");

    if (random == 0)
    {
        printf("교수님이 뒤를 돌아봤다!\n");
        stamina -= 10;
    }

    else if (random == 1)
    {
        printf("출석 체크가 시작됐다!\n");
        timeStat -= 5;
    }

    else
    {
        printf("친구가 몰래 출석을 대신 눌러줬다!\n");
        timeStat += 5;
    }

    Sleep(2000);
}

void floorEffect()
{
    system("cls");

    printf("==============================\n");
    printf("         %d층 도착\n", floorNum);
    printf("==============================\n\n");

    if (floorNum == 2)
    {
        printf("교수님이 더 빨라졌다...\n");
    }

    else if (floorNum == 3)
    {
        printf("출구가 얼마 남지 않았다!\n");
    }

    Sleep(2000);
}

int main()
{
    int frame = 0;

    srand((unsigned int)time(NULL));

    loadMap();

    while (1)
    {
        drawMap();

        if (_kbhit())
        {
            char key = _getch();
            movePlayer(key);
        }

        if (frame % 5 == 0)
        {
            moveTeacher();
            timeStat--;
        }

        if (frame == 25)
        {
            girlfriendEvent();
        }

        if (frame == 50)
        {
            randomEvent();
        }

        if (frame == 75)
        {
            girlfriendEvent();
        }

        if (timeStat <= 0 || stamina <= 0)
        {
            gameOver = 1;
        }

        if (gameOver == 1)
        {
            system("cls");

            printf("==============================\n");
            printf("         GAME OVER\n");
            printf("==============================\n\n");

            printf("교수님에게 붙잡혔습니다...\n");
            printf("교수님 : 다음 과제는 개인 프로젝트다.\n");

            break;
        }

        if (clearGame == 1)
        {
            system("cls");

            printf("==============================\n");
            printf("          탈출 성공\n");
            printf("==============================\n\n");

            if (love >= 80)
            {
                printf("TRUE END\n\n");
                printf("완벽한 기념일 성공!\n");
            }

            else if (love >= 60)
            {
                printf("GOOD END\n\n");
                printf("기념일 장소 도착 성공!\n");
            }

            else
            {
                printf("NORMAL END\n\n");
                printf("도착은 했지만 분위기가 어색하다...\n");
            }

            printf("\n최종 호감도 : %d\n", love);
            printf("남은 시간 : %d\n", timeStat);

            break;
        }

        Sleep(100);

        frame++;
    }

    system("pause");

    return 0;
}