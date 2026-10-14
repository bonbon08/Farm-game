#include "farm.h"
#include <stdio.h>

void Contracts_Init(GameState *gs) {
    gs->contract.active = false;
    gs->contract_spawn_timer = (float)GetRandomValue((int)CONTRACT_MIN_SPAWN_DELAY, (int)CONTRACT_MAX_SPAWN_DELAY);
}

// A separate, time-pressured bonus objective for one specific crop tier -
// distinct from the regular endless quest chain, which never expires and
// never demands a particular crop. Only one runs at a time; missing it just
// costs the bonus, not anything the player already had.
void Contracts_Update(GameState *gs, float dt) {
    if (gs->contract.active) {
        gs->contract.time_left -= dt;
        if (gs->contract.time_left <= 0.0f) {
            gs->contract.active = false;
            Toast_Show(gs, "Grossauftrag verpasst!");
            gs->contract_spawn_timer = (float)GetRandomValue((int)CONTRACT_MIN_SPAWN_DELAY, (int)CONTRACT_MAX_SPAWN_DELAY);
        }
        return;
    }

    gs->contract_spawn_timer -= dt;
    if (gs->contract_spawn_timer > 0.0f) return;
    gs->contract_spawn_timer = (float)GetRandomValue((int)CONTRACT_MIN_SPAWN_DELAY, (int)CONTRACT_MAX_SPAWN_DELAY);

    int tier = GetRandomValue(0, gs->economy.crop_unlocked);
    int target = GetRandomValue(CONTRACT_MIN_TARGET, CONTRACT_MAX_TARGET);
    int reward = (int)(target * CONTRACT_REWARD_PER_UNIT * Crop_Mult(tier));

    gs->contract.active = true;
    gs->contract.crop_type = tier;
    gs->contract.target = target;
    gs->contract.progress = 0;
    gs->contract.time_left = CONTRACT_TIME_LIMIT;
    gs->contract.time_total = CONTRACT_TIME_LIMIT;
    gs->contract.reward = reward;

    char msg[96];
    snprintf(msg, sizeof(msg), "Grossauftrag: %d x %s in %ds!", target, Crop_Name(tier), (int)CONTRACT_TIME_LIMIT);
    Toast_Show(gs, msg);
    Audio_Play(SFX_SHOP_TOGGLE);
}

void Contracts_Notify(GameState *gs, int crop_type, int amount) {
    if (!gs->contract.active) return;
    if (gs->contract.crop_type != crop_type) return;

    gs->contract.progress += amount;
    if (gs->contract.progress < gs->contract.target) return;

    gs->economy.money += gs->contract.reward;
    Progress_AddEarnings(gs, gs->contract.reward);
    char msg[96];
    snprintf(msg, sizeof(msg), "Grossauftrag erfuellt! +%d Gold", gs->contract.reward);
    Toast_Show(gs, msg);
    Audio_Play(SFX_QUEST_COMPLETE);
    gs->contract.active = false;
}
