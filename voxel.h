#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>

#include "SDL.h"

#define M_PI			3.14159265358979323846
#define TERRAIN_WIDTH	200
#define TERRAIN_HEIGHT	200

#define WINDOW_WIDTH	600
#define WINDOW_HEIGHT	600

#define HIGHEST_PEAK_TO_HEIGHT	0.8

struct vector {
	// TODO Use ints. Or maybe not. But figure out a way to make this more
	// efficient if need be. Maybe. Perhaps.
	float x;
	float y;
};

// Should a gradient define a min *AND* a max colour/value?
// Surely, the whole colour ramp has a min (ie. 0.) and a max (ie. 1.) and associated colour (blue and white) and all we should do is inject points and colours in between
struct gradient {
	float min;
	float max;
	SDL_Color min_colour;
	SDL_Color max_colour;
};

struct elevation_map {
	unsigned int width;
	unsigned int height;
	float elevations[TERRAIN_HEIGHT][TERRAIN_WIDTH];
	struct gradient *colour_ramp;
	struct vector *node_vectors;
};

////////////////

void random_unit_vector(struct vector*);

float increasing_interpolant(float);

float decreasing_interpolant(float);

float dot_product(const struct vector*, const struct vector*);

void create_noise_map(struct elevation_map *, const unsigned int, float*, float*);

void elevation_to_colour(float, struct gradient*, SDL_Colour*);

void render_terrain(SDL_Renderer*, const struct elevation_map*, const struct vector*, const unsigned int);

void render_top_down_map(SDL_Surface*, struct elevation_map*);

void normalise_map(struct elevation_map*, const float, const float);
