/* Let's Make: Dangerous Dave
 * MaiZure 2017
 * shane kenny
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <SDL.h>
#include "lmdave.h"

/* Quick conversion between grid and pixel basis */
const uint8_t TILE_SIZE = 16;

int main(int argc, char* argv[])
{
	SDL_Window *window;
	SDL_Renderer *renderer;
	const uint8_t DISPLAY_SCALE = 3;
	uint32_t timer_begin;
	uint32_t timer_end;
	uint32_t delay;
	struct game_state *game;
	struct game_assets *assets;

	game = malloc(sizeof(struct game_state));
	init_game(game);

	if (SDL_Init(SDL_INIT_VIDEO))
	{
		SDL_Log("SDL error: %s", SDL_GetError());
	}

	if (SDL_CreateWindowAndRenderer(320*DISPLAY_SCALE, 200*DISPLAY_SCALE, 0, &window, &renderer))
	{
		SDL_Log("Window/Renderer error: %s", SDL_GetError());
	}
	SDL_RenderSetScale(renderer, DISPLAY_SCALE, DISPLAY_SCALE);

	assets = malloc(sizeof(struct game_assets));
	init_assets(assets, renderer);

	SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
	SDL_RenderClear(renderer);

	start_level(game);

	while (!game -> quit)
	{
		timer_begin = SDL_GetTicks();

		check_input(game);
		update_game(game);
		render(game, renderer, assets);

		timer_end = SDL_GetTicks();

		delay = 25 - (timer_end - timer_begin);
		delay = delay > 25 ? 0 : delay;
		SDL_Delay(delay);
	}

	SDL_Quit();
	free(game);
	free(assets);

	return 0;
}

void init_game(struct game_state *game)
{
	FILE *file_level;
	char fname[13];
	char file_num[4];
	int i, j;

	game->quit = 0;
	game->score = 0;
	game->tick = 0;
	game->current_level = 0;
	game->lives = 3;
	game->view_x = 0;
	game->view_y = 0;
	game->scroll_x = 0;
	game->dave_x = 2;
	game->dave_y = 8;
	game->dave_px = game->dave_x * TILE_SIZE;
	game->dave_py = game->dave_y * TILE_SIZE;
	game->jump_timer = 0;
	game->on_ground = 1;
	game->try_right = 0;
	game->try_left = 0;
	game->try_jump = 0;
	game->dave_right = 0;
	game->dave_left = 0;
	game->dave_jump = 0;

	for (j = 0; j < 5; j++)
	{
		game->monster[j].type = 0;
	}

	for (j = 0; j < 10; j++)
	{

		fname[0] = '\0';
		strcat(fname, "level");
		sprintf(&file_num[0], "%u", j);
		strcat(fname, file_num);
		strcat(fname, ".dat");

		file_level = fopen(fname, "rb");

		for (i = 0; i < sizeof(game->level[j].path); i++)
		{
			game->level[j].path[i] = fgetc(file_level);
		}
		for (i = 0; i < sizeof(game->level[j].tiles); i++)
		{
			game->level[j].tiles[i] = fgetc(file_level);
		}
		for (i = 0; i < sizeof(game->level[j].padding); i++)
		{
			game->level[j].padding[i] = fgetc(file_level);
		}
		fclose(file_level);
	}
}
void init_assets(struct game_assets *assets, SDL_Renderer *renderer)
{
	int i, j;
	char fname[13];
	char file_num[4];
	char mname[13];
	char mask_num[4];
	SDL_Surface *surface;
	SDL_Surface *mask;
	uint8_t mask_offset;
	uint8_t *surf_p;
	uint8_t *mask_p;

	for (i = 0; i < 158; i++) 
	{
		fname[0] = '\0';
		strcat(fname, "tile");
		sprintf(&file_num[0], "%u", i);
		strcat(fname, file_num);
		strcat(fname, ".bmp");
		if ((i >= 53 && i <= 59) || i == 67 || i == 68 || (i >= 71 && i <= 73) || (i >= 77 && i <= 82))
		{
			if (i >= 53 && i <= 59)
				mask_offset = 7;
			if (i >= 67 && i <= 68)
				mask_offset = 2;
			if (i >= 71 && i <= 73)
				mask_offset = 3;
			if (i >= 77 && i <= 82)
				mask_offset = 6;

			mname[0] = '\0';
			strcat(mname, "tile");
			sprintf(&mask_num[0], "%u", i + mask_offset);
			strcat(mname, mask_num);
			strcat(mname, ".bmp");

			surface = SDL_LoadBMP(fname);
			mask = SDL_LoadBMP(mname);	

			surf_p = (uint8_t*)surface->pixels;
			mask_p = (uint8_t*)mask->pixels;

			for (j = 0; j < (mask->pitch*mask->h); j++)
				surf_p[j] = mask_p[j] ? 0xFF : surf_p[j];

			SDL_SetColorKey(surface, 1, SDL_MapRGB(surface->format, 0xFF, 0xFF, 0xFF));
			assets->graphics_tiles[i] = SDL_CreateTextureFromSurface(renderer, surface);
			SDL_FreeSurface(surface);
			SDL_FreeSurface(mask);
			
		}
		else
		{
			surface = SDL_LoadBMP(fname);
			if ((i >= 89 && i <= 120) || (i >= 129 && i <= 132))
				SDL_SetColorKey(surface, 1, SDL_MapRGB(surface->format, 0x00, 0x00, 0x00));
			assets->graphics_tiles[i] = SDL_CreateTextureFromSurface(renderer, surface);
			SDL_FreeSurface(surface);

		}
	}
}

void start_level(struct game_state *game)
{
	uint8_t i;

	restart_level(game);

	for (i = 0; i < 5; i++)
	{
		game->monster[i].type = 0;
		game->monster[i].path_index = 0;
		game->monster[i].dead_timer = 0;
		game->monster[i].next_px = 0;
		game->monster[i].next_py = 0;
	}

	switch (game->current_level)
	{
		case 2:
		{
			game->monster[0].type = 89;
			game->monster[0].monster_px = 44 * TILE_SIZE;
			game->monster[0].monster_py = 4 * TILE_SIZE;

			game->monster[1].type = 89;
			game->monster[1].monster_px = 59 * TILE_SIZE;
			game->monster[1].monster_py = 4 * TILE_SIZE;
		} break;
		case 3:
		{
			game->monster[0].type = 93;
			game->monster[0].monster_px = 32 * TILE_SIZE;
			game->monster[0].monster_py = 2 * TILE_SIZE;
		} break;
		case 4:
		{
			game->monster[0].type = 97;
			game->monster[0].monster_px = 15 * TILE_SIZE;
			game->monster[0].monster_py = 3 * TILE_SIZE;
			game->monster[0].type = 97;
			game->monster[0].monster_px = 33 * TILE_SIZE;
			game->monster[0].monster_py = 3 * TILE_SIZE;
			game->monster[0].type = 97;
			game->monster[0].monster_px = 49 * TILE_SIZE;
			game->monster[0].monster_py = 3 * TILE_SIZE;
		} break;
		case 5:
		{
			game->monster[0].type = 101;
			game->monster[0].monster_px = 10 * TILE_SIZE;
			game->monster[0].monster_py = 8 * TILE_SIZE;
			game->monster[0].type = 101;
			game->monster[0].monster_px = 28 * TILE_SIZE;
			game->monster[0].monster_py = 8 * TILE_SIZE;
			game->monster[0].type = 101;
			game->monster[0].monster_px = 45 * TILE_SIZE;
			game->monster[0].monster_py = 2 * TILE_SIZE;
			game->monster[0].type = 101;
			game->monster[0].monster_px = 40 * TILE_SIZE;
			game->monster[0].monster_py = 8 * TILE_SIZE;
		} break;
		case 6:
		{
			game->monster[0].type = 105;
			game->monster[0].monster_px = 5 * TILE_SIZE;
			game->monster[0].monster_py = 2 * TILE_SIZE;
			game->monster[0].type = 105;
			game->monster[0].monster_px = 16 * TILE_SIZE;
			game->monster[0].monster_py = 1 * TILE_SIZE;
			game->monster[0].type = 105;
			game->monster[0].monster_px = 46 * TILE_SIZE;
			game->monster[0].monster_py = 2 * TILE_SIZE;
			game->monster[0].type = 105;
			game->monster[0].monster_px = 56 * TILE_SIZE;
			game->monster[0].monster_py = 3 * TILE_SIZE;
		} break;
		case 7:
		{
			game->monster[0].type = 109;
			game->monster[0].monster_px = 53 * TILE_SIZE;
			game->monster[0].monster_py = 5 * TILE_SIZE;
			game->monster[0].type = 109;
			game->monster[0].monster_px = 72 * TILE_SIZE;
			game->monster[0].monster_py = 2 * TILE_SIZE;
			game->monster[0].type = 109;
			game->monster[0].monster_px = 84 * TILE_SIZE;
			game->monster[0].monster_py = 1 * TILE_SIZE;
		} break;
		case 8:
		{
			game->monster[0].type = 113;
			game->monster[0].monster_px = 35 * TILE_SIZE;
			game->monster[0].monster_py = 8 * TILE_SIZE;
			game->monster[0].type = 113;
			game->monster[0].monster_px = 41 * TILE_SIZE;
			game->monster[0].monster_py = 8 * TILE_SIZE;
			game->monster[0].type = 113;
			game->monster[0].monster_px = 49 * TILE_SIZE;
			game->monster[0].monster_py = 2 * TILE_SIZE;
			game->monster[0].type = 113;
			game->monster[0].monster_px = 65 * TILE_SIZE;
			game->monster[0].monster_py = 8 * TILE_SIZE;
		} break;
		case 9:
		{
			game->monster[0].type = 117;
			game->monster[0].monster_px = 45 * TILE_SIZE;
			game->monster[0].monster_py = 8 * TILE_SIZE;
			game->monster[0].type = 117;
			game->monster[0].monster_px = 51 * TILE_SIZE;
			game->monster[0].monster_py = 8 * TILE_SIZE;
			game->monster[0].type = 117;
			game->monster[0].monster_px = 65 * TILE_SIZE;
			game->monster[0].monster_py = 3 * TILE_SIZE;
			game->monster[0].type = 117;
			game->monster[0].monster_px = 82 * TILE_SIZE;
			game->monster[0].monster_py = 2 * TILE_SIZE;
		} break;
	}

	game->dave_px = game->dave_x * TILE_SIZE;
	game->dave_py = game->dave_y * TILE_SIZE;
	game->dave_fire = 0;
	game->dave_jetpack = 0;
	game->dave_dead_timer = 0;
	game->trophy = 0;
	game->gun = 0;
	game->jetpack = 0;
	game->check_door = 0;
	game->view_x = 0;
	game->view_y = 0;
	game->jump_timer = 0;
	game->last_dir = 0;
	game->dbullet_px = 0;
	game->dbullet_py = 0;
	game->ebullet_px = 0;
	game->ebullet_py = 0;
}

void check_input(struct game_state *game)
{
	SDL_Event event;
	SDL_PollEvent(&event);
	const uint8_t *keystate = SDL_GetKeyboardState(NULL);
	if(keystate[SDL_SCANCODE_RIGHT])
		game->try_right = 1;
	if(keystate[SDL_SCANCODE_LEFT])
		game->try_left = 1;
	if(keystate[SDL_SCANCODE_UP])
		game->try_jump = 1;
	if(keystate[SDL_SCANCODE_DOWN])
		game->try_down = 1;
	if(keystate[SDL_SCANCODE_X])
		game->try_fire = 1;
	if(keystate[SDL_SCANCODE_Z])
		game->try_jetpack = 1;

	if (event.type == SDL_QUIT)
		game->quit = 1;
}

void update_game(struct game_state *game)
{
	check_collisions(game);
	pickup_items(game, game->check_pickup_x, game->check_pickup_y);
	update_dbullet(game);
	update_ebullet(game);
	verify_input(game);
	move_dave(game);
	move_monsters(game);
	fire_monsters(game);
	scroll_screen(game);
	apply_gravity(game);
	update_level(game);
	clear_input(game);
}

void render(struct game_state *game, SDL_Renderer *renderer, struct game_assets *assets)
{
	/* Clear back buffer with black */
	SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
	SDL_RenderClear(renderer);

	/* Draw world elements */
	draw_world(game, assets, renderer);
	draw_dave(game, assets, renderer);
	draw_monsters(game, assets, renderer);
	draw_dave_bullet(game, assets, renderer);
	draw_monster_bullet(game, assets, renderer);

	/* Draw user interface */
	draw_ui(game, assets, renderer);

	SDL_RenderPresent(renderer);
}

void check_collisions(struct game_state *game)
{
	uint8_t grid_x, grid_y;
	uint8_t type;
	game->collision_point[0] = is_clear(game, game->dave_px-1, game->dave_py-1, 1);
	game->collision_point[1] = is_clear(game, game->dave_px+10, game->dave_py+4, 1);
	game->collision_point[2] = is_clear(game, game->dave_px+11, game->dave_py+4, 1);
	game->collision_point[3] = is_clear(game, game->dave_px+11, game->dave_py+12, 1);
	game->collision_point[4] = is_clear(game, game->dave_px+10, game->dave_py+16, 1);
	game->collision_point[5] = is_clear(game, game->dave_px+4, game->dave_py+16, 1);
	game->collision_point[6] = is_clear(game, game->dave_px+3, game->dave_py+12, 1);
	game->collision_point[7] = is_clear(game, game->dave_px+3, game->dave_py+4, 1);

	game->on_ground = ((!game-> collision_point[4] && !game->collision_point[5]) || game->dave_climb);
	grid_x = (game->dave_px+6) / TILE_SIZE;
	grid_y = (game->dave_py+8) / TILE_SIZE;

	if (grid_x < 100 && grid_y < 10)
		type = game->level[game->current_level].tiles[grid_y*100+grid_x];
	else
		type = 0;

	if ((type >= 33 && type <= 35) || type == 41)
		game->can_climb = 1;
	else
	{
		game->can_climb = 0;
		game->dave_climb = 0;
	}
}

uint8_t is_clear(struct game_state *game, uint16_t px, uint16_t py, uint8_t is_dave)
{
	uint8_t grid_x;
	uint8_t grid_y;
	uint8_t type;

	grid_x = px / TILE_SIZE;
	grid_y = py / TILE_SIZE;

	if (grid_x > 99 || grid_y > 9)
		return 1;

	type = game->level[game->current_level].tiles[grid_y*100+grid_x];

	if (type == 1) { return 0; }
	if (type == 3) { return 0; }
	if (type == 5) { return 0; }
	if (type == 15) { return 0; }
	if (type == 16) { return 0; }
	if (type == 17) { return 0; }
	if (type == 18) { return 0; }
	if (type == 19) { return 0; }
	if (type == 21) { return 0; }
	if (type == 22) { return 0; }
	if (type == 23) { return 0; }
	if (type == 24) { return 0; }
	if (type == 29) { return 0; }
	if (type == 30) { return 0; }

	if (is_dave)
	{
		switch (type)
		{
			case 2: game->check_door = 1; break;
			case 4:
			case 10:
			case 20:
			case 47:
			case 48:
			case 49:
			case 50:
			case 51:
			case 52:
			{
				game->check_pickup_x = grid_x;
				game->check_pickup_y = grid_y;
			} break;
			case 6:
			case 25:
			case 36:
			{
				if (!game->dave_dead_timer)
					game->dave_dead_timer = 30;
			}
			default: break;
		}
	}

	return 1;
}

void verify_input(struct game_state *game)
{
	if (game->dave_dead_timer)
		return;

	if (game->try_right && game->collision_point[2] && game->collision_point[3])
		game->dave_right = 1;

	if (game->try_left && game->collision_point[6] && game->collision_point[7])
		game->dave_left = 1;

	if (game->try_jump && game->on_ground && !game->dave_jump && !game->dave_jetpack && !game->can_climb && game->collision_point[0] && game->collision_point[1])
		game->dave_jump = 1;

	if (game->try_jump && game->can_climb)
	{
		game->dave_up = 1;
		game->dave_climb = 1;
	}
	
	if (game->try_fire && game->gun && !game->dbullet_px && !game->dbullet_py)
		game->dave_fire = 1;

	if (game->try_jetpack && game->jetpack && !game->jetpack_delay)
	{
		game->dave_jetpack = !game->dave_jetpack;
		game->jetpack_delay = 10;
	}

	if (game->try_down && (game->dave_jetpack || game->dave_climb) && game->collision_point[4] && game->collision_point[5])
		game->dave_down = 1;

	if (game->try_jump && game->dave_jetpack && game->collision_point[0] && game->collision_point[1])
		game->dave_up = 1;
}

void move_dave(struct game_state *game)
{
	game->dave_x = game->dave_px / TILE_SIZE;
	game->dave_y = game->dave_py / TILE_SIZE;

	if (game->dave_y > 9)
	{
		game->dave_y = 0;
		game->dave_py = -16;
	}

	if (game->dave_dead_timer)
		return;

	if (game->dave_right)
	{
		game->dave_px += 2;
		game->last_dir = 1;
		game->dave_tick++;
		game->dave_right = 0;
	}

	if (game->dave_left)
	{
		game->dave_px -= 2;
		game->last_dir = -1;
		game->dave_tick++;
		game->dave_left = 0;
	}

	if (game->dave_down)
	{
		game->dave_py += 2;
		game->dave_down = 0;
	}

	if (game->dave_up)
	{
		game->dave_py -= 2;
		game->dave_up = 0;
	}

	if (game->dave_jump)
	{
		if (!game->jump_timer)
		{
			game->jump_timer = 25;
			game->last_dir = 0;
		}

		if (game->collision_point[0] && game->collision_point[1])
		{
			if (game->jump_timer > 10)
				game->dave_py -= 2;
			if (game->jump_timer >= 5 && game->jump_timer <= 10)
				game->dave_py -= 1;
		}

		game->jump_timer--;

		if (!game->jump_timer)
			game->dave_jump = 0;
	}

	if (game->dave_fire)
	{
		game->dbullet_dir = game->last_dir;

		if (!game->dbullet_dir)
			game->dbullet_dir = 1;

		if (game->dbullet_dir == 1)
			game->dbullet_px = game->dave_px + 18;

		if (game->dbullet_dir == -1)
			game->dbullet_px = game->dave_px - 8;

		game->dbullet_py = game->dave_py + 8;
		game->dave_fire = 0;
	}
}

void move_monsters(struct game_state *game)
{
	uint8_t i, j;

	for (i = 0; i < 8; i++)
	{
		struct monster_state *m;
		m = &game->monster[i];

		if (m->type && !m->dead_timer)
		{
			for (j = 0; j < 2; j++)
			{
				if (!m->next_px && !m->next_py)
				{
					m->next_px = game->level[game->current_level].path[m->path_index];
					m->next_py = game->level[game->current_level].path[m->path_index+1];
					m->path_index += 2;
				}

				if (m->next_px == (signed char)0xEA && m->next_py == (signed char)0xEA)
				{
					m->next_px = game->level[game->current_level].path[0];
					m->next_py = game->level[game->current_level].path[1];
					m->path_index = 2;
				}

				if (m->next_px < 0)
				{
					m->monster_px -= 1;
					m->next_px++;
				}

				if (m->next_px > 0)
				{
					m->monster_px += 1;
					m->next_px--;
				}

				if (m->next_py < 0)
				{
					m->monster_py -= 1;
					m->next_py++;
				}

				if (m->next_py > 0)
				{
					m->monster_py += 1;
					m->next_py--;
				}

				m->monster_x = m->monster_px / TILE_SIZE;
				m->monster_y = m->monster_py / TILE_SIZE;
			}
		}
	}
}

void fire_monsters(struct game_state *game)
{
	uint8_t i;

	if (!game->ebullet_px && !game->ebullet_py)
	{
		for (i = 0; i < 5; i++)
		{
			if (game->monster[i].type && is_visible(game, game->monster[i].monster_px) && !game->monster[i].dead_timer)
			{
				game->ebullet_dir = game->dave_px < game->monster[i].monster_px ? -1 : 1;

				if (!game->ebullet_dir)
					game->ebullet_dir = 1;

				if (game->ebullet_dir == 1)
					game->ebullet_px = game->monster[i].monster_px	+ 18;

				if (game->ebullet_dir == -1)
					game->ebullet_px = game->monster[i].monster_px	- 8;

				game -> ebullet_py = game->monster[i].monster_py + 8;
			}
		}
	}
}

void apply_gravity(struct game_state *game)
{
	if (!game->dave_jump && !game->on_ground && !game->dave_jetpack && !game->dave_climb)
	{
		if (is_clear(game, game->dave_px+4, game->dave_py+17, 1))
			game->dave_py += 2;
		else
		{
			uint8_t not_align;
			not_align = game->dave_py % TILE_SIZE;

			if (not_align)
			{
				game->dave_py = not_align < 8 ?
					game->dave_py - not_align :
					game->dave_py + TILE_SIZE - not_align;
			}
		}
	}
}

void update_level(struct game_state *game)
{
	uint8_t i;

	game->tick++;
	if (game->jetpack_delay)
		game->jetpack_delay--;

	if (game->dave_jetpack)
	{
		game->jetpack--;
		if (!game->jetpack)
			game->jetpack = 0;
	}
	if (game->check_door)
	{
		if (game->trophy)
		{
			add_score(game, 2000);
			if (game->current_level < 9)
			{
				game->current_level++;
				start_level(game);
			}
			else
			{
				printf("You won with %u points!\n", game->score);
				game->quit = 1;
			}
		}
		else
			game->check_door = 0;
	}

	if (game->dave_dead_timer)
	{
		game->dave_dead_timer--;
		if (!game->dave_dead_timer)
		{
			if (game->lives)
			{
				game->lives--;
				restart_level(game);
			}
			else
				game->quit = 1;
		}
	}

	for (i = 0; i < 5; i++)
	{
		if (game->monster[i].dead_timer)
		{
			game->monster[i].dead_timer--;
			if (!game->monster[i].dead_timer)
			{
				game->monster[i].type = 0;
			}
		}
		else
		{
			if (game->monster[i].type)
			{
				if (game->monster[i].monster_x == game->dave_x && game->monster[i].monster_y == game->dave_y)
				{
					game->dave_dead_timer = 30;
					game->monster[i].dead_timer = 30;
					add_score(game, 300);
				}
			}
		}
	}
}

void restart_level(struct game_state *game)
{
	switch (game->current_level)
	{
		case 0: game->dave_x = 2; game->dave_y = 8; break;
		case 1: game->dave_x = 1; game->dave_y = 8; break;
		case 2: game->dave_x = 2; game->dave_y = 5; break;
		case 3: game->dave_x = 1; game->dave_y = 5; break;
		case 4: game->dave_x = 2; game->dave_y = 8; break;
		case 5: game->dave_x = 2; game->dave_y = 8; break;
		case 6: game->dave_x = 1; game->dave_y = 2; break;
		case 7: game->dave_x = 2; game->dave_y = 8; break;
		case 8: game->dave_x = 0; game->dave_y = 1; break;
		case 9: game->dave_x = 2; game->dave_y = 8; break;
	}

	game->dave_px = game->dave_x * TILE_SIZE;
	game->dave_py = game->dave_y * TILE_SIZE;
	game->dave_jump = game->try_jump = game->dave_jetpack = 0;
	clear_input(game);
}

void clear_input(struct game_state *game)
{
	game->try_jump = 0;
	game->try_left = 0;
	game->try_right = 0;
	game->try_fire = 0;
	game->try_jetpack = 0;
	game->try_down = 0;
	game->try_up = 0;
}

void scroll_screen(struct game_state *game)
{
	if (game->dave_x - game->view_x >= 18)
		game->scroll_x = 15;

	if (game->dave_x - game->view_x < 2)
		game-> scroll_x = -15;
	if (game->scroll_x > 0)
	{
		if (game->view_x == 80)
			game->scroll_x = 0;
		else
		{
			game->view_x++;
			game->scroll_x--;
		}
	}
	if (game->scroll_x < 0)
	{
		if (game->view_x == 0)
			game->scroll_x = 0;
		else
		{
			game->view_x--;
			game->scroll_x++;
		}
	}
}

void pickup_items(struct game_state *game, uint8_t grid_x, uint8_t grid_y)
{
	uint8_t type;

	if (!grid_x || !grid_y)
		return;

	type = game->level[game->current_level].tiles[grid_y*100+grid_x];

	switch (type)
	{
		/* Add score and special item cases here later */
		case 4: 
		{
			game->jetpack = 0xFF;
		} break;
		case 10:
		{
			add_score(game, 1000);
			game->trophy = 1;
		} break;
		case 20:
		{
			game->gun = 1;
		} break;
		case 47: add_score(game, 100); break;
		case 48: add_score(game, 50); break;
		case 49: add_score(game, 150); break;
		case 50: add_score(game, 300); break;
		case 51: add_score(game, 200); break;
		case 52: add_score(game, 500); break;
		default: break;

	}
	game->level[game->current_level].tiles[grid_y*100+grid_x] = 0;

	game->check_pickup_x = 0;
	game->check_pickup_y = 0;
}

/* Update frame animation based on tick timer and tile's type count */
uint8_t update_frame(struct game_state *game, uint8_t tile, uint8_t salt)
{
	uint8_t mod;

	switch (tile)
	{
		case 6: mod = 4; break;
		case 10: mod = 5; break;
		case 25: mod = 4; break;
		case 36: mod = 5; break;
		case 129: mod = 4; break;
		default: mod = 1; break;
	}

	return tile + (salt + game->tick/5) % mod;
}

void update_dbullet(struct game_state *game)
{
	uint8_t i;
	uint8_t grid_x, grid_y;
	uint8_t mx, my;

	if (!game->dbullet_px || !game->dbullet_py)
		return;

	if (!is_clear(game, game->dbullet_px, game->dbullet_py, 0))
		game->dbullet_px = game->dbullet_py = 0;

	grid_x = game->dbullet_px / TILE_SIZE;
	grid_y = game->dbullet_py / TILE_SIZE;

	if (grid_x - game->view_x < 1 || grid_x - game->view_x > 20)
		game->dbullet_px = game->dbullet_py = 0;

	if (game->dbullet_px)
	{
		game->dbullet_px += game->dbullet_dir * 4;

		for (i = 0; i < 5; i++)
		{
			if (game->monster[i].type)
			{
				//printf("%d == %d || %d == %d && %d == %d || %d == %d\n", grid_y, my, grid_y, my + 1, grid_x, mx, grid_x, mx + 1);
				mx = game->monster[i].monster_px / TILE_SIZE;
				my = game->monster[i].monster_py / TILE_SIZE;

				if ((grid_y == my || grid_y == my + 1) && (grid_x == mx || grid_x == mx + 1))
				{
					//printf("here0");
					game->dbullet_px = game->dbullet_py = 0;
					game->monster[i].dead_timer = 30;
				}
			}
		}
	}
}

void update_ebullet(struct game_state *game)
{
	if (!game->ebullet_px || !game->ebullet_py)
		return;

	if (!is_clear(game, game->ebullet_px, game->ebullet_py, 0))
		game->ebullet_px = game->ebullet_py = 0;

	if (!is_visible(game, game->ebullet_px))
		game->ebullet_px = game->ebullet_py = 0;

	if (game->ebullet_px)
	{
		uint8_t grid_x, grid_y;

		game->ebullet_px += game->ebullet_dir * 4;

		grid_x = game->ebullet_px / TILE_SIZE;
		grid_y = game->ebullet_py / TILE_SIZE;
		
		if (grid_y == game->dave_y && grid_x == game->dave_x)
		{
			game->ebullet_px = game->ebullet_py = 0;
			game->dave_dead_timer = 30;
		}
	}
}

/* Render the world */
void draw_world(struct game_state *game, struct game_assets *assets, SDL_Renderer *renderer)
{
	SDL_Rect dest;
	uint8_t tile_index;
	uint8_t i, j;

	/* Draw each tile in row-major */
	for (j=0; j < 10; j++)
	{
		dest.y = TILE_SIZE + j * TILE_SIZE;
		dest.w = TILE_SIZE;
		dest.h = TILE_SIZE;
		for (i=0; i < 20; i++)
		{
			dest.x = i * TILE_SIZE;
			tile_index = game->level[game->current_level].tiles[j*100+game->view_x+i];
			tile_index = update_frame(game,tile_index,i);
			SDL_RenderCopy(renderer, assets->graphics_tiles[tile_index], NULL, &dest);
		}
	}
}

void draw_dave(struct game_state *game, struct game_assets *assets, SDL_Renderer *renderer)
{
	SDL_Rect dest;
	uint8_t tile_index;

	dest.x = game->dave_px - game->view_x * TILE_SIZE;
	dest.y = TILE_SIZE + game->dave_py;
	dest.w = 20;
	dest.h = 16;

	if (!game->last_dir)
		tile_index = 56;
	else
	{
		tile_index = game->last_dir >= 0 ? 53 : 57;
		tile_index += (game->dave_tick/5) % 3;
	}

	if (game->dave_jetpack)
		tile_index = game->last_dir >= 0 ? 77 : 80;
	else 
	{
		if (game->dave_jump || !game->on_ground)
			tile_index = game->last_dir >= 0 ? 67 : 68;
		if (game->dave_climb)
			tile_index = 71 + (game->dave_tick/5) % 3;
	}

	if (game->dave_dead_timer)
		tile_index = 129 + (game->tick/3) % 4;

	SDL_RenderCopy(renderer, assets->graphics_tiles[tile_index], NULL, &dest);
}

void draw_monsters(struct game_state *game, struct game_assets *assets, SDL_Renderer *renderer)
{
	SDL_Rect dest;
	uint8_t tile_index;
	uint8_t i;

	for (i = 0; i < 5; i++)
	{
		struct monster_state *m;
		m = &game->monster[i];

		if (m->type)
		{
			dest.x = m->monster_px - game->view_x * TILE_SIZE;
			dest.y = TILE_SIZE + m->monster_py;
			dest.w = 20;
			dest.h = 16;

			tile_index = m->dead_timer ? 129 : m->type;
			tile_index += (game->tick/3) % 4;
		}
		SDL_RenderCopy(renderer, assets->graphics_tiles[tile_index], NULL, &dest);
	}
}

void draw_dave_bullet(struct game_state *game, struct game_assets *assets, SDL_Renderer *renderer)
{
	SDL_Rect dest;
	uint8_t tile_index;

	if (game->dbullet_px && game->dbullet_py)
	{
		dest.x = game->dbullet_px - game->view_x * TILE_SIZE;
		dest.y = TILE_SIZE + game->dbullet_py;
		dest.w = 12;
		dest.h = 3;

		tile_index = game->dbullet_dir > 0 ? 127: 128;

		SDL_RenderCopy(renderer, assets->graphics_tiles[tile_index], NULL, &dest);
	}
}

void draw_monster_bullet(struct game_state *game, struct game_assets *assets, SDL_Renderer *renderer)
{
	SDL_Rect dest;
	uint8_t tile_index;

	if (game->ebullet_px && game->ebullet_py)
	{
		dest.x = game->ebullet_px - game->view_x * TILE_SIZE;
		dest.y = TILE_SIZE + game->ebullet_py;
		dest.w = 12;
		dest.h = 3;

		tile_index = game->ebullet_dir > 0 ? 121: 124;

		SDL_RenderCopy(renderer, assets->graphics_tiles[tile_index], NULL, &dest);
	}
}

void draw_ui(struct game_state *game, struct game_assets *assets, SDL_Renderer *renderer)
{
	SDL_Rect dest;
	uint8_t i;

	dest.x = 0;
	dest.y = 16;
	dest.w = 960;
	dest.h = 1;
	SDL_SetRenderDrawColor(renderer, 0xEE, 0xEE, 0xEE, 0xFF);
	SDL_RenderFillRect(renderer, &dest);
	dest.y = 176;
	SDL_RenderFillRect(renderer, &dest);

	/* SCORE */
	dest.x = 1;
	dest.y = 2;
	dest.w = 62;
	dest.h = 11;
	SDL_RenderCopy(renderer, assets->graphics_tiles[137], NULL, &dest);

	/* LEVEL */
	dest.x = 120;
	SDL_RenderCopy(renderer, assets->graphics_tiles[136], NULL, &dest);

	/* LIVES */
	dest.x = 200;
	SDL_RenderCopy(renderer, assets->graphics_tiles[135], NULL, &dest);

	/* NUMBERS */
	// score
	dest.x = 64;
	dest.w = 8;
	dest.h = 11;
	SDL_RenderCopy(renderer, assets->graphics_tiles[148 + (game->score  / 10000) % 10], NULL, &dest);
	dest.x = 72;
	SDL_RenderCopy(renderer, assets->graphics_tiles[148 + (game->score  / 1000) % 10], NULL, &dest);
	dest.x = 80;
	SDL_RenderCopy(renderer, assets->graphics_tiles[148 + (game->score  / 100) % 10], NULL, &dest);
	dest.x = 88;
	SDL_RenderCopy(renderer, assets->graphics_tiles[148 + (game->score  / 10) % 10], NULL, &dest);
	dest.x = 96;
	SDL_RenderCopy(renderer, assets->graphics_tiles[148], NULL, &dest);

	// level
	dest.x = 170;
	SDL_RenderCopy(renderer, assets->graphics_tiles[148 + (game->current_level+1  / 10) % 10], NULL, &dest);
	SDL_RenderCopy(renderer, assets->graphics_tiles[148 + (game->current_level+1) % 10], NULL, &dest);

	// lives
	for (i = 0; i < game->lives; i++)
	{
		dest.x = (255+16*i);
		dest.w = 16;
		dest.h = 12;
		SDL_RenderCopy(renderer, assets->graphics_tiles[143], NULL, &dest);
	}

	// trophy message
	if (game->trophy)
	{
		dest.x = 72;
		dest.y = 180;
		dest.w = 176;
		dest.h = 14;
		SDL_RenderCopy(renderer, assets->graphics_tiles[138], NULL, &dest);
	}

	// gun message
	if (game->gun)
	{
		dest.x = 255;
		dest.y = 180;
		dest.w = 62;
		dest.h = 11;
		SDL_RenderCopy(renderer, assets->graphics_tiles[134], NULL, &dest);
	}

	// jetpack message
	if (game->jetpack)
	{
		dest.x = 1;
		dest.y = 177;
		dest.w = 62;
		dest.h = 11;
		SDL_RenderCopy(renderer, assets->graphics_tiles[133], NULL, &dest);

		dest.x = 1;
		dest.y = 190;
		dest.w = 62;
		dest.h = 8;
		SDL_RenderCopy(renderer, assets->graphics_tiles[141], NULL, &dest);

		dest.x = 2;
		dest.y = 192;
		dest.w = game->jetpack * 0.23;
		dest.h = 4;
		SDL_SetRenderDrawColor(renderer, 0xEE, 0x00, 0x00, 0xFF);
		SDL_RenderFillRect(renderer, &dest);
	}

}

inline uint8_t is_visible(struct game_state *game, uint16_t px)
{
	uint8_t pos_x;
	pos_x = px / TILE_SIZE;
	return (pos_x - game->view_x < 20 && pos_x - game->view_x >= 0);
}

inline void add_score(struct game_state *game, uint16_t new_score)
{
	if (game->score / 20000 != ((game->score+new_score) / 20000))
		game->lives++;

	game->score += new_score;
}
