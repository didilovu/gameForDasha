#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <math.h>
#include <stdio.h>
#include <windows.h>
#define W 960
#define H 640
#include "game.h"
static Flower flowers[N];
static float px = 480, py = 355, walk = 0, pour = 0;
static int facing = 1, watered = 0;
static DWORD previous;
static Progress progress;
static wchar_t save_path[MAX_PATH], temp_path[MAX_PATH];
static int congratulations = 0, save_failed = 0, combo_count = 0;
static float combo_time = 0, pulse_clock = 0;
static int f_down = 0, shop_open = 0, reset_prompt = 0;
static wchar_t shop_message[100] = L"Выбери вещь — покупка сразу надевается!";
static void save_game(void) {
  if (!save_path[0]) {
    save_failed = 1;
    return;
  }
  FILE *file = _wfopen(temp_path, L"wb");
  if (!file) {
    save_failed = 1;
    return;
  }
  int ok = fwrite(&progress, sizeof(progress), 1, file) == 1;
  if (fclose(file) != 0)
    ok = 0;
  save_failed =
      !(ok && MoveFileExW(temp_path, save_path,
                          MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH));
}
static void load_game(void) {
  new_progress(&progress);
  wchar_t dir[MAX_PATH];
  DWORD len = GetEnvironmentVariableW(L"LOCALAPPDATA", dir, MAX_PATH);
  if (!len || len > MAX_PATH - 60) {
    save_failed = 1;
    return;
  }
  lstrcatW(dir, L"\\FlowerGarden");
  if (!CreateDirectoryW(dir, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
    save_failed = 1;
    return;
  }
  swprintf(save_path, MAX_PATH, L"%ls\\progress.dat", dir);
  swprintf(temp_path, MAX_PATH, L"%ls\\progress.tmp", dir);
  FILE *file = _wfopen(save_path, L"rb");
  if (file) {
    Progress loaded;
    unsigned char data[sizeof(Progress) + 1];
    size_t size = fread(data, 1, sizeof(data), file);
    int ok = decode_progress(&loaded, data, size);
    fclose(file);
    if (ok && valid_progress(&loaded))
      progress = loaded;
    else
      save_failed = 1;
  }
}
static COLORREF palette[] = {0x00839FF4, 0x00BB8AE1, 0x006CD8FF,
                             0x00D9B78C, 0x00A7A1FF, 0x00E6D9FF};
static void oval(HDC dc, int x, int y, int w, int h, COLORREF c) {
  HBRUSH b = CreateSolidBrush(c);
  HGDIOBJ old = SelectObject(dc, b);
  Ellipse(dc, x, y, x + w, y + h);
  SelectObject(dc, old);
  DeleteObject(b);
}
static void box(HDC dc, int x, int y, int w, int h, COLORREF c) {
  RECT r = {x, y, x + w, y + h};
  HBRUSH b = CreateSolidBrush(c);
  FillRect(dc, &r, b);
  DeleteObject(b);
}
static void line(HDC dc, int x, int y, int x2, int y2, int width, COLORREF c) {
  HPEN p = CreatePen(PS_SOLID, width, c);
  HGDIOBJ old = SelectObject(dc, p);
  MoveToEx(dc, x, y, NULL);
  LineTo(dc, x2, y2);
  SelectObject(dc, old);
  DeleteObject(p);
}
static void label(HDC dc, int x, int y, const wchar_t *s, int size,
                  COLORREF c) {
  HFONT f = CreateFontW(size, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0,
                        0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
  HGDIOBJ old = SelectObject(dc, f);
  SetTextColor(dc, c);
  SetBkMode(dc, TRANSPARENT);
  TextOutW(dc, x, y, s, lstrlenW(s));
  SelectObject(dc, old);
  DeleteObject(f);
}
static void init(void) {
  int centers[6][2] = {{190, 210}, {450, 180}, {745, 215},
                       {215, 465}, {500, 470}, {770, 465}};
  for (int g = 0; g < 6; g++)
    for (int j = 0; j < 12; j++) {
      int i = g * 12 + j;
      flowers[i] = (Flower){centers[g][0] + (j % 4) * 30 - 45 + (j / 4 % 2) * 9,
                            centers[g][1] + (j / 4) * 30 - 30,
                            progress.wet[progress.location][i], g};
    }
  watered = count_watered(&progress);
  px = 480;
  py = 355;
  pour = 0;
  pulse_clock = 0;
  f_down = 0;
}
static void hat_art(HDC dc, int x, int y, int style) {
  if (!style)
    return;
  COLORREF c = style == 1   ? RGB(224, 184, 103)
               : style == 2 ? RGB(213, 151, 170)
                            : RGB(117, 102, 163);
  oval(dc, x - 23, y + 9, 48, 13, RGB(105, 89, 64));
  oval(dc, x - 25, y + 5, 50, 13, c);
  if (style == 3) {
    POINT triangle[] = {{x - 14, y + 9}, {x + 12, y + 9}, {x + 6, y - 24}};
    HBRUSH brush = CreateSolidBrush(c);
    HGDIOBJ old = SelectObject(dc, brush);
    Polygon(dc, triangle, 3);
    SelectObject(dc, old);
    DeleteObject(brush);
    oval(dc, x + 2, y - 10, 5, 5, RGB(255, 230, 143));
  } else {
    oval(dc, x - 16, y - 10, 32, 26, c);
    line(dc, x - 13, y + 6, x + 13, y + 6, 5,
         style == 1 ? RGB(155, 102, 72) : RGB(165, 88, 121));
    if (style == 2) {
      oval(dc, x + 10, y - 2, 11, 11, RGB(255, 237, 190));
      oval(dc, x + 14, y + 1, 4, 4, RGB(217, 160, 73));
    }
    if (style == 1)
      for (int i = -10; i <= 10; i += 5)
        line(dc, x + i, y - 5, x + i - 3, y + 2, 1, RGB(242, 210, 137));
  }
}
static COLORREF can_color(int style) {
  return style == 1   ? RGB(214, 147, 89)
         : style == 2 ? RGB(164, 146, 214)
                      : RGB(102, 171, 175);
}
static void can_art(HDC dc, int x, int y, int style, int direction) {
  COLORREF c = can_color(style);
  oval(dc, x - 12, y - 18, 24, 19, RGB(70, 104, 109));
  oval(dc, x - 8, y - 14, 16, 11, RGB(196, 220, 210));
  box(dc, x - 13, y - 7, 26, 18, c);
  oval(dc, x - 13, y + 4, 26, 10, c);
  line(dc, x - 9, y - 3, x - 9, y + 7, 2, RGB(242, 219, 185));
  line(dc, x + direction * 8, y + 2, x + direction * 27, y - 9, 6, c);
  line(dc, x + direction * 24, y - 12, x + direction * 29, y - 7, 4, c);
  if (style == 2) {
    oval(dc, x - 3, y - 4, 7, 7, RGB(255, 235, 164));
    line(dc, x, y - 7, x, y + 6, 1, RGB(255, 235, 164));
  }
}
static int near_shop(void) {
  float dx = px - 853, dy = py - 552;
  return dx * dx + dy * dy < 125 * 125;
}
static void stall(HDC dc) {
  oval(dc, 787, 575, 156, 23, RGB(107, 136, 97));
  box(dc, 804, 493, 5, 95, RGB(110, 77, 56));
  box(dc, 923, 493, 5, 95, RGB(110, 77, 56));
  oval(dc, 851, 508, 27, 31, RGB(238, 197, 157));
  oval(dc, 849, 501, 31, 15, RGB(110, 80, 59));
  box(dc, 846, 537, 38, 33, RGB(104, 135, 113));
  oval(dc, 858, 521, 3, 3, RGB(61, 55, 47));
  oval(dc, 869, 521, 3, 3, RGB(61, 55, 47));
  line(dc, 860, 532, 870, 532, 2, RGB(162, 99, 79));
  box(dc, 795, 554, 144, 34, RGB(155, 104, 68));
  box(dc, 791, 550, 152, 9, RGB(207, 159, 100));
  for (int i = 0; i < 4; i++) {
    line(dc, 799, 565 + i * 6, 935, 565 + i * 6, 1, RGB(125, 83, 55));
  }
  box(dc, 795, 480, 144, 17, RGB(246, 222, 177));
  for (int i = 0; i < 8; i++)
    box(dc, 795 + i * 18, 480, 9, 17, RGB(159, 103, 112));
  label(dc, 808, 460, L"Лавка садовника", 16, RGB(68, 91, 63));
  hat_art(dc, 818, 532, 1);
  can_art(dc, 911, 540, 2, 1);
  if (near_shop())
    label(dc, 782, 592, L"E — поговорить", 16, RGB(51, 83, 57));
}
static void shop_panel(HDC dc) {
  box(dc, 139, 117, 690, 475, RGB(79, 103, 77));
  box(dc, 146, 124, 676, 461, RGB(255, 246, 223));
  label(dc, 175, 142, L"Лавка садовника", 29, RGB(68, 91, 63));
  wchar_t text[140];
  swprintf(text, 140, L"Листики: %d", progress.leaves);
  label(dc, 615, 150, text, 21, RGB(68, 91, 63));
  const wchar_t *names[] = {L"Соломенная шляпка", L"Панама с ромашкой",
                            L"Звёздная шляпка", L"Медная лейка",
                            L"Лунная лейка"};
  for (int i = 0; i < SHOP_ITEMS; i++) {
    int y = 197 + i * 59, index = i < 3 ? i + 1 : i - 2;
    unsigned mask = i < 3 ? progress.hats : progress.cans;
    int equipped = (i < 3 ? progress.hat : progress.can) == index;
    box(dc, 166, y - 2, 634, 54,
        i % 2 ? RGB(242, 231, 205) : RGB(247, 237, 216));
    if (i < 3)
      hat_art(dc, 208, y + 18, index);
    else
      can_art(dc, 208, y + 21, index, 1);
    swprintf(text, 140, L"%d · %ls", i + 1, names[i]);
    label(dc, 248, y + 8, text, 20, RGB(68, 91, 63));
    if (equipped)
      lstrcpyW(text, L"Надето");
    else if (mask & (1u << index))
      lstrcpyW(text, L"Выбрать бесплатно");
    else
      swprintf(text, 140, L"%d листиков", item_prices[i]);
    label(dc, 596, y + 11, text, 17, RGB(110, 111, 81));
  }
  label(dc, 176, 501, shop_message, 18, RGB(110, 98, 80));
  label(dc, 176, 534,
        L"1–5 — купить / надеть · 6 — снять шляпку · 7 — обычная лейка", 16,
        RGB(88, 104, 78));
  label(dc, 176, 557, L"E или Esc — вернуться в сад · Все вещи сохраняются", 16,
        RGB(88, 104, 78));
}
static void girl(HDC dc) {
  int x = (int)px, y = (int)py, b = (int)(sin(walk) * 3);
  oval(dc, x - 21, y + 1, 44, 13, RGB(93, 139, 93));
  line(dc, x - 8, y - 12, x - 9 + (int)(sin(walk) * 5), y + 4, 7,
       RGB(93, 70, 66));
  line(dc, x + 8, y - 12, x + 9 - (int)(sin(walk) * 5), y + 4, 7,
       RGB(93, 70, 66));
  oval(dc, x - 19, y - 70 + b, 38, 43, RGB(241, 196, 83));
  oval(dc, x - 13, y - 65 + b, 26, 28, RGB(255, 222, 187));
  oval(dc, x - 16, y - 69 + b, 31, 13, RGB(255, 218, 111));
  oval(dc, x - 23, y - 49 + b, 12, 23, RGB(241, 196, 83));
  oval(dc, x + 12, y - 49 + b, 12, 23, RGB(241, 196, 83));
  POINT dress[] = {{x - 10, y - 38 + b},
                   {x + 10, y - 38 + b},
                   {x + 21, y - 10},
                   {x - 21, y - 10}};
  HBRUSH br = CreateSolidBrush(RGB(217, 134, 155));
  HGDIOBJ old = SelectObject(dc, br);
  Polygon(dc, dress, 4);
  SelectObject(dc, old);
  DeleteObject(br);
  oval(dc, x - 5, y - 54 + b, 3, 4, RGB(80, 70, 65));
  oval(dc, x + 5, y - 54 + b, 3, 4, RGB(80, 70, 65));
  line(dc, x - 2, y - 44 + b, x + 3, y - 44 + b, 1, RGB(187, 109, 105));
  hat_art(dc, x, y - 72 + b, progress.hat);
  int cx = x + facing * 30;
  line(dc, x + facing * 12, y - 32 + b, cx, y - 26, 7, RGB(255, 222, 187));
  can_art(dc, cx, y - 21, progress.can, facing);
  if (pour > 0)
    for (int k = 0; k < 8; k++) {
      int t = ((int)(pour * 220) + k * 7) % 45;
      oval(dc, cx + facing * (27 + t / 3), y - 27 + t, 3, 6,
           RGB(168, 225, 245));
    }
}
static void scenery(HDC dc) {
  for (int i = 0; i < 600; i++) {
    int x = 14 + (i * 137 % 932), y = 140 + (i * 79 % 461);
    if ((y > 289 && y < 365) || (x > 435 && x < 520))
      continue;
    COLORREF c = progress.location ? RGB(125, 161, 145) : RGB(137, 175, 112);
    line(dc, x, y, x - 3, y - 6, 1, c);
    line(dc, x, y, x + 2, y - 9, 1, c);
    if (i % 5 == 0)
      line(dc, x, y, x + 5, y - 4, 1, RGB(211, 227, 161));
  }
  for (int i = 0; i < 150; i++) {
    int x = i * 89 % 950, y = 300 + i * 37 % 54;
    oval(dc, x, y, 3 + i % 5, 2, RGB(195, 178, 145));
  }
  for (int i = 0; i < 12; i++) {
    int y = 151 + i * 38;
    oval(dc, 451, y + 4, 49, 23, RGB(189, 174, 142));
    oval(dc, 449, y, 49, 23, RGB(221, 210, 183));
    line(dc, 458, y + 4, 480, y + 3, 1, RGB(243, 232, 202));
  }
  box(dc, 0, 123, W, 7, RGB(137, 158, 108));
  for (int i = 0; i < 24; i++) {
    int x = 16 + i * 40;
    box(dc, x + 4, 96, 13, 37, RGB(147, 118, 79));
    box(dc, x, 92, 12, 36, RGB(232, 208, 163));
    line(dc, x + 3, 98, x + 3, 123, 1, RGB(250, 230, 185));
    line(dc, x + 8, 99, x + 8, 124, 1, RGB(190, 156, 112));
    oval(dc, x + 4, 109, 3, 3, RGB(115, 92, 69));
  }
  for (int r = 0; r < 2; r++) {
    box(dc, 0, 102 + r * 16, W, 6, RGB(170, 135, 91));
    box(dc, 0, 102 + r * 16, W, 2, RGB(246, 224, 179));
  }
  for (int i = 0; i < 11; i++) {
    int x = i < 6 ? 23 : 906, y = 160 + (i % 6) * 76;
    oval(dc, x - 9, y + 9, 45, 22, RGB(117, 146, 95));
    oval(dc, x - 10, y - 4, 39, 30, RGB(90, 136, 88));
    oval(dc, x - 8, y - 8, 31, 25, RGB(123, 162, 99));
    oval(dc, x + 2, y - 5, 15, 11, RGB(154, 186, 117));
    for (int j = 0; j < 3; j++)
      oval(dc, x + j * 7, y + 5, 4, 4, RGB(222, 148, 139));
  }
  for (int i = 0; i < 14; i++) {
    int x = 60 + i * 63, y = 573 + i % 3 * 8;
    oval(dc, x, y + 3, 17, 8, RGB(134, 157, 109));
    oval(dc, x, y, 15, 9, RGB(174, 177, 156));
    line(dc, x + 3, y + 2, x + 9, y + 1, 1, RGB(213, 211, 183));
  }
}
static void render(HDC dc) {
  SelectObject(dc, GetStockObject(NULL_PEN));
  box(dc, 0, 0, W, H,
      (progress.location ? RGB(172, 198, 207) : RGB(196, 218, 164)));
  for (int y = 110; y < H; y += 22)
    for (int x = 30; x < W; x += 31)
      oval(dc, x + (y % 7), y, 3, 4, RGB(174, 199, 140));
  box(dc, 0, 295, W, 65, RGB(228, 215, 180));
  box(dc, 440, 110, 75, H - 110, RGB(228, 215, 180));
  for (int i = 0; i < 24; i++) {
    int x = 20 + i * 40;
    box(dc, x, 95, 9, 37, RGB(246, 235, 207));
  }
  box(dc, 0, 105, W, 6, RGB(246, 235, 207));
  box(dc, 0, 122, W, 5, RGB(246, 235, 207));
  scenery(dc);
  for (int i = 0; i < 6; i++) {
    Flower *f = &flowers[i * 12];
    oval(dc, (int)f->x - 30, (int)f->y - 7, 169, 118, RGB(127, 151, 101));
    oval(dc, (int)f->x - 28, (int)f->y - 13, 165, 116, RGB(161, 133, 97));
    oval(dc, (int)f->x - 22, (int)f->y - 9, 153, 105, RGB(184, 156, 115));
    for (int k = 0; k < 45; k++) {
      int sx = (int)f->x - 4 + k * 37 % 113, sy = (int)f->y + 3 + k * 19 % 68;
      oval(dc, sx, sy, 3, 2, RGB(155, 129, 92));
    }
  }
  for (int i = 0; i < N; i++) {
    Flower *f = &flowers[i];
    int x = (int)f->x, y = (int)f->y;
    oval(dc, x - 7, y - 2, 19, 6, RGB(126, 130, 88));
    float sway = sinf((float)GetTickCount64() / 1200 + i) * 2;
    line(dc, x, y, x + (int)sway, y - 23, 2, RGB(82, 135, 78));
    oval(dc, x - 9, y - 13, 9, 5, RGB(101, 157, 84));
    oval(dc, x + 1, y - 17, 10, 5, RGB(101, 157, 84));
    x += (int)sway;
    y -= 25;
    int r = f->water > 0 ? 7 : 6;
    int petals = progress.location ? 8 : 5;
    for (int k = 0; k < petals; k++) {
      float a = k * 6.2831853f / petals;
      oval(dc, x + (int)(cos(a) * 6) - r, y + (int)(sin(a) * 6) - r, r * 2,
           r * 2, palette[f->color]);
    }
    if (progress.location) {
      oval(dc, x - 7, y - 7, 14, 14, RGB(248, 243, 210));
      oval(dc, x - 3, y - 3, 6, 6, RGB(146, 111, 187));
      line(dc, x - 14, y - 17, x - 14, y - 25, 1, RGB(244, 238, 204));
      line(dc, x - 18, y - 21, x - 10, y - 21, 1, RGB(244, 238, 204));
    } else
      oval(dc, x - 4, y - 4, 8, 8, RGB(255, 226, 124));
    if (f->water > 0) {
      oval(dc, x + 10, y - 17, 4, 7, RGB(110, 189, 222));
    }
  }
  stall(dc);
  girl(dc);
  box(dc, 0, 0, W, 83, RGB(249, 243, 225));
  label(dc, 29, 13,
        (progress.location ? L"Лунный сад · звёздные цветы"
                           : L"Маленький цветочный сад"),
        25, RGB(78, 100, 73));
  label(dc, 30, 49,
        L"WASD — гулять · F — полив · E — лавка · L — сад · Tab — перейти · "
        L"Esc — выход",
        16, RGB(115, 124, 101));
  wchar_t count[64];
  swprintf(count, 64, L"Полито: %d / %d", watered, N);
  label(dc, 785, 12, count, 18, RGB(78, 100, 73));
  swprintf(count, 64, L"Листики: %d", progress.leaves);
  label(dc, 785, 36, count, 18, RGB(78, 100, 73));
  box(dc, 0, 610, W, 30, RGB(249, 243, 225));
  label(dc, 24, 615,
        progress.unlocked ? L"Лунный сад открыт! Tab — сменить сад"
                          : L"Лунный сад: 30 листиков · Комбо: 2+ новых цветка "
                            L"за один полив → +2 листика",
        16, RGB(78, 100, 73));
  label(dc, 775, 615, L"F2 — новая игра", 16, RGB(78, 100, 73));
  if (combo_time > 0) {
    swprintf(count, 64, L"КОМБО ×%d! +2 листика", combo_count);
    label(dc, 340, 100, count, 24, RGB(51, 97, 81));
  }
  if (save_failed)
    label(dc, 24, 584, L"Не удалось сохранить прогресс на диск", 17,
          RGB(155, 60, 60));
  if (shop_open)
    shop_panel(dc);
  if (congratulations) {
    box(dc, 170, 180, 620, 290, RGB(112, 143, 104));
    box(dc, 178, 188, 604, 274, RGB(255, 248, 229));
    label(dc, 260, 210, L"Поздравляем! Весь сад полит!", 28, RGB(78, 100, 73));
    label(dc, 235, 258,
          progress.location ? L"Звёздные цветы сияют благодаря тебе!"
                            : L"Каждый цветочек получил свою капельку заботы.",
          20, RGB(100, 116, 90));
    swprintf(count, 64, L"Твои листики: %d", progress.leaves);
    label(dc, 360, 298, count, 23, RGB(78, 100, 73));
    label(dc, 220, 341,
          progress.unlocked ? L"Tab — перейти в другой сад"
                            : L"L — купить лунный сад за 30 листиков",
          21, RGB(78, 100, 73));
    label(dc, 220, 380, L"R — вырастить новые цветы и заработать ещё", 20,
          RGB(100, 116, 90));
    label(dc, 220, 417, L"Enter — вернуться в сад", 18, RGB(100, 116, 90));
  }
}
static void update(HWND hwnd) {
  DWORD now = GetTickCount();
  float dt = (now - previous) / 1000.f;
  previous = now;
  if (dt > .05f)
    dt = .05f;
  if (GetForegroundWindow() != hwnd)
    return;
  if (combo_time > 0)
    combo_time -= dt;
  if (congratulations || shop_open || reset_prompt)
    return;
  float dx = 0, dy = 0;
  if (GetAsyncKeyState('W') & 0x8000)
    dy--;
  if (GetAsyncKeyState('S') & 0x8000)
    dy++;
  if (GetAsyncKeyState('A') & 0x8000)
    dx--;
  if (GetAsyncKeyState('D') & 0x8000)
    dx++;
  if (dx || dy) {
    float length = sqrtf(dx * dx + dy * dy);
    px += dx / length * 155 * dt;
    py += dy / length * 155 * dt;
    walk += dt * 10;
    if (dx)
      facing = dx > 0 ? 1 : -1;
  } else
    walk = 0;
  if (px < 40)
    px = 40;
  if (px > 920)
    px = 920;
  if (py < 180)
    py = 180;
  if (py > 600)
    py = 600;
  if (px > 790 && py > 528) {
    py = 528;
  }
  int down = (GetAsyncKeyState('F') & 0x8000) != 0;
  if (down) {
    pour += dt;
    pulse_clock -= dt;
    if (!f_down || pulse_clock <= 0) {
      pulse_clock = .4f;
      int n = water_pulse(&progress, flowers, px, py);
      if (n >= 2) {
        combo_count = n;
        combo_time = 2.0f;
      }
      watered = count_watered(&progress);
      if (n) {
        if (watered == N)
          congratulations = 1;
        save_game();
      }
    }
  } else {
    pour = 0;
    pulse_clock = 0;
  }
  f_down = down;
}
static void key_action(HWND hwnd, WPARAM key) {
  if (key == VK_F2) {
    reset_prompt = 1;
    int answer = MessageBoxW(
        hwnd,
        L"Начать новую игру?\n\nВсе листики, шляпки, лейки, открытые "
        L"локации и политые цветы будут сброшены. Отменить это нельзя.",
        L"Новая игра", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
    reset_prompt = 0;
    previous = GetTickCount();
    if (answer != IDYES)
      return;
    new_progress(&progress);
    congratulations = 0;
    shop_open = 0;
    combo_count = 0;
    combo_time = 0;
    walk = 0;
    facing = 1;
    init();
    save_game();
    if (save_failed)
      MessageBoxW(hwnd,
                  L"Новая игра начата, но сохранить сброс на диск не удалось. "
                  L"При следующем запуске может вернуться старый прогресс.",
                  L"Ошибка сохранения", MB_OK | MB_ICONWARNING);
    InvalidateRect(hwnd, NULL, FALSE);
    return;
  }
  if (key == VK_ESCAPE) {
    if (shop_open) {
      shop_open = 0;
      return;
    }
    DestroyWindow(hwnd);
    return;
  }
  if (shop_open) {
    if (key == 'E') {
      shop_open = 0;
      return;
    }
    if (key >= '1' && key <= '5') {
      if (shop_item(&progress, (int)(key - '1'))) {
        lstrcpyW(shop_message, L"Готово! Обновка уже на тебе.");
        save_game();
      } else
        lstrcpyW(shop_message,
                 L"Не хватает листиков. Приходи после новых комбо!");
    } else if (key == '6') {
      progress.hat = 0;
      save_game();
    } else if (key == '7') {
      progress.can = 0;
      save_game();
    }
    return;
  }
  if (key == 'E' && near_shop() && !congratulations) {
    shop_open = 1;
    pour = 0;
    walk = 0;
    lstrcpyW(shop_message, L"Выбери вещь — покупка сразу надевается!");
    return;
  }
  if (key == VK_RETURN) {
    congratulations = 0;
    return;
  }
  if (key == 'L' && !progress.unlocked) {
    if (buy_garden(&progress)) {
      progress.location = 1;
      congratulations = 0;
      combo_time = 0;
      init();
      save_game();
    } else
      MessageBoxW(hwnd,
                  L"Нужно 30 листиков. Поливай сразу несколько новых цветов: "
                  L"каждое комбо даёт 2 листика!",
                  L"Лунный сад", MB_OK | MB_ICONINFORMATION);
  } else if (key == VK_TAB && progress.unlocked) {
    progress.location = 1 - progress.location;
    congratulations = 0;
    combo_time = 0;
    init();
    save_game();
  } else if (key == 'R' && watered == N) {
    memset(progress.wet[progress.location], 0, N);
    congratulations = 0;
    combo_time = 0;
    init();
    save_game();
  }
}
static LRESULT CALLBACK proc(HWND h, UINT m, WPARAM w, LPARAM l) {
  switch (m) {
  case WM_KEYDOWN:
    if (!(l & (1L << 30)))
      key_action(h, w);
    return 0;
  case WM_CREATE:
    previous = GetTickCount();
    SetTimer(h, 1, 16, NULL);
    return 0;
  case WM_TIMER:
    update(h);
    InvalidateRect(h, NULL, FALSE);
    return 0;
  case WM_ERASEBKGND:
    return 1;
  case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(h, &ps);
    HDC mem = CreateCompatibleDC(dc);
    HBITMAP bmp = CreateCompatibleBitmap(dc, W, H);
    HGDIOBJ old = SelectObject(mem, bmp);
    render(mem);
    BitBlt(dc, 0, 0, W, H, mem, 0, 0, SRCCOPY);
    SelectObject(mem, old);
    DeleteObject(bmp);
    DeleteDC(mem);
    EndPaint(h, &ps);
    return 0;
  }
  case WM_DESTROY:
    save_game();
    KillTimer(h, 1);
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProcW(h, m, w, l);
}
int WINAPI wWinMain(HINSTANCE inst, HINSTANCE prev, LPWSTR cmd, int show) {
  (void)prev;
  (void)cmd;
  SetProcessDPIAware();
  load_game();
  init();
  WNDCLASSW wc = {0};
  wc.lpfnWndProc = proc;
  wc.hInstance = inst;
  wc.lpszClassName = L"LittleFlowerGarden";
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  RegisterClassW(&wc);
  RECT r = {0, 0, W, H};
  DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
  AdjustWindowRect(&r, style, FALSE);
  HWND h = CreateWindowW(wc.lpszClassName, L"Цветочный сад", style,
                         CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left,
                         r.bottom - r.top, NULL, NULL, inst, NULL);
  if (!h)
    return 1;
  ShowWindow(h, show);
  MSG msg;
  while (GetMessageW(&msg, NULL, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  return 0;
}
