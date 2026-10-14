#include "farm.h"
#include <math.h>

// Birds: an adversarial pest, not purchasable - they spawn on their own ever
// more often as the days go by, and from BIRD_STRONG_DAY onward they're bold
// enough to eat sprouting crops too, not just ripe ones. Vogelscheuchen
// (scarecrows) passively cut down how often they spawn at all; Hunde (dogs)
// actively hunt down ones that already made it onto the field.
#define BIRD_SPEED 70.0f
#define BIRD_ARRIVE_DIST 6.0f
#define DOG_SPEED 160.0f
#define DOG_ARRIVE_DIST 8.0f

void Pests_Init(GameState *gs) {
    for (int i = 0; i < MAX_BIRDS; i++) gs->birds[i] = (Bird){ 0 };
    gs->bird_spawn_timer = 6.0f;
    gs->dog_count = 0;
    for (int i = 0; i < MAX_DOGS; i++) gs->dogs[i] = (Dog){ 0 };
}

static Bird MakeBird(void) {
    Bird b = { 0 };
    b.active = true;
    switch (GetRandomValue(0, 3)) {
        case 0: b.x = (float)GetRandomValue(0, WORLD_PX_W); b.y = 0; break;
        case 1: b.x = (float)GetRandomValue(0, WORLD_PX_W); b.y = (float)WORLD_PX_H; break;
        case 2: b.x = 0; b.y = (float)GetRandomValue(0, WORLD_PX_H); break;
        default: b.x = (float)WORLD_PX_W; b.y = (float)GetRandomValue(0, WORLD_PX_H); break;
    }
    b.has_target = false;
    return b;
}

void Birds_Update(GameState *gs, float dt) {
    gs->bird_spawn_timer -= dt;
    if (gs->bird_spawn_timer <= 0.0f) {
        // Ramps up much harder than before - by day 10 a check comes almost
        // three times as often as on day 1, and keeps getting faster still.
        float interval = 8.0f / (1.0f + gs->daynight.day_count * 0.25f);
        if (interval < 1.0f) interval = 1.0f;
        gs->bird_spawn_timer = interval;

        float spawn_chance = 0.7f * gs->economy.scarecrow_mult * Weather_BirdFactor(gs);
        if ((float)GetRandomValue(0, 1000) / 1000.0f < spawn_chance) {
            for (int i = 0; i < MAX_BIRDS; i++) {
                if (!gs->birds[i].active) { gs->birds[i] = MakeBird(); break; }
            }
        }
    }

    bool strong = gs->daynight.day_count >= BIRD_STRONG_DAY;
    int fw = Field_W(gs), fh = Field_H(gs);

    for (int i = 0; i < MAX_BIRDS; i++) {
        Bird *b = &gs->birds[i];
        if (!b->active) continue;

        if (!b->has_target) {
            float best_d = 1e18f;
            int bx = -1, by = -1;
            for (int x = 0; x < fw; x++) {
                for (int y = 0; y < fh; y++) {
                    TileState s = gs->world[x][y].state;
                    bool match = (s == TILE_RIPE) || (strong && s == TILE_SPROUT);
                    if (!match) continue;
                    float cx = x * TILE_DRAW + TILE_DRAW / 2.0f;
                    float cy = y * TILE_DRAW + TILE_DRAW / 2.0f;
                    float dx = cx - b->x, dy = cy - b->y;
                    float d = dx * dx + dy * dy;
                    if (d < best_d) { best_d = d; bx = x; by = y; }
                }
            }
            if (bx < 0) { b->active = false; continue; } // nothing worth eating - flies off
            b->target_x = bx;
            b->target_y = by;
            b->has_target = true;
        }

        float target_px = b->target_x * TILE_DRAW + TILE_DRAW / 2.0f - PLAYER_SIZE / 2.0f;
        float target_py = b->target_y * TILE_DRAW + TILE_DRAW / 2.0f - PLAYER_SIZE / 2.0f;
        float dx = target_px - b->x, dy = target_py - b->y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist < BIRD_ARRIVE_DIST) {
            // A bird doesn't just peck at one tile - once it lands on a
            // field, it wrecks everything ripe (and, past BIRD_STRONG_DAY,
            // sprouting) in a radius around it. That radius grows with the
            // days too, so late game a single bird that gets through can
            // wipe out a real chunk of the field, not just one square.
            int radius = 1 + gs->daynight.day_count / 6;
            if (radius > 4) radius = 4;
            bool hit_any = false;
            for (int dx2 = -radius; dx2 <= radius; dx2++) {
                for (int dy2 = -radius; dy2 <= radius; dy2++) {
                    int tx = b->target_x + dx2, ty = b->target_y + dy2;
                    if (tx < 0 || tx >= fw || ty < 0 || ty >= fh) continue;
                    Tile *t = &gs->world[tx][ty];
                    if (t->state == TILE_RIPE || (strong && t->state == TILE_SPROUT)) {
                        t->state = TILE_EMPTY;
                        t->pop_timer = 0.3f;
                        hit_any = true;
                    }
                }
            }
            if (hit_any) Audio_Play(SFX_DENY);
            b->active = false; // flies off after striking, whether it found anything or not
        } else {
            float step = BIRD_SPEED * dt;
            if (step >= dist) {
                b->x = target_px;
                b->y = target_py;
            } else {
                b->x += dx / dist * step;
                b->y += dy / dist * step;
            }
        }
    }
}

static Dog MakeDog(void) {
    Dog d = { 0 };
    d.active = true;
    d.x = WORLD_PX_W * 0.5f + (float)GetRandomValue(-150, 150);
    d.y = WORLD_PX_H * 0.5f + (float)GetRandomValue(-150, 150);
    d.has_target = false;
    d.bob_seed = (float)GetRandomValue(0, 1000) / 100.0f;
    return d;
}

int Dogs_Cost(GameState *gs) {
    if (gs->dog_count >= MAX_DOGS) return -1;
    return (int)(80.0f * powf(1.3f, (float)gs->dog_count));
}

bool Dogs_Buy(GameState *gs) {
    int cost = Dogs_Cost(gs);
    if (cost < 0) return false;
    if (gs->economy.money < cost) {
        Audio_Play(SFX_DENY);
        return false;
    }
    gs->economy.money -= cost;
    gs->dogs[gs->dog_count] = MakeDog();
    gs->dog_count++;
    Toast_Show(gs, "Hund gekauft!");
    Audio_Play(SFX_UPGRADE);
    return true;
}

void Dogs_SetCount(GameState *gs, int count) {
    if (count < 0) count = 0;
    if (count > MAX_DOGS) count = MAX_DOGS;
    gs->dog_count = count;
    for (int i = 0; i < count; i++) gs->dogs[i] = MakeDog();
}

void Dogs_Update(GameState *gs, float dt) {
    for (int i = 0; i < gs->dog_count; i++) {
        Dog *dg = &gs->dogs[i];
        if (!dg->active) continue;

        if (dg->has_target && !gs->birds[dg->target_bird].active) dg->has_target = false;

        if (!dg->has_target) {
            float best_d = 1e18f;
            int bi = -1;
            for (int j = 0; j < MAX_BIRDS; j++) {
                if (!gs->birds[j].active) continue;
                float ddx = gs->birds[j].x - dg->x, ddy = gs->birds[j].y - dg->y;
                float d = ddx * ddx + ddy * ddy;
                if (d < best_d) { best_d = d; bi = j; }
            }
            if (bi < 0) continue;
            dg->target_bird = bi;
            dg->has_target = true;
        }

        Bird *b = &gs->birds[dg->target_bird];
        float dx = b->x - dg->x, dy = b->y - dg->y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist < DOG_ARRIVE_DIST) {
            b->active = false; // scared off
            dg->has_target = false;
            Audio_Play(SFX_UI_CLICK);
        } else {
            float step = DOG_SPEED * dt;
            if (step >= dist) {
                dg->x = b->x;
                dg->y = b->y;
            } else {
                dg->x += dx / dist * step;
                dg->y += dy / dist * step;
            }
        }
    }
}
