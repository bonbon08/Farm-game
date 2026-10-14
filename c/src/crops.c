#include "farm.h"
#include <math.h>
#include <stdio.h>

static const char *NAMED_CROPS[CROP_NAMED_COUNT] = { "Weizen", "Kartoffeln", "Karotten", "Zucchini" };

const char *Crop_Name(int tier) {
    static char buf[40];
    if (tier < 0) return "?";
    if (tier < CROP_NAMED_COUNT) return NAMED_CROPS[tier];
    snprintf(buf, sizeof(buf), "Mysterioeses Gemuese #%d", tier - CROP_NAMED_COUNT + 1);
    return buf;
}

// Tier 0 (the starting crop, Weizen) pays the base rate; each further tier
// pays CROP_PAYOUT_MULT times the one before it (3x, 9x, 27x, ...).
float Crop_Mult(int tier) {
    if (tier <= 0) return 1.0f;
    return powf(CROP_PAYOUT_MULT, (float)tier);
}

// Cost to unlock tier `tier` (1-indexed: tier 1 is the first paid unlock).
// Tier 1 costs CROP_FIRST_UNLOCK_COST, each further tier 2.5x the previous.
int Crop_UnlockCost(int tier) {
    if (tier <= 0) return 0;
    return (int)(CROP_FIRST_UNLOCK_COST * powf(CROP_UNLOCK_COST_MULT, (float)(tier - 1)));
}

// Unlocking a new crop costs real money but is NOT destructive: every other
// tier's growth/sell progress, and all shared infrastructure (drones, mowers,
// auto-farm, speed), is left completely untouched. The new tier just starts
// its own growth/sell track at level 0, same as every tier did.
void Crops_Unlock(GameState *gs) {
    Economy *e = &gs->economy;
    if (e->crop_unlocked >= MAX_CROP_TIERS - 1) return;

    int next_tier = e->crop_unlocked + 1;
    int cost = Crop_UnlockCost(next_tier);
    if (e->money < cost) {
        Audio_Play(SFX_DENY);
        return;
    }

    e->money -= cost;
    e->crop_unlocked = next_tier;
    e->crop_active = next_tier;

    char msg[96];
    snprintf(msg, sizeof(msg), "%s freigeschaltet!", Crop_Name(next_tier));
    Toast_Show(gs, msg);
    Audio_Play(SFX_ACHIEVEMENT);
}

// Small bottom-of-screen bar to switch the currently farmed crop, shown only
// once more than one crop is unlocked (free to switch, no cooldown). Button
// width shrinks to fit as more tiers get unlocked, down to a usable minimum.
Rectangle Crop_ButtonRect(int slot, int total) {
    float gap = 6.0f;
    float avail = WINDOW_W - 40.0f;
    float w = (avail - (total - 1) * gap) / (float)total;
    if (w > 150.0f) w = 150.0f;
    if (w < 56.0f) w = 56.0f;
    float h = 44.0f;
    float total_w = total * w + (total - 1) * gap;
    float x0 = WINDOW_W / 2.0f - total_w / 2.0f;
    return (Rectangle){ x0 + slot * (w + gap), WINDOW_H - h - 10.0f, w, h };
}

void Crops_HandleInput(GameState *gs) {
    Economy *e = &gs->economy;
    if (e->crop_unlocked <= 0) return;

    int total = e->crop_unlocked + 1;
    Vector2 mouse = GetMousePosition();
    for (int i = 0; i < total; i++) {
        Rectangle r = Crop_ButtonRect(i, total);
        if (CheckCollisionPointRec(mouse, r) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (e->crop_active != i) {
                e->crop_active = i;
                Audio_Play(SFX_UI_CLICK);
            }
            break;
        }
    }

    if (IsKeyPressed(KEY_ONE)   && total > 0) e->crop_active = 0;
    if (IsKeyPressed(KEY_TWO)   && total > 1) e->crop_active = 1;
    if (IsKeyPressed(KEY_THREE) && total > 2) e->crop_active = 2;
    if (IsKeyPressed(KEY_FOUR)  && total > 3) e->crop_active = 3;
}

// Procedural per-crop icon, used both on ripe field tiles and on the crop
// switch buttons - there's no spare art in the sprite sheet for extra crop
// types, so each tier gets a small hand-drawn vector icon instead. The first
// four tiers (the named ones) get a distinct recognizable shape each; every
// tier beyond that gets a colour that rotates with the tier index (so they're
// still visually distinguishable from each other) and a plain "mystery" mark.
void DrawCropIcon(int tier, float cx, float cy, float scale, float time) {
    if (tier == 0) { // Weizen: three golden stalks with ears, swaying gently
        Color gold = (Color){ 235, 200, 60, 255 };
        float sway = sinf(time * 2.0f) * 2.0f * scale;
        for (int i = -1; i <= 1; i++) {
            float sx = cx + i * 6.0f * scale;
            Vector2 base = { sx, cy + 10.0f * scale };
            Vector2 tip  = { sx + sway, cy - 12.0f * scale };
            DrawLineEx(base, tip, 2.5f * scale, gold);
            DrawLineEx(tip, (Vector2){ tip.x - 4.0f * scale, tip.y + 4.0f * scale }, 2.0f * scale, gold);
            DrawLineEx(tip, (Vector2){ tip.x + 4.0f * scale, tip.y + 4.0f * scale }, 2.0f * scale, gold);
        }
    } else if (tier == 1) { // Kartoffeln: a cluster of brown ovals
        Color brown1 = (Color){ 150, 105, 60, 255 };
        Color brown2 = (Color){ 120, 82, 45, 255 };
        DrawEllipse((int)(cx - 5 * scale), (int)(cy + 3 * scale), 8 * scale, 6 * scale, brown1);
        DrawEllipse((int)(cx + 6 * scale), (int)(cy + 1 * scale), 7 * scale, 5.5f * scale, brown2);
        DrawEllipse((int)cx, (int)(cy - 6 * scale), 6 * scale, 5 * scale, brown1);
    } else if (tier == 2) { // Karotten: orange root, green leafy top
        Color orange = (Color){ 235, 130, 40, 255 };
        Color green = (Color){ 90, 170, 70, 255 };
        Vector2 v1 = { cx, cy + 12 * scale };
        Vector2 v2 = { cx - 7 * scale, cy - 6 * scale };
        Vector2 v3 = { cx + 7 * scale, cy - 6 * scale };
        DrawTriangle(v2, v1, v3, orange);
        Vector2 top = { cx, cy - 6 * scale };
        DrawLineEx(top, (Vector2){ cx - 5 * scale, cy - 14 * scale }, 2.0f * scale, green);
        DrawLineEx(top, (Vector2){ cx, cy - 16 * scale }, 2.0f * scale, green);
        DrawLineEx(top, (Vector2){ cx + 5 * scale, cy - 14 * scale }, 2.0f * scale, green);
    } else if (tier == 3) { // Zucchini: a long dark-green capsule
        Color dgreen = (Color){ 55, 110, 55, 255 };
        Color lgreen = (Color){ 90, 150, 85, 255 };
        DrawRectangleRounded((Rectangle){ cx - 6 * scale, cy - 13 * scale, 12 * scale, 26 * scale },
                             0.9f, 8, dgreen);
        DrawLineEx((Vector2){ cx, cy - 11 * scale }, (Vector2){ cx, cy + 11 * scale }, 2.0f * scale, lgreen);
    } else { // Mystery tier: colour rotates with the tier index, shape stays a diamond
        Color c = ColorFromHSV(fmodf(tier * 47.0f, 360.0f), 0.65f, 0.85f);
        DrawPoly((Vector2){ cx, cy }, 4, 12.0f * scale, 45.0f, c);
        DrawText("?", (int)(cx - 4 * scale), (int)(cy - 9 * scale), (int)(16 * scale), BLACK);
    }
}
