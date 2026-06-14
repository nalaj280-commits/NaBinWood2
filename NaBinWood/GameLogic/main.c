#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <windows.h>

int main() {
    // 형님의 본 게임과 동일한 콘솔 창 크기 및 UTF-8 인코딩 설정
    system("mode con cols=170 lines=60");
    system("chcp 65001 > nul");

    // 확인하고 싶은 아스키 아트 파일명 입력 (예: art_cut.txt, art_lab.txt 등)
    const char* filename = "art_cut.txt";

    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        printf("[!] '%s' 파일을 찾을 수 없습니다. 파일이 같은 폴더에 있는지 확인해 주십시오.\n", filename);
        system("pause");
        return 1;
    }

    // 표준 출력 핸들 가져오기 (더블 버퍼링 대신 기본 콘솔 화면에 출력)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, 8); // COLOR_DARKGRAY (원하시는 색상 번호로 변경 가능)

    char utf8Buf[4096];
    wchar_t wideBuf[4096];

    // 게임 내 출력 로직과 완벽히 동일한 구조
    while (fgets(utf8Buf, sizeof(utf8Buf), fp)) {
        MultiByteToWideChar(CP_UTF8, 0, utf8Buf, -1, wideBuf, 4096);

        wchar_t* p = wideBuf;
        while (*p) {
            DWORD w;
            WriteConsoleW(hConsole, p, 1, &w, NULL);
            p++;
        }
    }

    fclose(fp);
    SetConsoleTextAttribute(hConsole, 15); // 원래 색상(흰색)으로 복구

    printf("\n\n=================================================================\n");
    printf("[출력 테스트 완료] 형태가 어긋난 곳이 없는지 확인해 주십시오.\n");
    printf("=================================================================\n");
    system("pause");

    return 0;
}