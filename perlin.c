#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "SDL.h"

#define NOISE_WIDTH		100
#define NOISE_HEIGHT	100

#define M_PI			3.14159265358979323846


void prepare_colour_gradient(SDL_Palette *noise_palette) {
	unsigned int colour_idx;
	for(colour_idx=0; colour_idx < (noise_palette->ncolors - 1); ++colour_idx) {
		noise_palette->colors[colour_idx].r = colour_idx;
		noise_palette->colors[colour_idx].g = colour_idx;
		noise_palette->colors[colour_idx].b = colour_idx;
		noise_palette->colors[colour_idx].a = 0;
	}
};

struct vector {
	// TODO Use ints
	float x;
	float y;
};

void random_unit_vector(struct vector *dest) {
	float angle = ((float) rand() * 2 * M_PI) /  (RAND_MAX);
	dest->x = cos(angle);
	dest->y = sin(angle);
	//printf("random x=%f y=%f length=%f\n", dest->x, dest->y, (dest->x * dest->x) + (dest->y * dest->y));
};

float ease(float x) {
	return 3 * pow(x, 2) - 2 * pow(x, 3);
}

float dot_product(const struct vector *lhs, const struct vector *rhs) {
	/*
	 * For each vector, we have -1 <= x <= 1 (within a cell of the grid)
	 * or -surface->width <= x <= surface->width
	 * the product is 
	 * -2 width^2 <=prod <= 2 width^2
	 */
	//printf("Dot product of (%f,%f) and (%f,%f)\n", lhs.x, lhs.y, rhs.x, rhs.y);
	return (lhs->x * rhs->x) + (lhs->y * rhs->y);
};

void draw_noise(SDL_Surface *surface, const unsigned int step) {
	/*
	 * So... The FAQ says that we need to overlay a grid in which
	 * the intersections (whole numbers) will have pseudo-random vectors
	 * attached to them
	 *
	 * For now, let's pretend that the whole surface is one single cell of that
	 * grid, and that therefore the intersections are the four corners of the
	 * surface.
	 *
	 * Given that assumption, the surface (WHICH IS ASSUMED TO BE SQUARE) is
	 * one unit length on the side
	 */

	assert(surface->w == surface->h);
	assert((surface->w) % step == 0);

	const unsigned int nodes_per_side = 1 + (surface->w / step);
	printf("surface side=%d step=%d nodes_per_side %d\n", surface->w, step, nodes_per_side);

	// There'll be (surface->side/step + 1) ^ 2 nodes in the grid
	struct vector *node_vectors = (struct vector*) calloc(nodes_per_side * nodes_per_side, sizeof(struct vector));

	// Would be nice if there were a nicer way that doesn't rely on integer counters
	unsigned int node_x, node_y;
	for(node_y = 0; node_y < nodes_per_side; ++node_y)
		for(node_x = 0; node_x < nodes_per_side; ++node_x)
			random_unit_vector(&node_vectors[node_y * nodes_per_side + node_x]);

	// We need to iterate on the corners of the surface/nodes on the grid
	// OR DO WE? We're going to need to paint every pixel of the surface, for
	// each we need to know the cell it belongs to and the four corners of the
	// cell
	struct vector *node_above_left;
	struct vector *node_above_right;
	struct vector *node_below_left;
	struct vector *node_below_right;

	struct vector from_above_left;
	struct vector from_above_right;
	struct vector from_below_left;
	struct vector from_below_right;

	unsigned int segment_x, segment_y;
	unsigned int x, y;
	for(y=0; y < surface->h; ++y) {
		segment_y = y / step;

		for(x=0; x < surface->w; ++x) {
			segment_x = x / step;

			node_above_left = &node_vectors[segment_y * nodes_per_side + segment_x];
			node_above_right = &node_vectors[segment_y * nodes_per_side + segment_x + 1];
			node_below_left = &node_vectors[(segment_y + 1) * nodes_per_side + segment_x];
			node_below_right = &node_vectors[(segment_y + 1) * nodes_per_side + (segment_x + 1)];

			//printf("x=%d y=%d segment_x=%d segment_y=%d\n", x, y, segment_x, segment_y);

			unsigned int node_left_x = segment_x * step;
			unsigned int node_right_x = node_left_x + step;
			unsigned int node_above_y = segment_y * step;
			unsigned int node_below_y = node_above_y + step;
			/*
			printf("left x %d, right x %d | above y %d, below_y %d\n", node_left_x, node_right_x, node_above_y, node_below_y);
			printf("above left (%f,%f) below left(%f,%f)\n", node_above_left->x, node_above_left->y, node_below_left->x, node_below_left->y);
			printf("above right (%f,%f) below right(%f,%f)\n", node_above_right->x, node_above_right->y, node_below_right->x, node_below_right->y);
			*/

			from_below_left.x = ((float) x - node_left_x) / step;
			from_below_left.y = ((float) y - node_below_y) / step;

			from_below_right.x = ((float) x - node_right_x) / step;
			from_below_right.y = ((float) y - node_below_y) / step;

			from_above_left.x = ((float) x - node_left_x) / step;
			from_above_left.y = ((float) y - node_above_y) / step;

			from_above_right.x = ((float) x - node_right_x) / step;
			from_above_right.y = ((float) y - node_above_y) / step;

			/*
			printf("from above left (%f,%f) from above right(%f,%f)\n", from_above_left.x, from_above_left.y, from_above_right.x, from_above_right.y);
			*/

			float s = dot_product(node_below_left, &from_below_left);
			float t = dot_product(node_below_right, &from_below_right);
			float u = dot_product(node_above_left, &from_above_left);
			float v = dot_product(node_above_right, &from_above_right);

			float raw_avg = (s+t+u+v) / 4;
			float sx_weight = ease(from_above_left.x);
			//printf("x-x0: %f y-y0: %f\n", from_above_left.x, from_above_left.y);
			float a = s + sx_weight*(t-s);
			float b = u + sx_weight*(v-u);

			// Why the (1-X)? Well, it's because the origin of the coordinate system is on the top left.
			float sy_weight = ease(1 - from_above_left.y);
			float z = a + sy_weight*(b-a);

			//printf("X=%d Y=%d s=%f t=%f u=%f v=%f avg=%f\n", x, y, s, t, u, v, raw_avg);
			unsigned int noise_idx = (unsigned int) 128 + (z * 128);
			((Uint8*) surface->pixels)[y * surface->w + x] = (Uint8) noise_idx;
		}
	}
	free(node_vectors);
};


int main(int arg_count, char **args) {
	SDL_Window		*window;
	SDL_Renderer	*renderer;
	SDL_Texture		*display_texture;
	SDL_Surface		*noise_surface;

	SDL_Init(SDL_INIT_VIDEO);

	window = SDL_CreateWindow(
		"Noise",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		NOISE_WIDTH,
		NOISE_HEIGHT,
		0
	);
	SDL_ShowCursor(false);

	renderer = SDL_CreateRenderer(
		window,
		-1,
		SDL_RENDERER_PRESENTVSYNC|SDL_RENDERER_ACCELERATED
	);

	SDL_Surface *rgb_surface = SDL_CreateRGBSurface(
		0,
		NOISE_WIDTH,
		NOISE_HEIGHT,
		32,
		0, 0, 0, 0
	);

	noise_surface = SDL_ConvertSurfaceFormat(
		rgb_surface,
		SDL_PIXELFORMAT_INDEX8,
		0
	);
	SDL_FreeSurface(rgb_surface);
	prepare_colour_gradient(noise_surface->format->palette);

	SDL_LockSurface(noise_surface);
	draw_noise(noise_surface, 25);
	SDL_UnlockSurface(noise_surface);

	SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(renderer);

	display_texture = SDL_CreateTextureFromSurface(
		renderer,
		noise_surface
	);

	SDL_RenderCopy(renderer, display_texture, NULL, NULL);
	SDL_DestroyTexture(display_texture);

	SDL_RenderPresent(renderer);


	bool running = true;
	while (running) {

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

	SDL_FreeSurface(noise_surface);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();

	return EXIT_SUCCESS;
}
