#include "farm.h"

// Computes the on-screen rectangle for a given player's viewport, depending on
// how many players are active: 1 = fullscreen, 2 = side by side, 3-4 = quad grid
// (the 4th cell stays empty when only 3 players are active).
Rectangle Layout_Viewport(int active_players, int slot) {
    int play_h = WINDOW_H - HUD_H;

    if (active_players <= 1) {
        return (Rectangle){ 0, HUD_H, WINDOW_W, play_h };
    }
    if (active_players == 2) {
        int w = WINDOW_W / 2;
        return (Rectangle){ (float)(slot * w), HUD_H, (float)w, (float)play_h };
    }

    int w = WINDOW_W / 2;
    int h = play_h / 2;
    int col = slot % 2;
    int row = slot / 2;
    return (Rectangle){ (float)(col * w), (float)(HUD_H + row * h), (float)w, (float)h };
}

void Layout_CameraForPlayer(GameState *gs, int slot, Rectangle vp, float *cam_x, float *cam_y) {
    Player *p = &gs->players[slot];
    float cx = p->x + PLAYER_SIZE / 2.0f - vp.width / 2.0f;
    float cy = p->y + PLAYER_SIZE / 2.0f - vp.height / 2.0f;

    float max_x = WORLD_PX_W - vp.width;
    float max_y = WORLD_PX_H - vp.height;
    if (max_x < 0) max_x = 0;
    if (max_y < 0) max_y = 0;

    if (cx < 0) cx = 0;
    if (cy < 0) cy = 0;
    if (cx > max_x) cx = max_x;
    if (cy > max_y) cy = max_y;

    *cam_x = cx;
    *cam_y = cy;
}
