#ifndef FARM_H
#define FARM_H

#include "raylib.h"
#include <stdbool.h>

// ---- Display / world geometry ----
#define TILE_PX        16      // size of one tile in the source spritesheet
#define TILE_SCALE     2       // on-screen scale factor
#define TILE_DRAW      (TILE_PX * TILE_SCALE)

// The map is allocated at its largest possible size up front; "Feld-
// Erweiterung" (see MAX_FIELD_LEVEL below) just raises how much of it is
// actually usable, from FIELD_BASE_W x FIELD_BASE_H up to the full WORLD_W x
// WORLD_H. Players can walk and look anywhere on the physical map (no
// invisible walls), but planting/automation only work within the unlocked
// area, and locked tiles render as visibly closed-off ground.
#define FIELD_BASE_W   50
#define FIELD_BASE_H   30
#define FIELD_STEP_W   4
#define FIELD_STEP_H   2
#define MAX_FIELD_LEVEL 10
#define WORLD_W        (FIELD_BASE_W + MAX_FIELD_LEVEL * FIELD_STEP_W) // 90 tiles
#define WORLD_H        (FIELD_BASE_H + MAX_FIELD_LEVEL * FIELD_STEP_H) // 50 tiles
#define WORLD_PX_W     (WORLD_W * TILE_DRAW)
#define WORLD_PX_H     (WORLD_H * TILE_DRAW)

#define HUD_H          84
#define VIEW_W         640
#define VIEW_H         700
#define WINDOW_W       (VIEW_W * 2)
#define WINDOW_H       (VIEW_H + HUD_H)

#define MAX_PLAYERS    4
#define PLAYER_SIZE    (TILE_DRAW)
#define GAMEPAD_DEADZONE 0.25f

// 60-level upgrade ladder (raised from 40). Per-level steps are deliberately
// small so the endgame multiplier lands around a reasonable place rather than
// exploding - levelling up just happens often and gradually, which is the
// point: more frequent purchases. NOTE: radius-based upgrades (mow/swarm
// range, auto-farm rings, field size) deliberately do NOT follow this same
// cap - their area grows quadratically with level, so matching 60 here would
// make them spatially absurd long before the map runs out. That's an
// intentional asymmetry, not an oversight.
#define MAX_UPGRADE_LEVEL 60
#define SPEED_STEP   0.07f
#define GROWTH_STEP  0.08f
#define SELL_STEP    0.05f

// Auto-farm levels are diamond-shaped rings around each player: level 1 works
// the 4 orthogonal neighbours, level 2 adds the next ring of 8 (12 total -
// "two full rings"), each further level adds one more ring of 4*level tiles.
// Empty tiles in range get planted, ripe tiles get harvested. Raised by +10
// along with every other upgrade cap; at the new max the rings cover most of
// the map, which is an intended (if extreme) late-game payoff, not a bug.
#define MAX_AUTO_PLANT_LEVEL 18
#define DRONE_SPEED_STEP 0.08f
#define MAX_DRONES 30

// Mowers: dedicated weed-removal units (drones/rings never touch weeds or
// overripe plants - this is their one job). They patrol the whole map in
// horizontal lanes, raster-scanning left-to-right then down, one lane per
// mower so several of them spread out across the field instead of covering
// the same ground twice. MOWER_SPEED_STEP is deliberately steeper than the
// other speed upgrades so a fully upgraded mower is dramatically, visibly
// faster than anything else on the field by the time it hits max level.
#define MAX_MOWERS 30
#define MOWER_SPEED_STEP 0.15f
#define MAX_MOWER_RANGE_LEVEL 16    // mow-radius upgrade: radius in tiles == level
#define WEED_RESIST_STEP 0.03f      // per level, multiplicative reduction of weed spawn chance
#define WEED_RESIST_FLOOR 0.1f      // weed chance never drops below 10% of its base value

// Crop tiers: a prestige ladder on top of the regular shop. Unlocking the
// next crop costs real money (first tier 1,000,000 Gold, each further tier
// 2.5x the previous unlock cost) but is no longer destructive: growth/sell
// progress is tracked per crop tier (see CropTrack below) and nothing is ever
// wiped - switching crops (free, any time once unlocked) just changes which
// tier's own growth/sell levels and field tiles are currently active, while
// shared infrastructure (drones, mowers, auto-farm, speed) keeps working
// regardless of which crop is active. Each unlocked tier sells for 3x the
// previous tier's payout, compounding (tier1=3x, tier2=9x, tier3=27x, ...).
// Only the first 4 tiers have real names/art (Weizen/Kartoffeln/Karotten/
// Zucchini); MAX_CROP_TIERS goes far beyond that so the prestige ladder is,
// in practical terms, endless - the exponential cost curve alone makes tier
// ~10+ a many-lifetimes-of-play goal, not a hard wall.
#define MAX_CROP_TIERS 20
#define CROP_NAMED_COUNT 4
#define CROP_FIRST_UNLOCK_COST 1000000
#define CROP_UNLOCK_COST_MULT 2.5f
#define CROP_PAYOUT_MULT 3.0f

// Drones get a batch-radius upgrade mirroring the mowers' mow-radius - once
// the drone count itself is capped at MAX_DRONES, this is the next place to
// keep investing: each drone also finishes off matching tiles in a radius
// around wherever it's already standing, instead of exactly one per stop.
#define MAX_DRONE_SWARM_LEVEL 16

// Scheune ("barn"): passive income, trickling in whether or not anyone is
// actively harvesting - scales with Hof-Level so it stays relevant lategame.
#define MAX_BARN_LEVEL 60
#define BARN_STEP 0.4f

// Vogelscheuche ("scarecrow"): a plain multiplicative reduction of bird
// spawn chance, same shape as Unkraut-Resistenz.
#define MAX_SCARECROW_LEVEL 60
#define SCARECROW_STEP 0.025f
#define SCARECROW_FLOOR 0.15f
#define SCARECROW_VISIBLE_MAX 24  // purely cosmetic cap on how many posts get drawn

// Hunde ("dogs"): purchasable autonomous defenders that hunt down birds
// already on the field, the active counterpart to the passive scarecrow.
#define MAX_DOGS 20

// Birds are an adversarial, non-purchasable pest: they spawn on their own,
// hunt the nearest ripe (and, from BIRD_STRONG_DAY on, sprouting) crop, and
// destroy everything ripe in a radius around wherever they land if they get
// there first - not just the one tile. Both how often they show up and how
// wide that radius is get worse the more days go by.
#define MAX_BIRDS 12
#define BIRD_STRONG_DAY 10

// Drones and mowers can break down from wear and need repairing - keeps
// automation from being a permanent "set and forget" once fully upgraded.
// The per-action wear amount is deliberately small - breakdowns should be an
// occasional, noticeable event over a long session, not a constant chore.
#define UNIT_WEAR_MAX 100.0f
#define UNIT_WEAR_PER_ACTION_MIN 0.2f
#define UNIT_WEAR_PER_ACTION_MAX 0.6f
#define UNIT_REPAIR_COST_EACH 50

#define SHOP_ITEM_COUNT 17

// Weather: rolls a new condition whenever the current one runs out, lasting
// WEATHER_MIN_DURATION..WEATHER_MAX_DURATION seconds. WEATHER_CLEAR is by far
// the most common roll so the others stay noteworthy events, not the norm.
#define WEATHER_MIN_DURATION 20.0f
#define WEATHER_MAX_DURATION 40.0f
#define WEATHER_RAIN_GROWTH_FACTOR    0.65f  // <1 = faster (shorter stage duration)
#define WEATHER_DROUGHT_GROWTH_FACTOR 1.6f   // >1 = slower
#define WEATHER_RAIN_WEED_FACTOR      1.3f
#define WEATHER_STORM_BIRD_FACTOR     2.2f
#define WEATHER_BLESSING_SELL_MULT    2.0f

// Contracts: a bonus, time-limited objective for one specific (randomly
// chosen, currently unlocked) crop tier, on top of the regular endless quest.
#define CONTRACT_MIN_SPAWN_DELAY 60.0f
#define CONTRACT_MAX_SPAWN_DELAY 120.0f
#define CONTRACT_MIN_TARGET 10
#define CONTRACT_MAX_TARGET 30
#define CONTRACT_TIME_LIMIT 75.0f
#define CONTRACT_REWARD_PER_UNIT 15

// ---- Sprite indices in the spritesheet (12 tiles, 16x16 each) ----
#define SPR_SPROUT     0
#define SPR_RIPE       1
#define SPR_OVERRIPE   2
#define SPR_WEED       3
#define SPR_SOIL       4
#define SPR_FRONT_A    5
#define SPR_FRONT_B    6
#define SPR_FRONT_C    7
#define SPR_BACK_A     8
#define SPR_BACK_B     9
#define SPR_BACK_C     10
#define SPR_CURSOR     11

// ---- App state ----
typedef enum { STATE_MENU = 0, STATE_PLAYING } AppState;

// ---- Tiles ----
typedef enum {
    TILE_EMPTY    = 0,
    TILE_SPROUT   = 1,
    TILE_RIPE     = 2,
    TILE_OVERRIPE = 3,
    TILE_WEED     = 4
} TileState;

typedef struct {
    TileState state;
    float timer;     // seconds until the next growth step
    float pop_timer; // >0 right after a state change, drives a brief "pop" juice effect
    int crop_type;   // which crop this tile was planted as (0..MAX_CROP_TIERS-1) - fixed at
                      // planting time and never changes when the player later switches
                      // which crop is "active"; growth speed and sell price both use this.
} Tile;

// ---- Players ----
typedef enum { FACE_DOWN = 0, FACE_LEFT, FACE_UP, FACE_RIGHT } Facing;
typedef enum { INPUT_KEYBOARD_MOUSE = 0, INPUT_GAMEPAD } InputScheme;

typedef struct {
    int id;
    float x, y;        // pixel position in world space (top-left of sprite)
    Facing facing;
    bool moving;
    float anim_timer;
    int anim_frame;
    float bob_phase;
    float interact_cd;
    float interact_anim; // >0 right after interacting, drives a quick "bump" animation
    Color tint;

    InputScheme scheme;
    int gamepad_index; // valid only when scheme == INPUT_GAMEPAD
    KeyboardKey k_up, k_down, k_left, k_right, k_interact, k_menu; // valid only for keyboard/mouse

    bool mouse_hover; // true this frame if the mouse points at a tile in this player's viewport
    int facing_tile_x, facing_tile_y;
} Player;

// Per-crop-tier progress: how fast *that* crop grows and what it sells for.
// Deliberately NOT reset when a new tier is unlocked or when the player
// switches which tier is active - each tier's own little upgrade track just
// sits there, parked, exactly as the player left it.
typedef struct {
    int growth_level; float growth_mult;
    int sell_level;   float sell_mult;
} CropTrack;

// ---- Economy / upgrades ----
typedef struct {
    int money;

    int speed_level;
    float speed_mult;   // movement speed multiplier (global - not plant-specific)

    CropTrack crop_track[MAX_CROP_TIERS];
    int crop_unlocked; // highest crop tier unlocked, 0..MAX_CROP_TIERS-1
    int crop_active;   // crop tier currently being planted, 0..crop_unlocked

    int auto_plant_level;  // 0 = off, up to MAX_AUTO_PLANT_LEVEL
    float auto_plant_cd;   // seconds until the next automatic planting pass

    int drone_speed_level;
    float drone_speed_mult; // movement + turnaround speed multiplier for all drones
    int drone_swarm_level;  // 0..MAX_DRONE_SWARM_LEVEL, batch-radius around each drone's stop

    int mower_speed_level;
    float mower_speed_mult; // movement speed multiplier for all mowers

    int mower_range_level;  // 0..MAX_MOWER_RANGE_LEVEL, mow-radius in tiles around each mower

    int weed_resist_level;  // 0..MAX_UPGRADE_LEVEL
    float weed_resist_mult; // multiplies the base weed-spawn chance, down to WEED_RESIST_FLOOR

    int barn_level;    // 0..MAX_BARN_LEVEL, passive gold/sec (scaled by Hof-Level)
    float barn_accum;  // fractional gold carried between frames

    int field_level;  // 0..MAX_FIELD_LEVEL, how much of the map is currently usable

    int scarecrow_level;   // 0..MAX_SCARECROW_LEVEL
    float scarecrow_mult;  // multiplies bird spawn chance, down to SCARECROW_FLOOR
} Economy;

// ---- Drones: purchasable autonomous workers that plant and harvest on their own ----
typedef enum { DRONE_TASK_HARVEST, DRONE_TASK_HARVEST_OVERRIPE, DRONE_TASK_PLANT } DroneTask;

typedef struct {
    bool active;
    float x, y;
    int target_x, target_y;
    bool has_target;
    DroneTask task;
    float action_timer;
    float bob_seed;
    bool broken;  // worn out - sits idle until repaired from the shop
    float wear;   // 0..UNIT_WEAR_MAX, ticks up with each completed action
} Drone;

// ---- Mowers: purchasable autonomous weed-removal units ----
// Each mower raster-scans the whole map in its own horizontal lane (row index
// i, i+count, i+2*count, ...), moving left-to-right then dropping to its next
// lane, mowing any weed within mower_range tiles of wherever it currently is.
typedef struct {
    bool active;
    float x, y;
    int tx, ty;          // current scan-destination tile
    float action_timer;
    float bob_seed;
    bool broken;
    float wear;
} Mower;

// ---- Birds: adversarial pest that eats ripe (and, past BIRD_STRONG_DAY,
// sprouting) crops before the player can harvest them ----
typedef struct {
    bool active;
    float x, y;
    int target_x, target_y;
    bool has_target;
} Bird;

// ---- Dogs: purchasable autonomous defenders that chase down active birds ----
typedef struct {
    bool active;
    float x, y;
    int target_bird;
    bool has_target;
    float bob_seed;
} Dog;

// ---- Day / night ----
typedef struct {
    float day_length;   // seconds per full day/night cycle
    float time_of_day;  // 0..1, 0 = sunrise
    int day_count;
    float night_factor; // 0 = full day, 1 = deep night (for tint + weed chance)
} DayNight;

// ---- Weather: temporary randomised modifiers, so sessions don't all play
// out identically - something to react to beyond "buy the next upgrade" ----
typedef enum {
    WEATHER_CLEAR = 0,
    WEATHER_RAIN,
    WEATHER_DROUGHT,
    WEATHER_STORM,
    WEATHER_BLESSING,
    WEATHER_COUNT
} WeatherType;

typedef struct {
    WeatherType type;
    float timer; // seconds remaining in the current weather
} Weather;

// ---- Contracts: periodic time-limited bonus objectives for a specific crop,
// separate from and on top of the regular endless quest chain - urgency and
// variety rather than another number to grind ----
typedef struct {
    bool active;
    int crop_type;
    int target;
    int progress;
    float time_left;
    float time_total; // for drawing the countdown bar
    int reward;
} Contract;

// ---- Quests ----
typedef enum { QUEST_HARVEST, QUEST_EARN, QUEST_PLANT, QUEST_TYPE_COUNT } QuestType;

typedef struct {
    QuestType type;
    int target;
    int progress;
    int reward;
    int baseline_money; // money snapshot at quest start, for QUEST_EARN
    char description[96];
    int index; // quest serial number, used to scale difficulty
} Quest;

// ---- Shared stats used by quests ----
typedef struct {
    int total_harvested;
    int total_planted;
} Stats;

// ---- Long-term progression: the "why farm" motivation layer ----
#define ACHIEVEMENT_COUNT 10

typedef struct {
    int total_money_earned;   // lifetime, cumulative positive income only, never decreases
    int quests_completed;     // lifetime
    bool ever_four_players;   // unlocks the 4-player achievement
    bool achievements[ACHIEVEMENT_COUNT];
} Progress;

// ---- Toast / notification ----
typedef struct {
    char text[128];
    float timer;
} Toast;

// ---- Shop ----
typedef struct {
    bool open;
    int selection; // 0..SHOP_ITEM_COUNT-1
} Shop;

// ---- Main menu ----
typedef struct {
    int player_count;      // 1..MAX_PLAYERS
    bool has_save;
    bool show_achievements;
} Menu;

// ---- Aggregate game state ----
typedef struct {
    AppState state;
    Menu menu;

    Tile world[WORLD_W][WORLD_H];
    Player players[MAX_PLAYERS];
    int active_players;

    Drone drones[MAX_DRONES];
    int drone_count;

    Mower mowers[MAX_MOWERS];
    int mower_count;

    Bird birds[MAX_BIRDS];
    float bird_spawn_timer;

    Dog dogs[MAX_DOGS];
    int dog_count;

    Economy economy;
    DayNight daynight;
    Weather weather;
    Contract contract;
    float contract_spawn_timer;
    Quest quest;
    Stats stats;
    Progress progress;
    Toast toast;
    Shop shop;
    Texture2D spritesheet;
} GameState;

// ---- layout.c ----
Rectangle Layout_Viewport(int active_players, int slot);
void Layout_CameraForPlayer(GameState *gs, int slot, Rectangle vp, float *cam_x, float *cam_y);

// ---- world.c ----
void World_Init(GameState *gs);
void World_Update(GameState *gs, float dt);
void World_UpdateFacingTile(Player *p);
void World_Interact(GameState *gs, Player *p);
float World_StageDuration(GameState *gs, int crop_type);
int Field_W(GameState *gs);
int Field_H(GameState *gs);

// ---- player.c ----
void Player_InitAll(GameState *gs, int count);
void Player_Update(GameState *gs, Player *p, float dt);

// ---- systems.c ----
void Systems_Init(GameState *gs);
void DayNight_Update(GameState *gs, float dt);
void Shop_ToggleRequested(GameState *gs);
void Shop_HandleInput(GameState *gs);
void Shop_Buy(GameState *gs, int which);
Rectangle Shop_PanelRect(void);
Rectangle Shop_RowRect(int index);
int Shop_UpgradeCost(int level);
void Quest_Init(GameState *gs, int index);
void Quest_Restore(GameState *gs, QuestType type, int index, int progress);
void Quest_Notify(GameState *gs, QuestType type, int amount);
void Quest_CheckComplete(GameState *gs);
void Toast_Show(GameState *gs, const char *text);
void Toast_Update(GameState *gs, float dt);

// ---- progress.c ----
void Progress_Init(GameState *gs);
void Progress_AddEarnings(GameState *gs, int amount);
void Progress_Update(GameState *gs);
int Progress_Level(GameState *gs);
const char *Progress_Title(GameState *gs);
float Progress_LevelProgress(GameState *gs);
const char *Achievement_Name(int id);
const char *Achievement_Desc(int id);

// ---- audio.c ----
typedef enum {
    SFX_PLANT = 0,
    SFX_HARVEST,
    SFX_WEED,
    SFX_DENY,
    SFX_UPGRADE,
    SFX_QUEST_COMPLETE,
    SFX_ACHIEVEMENT,
    SFX_UI_CLICK,
    SFX_UI_MOVE,
    SFX_SHOP_TOGGLE,
    SFX_COUNT
} SfxId;

void Audio_Init(void);
void Audio_Shutdown(void);
void Audio_Play(SfxId id);
void Audio_UpdateMusic(void);
void Audio_ToggleMute(void);
bool Audio_IsMuted(void);
void Audio_SetVolume(float v);
float Audio_GetVolume(void);

// ---- drones.c ----
void Drones_Init(GameState *gs);
void Drones_Update(GameState *gs, float dt);
bool Drones_Buy(GameState *gs);
int Drones_Cost(GameState *gs);
void Drones_SetCount(GameState *gs, int count);

// ---- mowers.c ----
void Mowers_Init(GameState *gs);
void Mowers_Update(GameState *gs, float dt);
bool Mowers_Buy(GameState *gs);
int Mowers_Cost(GameState *gs);
void Mowers_SetCount(GameState *gs, int count);

// ---- crops.c ----
const char *Crop_Name(int tier);
float Crop_Mult(int tier);
int Crop_UnlockCost(int tier); // cost to unlock tier `tier` (1..MAX_CROP_TIERS-1)
void Crops_Unlock(GameState *gs);
void Crops_HandleInput(GameState *gs);
Rectangle Crop_ButtonRect(int slot, int total);
void DrawCropIcon(int tier, float cx, float cy, float scale, float time);

// ---- pests.c ----
void Pests_Init(GameState *gs);
void Birds_Update(GameState *gs, float dt);
void Dogs_Update(GameState *gs, float dt);
bool Dogs_Buy(GameState *gs);
int Dogs_Cost(GameState *gs);
void Dogs_SetCount(GameState *gs, int count);

// ---- weather.c ----
void Weather_Init(GameState *gs);
void Weather_Update(GameState *gs, float dt);
const char *Weather_Name(WeatherType type);
float Weather_GrowthFactor(GameState *gs);  // multiplies stage duration (>1 = slower)
float Weather_WeedFactor(GameState *gs);    // multiplies weed-spawn chance
float Weather_BirdFactor(GameState *gs);    // multiplies bird spawn chance
float Weather_SellMult(GameState *gs);      // multiplies harvest payout

// ---- contracts.c ----
void Contracts_Init(GameState *gs);
void Contracts_Update(GameState *gs, float dt);
void Contracts_Notify(GameState *gs, int crop_type, int amount);

// ---- save.c ----
bool Save_Exists(void);
void Save_Game(GameState *gs);
bool Save_Load(GameState *gs);

// ---- menu.c ----
void Menu_Init(GameState *gs);
void Menu_Update(GameState *gs);
void Game_StartSession(GameState *gs);
Rectangle Menu_LeftArrowRect(void);
Rectangle Menu_RightArrowRect(void);
Rectangle Menu_ContinueButtonRect(void);
Rectangle Menu_NewGameButtonRect(void);
Rectangle Menu_AchievementsButtonRect(void);
Rectangle Menu_VolumeSliderRect(void);

// ---- render.c ----
void Render_Frame(GameState *gs);

#endif
