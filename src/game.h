#ifndef GARDEN_GAME_H
#define GARDEN_GAME_H
#include <string.h>
#define N 72
#define GARDEN_PRICE 6
typedef struct { float x,y,water; int color; } Flower;
typedef struct {
    unsigned magic;
    int leaves, unlocked, location;
    unsigned char wet[2][N];
} Progress;
#define SAVE_MAGIC 0x47415232u
static void new_progress(Progress *p) {
    memset(p,0,sizeof(*p)); p->magic=SAVE_MAGIC;
}
static int valid_progress(const Progress *p) {
    if(p->magic!=SAVE_MAGIC || p->leaves<0 || p->leaves>1000000 ||
       p->unlocked<0 || p->unlocked>1 || p->location<0 ||
       p->location>p->unlocked) return 0;
    for(int g=0;g<2;g++) for(int i=0;i<N;i++) if(p->wet[g][i]>1) return 0;
    return 1;
}
static int count_watered(const Progress *p) {
    int n=0; for(int i=0;i<N;i++) n+=p->wet[p->location][i]; return n;
}
/* A pulse pays once, only for two or more previously dry flowers. */
static int water_pulse(Progress *p,Flower *flowers,float x,float y) {
    int n=0;
    for(int i=0;i<N;i++) {
        float dx=flowers[i].x-x,dy=flowers[i].y-y;
        if(dx*dx+dy*dy<10000 && !p->wet[p->location][i]) {
            p->wet[p->location][i]=1; flowers[i].water=1; n++;
        }
    }
    if(n>=2 && p->leaves<=999998) p->leaves+=2;
    return n;
}
static int buy_garden(Progress *p) {
    if(p->unlocked || p->leaves<GARDEN_PRICE) return 0;
    p->leaves-=GARDEN_PRICE; p->unlocked=1; return 1;
}
#endif
