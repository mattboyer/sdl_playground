#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>

#include "SDL.h"

#define M_PI			3.14159265358979323846
#define TERRAIN_WIDTH	200
#define TERRAIN_HEIGHT	200

#define WINDOW_HEIGHT	600
#define WINDOW_WIDTH	600

#define HIGHEST_PEAK_TO_HEIGHT	0.8

struct vector {
	// TODO Use ints
	float x;
	float y;
};

struct gradient {
	float min;
	float max;
	Uint8 min_r;
	Uint8 min_g;
	Uint8 min_b;
	Uint8 max_r;
	Uint8 max_g;
	Uint8 max_b;
};

struct elevation_map {
	unsigned int width;
	unsigned int height;
	float elevations[TERRAIN_HEIGHT][TERRAIN_WIDTH];
	struct gradient *colour_ramp;
};

////////////////

void random_unit_vector(struct vector*);

float increasing_interpolant(float);

float decreasing_interpolant(float);

float dot_product(const struct vector*, const struct vector*);

void create_noise_map(struct elevation_map *, const unsigned int, float*, float*);

void elevation_to_colour(float, struct gradient*, SDL_Colour*);

void render_map(SDL_Renderer*, const struct elevation_map*, const struct vector*, const unsigned int);
