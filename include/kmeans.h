#ifndef KMEANS_HEADER
#define KMEANS_HEADER

void getDominantColors(int n_points, Color *image_color_data, Color *dominant_colors, int n_cluster);
int distanceSquared(Color *color1, Color *color2);
void InitializeCentroid(int **centroid, int n_points, Color *image_color_data,int n_cluster);
void NewCentroid(int *cluster_list, int *centroid, int n_points, Color *image_color_data);

#endif // KMEANS_HEADER
