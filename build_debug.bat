@echo off
gcc -Wall -g -std=c11 src/main.c src/fourier1.c src/realfft.c src/kmeans.c -o bin/Debug/SonicSpectra -Iinclude -Llib -lraylib -ltag_c -ltag -lopengl32 -lgdi32 -lwinmm