#include <stdio.h>
#include <math.h>

#define MINIAUDIO_IMPLEMENTATION 
#include "miniaudio.h"

#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "style_terminal.h"

#define WINDOW_W 500
#define WINDOW_H 420

#define TOTAL_CHANNELS 2
#define SAMPLE_RATE 48000
#define MAX_FREQ 400
#define MIN_FREQ 20
#define MAX_BEAT_FREQ 100.0f
#define MIN_BEAT_FREQ 0.5f

#define MAX_SINE_BUFFER 900
#define PLOT_OUTLINE_THICKNESS 5

ma_waveform waveform_left;
ma_waveform waveform_right;

typedef struct {
    float sine_buffer[MAX_SINE_BUFFER];
    int write_head;
} WaveBuffer;

WaveBuffer sine_buffer_left  = {0};
WaveBuffer sine_buffer_right = {0};

void write_wave_buffer(WaveBuffer* buf, int n, float* values)
{
    for (int i = 0; i < n; ++i) {
        if (buf->write_head >= MAX_SINE_BUFFER)
            buf->write_head = 0;
        buf->sine_buffer[buf->write_head++] = values[i];
    }
}

void draw_wave(float buf[MAX_SINE_BUFFER], int x, int y, int w, int h)
{
    Rectangle rec = {x, y, w, h};
    DrawRectangleLinesEx(rec, PLOT_OUTLINE_THICKNESS,
        GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL)));

    for (int i = 0; i < MAX_SINE_BUFFER; ++i) {
        float sample = buf[i];
        if (sample >  1.0f) sample =  1.0f;
        if (sample < -1.0f) sample = -1.0f;

        int sx = (x + PLOT_OUTLINE_THICKNESS)
                 + (i * (w - 2 * PLOT_OUTLINE_THICKNESS)) / MAX_SINE_BUFFER;
        int sy = (y + h / 2)
                 + (int)(sample * ((h - 2 * PLOT_OUTLINE_THICKNESS) / 2.0f));

        DrawPixel(sx, sy, GetColor(GuiGetStyle(DEFAULT, LINE_COLOR)));
    }
}

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    float* pOutputF32 = (float*)pOutput;
    float temp_left[frameCount];
    float temp_right[frameCount];

    ma_waveform_read_pcm_frames(&waveform_left,  temp_left,  frameCount, NULL);
    ma_waveform_read_pcm_frames(&waveform_right, temp_right, frameCount, NULL);

    write_wave_buffer(&sine_buffer_left,  frameCount, temp_left);
    write_wave_buffer(&sine_buffer_right, frameCount, temp_right);

    for (ma_uint32 i = 0; i < frameCount; i++) {
        pOutputF32[i * 2 + 0] = temp_left[i];
        pOutputF32[i * 2 + 1] = temp_right[i];
    }
}

// ── Preset table ──────────────────────────────────────────────────────────────
typedef struct { const char* label; float base; float beat; const char* description; } Preset;

static const Preset PRESETS[] = {
    { "Delta\n0.5-4 Hz",   100.0f,  2.0f,  "Deep sleep, healing"         },
    { "Theta\n4-8 Hz",     100.0f,  6.0f,  "Meditation, creativity"      },
    { "Alpha\n8-13 Hz",    100.0f, 10.0f,  "Relaxed focus, calm"         },
    { "Schumann\n7.83 Hz", 100.0f,  7.83f, "Earth resonance, grounding"  },
    { "Beta\n13-30 Hz",    100.0f, 20.0f,  "Active thinking, focus"      },
    { "Gamma\n30-100 Hz",  100.0f, 40.0f,  "High cognition, perception"  },
};
#define PRESET_COUNT (sizeof(PRESETS) / sizeof(PRESETS[0]))

static void apply_frequencies(float base, float beat, bool left_is_higher,
                               float* out_left, float* out_right)
{
    *out_left  = left_is_higher ? base + beat * 0.5f : base - beat * 0.5f;
    *out_right = left_is_higher ? base - beat * 0.5f : base + beat * 0.5f;
}

int main()
{
    // ── Initial state ─────────────────────────────────────────────────────────
    float base_freq      = 100.0f;
    float beat_freq      = 7.83f;
    float amplitude      = 0.5f;
    bool  left_is_higher = true;

    float sine_freq_left, sine_freq_right;
    apply_frequencies(base_freq, beat_freq, left_is_higher,
                      &sine_freq_left, &sine_freq_right);

    int   active_preset  = 3;   // Schumann
    const char* preset_desc = PRESETS[3].description;

    // ── miniaudio init ────────────────────────────────────────────────────────
    ma_waveform_config wcfg_left = ma_waveform_config_init(
        ma_format_f32, 1, SAMPLE_RATE,
        ma_waveform_type_sine, amplitude, sine_freq_left);
    if (ma_waveform_init(&wcfg_left, &waveform_left) != MA_SUCCESS) return -1;

    ma_waveform_config wcfg_right = ma_waveform_config_init(
        ma_format_f32, 1, SAMPLE_RATE,
        ma_waveform_type_sine, amplitude, sine_freq_right);
    if (ma_waveform_init(&wcfg_right, &waveform_right) != MA_SUCCESS) return -1;

    ma_device_config config  = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32;
    config.playback.channels = TOTAL_CHANNELS;
    config.sampleRate        = SAMPLE_RATE;
    config.dataCallback      = data_callback;

    ma_device device;
    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) return -1;
    ma_device_start(&device);

    // ── Raylib / raygui init ──────────────────────────────────────────────────
    InitWindow(WINDOW_W, WINDOW_H, "Janus - Binaural Beat Generator");
    GuiLoadStyleTerminal();
    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        bool freq_dirty = false;
        bool amp_dirty  = false;

        // ── Sliders ───────────────────────────────────────────────────────────
        if (GuiSlider((Rectangle){120, 10, 150, 20}, "Base Freq",
                      TextFormat("%5.1f Hz", base_freq),
                      &base_freq, (float)MIN_FREQ, (float)MAX_FREQ))
        {
            freq_dirty   = true;
            active_preset = -1;
        }

        if (GuiSlider((Rectangle){120, 35, 150, 20}, "Beat Freq",
                      TextFormat("%5.2f Hz", beat_freq),
                      &beat_freq, MIN_BEAT_FREQ, MAX_BEAT_FREQ))
        {
            freq_dirty   = true;
            active_preset = -1;
        }

        if (GuiSlider((Rectangle){120, 60, 150, 20}, "Amplitude",
                      TextFormat("%4.2f", amplitude),
                      &amplitude, 0.0f, 1.0f))
        {
            amp_dirty    = true;
            active_preset = -1;
        }

        // Left-is-higher toggle
        bool new_left_is_higher = left_is_higher;
        GuiCheckBox((Rectangle){120, 85, 16, 16}, "L ear higher", &new_left_is_higher);
        if (new_left_is_higher != left_is_higher) {
            left_is_higher = new_left_is_higher;
            freq_dirty     = true;
            active_preset  = -1;
        }

        // ── Preset buttons ────────────────────────────────────────────────────
        GuiLabel((Rectangle){10, 115, 480, 20}, "PRESETS");

        for (int i = 0; i < (int)PRESET_COUNT; i++) {
            Rectangle btn = { 10 + i * 81, 135, 78, 42 };

            // Highlight active preset
            if (i == active_preset)
                GuiSetState(STATE_PRESSED);

            if (GuiButton(btn, PRESETS[i].label)) {
                base_freq     = PRESETS[i].base;
                beat_freq     = PRESETS[i].beat;
                active_preset = i;
                preset_desc   = PRESETS[i].description;
                freq_dirty    = true;
            }

            GuiSetState(STATE_NORMAL);
        }

        // Preset description
        if (active_preset >= 0)
            GuiLabel((Rectangle){10, 182, 480, 20}, preset_desc);

        // ── Apply changes ─────────────────────────────────────────────────────
        if (freq_dirty) {
            apply_frequencies(base_freq, beat_freq, left_is_higher,
                              &sine_freq_left, &sine_freq_right);
            ma_waveform_set_frequency(&waveform_left,  sine_freq_left);
            ma_waveform_set_frequency(&waveform_right, sine_freq_right);
        }
        if (amp_dirty) {
            ma_waveform_set_amplitude(&waveform_left,  amplitude);
            ma_waveform_set_amplitude(&waveform_right, amplitude);
        }

        // ── Info panel ────────────────────────────────────────────────────────
        GuiLabel((Rectangle){330, 10, 185, 80},
                 TextFormat("L:  %.2f Hz\nR:  %.2f Hz\nBeat: %.2f Hz\nAmp:  %.2f",
                            sine_freq_left, sine_freq_right, beat_freq, amplitude));

        // Beat pulse visualizer
        float beat_period   = 1.0f / beat_freq;
        float phase         = fmodf((float)GetTime(), beat_period) / beat_period;
        float pulse         = (sinf(phase * 2.0f * PI) + 1.0f) / 2.0f;
        DrawCircle(450, 50, (int)((WINDOW_W * 0.05) * pulse),
                      Fade(GetColor(GuiGetStyle(DEFAULT, LINE_COLOR)), 0.6f));

        // ── Waveform plots ────────────────────────────────────────────────────
        float difference_buffer[MAX_SINE_BUFFER];
        for (int i = 0; i < MAX_SINE_BUFFER; i++)
            difference_buffer[i] = (sine_buffer_left.sine_buffer[i]
                                  + sine_buffer_right.sine_buffer[i]) * 0.5f;

        BeginDrawing();
            ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

            draw_wave(sine_buffer_left.sine_buffer,  0,   205, 200, 100);
            draw_wave(sine_buffer_right.sine_buffer, 0,   310, 200, 100);
            draw_wave(difference_buffer,             200, 205, 300, 205);

            GuiLabel((Rectangle){5,   205, 60, 12}, "LEFT");
            GuiLabel((Rectangle){5,   310, 60, 12}, "RIGHT");
            GuiLabel((Rectangle){205, 205, 60, 12}, "SUM");

        EndDrawing();
    }

    CloseWindow();
    ma_device_uninit(&device);
    return 0;
}