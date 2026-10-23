#ifndef GARDEN_GAME_H
#define GARDEN_GAME_H
#include <stddef.h>
#include <string.h>
#define N 72
#define GARDEN_PRICE 1500
#define WATER_SECONDS 2.0f
#define SHOP_ITEMS 5
typedef struct {
  float x, y, water;
  int color;
} Flower;
typedef struct {
  unsigned magic;
  int leaves, unlocked, location;
  unsigned char wet[2][N];
  unsigned hats, cans;
  int hat, can;
  int beds[2];
  unsigned day;
} Progress;
typedef struct {
  int bed;
  float elapsed;
} Watering;
typedef struct {
  unsigned magic;
  int leaves, unlocked, location;
  unsigned char wet[2][N];
} LegacyProgress;
static const int item_prices[SHOP_ITEMS] = {4, 8, 12, 6, 10};
#define SAVE_MAGIC 0x47415232u
static void new_progress(Progress *p) {
  memset(p, 0, sizeof(*p));
  p->magic = SAVE_MAGIC;
  p->hats = 1;
  p->cans = 1;
  p->beds[0] = p->beds[1] = 1;
  p->day = 1;
}
static int valid_progress(const Progress *p) {
  if (p->day < 1 || p->day > 1000000)
    return 0;
  if (p->magic != SAVE_MAGIC || p->leaves < 0 || p->leaves > 1000000 ||
      p->unlocked < 0 || p->unlocked > 1 || p->location < 0 ||
      p->location > p->unlocked)
    return 0;
  for (int g = 0; g < 2; g++)
    for (int i = 0; i < N; i++)
      if (p->wet[g][i] > 1)
        return 0;
  if (p->hats > 15 || p->cans > 7 || !(p->hats & 1) || !(p->cans & 1) ||
      p->hat < 0 || p->hat > 3 || p->can < 0 || p->can > 2 ||
      !(p->hats & (1u << p->hat)) || !(p->cans & (1u << p->can)))
    return 0;
  for (int g = 0; g < 2; g++) {
    if (p->beds[g] < 1 || p->beds[g] > 6)
      return 0;
    for (int b = 0; b < 6; b++) {
      for (int i = 0; i < 12; i++) {
        if (b >= p->beds[g] && p->wet[g][b * 12 + i])
          return 0;
        if (p->wet[g][b * 12 + i] != p->wet[g][b * 12])
          return 0;
      }
    }
  }
  return 1;
}
/* Import old saves without changing their balance or already unlocked garden.
 */
static int decode_progress(Progress *out, const void *data, size_t size) {
  Progress p;
  new_progress(&p);
  if (size == sizeof(LegacyProgress))
    memcpy(&p, data, sizeof(LegacyProgress));
  else if (size == sizeof(Progress))
    memcpy(&p, data, sizeof(Progress));
  else if (size == offsetof(Progress, beds) || size == offsetof(Progress, day))
    memcpy(&p, data, size);
  else
    return 0;
  if (size < offsetof(Progress, day)) {
    /* The new planting economy starts with one dry bed per garden. */
    for (int g = 0; g < 2; g++)
      for (int i = 0; i < N; i++)
        if (p.wet[g][i] > 1)
          return 0;
    memset(p.wet, 0, sizeof(p.wet));
  }
  if (!valid_progress(&p))
    return 0;
  *out = p;
  return 1;
}
/* Owned goods are equipped for free; failed purchases do not mutate state. */
static int shop_item(Progress *p, int item) {
  if (item < 0 || item >= SHOP_ITEMS)
    return 0;
  int is_hat = item < 3, index = is_hat ? item + 1 : item - 2;
  unsigned bit = 1u << index;
  unsigned *owned = is_hat ? &p->hats : &p->cans;
  if (!(*owned & bit)) {
    if (p->leaves < item_prices[item])
      return 0;
    p->leaves -= item_prices[item];
    *owned |= bit;
  }
  if (is_hat)
    p->hat = index;
  else
    p->can = index;
  return 1;
}
static int count_watered(const Progress *p) {
  int n = 0;
  for (int i = 0; i < N; i++)
    n += p->wet[p->location][i];
  return n;
}
static int bed_price(const Progress *p) {
  static const int prices[5] = {3, 8, 18, 32, 50};
  int n = p->beds[p->location];
  return n < 6 ? prices[n - 1] : 0;
}
static int buy_bed(Progress *p) {
  int price = bed_price(p);
  if (!price || p->leaves < price)
    return 0;
  p->leaves -= price;
  p->beds[p->location]++;
  return 1;
}
/* Only a full uninterrupted watering of one dry, planted bed pays. */
static int water_tick(Progress *p, Flower *flowers, Watering *w, float x,
                      float y, int held, float dt) {
  int target = -1;
  float distance = 10000;
  if (held)
    for (int b = 0; b < p->beds[p->location]; b++) {
      if (p->wet[p->location][b * 12])
        continue;
      float cx = 0, cy = 0;
      for (int j = 0; j < 12; j++) {
        cx += flowers[b * 12 + j].x / 12;
        cy += flowers[b * 12 + j].y / 12;
      }
      float dx = x - cx, dy = y - cy, d = dx * dx + dy * dy;
      if (d < distance) {
        distance = d;
        target = b;
      }
    }
  if (target < 0) {
    w->bed = -1;
    w->elapsed = 0;
    return 0;
  }
  if (w->bed != target) {
    w->bed = target;
    w->elapsed = 0;
  }
  if (dt <= 0 || dt > .1f)
    return 0;
  w->elapsed += dt;
  if (w->elapsed + 0.00001f < WATER_SECONDS)
    return 0;
  for (int j = 0; j < 12; j++) {
    p->wet[p->location][target * 12 + j] = 1;
    flowers[target * 12 + j].water = 1;
  }
  int reward = p->beds[p->location];
  if (p->leaves <= 1000000 - reward)
    p->leaves += reward;
  w->bed = -1;
  w->elapsed = 0;
  return reward;
}
static void next_day(Progress *p) {
  memset(p->wet, 0, sizeof(p->wet));
  if (p->day < 1000000)
    p->day++;
}
static int buy_garden(Progress *p) {
  if (p->unlocked || p->leaves < GARDEN_PRICE)
    return 0;
  p->leaves -= GARDEN_PRICE;
  p->unlocked = 1;
  return 1;
}
#endif
