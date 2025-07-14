
/*
    AUTHOR: JAMES "MACKENZIE" REGALADO
    Copyright (c) 2024 [James "Mckenzie" Regalado]
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "raylib.h"
#include "realfft.h"
#include "tag_c.h"
#include "kmeans.h"

#define GLSL_VERSION 330

#define TWO_PI 6.28318530717959
#define N (1 << 12)
#define TARGET_FREQ_SIZE 10


#define SCREEN_HEIGHT 512
#define SCREEN_WIDTH 512
#define PROGRESS_BAR_HEIGHT 4
#define ALBUM_COVER_SIZE 200
#define FONTSIZE 15

/*
 * Author: Mckenzie J. Regalado
 * Philippines
 * 02/03/2024
 */

typedef struct
{
    /* data */
    char *title;
    char *artist;
    char *album;
    char *genre;
    unsigned int year;
} MusicInfo;

typedef struct
{
    /* data */
    float input_raw_Data[N];
    float input_data_with_windowing_function[N];
    float output_raw_Data[N];
    float amplitudes[N / 2];
    float smooth_spectrum[TARGET_FREQ_SIZE - 1];
} Data;

Data data;

/* Functions declaration. */
void InitializeMusicInfo(const char *music_file_path, MusicInfo *music_info, Texture2D *album_cover_texture);
void UninitializeMusicInfo(MusicInfo *music_info);
void CleanUp();
void PushSample(float sample);
void ProcessAudioStreamCallback(void *bufferData, unsigned int frames);
void ApplyHanningWindow();
void DoFFT();
float GetAmp(float a, float b);
void CalculateAmplitudes();
void ApplyParsevalTheorem(float rms_values[], float target_frequencies[], unsigned int sample_rate);
void RMS_TO_DBFS(float rms_values[], float full_scale, float dt, float smoothing_factor);
void VisualizeSpectrum();


// The default color theme
Color BG_COLOR = {12, 4, 4, 255};
Color TEXT_COLOR = {255, 255, 255, 255};
Color BOX_BORDER_COLOR = {255, 255, 255, 255};
Color SPECTRUM_COLOR = {255, 255, 255, 255};

const char *default_music_cover = "../../assets/img/default_cover.jpg"; // Default music album cover.

int main(void)
{
    // Initialization
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "SonicSpectra | © 2024 MacKenzie Regalado");
    InitAudioDevice(); // Initialize audio device driver.
    SetTargetFPS(60);  // Set target FPS (maximum)

    //--------------------------------------------------------------------------------------
    const char *icon_path = "../../assets/img/app-icon.png";
    Image app_icon = LoadImage(icon_path);
    SetWindowIcon(app_icon);
    UnloadImage(app_icon);

    //--------------------------------------------------------------------------------------
    Music music_stream;
    bool has_music_loaded = false;

    //--------------------------------------------------------------------------------------
    Texture2D album_cover_texture;
    int album_cover_texture_posX = 32.0, album_cover_texture_posY = (SCREEN_HEIGHT/2 - ALBUM_COVER_SIZE) / 2.0;
    MusicInfo music_info = {NULL, NULL, NULL, NULL, 0};

    //--------------------------------------------------------------------------------------
    float target_frequencies[TARGET_FREQ_SIZE] = {20.0f, 40.0f, 80.0f, 160.0f, 320.0f, 640.0f, 1280.0f, 2560.0f, 5120.0f, 10200.0f};
    float smoothing_factor = 90.0f;
    float full_scale;   // digital full scale

    //--------------------------------------------------------------------------------------
    float durations = 0.0f;
    float time_played = 0.0f;

    /* Shaders */ 
    // Load shader
    Shader shader = LoadShader(0, TextFormat("../../shaders/ripple_effect.fs", GLSL_VERSION));
    
    float time = 0.0f;
    int time_loc = GetShaderLocation(shader, "uTime");
    SetShaderValue(shader, time_loc, &time, SHADER_UNIFORM_FLOAT);

    float ripple_wave_height = 0.0f;
    int ripple_wave_height_loc = GetShaderLocation(shader, "uHeight");
    SetShaderValue(shader, ripple_wave_height_loc, &ripple_wave_height, SHADER_UNIFORM_FLOAT);

    int resolution_loc = GetShaderLocation(shader, "iResolution");
    SetShaderValue(shader, resolution_loc, (float[2]){(float)ALBUM_COVER_SIZE, (float)ALBUM_COVER_SIZE}, SHADER_UNIFORM_VEC2);

    int target_texture_pos_loc = GetShaderLocation(shader, "targetTexturePos");
    SetShaderValue(shader, target_texture_pos_loc, (float[2]) {(float)album_cover_texture_posX, (float)album_cover_texture_posY}, SHADER_UNIFORM_VEC2);

    int texture2dLoc = GetShaderLocation(shader, "targetTexture");
    
    //--------------------------------------------------------------------------------------
    // Load font
    Font pt_sans = LoadFont("../../assets/font/PTSans-Bold.ttf");
    SetTextureFilter(pt_sans.texture, 1);
    float font_spacing = 1.5f;

    // Instruction text
    const char *instruction_text = "Drag & Drop and .mp3 file";
    Vector2 instruction_text_measure = MeasureTextEx(pt_sans, instruction_text, FONTSIZE, font_spacing);

    // Music information texts
    Vector2 title_text_measure, artist_text_measure, album_text_measure, genre_text_measure, year_text_measure;

    //--------------------------------------------------------------------------------------
    // Animation
    Texture2D sonic_prog_bar_sprite = LoadTexture("../../assets/spritesheet/sonicdash.png");
    Texture2D flag_prog_bar_sprite = LoadTexture("../../assets/spritesheet/flag.png");

    Vector2 sonic_animation_pos = {32.0f, SCREEN_HEIGHT - sonic_prog_bar_sprite.height};
    Vector2 flag_animation_pos = {SCREEN_WIDTH - 60.0f, SCREEN_HEIGHT - flag_prog_bar_sprite.height - 5.0f};

    int sonic_animation_current_frame = 0, sonic_animation_frames_counter = 0, sonic_animation_frames_speed = 12;
    int flag_animation_current_frame = 0, flag_animation_frames_counter = 0, flag_animation_frames_speed = 8;

    Rectangle sonic_frame_rec;
    Rectangle flag_frame_rec;

    if (IsTextureReady(sonic_prog_bar_sprite)) {
        sonic_frame_rec = (Rectangle) { 0.0f, 0.0f, (float)sonic_prog_bar_sprite.width/4, (float)sonic_prog_bar_sprite.height };
    }

    if (IsTextureReady(flag_prog_bar_sprite)) {
        flag_frame_rec = (Rectangle) { 0.0f, 0.0f, (float)flag_prog_bar_sprite.width/5, (float)flag_prog_bar_sprite.height };
    }
    

    // Main game loop
    while (!WindowShouldClose())  // Detect window close button or ESC key
    {

        float dt = GetFrameTime(); // Get time in seconds for last frame drawn (delta time)

        /** Update */
        //----------------------------------------------------------------------------------
        if (IsMusicReady(music_stream))
        {
            UpdateMusicStream(music_stream); // Update music buffer with new stream data
        }

        /** Handle drag & drop file. */
        //----------------------------------------------------------------------------------
        if (IsFileDropped())
        {
            FilePathList droppedFiles = LoadDroppedFiles(); // Load dropped filepaths
            if (droppedFiles.count > 0)
            {
                const char *file_path = droppedFiles.paths[0]; // We are only interested in a single file.

                if (IsPathFile(file_path) && IsFileExtension(file_path, ".mp3"))
                { // Check if the file is a valid mp3 file.
                    /**
                     * Before loading a new music file, make sure to unload any existing ones.
                     */
                    if (has_music_loaded)
                    {
                        UninitializeMusicInfo(&music_info);                                          // Deallocate memory.
                        StopMusicStream(music_stream);                                               // Stop music playing
                        DetachAudioStreamProcessor(music_stream.stream, ProcessAudioStreamCallback); // Disconnect audio stream processor
                        UnloadMusicStream(music_stream);                                             // Unload music stream buffers from RAM
                        UnloadTexture(album_cover_texture);                                          // Texture unloading
                    }
                    //----------------------------------------------------------------------------------
                    music_stream = LoadMusicStream(file_path); // Load music stream from file
                    // ----------------------------------------------------------------------------------
                    if (IsMusicReady(music_stream))   // Checks if the music stream is ready
                    {
                        // ----------------------------------------------------------------------------------
                        has_music_loaded = true;
                        sonic_animation_current_frame = 0;
                        durations = GetMusicTimeLength(music_stream);
                        // ----------------------------------------------------------------------------------
                        InitializeMusicInfo(file_path, &music_info, &album_cover_texture);
                        // ----------------------------------------------------------------------------------
                        full_scale = 20.0f * log10f(powf(2.0f, (float) music_stream.stream.sampleSize) * sqrtf(3.0f / 2.0f));
                        // ----------------------------------------------------------------------------------
                        CleanUp();
                        // ----------------------------------------------------------------------------------
                        PlayMusicStream(music_stream);                                               // Start music playing
                        AttachAudioStreamProcessor(music_stream.stream, ProcessAudioStreamCallback); // Attach audio stream processor to stream, receives the samples as <float>s
                        // ----------------------------------------------------------------------------------
                        title_text_measure = MeasureTextEx(pt_sans, TextFormat("Title: %s", music_info.title), FONTSIZE, font_spacing);
                        artist_text_measure = MeasureTextEx(pt_sans, TextFormat("Artist: %s", music_info.artist), FONTSIZE, font_spacing);
                        album_text_measure = MeasureTextEx(pt_sans, TextFormat("Album: %s", music_info.album), FONTSIZE, font_spacing);
                        genre_text_measure = MeasureTextEx(pt_sans, TextFormat("Genre: %s", music_info.genre), FONTSIZE, font_spacing);
                        year_text_measure = MeasureTextEx(pt_sans, TextFormat("Year: %d", music_info.year), FONTSIZE, font_spacing);
                        // ----------------------------------------------------------------------------------
                    }
                }
            }

            UnloadDroppedFiles(droppedFiles); // Unload dropped filepaths
        }

        //----------------------------------------------------------------------------------
        if (IsMusicReady(music_stream))
        {
            /** Apply the hanning window. */
            ApplyHanningWindow();

            /** Do FFT */
            DoFFT();

            /** Calculate amplitudes. */
            CalculateAmplitudes();

            //----------------------------------------------------------------------------------
            float rms_values[TARGET_FREQ_SIZE - 1] = {0.0};
            //----------------------------------------------------------------------------------
            ApplyParsevalTheorem(rms_values, target_frequencies, music_stream.stream.sampleRate);
            //----------------------------------------------------------------------------------
            RMS_TO_DBFS(rms_values, full_scale, dt, smoothing_factor);
            //----------------------------------------------------------------------------------
            if (durations > 0.0f)
            {
                time_played = GetMusicTimePlayed(music_stream) / durations * (SCREEN_WIDTH - 64);
                sonic_animation_pos.x = time_played;
            }
            //----------------------------------------------------------------------------------
        }


        //----------------------------------------------------------------------------------
        if (has_music_loaded) {
            // Update animation frame
            sonic_animation_frames_counter++;
            if (sonic_animation_frames_counter >= (60/sonic_animation_frames_speed))
            {
                sonic_animation_frames_counter = 0;
                sonic_animation_current_frame++;
                if (sonic_animation_current_frame > 3) sonic_animation_current_frame = 0;

                sonic_frame_rec.x = (float)sonic_animation_current_frame*(float)sonic_prog_bar_sprite.width/4;
            }

            flag_animation_frames_counter++;
            if (flag_animation_frames_counter >= (60/flag_animation_frames_speed))
            {
                flag_animation_frames_counter = 0;
                flag_animation_current_frame++;
                if (flag_animation_current_frame > 4) flag_animation_current_frame = 0;

                flag_frame_rec.x = (float)flag_animation_current_frame*(float)flag_prog_bar_sprite.width/5;
            }
        }


        //----------------------------------------------------------------------------------
        /** Draw */
        //----------------------------------------------------------------------------------
        BeginDrawing();
        //----------------------------------------------------------------------------------
        ClearBackground(BG_COLOR);
        //----------------------------------------------------------------------------------
        if (!has_music_loaded) {
            DrawTextEx(pt_sans, instruction_text, (Vector2) {(SCREEN_WIDTH - instruction_text_measure.x) / 2, (SCREEN_HEIGHT - instruction_text_measure.y) / 2}, FONTSIZE, font_spacing, TEXT_COLOR);
        }
        //----------------------------------------------------------------------------------
        if (has_music_loaded)
        {
            time = (float)GetTime();   // Get elapsed time in seconds since InitWindow()
            SetShaderValue(shader, time_loc, &time, SHADER_UNIFORM_FLOAT);
            float mid_bass = data.smooth_spectrum[2];
            ripple_wave_height = mid_bass / 1000.0f;
            SetShaderValue(shader, ripple_wave_height_loc, &ripple_wave_height, SHADER_UNIFORM_FLOAT);

            //----------------------------------------------------------------------------------
            BeginShaderMode(shader);
                SetShaderValueTexture(shader, texture2dLoc, album_cover_texture); // Set shader uniform value for texture (sampler2d)
                DrawTexture(album_cover_texture, album_cover_texture_posX, album_cover_texture_posY, WHITE);
            EndShaderMode();

            //----------------------------------------------------------------------------------
            DrawRectangleLines(32, SCREEN_HEIGHT/2, SCREEN_WIDTH - 64, 180, BOX_BORDER_COLOR);

            //----------------------------------------------------------------------------------
            int total_text_height = title_text_measure.y + artist_text_measure.y + album_text_measure.y + genre_text_measure.y + year_text_measure.y;
            int max_text_width = fmaxf(title_text_measure.x, artist_text_measure.x);
                max_text_width = fmaxf(max_text_width, album_text_measure.x);
                max_text_width = fmaxf(max_text_width, genre_text_measure.x);
                max_text_width = fmaxf(max_text_width, year_text_measure.x);

            //----------------------------------------------------------------------------------
            int offSetY = (180 - total_text_height) / 2;
            int offSetX = ((SCREEN_WIDTH - 64) - max_text_width) / 2;

            //----------------------------------------------------------------------------------
            DrawTextEx(pt_sans, TextFormat("Title: %s", music_info.title), (Vector2) {32 + offSetX, SCREEN_HEIGHT/2 + offSetY}, FONTSIZE, font_spacing, TEXT_COLOR);
            DrawTextEx(pt_sans, TextFormat("Artist: %s", music_info.artist), (Vector2) {32 + offSetX, (SCREEN_HEIGHT/2 + FONTSIZE) + offSetY}, FONTSIZE, font_spacing, TEXT_COLOR);
            DrawTextEx(pt_sans, TextFormat("Album: %s", music_info.album), (Vector2) {32 + offSetX, (SCREEN_HEIGHT/2 + (2 * FONTSIZE)) + offSetY}, FONTSIZE, font_spacing, TEXT_COLOR);
            DrawTextEx(pt_sans, TextFormat("Genre: %s", music_info.genre), (Vector2) {32 + offSetX, (SCREEN_HEIGHT/2 + (3 * FONTSIZE)) + offSetY}, FONTSIZE, font_spacing, TEXT_COLOR);
            DrawTextEx(pt_sans, TextFormat("Year: %d", music_info.year), (Vector2) {32 + offSetX, (SCREEN_HEIGHT/2 + (4 * FONTSIZE)) + offSetY}, FONTSIZE, font_spacing, TEXT_COLOR);
        }
        //----------------------------------------------------------------------------------
        if (IsMusicReady(music_stream))
        {
            //----------------------------------------------------------------------------------
            VisualizeSpectrum();
            //----------------------------------------------------------------------------------
            DrawTextureRec(sonic_prog_bar_sprite, sonic_frame_rec, sonic_animation_pos, WHITE);  // Draw part of the texture
            DrawTextureRec(flag_prog_bar_sprite, flag_frame_rec, flag_animation_pos, WHITE);  // Draw part of the texture
        }
        //----------------------------------------------------------------------------------
        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    /** De-Initialization */
    //----------------------------------------------------------------------------------
    if (has_music_loaded)
    {
        DetachAudioStreamProcessor(music_stream.stream, ProcessAudioStreamCallback); // Disconnect audio stream processor
        UnloadMusicStream(music_stream);                                             // Unload music stream buffers from RAM
        UnloadTexture(album_cover_texture);                                         // Texture unloading
    }
    //----------------------------------------------------------------------------------
    UnloadShader(shader);
    //----------------------------------------------------------------------------------
    UnloadFont(pt_sans);
    //----------------------------------------------------------------------------------
    UnloadTexture(sonic_prog_bar_sprite); 
    //----------------------------------------------------------------------------------
    UnloadTexture(flag_prog_bar_sprite);
    //----------------------------------------------------------------------------------
    CloseAudioDevice(); // Close audio device (music streaming is automatically stopped)
    //----------------------------------------------------------------------------------
    CloseWindow(); // Close window and OpenGL context
    //----------------------------------------------------------------------------------

    return EXIT_SUCCESS;
}

void InitializeMusicInfo(const char *music_file_path, MusicInfo *music_info, Texture2D *album_cover_texture)
{
    taglib_set_strings_unicode(1);
    TagLib_File *file;
    TagLib_Tag *tag;

    file = taglib_file_new(music_file_path);

    if (file == NULL)
    {
        printf("Unable to create file!\n");
        return;
    }

    tag = taglib_file_tag(file);
    if (tag != NULL)
    {
        char *title  = taglib_tag_title(tag);
        char *artist = taglib_tag_artist(tag);
        char *album  = taglib_tag_album(tag);
        char *genre  = taglib_tag_genre(tag);

        /* Set the music file's basic information to the music_info struct. */
        music_info->title  = calloc(strlen(title)  + 1, sizeof(char));
        music_info->artist = calloc(strlen(artist) + 1, sizeof(char));
        music_info->album  = calloc(strlen(album)  + 1, sizeof(char));
        music_info->genre  = calloc(strlen(genre)  + 1, sizeof(char));
        music_info->year   = taglib_tag_year(tag);

        if (music_info->title != NULL)
        {
            strcpy(music_info->title, title);
        }
        if (music_info->artist != NULL)
        {
            strcpy(music_info->artist, artist);
        }
        if (music_info->album != NULL)
        {
            strcpy(music_info->album, album);
        }
        if (music_info->genre != NULL)
        {
            strcpy(music_info->genre, genre);
        }
    }

    /* Extract album picture */
    TagLib_Complex_Property_Attribute ***properties = taglib_complex_property_get(file, "PICTURE");
    TagLib_Complex_Property_Picture_Data picture;
    taglib_picture_from_complex_property(properties, &picture);

    /* Set album cover photo */
    Image image = LoadImageFromMemory(".jpg", (unsigned char *)picture.data, picture.size); // Load image from memory buffer, fileType refers to extension: i.e. '.png'

    if (!IsImageReady(image))
    {
        image = LoadImage(default_music_cover);
    }

    ImageResize(&image, ALBUM_COVER_SIZE, ALBUM_COVER_SIZE); // Resize image (Bicubic scaling algorithm)

    *album_cover_texture = LoadTextureFromImage(image); // Image converted to texture, GPU memory (VRAM)

    int n_points = ALBUM_COVER_SIZE * ALBUM_COVER_SIZE;

    Color *color_data = malloc(n_points * sizeof(Color));

    if (color_data != NULL) 
    {
        // Fill with colors
        for (int i = 0; i < n_points; i++)
        {            
            int row = (int) i / ALBUM_COVER_SIZE;
            int col = (int) i % ALBUM_COVER_SIZE;
            color_data[i] = GetImageColor(image, row, col);
        }

        /*
         * Extract Color pallete of size 4 in an image.
         * Color Quantization Using k-Means Clustering Algorithm.
         */
        int color_pallete_size = 4;
        Color *color_pallete = malloc(color_pallete_size * sizeof(Color));        // Note: Size = 4 is fixed.
        getDominantColors(n_points, color_data, color_pallete, color_pallete_size); // From kmeans.h

        BG_COLOR = color_pallete[0];
        TEXT_COLOR = color_pallete[2];
        SPECTRUM_COLOR = color_pallete[1];
        BOX_BORDER_COLOR = color_pallete[3];

        free(color_pallete);
        color_pallete = NULL;
    }

    free(color_data);
    UnloadImage(image);

    // free
    taglib_complex_property_free(properties);
    taglib_tag_free_strings();
    taglib_file_free(file);
}

void UninitializeMusicInfo(MusicInfo *music_info)
{
    free(music_info->title);
    music_info->title = NULL;
    free(music_info->artist);
    music_info->artist = NULL;
    free(music_info->album);
    music_info->album = NULL;
    free(music_info->genre);
    music_info->genre = NULL;
    music_info->year = 0;
}

void CleanUp()
{
    memset(data.input_raw_Data, 0, N * sizeof(float));
    memset(data.input_data_with_windowing_function, 0, N * sizeof(float));
    memset(data.output_raw_Data, 0, N * sizeof(float));
    memset(data.amplitudes, 0, (N / 2) * sizeof(float));
    memset(data.smooth_spectrum, 0, (TARGET_FREQ_SIZE - 1) * sizeof(float));
}

void PushSample(float sample)
{
    memmove(data.input_raw_Data, data.input_raw_Data + 1, (N - 1) * sizeof(data.input_raw_Data[0])); // Moving history to the left
    data.input_raw_Data[N - 1] = sample;                                                             // Adding last average value
}

void ProcessAudioStreamCallback(void *bufferData, unsigned int frames)
{
    /**
     * https://cdecl.org/?q=float+%28*fs%29%5B2%5D
     * Samples internally stored as <float>s
     */
    float(*samples)[2] = bufferData;

    for (size_t frame = 0; frame < frames; frame++)
    {
        /* code */
        PushSample(samples[frame][0]); // We only interested in the left audio channel.
    }
    return;
}

void ApplyHanningWindow()
{
    /*
     * https://community.sw.siemens.com/s/article/window-correction-factors
     * https://www.vibrationdata.com/tutorials_alt/Hanning_compensation.pdf
     */
    float energy_correction_factor = sqrtf(8.0f / 3.0f);
    for (size_t n = 0; n < N; n++)
    {
        /* code */
        float t = (float)n / (N - 1);
        float h = (0.5 * (1.0 - cosf(TWO_PI * t)));
        data.input_data_with_windowing_function[n] = data.input_raw_Data[n] * h * energy_correction_factor;
    }
}

void DoFFT()
{
    memcpy(data.output_raw_Data, data.input_data_with_windowing_function, N * sizeof(float));
    realft(data.output_raw_Data, N, 1);
}

float GetAmp(float a, float b)
{
    return sqrtf(a*a + b*b);
}

void CalculateAmplitudes()
{
    for (size_t c = 0; c < (N / 2); c++)
    {
        /* code */
        float amp = GetAmp(data.output_raw_Data[2 * c], data.output_raw_Data[2 * c + 1]); // even indexes are real values and odd indexes are complex value.
        data.amplitudes[c] = 2.0f * amp;  // 2.0 is the amplitude correction factor for hanning window.
    }
}

void ApplyParsevalTheorem(float rms_values[], float target_frequencies[], unsigned int sample_rate)
{

    /******************************************************************************************************************
     * Get the amplitude of the specific set of bins using Perseval's theorem.                                        *
     * https://www.ni.com/docs/en-US/bundle/labwindows-cvi/page/advancedanalysisconcepts/lvac_parseval_s_theorem.html *
     * https://www.gaussianwaves.com/2015/07/significance-of-rms-root-mean-square-value/                              *
     * https://stackoverflow.com/questions/15455698/extract-treble-and-bass-from-audio-in-ios                         *
     *                                                                                                                *
     ******************************************************************************************************************/
    int n_elem[TARGET_FREQ_SIZE - 1] = {0};
    float spectrum[TARGET_FREQ_SIZE - 1] = {0.0};

    for (size_t bin_index = 0; bin_index < (N / 2); bin_index++)
    {
        /* code */
        float freq = (bin_index + 1) * (sample_rate / N);
        for (size_t j = 0; j < TARGET_FREQ_SIZE - 1; j++)
        {
            /* code */
            if (freq >= target_frequencies[j] && freq < target_frequencies[j + 1])
            {
                float amp = data.amplitudes[bin_index];
                spectrum[j] += amp * amp;
                n_elem[j] += 1;
            }
        }
    }

    // Get rms values
    for (int i = 0; i < TARGET_FREQ_SIZE - 1; i++)
    {
        float mean_square = spectrum[i] / n_elem[i];
        float root_mean_square = sqrtf(mean_square); // Calculate the RMS value.
        rms_values[i] = root_mean_square;
    }
}

void RMS_TO_DBFS(float rms_values[], float full_scale, float dt, float smoothing_factor)
{
    /* https://stackoverflow.com/questions/20408388/how-to-filter-fft-data-for-audio-visualisation * 
     * https://dsp.stackexchange.com/questions/8785/how-to-compute-dbfs                            */

    for (size_t i = 0; i < TARGET_FREQ_SIZE - 1; i++)
    {
        float root_mean_square = rms_values[i];
        float dBFS = 0.0f;
        if (root_mean_square > 1e-5)
        {
            dBFS = 20.0f * log10f(root_mean_square / full_scale); // Convert RMS value to Decibel Full Scale (dBFS) value.
        }
        data.smooth_spectrum[i] += (dBFS - data.smooth_spectrum[i]) * dt * smoothing_factor; // Smoothing the spectrum output value.
    }
}

void VisualizeSpectrum()
{
    int h = FONTSIZE + 4 + ALBUM_COVER_SIZE;
    for (size_t i = 0; i < TARGET_FREQ_SIZE - 1; i++)
    {
        float smooth_spectrum_i = data.smooth_spectrum[i];
        if (smooth_spectrum_i < 0.0f) {
            smooth_spectrum_i *= -1.0f;
            DrawRectangleLines((32 + ALBUM_COVER_SIZE + 52) + (i * 16), h/2, 15, smooth_spectrum_i, SPECTRUM_COLOR);
        } else {
            DrawRectangle((32 + ALBUM_COVER_SIZE + 52) + (i * 16), (h/2) - 3.0f * smooth_spectrum_i, 15, (3.0f * smooth_spectrum_i), SPECTRUM_COLOR);
        }
    }
}
