#include "farm.h"
#include <stdio.h>
#include <math.h>

static Rectangle SpriteRect(int index, bool flip) {
    Rectangle r = { (float)(index * TILE_PX), 0.0f, (float)TILE_PX, (float)TILE_PX };
    if (flip) r.width = -r.width;
    return r;
}

static void DrawTile(GameState *gs, int tx, int ty, float sx, float sy, float time) {
    // Locked (not-yet-unlocked-by-Feld-Erweiterung) ground renders as plain
    // closed-off dirt - visibly different from farmland, no crops possible.
    if (tx >= Field_W(gs) || ty >= Field_H(gs)) {
        DrawRectangle((int)sx, (int)sy, TILE_DRAW, TILE_DRAW, (Color){ 35, 32, 30, 255 });
        DrawRectangleLines((int)sx, (int)sy, TILE_DRAW, TILE_DRAW, (Color){ 55, 50, 46, 255 });
        return;
    }

    Tile *t = &gs->world[tx][ty];
    Color soil_tint = ((tx + ty) % 2 == 0) ? WHITE : (Color){ 228, 222, 206, 255 };
    DrawTexturePro(gs->spritesheet, SpriteRect(SPR_SOIL, false),
                   (Rectangle){ sx, sy, TILE_DRAW, TILE_DRAW }, (Vector2){ 0, 0 }, 0.0f, soil_tint);

    float pop = t->pop_timer > 0.0f ? t->pop_timer / 0.3f : 0.0f;
    float scale = 1.0f + 0.4f * pop;

    if (t->state == TILE_RIPE) {
        // Each crop tier gets its own hand-drawn icon instead of the generic
        // sprite - this is the state players actually look for, so it's the
        // one state where "which crop is this" matters most visually.
        DrawCropIcon(t->crop_type, sx + TILE_DRAW / 2.0f, sy + TILE_DRAW / 2.0f, scale, time);
        float pulse = 0.5f + 0.5f * sinf(time * 6.0f);
        DrawRectangleLinesEx((Rectangle){ sx + 1, sy + 1, TILE_DRAW - 2, TILE_DRAW - 2 },
                              2.0f, Fade(GOLD, 0.35f + 0.5f * pulse));
        return;
    }
    if (t->state == TILE_SPROUT) {
        // Same per-crop icon, just small and dim - a young wheat stalk
        // already looks different from a young carrot, not just "green
        // sprout" for everything until it ripens.
        DrawCropIcon(t->crop_type, sx + TILE_DRAW / 2.0f, sy + TILE_DRAW / 2.0f, scale * 0.5f, time);
        return;
    }

    int sprite = -1;
    switch (t->state) {
        case TILE_OVERRIPE: sprite = SPR_OVERRIPE; break;
        case TILE_WEED:     sprite = SPR_WEED;     break;
        default: break;
    }
    if (sprite >= 0) {
        float size = TILE_DRAW * scale;
        float ox = sx + (TILE_DRAW - size) / 2.0f;
        float oy = sy + (TILE_DRAW - size) / 2.0f;
        DrawTexturePro(gs->spritesheet, SpriteRect(sprite, false),
                       (Rectangle){ ox, oy, size, size }, (Vector2){ 0, 0 }, 0.0f, WHITE);
    }
}

static void DrawPlayerSprite(GameState *gs, Player *p, float sx, float sy, float time) {
    int frame_index;
    bool flip = false;
    switch (p->facing) {
        case FACE_UP:    frame_index = SPR_BACK_A + p->anim_frame; break;
        case FACE_LEFT:  frame_index = SPR_FRONT_A + p->anim_frame; flip = true; break;
        case FACE_RIGHT: frame_index = SPR_FRONT_A + p->anim_frame; break;
        case FACE_DOWN:
        default:         frame_index = SPR_FRONT_A + p->anim_frame; break;
    }

    float bob = 0.0f;
    if (p->moving) {
        bob = fabsf(sinf(p->bob_phase * 2.0f * PI)) * -4.0f;
    } else {
        bob = sinf(time * 2.0f + p->id * 1.7f) * 1.5f;
    }

    float squish = p->interact_anim > 0.0f ? p->interact_anim / 0.22f : 0.0f;
    float scale_y = 1.0f - 0.18f * squish;
    float scale_x = 1.0f + 0.12f * squish;
    float w = PLAYER_SIZE * scale_x;
    float h = PLAYER_SIZE * scale_y;
    float dx = sx + (PLAYER_SIZE - w) / 2.0f;
    float dy = sy + bob + (PLAYER_SIZE - h);

    DrawEllipse((int)(sx + PLAYER_SIZE / 2), (int)(sy + PLAYER_SIZE - 4),
                PLAYER_SIZE * 0.32f, PLAYER_SIZE * 0.14f, Fade(BLACK, 0.35f));
    DrawTexturePro(gs->spritesheet, SpriteRect(frame_index, flip),
                   (Rectangle){ dx, dy, w, h }, (Vector2){ 0, 0 }, 0.0f, p->tint);
}

static void DrawCursor(GameState *gs, Player *p, float cam_x, float cam_y, float view_x0, float view_y0, float time) {
    float sx = view_x0 + p->facing_tile_x * TILE_DRAW - cam_x;
    float sy = view_y0 + p->facing_tile_y * TILE_DRAW - cam_y;
    float pulse = 0.55f + 0.45f * sinf(time * 5.0f + p->id * 2.0f);
    DrawTexturePro(gs->spritesheet, SpriteRect(SPR_CURSOR, false),
                   (Rectangle){ sx, sy, TILE_DRAW, TILE_DRAW }, (Vector2){ 0, 0 }, 0.0f,
                   Fade(p->tint, pulse));
}

static void DrawDrone(Drone *d, float sx, float sy, float time) {
    float cx = sx + PLAYER_SIZE / 2.0f;
    float ground_y = sy + PLAYER_SIZE - 2.0f;

    if (d->broken) {
        // Broken drones just sit dead on the ground, rotors still, until
        // repaired from the shop - a small grey "X" eye instead of a task
        // colour makes the state unmistakable at a glance.
        float cy = ground_y - 6.0f;
        DrawEllipse((int)cx, (int)ground_y, 9.0f, 3.5f, Fade(BLACK, 0.3f));
        DrawCircle((int)cx, (int)cy, 9.0f, (Color){ 60, 55, 50, 255 });
        DrawCircleLines((int)cx, (int)cy, 9.0f, (Color){ 120, 110, 100, 255 });
        DrawLineEx((Vector2){ cx - 3, cy - 3 }, (Vector2){ cx + 3, cy + 3 }, 2.0f, (Color){ 200, 60, 60, 255 });
        DrawLineEx((Vector2){ cx - 3, cy + 3 }, (Vector2){ cx + 3, cy - 3 }, 2.0f, (Color){ 200, 60, 60, 255 });
        return;
    }

    float hover = sinf(time * 3.0f + d->bob_seed) * 3.0f;
    float cy = sy + PLAYER_SIZE / 2.0f + hover;

    DrawEllipse((int)cx, (int)ground_y, 9.0f, 3.5f, Fade(BLACK, 0.3f));

    float spin = time * 9.0f;
    for (int i = 0; i < 2; i++) {
        float a = spin + i * (PI / 2.0f);
        Vector2 p1 = { cx - cosf(a) * 12.0f, cy - sinf(a) * 12.0f };
        Vector2 p2 = { cx + cosf(a) * 12.0f, cy + sinf(a) * 12.0f };
        DrawLineEx(p1, p2, 2.0f, Fade(LIGHTGRAY, 0.6f));
    }

    Color eye = (Color){ 150, 150, 150, 255 };
    if (d->has_target) {
        switch (d->task) {
            case DRONE_TASK_HARVEST:          eye = GOLD; break;
            case DRONE_TASK_HARVEST_OVERRIPE: eye = ORANGE; break;
            case DRONE_TASK_PLANT:            eye = LIME; break;
        }
    }

    DrawCircle((int)cx, (int)cy, 9.0f, (Color){ 70, 70, 80, 255 });
    DrawCircleLines((int)cx, (int)cy, 9.0f, GOLD);
    DrawCircle((int)cx, (int)(cy - 3.0f), 3.0f, eye);
}

static void DrawMower(Mower *m, float sx, float sy, float time) {
    float cx = sx + PLAYER_SIZE / 2.0f;
    float cy = sy + PLAYER_SIZE * 0.55f;
    float body_w = 20.0f, body_h = 13.0f;

    DrawEllipse((int)cx, (int)(sy + PLAYER_SIZE - 2.0f), 11.0f, 3.5f, Fade(BLACK, 0.3f));

    if (m->broken) {
        // Broken down, blade stopped, parked where it died - same visual
        // language as a broken drone (grey body, red X) for consistency.
        DrawRectangleRounded((Rectangle){ cx - body_w / 2.0f, cy - body_h / 2.0f, body_w, body_h },
                             0.35f, 6, (Color){ 60, 55, 50, 255 });
        DrawRectangleRoundedLines((Rectangle){ cx - body_w / 2.0f, cy - body_h / 2.0f, body_w, body_h },
                                  0.35f, 6, (Color){ 120, 110, 100, 255 });
        DrawLineEx((Vector2){ cx - 4, cy - 4 }, (Vector2){ cx + 4, cy + 4 }, 2.0f, (Color){ 200, 60, 60, 255 });
        DrawLineEx((Vector2){ cx - 4, cy + 4 }, (Vector2){ cx + 4, cy - 4 }, 2.0f, (Color){ 200, 60, 60, 255 });
        return;
    }

    // body: a small boxy mower chassis sitting on the ground
    DrawRectangleRounded((Rectangle){ cx - body_w / 2.0f, cy - body_h / 2.0f, body_w, body_h },
                         0.35f, 6, (Color){ 90, 70, 40, 255 });
    DrawRectangleRoundedLines((Rectangle){ cx - body_w / 2.0f, cy - body_h / 2.0f, body_w, body_h },
                              0.35f, 6, (Color){ 170, 90, 220, 255 });

    // wheels
    DrawCircle((int)(cx - body_w / 2.0f + 2.0f), (int)(cy + body_h / 2.0f), 3.0f, DARKGRAY);
    DrawCircle((int)(cx + body_w / 2.0f - 2.0f), (int)(cy + body_h / 2.0f), 3.0f, DARKGRAY);

    // spinning blade underneath, visible as it flashes past
    float spin = time * 14.0f + m->bob_seed;
    Vector2 bp1 = { cx - cosf(spin) * 9.0f, cy + body_h / 2.0f + sinf(spin) * 2.0f };
    Vector2 bp2 = { cx + cosf(spin) * 9.0f, cy + body_h / 2.0f - sinf(spin) * 2.0f };
    DrawLineEx(bp1, bp2, 2.0f, Fade(LIGHTGRAY, 0.8f));
}

static void DrawBird(Bird *b, float sx, float sy, float time) {
    float phase = (float)(b->target_x * 7 + b->target_y * 3); // desync multiple birds' wingbeats
    float cx = sx + PLAYER_SIZE / 2.0f;
    float flap = sinf(time * 16.0f + phase) * 6.0f;
    float cy = sy + PLAYER_SIZE / 2.0f + sinf(time * 4.0f + phase) * 2.0f;

    Color body = (Color){ 50, 45, 55, 255 };
    DrawEllipse((int)cx, (int)cy, 5.0f, 3.5f, body);
    DrawLineEx((Vector2){ cx, cy }, (Vector2){ cx - 9.0f, cy - flap }, 2.0f, body);
    DrawLineEx((Vector2){ cx, cy }, (Vector2){ cx + 9.0f, cy - flap }, 2.0f, body);
    DrawCircle((int)(cx + 4.0f), (int)(cy - 1.0f), 1.5f, (Color){ 235, 180, 60, 255 }); // beak
}

static void DrawDog(Dog *dg, float sx, float sy, float time) {
    float cx = sx + PLAYER_SIZE / 2.0f;
    float cy = sy + PLAYER_SIZE * 0.6f;
    float bounce = dg->has_target ? fabsf(sinf(time * 10.0f + dg->bob_seed)) * 3.0f : 0.0f;

    DrawEllipse((int)cx, (int)(sy + PLAYER_SIZE - 2.0f), 9.0f, 3.0f, Fade(BLACK, 0.3f));
    Color fur = (Color){ 150, 110, 70, 255 };
    DrawEllipse((int)cx, (int)(cy - bounce), 8.0f, 6.0f, fur);
    DrawCircle((int)(cx + 7.0f), (int)(cy - 2.0f - bounce), 4.0f, fur);
    DrawCircle((int)(cx + 9.0f), (int)(cy - 3.0f - bounce), 1.2f, BLACK); // eye
    DrawTriangle((Vector2){ cx - 8.0f, cy - bounce }, (Vector2){ cx - 13.0f, cy - 3.0f - bounce },
                 (Vector2){ cx - 8.0f, cy - 5.0f - bounce }, fur); // tail
}

// Vogelscheuchen are static (not autonomous units like dogs) - one purchased
// level just means one more post standing guard. Purely decorative: the
// actual spawn-chance reduction already happens in pests.c regardless of
// whether a post is in view.
static void DrawScarecrow(float sx, float sy, float time) {
    float cx = sx + PLAYER_SIZE / 2.0f;
    float sway = sinf(time * 1.5f + sx * 0.01f) * 2.0f;
    Color wood = (Color){ 120, 90, 55, 255 };
    Color sack = (Color){ 200, 170, 90, 255 };

    DrawEllipse((int)cx, (int)(sy + PLAYER_SIZE - 2.0f), 8.0f, 3.0f, Fade(BLACK, 0.3f));
    DrawLineEx((Vector2){ cx, sy + PLAYER_SIZE - 2.0f }, (Vector2){ cx, sy + 6.0f }, 3.0f, wood); // post
    DrawLineEx((Vector2){ cx - 10.0f + sway, sy + 14.0f }, (Vector2){ cx + 10.0f + sway, sy + 14.0f }, 2.5f, wood); // arms
    DrawCircle((int)cx, (int)(sy + 6.0f), 6.0f, sack); // head/sack
    DrawLineEx((Vector2){ cx - 3.0f, sy + 4.0f }, (Vector2){ cx - 1.0f, sy + 6.0f }, 1.5f, BLACK); // eyes
    DrawLineEx((Vector2){ cx + 1.0f, sy + 4.0f }, (Vector2){ cx + 3.0f, sy + 6.0f }, 1.5f, BLACK);
}

static void DrawViewport(GameState *gs, int slot, float time) {
    Rectangle vp = Layout_Viewport(gs->active_players, slot);
    float cam_x, cam_y;
    Layout_CameraForPlayer(gs, slot, vp, &cam_x, &cam_y);

    BeginScissorMode((int)vp.x, (int)vp.y, (int)vp.width, (int)vp.height);
    DrawRectangle((int)vp.x, (int)vp.y, (int)vp.width, (int)vp.height, (Color){ 120, 150, 90, 255 });

    int tx0 = (int)(cam_x / TILE_DRAW);
    int ty0 = (int)(cam_y / TILE_DRAW);
    int tx1 = (int)((cam_x + vp.width) / TILE_DRAW) + 1;
    int ty1 = (int)((cam_y + vp.height) / TILE_DRAW) + 1;
    if (tx0 < 0) tx0 = 0;
    if (ty0 < 0) ty0 = 0;
    if (tx1 > WORLD_W) tx1 = WORLD_W;
    if (ty1 > WORLD_H) ty1 = WORLD_H;

    for (int tx = tx0; tx < tx1; tx++) {
        for (int ty = ty0; ty < ty1; ty++) {
            float sx = vp.x + tx * TILE_DRAW - cam_x;
            float sy = vp.y + ty * TILE_DRAW - cam_y;
            DrawTile(gs, tx, ty, sx, sy, time);
        }
    }

    for (int i = 0; i < gs->active_players; i++) {
        DrawCursor(gs, &gs->players[i], cam_x, cam_y, vp.x, vp.y, time);
    }

    int order[MAX_PLAYERS];
    for (int i = 0; i < gs->active_players; i++) order[i] = i;
    for (int i = 1; i < gs->active_players; i++) {
        int key = order[i], j = i - 1;
        while (j >= 0 && gs->players[order[j]].y > gs->players[key].y) {
            order[j + 1] = order[j];
            j--;
        }
        order[j + 1] = key;
    }
    for (int i = 0; i < gs->active_players; i++) {
        Player *p = &gs->players[order[i]];
        float sx = vp.x + p->x - cam_x;
        float sy = vp.y + p->y - cam_y;
        DrawPlayerSprite(gs, p, sx, sy, time);
    }

    for (int i = 0; i < gs->drone_count; i++) {
        Drone *d = &gs->drones[i];
        if (!d->active) continue;
        float sx = vp.x + d->x - cam_x;
        float sy = vp.y + d->y - cam_y;
        DrawDrone(d, sx, sy, time);
    }

    for (int i = 0; i < gs->mower_count; i++) {
        Mower *m = &gs->mowers[i];
        if (!m->active) continue;
        float sx = vp.x + m->x - cam_x;
        float sy = vp.y + m->y - cam_y;
        DrawMower(m, sx, sy, time);
    }

    for (int i = 0; i < MAX_BIRDS; i++) {
        Bird *b = &gs->birds[i];
        if (!b->active) continue;
        float sx = vp.x + b->x - cam_x;
        float sy = vp.y + b->y - cam_y;
        DrawBird(b, sx, sy, time);
    }

    for (int i = 0; i < gs->dog_count; i++) {
        Dog *dg = &gs->dogs[i];
        if (!dg->active) continue;
        float sx = vp.x + dg->x - cam_x;
        float sy = vp.y + dg->y - cam_y;
        DrawDog(dg, sx, sy, time);
    }

    // Scarecrow posts are spread evenly along the top edge of the currently
    // unlocked field - a static, visible headcount of the Vogelscheuche
    // investment, not an autonomous unit like dogs.
    int scarecrow_count = gs->economy.scarecrow_level;
    if (scarecrow_count > SCARECROW_VISIBLE_MAX) scarecrow_count = SCARECROW_VISIBLE_MAX;
    int fw_sc = Field_W(gs);
    for (int i = 0; i < scarecrow_count; i++) {
        int tx = (fw_sc * (i + 1)) / (scarecrow_count + 1);
        float wx = tx * TILE_DRAW + TILE_DRAW / 4.0f;
        float wy = 0.0f;
        float sx = vp.x + wx - cam_x;
        float sy = vp.y + wy - cam_y;
        DrawScarecrow(sx, sy, time);
    }

    float night = gs->daynight.night_factor;
    if (night > 0.01f) {
        DrawRectangle((int)vp.x, (int)vp.y, (int)vp.width, (int)vp.height,
                      Fade((Color){ 15, 20, 55, 255 }, night * 0.6f));
    }

    // A faint colour wash per weather condition, so it's visible at a glance
    // without having to read the HUD label - plus falling streaks for rain
    // and storm, since those two are the ones that should feel like weather.
    switch (gs->weather.type) {
        case WEATHER_RAIN:
            DrawRectangle((int)vp.x, (int)vp.y, (int)vp.width, (int)vp.height, Fade((Color){ 40, 70, 130, 255 }, 0.18f));
            break;
        case WEATHER_DROUGHT:
            DrawRectangle((int)vp.x, (int)vp.y, (int)vp.width, (int)vp.height, Fade((Color){ 170, 120, 50, 255 }, 0.18f));
            break;
        case WEATHER_STORM:
            DrawRectangle((int)vp.x, (int)vp.y, (int)vp.width, (int)vp.height, Fade((Color){ 35, 35, 42, 255 }, 0.3f));
            break;
        case WEATHER_BLESSING:
            DrawRectangle((int)vp.x, (int)vp.y, (int)vp.width, (int)vp.height, Fade((Color){ 255, 220, 120, 255 }, 0.12f));
            break;
        default: break;
    }
    if (gs->weather.type == WEATHER_RAIN || gs->weather.type == WEATHER_STORM) {
        Color streak_col = gs->weather.type == WEATHER_STORM ? Fade(LIGHTGRAY, 0.5f) : Fade(SKYBLUE, 0.4f);
        for (int i = 0; i < 24; i++) {
            float sx0 = fmodf(i * 97.0f + time * 240.0f, vp.width);
            float sy0 = fmodf(i * 53.0f + time * 400.0f, vp.height);
            DrawLineEx((Vector2){ vp.x + sx0, vp.y + sy0 }, (Vector2){ vp.x + sx0 - 6.0f, vp.y + sy0 + 14.0f }, 1.5f, streak_col);
        }
    }

    EndScissorMode();
}

static void DrawEmptySlot(int slot, int active_players) {
    Rectangle vp = Layout_Viewport(active_players, slot);
    DrawRectangle((int)vp.x, (int)vp.y, (int)vp.width, (int)vp.height, (Color){ 25, 25, 30, 255 });
    const char *msg = "Kein Spieler";
    int w = MeasureText(msg, 20);
    DrawText(msg, (int)(vp.x + vp.width / 2 - w / 2), (int)(vp.y + vp.height / 2 - 10), 20, GRAY);
}

static void DrawViewportDividers(GameState *gs) {
    int play_h = WINDOW_H - HUD_H;
    if (gs->active_players == 2) {
        DrawRectangle(WINDOW_W / 2 - 1, HUD_H, 2, play_h, BLACK);
    } else if (gs->active_players >= 3) {
        DrawRectangle(WINDOW_W / 2 - 1, HUD_H, 2, play_h, BLACK);
        DrawRectangle(0, HUD_H + play_h / 2 - 1, WINDOW_W, 2, BLACK);
    }
}

static void DrawHud(GameState *gs) {
    DrawRectangle(0, 0, WINDOW_W, HUD_H, (Color){ 30, 26, 20, 235 });

    char buf[128];
    snprintf(buf, sizeof(buf), "Gold: %d", gs->economy.money);
    DrawText(buf, 16, 8, 22, GOLD);

    snprintf(buf, sizeof(buf), "Tag %d", gs->daynight.day_count);
    DrawText(buf, 16, 34, 18, RAYWHITE);

    Color sun_col = {
        (unsigned char)(255 - 140 * gs->daynight.night_factor),
        (unsigned char)(220 - 140 * gs->daynight.night_factor),
        (unsigned char)(80 + 140 * gs->daynight.night_factor),
        255
    };
    DrawCircle(150, 43, 9, sun_col);

    // Row 1 (y=8): Gold | quest description | hint (right-aligned). Quest
    // targets grow without bound over a long session, so its text lives on
    // its own row with a wide gap before the hint rather than sharing a
    // fixed-width column with anything else.
    snprintf(buf, sizeof(buf), "%s (%d/%d)", gs->quest.description,
              gs->quest.progress > gs->quest.target ? gs->quest.target : gs->quest.progress, gs->quest.target);
    DrawText(buf, 230, 8, 20, SKYBLUE);

    const char *hint = Audio_IsMuted() ? "ESC: Menue  [B]/Start: Shop  [M] Ton: aus"
                                        : "ESC: Menue  [B]/Start: Shop  [M] Ton: an";
    int w = MeasureText(hint, 16);
    DrawText(hint, WINDOW_W - w - 16, 10, 16, LIGHTGRAY);

    // Row 2 (y=34): Tag/sun | quest bar | level title. The level title can
    // now run quite long (up to ~22 chars at the highest tiers), so it gets
    // its own row entirely separate from the quest text above, with a wide
    // gap after the quest bar (which ends at x=490) before it starts.
    float frac = (float)gs->quest.progress / (float)gs->quest.target;
    if (frac > 1.0f) frac = 1.0f;
    if (frac < 0.0f) frac = 0.0f;
    DrawRectangle(230, 34, 260, 14, (Color){ 60, 60, 60, 255 });
    DrawRectangle(230, 34, (int)(260 * frac), 14, LIME);

    snprintf(buf, sizeof(buf), "Lvl %d: %s", Progress_Level(gs), Progress_Title(gs));
    DrawText(buf, 550, 34, 18, VIOLET);

    // Row 3 (y=60): Sorte/crop label | weather label | level progress bar.
    if (gs->economy.crop_unlocked > 0) {
        snprintf(buf, sizeof(buf), "Sorte: %s", Crop_Name(gs->economy.crop_active));
        DrawText(buf, 16, 60, 16, LIME);
    }
    if (gs->weather.type != WEATHER_CLEAR) {
        Color weather_col;
        switch (gs->weather.type) {
            case WEATHER_RAIN:     weather_col = SKYBLUE; break;
            case WEATHER_DROUGHT:  weather_col = (Color){ 210, 150, 70, 255 }; break;
            case WEATHER_STORM:    weather_col = (Color){ 170, 170, 180, 255 }; break;
            case WEATHER_BLESSING: weather_col = GOLD; break;
            default:               weather_col = RAYWHITE; break;
        }
        snprintf(buf, sizeof(buf), "Wetter: %s (%ds)", Weather_Name(gs->weather.type), (int)gs->weather.timer + 1);
        int weather_w = MeasureText(buf, 16);
        DrawText(buf, 535 - weather_w, 60, 16, weather_col);
    }
    float lvl_frac = Progress_LevelProgress(gs);
    DrawRectangle(550, 62, 180, 10, (Color){ 60, 60, 60, 255 });
    DrawRectangle(550, 62, (int)(180 * lvl_frac), 10, VIOLET);

    if (gs->toast.timer > 0.0f) {
        float alpha = gs->toast.timer > 1.0f ? 1.0f : gs->toast.timer;
        int tw = MeasureText(gs->toast.text, 24);
        int cx = WINDOW_W / 2 - tw / 2;
        DrawRectangle(cx - 20, HUD_H + 10, tw + 40, 36, Fade(BLACK, 0.55f * alpha));
        DrawText(gs->toast.text, cx, HUD_H + 18, 24, Fade(GOLD, alpha));
    }
}

static void DrawContractBanner(GameState *gs) {
    Contract *c = &gs->contract;
    if (!c->active) return;

    int w = 260, h = 64;
    int px = WINDOW_W - w - 16, py = HUD_H + 10;
    DrawRectangle(px, py, w, h, Fade((Color){ 25, 35, 45, 255 }, 0.9f));
    DrawRectangleLines(px, py, w, h, SKYBLUE);

    char buf[96];
    snprintf(buf, sizeof(buf), "Grossauftrag: %s", Crop_Name(c->crop_type));
    DrawText(buf, px + 10, py + 6, 16, SKYBLUE);
    DrawCropIcon(c->crop_type, (float)(px + w - 22), (float)(py + 20), 0.7f, (float)GetTime());

    snprintf(buf, sizeof(buf), "%d / %d", c->progress, c->target);
    DrawText(buf, px + 10, py + 26, 18, RAYWHITE);

    float frac = c->progress / (float)c->target;
    if (frac > 1.0f) frac = 1.0f;
    DrawRectangle(px + 10, py + 48, w - 20, 8, (Color){ 60, 60, 60, 255 });
    DrawRectangle(px + 10, py + 48, (int)((w - 20) * frac), 8, LIME);

    float time_frac = c->time_left / c->time_total;
    if (time_frac < 0.0f) time_frac = 0.0f;
    Color time_col = time_frac < 0.25f ? (Color){ 220, 80, 80, 255 } : GOLD;
    DrawRectangle(px + w - 70, py + 26, 60, 8, (Color){ 60, 60, 60, 255 });
    DrawRectangle(px + w - 70, py + 26, (int)(60 * time_frac), 8, time_col);
}

static void DrawCropBar(GameState *gs, float time) {
    int total = gs->economy.crop_unlocked + 1;
    for (int i = 0; i < total; i++) {
        Rectangle r = Crop_ButtonRect(i, total);
        bool active = (i == gs->economy.crop_active);
        bool hover = CheckCollisionPointRec(GetMousePosition(), r);
        Color bg = active ? (Color){ 60, 110, 40, 255 } : (hover ? (Color){ 65, 58, 50, 230 } : (Color){ 40, 36, 32, 220 });
        DrawRectangleRec(r, bg);
        DrawRectangleLinesEx(r, active ? 2.0f : 1.0f, active ? LIME : GRAY);

        // The button IS the plant - a small icon of the crop itself, not
        // just a text label (the label stays too, smaller, for the mystery
        // tiers where the icon alone can't tell them apart by name).
        DrawCropIcon(i, r.x + r.width / 2.0f, r.y + r.height / 2.0f - 6.0f, 0.8f, time);
        int tw = MeasureText(Crop_Name(i), 12);
        DrawText(Crop_Name(i), (int)(r.x + r.width / 2 - tw / 2), (int)(r.y + r.height - 16), 12,
                 active ? RAYWHITE : LIGHTGRAY);
    }
}

static void DrawShopRow(int index, int selection, const char *title, const char *cost_line) {
    Rectangle r = Shop_RowRect(index);
    bool selected = (index == selection);
    bool hover = CheckCollisionPointRec(GetMousePosition(), r);
    Color bg = selected ? (Color){ 90, 70, 30, 255 } : (hover ? (Color){ 65, 58, 50, 230 } : (Color){ 50, 45, 40, 220 });
    DrawRectangleRec(r, bg);
    DrawText(title, (int)r.x + 20, (int)r.y + 8, 20, RAYWHITE);
    DrawText(cost_line, (int)r.x + 20, (int)r.y + 30, 16, GOLD);
    if (selected) DrawText(">", (int)r.x - 20, (int)r.y + 16, 24, GOLD);
}

static void DrawShop(GameState *gs) {
    DrawRectangle(0, 0, WINDOW_W, WINDOW_H, Fade(BLACK, 0.65f));
    Rectangle panel = Shop_PanelRect();
    int px = (int)panel.x, py = (int)panel.y, panel_w = (int)panel.width, panel_h = (int)panel.height;
    DrawRectangle(px, py, panel_w, panel_h, (Color){ 35, 30, 25, 245 });
    DrawRectangleLinesEx(panel, 3, GOLD);
    char header[48];
    snprintf(header, sizeof(header), "Dorfladen - Sorte: %s", Crop_Name(gs->economy.crop_active));
    DrawText(header, px + 20, py + 14, 24, GOLD);

    Economy *e = &gs->economy;
    CropTrack *ct = &e->crop_track[e->crop_active];
    char title[96], cost_line[64];

    snprintf(title, sizeof(title), "Lauftempo - Stufe %d (+%.0f%%)", e->speed_level, (e->speed_mult - 1.0f) * 100.0f);
    snprintf(cost_line, sizeof(cost_line), e->speed_level >= MAX_UPGRADE_LEVEL ? "MAX" : "Kosten: %d Gold", Shop_UpgradeCost(e->speed_level));
    DrawShopRow(0, gs->shop.selection, title, cost_line);

    snprintf(title, sizeof(title), "Wachstum - Stufe %d (+%.0f%%)", ct->growth_level, (ct->growth_mult - 1.0f) * 100.0f);
    snprintf(cost_line, sizeof(cost_line), ct->growth_level >= MAX_UPGRADE_LEVEL ? "MAX" : "Kosten: %d Gold", Shop_UpgradeCost(ct->growth_level));
    DrawShopRow(1, gs->shop.selection, title, cost_line);

    snprintf(title, sizeof(title), "Verkaufspreis - Stufe %d (+%.0f%%)", ct->sell_level, (ct->sell_mult - 1.0f) * 100.0f);
    snprintf(cost_line, sizeof(cost_line), ct->sell_level >= MAX_UPGRADE_LEVEL ? "MAX" : "Kosten: %d Gold", Shop_UpgradeCost(ct->sell_level));
    DrawShopRow(2, gs->shop.selection, title, cost_line);

    snprintf(title, sizeof(title), "Auto-Farmarbeit - Stufe %d/%d", e->auto_plant_level, MAX_AUTO_PLANT_LEVEL);
    snprintf(cost_line, sizeof(cost_line), e->auto_plant_level >= MAX_AUTO_PLANT_LEVEL ? "MAX" : "Kosten: %d Gold", Shop_UpgradeCost(e->auto_plant_level));
    DrawShopRow(3, gs->shop.selection, title, cost_line);

    int drone_cost = Drones_Cost(gs);
    snprintf(title, sizeof(title), "Farm-Drohne - %d/%d gekauft", gs->drone_count, MAX_DRONES);
    snprintf(cost_line, sizeof(cost_line), drone_cost < 0 ? "MAX" : "Kosten: %d Gold", drone_cost);
    DrawShopRow(4, gs->shop.selection, title, cost_line);

    snprintf(title, sizeof(title), "Drohnen-Tempo - Stufe %d (+%.0f%%)", e->drone_speed_level, (e->drone_speed_mult - 1.0f) * 100.0f);
    snprintf(cost_line, sizeof(cost_line), e->drone_speed_level >= MAX_UPGRADE_LEVEL ? "MAX" : "Kosten: %d Gold", Shop_UpgradeCost(e->drone_speed_level));
    DrawShopRow(5, gs->shop.selection, title, cost_line);

    int mower_cost = Mowers_Cost(gs);
    snprintf(title, sizeof(title), "Rasenmaeher - %d/%d gekauft", gs->mower_count, MAX_MOWERS);
    snprintf(cost_line, sizeof(cost_line), mower_cost < 0 ? "MAX" : "Kosten: %d Gold", mower_cost);
    DrawShopRow(6, gs->shop.selection, title, cost_line);

    snprintf(title, sizeof(title), "Maeher-Tempo - Stufe %d (+%.0f%%)", e->mower_speed_level, (e->mower_speed_mult - 1.0f) * 100.0f);
    snprintf(cost_line, sizeof(cost_line), e->mower_speed_level >= MAX_UPGRADE_LEVEL ? "MAX" : "Kosten: %d Gold", Shop_UpgradeCost(e->mower_speed_level));
    DrawShopRow(7, gs->shop.selection, title, cost_line);

    snprintf(title, sizeof(title), "Maeher-Reichweite - Stufe %d/%d (Radius %d)", e->mower_range_level, MAX_MOWER_RANGE_LEVEL, e->mower_range_level);
    snprintf(cost_line, sizeof(cost_line), e->mower_range_level >= MAX_MOWER_RANGE_LEVEL ? "MAX" : "Kosten: %d Gold", Shop_UpgradeCost(e->mower_range_level));
    DrawShopRow(8, gs->shop.selection, title, cost_line);

    snprintf(title, sizeof(title), "Unkraut-Resistenz - Stufe %d (-%.0f%% Unkrautchance)", e->weed_resist_level, (1.0f - e->weed_resist_mult) * 100.0f);
    snprintf(cost_line, sizeof(cost_line), e->weed_resist_level >= MAX_UPGRADE_LEVEL ? "MAX" : "Kosten: %d Gold", Shop_UpgradeCost(e->weed_resist_level));
    DrawShopRow(9, gs->shop.selection, title, cost_line);

    if (e->crop_unlocked >= MAX_CROP_TIERS - 1) {
        snprintf(title, sizeof(title), "Alle Sorten freigeschaltet (%s aktiv)", Crop_Name(e->crop_active));
        snprintf(cost_line, sizeof(cost_line), "MAX");
    } else {
        snprintf(title, sizeof(title), "Naechste Sorte: %s (+%.0fx)",
                 Crop_Name(e->crop_unlocked + 1), Crop_Mult(e->crop_unlocked + 1));
        snprintf(cost_line, sizeof(cost_line), "Kosten: %d Gold", Crop_UnlockCost(e->crop_unlocked + 1));
    }
    DrawShopRow(10, gs->shop.selection, title, cost_line);

    snprintf(title, sizeof(title), "Scheune - Stufe %d/%d (passives Gold)", e->barn_level, MAX_BARN_LEVEL);
    snprintf(cost_line, sizeof(cost_line), e->barn_level >= MAX_BARN_LEVEL ? "MAX" : "Kosten: %d Gold", Shop_UpgradeCost(e->barn_level));
    DrawShopRow(11, gs->shop.selection, title, cost_line);

    snprintf(title, sizeof(title), "Feld-Erweiterung - Stufe %d/%d (%dx%d)", e->field_level, MAX_FIELD_LEVEL, Field_W(gs), Field_H(gs));
    snprintf(cost_line, sizeof(cost_line), e->field_level >= MAX_FIELD_LEVEL ? "MAX" : "Kosten: %d Gold", Shop_UpgradeCost(e->field_level));
    DrawShopRow(12, gs->shop.selection, title, cost_line);

    snprintf(title, sizeof(title), "Drohnen-Schwarmradius - Stufe %d/%d", e->drone_swarm_level, MAX_DRONE_SWARM_LEVEL);
    snprintf(cost_line, sizeof(cost_line), e->drone_swarm_level >= MAX_DRONE_SWARM_LEVEL ? "MAX" : "Kosten: %d Gold", Shop_UpgradeCost(e->drone_swarm_level));
    DrawShopRow(13, gs->shop.selection, title, cost_line);

    snprintf(title, sizeof(title), "Vogelscheuche - Stufe %d (-%.0f%% Vogelchance)", e->scarecrow_level, (1.0f - e->scarecrow_mult) * 100.0f);
    snprintf(cost_line, sizeof(cost_line), e->scarecrow_level >= MAX_SCARECROW_LEVEL ? "MAX" : "Kosten: %d Gold", Shop_UpgradeCost(e->scarecrow_level));
    DrawShopRow(14, gs->shop.selection, title, cost_line);

    int dog_cost = Dogs_Cost(gs);
    snprintf(title, sizeof(title), "Hund - %d/%d gekauft", gs->dog_count, MAX_DOGS);
    snprintf(cost_line, sizeof(cost_line), dog_cost < 0 ? "MAX" : "Kosten: %d Gold", dog_cost);
    DrawShopRow(15, gs->shop.selection, title, cost_line);

    int broken = 0;
    for (int i = 0; i < gs->drone_count; i++) if (gs->drones[i].broken) broken++;
    for (int i = 0; i < gs->mower_count; i++) if (gs->mowers[i].broken) broken++;
    snprintf(title, sizeof(title), "Reparatur - %d kaputte Geraete", broken);
    snprintf(cost_line, sizeof(cost_line), broken == 0 ? "nichts kaputt" : "Kosten: %d Gold", broken * UNIT_REPAIR_COST_EACH);
    DrawShopRow(16, gs->shop.selection, title, cost_line);

    DrawText("Maus / Pfeile / D-Pad: auswaehlen+kaufen   ESC/Start: schliessen",
              px + 20, py + panel_h - 28, 15, LIGHTGRAY);
}

static void DrawMenuButton(Rectangle r, const char *label, int font_size, Color bg) {
    bool hover = CheckCollisionPointRec(GetMousePosition(), r);
    DrawRectangleRec(r, hover ? ColorBrightness(bg, 0.25f) : bg);
    DrawRectangleLinesEx(r, 2, GOLD);
    int w = MeasureText(label, font_size);
    DrawText(label, (int)(r.x + r.width / 2 - w / 2), (int)(r.y + r.height / 2 - font_size / 2), font_size, RAYWHITE);
}

static void DrawAchievementsOverlay(GameState *gs) {
    DrawRectangle(0, 0, WINDOW_W, WINDOW_H, Fade(BLACK, 0.75f));
    int panel_w = 920, panel_h = 480;
    int px = WINDOW_W / 2 - panel_w / 2, py = WINDOW_H / 2 - panel_h / 2;
    DrawRectangle(px, py, panel_w, panel_h, (Color){ 30, 28, 24, 250 });
    DrawRectangleLinesEx((Rectangle){ (float)px, (float)py, (float)panel_w, (float)panel_h }, 3, GOLD);

    int unlocked = 0;
    for (int i = 0; i < ACHIEVEMENT_COUNT; i++) if (gs->progress.achievements[i]) unlocked++;
    char header[64];
    snprintf(header, sizeof(header), "Erfolge (%d/%d)", unlocked, ACHIEVEMENT_COUNT);
    DrawText(header, px + 24, py + 16, 28, GOLD);

    int col_w = (panel_w - 48 - 20) / 2;
    for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
        int col = i % 2, row = i / 2;
        int x = px + 24 + col * (col_w + 20);
        int y = py + 66 + row * 76;
        bool done = gs->progress.achievements[i];
        Color bg = done ? (Color){ 45, 70, 35, 255 } : (Color){ 45, 40, 38, 255 };
        DrawRectangle(x, y, col_w, 64, bg);
        DrawText(done ? "[X]" : "[ ]", x + 14, y + 20, 20, done ? LIME : GRAY);
        DrawText(Achievement_Name(i), x + 58, y + 10, 18, done ? RAYWHITE : LIGHTGRAY);
        DrawText(Achievement_Desc(i), x + 58, y + 34, 13, GRAY);
    }

    DrawText("ESC / E: schliessen", px + 24, py + panel_h - 26, 15, LIGHTGRAY);
}

static void Render_Menu(GameState *gs) {
    ClearBackground((Color){ 40, 60, 35, 255 });

    const char *title = "FARMI";
    int tw = MeasureText(title, 64);
    DrawText(title, WINDOW_W / 2 - tw / 2, 50, 64, GOLD);
    const char *subtitle = "Couch-Coop Farmspiel";
    int sw = MeasureText(subtitle, 22);
    DrawText(subtitle, WINDOW_W / 2 - sw / 2, 124, 22, RAYWHITE);

    char level_buf[64];
    snprintf(level_buf, sizeof(level_buf), "Hof-Level %d: %s", Progress_Level(gs), Progress_Title(gs));
    int lvw = MeasureText(level_buf, 20);
    DrawText(level_buf, WINDOW_W / 2 - lvw / 2, 156, 20, VIOLET);

    int panel_w = 560;
    int panel_h = 490;
    int px = (WINDOW_W - panel_w) / 2;
    DrawRectangle(px, 190, panel_w, panel_h, Fade(BLACK, 0.4f));
    DrawRectangleLinesEx((Rectangle){ (float)px, 190, (float)panel_w, (float)panel_h }, 2, GOLD);

    const char *label = "Spieleranzahl";
    int lw = MeasureText(label, 22);
    DrawText(label, WINDOW_W / 2 - lw / 2, 215, 22, RAYWHITE);

    DrawMenuButton(Menu_LeftArrowRect(), "<", 32, (Color){ 70, 55, 25, 255 });
    DrawMenuButton(Menu_RightArrowRect(), ">", 32, (Color){ 70, 55, 25, 255 });

    char count_buf[4];
    snprintf(count_buf, sizeof(count_buf), "%d", gs->menu.player_count);
    int cw = MeasureText(count_buf, 48);
    DrawText(count_buf, WINDOW_W / 2 - cw / 2, 288, 48, GOLD);

    if (gs->menu.has_save) {
        DrawMenuButton(Menu_ContinueButtonRect(), "Fortsetzen", 22, (Color){ 40, 90, 40, 255 });
    }
    DrawMenuButton(Menu_NewGameButtonRect(), "Neues Spiel", 22, (Color){ 90, 70, 30, 255 });

    int list_y = 470;
    DrawText("Spieler 1: Tastatur (WASD) + Maus", px + 30, list_y, 18, RAYWHITE);
    for (int i = 1; i < gs->menu.player_count; i++) {
        bool connected = IsGamepadAvailable(i - 1);
        char line[96];
        snprintf(line, sizeof(line), "Spieler %d: Gamepad %d  %s", i + 1, i, connected ? "(verbunden)" : "(nicht gefunden)");
        DrawText(line, px + 30, list_y + i * 24, 18, connected ? LIME : (Color){ 220, 90, 90, 255 });
    }

    int unlocked = 0;
    for (int i = 0; i < ACHIEVEMENT_COUNT; i++) if (gs->progress.achievements[i]) unlocked++;
    char ach_buf[32];
    snprintf(ach_buf, sizeof(ach_buf), "Erfolge (%d/%d)", unlocked, ACHIEVEMENT_COUNT);
    DrawMenuButton(Menu_AchievementsButtonRect(), ach_buf, 18, (Color){ 55, 50, 70, 255 });

    const char *mute_hint = "[M] Ton an/aus";
    DrawText(mute_hint, 20, WINDOW_H - 30, 16, LIGHTGRAY);

    Rectangle vol_r = Menu_VolumeSliderRect();
    char vol_buf[32];
    snprintf(vol_buf, sizeof(vol_buf), "Lautstaerke: %d%%", (int)(Audio_GetVolume() * 100.0f + 0.5f));
    DrawText(vol_buf, (int)vol_r.x, (int)vol_r.y - 20, 15, LIGHTGRAY);
    DrawRectangleRec(vol_r, (Color){ 50, 45, 40, 255 });
    DrawRectangleRec((Rectangle){ vol_r.x, vol_r.y, vol_r.width * Audio_GetVolume(), vol_r.height },
                      Audio_IsMuted() ? GRAY : LIME);
    DrawRectangleLinesEx(vol_r, 2, GOLD);

    if (gs->menu.show_achievements) DrawAchievementsOverlay(gs);
}

void Render_Frame(GameState *gs) {
    float time = (float)GetTime();
    BeginDrawing();
    ClearBackground(BLACK);

    if (gs->state == STATE_MENU) {
        Render_Menu(gs);
    } else {
        for (int i = 0; i < gs->active_players; i++) DrawViewport(gs, i, time);
        if (gs->active_players == 3) DrawEmptySlot(3, gs->active_players);
        DrawViewportDividers(gs);
        DrawHud(gs);
        DrawContractBanner(gs);
        if (gs->economy.crop_unlocked > 0) DrawCropBar(gs, time);
        if (gs->shop.open) DrawShop(gs);
    }

    EndDrawing();
}
