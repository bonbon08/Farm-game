#include "farm.h"
#include <stdlib.h>

#define POP_DURATION 0.3f

// How much of the (fixed-size) map is actually usable right now. Players can
// walk and look anywhere on the physical map - the "Feld-Erweiterung" upgrade
// doesn't gate movement, only where planting/automation are allowed to act -
// but the field itself only ever grows, from FIELD_BASE_W x FIELD_BASE_H up
// to the full WORLD_W x WORLD_H.
int Field_W(GameState *gs) {
    int w = FIELD_BASE_W + gs->economy.field_level * FIELD_STEP_W;
    return w > WORLD_W ? WORLD_W : w;
}
int Field_H(GameState *gs) {
    int h = FIELD_BASE_H + gs->economy.field_level * FIELD_STEP_H;
    return h > WORLD_H ? WORLD_H : h;
}

void World_Init(GameState *gs) {
    for (int x = 0; x < WORLD_W; x++) {
        for (int y = 0; y < WORLD_H; y++) {
            gs->world[x][y].state = TILE_EMPTY;
            gs->world[x][y].timer = 0.0f;
            gs->world[x][y].pop_timer = 0.0f;
            gs->world[x][y].crop_type = 0;
        }
    }
}

// How long a tile stays in its current growth stage before advancing. Growth
// speed is tracked per crop tier (crop_type), not globally - a tile keeps
// using the crop it was planted with even if the player later switches which
// crop is "active", so this always needs to know which crop it's asking
// about rather than assuming "the current one".
float World_StageDuration(GameState *gs, int crop_type) {
    if (crop_type < 0 || crop_type >= MAX_CROP_TIERS) crop_type = 0;
    float base = 6.0f; // seconds per stage at level 0, daytime
    // Night clearly slows growth down - up to 2.5x as long per stage at the
    // deepest point of the night, not just a mild 1.5x dip.
    float night_penalty = 1.0f + gs->daynight.night_factor * 1.5f;
    return (base * night_penalty * Weather_GrowthFactor(gs)) / gs->economy.crop_track[crop_type].growth_mult;
}

// The "Auto-Farmarbeit" upgrade works the ground in diamond-shaped rings
// around each player: level 1 covers the 4 orthogonal neighbour tiles, level
// 2 additionally covers the 8 tiles of the next ring out (12 total - "two
// full rings"), and each further level adds one more ring. Empty tiles in
// range get planted (as whichever crop is currently active), ripe tiles get
// harvested (paid out as whichever crop *they* were planted as) - all for the
// usual price/payout, just without having to walk there. Weed removal is
// handled exclusively by the dedicated mower units (mowers.c), not here and
// not by drones - keeps each automation system to one clear job.
typedef enum { AUTO_NONE, AUTO_PLANTED, AUTO_HARVESTED } AutoAction;

static AutoAction TryAutoFarmTile(GameState *gs, int tx, int ty) {
    if (tx < 0 || tx >= Field_W(gs) || ty < 0 || ty >= Field_H(gs)) return AUTO_NONE;
    Tile *t = &gs->world[tx][ty];

    if (t->state == TILE_EMPTY) {
        if (gs->economy.money < 1) return AUTO_NONE;
        gs->economy.money -= 1;
        t->state = TILE_SPROUT;
        t->crop_type = gs->economy.crop_active;
        t->timer = World_StageDuration(gs, t->crop_type);
        t->pop_timer = POP_DURATION;
        gs->stats.total_planted++;
        Quest_Notify(gs, QUEST_PLANT, 1);
        return AUTO_PLANTED;
    }

    if (t->state == TILE_RIPE) {
        int payout = (int)(10 * gs->economy.crop_track[t->crop_type].sell_mult * Crop_Mult(t->crop_type) * Weather_SellMult(gs));
        gs->economy.money += payout;
        int crop = t->crop_type;
        t->state = TILE_EMPTY;
        t->pop_timer = POP_DURATION;
        gs->stats.total_harvested++;
        Quest_Notify(gs, QUEST_HARVEST, 1);
        Quest_Notify(gs, QUEST_EARN, payout);
        Progress_AddEarnings(gs, payout);
        Contracts_Notify(gs, crop, 1);
        return AUTO_HARVESTED;
    }

    return AUTO_NONE;
}

static void AutoFarmRingsAroundPlayer(GameState *gs, Player *p, int max_ring,
                                      bool *planted_any, bool *harvested_any) {
    int cx = (int)(p->x / TILE_DRAW);
    int cy = (int)(p->y / TILE_DRAW);

    for (int ring = 1; ring <= max_ring; ring++) {
        for (int dx = -ring; dx <= ring; dx++) {
            int dy = ring - abs(dx);
            AutoAction a1 = TryAutoFarmTile(gs, cx + dx, cy + dy);
            if (a1 == AUTO_PLANTED) *planted_any = true;
            if (a1 == AUTO_HARVESTED) *harvested_any = true;
            if (dy != 0) {
                AutoAction a2 = TryAutoFarmTile(gs, cx + dx, cy - dy);
                if (a2 == AUTO_PLANTED) *planted_any = true;
                if (a2 == AUTO_HARVESTED) *harvested_any = true;
            }
        }
    }
}

static void AutoPlantTick(GameState *gs, float dt) {
    if (gs->economy.auto_plant_level <= 0) return;

    gs->economy.auto_plant_cd -= dt;
    if (gs->economy.auto_plant_cd > 0.0f) return;
    gs->economy.auto_plant_cd = 2.5f - gs->economy.auto_plant_level * 0.5f;
    if (gs->economy.auto_plant_cd < 0.5f) gs->economy.auto_plant_cd = 0.5f;

    bool planted_any = false, harvested_any = false;
    for (int i = 0; i < gs->active_players; i++) {
        AutoFarmRingsAroundPlayer(gs, &gs->players[i], gs->economy.auto_plant_level,
                                  &planted_any, &harvested_any);
    }
    if (planted_any) Audio_Play(SFX_PLANT);
    if (harvested_any) Audio_Play(SFX_HARVEST);
    if (planted_any || harvested_any) Quest_CheckComplete(gs);
}

// Scheune: passive income regardless of what's happening on the field,
// scaled up a bit by Hof-Level so it stays relevant deep into a long session.
static void BarnTick(GameState *gs, float dt) {
    if (gs->economy.barn_level <= 0) return;
    float rate = gs->economy.barn_level * BARN_STEP * (1.0f + Progress_Level(gs) * 0.1f);
    gs->economy.barn_accum += rate * dt;
    int whole = (int)gs->economy.barn_accum;
    if (whole > 0) {
        gs->economy.money += whole;
        gs->economy.barn_accum -= (float)whole;
        Progress_AddEarnings(gs, whole);
    }
}

void World_Update(GameState *gs, float dt) {
    float weed_chance = 0.00015f * (1.0f + gs->daynight.night_factor * 2.0f)
                        * gs->economy.weed_resist_mult * Weather_WeedFactor(gs);

    for (int x = 0; x < WORLD_W; x++) {
        for (int y = 0; y < WORLD_H; y++) {
            Tile *t = &gs->world[x][y];
            if (t->pop_timer > 0.0f) t->pop_timer -= dt;
            if (t->state == TILE_EMPTY) continue;

            if (t->state == TILE_SPROUT && (float)GetRandomValue(0, 1000000) / 1000000.0f < weed_chance) {
                t->state = TILE_WEED;
                t->pop_timer = POP_DURATION;
                continue;
            }

            if (t->state == TILE_SPROUT || t->state == TILE_RIPE) {
                t->timer -= dt;
                if (t->timer <= 0.0f) {
                    t->state = (TileState)(t->state + 1);
                    t->timer = World_StageDuration(gs, t->crop_type);
                    t->pop_timer = POP_DURATION;
                }
            }
        }
    }

    AutoPlantTick(gs, dt);
    BarnTick(gs, dt);
}

void World_UpdateFacingTile(Player *p) {
    int base_tx = (int)(p->x / TILE_DRAW);
    int base_ty = (int)(p->y / TILE_DRAW);
    int tx = base_tx, ty = base_ty;

    switch (p->facing) {
        case FACE_DOWN:  if (base_ty < WORLD_H - 1) ty = base_ty + 1; break;
        case FACE_LEFT:  if (base_tx > 0)            tx = base_tx - 1; break;
        case FACE_UP:    if (base_ty > 0)            ty = base_ty - 1; break;
        case FACE_RIGHT: if (base_tx < WORLD_W - 1)  tx = base_tx + 1; break;
    }
    p->facing_tile_x = tx;
    p->facing_tile_y = ty;
}

void World_Interact(GameState *gs, Player *p) {
    int tx = p->facing_tile_x, ty = p->facing_tile_y;
    if (tx < 0 || tx >= Field_W(gs) || ty < 0 || ty >= Field_H(gs)) {
        Audio_Play(SFX_DENY);
        return;
    }
    Tile *t = &gs->world[tx][ty];

    switch (t->state) {
        case TILE_EMPTY:
            if (gs->economy.money >= 1) {
                gs->economy.money -= 1;
                t->state = TILE_SPROUT;
                t->crop_type = gs->economy.crop_active;
                t->timer = World_StageDuration(gs, t->crop_type);
                t->pop_timer = POP_DURATION;
                gs->stats.total_planted++;
                Quest_Notify(gs, QUEST_PLANT, 1);
                Audio_Play(SFX_PLANT);
            } else {
                Audio_Play(SFX_DENY);
            }
            break;
        case TILE_SPROUT:
            t->state = TILE_EMPTY; // pulled up too early, nothing gained
            t->pop_timer = POP_DURATION;
            break;
        case TILE_RIPE: {
            int payout = (int)(10 * gs->economy.crop_track[t->crop_type].sell_mult * Crop_Mult(t->crop_type) * Weather_SellMult(gs));
            gs->economy.money += payout;
            int crop = t->crop_type;
            t->state = TILE_EMPTY;
            t->pop_timer = POP_DURATION;
            gs->stats.total_harvested++;
            Quest_Notify(gs, QUEST_HARVEST, 1);
            Quest_Notify(gs, QUEST_EARN, payout);
            Progress_AddEarnings(gs, payout);
            Contracts_Notify(gs, crop, 1);
            Audio_Play(SFX_HARVEST);
            break;
        }
        case TILE_OVERRIPE:
            t->state = TILE_EMPTY; // missed the window, nothing gained or lost
            t->pop_timer = POP_DURATION;
            break;
        case TILE_WEED: {
            int loss = GetRandomValue(1, 30);
            gs->economy.money -= loss;
            t->state = TILE_EMPTY;
            t->pop_timer = POP_DURATION;
            Audio_Play(SFX_WEED);
            break;
        }
    }
    Quest_CheckComplete(gs);
}
