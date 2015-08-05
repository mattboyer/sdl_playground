#include "voxel.h"

void random_unit_vector(struct vector *dest) {
	float angle = ((float) rand() * 2 * M_PI) / (RAND_MAX);
	dest->x = cos(angle);
	dest->y = sin(angle);
};

inline float increasing_interpolant(float x) {
	// x=0 -> 0
	// x=1 -> 1
	// x=0.5 -> 0.5
	return 3 * pow(x, 2) - 2 * pow(x, 3);
};

inline float decreasing_interpolant(float x) {
	return 1 - increasing_interpolant(x);
};

float dot_product(const struct vector *lhs, const struct vector *rhs) {
	/*
	 * For each vector, we have -1 <= x,y <= 1 (within a cell of the grid)
	 * the product is 
	 * -2 <= dot_product <= 2
	 */
	return (lhs->x * rhs->x) + (lhs->y * rhs->y);
};

void create_noise_map(struct elevation_map *map, const unsigned int step, float *min_sum, float *max_sum) {
	assert(map->width == map->height);
	assert((map->width) % step == 0);

	const unsigned int nodes_per_side = 1 + (map->width / step);

	// Allocate vectors
	map->node_vectors = (struct vector*) calloc(nodes_per_side * nodes_per_side, sizeof(struct vector));

	// Would be nice if there were a nicer way that doesn't rely on integer counters
	unsigned int node_x, node_y;
	for(node_y = 0; node_y < nodes_per_side; ++node_y)
		for(node_x = 0; node_x < nodes_per_side; ++node_x)
			random_unit_vector(&map->node_vectors[node_y * nodes_per_side + node_x]);

	float min, max;
	min=max=0;

	unsigned int segment_x, segment_y;
	unsigned int x, y;
	for(y=0; y < map->height; ++y) {
		segment_y = y / step;

		unsigned int node_above_y = segment_y * step;
		unsigned int node_below_y = node_above_y + step;

		for(x=0; x < map->width; ++x) {
			segment_x = x / step;

			unsigned int vector_idx_above_left, vector_idx_above_right, vector_idx_below_left, vector_idx_below_right;
			vector_idx_above_left = segment_y * nodes_per_side + segment_x;
			vector_idx_above_right = segment_y * nodes_per_side + segment_x + 1;
			vector_idx_below_left = (segment_y + 1) * nodes_per_side + segment_x;
			vector_idx_below_right = (segment_y + 1) * nodes_per_side + segment_x + 1;

			unsigned int node_left_x = segment_x * step;
			unsigned int node_right_x = node_left_x + step;

			struct vector from_below_left = {
				.x = ((float) x - node_left_x) / step,
				.y = ((float) y - node_below_y) / step,
			};

			struct vector from_below_right = {
				.x = ((float) x - node_right_x) / step,
				.y = from_below_left.y,
			};

			struct vector from_above_left = {
				.x = from_below_left.x,
				.y = ((float) y - node_above_y) / step,
			};

			struct vector from_above_right = {
				.x = from_below_right.x,
				.y = from_above_left.y,
			};

			float s = dot_product(&map->node_vectors[vector_idx_below_left], &from_below_left);
			float t = dot_product(&map->node_vectors[vector_idx_below_right], &from_below_right);
			float u = dot_product(&map->node_vectors[vector_idx_above_left], &from_above_left);
			float v = dot_product(&map->node_vectors[vector_idx_above_right], &from_above_right);

			float bottom_pair_avg = decreasing_interpolant(from_above_left.x) * s + increasing_interpolant(from_above_left.x) * t;
			float top_pair_avg = decreasing_interpolant(from_above_left.x) * u + increasing_interpolant(from_above_left.x) * v;
			float sum = decreasing_interpolant(from_above_left.y) * top_pair_avg + increasing_interpolant(from_above_left.y) * bottom_pair_avg;

			if (sum < min)
				min = sum;
			if (sum > max)
				max = sum;
			map->elevations[y][x] = sum;
		}
	}
	*min_sum = min;
	*max_sum = max;
};

void normalise_map(struct elevation_map *map, const float min, const float max) {
	unsigned int map_x, map_y;

	for (map_y = 0; map_y < map->height; ++map_y) {
		for (map_x = 0; map_x < map->width; ++map_x) {
			float normalised_z = map->elevations[map_y][map_x];
			// Normalise the values to be 0 <= x <= 1
			normalised_z += fabs(min);
			normalised_z /= (max + fabs(min));
			map->elevations[map_y][map_x] = normalised_z;
		};
	};
};

void push_gradient(struct colour_ramp *ramp, float gradient_max, unsigned int hex_colour) {
	assert(gradient_max > ramp->min);
	assert(gradient_max < ramp->max);

	struct ramp_gradient *new_gradient = malloc(sizeof(struct ramp_gradient));
	*new_gradient = (struct ramp_gradient) {
		.max = gradient_max,
		.colour = {
			.r=(hex_colour & 0xFF0000) >> 16,
			.g=(hex_colour & 0x00FF00) >> 8,
			.b=(hex_colour & 0x0000FF),
			.a=0x00,
		},
		.next = NULL,
	};

	struct ramp_gradient *previous, *current;
	previous = current = ramp->gradients;
	while (current) {
		if (current->max > new_gradient->max)
			break;
		previous = current;
		current = current->next;
	};
	new_gradient->next = current;

	if (previous)
		previous->next = new_gradient;
	else
		ramp->gradients = new_gradient;
};

void elevation_to_colour(float elevation, struct colour_ramp *ramp, SDL_Colour *colour) {
	/*
	 * It is assumed that the list of gradients is sorted!
	 */
	SDL_Color *bottom_colour, *top_colour;
	float min, max;
	bottom_colour = &ramp->min_colour;
	min = ramp->min;

	struct ramp_gradient *current = ramp->gradients;
	while (current) {
		if (elevation <= current->max)
			break;
		min = current->max;
		bottom_colour = &current->colour;
		current = current->next;
	};

	if (current) {
		max = current->max;
		top_colour = &current->colour;
	} else {
		max = ramp->max;
		top_colour = &ramp->max_colour;
	};

	// Normalise the elevation relative to the elevation_gradient
	float normalised = (elevation - min) / (max - min);

	//FIXME we don't need to call the interpolant 6 times!!!!
	colour->r = bottom_colour->r * decreasing_interpolant(normalised) + top_colour->r * increasing_interpolant(normalised);
	colour->g = bottom_colour->g * decreasing_interpolant(normalised) + top_colour->g * increasing_interpolant(normalised);
	colour->b = bottom_colour->b * decreasing_interpolant(normalised) + top_colour->b * increasing_interpolant(normalised);
};

void render_terrain(SDL_Renderer *renderer, const struct elevation_map *map, const struct vector *position, const unsigned int depth) {
	unsigned int map_x, map_y;
	map_x = (unsigned int) position->x;
	map_y = (unsigned int) position->y;

	SDL_Texture *target;
	int target_w, target_h;
	target = SDL_GetRenderTarget(renderer);
	SDL_QueryTexture(target, NULL, NULL, &target_w, &target_h);

	//TODO Draw a better sky
	SDL_SetRenderDrawColor(renderer, 0x77, 0xB5, 0xFE, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(renderer);

	// That's for the furthest line from the camera
	unsigned int rectangles_per_row = 100;
	int rectangle_y_on_map, rectangle_idx;
	for (rectangle_y_on_map=map_y-depth; rectangle_y_on_map <= map_y; ++rectangle_y_on_map) {
		float highest_peak_factor = HIGHEST_PEAK_TO_HEIGHT;
		for (rectangle_idx=0; rectangle_idx<rectangles_per_row; ++rectangle_idx) {
			unsigned int rectangle_x_on_map = map_x + rectangle_idx - rectangles_per_row/2;

			//TODO draw gradients instead of single-colour rectangles
			SDL_Colour rectangle_colour;
			elevation_to_colour(map->elevations[rectangle_y_on_map][rectangle_x_on_map], map->colour_ramp, &rectangle_colour);
			SDL_SetRenderDrawColor(renderer, rectangle_colour.r, rectangle_colour.g, rectangle_colour.b, rectangle_colour.a);

			SDL_Rect voxel_rect = {
				.x = rectangle_idx * (target_w/rectangles_per_row),
				.y = target_h * (1 - highest_peak_factor * map->elevations[rectangle_y_on_map][rectangle_x_on_map]),
				.w = (target_w/rectangles_per_row),
				.h = target_h * highest_peak_factor * (map->elevations[rectangle_y_on_map][rectangle_x_on_map]),
			};
			SDL_RenderFillRect(renderer, &voxel_rect);
		};
		rectangles_per_row-=2;
	};
};

void render_top_down_map(SDL_Surface *map_surface, struct elevation_map *map) {
	unsigned int surf_x, surf_y;
	SDL_Colour pix_colour;
	for (surf_y = 0; surf_y < map_surface->h; ++surf_y) {
		for (surf_x = 0; surf_x < map_surface->w; ++surf_x) {

			elevation_to_colour(
				map->elevations[surf_y][surf_x],
				map->colour_ramp,
				&pix_colour
			);

			((Uint32*) map_surface->pixels)[surf_y * map_surface->w + surf_x] = SDL_MapRGB(
				map_surface->format,
				pix_colour.r,
				pix_colour.g,
				pix_colour.b
			);
		}
	}
};

int main(int argc, char **argv) {
	SDL_Window *window;

	SDL_Renderer *renderer;

	SDL_Surface *height_map_surface;

	SDL_Texture *map_texture;
	SDL_Texture *terrain_texture;

	srand((unsigned int) time(NULL));


	struct elevation_map map;
	map.width = TERRAIN_WIDTH;
	map.height = TERRAIN_HEIGHT;
	// TODO Refactor the gradients
	map.colour_ramp = &(struct colour_ramp) {
		.min=0.,
		.max=1.,
		.min_colour={.r=0x00, .g=0x00, .b=0x80, .a=0x00},
		.max_colour={.r=0xFF, .g=0xFF, .b=0xFF, .a=0x00},
		.gradients=NULL,
	};

	push_gradient(map.colour_ramp, 0.3, 0x228B22);
	push_gradient(map.colour_ramp, 0.85, 0xC19A6B);
	push_gradient(map.colour_ramp, 0.95, 0xC8C8C8);

	float min, max;
	create_noise_map(&map, (map.width/2), &min, &max);
	normalise_map(&map, min, max);

	height_map_surface = SDL_CreateRGBSurface(
		0,
		map.width,
		map.height,
		32,
		0, 0, 0, 0
	);

	// This needs to be done *AFTER* the map's colour ramp has been defined
	render_top_down_map(height_map_surface, &map);

	SDL_Init(SDL_INIT_VIDEO);

	window = SDL_CreateWindow(
		"Voxel",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		0
	);
	SDL_ShowCursor(false);

	renderer = SDL_CreateRenderer(
		window,
		-1,
		SDL_RENDERER_PRESENTVSYNC|SDL_RENDERER_ACCELERATED
	);

	struct vector camera_position = {
		.x=map.width/2,
		.y=3*map.height/4,
	};

	terrain_texture = SDL_CreateTexture(
		renderer,
		height_map_surface->format->format,
		SDL_TEXTUREACCESS_TARGET,
		WINDOW_WIDTH,
		WINDOW_HEIGHT
	);

	bool running = true;
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

		SDL_SetRenderTarget(renderer, terrain_texture);
		render_terrain(renderer, &map, &camera_position, 20);
		SDL_SetRenderTarget(renderer, NULL);
		SDL_RenderCopy(renderer, terrain_texture, NULL, NULL);

		/* Copy the 2D top-down map surface to the top-left corner
		 */
		map_texture = SDL_CreateTextureFromSurface(
			renderer,
			height_map_surface
		);
		SDL_Rect map_rect = {
			.x=0,
			.y=0,
			.w=height_map_surface->w,
			.h=height_map_surface->h,
		};
		SDL_Rect camera_rect = {
			.x=camera_position.x-1,
			.y=camera_position.y-1,
			.w=2,
			.h=2,
		};
		SDL_RenderCopy(renderer, map_texture, NULL, &map_rect);
		SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, SDL_ALPHA_OPAQUE);
		SDL_RenderFillRect(renderer, &camera_rect);

		// Causes the renderer to push whatever it's done since last time
		// to the window it's tied to
		SDL_RenderPresent(renderer);

		SDL_Event event;
		while (0 != SDL_PollEvent(&event)) {
			if (SDL_QUIT == event.type)
				running = false;
			if (SDL_KEYDOWN == event.type) {
				switch (event.key.keysym.sym) {
					case SDLK_q:
						running = false;
						break;
					case SDLK_UP:
						camera_position.y-=1;
						break;
					case SDLK_DOWN:
						camera_position.y+=1;
						break;
					case SDLK_LEFT:
						camera_position.x-=1;
						break;
					case SDLK_RIGHT:
						camera_position.x+=1;
						break;
				};
			}
		}
	}

	SDL_FreeSurface(height_map_surface);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyTexture(map_texture);
	SDL_DestroyTexture(terrain_texture);
	SDL_DestroyWindow(window);
	free(map.node_vectors);
	//free(map.colour_ramp);

	SDL_Quit();
	return EXIT_SUCCESS;
}
