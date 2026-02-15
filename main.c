#include <stdio.h>

#define MINIAUDIO_IMPLEMENTATION 
#include "miniaudio.h"

#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "style_terminal.h"

#define WINDOW_W 500
#define WINDOW_H 300

#define TOTAL_CHANNELS 2
#define SAMPLE_RATE 48000
#define AMPLITUDE 0.5
#define MAX_FREQ 500
#define MIN_FREQ 20
#define BI_FREQ_RANGE 100

#define MAX_SINE_BUFFER 900
#define PLOT_OUTLINE_THICKNESS 5

ma_waveform waveform_left;
ma_waveform waveform_right;

typedef struct {
    float sine_buffer[MAX_SINE_BUFFER];
    int write_head;
} WaveBuffer;

void write_wave_buffer(WaveBuffer* buf, int n, float values[n])
{
    for(int i = 0; i < n; ++i) {
        if(buf->write_head >= MAX_SINE_BUFFER)
            buf->write_head = 0;

        buf->sine_buffer[buf->write_head++] = values[i]; 
    }
}

void draw_wave(float buf[MAX_SINE_BUFFER], int x, int y, int w, int h)
{
    Rectangle rec = {x, y, w, h};
    
    DrawRectangleLinesEx(
        rec, 
        PLOT_OUTLINE_THICKNESS,
        GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL))
    );
    
    for(int i = 0 ; i < MAX_SINE_BUFFER; ++i){
        

        int screen_x = (x+PLOT_OUTLINE_THICKNESS) + (i * (w-(2*PLOT_OUTLINE_THICKNESS))) / MAX_SINE_BUFFER;
        int screen_y = (y-(PLOT_OUTLINE_THICKNESS/4)) + (buf[i] * (h-(2*PLOT_OUTLINE_THICKNESS))) + (h / 2);
        DrawPixel(screen_x, screen_y, GetColor(GuiGetStyle(DEFAULT, LINE_COLOR)));
    }

}

WaveBuffer sine_buffer_left = {0};
WaveBuffer sine_buffer_right = {0};

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    float* pOutputF32 = (float*)pOutput;
    float temp_left[frameCount];
    float temp_right[frameCount];


    
    ma_waveform_read_pcm_frames(&waveform_left, temp_left, frameCount, NULL);
    ma_waveform_read_pcm_frames(&waveform_right, temp_right, frameCount, NULL);

    write_wave_buffer(&sine_buffer_left, frameCount, temp_left);
    write_wave_buffer(&sine_buffer_right, frameCount, temp_right);
    
    // Interleave left and right channels
    for (ma_uint32 i = 0; i < frameCount; i++) {
        pOutputF32[i * 2 + 0] = temp_left[i];   // Left channel
        pOutputF32[i * 2 + 1] = temp_right[i];  // Right channel
    }
}




int main()     
{   // Audio Setup
    //----------------------------------------------------------------------------------
    float base_freq = 100.0;
    float diff_freq = 7.83;
    
    float sine_freq_left = base_freq;
    float sine_freq_right = base_freq + diff_freq;


    ma_waveform_config wave_config_left = ma_waveform_config_init(
        ma_format_f32,
        1,
        SAMPLE_RATE,
        ma_waveform_type_sine,
        AMPLITUDE,
        sine_freq_left
    );
    

    ma_result result_left = ma_waveform_init(&wave_config_left, &waveform_left);
    if (result_left != MA_SUCCESS) {
        // Error.
    }

    ma_waveform_config wave_config_right = ma_waveform_config_init(
        ma_format_f32,
        1,
        SAMPLE_RATE,
        ma_waveform_type_sine,
        AMPLITUDE,
        sine_freq_right
    );

    ma_result result_right = ma_waveform_init(&wave_config_right, &waveform_right);
    if (result_right != MA_SUCCESS) {
        // Error.
    }


    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32;   // Set to ma_format_unknown to use the device's native format.
    config.playback.channels = TOTAL_CHANNELS;               // Set to 0 to use the device's native channel count.
    config.sampleRate        = SAMPLE_RATE;           // Set to 0 to use the device's native sample rate.
    config.dataCallback      = data_callback;   // This function will be called when miniaudio needs more data.

    ma_device device;
    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        return -1;  // Failed to initialize the device.
    }

    ma_device_start(&device);
    //----------------------------------------------------------------------------------
    InitWindow(WINDOW_W, WINDOW_H, "Janus - Binaural Beat Generator");
    GuiLoadStyleTerminal();
    SetTargetFPS(60);


    while (!WindowShouldClose())
    {
        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();
            ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
            if (GuiSlider(
                (Rectangle){ 100, 10, 165, 20 },
                "Base Frequency",
                TextFormat("%2.2f", base_freq),
                &base_freq, MIN_FREQ, MAX_FREQ
            )) {
                sine_freq_left = base_freq;
                sine_freq_right = base_freq - diff_freq;

                ma_waveform_set_frequency(&waveform_left, sine_freq_left);
                ma_waveform_set_frequency(&waveform_right, sine_freq_right);
            }

            if (GuiSlider(
                (Rectangle){ 100, 30, 165, 20 },
                "Diff Frequency",
                TextFormat("%2.2f", diff_freq),
                &diff_freq, -BI_FREQ_RANGE, BI_FREQ_RANGE
            )) {
                sine_freq_left = base_freq;
                sine_freq_right = base_freq - diff_freq;

                ma_waveform_set_frequency(&waveform_left, sine_freq_left);
                ma_waveform_set_frequency(&waveform_right, sine_freq_right);
            }

            if (GuiSlider(
                (Rectangle){ 100, 50, 165, 20 },
                "Left Frequency",
                TextFormat("%2.2f", sine_freq_left),
                &sine_freq_left, MIN_FREQ, MAX_FREQ
            )) {
                base_freq = sine_freq_left;
                diff_freq = sine_freq_right - sine_freq_left;

                ma_waveform_set_frequency(&waveform_left, sine_freq_left);
            }

            if (GuiSlider(
                (Rectangle){ 100, 70, 165, 20 },
                "Right Frequency",
                TextFormat("%2.2f", sine_freq_right),
                &sine_freq_right, MIN_FREQ, MAX_FREQ
            )) {
                base_freq = sine_freq_left;
                diff_freq = sine_freq_right - sine_freq_left;

                ma_waveform_set_frequency(&waveform_right, sine_freq_right);
            }

            float binaural_freq = fabs(diff_freq);
            float beat_period = 1.0 / binaural_freq;

            float time = GetTime();  
            float phase = fmod(time, beat_period) / beat_period;  
            float pulse = (sin(phase * 2 * PI) + 1) / 2.0;
            float pulse_normalized = (pulse + 1.0f) / 2.0f;

            float difference_buffer[MAX_SINE_BUFFER];
            for(int i = 0; i < MAX_SINE_BUFFER; i++) {
                difference_buffer[i] = (sine_buffer_left.sine_buffer[i] + sine_buffer_right.sine_buffer[i]) * 0.5;
            }

            GuiLabel((Rectangle){310, 0, WINDOW_W-310, 45}, TextFormat("CURRENTLY EXPERIENCING:\nBinaural Frequency: %.1f Hz", binaural_freq));
            DrawRectangle(310, 45, WINDOW_W-310, 45, Fade(WHITE, pulse_normalized));

        
            draw_wave(sine_buffer_left.sine_buffer, 0, 90, 200, 100);
            draw_wave(sine_buffer_right.sine_buffer, 0, 190, 200, 100);
            draw_wave(difference_buffer, 200, 90, WINDOW_W - 200, WINDOW_H - 90);
            
        EndDrawing();
    }

    CloseWindow();
    ma_device_uninit(&device);
    return 0;
}

    