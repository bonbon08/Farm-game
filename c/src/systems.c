#include "farm.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

void Toast_Show(GameState *gs, const char *text) {
    strncpy(gs->toast.text, text, sizeof(gs->toast.text) - 1);
    gs->toast.text[sizeof(gs->toast.text) - 1] = '\0';
    gs->toast.timer = 3.0f;
}

void Toast_Update(GameState *gs, float dt) {
    if (gs->toast.timer > 0.0f) gs->toast.timer -= dt;
}

static void QuestApply(GameState *gs, QuestType type, int index) {
    gs->quest.type = type;
    gs->quest.index = index;
    gs->quest.reward = 20 + index * 15;
    gs->quest.baseline_money = gs->economy.money;

    switch (type) {
        case QUEST_HARVEST:
            gs->quest.target = 5 + index * 3;
            snprintf(gs->quest.description, sizeof(gs->quest.description),
                     "Ernte %d reife Pflanzen", gs->quest.target);
            break;
        case QUEST_PLANT:
            gs->quest.target = 5 + index * 3;
            snprintf(gs->quest.description, sizeof(gs->quest.description),
                     "Pflanze %d Samen", gs->quest.target);
            break;
        case QUEST_EARN:
            gs->quest.target = 50 + index * 40;
            snprintf(gs->quest.description, sizeof(gs->quest.description),
                     "Verdiene %d Gold durch Ernten", gs->quest.target);
            break;
        default: break;
    }
}

void Quest_Init(GameState *gs, int index) {
    QuestType prev = gs->quest.type;
    QuestType type = (QuestType)GetRandomValue(0, QUEST_TYPE_COUNT - 1);
    if (index > 0) {
        int guard = 0;
        while (type == prev && guard++ < 5) type = (QuestType)GetRandomValue(0, QUEST_TYPE_COUNT - 1);
    }
    gs->quest.progress = 0;
    QuestApply(gs, type, index);
}

void Quest_Restore(GameState *gs, QuestType type, int index, int progress) {
    QuestApply(gs, type, index);
    gs->quest.progress = progress;
}

void Quest_Notify(GameState *gs, QuestType type, int amount) {
    if (gs->quest.type != type) return;
    gs->quest.progress += amount;
}

void Quest_CheckComplete(GameState *gs) {
    if (gs->quest.progress < gs->quest.target) return;
    gs->economy.money += gs->quest.reward;
    gs->progress.quests_completed++;
    Progress_AddEarnings(gs, gs->quest.reward);
    char msg[128];
    snprintf(msg, sizeof(msg), "Quest erfuellt! +%d Gold", gs->quest.reward);
    Toast_Show(gs, msg);
    Audio_Play(SFX_QUEST_COMPLETE);
    Quest_Init(gs, gs->quest.index + 1);
}

void DayNight_Update(GameState *gs, float dt) {
    DayNight *d = &gs->daynight;
    d->time_of_day += dt / d->day_length;
    if (d->time_of_day >= 1.0f) {
        d->time_of_day -= 1.0f;
        d->day_count++;
    }
    float daylight = 0.5f + 0.5f * cosf(d->time_of_day * 2.0f * PI);
    d->night_factor = 1.0f - daylight;
}

void Shop_ToggleRequested(GameState *gs) {
    bool pressed = false;
    for (int i = 0; i < gs->active_players; i++) {
        Player *p = &gs->players[i];
        if (p->scheme == INPUT_KEYBOARD_MOUSE) {
            if (IsKeyPressed(p->k_menu)) pressed = true;
        } else if (IsGamepadAvailable(p->gamepad_index)) {
            if (IsGamepadButtonPressed(p->gamepad_index, GAMEPAD_BUTTON_MIDDLE_RIGHT)) pressed = true;
        }
    }
    if (pressed) {
        gs->shop.open = !gs->shop.open;
        gs->shop.selection = 0;
        Audio_Play(SFX_SHOP_TOGGLE);
    }
    if (gs->shop.open && IsKeyPressed(KEY_ESCAPE)) {
        gs->shop.open = false;
        Audio_Play(SFX_SHOP_TOGGLE);
    }
}

// The shop outgrew a single scrollable column once defense/economy/prestige
// upgrades joined the original list, so rows now flow into two columns side
// by side instead - column 0 takes the first SHOP_COL0_COUNT entries, column
// 1 takes the rest.
#define SHOP_COL0_COUNT 9
#define SHOP_PANEL_W 1200
#define SHOP_PANEL_H 680
#define SHOP_ROW_Y0  54
#define SHOP_ROW_STEP 56
#define SHOP_ROW_W   540
#define SHOP_ROW_H   50
#define SHOP_COL_GAP 20

Rectangle Shop_PanelRect(void) {
    int px = WINDOW_W / 2 - SHOP_PANEL_W / 2;
    int py = WINDOW_H / 2 - SHOP_PANEL_H / 2;
    return (Rectangle){ (float)px, (float)py, (float)SHOP_PANEL_W, (float)SHOP_PANEL_H };
}

Rectangle Shop_RowRect(int index) {
    Rectangle panel = Shop_PanelRect();
    int col = index < SHOP_COL0_COUNT ? 0 : 1;
    int row = index < SHOP_COL0_COUNT ? index : index - SHOP_COL0_COUNT;
    float x = panel.x + 20.0f + col * (SHOP_ROW_W + SHOP_COL_GAP);
    float y = panel.y + SHOP_ROW_Y0 + row * SHOP_ROW_STEP;
    return (Rectangle){ x, y, (float)SHOP_ROW_W, (float)SHOP_ROW_H };
}

int Shop_UpgradeCost(int level) {
    // Gentler curve than the old 10-level ladder (1.6x/level would be
    // astronomical by level 30) - still a real late-game grind, just reachable.
    return (int)(20.0f * powf(1.18f, (float)level));
}

void Shop_Buy(GameState *gs, int which) {
    Economy *e = &gs->economy;

    if (which == 4) { Drones_Buy(gs); return; }
    if (which == 6) { Mowers_Buy(gs); return; }
    if (which == 10) { Crops_Unlock(gs); return; }
    if (which == 15) { Dogs_Buy(gs); return; }

    if (which == 16) {
        int broken = 0;
        for (int i = 0; i < gs->drone_count; i++) if (gs->drones[i].broken) broken++;
        for (int i = 0; i < gs->mower_count; i++) if (gs->mowers[i].broken) broken++;
        if (broken == 0) return;
        int cost = broken * UNIT_REPAIR_COST_EACH;
        if (e->money < cost) { Audio_Play(SFX_DENY); return; }
        e->money -= cost;
        for (int i = 0; i < gs->drone_count; i++) {
            if (gs->drones[i].broken) { gs->drones[i].broken = false; gs->drones[i].wear = 0.0f; }
        }
        for (int i = 0; i < gs->mower_count; i++) {
            if (gs->mowers[i].broken) { gs->mowers[i].broken = false; gs->mowers[i].wear = 0.0f; }
        }
        Toast_Show(gs, "Alle kaputten Geraete repariert!");
        Audio_Play(SFX_UPGRADE);
        return;
    }

    if (which == 3) {
        if (e->auto_plant_level >= MAX_AUTO_PLANT_LEVEL) return;
        int cost = Shop_UpgradeCost(e->auto_plant_level);
        if (e->money < cost) { Audio_Play(SFX_DENY); return; }
        e->money -= cost;
        e->auto_plant_level++;
        Toast_Show(gs, "Auto-Farmarbeit verbessert!");
        Audio_Play(SFX_UPGRADE);
        return;
    }
    if (which == 8) {
        if (e->mower_range_level >= MAX_MOWER_RANGE_LEVEL) return;
        int cost = Shop_UpgradeCost(e->mower_range_level);
        if (e->money < cost) { Audio_Play(SFX_DENY); return; }
        e->money -= cost;
        e->mower_range_level++;
        Toast_Show(gs, "Maeher-Reichweite verbessert!");
        Audio_Play(SFX_UPGRADE);
        return;
    }
    if (which == 9) {
        if (e->weed_resist_level >= MAX_UPGRADE_LEVEL) return;
        int cost = Shop_UpgradeCost(e->weed_resist_level);
        if (e->money < cost) { Audio_Play(SFX_DENY); return; }
        e->money -= cost;
        e->weed_resist_level++;
        e->weed_resist_mult = 1.0f - e->weed_resist_level * WEED_RESIST_STEP;
        if (e->weed_resist_mult < WEED_RESIST_FLOOR) e->weed_resist_mult = WEED_RESIST_FLOOR;
        Toast_Show(gs, "Unkraut-Resistenz verbessert!");
        Audio_Play(SFX_UPGRADE);
        return;
    }
    if (which == 11) {
        if (e->barn_level >= MAX_BARN_LEVEL) return;
        int cost = Shop_UpgradeCost(e->barn_level);
        if (e->money < cost) { Audio_Play(SFX_DENY); return; }
        e->money -= cost;
        e->barn_level++;
        Toast_Show(gs, "Scheune ausgebaut!");
        Audio_Play(SFX_UPGRADE);
        return;
    }
    if (which == 12) {
        if (e->field_level >= MAX_FIELD_LEVEL) return;
        int cost = Shop_UpgradeCost(e->field_level);
        if (e->money < cost) { Audio_Play(SFX_DENY); return; }
        e->money -= cost;
        e->field_level++;
        Toast_Show(gs, "Feld erweitert!");
        Audio_Play(SFX_UPGRADE);
        return;
    }
    if (which == 13) {
        if (e->drone_swarm_level >= MAX_DRONE_SWARM_LEVEL) return;
        int cost = Shop_UpgradeCost(e->drone_swarm_level);
        if (e->money < cost) { Audio_Play(SFX_DENY); return; }
        e->money -= cost;
        e->drone_swarm_level++;
        Toast_Show(gs, "Drohnen-Schwarmradius verbessert!");
        Audio_Play(SFX_UPGRADE);
        return;
    }
    if (which == 14) {
        if (e->scarecrow_level >= MAX_SCARECROW_LEVEL) return;
        int cost = Shop_UpgradeCost(e->scarecrow_level);
        if (e->money < cost) { Audio_Play(SFX_DENY); return; }
        e->money -= cost;
        e->scarecrow_level++;
        e->scarecrow_mult = 1.0f - e->scarecrow_level * SCARECROW_STEP;
        if (e->scarecrow_mult < SCARECROW_FLOOR) e->scarecrow_mult = SCARECROW_FLOOR;
        Toast_Show(gs, "Vogelscheuche verbessert!");
        Audio_Play(SFX_UPGRADE);
        return;
    }

    // Growth and sell price are tracked per crop tier - they operate on
    // whichever crop is currently active, and that tier's own progress is
    // exactly what stays parked when the player switches to a different one.
    if (which == 1 || which == 2) {
        CropTrack *ct = &e->crop_track[e->crop_active];
        int *level = (which == 1) ? &ct->growth_level : &ct->sell_level;
        float *mult = (which == 1) ? &ct->growth_mult : &ct->sell_mult;
        float step = (which == 1) ? GROWTH_STEP : SELL_STEP;
        if (*level >= MAX_UPGRADE_LEVEL) return;
        int cost = Shop_UpgradeCost(*level);
        if (e->money < cost) { Audio_Play(SFX_DENY); return; }
        e->money -= cost;
        (*level)++;
        *mult = 1.0f + (*level) * step;
        Toast_Show(gs, "Upgrade gekauft!");
        Audio_Play(SFX_UPGRADE);
        return;
    }

    int *level = NULL;
    float *mult = NULL;
    float step = 0.0f;

    switch (which) {
        case 0: level = &e->speed_level;       mult = &e->speed_mult;       step = SPEED_STEP;       break;
        case 5: level = &e->drone_speed_level; mult = &e->drone_speed_mult; step = DRONE_SPEED_STEP; break;
        case 7: level = &e->mower_speed_level;  mult = &e->mower_speed_mult; step = MOWER_SPEED_STEP; break;
        default: return;
    }

    if (*level >= MAX_UPGRADE_LEVEL) return;
    int cost = Shop_UpgradeCost(*level);
    if (e->money < cost) {
        Audio_Play(SFX_DENY);
        return;
    }

    e->money -= cost;
    (*level)++;
    *mult = 1.0f + (*level) * step;

    Toast_Show(gs, "Upgrade gekauft!");
    Audio_Play(SFX_UPGRADE);
}

void Shop_HandleInput(GameState *gs) {
    if (!gs->shop.open) return;

    Vector2 mouse = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !CheckCollisionPointRec(mouse, Shop_PanelRect())) {
        gs->shop.open = false;
        Audio_Play(SFX_SHOP_TOGGLE);
        return;
    }

    bool up = IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP);
    bool down = IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN);
    bool confirm = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER);

    for (int i = 0; i < gs->active_players; i++) {
        Player *p = &gs->players[i];
        if (p->scheme != INPUT_GAMEPAD || !IsGamepadAvailable(p->gamepad_index)) continue;
        if (IsGamepadButtonPressed(p->gamepad_index, GAMEPAD_BUTTON_LEFT_FACE_UP)) up = true;
        if (IsGamepadButtonPressed(p->gamepad_index, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) down = true;
        if (IsGamepadButtonPressed(p->gamepad_index, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) confirm = true;
    }

    for (int i = 0; i < SHOP_ITEM_COUNT; i++) {
        if (!CheckCollisionPointRec(mouse, Shop_RowRect(i))) continue;
        if (gs->shop.selection != i) {
            gs->shop.selection = i;
            Audio_Play(SFX_UI_MOVE);
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) confirm = true;
        break;
    }

    if (up)   { gs->shop.selection = (gs->shop.selection + SHOP_ITEM_COUNT - 1) % SHOP_ITEM_COUNT; Audio_Play(SFX_UI_MOVE); }
    if (down) { gs->shop.selection = (gs->shop.selection + 1) % SHOP_ITEM_COUNT; Audio_Play(SFX_UI_MOVE); }
    if (confirm) Shop_Buy(gs, gs->shop.selection);
}

void Systems_Init(GameState *gs) {
    gs->economy.money = 100;
    gs->economy.speed_level = 0;
    gs->economy.speed_mult = 1.0f;

    for (int i = 0; i < MAX_CROP_TIERS; i++) {
        gs->economy.crop_track[i] = (CropTrack){ 0, 1.0f, 0, 1.0f };
    }
    gs->economy.crop_unlocked = 0;
    gs->economy.crop_active = 0;

    gs->economy.auto_plant_level = 0;
    gs->economy.auto_plant_cd = 0.0f;
    gs->economy.drone_speed_level = 0;
    gs->economy.drone_speed_mult = 1.0f;
    gs->economy.drone_swarm_level = 0;
    Drones_Init(gs);

    gs->economy.mower_speed_level = 0;
    gs->economy.mower_speed_mult = 1.0f;
    gs->economy.mower_range_level = 0;
    gs->economy.weed_resist_level = 0;
    gs->economy.weed_resist_mult = 1.0f;
    Mowers_Init(gs);

    gs->economy.barn_level = 0;
    gs->economy.barn_accum = 0.0f;

    gs->economy.field_level = 0;

    gs->economy.scarecrow_level = 0;
    gs->economy.scarecrow_mult = 1.0f;
    Pests_Init(gs);

    gs->daynight.day_length = 90.0f;
    gs->daynight.time_of_day = 0.0f; // start at full daylight (noon of the cosine curve)
    gs->daynight.day_count = 1;
    gs->daynight.night_factor = 0.0f;

    Weather_Init(gs);
    Contracts_Init(gs);

    gs->stats.total_harvested = 0;
    gs->stats.total_planted = 0;

    gs->toast.timer = 0.0f;
    gs->toast.text[0] = '\0';

    gs->shop.open = false;
    gs->shop.selection = 0;

    gs->quest.type = QUEST_HARVEST; // placeholder, overwritten below
    Quest_Init(gs, 0);
}
