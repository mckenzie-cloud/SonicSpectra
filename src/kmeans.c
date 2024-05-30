#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#include "raylib.h"
#include "kmeans.h"

#define K_CLUSTERS 4
#define MAX_ITERATION 50


/*
 * Here's my naive implementation of the K-Means Clustering
 * Algorithm for identifying the most dominant colors in an image.
 * -----------------------------------------------------------
 * Author: Regalado, Mckenzie
 * Philippines
 * 04/01/2024
 * -----------------------------------------------------------
 ****** FOR MOR INFO ABOUT THE K-MEANS CLUSTERING ALGORITHM ******
 * -----------------------------------------------------------
 * en.wikipedia.org/wiki/K-means_clustering
 * hackernoon.com/learn-k-means-clustering-by-quantizing-color-images-in-python
 * neptune.ai/blog/k-means-clustering
 * */

void getDominantColors(int n_points, Color *image_color_data, Color *dominant_colors)
{
    int centroids[K_CLUSTERS][3];

    srand(time(NULL));

    InitializeCentroid(centroids, n_points, image_color_data);

    int prev_c1_counter = 0;                // Stopping criterion.

    int cluster_size[K_CLUSTERS];

    memset(cluster_size, 0, K_CLUSTERS*sizeof(int));

    for (int iter = 0; iter < MAX_ITERATION; iter++) {
        // Initialize cluster lists.
        int **clusters = (int**) malloc(K_CLUSTERS * sizeof(int*));

        for (int i=0; i<K_CLUSTERS; i++) {
            clusters[i] = (int*) malloc(n_points * sizeof(int));
        }

        int c_counter[K_CLUSTERS];
        memset(c_counter, 0, K_CLUSTERS*sizeof(int));

        for (int d = 0; d < n_points; d++) {
            /*
             * 441 is the maximum distance of two colors.
             * i.e. the distance from white (255, 255, 255) to black (0,0,0) using the Euclidean distance.
            */
            int cluster_id = 0;

            int min_distance = INT_MAX;

            for (int i = 0; i < K_CLUSTERS; i++) {
                // calculate the distance from point-d to each cluster k.
                Color color = (Color) {
                    centroids[i][0], centroids[i][1], centroids[i][2]
                };
                float dist = distanceSquared(&color, &image_color_data[d]);
                if (dist < min_distance) {
                    min_distance = dist;
                    cluster_id = i;
                }
            }

            for (int j = 0; j < K_CLUSTERS; j++) {
                if (cluster_id == j) {
                    clusters[j][c_counter[j]++] = d;
                }
            }
        }

        if (prev_c1_counter == c_counter[0]) {          // If the size of the 1st-cluster is not changing, stop the loop.
            break;
        }
        prev_c1_counter = c_counter[0];

        for (int i = 0; i < K_CLUSTERS; i++) {
            cluster_size[i] = c_counter[i];
            getNewCentroid(clusters[i], centroids[i], c_counter[i], image_color_data);
        }

        for (int j=0; j<K_CLUSTERS; j++) {
            free(clusters[j]);
        }

        free(clusters);
    }

    for (int i=0; i<K_CLUSTERS; i++) {
        Color color = (Color) {
            centroids[i][0], centroids[i][1], centroids[i][2], 255
        };
        dominant_colors[i] = color;
    }
}


int gen_random_number(int lower_bound, int upper_bound)
{
    return rand() % (upper_bound - lower_bound + 1) + lower_bound;
}

/* This function returns the squared euclidean distance
	between the two data points.
*/

int distanceSquared(Color *color1, Color *color2)
{
    // Using euclidean distance
    int r = color2->r - color1->r;
    int g = color2->g - color1->g;
    int b = color2->b - color1->b;

    int d = r*r + g*g + b*b;
    return d;
}


// K-MEANS++
void InitializeCentroid(int centroid[][3], int n_points, Color *image_color_data)
{

    /* Initialize 1st centroid randomly */
    int c1_random_index = GetRandomValue(0, n_points-1);    // Get a random value between min and max (both included)
    Color cluster_color[K_CLUSTERS];
    cluster_color[0] = image_color_data[c1_random_index];

    // Initialize a centroid value for the 1st cluster
    for (int i = 1; i < K_CLUSTERS; i++) {

        int max_distance = 0;
        int c_id = 0;

        for (int p = 0; p < n_points; p++) {
            Color p_color = image_color_data[p];
            int min_d = INT_MAX;
            for (int c = 0; c < i; c++) {
                int temp_dist = distanceSquared(&cluster_color[c], &p_color);
                min_d = (min_d < temp_dist) ? min_d : temp_dist;
            }

            // Set the maximum distance and assign it to the next centroid.
            if (min_d > max_distance) {
                max_distance = min_d;
                c_id = p;
            }
        }
        cluster_color[i] = image_color_data[c_id];
    }

    for (int c = 0; c < K_CLUSTERS; c++) {
        centroid[c][0] = cluster_color[c].r;
        centroid[c][1] = cluster_color[c].g;
        centroid[c][2] = cluster_color[c].b;
    }
}

void getNewCentroid(int *cluster_list, int *centroid, int n_points, Color *image_color_data)
{
    if (cluster_list == NULL || n_points == 0) {
        return;
    }

    int r = 0, g = 0, b = 0;

    for (int i=0; i<n_points; i++) {
        int color_index = cluster_list[i];
        r += image_color_data[color_index].r;
        g += image_color_data[color_index].g;
        b += image_color_data[color_index].b;
    }

    r = (int) r / n_points;
    g = (int) g / n_points;
    b = (int) b / n_points;
    centroid[0] = r;
    centroid[1] = g;
    centroid[2] = b;
}

