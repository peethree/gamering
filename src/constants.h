#ifndef CONSTANTS_H
#define CONSTANTS_H

// mosquito
#define MAX_MOSQUITOES 20
#define MOSQUITO_SPAWN_INTERVAL 0.5f
#define MOSQUITO_SPAWN_TIMER 0.0f
#define MOSQUITO_VELOCITY_X 400.0f
#define MOSQUITO_WAVE_AMPLITUDE 60.0f
#define MOSQUITO_WAVE_FREQUENCY 5.0f
#define MOSQUITO_VELOCITY_DEATH_FALL 360.0f

// wasp
#define MAX_WASPS 10
#define WASP_SPAWN_INTERVAL 5.0f
#define WASP_SPAWN_TIMER -5.0f
#define WASP_VELOCITY_DEATH_FALL 460.0f

// fish
#define MAX_FISH 10

// heart
#define MAX_HEARTS 1

// lilly pads
#define MAX_LILLYPADS 1000
#define OFFSCREEN_LILYPAD_SPAWN_AMOUNT 12

// froggy
#define FROGGY_HEALTH 1000000.0
#define FROGGY_VELOCITY_X 200.0
#define FROGGY_JUMP_VELOCITY_Y 1300.0
#define FROGGY_JUMP_VELOCITY_X 380.0
#define FROGGY_BUMP_VELOCITY_Y 825
// #define FROGGY_BUMP_VELOCITY_X 380.0
#define FROGGY_FALL_VELOCITY 450.0 // if this value is changed, check lilypad collision
#define FROGGY_TONGUE_LENGTH 480.0f 
#define FROGGY_ATTACK_DURATION 2.0f
#define FROGGY_TONGUE_WIDTH 5.0f
#define FROGGY_TONGUE_TIMER 2.0f
#define FROGGY_TONGUE_BUG_PULL_SPEED 1100.0f
#define FROGGY_BUG_SPIT_DISTANCE 1500.0f
#define FROGGY_BUG_SPIT_SPEED 999.0f
#define FROGGY_MAX_BUG_SPIT 10
#define FROGGY_BUG_SPIT_COOLDOWN 0.1f
#define FROGGY_BUG_SPIT_TIMER 0.0f

// duckhorde
#define DUCKHORDE_UPWARD_VELOCITY 1.0
#define DUCKHORDE_WAVELENGTH 100.0
#define DUCKHORDE_WAVE_AMPLITUDE 4.0

// proximity volume
// TODO: 
#define VOLUME_MAX_DISTANCE 10.0
#define VOLUME_MIN_DISTANCE 1.0
#define MAX_VOLUME 1.0
#define MIN_VOLUME 0.0

#define EPSILON 0.0001f

#endif