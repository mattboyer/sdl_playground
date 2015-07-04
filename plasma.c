#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "SDL.h"

#define PLASMA_WIDTH	128
#define PLASMA_HEIGHT	128

#define SIN_AMPLITUDE	64
#define SIN_INDICES		256
#define PLASMA_PEAK		4 * SIN_AMPLITUDE

#define SIN(x) (x>=0?sin_array[x]:-sin_array[-x])

static int sin_array[SIN_INDICES];

struct rgb {
	unsigned int r;
	unsigned int g;
	unsigned int b;
};

static struct rgb palette[2 * PLASMA_PEAK];

void prepare_sin() {
	int i;
	for (i=0; i < SIN_INDICES; ++i) {
		sin_array[i] = (int) SIN_AMPLITUDE * sin(2 * M_PI * i / SIN_INDICES);
	}
};

void prepare_palette() {
	int palette_idx;

	// Colours for negative indices are shades of red
	for (palette_idx = -PLASMA_PEAK; palette_idx < 0; ++palette_idx) {
		palette[palette_idx + PLASMA_PEAK].r = -palette_idx;
		palette[palette_idx + PLASMA_PEAK].g = 0;
		palette[palette_idx + PLASMA_PEAK].b = 0;
	}

	// Colours for positive indices are shades of green
	for (palette_idx = 0; palette_idx < PLASMA_PEAK; ++palette_idx) {
		palette[palette_idx + PLASMA_PEAK].r = 0;
		palette[palette_idx + PLASMA_PEAK].g = palette_idx;
		palette[palette_idx + PLASMA_PEAK].b = 0;
	}
};

void draw_plasma_to_surface(SDL_Surface *plasma_surface, int p1, int p2, int p3, int p4) {

	// These are used as indices on the sin curve. They may be positive or
	// negative
	int t1, t2, t3, t4;

	unsigned int surface_row, row_offset;

	int row_base_palette_index, pixel_palette_index;

	t1 = p1;
	t2 = p2;
	for (surface_row = 0; surface_row < plasma_surface->h; ++surface_row) {

		// XXX Why do we reset t3 and t4 for every row?
		t3 = p3;
		t4 = p4;

		/* THE VALUES WE READ FROM sin_array MAY BE NEGATIVE!!!
		 * Ergo, -256 < pixel_palette_index < 256
		 * The palette must cover the range from -256 to 256
		 */

		/* There are SIN_INDICES increments between 0 and 2PI,
		 * therefore sin(x) = sin(x % SIN_INDICES)
		 */
		t1 %= SIN_INDICES;
		t2 %= SIN_INDICES;

		/* Also, the sin indices t1..t4 may have positive or negative values. sin(-a) = -sin(a) */
		row_base_palette_index = SIN(t1) + SIN(t2);
		for(row_offset = 0; row_offset < plasma_surface->w; ++row_offset) {
			t3 %= SIN_INDICES;
			t4 %= SIN_INDICES;

			pixel_palette_index = row_base_palette_index + SIN(t3) + SIN(t4);
			struct rgb *chosen_colour = &palette[pixel_palette_index + 256];

			Uint32 *gruh = (Uint32*) plasma_surface->pixels;
			gruh[plasma_surface->w * surface_row + row_offset] = SDL_MapRGBA(
				plasma_surface->format,
				chosen_colour->r,
				chosen_colour->g,
				chosen_colour->b,
				SDL_ALPHA_OPAQUE
			);

			t3 += 1;
			t4 += 2;
		}
		t1 += 2;
		t2 += 1;
	}
};

int main() {
	prepare_sin();
	prepare_palette();

	/*
	 * A window is just a window, with height, width, a title and a
	 * position on the screen (to be managed by the Window Manager).
	 */
	SDL_Window *window;

	/*
	 * A renderer is tied to one specific window (and conversely, a window
	 * should have one renderer attached to it) and makes the magic happen
	 * inside that window.
	 *
	 * A renderer has a target which is *always* a texture
	 *
	 * It is possible to create a renderer for a surface(???)
	 *
	 * It is possible to draw lines, rectangles and shit on the renderer
	 * directly
	 */
	SDL_Renderer *renderer;

	/*
	 * This is basically what other APIs call a Canvas. You've got a 2D
	 * array of pixels in a certain format. SDL_Surface is the old,
	 * painful, unaccelerated equivalent of SDL_Texture and is mostly useful
	 * for pixel-level stuff
	 */
	SDL_Surface *my_surface;

	/*
	 * A texture is created for a specific rendering context. It has a
	 * width, height and a specific pixel format.
	 *
	 * A texture can be copied onto the renderer's target (after it's been
	 * used for "offline" rendering).
	 *
	 * A texture can be created from a surface
	 *
	 * Textures can be locked and unlocked
	 */
	SDL_Texture *source_texture;

	/*
	 * A rectangle is just an "empty" rectangle with width, height and a
	 * position relative to the upper left corner of ???
	 *
	 * Among other things, these are used to designate sections of other
	 * 2-dimensional entities.
	 */
	SDL_Rect dest_rect;

	SDL_Init(SDL_INIT_VIDEO);

	window = SDL_CreateWindow(
		"Plasma",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		50,
		50,
		SDL_WINDOW_BORDERLESS|SDL_WINDOW_FULLSCREEN_DESKTOP
	);
	SDL_ShowCursor(false);

	renderer = SDL_CreateRenderer(
		window,
		-1,
		SDL_RENDERER_PRESENTVSYNC|SDL_RENDERER_ACCELERATED
	);

	// I want to use a Surface to directly access pixels
	// I'll then create a texture FROM that surface that the window renderer will copy FROM
	// XXX Why do we even need alpha??? Can I not create a surface that uses an
	// indexed pixel format referencing a propert SDL_Palette?
	my_surface = SDL_CreateRGBSurface(
		0,
		PLASMA_WIDTH,
		PLASMA_HEIGHT,
		32,
		0, 0, 0, 0
	);

	// These are incremented for every frame by their respective sp*
	// increments
	int p1, p2, p3, p4;
	p1=p2=p3=p4=0;

	// These are increments
	static unsigned int sp1, sp2, sp3, sp4;
	sp1 = 4;
	sp2 = 2;
	sp3 = 4;
	sp4 = 3;

	bool running = true;
	while (running) {

		SDL_LockSurface(my_surface);
		draw_plasma_to_surface(my_surface, p1, p2, p3, p4);
		SDL_UnlockSurface(my_surface);

		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(renderer);

		source_texture = SDL_CreateTextureFromSurface(
			renderer,
			my_surface
		);

		SDL_RenderCopy(renderer, source_texture, NULL, NULL);
		SDL_DestroyTexture(source_texture);

		// Causes the renderer to push whatever it's done since last time
		// to the window it's tied to
		SDL_RenderPresent(renderer);

		p1 += sp1;
		p2 -= sp2;
		p3 += sp3;
		p4 -= sp4;

		SDL_Event event;
		while (0 != SDL_PollEvent(&event)) {
			if (SDL_QUIT == event.type)
				running = false;
			if (SDL_KEYDOWN == event.type) {
				if (event.key.keysym.sym == SDLK_q)
					running = false;
				if (event.key.keysym.sym == SDLK_f) {
					if (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP) {
						SDL_SetWindowFullscreen(window, 0);
						SDL_SetWindowSize(window, PLASMA_WIDTH, PLASMA_HEIGHT);
						SDL_SetWindowBordered(window, true);
					} else {
						SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
						SDL_SetWindowBordered(window, false);
					}
				}
			}
		}
	}

	SDL_FreeSurface(my_surface);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();
}
