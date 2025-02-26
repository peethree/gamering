#include "constants.h"

const float SCREEN_WIDTH = 1280.0f;
const float SCREEN_HEIGHT = 800.0f;

// mosquito
const float MOSQUITO_SPAWN_INTERVAL = 0.5f;
const float MOSQUITO_SPAWN_TIMER = 0.0f;
const float MOSQUITO_VELOCITY_X = 400.0f;
const float MOSQUITO_WAVE_AMPLITUDE = 60.0f;
const float MOSQUITO_WAVE_FREQUENCY = 5.0f;
const float MOSQUITO_VELOCITY_DEATH_FALL = 360.0f;

// wasp
const float WASP_POISON_DURATION = 8.0f;
const float WASP_SPAWN_INTERVAL = 5.0f;
const float WASP_SPAWN_TIMER = -5.0f;
const float WASP_VELOCITY_DEATH_FALL = 460.0f;
const int WASP_RADIUS_MIN = 500;
const int WASP_RADIUS_MAX = 700;
const int WASP_SPIRAL_SPEED_MIN = 1;
const int WASP_SPIRAL_SPEED_MAX = 3;
const int WASP_CONVERGENCE_MIN = 12;
const int WASP_CONVERGENCE_MAX = 18;
const float WASP_CLOSEST_RADIUS = 3.0f;

// fish
const int FISH_MIN_SIZE = 1;
const int FISH_MAX_SIZE = 3;
const float FISH_ATTACK_DURATION = 1.4f;
const float FISH_ATTACK_DAMAGE = 0.15f;

// heart
// don't spawn until x time into the game
const float HEART_SPAWN_TIMER = -1.0f;

// lilly pads
const int OFFSCREEN_LILYPAD_SPAWN_AMOUNT = 12;

// froggy
const float FROGGY_HEALTH = 1000000.0f;
const float FROGGY_VELOCITY_X = 200.0f;
const float FROGGY_JUMP_VELOCITY_Y = 1300.0f;
const float FROGGY_JUMP_VELOCITY_X = 380.0f;
const int FROGGY_BUMP_VELOCITY_Y = 825;
// #define FROGGY_BUMP_VELOCITY_X 380.0
const float FROGGY_FALL_VELOCITY = 450.0f; // if this value is changed, check lilypad collision function
const float FROGGY_TONGUE_LENGTH = 480.0f;
const float FROGGY_ATTACK_DURATION = 2.0f;
const float FROGGY_TONGUE_WIDTH = 5.0f;
const float FROGGY_TONGUE_TIMER = 2.0f;
const float FROGGY_TONGUE_BUG_PULL_SPEED = 1100.0f;
const float FROGGY_BUG_SPIT_DISTANCE = 1500.0f;
const float FROGGY_BUG_SPIT_SPEED = 999.0f;
const float FROGGY_BUG_SPIT_COOLDOWN = 0.1f;
const float FROGGY_BUG_SPIT_TIMER = 0.0f;

// duckhorde
const float DUCKHORDE_UPWARD_VELOCITY = 1.0f;
const float DUCKHORDE_WAVELENGTH = 100.0f;
const float DUCKHORDE_WAVE_AMPLITUDE = 4.0f;

// flamespitters
const float FLAME_SPITTER_SPIT_COOLDOWN = 1.0f;
const float FLAME_SPITTER_HEALTH = 100.0f;

// flameprojectiles
const float FLAME_PROJECTILE_SPEED = 900.0f;

// proximity volume
const float VOLUME_MAX_DISTANCE = 10.0f;
const float VOLUME_MIN_DISTANCE = 1.0f;
const float MAX_VOLUME = 1.0f;
const float MIN_VOLUME = 0.0f;

const float EPSILON = 0.0001f;