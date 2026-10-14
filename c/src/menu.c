#include "farm.h"

#define PANEL_W 560
#define PANEL_X ((WINDOW_W - PANEL_W) / 2)

Rectangle Menu_LeftArrowRect(void)  { return (Rectangle){ PANEL_X + 60,  280, 56, 56 }; }
Rectangle Menu_RightArrowRect(void) { return (Rectangle){ PANEL_X + PANEL_W - 116, 280, 56, 56 }; }
Rectangle Menu_ContinueButtonRect(void)     { return (Rectangle){ PANEL_X + 30,  380, 240, 56 }; }
Rectangle Menu_NewGameButtonRect(void)      { return (Rectangle){ PANEL_X + 290, 380, 240, 56 }; }
Rectangle Menu_AchievementsButtonRect(void) { return (Rectangle){ PANEL_X + 140, 620, 280, 40 }; }
Rectangle Menu_VolumeSliderRect(void) { return (Rectangle){ WINDOW_W - 240.0f, WINDOW_H - 40.0f, 200.0f, 20.0f }; }

static bool volume_dragging = false;

void Menu_Init(GameState *gs) {
    gs->state = STATE_MENU;
    gs->menu.player_count = 1;
    gs->menu.show_achievements = false;
    Progress_Init(gs);
}

static void ChangeCount(GameState *gs, int delta) {
    gs->menu.player_count += delta;
    if (gs->menu.player_count < 1) gs->menu.player_count = 1;
    if (gs->menu.player_count > MAX_PLAYERS) gs->menu.player_count = MAX_PLAYERS;
    Audio_Play(SFX_UI_MOVE);
}

void Menu_Update(GameState *gs) {
    Vector2 mouse_pos = GetMousePosition();
    Rectangle vol_rect = Menu_VolumeSliderRect();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse_pos, vol_rect)) {
        volume_dragging = true;
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        volume_dragging = false;
    }
    if (volume_dragging) {
        float frac = (mouse_pos.x - vol_rect.x) / vol_rect.width;
        if (frac < 0.0f) frac = 0.0f;
        if (frac > 1.0f) frac = 1.0f;
        Audio_SetVolume(frac);
    }

    if (gs->menu.show_achievements) {
        bool close = IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_E);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            CheckCollisionPointRec(GetMousePosition(), Menu_AchievementsButtonRect())) close = true;
        if (close) {
            gs->menu.show_achievements = false;
            Audio_Play(SFX_UI_CLICK);
        }
        return;
    }

    bool left = IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A);
    bool right = IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D);
    bool do_continue = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
    bool do_new_game = false;
    bool open_achievements = IsKeyPressed(KEY_E);

    if (IsGamepadAvailable(0)) {
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) left = true;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) right = true;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) do_continue = true;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 m = GetMousePosition();
        if (CheckCollisionPointRec(m, Menu_LeftArrowRect())) left = true;
        if (CheckCollisionPointRec(m, Menu_RightArrowRect())) right = true;
        if (gs->menu.has_save && CheckCollisionPointRec(m, Menu_ContinueButtonRect())) do_continue = true;
        if (CheckCollisionPointRec(m, Menu_NewGameButtonRect())) do_new_game = true;
        if (CheckCollisionPointRec(m, Menu_AchievementsButtonRect())) open_achievements = true;
    }

    if (left) ChangeCount(gs, -1);
    if (right) ChangeCount(gs, 1);

    if (!gs->menu.has_save) {
        // without a save file, Enter/Space/gamepad-confirm also just starts a new game
        if (do_continue) do_new_game = true;
        do_continue = false;
    }

    if (do_continue) {
        Audio_Play(SFX_UI_CLICK);
        gs->state = STATE_PLAYING;
    }
    if (do_new_game) {
        Audio_Play(SFX_UI_CLICK);
        Game_StartSession(gs);
    }
    if (open_achievements) {
        gs->menu.show_achievements = true;
        Audio_Play(SFX_UI_CLICK);
    }
}

void Game_StartSession(GameState *gs) {
    World_Init(gs);
    Player_InitAll(gs, gs->menu.player_count);
    Systems_Init(gs);
    gs->state = STATE_PLAYING;
}
