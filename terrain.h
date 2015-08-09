#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>

#include "SDL.h"

#define M_PI			3.14159265358979323846
#define TERRAIN_WIDTH	2000
#define TERRAIN_HEIGHT	2000
#define TERRAIN_STEP	80

#define TERRAIN_NORMALISED_MIN	-0.5
#define TERRAIN_NORMALISED_MAX	0.65

#define TOP_DOWN_MAP_SIDE	200

#define WINDOW_WIDTH	600
#define WINDOW_HEIGHT	600

#define HIGHEST_PEAK_TO_HEIGHT	0.8

struct vector {
	// TODO Use ints. Or maybe not. But figure out a way to make this more
	// efficient if need be. Maybe. Perhaps.
	float x;
	float y;
};

struct colour_ramp {
	float min;
	float max;
	SDL_Color min_colour;
	SDL_Color max_colour;
	struct ramp_gradient *gradients;
};

struct ramp_gradient {
	float max;
	SDL_Color colour;
	struct ramp_gradient *next;
};

struct elevation_map {
	unsigned int width;
	unsigned int height;
	unsigned int step;
	struct colour_ramp *colour_ramp;
	struct vector *node_vectors;
};

////////////////

void random_unit_vector(struct vector*);

float increasing_interpolant(float);

float dot_product(const struct vector*, const struct vector*);

void create_noise_vectors(struct elevation_map*);

float get_map_elevation(const struct elevation_map*, unsigned int, unsigned int);

void elevation_to_colour(float, struct colour_ramp*, SDL_Color*);

void push_gradient(struct colour_ramp*, float, SDL_Color);

void render_terrain(SDL_Renderer*, const struct elevation_map*, const struct vector*, const unsigned int);

void render_top_down_map(SDL_Surface*, struct elevation_map*, const struct vector*);

SDL_Color hex_to_colour(unsigned int);
