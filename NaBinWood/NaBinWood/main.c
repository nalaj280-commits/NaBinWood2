#include <stdio.h>
#include <wchar.h>
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <conio.h>
#include <stdlib.h>
#include <stdbool.h>

#pragma warning(disable:4996)

// --- 대저택 게임 관련 매크로 및 전역 변수 ---
#define MAP_WIDTH 40
#define MAP_HEIGHT 20
#define NUM_ROOMS 4

// 0: 빈공간, 1: 벽/가구, 2~5: 방 이동 문, 6: 숨을 수 있는 장롱(H), 7: 열쇠(K)
int world_maps[NUM_ROOMS][MAP_HEIGHT][MAP_WIDTH];

// 타이틀 및 콘솔 제어 전역 변수
HANDLE hBuffer[2];
int screenIndex = 0;

int menu = 1;
int isRunning = 1;

// --- 더블 버퍼링 제어 함수 ---
void init_double_buffer()
{
	CONSOLE_CURSOR_INFO cursorInfo;
	cursorInfo.bVisible = FALSE;
	cursorInfo.dwSize = 1;

	for (int i = 0; i < 2; i++) {
		hBuffer[i] = CreateConsoleScreenBuffer(
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			CONSOLE_TEXTMODE_BUFFER,
			NULL
		);
		SetConsoleCursorInfo(hBuffer[i], &cursorInfo);

		COORD bufferSize = { 170, 60 };
		SetConsoleScreenBufferSize(hBuffer[i], bufferSize);
	}
}

void flip_buffer()
{
	SetConsoleActiveScreenBuffer(hBuffer[screenIndex]);
	screenIndex = !screenIndex;
}

void clear_buffer()
{
	COORD coord = { 0, 0 };
	DWORD dwWritten;
	FillConsoleOutputCharacterW(hBuffer[screenIndex], L' ', 170 * 60, coord, &dwWritten);
	FillConsoleOutputAttribute(hBuffer[screenIndex], 7, 170 * 60, coord, &dwWritten);
}

void set_color_buf(int color)
{
	SetConsoleTextAttribute(hBuffer[screenIndex], color);
}

void move_cursor_buf(int x, int y)
{
	COORD pos;
	pos.X = x;
	pos.Y = y;
	SetConsoleCursorPosition(hBuffer[screenIndex], pos);
}

void print_buf(const wchar_t* format, ...)
{
	wchar_t buffer[1024];
	va_list args;
	va_start(args, format);
	vswprintf(buffer, sizeof(buffer) / sizeof(wchar_t), format, args);
	va_end(args);

	DWORD written;
	WriteConsoleW(hBuffer[screenIndex], buffer, lstrlenW(buffer), &written, NULL);
}

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

void print_align_left(const wchar_t* text, int total_width)
{
	int width = get_width(text);
	print_buf(L"%ls", text);
	for (int i = width; i < total_width; i++)
	{
		print_buf(L" ");
	}
}

// --- 맵 초기화 함수 (형님의 원래 방 배치 유지 + 열쇠 추가) ---
void initWorldMaps() {
	for (int r = 0; r < NUM_ROOMS; r++) {
		for (int i = 0; i < MAP_HEIGHT; i++) {
			for (int j = 0; j < MAP_WIDTH; j++) {
				if (i == 0 || i == MAP_HEIGHT - 1 || j == 0 || j == MAP_WIDTH - 1) {
					world_maps[r][i][j] = 1;
				}
				else {
					world_maps[r][i][j] = 0;
				}
			}
		}
	}

	// --- [Room 0: 대저택 현관 로비] ---
	for (int j = 15; j <= 24; j++) { world_maps[0][9][j] = 1; world_maps[0][10][j] = 1; }
	world_maps[0][3][5] = 1; world_maps[0][3][34] = 1;
	world_maps[0][0][20] = 2;
	world_maps[0][10][MAP_WIDTH - 1] = 5;
	world_maps[0][2][2] = 6;

	// --- [Room 1: 침실] ---
	for (int i = 3; i <= 6; i++) { for (int j = 4; j <= 8; j++) world_maps[1][i][j] = 1; }
	for (int j = 25; j <= 35; j++) world_maps[1][15][j] = 1;
	world_maps[1][MAP_HEIGHT - 1][20] = 2;
	world_maps[1][10][0] = 4;
	world_maps[1][2][37] = 6;
	world_maps[1][5][35] = 7;              // 침실 구석 열쇠 배치

	// --- [Room 2: 비밀의 서재] ---
	for (int i = 3; i <= 14; i += 3) {
		for (int j = 5; j <= 30; j++) world_maps[2][i][j] = 1;
	}
	world_maps[2][10][MAP_WIDTH - 1] = 3;
	world_maps[2][1][1] = 6;

	// --- [Room 3: 어두운 긴 복도] ---
	for (int i = 4; i <= 15; i += 4) {
		for (int j = 10; j <= 15; j++) world_maps[3][i][j] = 1;
		for (int j = 25; j <= 30; j++) world_maps[3][i + 2][j] = 1;
	}
	world_maps[3][10][0] = 2;
	world_maps[3][1][37] = 6;
}

// --- 스마트폰 연출 관련 함수들 ---
void draw_phone()
{
	int x = 40;
	int y = 3;

	move_cursor_buf(x, y);
	print_buf(L"┌─────────────────────────────────────────┐");
	for (int i = 1; i < 40; i++)
	{
		move_cursor_buf(x, y + i);
		print_buf(L"│                                         │");
	}
	move_cursor_buf(x, y + 40);
	print_buf(L"└─────────────────────────────────────────┘");
}

void draw_lock_screen()
{
	move_cursor_buf(56, 6);
	print_buf(L"6월 22일 (월)");

	move_cursor_buf(47, 9);  print_buf(L" __ ___        ______ ______\n");
	move_cursor_buf(47, 10); print_buf(L"/_ | |__ \\   _  | ____| | ____|\n");
	move_cursor_buf(47, 11); print_buf(L" | |   ) | (_) | |__   | |__\n");
	move_cursor_buf(47, 12); print_buf(L" | |  / /      |___ \\  |___ \\ \n");
	move_cursor_buf(47, 13); print_buf(L" | | / /_   _   ___) |  ___) |\n");
	move_cursor_buf(47, 14); print_buf(L" |_| |____| (_) |____/  |____/\n");
}

void draw_notification(const wchar_t* name, const wchar_t* msg)
{
	int x = 45;
	int y = 32;

	for (int i = 0; i < 4; i++)
	{
		move_cursor_buf(x, y + i);
		print_buf(L"                                ");
	}

	move_cursor_buf(x, y);
	print_buf(L"┌──────────────────────────────┐");
	move_cursor_buf(x, y + 1);
	print_buf(L"│ ♥ "); print_align_left(name, 24); print_buf(L"  │");
	move_cursor_buf(x, y + 2);
	print_buf(L"│ "); print_align_left(msg, 29); print_buf(L"│");
	move_cursor_buf(x, y + 3);
	print_buf(L"└──────────────────────────────┘");
}

void draw_art()
{
	FILE* fp = fopen("art_title.txt", "rb");
	if (!fp) return;

	char utf8Buffer[4096];
	wchar_t wideBuffer[4096];
	int line = 0;

	while (fgets(utf8Buffer, sizeof(utf8Buffer), fp))
	{
		MultiByteToWideChar(CP_UTF8, 0, utf8Buffer, -1, wideBuffer, 4096);
		move_cursor_buf(0, line);
		int x = 0;
		wchar_t* p = wideBuffer;
		while (*p)
		{
			wchar_t ch = *p;
			set_color_buf(8);
			if (line >= 14 && line <= 24 && x >= 56 && x <= 121 && ch != L' ') set_color_buf(6);
			DWORD written;
			WriteConsoleW(hBuffer[screenIndex], &ch, 1, &written, NULL);
			p++; x++;
		}
		line++;
	}
	fclose(fp);
	set_color_buf(7);
}

void draw_menu()
{
	if (menu == 1) set_color_buf(14); else set_color_buf(15);
	move_cursor_buf(122, 23); print_buf(L"1. 게임 시작");
	if (menu == 2) set_color_buf(14); else set_color_buf(15);
	move_cursor_buf(122, 27); print_buf(L"2. 게임 설명");
	if (menu == 3) set_color_buf(14); else set_color_buf(15);
	move_cursor_buf(122, 31); print_buf(L"3. 제작자");
	if (menu == 4) set_color_buf(14); else set_color_buf(15);
	move_cursor_buf(122, 35); print_buf(L"4. 종료");
	set_color_buf(7);
}

int RenderTitle()
{
	clear_buffer();
	draw_art();
	draw_menu();
	flip_buffer();

	int ch = _getch();
	if (ch == 0 || ch == 224)
	{
		ch = _getch();
		switch (ch)
		{
		case 72: if (menu > 1) menu--; break;
		case 80: if (menu < 4) menu++; break;
		}
	}
	else if (ch == 32)
	{
		if (menu == 1) return 2;
		else if (menu == 2) return 3;
		else if (menu == 3) return 4;
		else if (menu == 4) isRunning = 0;
	}
	return 0;
}

// --- 오리지널 연출 틀 유지 + 버그 수정된 인게임 루프 ---
int MainGame()
{
	// ================= [1단계: 형님이 보내주신 원래 인트로 연출 틀 유지] =================
	clear_buffer(); draw_phone(); draw_lock_screen(); flip_buffer();
	Sleep(1000); Beep(1200, 200);

	clear_buffer(); draw_phone(); draw_lock_screen();
	draw_notification(L"여자친구", L"자기야 오늘 무슨 날인지 알지?"); flip_buffer();
	Sleep(2500); Beep(1000, 200);

	clear_buffer(); draw_phone(); draw_lock_screen();
	draw_notification(L"여자친구", L"자기야 오늘 무슨 날인지 알지?");
	draw_notification(L"여자친구", L"나 오늘 이쁘게 입고 갈게!!"); flip_buffer();
	Sleep(2500); Beep(700, 400);

	clear_buffer(); draw_phone(); draw_lock_screen();
	draw_notification(L"여자친구", L"나 오늘 이쁘게 입고 갈게!!");
	draw_notification(L"여자친구", L"터미널에 2시까지 와야해!!"); flip_buffer();
	Sleep(3000); Beep(1200, 200);

	// 우측 대사 연출들도 지워지지 않고 순차적으로 남도록 누적 백버퍼 빌드
	clear_buffer(); draw_phone(); draw_lock_screen(); draw_notification(L"여자친구", L"터미널에 2시까지 와야해!!");
	move_cursor_buf(100, 20); print_buf(L"아... 오늘.. 1주년 기념 여행가기로 했었지..."); flip_buffer();
	Sleep(3000); Beep(1200, 200);

	clear_buffer(); draw_phone(); draw_lock_screen(); draw_notification(L"여자친구", L"터미널에 2시까지 와야해!!");
	move_cursor_buf(100, 20); print_buf(L"아... 오늘.. 1주년 기념 여행가기로 했었지...");
	move_cursor_buf(100, 22); print_buf(L"이은석 교수님 수업인데.. 흠..."); flip_buffer();
	Sleep(3000); Beep(1200, 200);

	clear_buffer(); draw_phone(); draw_lock_screen(); draw_notification(L"여자친구", L"터미널에 2시까지 와야해!!");
	move_cursor_buf(100, 20); print_buf(L"아... 오늘.. 1주년 기념 여행가기로 했었지...");
	move_cursor_buf(100, 22); print_buf(L"이은석 교수님 수업인데.. 흠...");
	move_cursor_buf(100, 24); print_buf(L"영찬이한테 대리출석 부탁해야겠다."); flip_buffer();

	_getch(); // 키 입력 시 인게임 진입


	// ================= [2단계: 대저택 탈출 게임 엔진] =================
	initWorldMaps();

	int currentRoom = 0;
	int prevRoom = 0;
	int px = 20, py = 14;

	int mx = 30, my = 5;
	bool bossActive = true;
	int bossFollowTimer = 0;
	int roomChangeCountAfterHide = 0;

	bool isHidden = false;
	bool spacePressed = false;
	bool gameOver = false;
	bool gameClear = false;

	int hasKey = 0; // 열쇠 변수 복구

	int playerMoveTurn = 0;
	int monsterMoveTurn = 0;

	while (!gameOver && !gameClear) {
		clear_buffer();

		int (*currentMap)[MAP_WIDTH] = world_maps[currentRoom];

		// 열쇠 획득 판정
		if (currentMap[py][px] == 7) {
			hasKey = 1;
			currentMap[py][px] = 0;
		}

		// 상단 UI 렉 없이 동기화
		move_cursor_buf(0, 0);
		print_buf(L"+-------------------------------------------------------------------------------+\n");
		char* roomNames[] = { "Main Lobby (Room 0)", "Cozy Bedroom (Room 1)", "Secret Library (Room 2)", "Dark Hallway (Room 3)" };
		print_buf(L"| Room: %-22hs | Key: [ %d / 1 ] | Status: %-18hs |\n", roomNames[currentRoom], hasKey, isHidden ? "HIDING IN CLOSET" : "SURVIVING...");
		print_buf(L"+-------------------------------------------------------------------------------+\n");

		// 맵 그리기 엔진 (문 'D' 튀어나오게 처리)
		for (int i = 0; i < MAP_HEIGHT; i++) {
			for (int j = 0; j < MAP_WIDTH; j++) {

				// 문(D)이 벽과 겹치지 않고 무조건 위로 튀어나오도록 최우선 처리
				if (currentMap[i][j] >= 2 && currentMap[i][j] <= 5) {
					set_color_buf(11); print_buf(L"D "); set_color_buf(7);
				}
				else if (i == py && j == px && !isHidden) {
					set_color_buf(14); print_buf(L"P "); set_color_buf(7);
				}
				else if (i == my && j == mx && bossActive) {
					set_color_buf(12); print_buf(L"M "); set_color_buf(7);
				}
				else if (currentMap[i][j] == 7) { // 열쇠 렌더링
					set_color_buf(13); print_buf(L"K "); set_color_buf(7);
				}
				else if (currentMap[i][j] == 6) {
					if (i == py && j == px && isHidden) print_buf(L"  ");
					else { set_color_buf(10); print_buf(L"H "); set_color_buf(7); }
				}
				else if (currentMap[i][j] == 1) {
					if (i == 0 || i == MAP_HEIGHT - 1 || j == 0 || j == MAP_WIDTH - 1) print_buf(L"# ");
					else print_buf(L"X ");
				}
				else {
					print_buf(L"  ");
				}
			}
			print_buf(L"\n");
		}

		if (bossActive && !isHidden && px == mx && py == my) {
			gameOver = true;
			break;
		}

		// 스페이스바 숨기기 조작
		if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
			if (!spacePressed) {
				if (currentMap[py][px] == 6) {
					isHidden = !isHidden;
				}
				spacePressed = true;
			}
		}
		else {
			spacePressed = false;
		}

		// 플레이어 이동
		if (!isHidden) {
			playerMoveTurn++;
			if (playerMoveTurn >= 2) {
				int nextX = px;
				int nextY = py;

				if (GetAsyncKeyState(VK_UP) & 0x8000)    nextY--;
				if (GetAsyncKeyState(VK_DOWN) & 0x8000)  nextY++;
				if (GetAsyncKeyState(VK_LEFT) & 0x8000)  nextX--;
				if (GetAsyncKeyState(VK_RIGHT) & 0x8000) nextX++;

				if (currentMap[nextY][nextX] != 1) {
					px = nextX;
					py = nextY;
				}
				playerMoveTurn = 0;
			}
		}

		if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
			gameOver = true;
			break;
		}

		// 워프 및 열쇠 잠금 문 기믹 적용
		int tileValue = currentMap[py][px];
		if (tileValue >= 2 && tileValue <= 5 && !isHidden) {

			// 로비(Room 0)의 북쪽 문 탈출 시 열쇠 검사 틀 작동
			if (currentRoom == 0 && tileValue == 2) {
				if (hasKey == 1) {
					gameClear = true;
					break;
				}
				else {
					py += 1;
					continue;
				}
			}

			prevRoom = currentRoom;
			int targetRoom = tileValue - 2;

			if (bossActive) {
				bossFollowTimer = 40;
				bossActive = false;
			}

			if (!bossActive && bossFollowTimer == 0) {
				roomChangeCountAfterHide++;
				if (roomChangeCountAfterHide >= 3) {
					bossActive = true;
					mx = MAP_WIDTH - 2; my = 2;
					roomChangeCountAfterHide = 0;
				}
			}

			if (currentRoom == 0 && targetRoom == 1) { px = 20; py = MAP_HEIGHT - 3; }
			else if (currentRoom == 1 && targetRoom == 0) { px = 20; py = 2; }
			else if (currentRoom == 1 && targetRoom == 2) { px = MAP_WIDTH - 3; py = 10; }
			else if (currentRoom == 2 && targetRoom == 1) { px = 2; py = 10; }
			else if (currentRoom == 0 && targetRoom == 3) { px = 2; py = 10; }
			else if (currentRoom == 3 && targetRoom == 0) { px = MAP_WIDTH - 3; py = 10; }

			currentRoom = targetRoom;
			continue;
		}

		// 괴물 AI 및 타이머 제어
		if (!bossActive && bossFollowTimer > 0) {
			bossFollowTimer--;
			if (bossFollowTimer == 0) {
				bossActive = true;
				if (currentRoom == 1 && prevRoom == 0) { mx = 20; my = MAP_HEIGHT - 1; }
				else if (currentRoom == 0 && prevRoom == 1) { mx = 20; my = 0; }
				else if (currentRoom == 2 && prevRoom == 1) { mx = MAP_WIDTH - 1; my = 10; }
				else if (currentRoom == 1 && prevRoom == 2) { mx = 0; my = 10; }
				else if (currentRoom == 3 && prevRoom == 0) { mx = 0; my = 10; }
				else if (currentRoom == 0 && prevRoom == 3) { mx = MAP_WIDTH - 1; my = 10; }
			}
		}

		if (bossActive) {
			monsterMoveTurn++;
			if (monsterMoveTurn >= 3) {
				int targetX = mx;
				int targetY = my;

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
						roomChangeCountAfterHide = 0;
						mx = -10; my = -10;
					}
				}

				if (currentMap[targetY][targetX] != 1) {
					mx = targetX;
					my = targetY;
				}
				else {
					if (currentMap[my][targetX] != 1) mx = targetX;
					else if (currentMap[targetY][mx] != 1) my = targetY;
				}
				monsterMoveTurn = 0;
			}
		}

		flip_buffer();
		Sleep(30);
	}

	// 결과 화면 출력
	clear_buffer();
	move_cursor_buf(0, 0);
	if (gameClear) {
		print_buf(L"\n\n\n\n\t[ GAME CLEAR !! ]\n");
		print_buf(L"\t열쇠를 사용하여 무사히 저택에서 대리출석(?)과 탈출에 성공했습니다!\n\n\n");
	}
	else {
		print_buf(L"\n\n\n\n\t[ GAME OVER ]\n");
		print_buf(L"\t대저택에서 탈출하지 못하고 잡혔습니다...\n\n\n");
	}
	print_buf(L"\t아무 키나 누르면 메인 화면으로 돌아갑니다.");
	flip_buffer();
	_getch();

	return 0;
}

int GameEX()
{
	clear_buffer();
	move_cursor_buf(52, 10);  print_buf(L"게임 설명");
	move_cursor_buf(52, 12);  print_buf(L"방향키 위/아래로 메뉴를 이동하고 Space바로 선택합니다.");
	move_cursor_buf(52, 14);  print_buf(L"인게임에서 방향키로 이동하며, 침실(Room 1)에서 열쇠(K)를 찾아");
	move_cursor_buf(52, 16);  print_buf(L"로비(Room 0)의 북쪽 탈출구 문(D)으로 나가면 클리어입니다.");
	move_cursor_buf(52, 18);  print_buf(L"아무키나 누르면 타이틀로 돌아갑니다.");
	flip_buffer();
	_getch();
	return 0;
}

int Team()
{
	clear_buffer();
	move_cursor_buf(52, 8);   print_buf(L"팀소개");
	move_cursor_buf(52, 10);  print_buf(L"조건우 조장");
	move_cursor_buf(52, 12);  print_buf(L"이경빈 천재");
	move_cursor_buf(52, 14);  print_buf(L"정나라 천재");
	move_cursor_buf(52, 16);  print_buf(L"아무키나 누르면 타이틀로 돌아갑니다.");
	flip_buffer();
	_getch();
	return 0;
}

int start_game()
{
	clear_buffer();
	move_cursor_buf(52, 10);
	print_buf(L"Alt 와 Enter를 동시에 눌러 전체화면으로 플레이 해주세요");
	flip_buffer();
	Sleep(3000);
	return 0;
}

int main()
{
	_setmode(_fileno(stdout), _O_U16TEXT);

	system("mode con cols=170 lines=60");
	system("chcp 65001");
	system("color 00");

	init_double_buffer();

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

	return 0;
}