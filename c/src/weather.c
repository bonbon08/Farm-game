#include "farm.h"
#include <stdio.h>

static const char *WEATHER_NAMES[WEATHER_COUNT] = {
    "Klar", "Regen", "Duerre", "Sturm", "Erntesegen"
};

const char *Weather_Name(WeatherType type) {
    if (type < 0 || type >= WEATHER_COUNT) return "?";
    return WEATHER_NAMES[type];
}

void Weather_Init(GameState *gs) {
    gs->weather.type = WEATHER_CLEAR;
    gs->weather.timer = (float)GetRandomValue((int)WEATHER_MIN_DURATION, (int)WEATHER_MAX_DURATION);
}

void Weather_Update(GameState *gs, float dt) {
    gs->weather.timer -= dt;
    if (gs->weather.timer > 0.0f) return;

    // Weighted roll: clear weather is by far the most common, so the other
    // four stay noteworthy events rather than becoming the new normal.
    int roll = GetRandomValue(0, 99);
    WeatherType next;
    if (roll < 45)      next = WEATHER_CLEAR;
    else if (roll < 65) next = WEATHER_RAIN;
    else if (roll < 80) next = WEATHER_DROUGHT;
    else if (roll < 93) next = WEATHER_STORM;
    else                next = WEATHER_BLESSING;

    gs->weather.type = next;
    gs->weather.timer = (float)GetRandomValue((int)WEATHER_MIN_DURATION, (int)WEATHER_MAX_DURATION);

    if (next != WEATHER_CLEAR) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Wetter: %s!", Weather_Name(next));
        Toast_Show(gs, msg);
        Audio_Play(next == WEATHER_BLESSING ? SFX_ACHIEVEMENT : SFX_SHOP_TOGGLE);
    }
}

float Weather_GrowthFactor(GameState *gs) {
    switch (gs->weather.type) {
        case WEATHER_RAIN:    return WEATHER_RAIN_GROWTH_FACTOR;
        case WEATHER_DROUGHT: return WEATHER_DROUGHT_GROWTH_FACTOR;
        default:              return 1.0f;
    }
}

float Weather_WeedFactor(GameState *gs) {
    return (gs->weather.type == WEATHER_RAIN) ? WEATHER_RAIN_WEED_FACTOR : 1.0f;
}

float Weather_BirdFactor(GameState *gs) {
    return (gs->weather.type == WEATHER_STORM) ? WEATHER_STORM_BIRD_FACTOR : 1.0f;
}

float Weather_SellMult(GameState *gs) {
    return (gs->weather.type == WEATHER_BLESSING) ? WEATHER_BLESSING_SELL_MULT : 1.0f;
}
