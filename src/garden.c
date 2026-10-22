#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <math.h>
#include <stdio.h>
#define W 960
#define H 640
#include "game.h"
static Flower flowers[N];
static float px=480,py=355,walk=0,pour=0;
static int facing=1, watered=0;
static DWORD previous;
static Progress progress;
static wchar_t save_path[MAX_PATH], temp_path[MAX_PATH];
static int congratulations=0, save_failed=0, combo_count=0;
static float combo_time=0, pulse_clock=0;
static int f_down=0;
static void save_game(void) {
    if(!save_path[0]) { save_failed=1; return; }
    FILE *file=_wfopen(temp_path,L"wb");
    if(!file) { save_failed=1; return; }
    int ok=fwrite(&progress,sizeof(progress),1,file)==1;
    if(fclose(file)!=0) ok=0;
    save_failed=!(ok && MoveFileExW(temp_path,save_path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH));
}
static void load_game(void) {
    new_progress(&progress);
    wchar_t dir[MAX_PATH];
    DWORD len=GetEnvironmentVariableW(L"LOCALAPPDATA",dir,MAX_PATH);
    if(!len || len>MAX_PATH-60) { save_failed=1; return; }
    lstrcatW(dir,L"\\FlowerGarden");
    if(!CreateDirectoryW(dir,NULL) && GetLastError()!=ERROR_ALREADY_EXISTS) { save_failed=1; return; }
    swprintf(save_path,MAX_PATH,L"%ls\\progress.dat",dir);
    swprintf(temp_path,MAX_PATH,L"%ls\\progress.tmp",dir);
    FILE *file=_wfopen(save_path,L"rb");
    if(file) {
        Progress loaded;
        int ok=fread(&loaded,sizeof(loaded),1,file)==1;
        fclose(file);
        if(ok && valid_progress(&loaded)) progress=loaded;
        else save_failed=1;
    }
}
static COLORREF palette[]={0x00839FF4,0x00BB8AE1,0x006CD8FF,0x00D9B78C,0x00A7A1FF,0x00E6D9FF};
static void oval(HDC dc,int x,int y,int w,int h,COLORREF c){HBRUSH b=CreateSolidBrush(c);HGDIOBJ old=SelectObject(dc,b);Ellipse(dc,x,y,x+w,y+h);SelectObject(dc,old);DeleteObject(b);}
static void box(HDC dc,int x,int y,int w,int h,COLORREF c){RECT r={x,y,x+w,y+h};HBRUSH b=CreateSolidBrush(c);FillRect(dc,&r,b);DeleteObject(b);}
static void line(HDC dc,int x,int y,int x2,int y2,int width,COLORREF c){HPEN p=CreatePen(PS_SOLID,width,c);HGDIOBJ old=SelectObject(dc,p);MoveToEx(dc,x,y,NULL);LineTo(dc,x2,y2);SelectObject(dc,old);DeleteObject(p);}
static void label(HDC dc,int x,int y,const wchar_t *s,int size,COLORREF c){HFONT f=CreateFontW(size,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");HGDIOBJ old=SelectObject(dc,f);SetTextColor(dc,c);SetBkMode(dc,TRANSPARENT);TextOutW(dc,x,y,s,lstrlenW(s));SelectObject(dc,old);DeleteObject(f);}
static void init(void){int centers[6][2]={{190,210},{450,180},{745,215},{215,465},{500,470},{770,465}};for(int g=0;g<6;g++)for(int j=0;j<12;j++){int i=g*12+j;flowers[i]=(Flower){centers[g][0]+(j%4)*30-45+(j/4%2)*9,centers[g][1]+(j/4)*30-30,progress.wet[progress.location][i],g};}watered=count_watered(&progress);px=480;py=355;pour=0;pulse_clock=0;f_down=0;}
static void girl(HDC dc){int x=(int)px,y=(int)py,b=(int)(sin(walk)*3);oval(dc,x-21,y+1,44,13,RGB(93,139,93));line(dc,x-8,y-12,x-9+(int)(sin(walk)*5),y+4,7,RGB(93,70,66));line(dc,x+8,y-12,x+9-(int)(sin(walk)*5),y+4,7,RGB(93,70,66));oval(dc,x-19,y-70+b,38,43,RGB(241,196,83));oval(dc,x-13,y-65+b,26,28,RGB(255,222,187));oval(dc,x-16,y-69+b,31,13,RGB(255,218,111));oval(dc,x-23,y-49+b,12,23,RGB(241,196,83));oval(dc,x+12,y-49+b,12,23,RGB(241,196,83));POINT dress[]={{x-10,y-38+b},{x+10,y-38+b},{x+21,y-10},{x-21,y-10}};HBRUSH br=CreateSolidBrush(RGB(217,134,155));HGDIOBJ old=SelectObject(dc,br);Polygon(dc,dress,4);SelectObject(dc,old);DeleteObject(br);oval(dc,x-5,y-54+b,3,4,RGB(80,70,65));oval(dc,x+5,y-54+b,3,4,RGB(80,70,65));line(dc,x-2,y-44+b,x+3,y-44+b,1,RGB(187,109,105));int cx=x+facing*30;line(dc,x+facing*12,y-32+b,cx,y-26,7,RGB(255,222,187));oval(dc,cx-12,y-34,24,16,RGB(66,125,130));oval(dc,cx-8,y-31,16,10,RGB(116,184,185));box(dc,cx-12,y-25,24,17,RGB(102,171,175));line(dc,cx+facing*8,y-19,cx+facing*27,y-30,6,RGB(102,171,175));if(pour>0)for(int k=0;k<8;k++){int t=((int)(pour*220)+k*7)%45;oval(dc,cx+facing*(27+t/3),y-27+t,3,6,RGB(168,225,245));}}
static void render(HDC dc){SelectObject(dc,GetStockObject(NULL_PEN));box(dc,0,0,W,H,(progress.location?RGB(172,198,207):RGB(196,218,164)));for(int y=110;y<H;y+=22)for(int x=30;x<W;x+=31)oval(dc,x+(y%7),y,3,4,RGB(174,199,140));box(dc,0,295,W,65,RGB(228,215,180));box(dc,440,110,75,H-110,RGB(228,215,180));for(int i=0;i<24;i++){int x=20+i*40;box(dc,x,95,9,37,RGB(246,235,207));}box(dc,0,105,W,6,RGB(246,235,207));box(dc,0,122,W,5,RGB(246,235,207));for(int i=0;i<6;i++){Flower *f=&flowers[i*12];oval(dc,(int)f->x-28,(int)f->y-11,165,116,RGB(157,186,125));oval(dc,(int)f->x-22,(int)f->y-7,153,105,RGB(169,195,135));}for(int i=0;i<N;i++){Flower *f=&flowers[i];int x=(int)f->x,y=(int)f->y;float sway=sinf((float)GetTickCount64()/1200+i)*2;line(dc,x,y,x+(int)sway,y-23,2,RGB(82,135,78));oval(dc,x-9,y-13,9,5,RGB(101,157,84));oval(dc,x+1,y-17,10,5,RGB(101,157,84));x+=(int)sway;y-=25;int r=f->water>0?7:6;int petals=progress.location?8:5;for(int k=0;k<petals;k++){float a=k*6.2831853f/petals;oval(dc,x+(int)(cos(a)*6)-r,y+(int)(sin(a)*6)-r,r*2,r*2,palette[f->color]);}if(progress.location){
    oval(dc,x-7,y-7,14,14,RGB(248,243,210));
    oval(dc,x-3,y-3,6,6,RGB(146,111,187));
    line(dc,x-14,y-17,x-14,y-25,1,RGB(244,238,204));
    line(dc,x-18,y-21,x-10,y-21,1,RGB(244,238,204));
}else oval(dc,x-4,y-4,8,8,RGB(255,226,124));if(f->water>0){oval(dc,x+10,y-17,4,7,RGB(110,189,222));}}girl(dc);box(dc,0,0,W,83,RGB(249,243,225));label(dc,29,13,(progress.location?L"Лунный сад · звёздные цветы":L"Маленький цветочный сад"),25,RGB(78,100,73));label(dc,30,49,L"WASD — гулять · F — полив · L — открыть сад · Tab — перейти · Esc — выход",16,RGB(115,124,101));wchar_t count[64];swprintf(count,64,L"Полито: %d / %d",watered,N);label(dc,785,12,count,18,RGB(78,100,73));
    swprintf(count,64,L"Листики: %d",progress.leaves);
    label(dc,785,36,count,18,RGB(78,100,73));
    box(dc,0,610,W,30,RGB(249,243,225));
    label(dc,24,615,progress.unlocked?L"Лунный сад открыт! Tab — сменить сад":L"Лунный сад: 6 листиков · Комбо: 2+ новых цветка за один полив → +2 листика",16,RGB(78,100,73));
    if(combo_time>0){swprintf(count,64,L"КОМБО ×%d! +2 листика",combo_count);label(dc,340,100,count,24,RGB(51,97,81));}
    if(save_failed)label(dc,24,584,L"Не удалось сохранить прогресс на диск",17,RGB(155,60,60));
    if(congratulations){
        box(dc,170,180,620,290,RGB(112,143,104));
        box(dc,178,188,604,274,RGB(255,248,229));
        label(dc,260,210,L"Поздравляем! Весь сад полит!",28,RGB(78,100,73));
        label(dc,235,258,progress.location?L"Звёздные цветы сияют благодаря тебе!":L"Каждый цветочек получил свою капельку заботы.",20,RGB(100,116,90));
        swprintf(count,64,L"Твои листики: %d",progress.leaves);
        label(dc,360,298,count,23,RGB(78,100,73));
        label(dc,220,341,progress.unlocked?L"Tab — перейти в другой сад":L"L — купить лунный сад за 6 листиков",21,RGB(78,100,73));
        label(dc,220,380,L"R — вырастить новые цветы и заработать ещё",20,RGB(100,116,90));
        label(dc,220,417,L"Enter — вернуться в сад",18,RGB(100,116,90));
    }
}
static void update(HWND hwnd) {
    DWORD now=GetTickCount();float dt=(now-previous)/1000.f;previous=now;
    if(dt>.05f)dt=.05f;
    if(GetForegroundWindow()!=hwnd)return;
    if(combo_time>0)combo_time-=dt;
    if(congratulations)return;
    float dx=0,dy=0;
    if(GetAsyncKeyState('W')&0x8000)dy--;
    if(GetAsyncKeyState('S')&0x8000)dy++;
    if(GetAsyncKeyState('A')&0x8000)dx--;
    if(GetAsyncKeyState('D')&0x8000)dx++;
    if(dx||dy){float length=sqrtf(dx*dx+dy*dy);px+=dx/length*155*dt;py+=dy/length*155*dt;walk+=dt*10;if(dx)facing=dx>0?1:-1;}else walk=0;
    if(px<40)px=40;
    if(px>920)px=920;
    if(py<180)py=180;
    if(py>600)py=600;
    int down=(GetAsyncKeyState('F')&0x8000)!=0;
    if(down){
        pour+=dt;pulse_clock-=dt;
        if(!f_down || pulse_clock<=0){
            pulse_clock=.4f;
            int n=water_pulse(&progress,flowers,px,py);
            if(n>=2){combo_count=n;combo_time=2.0f;}
            watered=count_watered(&progress);
            if(n){
                if(watered==N)congratulations=1;
                save_game();
            }
        }
    }else{pour=0;pulse_clock=0;}
    f_down=down;
}
static void key_action(HWND hwnd,WPARAM key) {
    if(key==VK_ESCAPE){DestroyWindow(hwnd);return;}
    if(key==VK_RETURN){congratulations=0;return;}
    if(key=='L' && !progress.unlocked){
        if(buy_garden(&progress)){
            progress.location=1;congratulations=0;combo_time=0;init();save_game();
        }else MessageBoxW(hwnd,L"Нужно 6 листиков. Поливай сразу несколько новых цветов: каждое комбо даёт 2 листика!",L"Лунный сад",MB_OK|MB_ICONINFORMATION);
    }else if(key==VK_TAB && progress.unlocked){
        progress.location=1-progress.location;congratulations=0;combo_time=0;init();save_game();
    }else if(key=='R' && watered==N){
        memset(progress.wet[progress.location],0,N);
        congratulations=0;combo_time=0;init();save_game();
    }
}
static LRESULT CALLBACK proc(HWND h,UINT m,WPARAM w,LPARAM l){switch(m){case WM_KEYDOWN:if(!(l & (1L<<30)))key_action(h,w);return 0;case WM_CREATE:previous=GetTickCount();SetTimer(h,1,16,NULL);return 0;case WM_TIMER:update(h);InvalidateRect(h,NULL,FALSE);return 0;case WM_ERASEBKGND:return 1;case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);HDC mem=CreateCompatibleDC(dc);HBITMAP bmp=CreateCompatibleBitmap(dc,W,H);HGDIOBJ old=SelectObject(mem,bmp);render(mem);BitBlt(dc,0,0,W,H,mem,0,0,SRCCOPY);SelectObject(mem,old);DeleteObject(bmp);DeleteDC(mem);EndPaint(h,&ps);return 0;}case WM_DESTROY:save_game();KillTimer(h,1);PostQuitMessage(0);return 0;}return DefWindowProcW(h,m,w,l);}
int WINAPI wWinMain(HINSTANCE inst,HINSTANCE prev,LPWSTR cmd,int show){(void)prev;(void)cmd;SetProcessDPIAware();load_game();init();WNDCLASSW wc={0};wc.lpfnWndProc=proc;wc.hInstance=inst;wc.lpszClassName=L"LittleFlowerGarden";wc.hCursor=LoadCursor(NULL,IDC_ARROW);RegisterClassW(&wc);RECT r={0,0,W,H};DWORD style=WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX;AdjustWindowRect(&r,style,FALSE);HWND h=CreateWindowW(wc.lpszClassName,L"Цветочный сад",style,CW_USEDEFAULT,CW_USEDEFAULT,r.right-r.left,r.bottom-r.top,NULL,NULL,inst,NULL);if(!h)return 1;ShowWindow(h,show);MSG msg;while(GetMessageW(&msg,NULL,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}return 0;}
