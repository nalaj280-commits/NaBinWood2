#include <stdio.h>


#define COLOR_RESET "\x1b[0m"

#define FONT_COLOR_BLACK 30
#define BG_COLOR_BLACK 40
#define FONT_COLOR_RED 31
#define BG_COLOR_RED 41
#define FONT_COLOR_GREEN 32
#define BG_COLOR_GREEN 42
#define FONT_COLOR_YELLOW 33
#define BG_COLOR_YELLOW 43
#define FONT_COLOR_BLUE 34
#define BG_COLOR_BLUE 44
#define FONT_COLOR_MAGENTA 35
#define BG_COLOR_MAGENTA 45
#define FONT_COLOR_BRIGHTMAGENTA 95
#define BG_COLOR_BRIGHTMAGENTA 105
#define FONT_COLOR_WHITE 37
#define BG_COLOR_WHITE 47


void set_color(int code);
int move_cursor(int x, int y);

int menu = 1;
int isRunning = 1;

int RenderTitle()
{
	set_color(BG_COLOR_BRIGHTMAGENTA);
	set_color(FONT_COLOR_WHITE);
	move_cursor(50, 9);
	printf("                      ");
	move_cursor(50, 10);
	printf(" 이은석:추격의 시작  ");
	move_cursor(50, 11);
	printf("                      ");

	set_color(BG_COLOR_BLACK);

	if (menu == 1)
	{
		set_color(BG_COLOR_YELLOW);
	}
	move_cursor(52, 13);
	printf("  1. 시작  ");
	set_color(BG_COLOR_BLACK);

	if (menu == 2)
	{
		set_color(BG_COLOR_YELLOW);
	}
	move_cursor(52, 15);
	printf("  2. 게임설명  ");
	set_color(BG_COLOR_BLACK);

	if (menu == 3)
	{
		set_color(BG_COLOR_YELLOW);
	}
	move_cursor(52, 17);
	printf("  3. 팀소개  ");
	set_color(BG_COLOR_BLACK);

	if (menu == 4)
	{
		set_color(BG_COLOR_YELLOW);
	}
	move_cursor(52, 19);
	printf("  4. 게임종료  ");
	set_color(BG_COLOR_BLACK);

	char a = getch();
	switch (a)
	{
	case 'w':
		if (menu > 1)
		{
			menu = menu - 1;
		}
		break;
	case 's':
		if (menu < 4)
		{
			menu = menu + 1;
		}
		break;
	case 13:
		if (menu == 1) {
			system("cls");
			menu = 0;
			return 2;

			break;
		}
		else if (menu == 4)
		{
			isRunning = 0;

		}
	}

	return 0;
}

int MainGame()
{
	printf("game");
}

void set_color(int code)
{
	printf("\x1b[%dm", code);
}

int move_cursor(int x, int y)
{
	printf("\033[%d;%dH", y, x);
	return 0;
}

int main()
{
	system("chcp 65001");
	system("cls");

	int gameStatus = 0;

	while (isRunning)
	{
		switch (gameStatus)
		{
		case 0:
			gameStatus = RenderTitle();
			break;
		case 2:
			gameStatus = MainGame();
			break;
		}

	}

	system("cls");
	move_cursor(0, 25);

	return 0;
}