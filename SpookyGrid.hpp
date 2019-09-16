#include "Sprite.hpp"
#include "Mode.hpp"
#include "Sound.hpp"

#include <deque>

#define SIZEX 10
#define SIZEY 10
#define FLASHON 0.75f
#define FLASHOFF 0.25f
#define SHAKEBY 1.0f

struct Player {

	bool look_left = false; 
	bool look_right = false;
	bool look_up = false; 
	bool look_down = false;
	glm::vec2 grid_coords = glm::vec2(1,1); //Grid space, not screen space.

	bool is_shaking = false; //Should play low sound effect
	std::deque <float> shake_coords;
};

struct Obstacle {

	glm::vec2 grid_coords;
	//Perhaps associated sound effect?
	//Perhaps unique text bubble?
	Obstacle(glm::vec2 grid_coords_) {
		grid_coords = grid_coords_;
	}
};

struct Monster {

	glm::vec2 grid_coords = glm::vec2(10, 10);
	float speed = 1; //How fast the monster moves
	float till_update = 1;
	int current_growl = 1;
	bool random_movement = false;
	std::shared_ptr< Sound::PlayingSample > cur_growl = nullptr;
};

//9x9 dungeon
struct GridMode : Mode{

	GridMode();
	virtual ~GridMode();

	//The usual Mode stuff
	virtual bool handle_event(SDL_Event const&, glm::uvec2 const& window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const& drawable_size) override;

	Player player;
	Monster monster;
	int number_of_obstacles = 10;
	std::vector< Obstacle > obstacles; //Using vectors cause I'm lazy
	float flash_length = FLASHON; //Play flash sound effects
	bool flash_obstacle = true;
	float till_start_time = 4.0f;

	glm::vec2 const size = glm::vec2(SIZEX, SIZEY);
	glm::vec2 const view_min = glm::vec2(0, 0);
	glm::vec2 const view_max = glm::vec2(900, 900);


	//Monster path-finding and movement
	glm::vec2 breadth_first(glm::vec2 source, glm::vec2 target, int* distance);
	void move_monster(); 

	//Obstacle related stuff
	bool generate_obstacles();
	bool is_obstacle(glm::vec2 check);

	//Converts player grid space into pixel space
	glm::vec2 grid_to_screen(glm::vec2 coords);

};