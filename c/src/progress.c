#include "farm.h"
#include <stdio.h>

// The long-term reason to keep farming: a lifetime Hof-Level/title ladder and
// a handful of achievements, independent from the current farm session so
// starting a new game never erases what you have already accomplished.

typedef struct {
    int threshold;
    const char *title;
} LevelDef;

// Thresholds go all the way into the tens of millions now - lifetime earnings
// legitimately reach that range once crop-tier prestige (1-6M+ per unlock,
// 3x/9x/27x payout afterwards) kicks in, and the old 6-tier ladder capping out
// at 3000 was trivially maxed out within the first few in-game days.
static const LevelDef LEVELS[] = {
    { 0,         "Hobbygaertner" },
    { 100,       "Bauer" },
    { 300,       "Hofbesitzer" },
    { 700,       "Gutsherr" },
    { 1500,      "Agrar-Baron" },
    { 3000,      "Farm-Legende" },
    { 6000,      "Ernte-Meister" },
    { 12000,     "Feldherr" },
    { 25000,     "Agrar-Mogul" },
    { 50000,     "Gemuese-Tycoon" },
    { 100000,    "Farm-Imperator" },
    { 200000,    "Ernte-Kaiser" },
    { 400000,    "Agrar-Titan" },
    { 800000,    "Gemuese-Koenig" },
    { 1500000,   "Farm-Gottheit" },
    { 3000000,   "Ernte-Legende" },
    { 6000000,   "Agrar-Mythos" },
    { 12000000,  "Gemuese-Unsterblicher" },
    { 25000000,  "Farm-Universum" },
    { 50000000,  "Der Allererntende" },
    { 100000000, "Gemuese-Singularitaet" },
};
#define LEVEL_COUNT (int)(sizeof(LEVELS) / sizeof(LEVELS[0]))

typedef struct {
    const char *name;
    const char *desc;
} AchievementDef;

static const AchievementDef ACHIEVEMENTS[ACHIEVEMENT_COUNT] = {
    { "Erste Ernte",       "Ernte deine erste reife Pflanze" },
    { "Fleissiger Farmer", "Ernte insgesamt 50 Pflanzen" },
    { "Hof-Imperium",      "Verdiene insgesamt 1000 Gold" },
    { "Nachteule",         "Erreiche Tag 5" },
    { "Vollausgebaut",     "Bringe ein Upgrade auf die Maximalstufe" },
    { "Quest-Meister",     "Schliesse 10 Quests ab" },
    { "Vier Haende",       "Spiele eine Runde zu viert" },
    { "Erste Drohne",      "Kaufe deine erste Farm-Drohne" },
    { "Vollautomatisiert", "Maximiere Auto-Farmarbeit sowie alle Drohnen und Maeher" },
    { "Vier Sorten",       "Schalte alle Gemuesesorten frei" },
};

void Progress_Init(GameState *gs) {
    gs->progress.total_money_earned = 0;
    gs->progress.quests_completed = 0;
    gs->progress.ever_four_players = false;
    for (int i = 0; i < ACHIEVEMENT_COUNT; i++) gs->progress.achievements[i] = false;
}

void Progress_AddEarnings(GameState *gs, int amount) {
    if (amount > 0) gs->progress.total_money_earned += amount;
}

int Progress_Level(GameState *gs) {
    int level = 1;
    for (int i = 0; i < LEVEL_COUNT; i++) {
        if (gs->progress.total_money_earned >= LEVELS[i].threshold) level = i + 1;
    }
    return level;
}

const char *Progress_Title(GameState *gs) {
    int idx = Progress_Level(gs) - 1;
    if (idx >= LEVEL_COUNT) idx = LEVEL_COUNT - 1;
    return LEVELS[idx].title;
}

float Progress_LevelProgress(GameState *gs) {
    int idx = Progress_Level(gs) - 1;
    if (idx >= LEVEL_COUNT - 1) return 1.0f;
    int cur = LEVELS[idx].threshold;
    int next = LEVELS[idx + 1].threshold;
    float frac = (float)(gs->progress.total_money_earned - cur) / (float)(next - cur);
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;
    return frac;
}

const char *Achievement_Name(int id) {
    if (id < 0 || id >= ACHIEVEMENT_COUNT) return "";
    return ACHIEVEMENTS[id].name;
}

const char *Achievement_Desc(int id) {
    if (id < 0 || id >= ACHIEVEMENT_COUNT) return "";
    return ACHIEVEMENTS[id].desc;
}

static void Unlock(GameState *gs, int id) {
    if (gs->progress.achievements[id]) return;
    gs->progress.achievements[id] = true;
    char msg[128];
    snprintf(msg, sizeof(msg), "Erfolg freigeschaltet: %s", ACHIEVEMENTS[id].name);
    Toast_Show(gs, msg);
    Audio_Play(SFX_ACHIEVEMENT);
}

void Progress_Update(GameState *gs) {
    if (gs->active_players >= 4) gs->progress.ever_four_players = true;

    if (gs->stats.total_harvested >= 1)          Unlock(gs, 0);
    if (gs->stats.total_harvested >= 50)         Unlock(gs, 1);
    if (gs->progress.total_money_earned >= 1000) Unlock(gs, 2);
    if (gs->daynight.day_count >= 5)             Unlock(gs, 3);
    if (gs->progress.quests_completed >= 10)     Unlock(gs, 5);
    if (gs->progress.ever_four_players)          Unlock(gs, 6);
    if (gs->drone_count >= 1)                    Unlock(gs, 7);
    if (gs->economy.auto_plant_level >= MAX_AUTO_PLANT_LEVEL &&
        gs->drone_count >= MAX_DRONES &&
        gs->mower_count >= MAX_MOWERS)           Unlock(gs, 8);
    // "Four crops" means the four named ones (Weizen/Kartoffeln/Karotten/
    // Zucchini, tier index 3) - the endless mystery tiers beyond that are
    // deliberate late-game-forever content, not something this achievement
    // should gate on.
    if (gs->economy.crop_unlocked >= 3) Unlock(gs, 9);

    bool any_maxed = gs->economy.speed_level >= MAX_UPGRADE_LEVEL;
    for (int i = 0; i <= gs->economy.crop_unlocked && !any_maxed; i++) {
        if (gs->economy.crop_track[i].growth_level >= MAX_UPGRADE_LEVEL) any_maxed = true;
        if (gs->economy.crop_track[i].sell_level >= MAX_UPGRADE_LEVEL) any_maxed = true;
    }
    if (any_maxed) Unlock(gs, 4);
}
