#include <stdio.h>
#include <wchar.h>
#include <locale.h>
#include <windows.h>
#include <fcntl.h>
#include <io.h>

#pragma warning(disable:4996)

HANDLE hConsole;

void setColor(int color)
{
    SetConsoleTextAttribute(hConsole, color);
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

int main()
{
    char path[MAX_PATH];

    GetCurrentDirectoryA(MAX_PATH, path);

    MessageBoxA(NULL, path, "현재 경로", MB_OK);

    // 콘솔 핸들
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    // UTF-16 출력 모드
    _setmode(_fileno(stdout), _O_U16TEXT);

    // 콘솔 설정
    system("mode con cols=170 lines=60");
    system("color 00");

    // UTF-8 텍스트 파일 읽기
    FILE* fp = fopen("art.txt", "rb");



    if (!fp)
    {
        wprintf(L"art.txt 파일을 열 수 없습니다.\n");
        system("pause");
        return 1;
    }

    // UTF-8 → UTF-16 변환용 버퍼
    char utf8Buffer[4096];
    wchar_t wideBuffer[4096];

    int line = 0;

    while (fgets(utf8Buffer, sizeof(utf8Buffer), fp))
    {
        // UTF-8 -> UTF-16 변환
        MultiByteToWideChar(
            CP_UTF8,
            0,
            utf8Buffer,
            -1,
            wideBuffer,
            4096
        );

        int x = 0;
        wchar_t* p = wideBuffer;

        while (*p)
        {
            wchar_t ch = *p;

            // 기본 흰색
            setColor(7);

            // 특정 영역 노랑색
            if (
                line >= 14 && line <= 23 &&
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

    wprintf(L"\n\n");

    system("pause");

    return 0;
}