#ifndef CONSTANTS_H
#define CONSTANTS_H

extern const float SCREEN_WIDTH;
extern const float SCREEN_HEIGHT; 

// mosquito
#define MAX_MOSQUITOES 10 // preprocess max array length
extern const float MOSQUITO_SPAWN_INTERVAL; 
extern const float MOSQUITO_SPAWN_TIMER; 
extern const float MOSQUITO_VELOCITY_X;
extern const float MOSQUITO_WAVE_AMPLITUDE;
extern const float MOSQUITO_WAVE_FREQUENCY;
extern const float MOSQUITO_VELOCITY_DEATH_FALL; 

// wasp
#define MAX_WASPS 10
extern const float WASP_POISON_DURATION; 
extern const float WASP_SPAWN_INTERVAL; 
extern const float WASP_SPAWN_TIMER; 
extern const float WASP_VELOCITY_DEATH_FALL; 
extern const int WASP_RADIUS_MIN; 
extern const int WASP_RADIUS_MAX; 
extern const int WASP_SPIRAL_SPEED_MIN;
extern const int WASP_SPIRAL_SPEED_MAX;
extern const int WASP_CONVERGENCE_MIN; 
extern const int WASP_CONVERGENCE_MAX; 
extern const float WASP_CLOSEST_RADIUS;

// fish
#define MAX_FISH 5
extern const int FISH_MIN_SIZE;
extern const int FISH_MAX_SIZE;
extern const float FISH_ATTACK_DURATION;
extern const float FISH_ATTACK_DAMAGE;

// heart
#define MAX_HEARTS 1

// don't spawn until x time into the game
extern const float HEART_SPAWN_TIMER;

// lilly pads
#define MAX_LILLYPADS 1000
extern const int OFFSCREEN_LILYPAD_SPAWN_AMOUNT;

// froggy
extern const float FROGGY_HEALTH;
extern const float FROGGY_VELOCITY_X;
extern const float FROGGY_JUMP_VELOCITY_Y;
extern const float FROGGY_JUMP_VELOCITY_X;
extern const int FROGGY_BUMP_VELOCITY_Y;
// #define FROGGY_BUMP_VELOCITY_X 380.0
extern const float FROGGY_FALL_VELOCITY; // if this value is changed, check lilypad collision function
extern const float FROGGY_TONGUE_LENGTH;  
extern const float FROGGY_ATTACK_DURATION;
extern const float FROGGY_TONGUE_WIDTH;
extern const float FROGGY_TONGUE_TIMER;
extern const float FROGGY_TONGUE_BUG_PULL_SPEED;
extern const float FROGGY_BUG_SPIT_DISTANCE;
extern const float FROGGY_BUG_SPIT_SPEED;

#define FROGGY_MAX_BUG_SPIT 10
extern const float FROGGY_BUG_SPIT_COOLDOWN;
extern const float FROGGY_BUG_SPIT_TIMER;

// duckhorde
extern const float DUCKHORDE_UPWARD_VELOCITY;
extern const float DUCKHORDE_WAVELENGTH;
extern const float DUCKHORDE_WAVE_AMPLITUDE;

// flamespitters
#define MAX_FLAMESPITTERS 2
extern const float FLAME_SPITTER_SPIT_COOLDOWN;
extern const float FLAME_SPITTER_HEALTH;

// flameprojectiles
#define MAX_FLAMEPROJECTILES 100
extern const float FLAME_PROJECTILE_SPEED;

// proximity volume
extern const float VOLUME_MAX_DISTANCE;
extern const float VOLUME_MIN_DISTANCE;
extern const float MAX_VOLUME;
extern const float MIN_VOLUME;

extern const float EPSILON;

#endif