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

// ASCII ART 출력
void draw_art()
{
    FILE* fp = fopen("art.txt", "rb");

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
    /* 게임 제목
    setColor(15);

    move_cursor(120, 18);
    wprintf(L"게임 제목");
    */

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
    system("cls");

    draw_art();

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
    setColor(10);
    move_cursor(70, 30);
    wprintf(L"게임 시작!");

    _getch();

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
    wprintf(L"아무키나 누르면 타이틀로 돌아갑니더");
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
    wprintf(L"아무키나 누르면 타이틀로 돌아갑니더");
    _getch();
    return 0;
}

// 메인
int main()
{
    // 콘솔 핸들
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    // UTF16 출력
    _setmode(_fileno(stdout), _O_U16TEXT);

    // 콘솔 크기
    system("mode con cols=170 lines=60");

    // UTF8 코드페이지
    system("chcp 65001");

    // 배경 검정
    system("color 00");

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