#include "farm.h"
#include <math.h>

static const Color PLAYER_TINTS[MAX_PLAYERS] = {
    { 255, 255, 255, 255 }, // P1 - keyboard/mouse, natural colors
    { 150, 190, 255, 255 }, // P2 - cool blue
    { 170, 230, 150, 255 }, // P3 - green
    { 255, 180, 120, 255 }, // P4 - orange
};

void Player_InitAll(GameState *gs, int count) {
    gs->active_players = count;

    for (int i = 0; i < count; i++) {
        Player *p = &gs->players[i];
        p->id = i;
        p->x = WORLD_PX_W * (0.2f + 0.2f * i);
        p->y = WORLD_PX_H * 0.5f;
        p->facing = FACE_DOWN;
        p->moving = false;
        p->anim_timer = 0.0f;
        p->anim_frame = 0;
        p->bob_phase = 0.0f;
        p->interact_cd = 0.0f;
        p->interact_anim = 0.0f;
        p->tint = PLAYER_TINTS[i];
        p->mouse_hover = false;

        if (i == 0) {
            p->scheme = INPUT_KEYBOARD_MOUSE;
            p->k_up = KEY_W; p->k_down = KEY_S; p->k_left = KEY_A; p->k_right = KEY_D;
            p->k_interact = KEY_SPACE;
            p->k_menu = KEY_B;
        } else {
            p->scheme = INPUT_GAMEPAD;
            p->gamepad_index = i - 1;
        }
    }
}

static void UpdateMouseAim(GameState *gs, Player *p) {
    Rectangle vp = Layout_Viewport(gs->active_players, p->id);
    Vector2 mouse = GetMousePosition();

    if (!CheckCollisionPointRec(mouse, vp)) {
        p->mouse_hover = false;
        return;
    }

    float cam_x, cam_y;
    Layout_CameraForPlayer(gs, p->id, vp, &cam_x, &cam_y);

    float world_x = mouse.x - vp.x + cam_x;
    float world_y = mouse.y - vp.y + cam_y;
    int tx = (int)(world_x / TILE_DRAW);
    int ty = (int)(world_y / TILE_DRAW);
    if (tx < 0) tx = 0;
    if (ty < 0) ty = 0;
    if (tx >= WORLD_W) tx = WORLD_W - 1;
    if (ty >= WORLD_H) ty = WORLD_H - 1;

    p->facing_tile_x = tx;
    p->facing_tile_y = ty;
    p->mouse_hover = true;
}

void Player_Update(GameState *gs, Player *p, float dt) {
    float speed = 140.0f * gs->economy.speed_mult;
    bool moved = false;
    float move_strength = 1.0f;

    if (p->scheme == INPUT_KEYBOARD_MOUSE) {
        if (IsKeyDown(p->k_up))    { p->y -= speed * dt; p->facing = FACE_UP;    moved = true; }
        if (IsKeyDown(p->k_down))  { p->y += speed * dt; p->facing = FACE_DOWN;  moved = true; }
        if (IsKeyDown(p->k_left))  { p->x -= speed * dt; p->facing = FACE_LEFT;  moved = true; }
        if (IsKeyDown(p->k_right)) { p->x += speed * dt; p->facing = FACE_RIGHT; moved = true; }
    } else {
        float ax = 0.0f, ay = 0.0f;
        if (IsGamepadAvailable(p->gamepad_index)) {
            ax = GetGamepadAxisMovement(p->gamepad_index, GAMEPAD_AXIS_LEFT_X);
            ay = GetGamepadAxisMovement(p->gamepad_index, GAMEPAD_AXIS_LEFT_Y);
        }
        float mag = sqrtf(ax * ax + ay * ay);
        if (mag > GAMEPAD_DEADZONE) {
            move_strength = mag > 1.0f ? 1.0f : mag;
            p->x += ax * speed * move_strength * dt;
            p->y += ay * speed * move_strength * dt;
            moved = true;
            if (fabsf(ax) > fabsf(ay)) p->facing = ax < 0 ? FACE_LEFT : FACE_RIGHT;
            else                       p->facing = ay < 0 ? FACE_UP   : FACE_DOWN;
        }
    }

    if (p->x < 0) p->x = 0;
    if (p->y < 0) p->y = 0;
    if (p->x > WORLD_PX_W - PLAYER_SIZE) p->x = WORLD_PX_W - PLAYER_SIZE;
    if (p->y > WORLD_PX_H - PLAYER_SIZE) p->y = WORLD_PX_H - PLAYER_SIZE;

    p->moving = moved;
    if (moved) {
        p->bob_phase += dt * (3.0f + 3.0f * move_strength);
        p->anim_timer += dt;
        if (p->anim_timer > 0.15f) {
            p->anim_timer = 0.0f;
            p->anim_frame = (p->anim_frame + 1) % 3;
        }
    } else {
        p->anim_frame = 0;
        p->anim_timer = 0.0f;
        p->bob_phase = 0.0f;
    }

    World_UpdateFacingTile(p);
    if (p->scheme == INPUT_KEYBOARD_MOUSE) UpdateMouseAim(gs, p);

    bool interact_down = false;
    if (p->scheme == INPUT_KEYBOARD_MOUSE) {
        interact_down = IsKeyDown(p->k_interact) ||
                         (p->mouse_hover && IsMouseButtonDown(MOUSE_BUTTON_LEFT));
    } else if (IsGamepadAvailable(p->gamepad_index)) {
        interact_down = IsGamepadButtonDown(p->gamepad_index, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    }

    if (p->interact_cd > 0.0f) p->interact_cd -= dt;
    if (p->interact_anim > 0.0f) p->interact_anim -= dt;
    if (interact_down && p->interact_cd <= 0.0f) {
        World_Interact(gs, p);
        p->interact_cd = 0.25f;
        p->interact_anim = 0.22f;
    }
}
