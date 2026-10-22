#include "../src/game.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
  Progress p;
  Flower f[N];
  new_progress(&p);
  assert(valid_progress(&p));
  for (int i = 0; i < N; i++)
    f[i] = (Flower){1000 + i * 200, 1000, 0, 0};
  f[0].x = 0;
  f[0].y = 0;
  assert(water_pulse(&p, f, 0, 0) == 1 && p.leaves == 0);
  f[1].x = 10;
  f[1].y = 10;
  f[2].x = 20;
  f[2].y = 20;
  assert(water_pulse(&p, f, 0, 0) == 2 && p.leaves == 2);
  assert(water_pulse(&p, f, 0, 0) == 0 && p.leaves == 2);
  assert(!buy_garden(&p) && !p.unlocked && p.leaves == 2);
  for (int i = 3; i < N; i++) {
    f[i].x = 0;
    f[i].y = 0;
  }
  assert(water_pulse(&p, f, 0, 0) == N - 3 && p.leaves == 4);
  assert(count_watered(&p) == N);
  memset(p.wet[0], 0, N);
  assert(water_pulse(&p, f, 0, 0) == N && p.leaves == 6);
  assert(!buy_garden(&p) && p.leaves == 6);
  p.leaves = 29;
  assert(!buy_garden(&p) && p.leaves == 29);
  p.leaves = 30;
  assert(buy_garden(&p) && p.leaves == 0 && p.unlocked);
  assert(!buy_garden(&p) && p.leaves == 0);
  p.location = 1;
  assert(count_watered(&p) == 0);
  assert(water_pulse(&p, f, 0, 0) == N && p.leaves == 2);
  assert(count_watered(&p) == N);
  p.location = 0;
  assert(count_watered(&p) == N);
  FILE *save = tmpfile();
  assert(save);
  assert(fwrite(&p, sizeof(p), 1, save) == 1);
  rewind(save);
  Progress restored;
  assert(fread(&restored, sizeof(restored), 1, save) == 1);
  fclose(save);
  assert(valid_progress(&restored) && memcmp(&restored, &p, sizeof(p)) == 0);
  restored.leaves = -1;
  assert(!valid_progress(&restored));
  restored = p;
  restored.wet[1][0] = 2;
  assert(!valid_progress(&restored));
  restored = p;
  restored.unlocked = 0;
  restored.location = 1;
  assert(!valid_progress(&restored));
  restored = p;
  restored.magic = 0;
  assert(!valid_progress(&restored));
  /* Buying, equipping and failed transactions. */
  Progress shopper;
  new_progress(&shopper);
  for (int i = 0; i < SHOP_ITEMS; i++) {
    Progress before = shopper;
    assert(!shop_item(&shopper, i));
    assert(memcmp(&before, &shopper, sizeof(shopper)) == 0);
    shopper.leaves = item_prices[i];
    assert(shop_item(&shopper, i) && shopper.leaves == 0);
    assert(valid_progress(&shopper));
    assert(shop_item(&shopper, i) && shopper.leaves == 0);
    if (i < 3)
      assert(shopper.hat == i + 1);
    else
      assert(shopper.can == i - 2);
  }
  assert(shopper.hats == 15 && shopper.cans == 7);
  assert(!shop_item(&shopper, -1) && !shop_item(&shopper, SHOP_ITEMS));
  assert(shop_item(&shopper, 0) && shopper.hat == 1);
  assert(shop_item(&shopper, 3) && shopper.can == 1);
  assert(decode_progress(&restored, &shopper, sizeof(shopper)));
  assert(memcmp(&restored, &shopper, sizeof(shopper)) == 0);
  LegacyProgress legacy;
  memcpy(&legacy, &p, sizeof(legacy));
  assert(decode_progress(&restored, &legacy, sizeof(legacy)));
  assert(restored.leaves == p.leaves && restored.unlocked == p.unlocked);
  assert(memcmp(restored.wet, p.wet, sizeof(p.wet)) == 0);
  assert(restored.hats == 1 && restored.cans == 1 && restored.hat == 0 &&
         restored.can == 0);
  assert(!decode_progress(&restored, &legacy, sizeof(legacy) - 1));
  restored = shopper;
  restored.hat = 4;
  assert(!valid_progress(&restored));
  restored = shopper;
  restored.cans = 1;
  assert(!valid_progress(&restored));
  /* Reset a progressed game, then verify the state that will be saved. */
  shopper.leaves = 123;
  shopper.unlocked = 1;
  shopper.location = 1;
  memset(shopper.wet, 1, sizeof(shopper.wet));
  new_progress(&shopper);
  assert(valid_progress(&shopper));
  assert(shopper.leaves == 0 && shopper.unlocked == 0 && shopper.location == 0);
  assert(shopper.hats == 1 && shopper.cans == 1 && shopper.hat == 0 && shopper.can == 0);
  for (int g = 0; g < 2; g++)
    for (int i = 0; i < N; i++) assert(shopper.wet[g][i] == 0);
  assert(decode_progress(&restored, &shopper, sizeof(shopper)));
  assert(memcmp(&restored, &shopper, sizeof(shopper)) == 0);
  puts("PASS: full progress reset and reload");
  puts("PASS: shop purchases, insufficient funds, free re-equipping, all five "
       "items, migration, cosmetic persistence");
  puts("PASS: combos, no repeat rewards, price, purchase once, completion, "
       "replay, independent gardens, save roundtrip, invalid saves");
}
