#include "farm.h"
#include <stdio.h>
#include <math.h>

#define SAVE_PATH "savegame.dat"
#define SAVE_VERSION 7

bool Save_Exists(void) {
    return FileExists(SAVE_PATH);
}

void Save_Game(GameState *gs) {
    FILE *f = fopen(SAVE_PATH, "w");
    if (!f) return;

    fprintf(f, "%d\n", SAVE_VERSION);

    fprintf(f, "%d\n", gs->active_players);
    for (int i = 0; i < gs->active_players; i++) {
        fprintf(f, "%f %f\n", gs->players[i].x, gs->players[i].y);
    }

    Economy *e = &gs->economy;
    fprintf(f, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
            e->money, e->speed_level, e->auto_plant_level,
            gs->drone_count, e->drone_speed_level, e->drone_swarm_level,
            gs->mower_count, e->mower_speed_level, e->mower_range_level,
            e->weed_resist_level, e->crop_unlocked, e->crop_active,
            e->barn_level, e->field_level, e->scarecrow_level, gs->dog_count);
    for (int i = 0; i < MAX_CROP_TIERS; i++) {
        fprintf(f, "%d %d ", e->crop_track[i].growth_level, e->crop_track[i].sell_level);
    }
    fprintf(f, "\n");

    fprintf(f, "%d %f\n", gs->daynight.day_count, gs->daynight.time_of_day);
    fprintf(f, "%d %d\n", gs->stats.total_harvested, gs->stats.total_planted);
    fprintf(f, "%d %d %d\n", (int)gs->quest.type, gs->quest.index, gs->quest.progress);

    fprintf(f, "%d %d %d\n", gs->progress.total_money_earned, gs->progress.quests_completed,
            gs->progress.ever_four_players ? 1 : 0);
    for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
        fprintf(f, "%d ", gs->progress.achievements[i] ? 1 : 0);
    }
    fprintf(f, "\n");

    for (int x = 0; x < WORLD_W; x++) {
        for (int y = 0; y < WORLD_H; y++) {
            fprintf(f, "%d %d ", (int)gs->world[x][y].state, gs->world[x][y].crop_type);
        }
    }
    fprintf(f, "\n");

    fclose(f);
}

bool Save_Load(GameState *gs) {
    FILE *f = fopen(SAVE_PATH, "r");
    if (!f) return false;

    int version = 0;
    if (fscanf(f, "%d", &version) != 1 || version != SAVE_VERSION) { fclose(f); return false; }

    int count = 2;
    if (fscanf(f, "%d", &count) != 1) { fclose(f); return false; }
    if (count < 1) count = 1;
    if (count > MAX_PLAYERS) count = MAX_PLAYERS;

    Player_InitAll(gs, count);
    for (int i = 0; i < count; i++) {
        fscanf(f, "%f %f", &gs->players[i].x, &gs->players[i].y);
    }

    Systems_Init(gs); // sets sane defaults, overridden below
    Economy *e = &gs->economy;
    int drone_count = 0, mower_count = 0, dog_count = 0;
    fscanf(f, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
           &e->money, &e->speed_level, &e->auto_plant_level,
           &drone_count, &e->drone_speed_level, &e->drone_swarm_level,
           &mower_count, &e->mower_speed_level, &e->mower_range_level,
           &e->weed_resist_level, &e->crop_unlocked, &e->crop_active,
           &e->barn_level, &e->field_level, &e->scarecrow_level, &dog_count);
    for (int i = 0; i < MAX_CROP_TIERS; i++) {
        fscanf(f, "%d %d", &e->crop_track[i].growth_level, &e->crop_track[i].sell_level);
        e->crop_track[i].growth_mult = 1.0f + e->crop_track[i].growth_level * GROWTH_STEP;
        e->crop_track[i].sell_mult = 1.0f + e->crop_track[i].sell_level * SELL_STEP;
    }
    e->speed_mult = 1.0f + e->speed_level * SPEED_STEP;
    e->drone_speed_mult = 1.0f + e->drone_speed_level * DRONE_SPEED_STEP;
    e->mower_speed_mult = 1.0f + e->mower_speed_level * MOWER_SPEED_STEP;
    e->weed_resist_mult = 1.0f - e->weed_resist_level * WEED_RESIST_STEP;
    if (e->weed_resist_mult < WEED_RESIST_FLOOR) e->weed_resist_mult = WEED_RESIST_FLOOR;
    e->scarecrow_mult = 1.0f - e->scarecrow_level * SCARECROW_STEP;
    if (e->scarecrow_mult < SCARECROW_FLOOR) e->scarecrow_mult = SCARECROW_FLOOR;
    Drones_SetCount(gs, drone_count);
    Mowers_SetCount(gs, mower_count);
    Dogs_SetCount(gs, dog_count);

    fscanf(f, "%d %f", &gs->daynight.day_count, &gs->daynight.time_of_day);
    float daylight = 0.5f + 0.5f * cosf(gs->daynight.time_of_day * 2.0f * PI);
    gs->daynight.night_factor = 1.0f - daylight;

    fscanf(f, "%d %d", &gs->stats.total_harvested, &gs->stats.total_planted);

    int qtype = 0, qindex = 0, qprogress = 0;
    fscanf(f, "%d %d %d", &qtype, &qindex, &qprogress);
    Quest_Restore(gs, (QuestType)qtype, qindex, qprogress);

    int ever4p = 0;
    fscanf(f, "%d %d %d", &gs->progress.total_money_earned, &gs->progress.quests_completed, &ever4p);
    gs->progress.ever_four_players = ever4p != 0;
    for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
        int v = 0;
        fscanf(f, "%d", &v);
        gs->progress.achievements[i] = v != 0;
    }

    for (int x = 0; x < WORLD_W; x++) {
        for (int y = 0; y < WORLD_H; y++) {
            int s = 0, ct = 0;
            fscanf(f, "%d %d", &s, &ct);
            if (s < 0 || s > TILE_WEED) s = 0;
            if (ct < 0 || ct >= MAX_CROP_TIERS) ct = 0;
            gs->world[x][y].state = (TileState)s;
            gs->world[x][y].crop_type = ct;
            gs->world[x][y].pop_timer = 0.0f;
            gs->world[x][y].timer = (s == TILE_SPROUT || s == TILE_RIPE) ? World_StageDuration(gs, ct) : 0.0f;
        }
    }

    fclose(f);
    return true;
}
