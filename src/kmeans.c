#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include <time.h>
#include <math.h>

#include "raylib.h"
#include "kmeans.h"

#define MAX_ITERATION 50


/*
 * Here's my implementation of the naive K-Means Clustering
 * Algorithm method (Lloyd's algorithm) for identifying the most dominant colors in an image.
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
*/

void getDominantColors(int n_points, Color *image_color_data, Color *dominant_colors, int n_cluster)
{
    // Initialize cluster lists.
    int **cluster_lists = malloc(n_cluster * sizeof(int*));
    int **centroids     = malloc(n_cluster * sizeof(int*));

    int *cluster_size      = malloc(n_cluster * sizeof(int));
    int *prev_cluster_size = malloc(n_cluster * sizeof(int));

    for (int cluster = 0; cluster < n_cluster; cluster++) {
        centroids[cluster]     = malloc(3 * sizeof(int));
        cluster_lists[cluster] = malloc(n_points * sizeof(int));
    }

    InitializeCentroid(centroids, n_points, image_color_data, n_cluster);

    int iterate = 1;                // Stopping criterion.
    int iteration = 0;

    while (iterate && iteration < MAX_ITERATION) {
        memset(cluster_size, 0, n_cluster * sizeof(int));

        for (int cluster = 0; cluster < n_cluster; cluster++) {
            memset(cluster_lists[cluster], 0, n_points * sizeof(int));
        }

        for (int point = 0; point < n_points; point++) {
            int cluster_id = 0;

            int min_distance = INT_MAX;

            for (int cluster = 0; cluster < n_cluster; cluster++) {

                Color color = (Color) {
                    centroids[cluster][0], centroids[cluster][1], centroids[cluster][2]
                };

                int color_diff = distanceSquared(&color, &image_color_data[point]);

                if (color_diff < min_distance) {
                    min_distance = color_diff;
                    cluster_id   = cluster;
                }
            }

            cluster_lists[cluster_id][cluster_size[cluster_id]++] = point;
        }

        for (int cluster = 0; cluster < n_cluster; cluster++) {
            NewCentroid(cluster_lists[cluster], centroids[cluster], cluster_size[cluster], image_color_data);
        }

        int cluster_size_converge = 0;

        for (int cluster = 0; cluster < n_cluster; cluster++) {
            if (cluster_size[cluster] == prev_cluster_size[cluster]) {
                cluster_size_converge++;
            }
        }

        if (cluster_size_converge == n_cluster) { // if all cluster size converges, stop the iteration.
            iterate = 0;
        }

        memcpy(prev_cluster_size, cluster_size, n_cluster * sizeof(int));
        iteration++;
    }

    free(cluster_size);
    cluster_size = NULL;
    free(prev_cluster_size);
    prev_cluster_size = NULL;

    for (int cluster = 0; cluster < n_cluster; cluster++) {
        free(cluster_lists[cluster]);
    }

    free(cluster_lists);
    cluster_lists = NULL;

    for (int cluster = 0; cluster < n_cluster; cluster++) {
        Color color = (Color) {
            centroids[cluster][0], centroids[cluster][1], centroids[cluster][2], 255
        };
        dominant_colors[cluster] = color;
        free(centroids[cluster]);
    }

    free(centroids);
    centroids = NULL;
}

/* This function returns the squared euclidean distance
	between the two data points (specifically returns the color difference (distance) of two RGB colors).
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
void InitializeCentroid(int **centroid, int n_points, Color *image_color_data, int n_cluster)
{

    /* Initialize 1st centroid randomly */
    int c1_random_index = GetRandomValue(0, n_points-1);    // Get a random value between min and max (both included)
    Color *cluster_color = malloc(n_cluster * sizeof(Color));

    cluster_color[0] = image_color_data[c1_random_index];

    // Initialize a centroid value for the 1st cluster
    for (int cluster = 1; cluster < n_cluster; cluster++) {
        int max_distance = 0;
        int c_id = 0;

        for (int point = 0; point < n_points; point++) {
            int min_dist = INT_MAX;

            for (int c = 0; c < cluster; c++) {
                int temp_dist = distanceSquared(&cluster_color[c], &image_color_data[point]);
                min_dist = (min_dist < temp_dist) ? min_dist : temp_dist;
            }

            // Set the maximum distance and assign it to the next centroid.
            if (min_dist > max_distance) {
                max_distance = min_dist;
                c_id = point;
            }
        }
        cluster_color[cluster] = image_color_data[c_id];
    }
    for (int cluster = 0; cluster < n_cluster; cluster++) {
        centroid[cluster][0] = cluster_color[cluster].r;
        centroid[cluster][1] = cluster_color[cluster].g;
        centroid[cluster][2] = cluster_color[cluster].b;
    }

    free(cluster_color);
    cluster_color = NULL;
}

void NewCentroid(int *cluster_list, int *centroid, int n_points, Color *image_color_data)
{
    if (cluster_list == NULL || n_points == 0) {
        return;
    }

    int r = 0, g = 0, b = 0;

    for (int point = 0; point < n_points; point++) {
        int color_index = cluster_list[point];
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


