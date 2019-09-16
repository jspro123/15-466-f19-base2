#include "Load.hpp"
#include "SpookyGrid.hpp"
#include "data_path.hpp"
#include "gl_errors.hpp"
#include "DrawSprites.hpp"

#include <ctime> 
#include <stdlib.h>     
#include <queue>

#define PI 3.14159f

Sprite const* left_eye_up = nullptr;
Sprite const* left_eye_down = nullptr;
Sprite const* left_eye_left = nullptr;
Sprite const* left_eye_right = nullptr;
Sprite const* right_eye_up = nullptr;
Sprite const* right_eye_down = nullptr;
Sprite const* right_eye_left = nullptr;
Sprite const* right_eye_right = nullptr;
Sprite const* flash = nullptr;
Sprite const* dungeon = nullptr;


Load< SpriteAtlas > eyes(LoadTagDefault, []() -> SpriteAtlas const* {
	SpriteAtlas const* ret = new SpriteAtlas(data_path("the-planet"));

	left_eye_up = &ret->lookup("left-eye-up");
	left_eye_down = &ret->lookup("left-eye-down");
	left_eye_left = &ret->lookup("left-eye-left");
	left_eye_right = &ret->lookup("left-eye-right");
	right_eye_up = &ret->lookup("right-eye-up");
	right_eye_down = &ret->lookup("right-eye-down");
	right_eye_left = &ret->lookup("right-eye-left");
	right_eye_right = &ret->lookup("right-eye-right");

	flash = &ret->lookup("flash");
	dungeon = &ret->lookup("background");

	return ret;
	});

Load< Sound::Sample > effect_footsteps(LoadTagDefault, []() -> Sound::Sample* {
	return new Sound::Sample(data_path("footsteps.opus"));
	});

Load< Sound::Sample > effect_flash(LoadTagDefault, []() -> Sound::Sample* {
	return new Sound::Sample(data_path("flash_start.opus"));
	});

Load< Sound::Sample > effect_growl_1(LoadTagDefault, []() -> Sound::Sample* {
	return new Sound::Sample(data_path("low_growl_1.opus"));
	});

Load< Sound::Sample > effect_growl_2(LoadTagDefault, []() -> Sound::Sample* {
	return new Sound::Sample(data_path("low_growl_2.opus"));
	});

glm::vec2 GridMode::grid_to_screen(glm::vec2 coords) {
	
	int block_x = (int) (view_max.x / size.x); //Default is 90
	int block_y = (int) (view_max.y / size.y); //Default is 90

	int x = (int) coords.x * block_x - block_x;
	int y = (int) coords.y * block_y;
	
	return glm::vec2(x, y);
}

bool GridMode::is_obstacle(glm::vec2 check) {

	for (int i = 0; i < obstacles.size(); i++) {
		if (obstacles[i].grid_coords == check) {
			return true;
		}
	}

	return false;
}

glm::vec2 GridMode::breadth_first(glm::vec2 source, glm::vec2 target, int* distance) {

	bool discovered[SIZEX][SIZEX];
	glm::vec2 parents[SIZEY][SIZEY];
	std::queue< glm::vec2 > search;
	glm::vec2 current;

	auto mark_discovered = [&discovered](glm::vec2 node) {
		int x = (int) node.x;
		int y = (int) node.y;
		discovered[x-1][y-1] = true;
	};

	auto add_parent = [&parents](glm::vec2 parent, glm::vec2 child) {
		int xp = (int)parent.x;
		int yp = (int)parent.y;
		int xc = (int)child.x;
		int yc = (int)child.y;
		parents[xc - 1][yc - 1] = glm::vec2(xp, yp);
	};

	auto add_neighbor = [this, &search, &mark_discovered, &add_parent, &discovered]
						(glm::vec2 parent, glm::vec2 current) {
		int x = (int) current.x;
		int y = (int) current.y;
		if (!discovered[x-1][y-1] && !is_obstacle(current) &&
			x <= size.x && y <= size.y && x >= 1 && y >= 1) {

			mark_discovered(current);
			add_parent(parent, current);
			search.push(current);
		}

	};

	auto find_next = [&parents, &source, &distance](glm::vec2 current) {
		int x, y = 0;
		glm::vec2 child;
		while (current != source) {
			*distance += 1;
			child = current;
			x = (int) current.x;
			y = (int) current.y;
			current = parents[x-1][y-1];
		}
		return child;
	};

	for (int i = 0; i < size.x; i++) {
		for (int j = 0; j < size.y; j++) {
			discovered[i][j] = false;
			parents[i][j] = glm::vec2(0, 0);
		}
	}

	mark_discovered(source);
	search.push(source);

	while (!search.empty()) {
		current = search.front();
		if (current == target) {
			return find_next(current);
		}
		add_neighbor(current, glm::vec2(current.x - 1, current.y));
		add_neighbor(current, glm::vec2(current.x + 1, current.y));
		add_neighbor(current, glm::vec2(current.x, current.y - 1));
		add_neighbor(current, glm::vec2(current.x, current.y + 1));

		search.pop();
	}
	return glm::vec2(-1, -1);
}


bool GridMode::generate_obstacles() {
	int x, y = 0;
	srand((unsigned)(time(NULL)));
	for (int i = 0; i < number_of_obstacles; i++) {
		x = rand() % (int) size.x + 1;
		y = rand() % (int) size.y + 1;
		Obstacle obstacle = Obstacle(glm::vec2(x, y));
		obstacles.push_back(obstacle);
	}

	if (is_obstacle(monster.grid_coords) || is_obstacle(player.grid_coords) || 
		breadth_first(monster.grid_coords, player.grid_coords, &x) == glm::vec2(-1, -1)) {
		std::cout << "Redraw!" << std::endl;
		obstacles.clear();
		return false;
	}

	return true;
}

GridMode::GridMode() {

	player.shake_coords.push_back(SHAKEBY);
	player.shake_coords.push_back(SHAKEBY * 2);
	player.shake_coords.push_back(SHAKEBY);
	player.shake_coords.push_back(0.0f);
	player.shake_coords.push_back(-SHAKEBY);
	player.shake_coords.push_back(-SHAKEBY * 2);
	player.shake_coords.push_back(-SHAKEBY);
	player.shake_coords.push_back(0.0f);

	bool flag = generate_obstacles();
	while (!flag) {
		flag = generate_obstacles();
	}

	/*
	for (int i = 0; i < obstacles.size(); i++) {
	
		std::cout << i << ": " << obstacles[i].grid_coords.x << ", " << obstacles[i].grid_coords.y << std::endl;
	}
	*/
}


GridMode::~GridMode() {

}

bool GridMode::handle_event(SDL_Event const& evt, glm::uvec2 const& window_size) {


	auto handle_orientation = [this](int looking_which_way) {
		if (looking_which_way == 1) { //UP
			player.look_left = false;
			player.look_right = false;
			player.look_up = true;
			player.look_down = false;
		} else if (looking_which_way == 2) { //DOWN
			player.look_left = false;
			player.look_right = false;
			player.look_up = false;
			player.look_down = true;
		} else if (looking_which_way == 3) { //LEFT
			player.look_left = true;
			player.look_right = false;
			player.look_up = false;
			player.look_down = false;
		} else if (looking_which_way == 4) { //RIGHT
			player.look_left = false;
			player.look_right = true;
			player.look_up = false;
			player.look_down = false;
		}

		Sound::play(*effect_footsteps);
	};


	int px = (int) player.grid_coords.x;
	int py = (int) player.grid_coords.y;
	if (evt.type == SDL_KEYDOWN && till_start_time <= 0) {
		if (evt.key.keysym.sym == SDLK_UP && py < size.y) {
			if (is_obstacle(glm::vec2(px, py + 1))) {
			
			} else {
				handle_orientation(1);
				player.grid_coords = glm::vec2(px, py + 1);
			}
		}
		if (evt.key.keysym.sym == SDLK_DOWN && py > 1) {
			if (is_obstacle(glm::vec2(px, py - 1))) {

			} else {
				handle_orientation(2);
				player.grid_coords = glm::vec2(px, py - 1);
			}
		}
		if (evt.key.keysym.sym == SDLK_LEFT && px > 1) {
			if (is_obstacle(glm::vec2(px - 1, py))) {

			} else {
				handle_orientation(3);
				player.grid_coords = glm::vec2(px - 1, py);
			}
		}
		if (evt.key.keysym.sym == SDLK_RIGHT && px < size.x) {
			if (is_obstacle(glm::vec2(px + 1, py))) {

			} else {
				handle_orientation(4);
				player.grid_coords = glm::vec2(px + 1, py);
			}
		}
	}

	return false;
}


void GridMode::update(float elapsed) {

	if (monster.grid_coords == player.grid_coords) {
		exit(0);
	}

	auto calculate_angle = [this]() {

		glm::vec2 t_monster = monster.grid_coords - player.grid_coords;

		if (t_monster.y == 0) {
			return (t_monster.x > 0) ? 1.0f : -1.0f;
		}

		if (t_monster.x == 0) {
			return 0.0f;
		}

		float angle = atan(abs(t_monster.y) / (abs(t_monster.x))) * 180 / PI;
		float pan = (90.0f - angle) / 90.0f;
		if (t_monster.x > 0) {
			return pan;
		}
		
		return -pan;
	};


	if (till_start_time > 0) {
		till_start_time -= elapsed;
		flash_length -= elapsed;
		if (flash_length <= 0 && flash_obstacle) {
			//Sound effects here
			flash_obstacle = false;
			flash_length = FLASHOFF;
		} else if (flash_length <= 0 && !flash_obstacle) {
			//And here
			flash_obstacle = true;
			flash_length = FLASHON;
			Sound::play(*effect_flash);
		}
		return;
	}

	flash_obstacle = false;
	monster.till_update -= elapsed * monster.speed;
	int distance = 0;
	if (monster.till_update < 0) {
		monster.till_update = 1;
		monster.speed += 0.1f;
		monster.grid_coords = breadth_first(monster.grid_coords, player.grid_coords, &distance);

		if (distance <= 100) {
			if (!monster.cur_growl) {
				monster.cur_growl = Sound::play(*effect_growl_1);
			} else if (monster.cur_growl->stopped) {
				if (monster.current_growl == 1) {
					monster.current_growl = 2;
					monster.cur_growl = Sound::play(*effect_growl_1);
				} else if(monster.current_growl == 2) {
					monster.current_growl = 1;
					monster.cur_growl = Sound::play(*effect_growl_2);
				}
			} else {
				monster.cur_growl->set_pan(calculate_angle());
			}
		}

		if (distance <= 5) {
			player.is_shaking = true;
		} else {
			player.is_shaking = false;
		}
	}
}

void GridMode::draw(glm::uvec2 const& drawable_size) {
	//clear the color buffer:
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	//use alpha blending:
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//don't use the depth test:
	glDisable(GL_DEPTH_TEST);

	{ //use a DrawSprites to do the drawing:
		DrawSprites draw(*eyes, view_min, view_max, drawable_size, DrawSprites::AlignPixelPerfect);
		glm::vec2 u1 = glm::vec2(view_min.x, view_max.y);
		glm::vec2 u2 = grid_to_screen(player.grid_coords);
		float shake_val = 0.0f;

		if (player.is_shaking) {
			shake_val = player.shake_coords.front();
			u2 += glm::vec2(shake_val, 0.0f);
			player.shake_coords.pop_front();
			player.shake_coords.push_back(shake_val);
		}

		glm::vec2 u3;
		draw.draw(*dungeon, u1);
		if (player.look_left) {
			draw.draw(*left_eye_left, u2);
			draw.draw(*right_eye_left, u2);
		} else if(player.look_right){
			draw.draw(*left_eye_right, u2);
			draw.draw(*right_eye_right, u2);
		} else if (player.look_up) {
			draw.draw(*left_eye_up, u2);
			draw.draw(*right_eye_up, u2);
		} else if (player.look_down) {
			draw.draw(*left_eye_down, u2);
			draw.draw(*right_eye_down, u2);
		} else {
			draw.draw(*left_eye_right, u2);
			draw.draw(*right_eye_left, u2);
		}

		if (flash_obstacle) {
			for (int i = 0; i < obstacles.size(); i++) {
				u3 = grid_to_screen(obstacles[i].grid_coords);
				draw.draw(*flash, u3);
			}
		}
		
	}
	GL_ERRORS(); //did the DrawSprites do something wrong?
}