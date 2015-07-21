#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "SDL.h"

#define M_PI			3.14159265358979323846
#define TERRAIN_WIDTH	128
#define TERRAIN_HEIGHT	128

struct vector {
	// TODO Use ints
	float x;
	float y;
};

struct elevation_map {
	unsigned int width;
	unsigned int height;
	float elevations[TERRAIN_HEIGHT][TERRAIN_WIDTH];
};

void random_unit_vector(struct vector *dest) {
	float angle = ((float) rand() * 2 * M_PI) /  (RAND_MAX);
	dest->x = cos(angle);
	dest->y = sin(angle);
};

float decreasing_interpolant(float x) {
	return 2 * pow(x, 3) - 3 * pow(x, 3) + 1;
};

float increasing_interpolant(float x) {
	return 1 - decreasing_interpolant(x);
};

float dot_product(const struct vector *lhs, const struct vector *rhs) {
	/*
	 * For each vector, we have -1 <= x <= 1 (within a cell of the grid)
	 * or -surface->width <= x <= surface->width
	 * the product is 
	 * -2 width^2 <=prod <= 2 width^2
	 */
	return (lhs->x * rhs->x) + (lhs->y * rhs->y);
};

void create_noise_map(struct elevation_map *map, const unsigned int step, float *min_sum, float *max_sum) {
	assert(map->width == map->height);
	assert((map->width) % step == 0);

	const unsigned int nodes_per_side = 1 + (map->width / step);

	struct vector *node_vectors = (struct vector*) calloc(nodes_per_side * nodes_per_side, sizeof(struct vector));

	// Would be nice if there were a nicer way that doesn't rely on integer counters
	unsigned int node_x, node_y;
	for(node_y = 0; node_y < nodes_per_side; ++node_y)
		for(node_x = 0; node_x < nodes_per_side; ++node_x)
			random_unit_vector(&node_vectors[node_y * nodes_per_side + node_x]);

	struct vector from_above_left;
	struct vector from_above_right;
	struct vector from_below_left;
	struct vector from_below_right;

	float min, max;
	min=max=0;

	unsigned int segment_x, segment_y;
	unsigned int x, y;
	for(y=0; y < map->height; ++y) {
		segment_y = y / step;

		for(x=0; x < map->width; ++x) {
			segment_x = x / step;

			unsigned int vector_idx_above_left, vector_idx_above_right, vector_idx_below_left, vector_idx_below_right;
			vector_idx_above_left = segment_y * nodes_per_side + segment_x;
			vector_idx_above_right = segment_y * nodes_per_side + segment_x + 1;
			vector_idx_below_left = (segment_y + 1) * nodes_per_side + segment_x;
			vector_idx_below_right = (segment_y + 1) * nodes_per_side + segment_x + 1;

			unsigned int node_left_x = segment_x * step;
			unsigned int node_right_x = node_left_x + step;
			unsigned int node_above_y = segment_y * step;
			unsigned int node_below_y = node_above_y + step;

			from_below_left.x = ((float) x - node_left_x) / step;
			from_below_left.y = ((float) y - node_below_y) / step;

			from_below_right.x = ((float) x - node_right_x) / step;
			from_below_right.y = ((float) y - node_below_y) / step;

			from_above_left.x = ((float) x - node_left_x) / step;
			from_above_left.y = ((float) y - node_above_y) / step;

			from_above_right.x = ((float) x - node_right_x) / step;
			from_above_right.y = ((float) y - node_above_y) / step;

			float s = dot_product(&node_vectors[vector_idx_below_left], &from_below_left);
			float t = dot_product(&node_vectors[vector_idx_below_right], &from_below_right);
			float u = dot_product(&node_vectors[vector_idx_above_left], &from_above_left);
			float v = dot_product(&node_vectors[vector_idx_above_right], &from_above_right);

			float bottom_pair_avg = decreasing_interpolant(from_above_left.x) * s + increasing_interpolant(from_above_left.x) * t;
			float top_pair_avg = decreasing_interpolant(from_above_left.x) * u + increasing_interpolant(from_above_left.x) * v;
			float sum = decreasing_interpolant(from_above_left.y) * top_pair_avg + increasing_interpolant(from_above_left.y) * bottom_pair_avg;

			printf("\nPixel X/Y: %d/%d - SUM: %f (s: %f t: %f u:%f v:%f)\nNode to pixel vectors TL: (%f/%f) TR: (%f/%f) BL: (%f/%f) BR:(%f/%f)\nRandom node vectors TL: (%f/%f) TR: (%f/%f) BL: (%f/%f) BR: (%f/%f)\n",
					x,
					y,
					sum, s, t, u, v,
					from_above_left.x, from_above_left.y,
					from_above_right.x, from_above_right.y,
					from_below_left.x, from_below_left.y,
					from_below_right.x, from_below_right.y,
					node_vectors[vector_idx_above_left].x, node_vectors[vector_idx_above_left].y,
					node_vectors[vector_idx_above_right].x, node_vectors[vector_idx_above_right].y,
					node_vectors[vector_idx_below_left].x, node_vectors[vector_idx_below_left].y,
					node_vectors[vector_idx_below_right].x, node_vectors[vector_idx_below_right].y
			);
			if (sum < min)
				min = sum;
			if (sum > max)
				max = sum;
			map->elevations[y][x] = sum;

			/*
			float sx_weight = decreasing_interpolant(from_above_left.x);
			float bottom_pair_avg = s + sx_weight*(t-s);
			float top_pair_avg = u + sx_weight*(v-u);

			float sy_weight = decreasing_interpolant(1- from_above_left.y);
			//float z = top_pair_avg + sy_weight * (bottom_pair_avg - top_pair_avg);
			float z = top_pair_avg + sy_weight * (bottom_pair_avg - top_pair_avg);
			z = (z+1)/1.5;
			map->elevations[y][x] = z;
			*/
		}
	}
	free(node_vectors);
	*min_sum = min;
	*max_sum = max;
};

void elevation_to_colour(float elevation, SDL_Colour *colour) {
	/*
	 * Create a gradient between #000080 (Navy Blue) and #C19A6B ("Wood Brown")
	 * if height<=0.5, else use a gradient between #C19A6B and white
	 */
	/*
	colour->r = 0x00*(decreasing_interpolant(elevation)) + 0xC1*(increasing_interpolant(elevation));
	colour->g = 0x00*(decreasing_interpolant(elevation)) + 0x9A*(increasing_interpolant(elevation));
	colour->b = 0x80*(decreasing_interpolant(elevation)) + 0x6B*(increasing_interpolant(elevation));
	*/
	colour->r = 0xFF*(increasing_interpolant(elevation));
	colour->g = 0xFF*(increasing_interpolant(elevation));
	colour->b = 0xFF*(increasing_interpolant(elevation));
};

void create_gradient_map(struct elevation_map *map) {

	assert(map->width == map->height);

	unsigned int x, y;
	for(y=0; y < map->height; ++y) {
		float y_base = (float) y / map->height;

		for(x=0; x < map->width; ++x) {
			float x_base = (float) x / map->width;

			map->elevations[y][x] = (x_base +y_base)/2;
		}
	}
};


int main(int argc, char **argv) {
	SDL_Window *window;

	SDL_Renderer *renderer;

	SDL_Surface *height_map_surface;

	SDL_Texture *source_texture;

	struct elevation_map map;
	map.width = TERRAIN_WIDTH;
	map.height = TERRAIN_HEIGHT;

	float min, max;

	create_noise_map(&map, 16, &min, &max);
	//create_gradient_map(&map);

	height_map_surface = SDL_CreateRGBSurface(
		0,
		map.width,
		map.height,
		32,
		0, 0, 0, 0
	);

	float rendered_min, rendered_max;
	rendered_min = rendered_max = 0;
	// This rendering routine is correct!
	unsigned int surf_x, surf_y;
	SDL_Colour pix_colour;
	for(surf_y = 0; surf_y < height_map_surface->h; ++surf_y) {
		for(surf_x = 0; surf_x < height_map_surface->w; ++surf_x) {
			float pix_z = map.elevations[surf_y][surf_x];
			// Normalise the values to be 0 <= x <= 1
			pix_z += fabs(min);
			pix_z /= (max + fabs(min));

			if (pix_z < rendered_min)
				rendered_min = pix_z;
			if (pix_z > rendered_max)
				rendered_max = pix_z;

			elevation_to_colour(pix_z, &pix_colour);
			((Uint32*) height_map_surface->pixels)[surf_y * height_map_surface->w + surf_x] = SDL_MapRGB(height_map_surface->format, pix_colour.r, pix_colour.g, pix_colour.b);
		}
	}
	printf("min sum of dot products: %f | max sum of dot_products: %f\n", min, max);
	printf("renderd min: %f | rendered_max: %f\n", rendered_min, rendered_max);

	SDL_Init(SDL_INIT_VIDEO);

	window = SDL_CreateWindow(
		"Voxel",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		height_map_surface->w,
		height_map_surface->h,
		0
	);
	SDL_ShowCursor(false);

	renderer = SDL_CreateRenderer(
		window,
		-1,
		SDL_RENDERER_PRESENTVSYNC|SDL_RENDERER_ACCELERATED
	);

	bool running = true;
	//unsigned int time = 0;
	while (running) {

		// At this stage, the rough idea is to have a surface(texture???)
		// prepared of the same size as the surface/texture that will be used
		// to render the elevation map from back to front. The source
		// surface/texture would have NO transparent pixels but instead every
		// pixel should be painted with the elevation map-> color map, so it'd
		// look like horizontal stripes.
		// We'd then copy vertical rectangles from that source surface to the
		// destination surface starting from the bottom and going as far up as
		// the section of the elevation map we're plotting

		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(renderer);

		source_texture = SDL_CreateTextureFromSurface(
			renderer,
			height_map_surface
		);

		SDL_RenderCopy(renderer, source_texture, NULL, NULL);
		SDL_DestroyTexture(source_texture);

		// Causes the renderer to push whatever it's done since last time
		// to the window it's tied to
		SDL_RenderPresent(renderer);

		SDL_Event event;
		while (0 != SDL_PollEvent(&event)) {
			if (SDL_QUIT == event.type)
				running = false;
			if (SDL_KEYDOWN == event.type) {
				if (event.key.keysym.sym == SDLK_q)
					running = false;
			}
		}
	}

	SDL_FreeSurface(height_map_surface);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();
	return EXIT_SUCCESS;
}