#ifndef GARDEN_GAME_H
#define GARDEN_GAME_H
#include <string.h>
#define N 72
#define GARDEN_PRICE 30
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
} Progress;
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
}
static int valid_progress(const Progress *p) {
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
  else
    return 0;
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
/* A pulse pays once, only for two or more previously dry flowers. */
static int water_pulse(Progress *p, Flower *flowers, float x, float y) {
  int n = 0;
  for (int i = 0; i < N; i++) {
    float dx = flowers[i].x - x, dy = flowers[i].y - y;
    if (dx * dx + dy * dy < 10000 && !p->wet[p->location][i]) {
      p->wet[p->location][i] = 1;
      flowers[i].water = 1;
      n++;
    }
  }
  if (n >= 2 && p->leaves <= 999998)
    p->leaves += 2;
  return n;
}
static int buy_garden(Progress *p) {
  if (p->unlocked || p->leaves < GARDEN_PRICE)
    return 0;
  p->leaves -= GARDEN_PRICE;
  p->unlocked = 1;
  return 1;
}
#endif
