#include "farm.h"
#include <math.h>

#define DRONE_SPEED 110.0f
#define DRONE_ACTION_TIME 0.4f
#define DRONE_ACTION_TIME_OVERRIPE (DRONE_ACTION_TIME * 2.5f) // salvaging overripe crops is slower work
#define DRONE_ARRIVE_DIST 6.0f

void Drones_Init(GameState *gs) {
    gs->drone_count = 0;
    for (int i = 0; i < MAX_DRONES; i++) {
        gs->drones[i] = (Drone){ 0 };
    }
}

int Drones_Cost(GameState *gs) {
    if (gs->drone_count >= MAX_DRONES) return -1;
    return (int)(80.0f * powf(1.3f, (float)gs->drone_count));
}

static Drone MakeDrone(void) {
    Drone d = { 0 };
    d.active = true;
    d.x = WORLD_PX_W * 0.5f + (float)GetRandomValue(-150, 150);
    d.y = WORLD_PX_H * 0.5f + (float)GetRandomValue(-150, 150);
    d.has_target = false;
    d.action_timer = 0.0f;
    d.bob_seed = (float)GetRandomValue(0, 1000) / 100.0f;
    d.broken = false;
    d.wear = 0.0f;
    return d;
}

bool Drones_Buy(GameState *gs) {
    int cost = Drones_Cost(gs);
    if (cost < 0) return false;
    if (gs->economy.money < cost) {
        Audio_Play(SFX_DENY);
        return false;
    }
    gs->economy.money -= cost;
    gs->drones[gs->drone_count] = MakeDrone();
    gs->drone_count++;
    Toast_Show(gs, "Drohne gekauft!");
    Audio_Play(SFX_UPGRADE);
    return true;
}

void Drones_SetCount(GameState *gs, int count) {
    if (count < 0) count = 0;
    if (count > MAX_DRONES) count = MAX_DRONES;
    gs->drone_count = count;
    for (int i = 0; i < count; i++) {
        gs->drones[i] = MakeDrone();
    }
}

// True if some other active drone is already headed for this exact tile -
// keeps several drones from all converging on the same single target.
static bool IsTileClaimed(GameState *gs, int tx, int ty, int exclude_index) {
    for (int j = 0; j < gs->drone_count; j++) {
        if (j == exclude_index) continue;
        Drone *other = &gs->drones[j];
        if (other->active && other->has_target && other->target_x == tx && other->target_y == ty) return true;
    }
    return false;
}

// Scans the currently unlocked field for the nearest unclaimed tile matching
// `state`, relative to (from_x, from_y). Returns false if none exists.
static bool FindNearest(GameState *gs, float from_x, float from_y, int exclude_index,
                        TileState state, int *out_x, int *out_y) {
    float best_d = 1e18f;
    int bx = -1, by = -1;
    int fw = Field_W(gs), fh = Field_H(gs);

    for (int x = 0; x < fw; x++) {
        for (int y = 0; y < fh; y++) {
            if (gs->world[x][y].state != state) continue;
            if (IsTileClaimed(gs, x, y, exclude_index)) continue;
            float cx = x * TILE_DRAW + TILE_DRAW / 2.0f;
            float cy = y * TILE_DRAW + TILE_DRAW / 2.0f;
            float dx = cx - from_x, dy = cy - from_y;
            float d = dx * dx + dy * dy;
            if (d < best_d) { best_d = d; bx = x; by = y; }
        }
    }
    if (bx < 0) return false;
    *out_x = bx;
    *out_y = by;
    return true;
}

// Priority: harvest ripe crops (earns money, fast) > salvage overripe crops
// (same payout, but slower to process - see DRONE_ACTION_TIME_OVERRIPE below)
// > plant empty ground (costs the usual seed price). Weed removal is the
// mowers' job, not the drones'.
static bool FindTarget(GameState *gs, float from_x, float from_y, int exclude_index,
                       int *out_x, int *out_y, DroneTask *out_task) {
    if (FindNearest(gs, from_x, from_y, exclude_index, TILE_RIPE, out_x, out_y)) {
        *out_task = DRONE_TASK_HARVEST;
        return true;
    }
    if (FindNearest(gs, from_x, from_y, exclude_index, TILE_OVERRIPE, out_x, out_y)) {
        *out_task = DRONE_TASK_HARVEST_OVERRIPE;
        return true;
    }
    if (gs->economy.money >= 1 && FindNearest(gs, from_x, from_y, exclude_index, TILE_EMPTY, out_x, out_y)) {
        *out_task = DRONE_TASK_PLANT;
        return true;
    }
    return false;
}

// "Schwarmradius" - after the primary action, also sweep up any matching
// tiles within drone_swarm_level of wherever the drone is already standing,
// instead of only ever handling exactly one tile per stop. The next place to
// keep investing once drone count itself has maxed out at MAX_DRONES.
static void SwarmSweep(GameState *gs, int cx, int cy, TileState want_state, DroneTask task) {
    int range = gs->economy.drone_swarm_level;
    if (range <= 0) return;
    int fw = Field_W(gs), fh = Field_H(gs);

    for (int dx = -range; dx <= range; dx++) {
        for (int dy = -range; dy <= range; dy++) {
            if (dx == 0 && dy == 0) continue;
            int tx = cx + dx, ty = cy + dy;
            if (tx < 0 || tx >= fw || ty < 0 || ty >= fh) continue;
            Tile *t = &gs->world[tx][ty];
            if (t->state != want_state) continue;

            if (task == DRONE_TASK_HARVEST || task == DRONE_TASK_HARVEST_OVERRIPE) {
                int payout = (int)(10 * gs->economy.crop_track[t->crop_type].sell_mult * Crop_Mult(t->crop_type) * Weather_SellMult(gs));
                gs->economy.money += payout;
                int crop = t->crop_type;
                t->state = TILE_EMPTY;
                t->pop_timer = 0.3f;
                gs->stats.total_harvested++;
                Quest_Notify(gs, QUEST_HARVEST, 1);
                Quest_Notify(gs, QUEST_EARN, payout);
                Progress_AddEarnings(gs, payout);
                Contracts_Notify(gs, crop, 1);
            } else if (task == DRONE_TASK_PLANT && gs->economy.money >= 1) {
                gs->economy.money -= 1;
                t->state = TILE_SPROUT;
                t->crop_type = gs->economy.crop_active;
                t->timer = World_StageDuration(gs, t->crop_type);
                t->pop_timer = 0.3f;
                gs->stats.total_planted++;
                Quest_Notify(gs, QUEST_PLANT, 1);
            }
        }
    }
}

void Drones_Update(GameState *gs, float dt) {
    for (int i = 0; i < gs->drone_count; i++) {
        Drone *d = &gs->drones[i];
        if (!d->active || d->broken) continue;

        if (d->action_timer > 0.0f) {
            d->action_timer -= dt;
            continue;
        }

        if (!d->has_target) {
            int tx, ty;
            DroneTask task;
            if (FindTarget(gs, d->x, d->y, i, &tx, &ty, &task)) {
                d->target_x = tx;
                d->target_y = ty;
                d->task = task;
                d->has_target = true;
            } else {
                continue;
            }
        }

        float target_px = d->target_x * TILE_DRAW + TILE_DRAW / 2.0f - PLAYER_SIZE / 2.0f;
        float target_py = d->target_y * TILE_DRAW + TILE_DRAW / 2.0f - PLAYER_SIZE / 2.0f;
        float dx = target_px - d->x, dy = target_py - d->y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist < DRONE_ARRIVE_DIST) {
            Tile *t = &gs->world[d->target_x][d->target_y];
            bool acted = false;
            switch (d->task) {
                case DRONE_TASK_HARVEST:
                    if (t->state == TILE_RIPE) {
                        int payout = (int)(10 * gs->economy.crop_track[t->crop_type].sell_mult * Crop_Mult(t->crop_type) * Weather_SellMult(gs));
                        gs->economy.money += payout;
                        int crop = t->crop_type;
                        t->state = TILE_EMPTY;
                        t->pop_timer = 0.3f;
                        gs->stats.total_harvested++;
                        Quest_Notify(gs, QUEST_HARVEST, 1);
                        Quest_Notify(gs, QUEST_EARN, payout);
                        Progress_AddEarnings(gs, payout);
                        Contracts_Notify(gs, crop, 1);
                        Audio_Play(SFX_HARVEST);
                        Quest_CheckComplete(gs);
                        SwarmSweep(gs, d->target_x, d->target_y, TILE_RIPE, d->task);
                        acted = true;
                    }
                    break;
                case DRONE_TASK_HARVEST_OVERRIPE:
                    if (t->state == TILE_OVERRIPE) {
                        int payout = (int)(10 * gs->economy.crop_track[t->crop_type].sell_mult * Crop_Mult(t->crop_type) * Weather_SellMult(gs));
                        gs->economy.money += payout;
                        int crop = t->crop_type;
                        t->state = TILE_EMPTY;
                        t->pop_timer = 0.3f;
                        gs->stats.total_harvested++;
                        Quest_Notify(gs, QUEST_HARVEST, 1);
                        Quest_Notify(gs, QUEST_EARN, payout);
                        Progress_AddEarnings(gs, payout);
                        Contracts_Notify(gs, crop, 1);
                        Audio_Play(SFX_HARVEST);
                        Quest_CheckComplete(gs);
                        SwarmSweep(gs, d->target_x, d->target_y, TILE_OVERRIPE, d->task);
                        acted = true;
                    }
                    break;
                case DRONE_TASK_PLANT:
                    if (t->state == TILE_EMPTY && gs->economy.money >= 1) {
                        gs->economy.money -= 1;
                        t->state = TILE_SPROUT;
                        t->crop_type = gs->economy.crop_active;
                        t->timer = World_StageDuration(gs, t->crop_type);
                        t->pop_timer = 0.3f;
                        gs->stats.total_planted++;
                        Quest_Notify(gs, QUEST_PLANT, 1);
                        Audio_Play(SFX_PLANT);
                        Quest_CheckComplete(gs);
                        SwarmSweep(gs, d->target_x, d->target_y, TILE_EMPTY, d->task);
                        acted = true;
                    }
                    break;
            }
            if (acted) {
                d->wear += (float)GetRandomValue((int)(UNIT_WEAR_PER_ACTION_MIN * 10), (int)(UNIT_WEAR_PER_ACTION_MAX * 10)) / 10.0f;
                if (d->wear >= UNIT_WEAR_MAX) {
                    d->broken = true;
                    Toast_Show(gs, "Eine Drohne ist kaputt gegangen!");
                }
            }
            float base_action_time = (d->task == DRONE_TASK_HARVEST_OVERRIPE) ? DRONE_ACTION_TIME_OVERRIPE : DRONE_ACTION_TIME;
            d->has_target = false;
            d->action_timer = base_action_time / gs->economy.drone_speed_mult;
        } else {
            // Clamp the step to the remaining distance - otherwise a high
            // enough speed*dt can overshoot the target every frame and bounce
            // back and forth forever without ever registering as "arrived".
            float speed = DRONE_SPEED * gs->economy.drone_speed_mult;
            float step = speed * dt;
            if (step >= dist) {
                d->x = target_px;
                d->y = target_py;
            } else {
                d->x += dx / dist * step;
                d->y += dy / dist * step;
            }
        }
    }
}
