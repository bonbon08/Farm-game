#include "farm.h"

static GameState gs;

#define AUTOSAVE_INTERVAL 30.0f

int main(void) {
    InitWindow(WINDOW_W, WINDOW_H, "Farmi - Couch Coop");
    SetTargetFPS(60);
    Audio_Init();

    gs.spritesheet = LoadTexture("assets/SpriteSheet.png");
    SetTextureFilter(gs.spritesheet, TEXTURE_FILTER_POINT);

    Image icon_img = LoadImage("assets/SpriteSheet.png");
    Image icon = ImageFromImage(icon_img, (Rectangle){ SPR_RIPE * TILE_PX, 0, TILE_PX, TILE_PX });
    SetWindowIcon(icon);
    UnloadImage(icon);
    UnloadImage(icon_img);

    Menu_Init(&gs);
    gs.menu.has_save = Save_Load(&gs);
    gs.state = STATE_MENU; // Save_Load must not pull us straight into gameplay

    float autosave_timer = 0.0f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (IsKeyPressed(KEY_M)) Audio_ToggleMute();
        Audio_UpdateMusic();

        if (gs.state == STATE_MENU) {
            Menu_Update(&gs);
        } else if (!gs.shop.open && IsKeyPressed(KEY_ESCAPE)) {
            Save_Game(&gs);
            gs.menu.has_save = true;
            gs.state = STATE_MENU;
        } else {
            Shop_ToggleRequested(&gs);
            if (gs.shop.open) {
                Shop_HandleInput(&gs);
            } else {
                for (int i = 0; i < gs.active_players; i++) {
                    Player_Update(&gs, &gs.players[i], dt);
                }
                World_Update(&gs, dt);
                DayNight_Update(&gs, dt);
                Weather_Update(&gs, dt);
                Contracts_Update(&gs, dt);
                Drones_Update(&gs, dt);
                Mowers_Update(&gs, dt);
                Birds_Update(&gs, dt);
                Dogs_Update(&gs, dt);
                Crops_HandleInput(&gs);
            }
            Toast_Update(&gs, dt);
            Progress_Update(&gs);

            autosave_timer += dt;
            if (autosave_timer >= AUTOSAVE_INTERVAL) {
                autosave_timer = 0.0f;
                Save_Game(&gs);
                gs.menu.has_save = true;
            }
        }

        Render_Frame(&gs);
    }

    if (gs.state == STATE_PLAYING) Save_Game(&gs);

    UnloadTexture(gs.spritesheet);
    Audio_Shutdown();
    CloseWindow();
    return 0;
}
