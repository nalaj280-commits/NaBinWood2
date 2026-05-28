#include <stdio.h>
#include <wchar.h>
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <conio.h>

#pragma warning(disable:4996)

// 전역 변수
HANDLE hConsole;

int menu = 1;
int isRunning = 1;

// 콘솔 함수
void setColor(int color)
{
	SetConsoleTextAttribute(hConsole, color);
}

void move_cursor(int x, int y)
{
	COORD pos;

	pos.X = x;
	pos.Y = y;

	SetConsoleCursorPosition(hConsole, pos);
}

void printChar(wchar_t ch)
{
	DWORD written;

	WriteConsoleW(
		hConsole,
		&ch,
		1,
		&written,
		NULL
	);
}

// 문자열 실제 출력 폭 계산
int get_width(const wchar_t* str)
{
	int width = 0;

	while (*str)
	{
		if (*str >= 0xAC00 && *str <= 0xD7A3)
			width += 2;
		else
			width += 1;

		str++;
	}

	return width;
}

// 좌측 정렬 출력
void print_align_left(const wchar_t* text, int total_width)
{
	int width = get_width(text);

	wprintf(L"%ls", text);

	for (int i = width; i < total_width; i++)
	{
		wprintf(L" ");
	}
}


// 휴대폰 출력 함수
void draw_phone()
{
	int x = 40;
	int y = 3;

	move_cursor(x, y);
	wprintf(L"┌─────────────────────────────────────────┐");

	for (int i = 1; i < 40; i++)
	{
		move_cursor(x, y + i);
		wprintf(L"│                                         │");
	}

	move_cursor(x, y + 40);
	wprintf(L"└─────────────────────────────────────────┘");
}

// 핸드폰 잠금화면 출력
void draw_lock_screen()
{
	move_cursor(56, 6);
	wprintf(L"6월 22일 (월)");

	move_cursor(47, 9);
	wprintf(L" __  ___        ______  ______\n");
	move_cursor(47, 10);
	wprintf(L"/_ | |__ \\   _  | ____| | ____|\n");
	move_cursor(47, 11);
	wprintf(L" | |    ) | (_) | |__   | |__\n");
	move_cursor(47, 12);
	wprintf(L" | |   / /      |___ \\  |___ \\ \n");
	move_cursor(47, 13);
	wprintf(L" | |  / /_   _   ___) |  ___) |\n");
	move_cursor(47, 14);
	wprintf(L" |_| |____| (_) |____/  |____/\n");
}

void draw_notification(const wchar_t* name, const wchar_t* msg)
{
	int x = 45;
	int y = 32;

	// 기존 내용 지우기
	for (int i = 0; i < 4; i++)
	{
		move_cursor(x, y + i);
		wprintf(L"                                ");
	}

	move_cursor(x, y);
	wprintf(L"┌──────────────────────────────┐");

	move_cursor(x, y + 1);
	wprintf(L"│ ♥ ");
	print_align_left(name, 24);
	wprintf(L"  │");

	move_cursor(x, y + 2);
	wprintf(L"│ ");
	print_align_left(msg, 29);
	wprintf(L"│");

	move_cursor(x, y + 3);
	wprintf(L"└──────────────────────────────┘");
}

// ASCII ART 출력
void draw_art()
{
    FILE* fp = fopen("art_title.txt", "rb");

	if (!fp)
	{
		move_cursor(0, 0);
		wprintf(L"art.txt 파일을 열 수 없습니다.");
		return;
	}

	char utf8Buffer[4096];
	wchar_t wideBuffer[4096];

	int line = 0;

	while (fgets(utf8Buffer, sizeof(utf8Buffer), fp))
	{
		MultiByteToWideChar(
			CP_UTF8,
			0,
			utf8Buffer,
			-1,
			wideBuffer,
			4096
		);

		move_cursor(0, line);

		int x = 0;
		wchar_t* p = wideBuffer;

		while (*p)
		{
			wchar_t ch = *p;

			// 기본 회색
			setColor(8);

			// 노란색 영역
			if (
				line >= 14 && line <= 24 &&
				x >= 56 && x <= 121 &&
				ch != L' '
				)
			{
				setColor(6);
			}

			printChar(ch);

			p++;
			x++;
		}

		line++;
	}

	fclose(fp);

	setColor(7);
}

// 메뉴 출력
void draw_menu()
{
	// 메뉴 1
	if (menu == 1)
		setColor(14);
	else
		setColor(15);

	move_cursor(122, 23);
	wprintf(L"1. 게임 시작");

	// 메뉴 2
	if (menu == 2)
		setColor(14);
	else
		setColor(15);

	move_cursor(122, 27);
	wprintf(L"2. 게임 설명");

	// 메뉴 3
	if (menu == 3)
		setColor(14);
	else
		setColor(15);

	move_cursor(122, 31);
	wprintf(L"3. 제작자");

	// 메뉴 4
	if (menu == 4)
		setColor(14);
	else
		setColor(15);

	move_cursor(122, 35);
	wprintf(L"4. 종료");

	setColor(7);
}

// 타이틀 화면
int RenderTitle()
{
    static int first = 1;

    // 처음 한 번만 그림 출력
    if (first)
    {
        system("cls");
        draw_art();
        first = 0;
    }

    // 메뉴만 다시 그림
    draw_menu();

	char a = _getch();

	switch (a)
	{
	case 'w':

		if (menu > 1)
			menu--;

		break;

	case 's':

		if (menu < 4)
			menu++;

		break;

	case 13:

        first = 1; // 다른 화면 갔다가 다시 타이틀 올 때 재출력

        if (menu == 1)
        {
            return 2;
        }
        else if (menu == 2)
        {
            return 3;
        }
        else if (menu == 3)
        {
            return 4;
        }
        else if (menu == 4)
        {
            isRunning = 0;
        }

		break;
	}

	return 0;
}

// 게임 화면
int MainGame()
{
	system("cls");

	// 폰 출력
	draw_phone();

	// 잠금화면
	draw_lock_screen();

	// 1초 대기
	Sleep(1000);

	// 띠링
	Beep(1200, 200);

	// 첫 번째 알림
	draw_notification(
		L"여자친구",
		L"자기야 오늘 무슨 날인지 알지?"
	);

	Sleep(2500);

	// 두 번째 알림
	Beep(1000, 200);

	draw_notification(
		L"여자친구",
		L"나 오늘 이쁘게 입고 갈게!!"
	);

	Sleep(2500);

	// 세 번째 알림
	Beep(700, 400);

	draw_notification(
		L"여자친구",
		L"터미널에 2시까지 와야해!!"
	);

	Sleep(3000);

	//첫번째 대사
	Beep(1200, 200);
	move_cursor(100, 20);
	wprintf(L"아... 오늘.. 1주년 기념 여행가기로 했었지...");

	Sleep(3000);

	//두번째 대사
	Beep(1200, 200);
	move_cursor(100, 22);
	wprintf(L"이은석 교수님 수업인데.. 흠...");

	Sleep(3000);

    //세번째 대사
    Beep(1200, 200);
    move_cursor(100, 24);
    wprintf(L"영찬이한테 대리출석 부탁해야겠다.");

	_getch();

	system("cls");
	return 0;
}

int GameEX()
{
	system("cls");

	move_cursor(52, 10);
	wprintf(L"게임 설명");

	move_cursor(52, 12);
	wprintf(L"게임 설명하는 내용");

	move_cursor(52, 14);
	wprintf(L"아무키나 누르면 타이틀로 돌아갑니다.");

	_getch();

	return 0;
}

int Team()
{
	system("cls");

	move_cursor(52, 8);
	wprintf(L"팀소개");

	move_cursor(52, 10);
	wprintf(L"조건우 조장");

	move_cursor(52, 12);
	wprintf(L"이경빈 천재");

	move_cursor(52, 14);
	wprintf(L"정나라 천재");

	move_cursor(52, 16);
	wprintf(L"아무키나 누르면 타이틀로 돌아갑니다.");

	_getch();

	return 0;
}

int start_game()
{
	move_cursor(52, 10);
	wprintf(L"Alt 와 Enter를 동시에 눌러 전체화면으로 플레이 해주세요");
	Sleep(8000);

	return 0;
}

// 메인
int main()
{


	// 콘솔 핸들
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	// [추가] 콘솔 커서 숨기기 설정
	CONSOLE_CURSOR_INFO cursorInfo;
	GetConsoleCursorInfo(hConsole, &cursorInfo);
	cursorInfo.bVisible = FALSE; // 커서를 보이지 않게 설정
	SetConsoleCursorInfo(hConsole, &cursorInfo);

	// UTF16 출력
	_setmode(_fileno(stdout), _O_U16TEXT);

	// 콘솔 크기
	system("mode con cols=170 lines=60");

	// UTF8 코드페이지
	system("chcp 65001");

	// 배경 검정
	system("color 00");

	start_game();

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

		case 3:
			gameStatus = GameEX();
			break;

		case 4:
			gameStatus = Team();
			break;
		}
	}

    system("cls");

	return 0;
}
