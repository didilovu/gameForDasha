#include <assert.h>
#include <stdio.h>
#include "../src/game.h"
int main(void) {
    Progress p; Flower f[N]; new_progress(&p);
    assert(valid_progress(&p));
    for(int i=0;i<N;i++) f[i]=(Flower){1000+i*200,1000,0,0};
    f[0].x=0;f[0].y=0;
    assert(water_pulse(&p,f,0,0)==1 && p.leaves==0);
    f[1].x=10;f[1].y=10;f[2].x=20;f[2].y=20;
    assert(water_pulse(&p,f,0,0)==2 && p.leaves==2);
    assert(water_pulse(&p,f,0,0)==0 && p.leaves==2);
    assert(!buy_garden(&p) && !p.unlocked && p.leaves==2);
    for(int i=3;i<N;i++) { f[i].x=0;f[i].y=0; }
    assert(water_pulse(&p,f,0,0)==N-3 && p.leaves==4);
    assert(count_watered(&p)==N);
    memset(p.wet[0],0,N);
    assert(water_pulse(&p,f,0,0)==N && p.leaves==6);
    assert(buy_garden(&p) && p.leaves==0 && p.unlocked);
    assert(!buy_garden(&p) && p.leaves==0);
    p.location=1;
    assert(count_watered(&p)==0);
    assert(water_pulse(&p,f,0,0)==N && p.leaves==2);
    assert(count_watered(&p)==N);
    p.location=0; assert(count_watered(&p)==N);
    FILE *save=tmpfile();assert(save);
    assert(fwrite(&p,sizeof(p),1,save)==1);rewind(save);
    Progress restored;assert(fread(&restored,sizeof(restored),1,save)==1);fclose(save);
    assert(valid_progress(&restored) && memcmp(&restored,&p,sizeof(p))==0);
    restored.leaves=-1;assert(!valid_progress(&restored));
    restored=p;restored.wet[1][0]=2;assert(!valid_progress(&restored));
    restored=p;restored.unlocked=0;restored.location=1;assert(!valid_progress(&restored));
    restored=p;restored.magic=0;assert(!valid_progress(&restored));
    puts("PASS: combos, no repeat rewards, price, purchase once, completion, replay, independent gardens, save roundtrip, invalid saves");
}
