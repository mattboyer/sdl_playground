#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "SDL.h"

#define PLASMA_WIDTH	320
#define PLASMA_HEIGHT	240

#define SIN_INDICES		256

#define SIN(x) (x>=0?sin_array[x]:-sin_array[-x])

static int sin_array[SIN_INDICES];

struct rgb {
	unsigned int r;
	unsigned int g;
	unsigned int b;
};

static struct rgb palette[2 * SIN_INDICES];

void prepare_sin() {
	int i;
	for (i=0; i < SIN_INDICES; ++i) {
		sin_array[i] = (int) 64 * sin(2 * M_PI * i / SIN_INDICES);
	}
};

void prepare_palette() {
	int palette_idx;

	// Colours for negative indices are shades of red
	for (palette_idx = -256; palette_idx < 0; ++palette_idx) {
		palette[palette_idx + 256].r = -palette_idx;
		palette[palette_idx + 256].g = 0;
		palette[palette_idx + 256].b = 0;
	}

	// Colours for positive indices are shades of green
	for (palette_idx = 0; palette_idx < 256; ++palette_idx) {
		palette[palette_idx + 256].r = 0;
		palette[palette_idx + 256].g = palette_idx;
		palette[palette_idx + 256].b = 0;
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
	SDL_Rect draw_rect;

	SDL_Init(SDL_INIT_VIDEO);

	window = SDL_CreateWindow(
		"Plasma",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		450,
		250,
		SDL_WINDOW_BORDERLESS|SDL_WINDOW_FULLSCREEN_DESKTOP|SDL_WINDOW_BORDERLESS
	);

	renderer = SDL_CreateRenderer(window, -1, 0);

	SDL_RendererInfo driver_info;
	SDL_GetRendererInfo(renderer, &driver_info);
	printf("Active renderer uses \"%s\" driver\n", driver_info.name);

	SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(renderer);

	// I want to use a Surface to directly access pixels
	// I'll then create a texture FROM that surface that the window renderer will copy FROM
	my_surface = SDL_CreateRGBSurface(
		0,
		PLASMA_WIDTH,
		PLASMA_HEIGHT,
		// Why do we even need alpha???
		32,
		0, 0, 0, 0
	);

	unsigned int surface_row, row_offset;

	// These are incremented for every frame by their respective sp*
	// increments
	int p1, p2, p3, p4;
	p1=p2=p3=p4=0;

	// I think these are increments
	int sp1, sp2, sp3, sp4;
	sp1 = 4;
	sp2 = 2;
	sp3 = 4;
	sp4 = 2;

	// These are used as indices on the sin curve
	int t1, t2, t3, t4;

	// TODO WTF?
	int z0, z;

	while (true) {

		t1 = p1;
		t2 = p2;

		SDL_LockSurface(my_surface);
		for (surface_row = 0; surface_row < my_surface->h; ++surface_row) {
			t3 = p3;
			t4 = p4;

			/* THE VALUES WE READ FROM sin_array MAY BE NEGATIVE!!!
			 * Ergo, -256 < z < 256
			 * The palette must cover the range from -256 to 256
			 */

			/* Also, the sin indices t1..t4 may have positive or negative values. sin(-a) = -sin(a) */
			t1 %= SIN_INDICES;
			t2 %= SIN_INDICES;
			z0 = SIN(t1) + SIN(t2);
			for(row_offset = 0; row_offset < my_surface->h; ++row_offset) {
				t3 %= SIN_INDICES;
				t4 %= SIN_INDICES;
				z = z0 + SIN(t3) + SIN(t4);

				struct rgb *chosen_colour = &palette[z + 256];

				Uint32 *gruh = (Uint32*) my_surface->pixels;
				gruh[my_surface->w * surface_row + row_offset] = SDL_MapRGBA(
					my_surface->format,
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
		SDL_UnlockSurface(my_surface);
		source_texture = SDL_CreateTextureFromSurface(
			renderer,
			my_surface
		);
		SDL_RenderCopy(renderer, source_texture, NULL, NULL);

		// Causes the renderer to push whatever it's done since last time
		// to the window it's tied to
		SDL_RenderPresent(renderer);

		p1 += sp1;
		p2 -= sp2;
		p3 += sp3;
		p4 -= sp4;

		SDL_PumpEvents();
		if (SDL_QuitRequested())
			break;
	}

	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyTexture(source_texture);

	SDL_Quit();
}
