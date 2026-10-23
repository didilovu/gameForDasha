#include "../src/game.h"
#include <assert.h>
#include <stdio.h>
static Flower flowers[N];
static void setup_flowers(void) {
  for (int b = 0; b < 6; b++)
    for (int j = 0; j < 12; j++)
      flowers[b * 12 + j] =
          (Flower){b * 250 + (j % 4) * 10, (j / 4) * 10, 0, b};
}
static int finish(Progress *p, Watering *w, int bed) {
  int total = 0;
  for (int t = 0; t < 40; t++)
    total += water_tick(p, flowers, w, bed * 250 + 15, 10, 1, .05f);
  return total;
}
int main(void) {
  Progress p;
  Watering w = {-1, 0};
  new_progress(&p);
  setup_flowers();
  assert(valid_progress(&p) && p.beds[0] == 1 && p.beds[1] == 1);
  assert(GARDEN_PRICE == 1500);
  assert(!buy_bed(&p) && p.leaves == 0);
  assert(finish(&p, &w, 1) == 0 && p.leaves == 0); /* Not planted. */
  for (int i = 0; i < 20; i++)
    assert(!water_tick(&p, flowers, &w, 15, 10, 1, .05f));
  assert(p.leaves == 0 && count_watered(&p) == 0 && w.elapsed > .9f);
  assert(!water_tick(&p, flowers, &w, 15, 10, 0, .05f) && w.elapsed == 0);
  for (int i = 0; i < 20; i++)
    assert(!water_tick(&p, flowers, &w, 15, 10, 1, .05f));
  assert(!water_tick(&p, flowers, &w, 800, 600, 1, .05f) && w.bed == -1);
  assert(finish(&p, &w, 0) == 1 && p.leaves == 1 && count_watered(&p) == 12);
  assert(finish(&p, &w, 0) == 0 && p.leaves == 1); /* No duplicate rewards. */
  /* One starting bed can fund the second without cheats or a dead end. */
  for (int i = 0; i < 2; i++) {
    memset(p.wet[0], 0, N);
    assert(finish(&p, &w, 0) == 1);
  }
  assert(p.leaves == 3 && buy_bed(&p) && p.leaves == 0 && p.beds[0] == 2);
  assert(finish(&p, &w, 1) == 2 && p.leaves == 2);
  int previous_price = 3;
  for (int beds = 2; beds < 6; beds++) {
    int price = bed_price(&p);
    assert(price > previous_price);
    previous_price = price;
    p.leaves = price - 1;
    Progress before = p;
    assert(!buy_bed(&p) && memcmp(&before, &p, sizeof(p)) == 0);
    p.leaves = price;
    assert(buy_bed(&p) && p.leaves == 0);
    memset(p.wet[0], 0, N);
    for (int b = 0; b < p.beds[0]; b++)
      assert(finish(&p, &w, b) == p.beds[0]);
    assert(count_watered(&p) == p.beds[0] * 12);
    assert(p.leaves == p.beds[0] * p.beds[0]);
  }
  assert(bed_price(&p) == 0 && !buy_bed(&p));
  p.leaves = 1499;
  assert(!buy_garden(&p) && p.leaves == 1499);
  p.leaves = 1500;
  assert(buy_garden(&p) && p.leaves == 0 && p.unlocked);
  assert(!buy_garden(&p));
  p.location = 1;
  assert(p.beds[1] == 1 && count_watered(&p) == 0);
  assert(finish(&p, &w, 0) == 1 && p.leaves == 1);
  p.location = 0;
  assert(count_watered(&p) == 72);
  /* Cosmetic shop still works independently of planting. */
  for (int i = 0; i < SHOP_ITEMS; i++) {
    p.leaves = item_prices[i];
    assert(shop_item(&p, i) && p.leaves == 0);
    assert(shop_item(&p, i) && p.leaves == 0 && valid_progress(&p));
  }
  Progress switching;
  new_progress(&switching);
  switching.beds[0] = 2;
  w = (Watering){-1, 0};
  for (int i = 0; i < 30; i++)
    assert(!water_tick(&switching, flowers, &w, 15, 10, 1, .05f));
  assert(!water_tick(&switching, flowers, &w, 265, 10, 1, .05f));
  assert(w.bed == 1 && w.elapsed < .1f && switching.leaves == 0);
  /* The entire planting progression and moon garden are reachable via play. */
  Progress economy;
  new_progress(&economy);
  w = (Watering){-1, 0};
  for (int goal = 2; goal <= 6; goal++) {
    while (economy.leaves < bed_price(&economy)) {
      memset(economy.wet[0], 0, N);
      for (int b = 0; b < economy.beds[0]; b++)
        assert(finish(&economy, &w, b) == economy.beds[0]);
    }
    assert(buy_bed(&economy));
  }
  while (economy.leaves < GARDEN_PRICE) {
    memset(economy.wet[0], 0, N);
    for (int b = 0; b < 6; b++)
      assert(finish(&economy, &w, b) == 6);
  }
  assert(buy_garden(&economy) && valid_progress(&economy));
  Progress decoded;
  assert(decode_progress(&decoded, &p, sizeof(p)));
  assert(memcmp(&decoded, &p, sizeof(p)) == 0);
  FILE *file = tmpfile();
  assert(file);
  assert(fwrite(&p, sizeof(p), 1, file) == 1);
  rewind(file);
  assert(fread(&decoded, sizeof(decoded), 1, file) == 1);
  fclose(file);
  assert(valid_progress(&decoded) && memcmp(&decoded, &p, sizeof(p)) == 0);
  /* Both previous save formats retain balance, purchases and garden access. */
  p.leaves = 99;
  assert(decode_progress(&decoded, &p, sizeof(p) - sizeof(p.beds)));
  assert(decoded.leaves == 99 && decoded.unlocked && decoded.hats == 15 &&
         decoded.cans == 7);
  assert(decoded.beds[0] == 1 && decoded.beds[1] == 1 &&
         count_watered(&decoded) == 0);
  LegacyProgress legacy;
  memcpy(&legacy, &p, sizeof(legacy));
  assert(decode_progress(&decoded, &legacy, sizeof(legacy)));
  assert(decoded.leaves == 99 && decoded.hats == 1 && decoded.beds[0] == 1);
  assert(!decode_progress(&decoded, &legacy, sizeof(legacy) - 1));
  decoded = p;
  decoded.beds[0] = 7;
  assert(!valid_progress(&decoded));
  decoded = p;
  decoded.beds[1] = 1;
  decoded.wet[1][12] = 1;
  assert(!valid_progress(&decoded));
  decoded = p;
  decoded.wet[0][0] = 0;
  assert(!valid_progress(&decoded));
  decoded = p;
  decoded.hat = 4;
  assert(!valid_progress(&decoded));
  new_progress(&p);
  assert(valid_progress(&p) && p.leaves == 0 && p.beds[0] == 1 && !p.unlocked);
  puts("PASS: timed watering, interruptions, locked beds, no duplicate "
       "rewards, replay income");
  puts("PASS: increasing prices, all six reward tiers, purchases, 1500-leaf "
       "moon garden, separate gardens");
  puts("PASS: cosmetics, save roundtrip, both legacy migrations, invalid "
       "saves, full reset");
}
