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

    printf("===== ������ ��� Ż���ϱ� =====\n\n");

    printf("���� �� : %d��\n", floorNum);
    printf("���� �ð� : %d\n", timeStat);
    printf("����ģ�� ȣ���� : %d\n", love);
    printf("���¹̳� : %d\n\n", stamina);

    printf("WASD : �̵�\n\n");

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
    printf("        ����ģ�� ����\n");
    printf("==============================\n\n");

    printf("���� ��¥ ���� �� ����...?\n\n");

    printf("1. ���� �� �Ծ�!\n");
    printf("2. �������� �� ������...\n");
    printf("3. �о��Ѵ�\n\n");

    printf("���� : ");
    scanf("%d", &choice);

    if (choice == 1)
    {
        love += 15;
        printf("\nȣ���� ���!\n");
    }

    else if (choice == 2)
    {
        love -= 5;
        printf("\n����ģ���� �Ǹ��ߴ�...\n");
    }

    else
    {
        love -= 20;
        printf("\n����ģ���� ������...\n");
    }

    Sleep(2000);
}

void randomEvent()
{
    int random = rand() % 3;

    system("cls");

    printf("==============================\n");
    printf("         ���� �̺�Ʈ\n");
    printf("==============================\n\n");

    if (random == 0)
    {
        printf("�������� �ڸ� ���ƺô�!\n");
        stamina -= 10;
    }

    else if (random == 1)
    {
        printf("�⼮ üũ�� ���۵ƴ�!\n");
        timeStat -= 5;
    }

    else
    {
        printf("ģ���� ��� �⼮�� ��� �������!\n");
        timeStat += 5;
    }

    Sleep(2000);
}

void floorEffect()
{
    system("cls");

    printf("==============================\n");
    printf("         %d�� ����\n", floorNum);
    printf("==============================\n\n");

    if (floorNum == 2)
    {
        printf("�������� �� ��������...\n");
    }

    else if (floorNum == 3)
    {
        printf("�ⱸ�� �� ���� �ʾҴ�!\n");
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

            printf("�����Կ��� ���������ϴ�...\n");
            printf("������ : ���� ������ ���� ������Ʈ��.\n");

            break;
        }

        if (clearGame == 1)
        {
            system("cls");

            printf("==============================\n");
            printf("          Ż�� ����\n");
            printf("==============================\n\n");

            if (love >= 80)
            {
                printf("TRUE END\n\n");
                printf("�Ϻ��� ����� ����!\n");
            }

            else if (love >= 60)
            {
                printf("GOOD END\n\n");
                printf("����� ��� ���� ����!\n");
            }

            else
            {
                printf("NORMAL END\n\n");
                printf("������ ������ �����Ⱑ ����ϴ�...\n");
            }

            printf("\n���� ȣ���� : %d\n", love);
            printf("���� �ð� : %d\n", timeStat);

            break;
        }

        Sleep(100);

        frame++;
    }

    system("pause");

    return 0;
}