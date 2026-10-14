#include "farm.h"
#include <stdlib.h>
#include <math.h>

// All sound in this game is synthesized at startup (simple sine/square tones
// with an envelope) instead of shipping audio asset files.

#define SAMPLE_RATE 44100

typedef struct {
    float freq;
    float duration;
    int waveform; // 0 = sine, 1 = square
} Note;

static Sound sfx[SFX_COUNT];
static Sound music;
static bool muted = false;
static bool music_started = false;
static float volume = 1.0f;

static void ApplyVolume(void) {
    SetMasterVolume(muted ? 0.0f : volume);
}

static float Envelope(float t, float duration) {
    float attack = duration * 0.05f;
    float release = duration * 0.35f;
    if (t < attack) return t / attack;
    if (t > duration - release) return (duration - t) / release;
    return 1.0f;
}

static Wave BuildWave(const Note *notes, int count, float volume) {
    float total = 0.0f;
    for (int i = 0; i < count; i++) total += notes[i].duration;
    int frames = (int)(total * SAMPLE_RATE);
    short *data = (short *)malloc(sizeof(short) * (size_t)frames);

    int pos = 0;
    for (int i = 0; i < count; i++) {
        int n = (int)(notes[i].duration * SAMPLE_RATE);
        for (int s = 0; s < n && pos < frames; s++, pos++) {
            float t = (float)s / SAMPLE_RATE;
            float phase = notes[i].freq * t;
            float raw = (notes[i].waveform == 1)
                ? (fmodf(phase, 1.0f) < 0.5f ? 1.0f : -1.0f)
                : sinf(2.0f * PI * phase);
            float env = Envelope(t, notes[i].duration);
            data[pos] = (short)(raw * env * volume * 32000.0f);
        }
    }

    Wave w = { 0 };
    w.frameCount = (unsigned int)frames;
    w.sampleRate = SAMPLE_RATE;
    w.sampleSize = 16;
    w.channels = 1;
    w.data = data;
    return w;
}

static Sound MakeSfx(const Note *notes, int count, float volume) {
    Wave w = BuildWave(notes, count, volume);
    Sound s = LoadSoundFromWave(w);
    UnloadWave(w);
    return s;
}

void Audio_Init(void) {
    InitAudioDevice();

    sfx[SFX_PLANT] = MakeSfx((Note[]){ { 330, 0.08f, 0 }, { 440, 0.10f, 0 } }, 2, 0.5f);
    sfx[SFX_HARVEST] = MakeSfx((Note[]){ { 523, 0.09f, 0 }, { 659, 0.09f, 0 }, { 784, 0.14f, 0 } }, 3, 0.55f);
    sfx[SFX_WEED] = MakeSfx((Note[]){ { 180, 0.18f, 1 }, { 90, 0.22f, 1 } }, 2, 0.35f);
    sfx[SFX_DENY] = MakeSfx((Note[]){ { 160, 0.12f, 1 } }, 1, 0.3f);
    sfx[SFX_UPGRADE] = MakeSfx((Note[]){ { 659, 0.08f, 0 }, { 988, 0.16f, 0 } }, 2, 0.5f);
    sfx[SFX_QUEST_COMPLETE] = MakeSfx((Note[]){ { 523, 0.1f, 0 }, { 659, 0.1f, 0 }, { 784, 0.1f, 0 }, { 1046, 0.22f, 0 } }, 4, 0.55f);
    sfx[SFX_ACHIEVEMENT] = MakeSfx((Note[]){ { 784, 0.1f, 0 }, { 1046, 0.1f, 0 }, { 1318, 0.3f, 0 } }, 3, 0.6f);
    sfx[SFX_UI_CLICK] = MakeSfx((Note[]){ { 440, 0.05f, 0 } }, 1, 0.4f);
    sfx[SFX_UI_MOVE] = MakeSfx((Note[]){ { 330, 0.04f, 0 } }, 1, 0.3f);
    sfx[SFX_SHOP_TOGGLE] = MakeSfx((Note[]){ { 392, 0.06f, 0 }, { 523, 0.08f, 0 } }, 2, 0.4f);

    Note pad[] = { { 196, 2.0f, 0 }, { 246, 2.0f, 0 }, { 196, 2.0f, 0 }, { 294, 2.0f, 0 } };
    Wave music_wave = BuildWave(pad, 4, 0.16f);
    music = LoadSoundFromWave(music_wave);
    UnloadWave(music_wave);

    ApplyVolume();
}

void Audio_Shutdown(void) {
    for (int i = 0; i < SFX_COUNT; i++) UnloadSound(sfx[i]);
    UnloadSound(music);
    CloseAudioDevice();
}

void Audio_Play(SfxId id) {
    if (muted) return;
    if (id < 0 || id >= SFX_COUNT) return;
    PlaySound(sfx[id]);
}

void Audio_UpdateMusic(void) {
    if (muted) {
        if (IsSoundPlaying(music)) StopSound(music);
        music_started = false;
        return;
    }
    if (!music_started || !IsSoundPlaying(music)) {
        PlaySound(music);
        music_started = true;
    }
}

void Audio_ToggleMute(void) {
    muted = !muted;
    if (muted) StopSound(music);
    ApplyVolume();
}

bool Audio_IsMuted(void) { return muted; }

void Audio_SetVolume(float v) {
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    volume = v;
    ApplyVolume();
}

float Audio_GetVolume(void) { return volume; }
