#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "../include/raylib.h"
#include "../include/realfft.h"
#include "../include/tag_c.h"
#include "../include/kmeans.h"


#define SCREEN_HEIGHT 512
#define SCREEN_WIDTH 512
#define PROGRESS_BAR_HEIGHT 10
#define ALBUM_COVER_SIZE 200

#define TWO_PI 6.28318530717959
#define N (1 << 12)
#define TARGET_FREQ_SIZE 10

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
    float frequency_sqr_mag[N / 2];
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
float GetSqrMag(float a, float b);
void CalculateAmplitudes();
void ApplyParsevalTheorem(float rms_values[], float target_frequencies[], unsigned int fs);
void RMS_TO_DBFS(float rms_values[], float dt, float smoothing_factor);
void VisualizeSpectrum(float spectrum_scaling_factor);
void DisplayProgressBar(int time_played);

Color BG_COLOR = (Color){
    12, 4, 4, 255};
Color TEXT_COLOR = (Color){
    255, 255, 255, 255};
Color PROGRESS_BAR_COLOR = (Color){
    255, 255, 255, 255};
Color SPECTRUM_COLOR = (Color){
    255, 255, 255, 255};

const char *default_music_cover = "default_cover.jpg"; // Default music album cover.

int main(void)
{
    // Initialization
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "SonicSpectra");
    InitAudioDevice(); // Initialize audio device driver.

    //--------------------------------------------------------------------------------------
    const char *icon_path = "app-icon.png";
    Image app_icon = LoadImage(icon_path);
    SetWindowIcon(app_icon);
    UnloadImage(app_icon);

    //--------------------------------------------------------------------------------------
    /* Set font size */
    int font_size = 10;

    //--------------------------------------------------------------------------------------
    Music music_stream;
    bool has_music_loaded = false;

    //--------------------------------------------------------------------------------------
    Texture2D album_cover_texture;
    MusicInfo music_info = {NULL, NULL, NULL, NULL, 0};

    //--------------------------------------------------------------------------------------
    float target_frequencies[TARGET_FREQ_SIZE] = {20.0f, 40.0f, 80.0f, 160.0f, 300.0f, 600.0f, 1200.0f, 5000.0f, 10000.0f, 22050.0f};
    float smoothing_factor = 50.0f;
    float spectrum_scaling_factor = 2.0f;

    //--------------------------------------------------------------------------------------
    float durations = 0.0f;
    float time_played = 0.0f;
    unsigned int fs = 0;

    SetTargetFPS(60); // Set target FPS (maximum)

    // Main game loop
    while (!WindowShouldClose())
    { // Detect window close button or ESC key
        /** Get deltaTime in seconds. */
        float dt = GetFrameTime();

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
                    if (IsMusicReady(music_stream))
                    { // Checks if the music stream is ready
                        // ----------------------------------------------------------------------------------
                        has_music_loaded = true;
                        durations = GetMusicTimeLength(music_stream);
                        fs = music_stream.stream.sampleRate;
                        // ----------------------------------------------------------------------------------
                        InitializeMusicInfo(file_path, &music_info, &album_cover_texture);
                        // ----------------------------------------------------------------------------------
                        CleanUp();
                        // ----------------------------------------------------------------------------------
                        PlayMusicStream(music_stream);                                               // Start music playing
                        AttachAudioStreamProcessor(music_stream.stream, ProcessAudioStreamCallback); // Attach audio stream processor to stream, receives the samples as <float>s
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
            ApplyParsevalTheorem(rms_values, target_frequencies, fs);
            //----------------------------------------------------------------------------------
            RMS_TO_DBFS(rms_values, dt, smoothing_factor);
            //----------------------------------------------------------------------------------
            if (durations > 0.0f)
            {
                time_played = GetMusicTimePlayed(music_stream) / durations * (SCREEN_WIDTH - 64);
            }
            //----------------------------------------------------------------------------------
        }

        //----------------------------------------------------------------------------------
        /** Draw */
        //----------------------------------------------------------------------------------
        BeginDrawing();
        //----------------------------------------------------------------------------------
        ClearBackground(BG_COLOR);
        //----------------------------------------------------------------------------------
        DrawText("Drag & Drop an mp3 file", 2, 2, font_size, TEXT_COLOR);
        //----------------------------------------------------------------------------------
        if (has_music_loaded)
        {
            DrawTexture(album_cover_texture, 2, font_size + 4, WHITE);
            //----------------------------------------------------------------------------------
            DrawText(TextFormat("Title: %s", music_info.title), ALBUM_COVER_SIZE + 10, font_size + 4, 10, TEXT_COLOR);
            //----------------------------------------------------------------------------------
            DrawText(TextFormat("Artist: %s", music_info.artist), ALBUM_COVER_SIZE + 10, (2 * font_size) + 8, 10, TEXT_COLOR);
            //----------------------------------------------------------------------------------
            DrawText(TextFormat("Album: %s", music_info.album), ALBUM_COVER_SIZE + 10, (3 * font_size) + 12, 10, TEXT_COLOR);
            //----------------------------------------------------------------------------------
            DrawText(TextFormat("Genre: %s", music_info.genre), ALBUM_COVER_SIZE + 10, (4 * font_size) + 16, 10, TEXT_COLOR);
            //----------------------------------------------------------------------------------
            DrawText(TextFormat("Year: %d", music_info.year), ALBUM_COVER_SIZE + 10, (5 * font_size) + 20, 10, TEXT_COLOR);
        }
        //----------------------------------------------------------------------------------
        if (IsMusicReady(music_stream))
        {
            //----------------------------------------------------------------------------------
            VisualizeSpectrum(spectrum_scaling_factor);
            //----------------------------------------------------------------------------------
            DisplayProgressBar((int)time_played);
            //----------------------------------------------------------------------------------
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
        UnloadTexture(album_cover_texture);                                          // Texture unloading
    }
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
        char *title = taglib_tag_title(tag);
        char *artist = taglib_tag_artist(tag);
        char *album = taglib_tag_album(tag);
        char *genre = taglib_tag_genre(tag);

        /* Set the music file's basic information to the music_info struct. */
        music_info->title = (char *)calloc(strlen(title) + 1, sizeof(char));
        music_info->artist = (char *)calloc(strlen(artist) + 1, sizeof(char));
        music_info->album = (char *)calloc(strlen(album) + 1, sizeof(char));
        music_info->genre = (char *)calloc(strlen(genre) + 1, sizeof(char));
        music_info->year = taglib_tag_year(tag);

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

    Color *color_data = (Color *)malloc(n_points * sizeof(Color));

    if (color_data != NULL) 
    {
        // Fill with colors
        for (int i = 0; i < n_points; i++)
        {
            int row = (int)i / ALBUM_COVER_SIZE;
            int col = (int)i % ALBUM_COVER_SIZE;
            color_data[i] = GetImageColor(image, row, col);
        }

        /*
         * Get top 4 most dominant colors in an image randomly.
         * Color Quantization Using k-Means Clustering Algorithm.
         */

        Color dominant_colors[4];                                                            // Note: Size = 4 is fixed.
        getDominantColors(n_points, color_data, dominant_colors); // From kmeans.h

        BG_COLOR = dominant_colors[0];
        TEXT_COLOR = dominant_colors[1];
        SPECTRUM_COLOR = dominant_colors[2];
        PROGRESS_BAR_COLOR = dominant_colors[3];
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
    memset(data.frequency_sqr_mag, 0, (N / 2) * sizeof(float));
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

float GetSqrMag(float a, float b)
{
    return a * a + b * b;
}

void CalculateAmplitudes()
{
    for (size_t c = 0; c < (N / 2); c++)
    {
        /* code */
        float sqr_mag = GetSqrMag(data.output_raw_Data[2 * c], data.output_raw_Data[2 * c + 1]); // even indexes are real values and odd indexes are complex value.
        data.frequency_sqr_mag[c] = sqr_mag;
    }
}

void ApplyParsevalTheorem(float rms_values[], float target_frequencies[], unsigned int fs)
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
        float freq = (bin_index + 1) * (fs / N);
        for (size_t j = 0; j < TARGET_FREQ_SIZE - 1; j++)
        {
            /* code */
            if (freq >= target_frequencies[j] && freq < target_frequencies[j + 1])
            {
                spectrum[j] = spectrum[j] + data.frequency_sqr_mag[bin_index];
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

void RMS_TO_DBFS(float rms_values[], float dt, float smoothing_factor)
{
    /* https://stackoverflow.com/questions/20408388/how-to-filter-fft-data-for-audio-visualisation * 
     * https://dsp.stackexchange.com/questions/8785/how-to-compute-dbfs                            */
    for (size_t i = 0; i < TARGET_FREQ_SIZE - 1; i++)
    {
        /* code */
        float root_mean_square = rms_values[i];
        float dBFS = 0.0;
        if (root_mean_square > 0.0)
        {
            dBFS = 20.0f * log10f(root_mean_square); // Convert RMS value to Decibel Full Scale (dBFS) value.
            if (dBFS < 0)
            {
                dBFS = 0.0;
            }
        }
        data.smooth_spectrum[i] += (dBFS - data.smooth_spectrum[i]) * dt * smoothing_factor; // Smoothing the spectrum output value.
    }
}

void VisualizeSpectrum(float spectrum_scaling_factor)
{
    int h = SCREEN_HEIGHT - 32;
    for (size_t i = 0; i < TARGET_FREQ_SIZE - 1; i++)
    {
        /* code */
        DrawRectangleLines(184 + (i * 16), h - (spectrum_scaling_factor * data.smooth_spectrum[i]), 15, (spectrum_scaling_factor * data.smooth_spectrum[i]), SPECTRUM_COLOR);
    }
}

void DisplayProgressBar(int time_played)
{
    DrawRectangle(32, SCREEN_HEIGHT - (PROGRESS_BAR_HEIGHT + 6), (int)time_played, PROGRESS_BAR_HEIGHT, PROGRESS_BAR_COLOR);
}
