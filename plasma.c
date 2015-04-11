#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "SDL.h"

static int sin_array[256];

void prepare_sin() {
		int i;
		for (i=0; i < 256; ++i) {
				sin_array[i] = (int) 64 * sin(2 * M_PI * i / 256);
				//printf("i=%d -> sin=%d\n", i, sin_array[i]);
		}
};

int main() {
	prepare_sin();
	/*
	 * A window is just a window, with height, width, a title and a
	 * position on the screen (to be managed by the WM).
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
	 * painful, unaccelerated equivalent of SDL_Texture.
	 */
	SDL_Surface *my_surface;

	/*
	 * A texture is created for a specific rendering context. It has a
	 * width, height and a specific pixel format.
	 *
	 * A texture can be copied onto the renderer's target (after it's been
	 * used for "offline" rendering.
	 *
	 * A texture can be created from a surface
	 *
	 * Textures can be locked and unlocked
	 */
	SDL_Texture *render_target;

	/*
	 * A rectangle is just an "empty" rectangle with width, height and a
	 * position relative to the upper left corner of ???
	 *
	 * Among other things, these are used to designate sections of other
	 * 2-dimensional entities.
	 */
	SDL_Rect draw_rect;

	SDL_Init(SDL_INIT_VIDEO);

	//SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Foo", "Oh no!!!", window);

	SDL_CreateWindowAndRenderer(
		400, // width, in pixels
		400, // height, in pixels
		0,
		&window,
		&renderer
	);
	SDL_RendererInfo driver_info;
	SDL_GetRendererInfo(renderer, &driver_info);
	printf("Active renderer uses \"%s\" driver\n", driver_info.name);

	// TODO Use a different texture for rendering then SDL_RenderCopy() it
	// onto the window's renderer
	// Is that really necessary?
	SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(renderer);


	int t_w=0, t_h=0;
	SDL_GetRendererOutputSize(renderer, &t_w, &t_h);
	printf("%d x %d\n", t_w, t_h);

	if (render_target = SDL_GetRenderTarget(renderer)) {
		t_w = t_h = 0;
		SDL_QueryTexture(render_target, NULL, NULL, &t_w, &t_h);
		printf("%d x %d\n", t_w, t_h);
	}

	int row, row_offset;


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

	//while (true) {
		t1 = p1;
		t2 = p2;
		for (row = 0; row < t_h; ++row) {
			t3 = p3;
			t4 = p4;

			t1 = t1 % 256;
			t2 = t2 % 256;
			z0 = sin_array[t1] + sin_array[t2];
			for(row_offset = 0; row_offset < t_w; ++row_offset) {
				t3 = t3 % 256;
				t4 = t4 % 256;

				z = z0 + sin_array[t3] + sin_array[t4];
				z = (unsigned char) z;
				SDL_SetRenderDrawColor(renderer, z, z, z, SDL_ALPHA_OPAQUE);
				SDL_RenderDrawPoint(renderer, row_offset, row);

				t3 += 1;
				t4 += 2;
			}
			t1 += 2;
			t2 += 1;
		}

		// Causes the renderer to push whatever it's done since last time to
			// the window it's tied to
			SDL_RenderPresent(renderer);
			p1 += sp1; p1 %= 256;
			p2 -= sp2; p2 %= 256;
			p3 += sp3; p3 %= 256;
			p4 -= sp4; p4 %= 256;
	//}
	SDL_Delay(3000);

	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyTexture(render_target);

	SDL_Quit();
}
