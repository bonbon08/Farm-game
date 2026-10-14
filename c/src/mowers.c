#include "farm.h"
#include <math.h>

// Base speed is deliberately higher than drones/players, and MOWER_SPEED_STEP
// (in farm.h) is steeper too - at max upgrade level a mower should visibly
// scream across the field, not just casually trundle past like the others.
#define MOWER_SPEED 130.0f
#define MOWER_ARRIVE_DIST 6.0f
#define MOWER_MOW_PAUSE 0.15f

void Mowers_Init(GameState *gs) {
    gs->mower_count = 0;
    for (int i = 0; i < MAX_MOWERS; i++) {
        gs->mowers[i] = (Mower){ 0 };
    }
}

int Mowers_Cost(GameState *gs) {
    if (gs->mower_count >= MAX_MOWERS) return -1;
    return (int)(80.0f * powf(1.3f, (float)gs->mower_count));
}

// Lane i starts at row i (within the current mower count) and scans left to
// right, top to bottom - "von oben links nach rechts unten".
static Mower MakeMower(int lane) {
    Mower m = { 0 };
    m.active = true;
    m.tx = 0;
    m.ty = lane;
    m.x = m.tx * TILE_DRAW + TILE_DRAW / 2.0f - PLAYER_SIZE / 2.0f;
    m.y = m.ty * TILE_DRAW + TILE_DRAW / 2.0f - PLAYER_SIZE / 2.0f;
    m.action_timer = 0.0f;
    m.bob_seed = (float)GetRandomValue(0, 1000) / 100.0f;
    m.broken = false;
    m.wear = 0.0f;
    return m;
}

bool Mowers_Buy(GameState *gs) {
    int cost = Mowers_Cost(gs);
    if (cost < 0) return false;
    if (gs->economy.money < cost) {
        Audio_Play(SFX_DENY);
        return false;
    }
    gs->economy.money -= cost;
    gs->mowers[gs->mower_count] = MakeMower(gs->mower_count);
    gs->mower_count++;
    Toast_Show(gs, "Rasenmaeher gekauft!");
    Audio_Play(SFX_UPGRADE);
    return true;
}

void Mowers_SetCount(GameState *gs, int count) {
    if (count < 0) count = 0;
    if (count > MAX_MOWERS) count = MAX_MOWERS;
    gs->mower_count = count;
    for (int i = 0; i < count; i++) {
        gs->mowers[i] = MakeMower(i % (count > 0 ? count : 1));
    }
}

// Mows every weed or overripe (missed-the-window) plant within mower_range
// tiles (Chebyshev distance) of (cx, cy) - both count as "bad stuff" to clear,
// not just weeds. Weeds still cost the usual removal fee; overripe plants are
// just cleared for free, same as a manual overripe harvest. Returns true if
// anything was cleared.
static bool MowAround(GameState *gs, int cx, int cy) {
    int range = gs->economy.mower_range_level;
    bool any = false;
    int fw = Field_W(gs), fh = Field_H(gs);
    for (int dx = -range; dx <= range; dx++) {
        for (int dy = -range; dy <= range; dy++) {
            int tx = cx + dx, ty = cy + dy;
            if (tx < 0 || tx >= fw || ty < 0 || ty >= fh) continue;
            Tile *t = &gs->world[tx][ty];
            if (t->state == TILE_WEED) {
                int loss = GetRandomValue(1, 30);
                gs->economy.money -= loss;
                t->state = TILE_EMPTY;
                t->pop_timer = 0.3f;
                any = true;
            } else if (t->state == TILE_OVERRIPE) {
                t->state = TILE_EMPTY;
                t->pop_timer = 0.3f;
                any = true;
            }
        }
    }
    return any;
}

void Mowers_Update(GameState *gs, float dt) {
    int count = gs->mower_count;
    int fw = Field_W(gs), fh = Field_H(gs);
    for (int i = 0; i < count; i++) {
        Mower *m = &gs->mowers[i];
        if (!m->active || m->broken) continue;

        if (m->action_timer > 0.0f) {
            m->action_timer -= dt;
            continue;
        }

        float target_px = m->tx * TILE_DRAW + TILE_DRAW / 2.0f - PLAYER_SIZE / 2.0f;
        float target_py = m->ty * TILE_DRAW + TILE_DRAW / 2.0f - PLAYER_SIZE / 2.0f;
        float dx = target_px - m->x, dy = target_py - m->y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist < MOWER_ARRIVE_DIST) {
            bool mowed = MowAround(gs, m->tx, m->ty);
            if (mowed) {
                Audio_Play(SFX_WEED);
                m->action_timer = MOWER_MOW_PAUSE / gs->economy.mower_speed_mult;
                m->wear += (float)GetRandomValue((int)(UNIT_WEAR_PER_ACTION_MIN * 10), (int)(UNIT_WEAR_PER_ACTION_MAX * 10)) / 10.0f;
                if (m->wear >= UNIT_WEAR_MAX) {
                    m->broken = true;
                    Toast_Show(gs, "Ein Rasenmaeher ist kaputt gegangen!");
                }
            }

            m->tx++;
            if (m->tx >= fw) {
                m->tx = 0;
                m->ty += count;
                if (m->ty >= fh) m->ty = i % count;
            }
        } else {
            // Clamp the step to the remaining distance - at high enough
            // mower_speed_mult, speed*dt can exceed the distance to the
            // target, overshooting it every frame and bouncing back and
            // forth forever without ever getting within MOWER_ARRIVE_DIST
            // (the mower looks "frozen" even though it's actually vibrating
            // in place at high speed). Snapping to the target instead avoids
            // that regardless of how high the speed upgrade goes.
            float speed = MOWER_SPEED * gs->economy.mower_speed_mult;
            float step = speed * dt;
            if (step >= dist) {
                m->x = target_px;
                m->y = target_py;
            } else {
                m->x += dx / dist * step;
                m->y += dy / dist * step;
            }
        }
    }
}
