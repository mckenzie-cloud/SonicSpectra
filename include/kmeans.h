#ifndef KMEANS_HEADER
#define KMEANS_HEADER

void getDominantColors(int n_points, Color *image_color_data, Color *dominant_colors);
int gen_random_number(int lower_bound, int upper_bound);
int distanceSquared(Color *color1, Color *color2);
void InitializeCentroid(int centroid[][3], int n_points, Color *image_color_data);
void getNewCentroid(int *cluster_list, int *centroid, int n_points, Color *image_color_data);

#endif // KMEANS_HEADER
