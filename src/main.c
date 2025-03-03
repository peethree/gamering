#include "raylib.h"
#include "constants.h"
#include <math.h>
#include "resource_dir.h"	
#include <stdio.h> 

// TODO: 
// make a texture for flame spitter / flame projectile
// make the flame projectile spit cooldown independant per sprite
// add a burning status for the frog similar to poison, draw flame behind the froggy?
// consider adding sound effects for poison/ burning status 
// add sound effect for bug spit, jumping on mosquitoes / wasps, hitbox interaction of spit + wasp, a fading lily pad 
// look for a way to start audio files further in?
// instead of looping over every lilypad for collision checks with frog, only check the ones in range of the frog?
// add an objective to win the game, princess frog way up high? space ship 2 fly away and escape the ducks?
// change the wasp sprite
// delay the tongue swipe speed
// maybe add roguelike powers ?? 
// scoring 
// make landing on lilypads smoother ideally only interact with the pad when falling from above. keep track of y coordinate when jump was initiated?
// use jumpheight to fix lilypad interactions as well as jumping on bug?
// add some kind of menu when the game is over   
// and maybe a menu b4 playing
// add more bug movement patterns
// find a better way to deal with level building
// cool backgrounds, stages that change depending on y value
// stage 1: pond, stage 5: space, astronaut frog ???

// used for flipping textures from left to right
typedef enum Direction {
	LEFT = -1,
	RIGHT = 1,
} Direction;

// used for flipping textures upside down 
typedef enum Status{	
	ALIVE = 1,
	DEAD = -1,
} Status;

typedef struct Frog {
    Texture2D texture;
	Color color;
    Rectangle position;
    Rectangle hitbox;
    Rectangle tongue;
    Rectangle tongueHitbox;    
    Rectangle mouthPosition;	
	Vector2 velocity;
    float tongueTimer;	
	float spitCooldown;
    float attackDuration;
    float tongueAngle;
    float health;
    float jumpHeight;
    float jumpTimer;
    float frameTimer;
    float highestPosition;    
    float spitAngle;
	float tongueProgress;
	float poisonTimer;
	float size;
    int frame;
    int score;
    int bugsEaten;
    Direction direction;
    Status status;	
    bool isAttacking;
    bool isShooting;
    bool isJumping;
    bool isBouncing;
	bool isPoisoned;
	bool isBurning;
	bool dropDown;
} Frog;

typedef struct Bug{	
	Sound sound;
	Texture2D texture;
	Rectangle position;
	Rectangle hitbox;		
	Direction direction;
	Vector2 velocity;
	Vector2 targetPosition;
	Vector2 previousPosition;
	Vector2 spawnPosition;
	Vector2 desiredVelocity;
	char* type;	
	float angle;
	float radius;
	float spiralSpeed;
	float convergence;	
	float minRadius; 	
	float waveFrequency;
	float waveAmplitude;
	float frameTimer;	
	float health;
	float caughtTongueLength;
	int frame;
	int buzzIndex;
	Status status;			
	bool isActive;	
	bool isEaten;	
	bool isBuzzing;
} Bug;

// array used to keep track of which loaded sound is in use
bool buzzInUse[MAX_MOSQUITOES] = { false };

typedef struct Bugspit{
	Texture2D texture;
	Rectangle hitbox;
	Rectangle position;
	Vector2 velocity;
	// Status status;
	float angle;
	bool isActive;		
} Bugspit;

typedef struct Lilypad{
	Texture2D texture;
	Rectangle position;
	Rectangle hitbox;
	float despawnTimer;
	Status status;
	int frame;
	bool isActive;
	bool hasFishTrap;
	bool hasHeart;
	bool hasFlamespitter;
} Lilypad;

typedef struct Fish{
	Texture2D texture;
	Rectangle position;
	Rectangle hitbox;
	float attackDuration;
	float attackTimer;
	float frameTimer;
	Status status;
	Direction direction;
	int frame;
	int size;
	bool isActive;
	bool isAttacking;	
} Fish;

typedef struct Heart{
	Texture2D texture;
	Rectangle position;
	Rectangle hitbox;
	float caughtTongueLength;
	bool isActive;
	bool isEaten;
} Heart;

typedef struct Duckhorde{
	Texture2D texture;
	Rectangle position;
	Rectangle hitbox;
	float velocity;
	float wavetime;
	// what else?
} Duckhorde;

// can this be initialized as a Bug? yes, but do I want to ?
typedef struct Flamespitter{
	Texture2D texture;
	Rectangle position;
	Rectangle mouthPosition;
	Rectangle hitbox;
	// TODO: fields related to flame projectile
	Status status;
	Direction direction;
	float spitCooldown;
	float health;
	bool isActive;
} Flamespitter;

typedef struct Flameprojectile{
	Texture2D texture;
	Rectangle position;	
	Rectangle hitbox;
	Vector2 velocity;
	float angle;
	bool isActive;
	// TODO: aims at the frog
} Flameprojectile;

// initialize flamespitter
void spawn_flamespitter(Flamespitter *flamey, Lilypad *pads, Frog *froggy, int activePads, int *activeFlamespitters, Texture2D flamespitter_texture) {
	// spawn a flamespitter and place it ontop of a lilypad
	int randomLily = GetRandomValue(0, activePads - 1);

	// don't spawn on lilypads that already have a heart spawn.
	if (pads[randomLily].hasHeart) {
		return;
	}
	
	// TODO: don't spawn on lilypad that's offscreen, maybe change lilypad spawn logic?
	// look at heart spawn logic, apply something similar to flamespitters

	flamey->texture = flamespitter_texture;
	flamey->position = pads[randomLily].position;

	// set initial direction
	if (froggy->position.x > flamey->position.x) {
		flamey->direction = RIGHT;
	} else {
		flamey->direction = LEFT;
	}

	flamey->status = ALIVE;
	flamey->isActive = true;
	flamey->spitCooldown = FLAME_SPITTER_SPIT_COOLDOWN;
	flamey->health = FLAME_SPITTER_HEALTH;
	(*activeFlamespitters)++;
}

// update the direction the flamespitter is facing based on position of the frog
void update_flamespitter_direction(Flamespitter *flamey, Frog *froggy) {
	if (froggy->position.x > flamey->position.x) {
		flamey->direction = RIGHT;
	} else {
		flamey->direction = LEFT;
	}
}

// checks to see whether the froggy is in range / sight of the the flamespitter unit
bool target_spotted(Flamespitter *flamey, Camera2D camera) {
	// top-left corner of the camera in world coordinates
    Vector2 camCoords = GetScreenToWorld2D((Vector2){ 0, 0 }, camera);

    Rectangle cameraView = {
        camCoords.x,                
        camCoords.y,               
        1280.0f,                         
        800.0f                           
    };
    return CheckCollisionRecs(flamey->position, cameraView);
}

// shoot a flame projectile at the location of the frog
void shoot_flameprojectile(Flamespitter *flamey, Flameprojectile *projectiles, Frog *froggy, int *activeProjectiles, Texture2D flameprojectile_texture, float frameTime, Camera2D camera) {
	// only start blasting at the frog when he's in view
	if (target_spotted(flamey, camera)) {
		float dx = flamey->position.x - froggy->position.x;
		float dy = flamey->position.y - froggy->position.y;

		// the angle of the projectile
		float angle = atan2f(dy, dx);
		Vector2 direction = { cosf(angle), sinf(angle) };
		
		// decrement spit cooldown
		if (flamey->spitCooldown > 0) {
			flamey->spitCooldown -= frameTime;
		}

		if (flamey->spitCooldown < EPSILON && (*activeProjectiles) < MAX_FLAMEPROJECTILES) {
			// reset cooldown
			flamey->spitCooldown = FLAME_SPITTER_SPIT_COOLDOWN + (float)(GetRandomValue(0,50) / 100.0f);

			// initialize new projectile
			projectiles[*activeProjectiles] = (Flameprojectile){
				.position = flamey->position,
				.hitbox = (Rectangle){
					0.0f,
					0.0f,
					// TODO: consider the actual dimensions of the projectile
					10.0f,
					10.0f
				},
				.isActive = true,
				.velocity = (Vector2){
					direction.x * FLAME_PROJECTILE_SPEED,
					direction.y * FLAME_PROJECTILE_SPEED
				},
				.texture = flameprojectile_texture,
				.angle = angle * RAD2DEG
			};

			// +1 to active projectiles
			(*activeProjectiles)++;	
		}	
	}		
}

// apply velocity flame projectile
void apply_flame_projectile_velocity(Flameprojectile *projectile, float frameTime) {
	projectile->position.x -= projectile->velocity.x * frameTime;
	projectile->position.y -= projectile->velocity.y * frameTime;
}

void flamespitter_death(Flamespitter *flamey) {	
	if (flamey->health <= 0.0f) {
		flamey->status = DEAD;
	}
}

// deactivate dead and offscreen flamespitters
void deactivate_flamespitter(Flamespitter *flamey, Frog *froggy) {
	if (flamey->status == DEAD) {
		flamey->isActive = false;
	}

	// if flamey is too far out of range (below) on y axis
	if (flamey->isActive && (flamey->position.y > froggy->position.y + 1600.0f)) {
		flamey->isActive = false;		
	}
}

// deactivate offscreen projectiles
void deactivate_flameprojectile(Flameprojectile *projectile, Frog *froggy) {
	// too far offscreen on y axis
	if (projectile->isActive && (projectile->position.y > froggy->position.y + 1600.0f || projectile->position.y < froggy->position.y - 1600.0f)) {
		projectile->isActive = false;		
	}

	// too far offscreen on x axis
	if (projectile->isActive && (projectile->position.x > 2000.0f || projectile->position.x < -1000.0f)) {
		projectile->isActive = false;		
	}
}

// frog flamespitter collisions
void collision_check_flamespitter(Flamespitter *flamey, Frog *froggy) {	
	if (CheckCollisionRecs(flamey->hitbox, froggy->hitbox)) {
		// if frog lands a jump on the flamespitter, kill it
		if (froggy->position.y < flamey->position.y && froggy->isJumping) {
			flamey->status = DEAD;
		// else take damage
		} else {
			froggy->health -= 1.0f;
		}
	}	
}

void collision_check_spitties_flamespitter(Bugspit *spitty, Flamespitter *flamey) {
	if (CheckCollisionRecs(spitty->hitbox, flamey->hitbox)) {
		flamey->health -= 50.0f;
	}
}

void hitbox_flamespitter(Flamespitter *flamey) {
	flamey->hitbox = (Rectangle){
		flamey->position.x,
		flamey->position.y,
		100.0f,
		100.0f
	};
}

void hitbox_flameprojectile(Flameprojectile *projectile) {
	projectile->hitbox = (Rectangle){
		projectile->position.x,
		projectile->position.y,
		25.0f,
		25.0f
	};
}

// frog gets hit by flame projectile
void collision_check_flameprojectile(Flameprojectile *projectile, Frog *froggy) {
	if (CheckCollisionRecs(froggy->hitbox, projectile->hitbox)) {
		froggy->health -= 1.5f;
		froggy->isBurning = true;
	}
}

// TODO: take direction into account
// draw flamespitter
void draw_flamespitter(Flamespitter *flamey) {	
	DrawTextureRec(
		flamey->texture,
		(Rectangle){
			0.0f,
			0.0f,
			// flip texture logic
			(float)flamey->texture.width * ((flamey->direction == LEFT) ? -1 : 1),
			(float)flamey->texture.height
		},
		(Vector2){
			flamey->position.x,
			flamey->position.y
		},
		RAYWHITE);
}	

// draw flame projectiles
void draw_flameprojectile(Flameprojectile *projectile) {	
	DrawTexturePro(
		projectile->texture,
		(Rectangle){
			0.0f,
			0.0f,
			(float)projectile->texture.width,
			(float)projectile->texture.height
		},
		(Rectangle){				
			projectile->position.x,
			projectile->position.y,
			(float)projectile->texture.width,
			(float)projectile->texture.height
		},
		(Vector2){ 0.0f, 0.0f},
		// TODO: make sure this is in degrees
		projectile->angle,
		RAYWHITE
	);
}

// flame projectile lights the frog on fire
void burning_froggy(Frog *froggy) {
	//
}

// gravity frog
void apply_gravity(Frog *froggy) {	
	froggy->velocity.y += 36.0f;
	if (froggy->velocity.y > FROGGY_FALL_VELOCITY) {
		froggy->velocity.y = FROGGY_FALL_VELOCITY;
	}
}

// change froggy's color based on its status
void frog_color(Frog *froggy, float frameTime) {	
	switch (froggy->status) {
	case ALIVE:
		froggy->color = RAYWHITE;
		break;
	case DEAD:
		froggy->color = RED;		
		break;
	default:
		froggy->color = RAYWHITE;
		break;
	}

	if (froggy->isPoisoned) {
		float changeTimer = 1.0f;			
		
		froggy->poisonTimer += frameTime;			

		// every modulo of changeTimer, change color n apply damage taken
		if (fmod(froggy->poisonTimer, changeTimer * 2) < changeTimer) {
            froggy->color = DARKGREEN;
			froggy->health -= 0.5f;
        } else {
            froggy->color = RAYWHITE;
        }					
	}
}

// Wasp has poisoned the frog 8-(
void frog_poisoned(Frog *froggy, float frameTime) {
	float poisonDuration = WASP_POISON_DURATION;

	if (froggy->isPoisoned) {
		froggy->poisonTimer += frameTime;

		if (froggy->poisonTimer >= poisonDuration) {
			froggy->isPoisoned = false;
			froggy->poisonTimer = 0.0f;
		}
	}
}

// loops
// void jump_animation(Frog *froggy, int maxFrames, float frameTime, float frameDuration) {
// 	if (froggy->isJumping) {			
// 		froggy->frameTimer += frameTime;
		
// 		if (froggy->frameTimer >= frameDuration) {
// 			froggy->frameTimer = 0.0f;
// 			froggy->frame++;
// 			// cycling frames, result being jump animation
// 			froggy->frame %= maxFrames; 
// 		}	
// 	}

// 	// countdown jumptimer to 0
// 	froggy->jumpTimer -= frameTime;

// 	if (froggy->jumpTimer <= 0.0f) {
// 		froggy->jumpTimer = 0.0f; 		
// 		froggy->isJumping = false;
// 		// resting frog texture
// 		froggy->frame = 0;       
// 	}
// }

// TODO: currently keeps looping while froggy is freefalling FIX 
// single animation (no loop)
void jump_animation(Frog *froggy, int maxFrames, float frameTime, float frameDuration) {
    if (froggy->isJumping) {
        froggy->frameTimer += frameTime;
        
        if (froggy->frameTimer >= frameDuration) {
            froggy->frameTimer = 0.0f;
			// update frames for animation
            froggy->frame++;
            
            // upon reaching last frame, resting frame
            if (froggy->frame >= maxFrames) { 
                froggy->frame = 0;  
            }
        }
    }
}

// frog is at the starting position and doesn't land on a lilypad
void frog_reset_jumpstatus(Frog *froggy) {	
	if (froggy->position.y >= (float)GetScreenHeight() - 50.0f) froggy->isJumping = false;
}

// smaller jump
void frog_baby_jump(Frog *froggy, int maxFrames, float frameTime, Sound frog_leap) {	  
	const float jumpDuration = 1.1f; 
	const float frameDuration = jumpDuration / 6; 

	// froggy->velocity.x = 0.0f;
	if (froggy->status == ALIVE) {
		if (IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_SPACE) && !froggy->isJumping) {
			SetSoundPitch(frog_leap, 1.0f);
			SetSoundVolume(frog_leap, 0.3f);
			PlaySound(frog_leap);			
			froggy->velocity.y = -FROGGY_JUMP_VELOCITY_Y * 0.7f;		
			froggy->isJumping = true;
			froggy->jumpTimer = jumpDuration;	
			froggy->jumpHeight = froggy->position.y;					

			// start at second/third frame for more believable jump animation
			froggy->frame = 2;          
			froggy->frameTimer = 0.0f;  
			jump_animation(froggy, maxFrames, frameTime, frameDuration);
		}	
	}			
}

void frog_big_jump(Frog *froggy, int maxFrames, float frameTime, Sound frog_leap) {
	const float jumpDuration = 1.4f; 
	const float frameDuration = jumpDuration / 6;  	 

	if (froggy->status == ALIVE) {
		if (IsKeyPressed(KEY_SPACE) && !froggy->isJumping && !IsKeyPressed(KEY_LEFT_SHIFT)) {	
			SetSoundPitch(frog_leap, 0.90f);
			SetSoundVolume(frog_leap, 0.3f);
			PlaySound(frog_leap);		
			froggy->velocity.y = -FROGGY_JUMP_VELOCITY_Y;		
			froggy->isJumping = true;
			froggy->jumpTimer = jumpDuration;	
			froggy->jumpHeight = froggy->position.y;		

			// start at second/third frame for more believable jump animation
			froggy->frame = 2;          
			froggy->frameTimer = 0.0f;  			
		}	
	}
	jump_animation(froggy, maxFrames, frameTime, frameDuration);		
}

// movement	frog
void move_frog(Frog *froggy) { 
	froggy->velocity.x = 0.0f;

	// side movement
	if (IsKeyDown(KEY_D)) {
		if (froggy->isJumping) {
			// faster side movement in mid-air
			froggy->velocity.x = FROGGY_JUMP_VELOCITY_X;			
		} else {
			froggy->velocity.x = FROGGY_JUMP_VELOCITY_X;			
		}	
		froggy->direction = RIGHT;
					
	}
	if (IsKeyDown(KEY_A)) {
		if (froggy->isJumping) {
			froggy->velocity.x = -FROGGY_JUMP_VELOCITY_X;
		} else {
			froggy->velocity.x = -FROGGY_JUMP_VELOCITY_X;
		}				
		froggy->direction = LEFT;			
	}
}	

// allow to fall through a lilypad
void move_frog_down(Frog *froggy) {
	// if frog is currently on a lilypad (colliding)
	if (IsKeyDown(KEY_S)) {
		froggy->dropDown = true;
		froggy->velocity.y = FROGGY_FALL_VELOCITY;
	} else {
		froggy->dropDown = false;	
	}	
}

// allow movement from left side of the screen to right side 
void screen_flip(Frog *froggy) {	
	// 8 pixel grids / frames in the texture image -> divide by 8
	if (froggy->position.x < 0.0f - froggy->texture.width / 8) {
		froggy->position.x = (float)GetScreenWidth();
	} else if (froggy->position.x > (float)GetScreenWidth() + froggy->texture.width / 8) {
		froggy->position.x = 0.0f;
	}
}

// update frog hitbox based on position
void hitbox_frog(Frog *froggy) {		
	froggy->hitbox = (Rectangle){
		.x = froggy->position.x - 20.0f,
		.y = froggy->position.y - 20.0f,
		.width = 35.0f,
		.height = 35.0f
	};	
}
 
// tongue lash
void tongue_attack(Frog *froggy, float angle, float frameTime, Vector2 cameraMousePosition, Sound tongue_slurp) {	
	if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !froggy->isAttacking && !froggy->isShooting) {
		// this sound sucks
		SetSoundPitch(tongue_slurp, 1.0f);
		PlaySound(tongue_slurp);
        froggy->isAttacking = true;
        froggy->tongueTimer = 0.0f;   		
		froggy->direction = (cameraMousePosition.x < froggy->mouthPosition.x) ? LEFT : RIGHT;     
    }	

	if (froggy->isAttacking) {
        froggy->tongueTimer += frameTime;

		// attack animation
        // tongue extends
        if (froggy->tongueTimer <= froggy->attackDuration / 2) {
            froggy->tongueProgress = froggy->tongueTimer / (froggy->attackDuration / 2);
        }
        // tongue retracts
        else {
            froggy->tongueProgress = 1.0f - ((froggy->tongueTimer - froggy->attackDuration / 2) / (froggy->attackDuration / 2));
		}

        // tongue length calculation
        float currentLength = FROGGY_TONGUE_LENGTH * froggy->tongueProgress;

		float cosAngle = cosf(angle);
		float sinAngle = sinf(angle);
        
        // tongue endpoint
        float tongueEndX = froggy->mouthPosition.x + cosAngle * currentLength;
        float tongueEndY = froggy->mouthPosition.y + sinAngle * currentLength;
        
        // TODO: this spazzes out the frog when aiming above it
		// turn frog in direction of the mouseclick, added deadzone	        		

		// tongue rectangle for drawing
        froggy->tongue = (Rectangle){
            froggy->mouthPosition.x,
            froggy->mouthPosition.y,
            currentLength,
            FROGGY_TONGUE_WIDTH
        };
        
        // angle needed for drawing
        froggy->tongueAngle = angle * RAD2DEG;          
		    
		float halfWidth = FROGGY_TONGUE_WIDTH / 2;

		// corners of the froggy tongue hitbox      
		Vector2 corners[4];   
        // offset vectors for the four corners
        Vector2 widthOffset = {
            halfWidth * sinAngle,
            -halfWidth * cosAngle
        };

        // rec coords closest to froggy
        corners[0] = (Vector2){ 
            froggy->mouthPosition.x - widthOffset.x, 
            froggy->mouthPosition.y - widthOffset.y 
        };
        corners[1] = (Vector2){ 
            froggy->mouthPosition.x + widthOffset.x, 
            froggy->mouthPosition.y + widthOffset.y 
        };
		// rec coords furthest away from froggy
        corners[2] = (Vector2){ 
            tongueEndX + widthOffset.x, 
            tongueEndY + widthOffset.y 
        };
        corners[3] = (Vector2){ 
            tongueEndX - widthOffset.x, 
            tongueEndY - widthOffset.y 
        };

        // bounding box of the corners
        float minX = corners[0].x, maxX = corners[0].x;
        float minY = corners[0].y, maxY = corners[0].y;
        for (int i = 1; i < 4; i++) {
            minX = fminf(minX, corners[i].x);
            maxX = fmaxf(maxX, corners[i].x);
            minY = fminf(minY, corners[i].y);
            maxY = fmaxf(maxY, corners[i].y);
        }

        // hitbox
        froggy->tongueHitbox = (Rectangle){
            minX,
            minY,
            maxX - minX,
            maxY - minY
        };

        // reset hitbox when attack is over.
		// was killing bugs at 0,0 coords before when initialized at those coords 
		// might not be necessary ?
        if (froggy->tongueTimer >= froggy->attackDuration) {
            froggy->isAttacking = false;
            froggy->tongue = (Rectangle){ 800.0f, 1280.0f, 0.0f, 0.0f };
            froggy->tongueHitbox = (Rectangle){ 800.0f, 1280.0f, 0.0f, 0.0f };
            froggy->tongueAngle = 0.0f;
        }
	}
}

// for the first half of the tongue attack, move the mosquitoes with the tongue.
void move_caught_bug(Bug *bug, Frog *froggy, float frameTime) {
    if (bug->isEaten && froggy->tongueTimer <= FROGGY_TONGUE_TIMER / 2) {        
        float angle = froggy->tongueAngle * DEG2RAD;

        bug->position.x = froggy->mouthPosition.x + cosf(angle) * bug->caughtTongueLength;
        bug->position.y = froggy->mouthPosition.y + sinf(angle) * bug->caughtTongueLength;
    }
}

// TODO: consider making this a generic function
void move_caught_heart(Heart *hearty, Frog *froggy, float frameTime) {
	if (hearty->isEaten && froggy->tongueTimer <= FROGGY_TONGUE_TIMER / 2) {        
        float angle = froggy->tongueAngle * DEG2RAD;

        hearty->position.x = froggy->mouthPosition.x + cosf(angle) * hearty->caughtTongueLength;
        hearty->position.y = froggy->mouthPosition.y + sinf(angle) * hearty->caughtTongueLength;
    }
}

// use the eaten bugs as ammo to kill wasps
void spit_bug(Frog *froggy, float angle, Vector2 cameraMousePosition, Texture2D bugspit_texture, int *activeSpit, Bugspit *spitties, float frameTime) {	
	if (froggy->bugsEaten > 0 && *activeSpit < FROGGY_MAX_BUG_SPIT && froggy->status == ALIVE) {

		if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && !froggy->isAttacking && !froggy->isShooting) {
			froggy->isShooting = true;		
		}
						
		if (froggy->isShooting) {
			froggy->spitCooldown -= frameTime;	
			
			if (froggy->spitCooldown <= EPSILON) {

				// swap direction based on mouse cursor position compared to the frog	
				froggy->direction = (cameraMousePosition.x < froggy->mouthPosition.x + 10.0f) ? LEFT : RIGHT;

				// direction x, y
				Vector2 spitDirection = {
					cosf(angle),
					sinf(angle)
				};			

				// initialize new bug spit at activespit index
				// TODO: simplify this?
				spitties[*activeSpit] = (Bugspit){
					.position = froggy->mouthPosition,
					.texture = bugspit_texture,
					.isActive = true,
					.velocity = (Vector2){
						spitDirection.x * FROGGY_BUG_SPIT_SPEED,
						spitDirection.y * FROGGY_BUG_SPIT_SPEED
					},
					.angle = angle * RAD2DEG
				};	

				// increment amount active
				(*activeSpit)++;	
				// decrement bugs eaten (ammo)
				froggy->bugsEaten--;
				froggy->spitCooldown = FROGGY_BUG_SPIT_COOLDOWN;	
				froggy->isShooting = false;					
			}			
		}
	} 
}

// update spit hitbox
void hitbox_spit(Bugspit *spitty) {
	spitty->hitbox = (Rectangle){			
		.x = spitty->position.x + 13.0f,
		.y = spitty->position.y + 1.0f,
		.width = 30.0f,
		.height = 30.0f
	};	
}

void collision_check_spit(Bugspit *spitty, Bug *bug) {
	if (CheckCollisionRecs(spitty->hitbox, bug->hitbox)) {
		// TODO: figure out a value that doesn't 1 shot.
		if (bug->type == "wasp") {
			bug->health -= 10.0f;
		// oneshot
		} else if (bug->type = "mosquito") {
			bug->health -= 1.0f;
		} 	
	}
}

void draw_spit(Bugspit *spitty) {
	DrawTexture(
		spitty->texture, 
		spitty->position.x, 
		spitty->position.y, 
		RAYWHITE
	);

	// debug spit hitbox
    // DrawRectanglePro(
	// 	spitty->hitbox,
	// 	(Vector2){ 0.0f, 0.0f},
	// 	spitty->angle,
	// 	RAYWHITE
	// );
}

// apply velocity to spit projectile
void apply_spit_velocity(Bugspit *spitty, float frameTime) {
	spitty->position.x += spitty->velocity.x * frameTime;
	spitty->position.y += spitty->velocity.y * frameTime;
}

// deactivate offscreen spits
void deactivate_spit(Bugspit *spitty, Frog *froggy) {
	// 	clean up spits at y distance difference
	if (spitty->isActive && (spitty->position.y > froggy->position.y + 1600.0f || spitty->position.y < froggy->position.y - 1600.0f)) {
		spitty->isActive = false;		
	}

	if (spitty->isActive && (spitty->position.x > 2000.0f || spitty->position.x < -1000.0f)) {
		spitty->isActive = false;		
	}
}

// keep track of where on the sprite the tongue should appear from
void frog_mouth_position(Frog *froggy) {
	if (froggy->direction == RIGHT) {				   //offsets to spawn tongue closer to mouth
		froggy->mouthPosition.x = froggy->position.x + (7.0f * froggy->size * 1.9f);
		froggy->mouthPosition.y = froggy->position.y - (4.0f * froggy->size * 1.9f);
	} else {
		froggy->mouthPosition.x = froggy->position.x - (7.0f * froggy->size * 1.9f);
		froggy->mouthPosition.y = froggy->position.y - (4.0f * froggy->size * 1.9f);				
	}
}
 
void frog_attack_params(Frog *froggy, float frameTime, Camera2D camera, Texture2D bugspit_texture, int *activeSpit, Bugspit *spitties, Sound tongue_slurp) {	
	// despawn tongue when froggy dies
	if (froggy->status == DEAD) {
		froggy->isAttacking = false;
		froggy->isShooting = false;
	}

	// keep track of cursor coordinates, translate them into camera coords for hitbox logic
	Vector2 cursorPosition = GetMousePosition();
	Vector2 cameraMousePosition = GetScreenToWorld2D(cursorPosition, camera);

	// direction vectors
	float dx = cameraMousePosition.x - froggy->mouthPosition.x;
	float dy = cameraMousePosition.y - froggy->mouthPosition.y;

	// angle --> tan⁻¹(dy/dx)
	float angle = atan2f(dy, dx);

	tongue_attack(froggy, angle, frameTime, cameraMousePosition, tongue_slurp);	
	spit_bug(froggy, angle, cameraMousePosition, bugspit_texture, activeSpit, spitties, frameTime);
}

void draw_tongue(Frog *froggy) {
	if (froggy->isAttacking) {
        DrawRectanglePro(
			froggy->tongue,
			(Vector2){0.0f, FROGGY_TONGUE_WIDTH / 2},		
			froggy->tongueAngle,
			RED
		);		
		// DrawRectanglePro(
		// 	froggy->tongueHitbox,
		// 	(Vector2){ 0.0f, FROGGY_TONGUE_WIDTH },
		// 	froggy->tongueAngle,
		// 	WHITE
		// );
	}
}

void frog_prevent_overheal(Frog *froggy) {
	if (froggy->health > FROGGY_HEALTH) {
		froggy->health = FROGGY_HEALTH;
	}
}

// movement mosquito
void move_mosquito(Bug *mosquito, float frameTime) {	
	
	if (mosquito->status == ALIVE && mosquito->isEaten == false) {
		mosquito->waveAmplitude = MOSQUITO_WAVE_AMPLITUDE; 
		mosquito->waveFrequency = MOSQUITO_WAVE_FREQUENCY; 
		mosquito->velocity.x = MOSQUITO_VELOCITY_X; 
		mosquito->angle += frameTime;	

		// update position
		mosquito->position.x += (float)mosquito->direction * mosquito->velocity.x * frameTime;
		mosquito->position.y = mosquito->spawnPosition.y + mosquito->waveAmplitude * sinf(mosquito->waveFrequency * mosquito->angle);
		// don't update position anymore in this func when bug is eaten	
		
	// bug should fall off the screen in case of jump death
	} else {			
		mosquito->velocity.y = MOSQUITO_VELOCITY_DEATH_FALL; 
		mosquito->position.y += mosquito->velocity.y * frameTime;	
	}
}

// TODO: consider adding another wasp frame that's attempting to grab the froggy once
// the wasp reaches a certain distance away from the froggy.
void animate_wasp(Bug *wasp, float frameTime, int maxFramesWasp) {
	// flying animation for wasps
	float frameDuration = 0.1f;
	wasp->frameTimer += frameTime;

	if (wasp->frameTimer >= frameDuration) {
		wasp->frameTimer = 0.0f;
		wasp->frame++;
		wasp->frame %= maxFramesWasp;
	}
}

// TODO: maybe invert the spiral so the wasp doesn't auto kill itself
// movement wasp
void move_wasp(Bug *wasp, Frog *froggy, float frameTime) {	

	if (wasp->status == ALIVE) {
		// previous position for frame flipping
		wasp->previousPosition.x = wasp->position.x;

		// update angle
		wasp->angle += wasp->spiralSpeed * frameTime;

		// decrease radius from froggy
		if (wasp->radius > wasp->minRadius) {
			wasp->radius -= wasp->convergence * frameTime;
			// increase the convergence over time
			wasp->convergence += 0.1f;
		}

		// convert radius and angle into x, y coordinates
		float wasp_x = wasp->radius * cosf(wasp->angle); 
		float wasp_y = wasp->radius * sinf(wasp->angle);	

		// set the target position to chase
		wasp->targetPosition.x = froggy->position.x + wasp_x; 
		wasp->targetPosition.y = froggy->position.y + wasp_y;

		// modify how fast the wasp wants to reach the froggy
		wasp->desiredVelocity.x = (wasp->targetPosition.x - wasp->position.x) * 4.6f,
		wasp->desiredVelocity.y = (wasp->targetPosition.y - wasp->position.y) * 4.6f;

		// gradually increase velocity
		wasp->velocity.x += (wasp->desiredVelocity.x - wasp->velocity.x) * 0.2f;
		wasp->velocity.y += (wasp->desiredVelocity.y - wasp->velocity.y) * 0.2f;

		// apply velocity with delay modifier from previous block
		wasp->position.x += wasp->velocity.x * frameTime;
		wasp->position.y += wasp->velocity.y * frameTime;	
		
		// if horizontal movement is +x flip frame right, if -x flip frame pointing left
		float dif = wasp->position.x - wasp->previousPosition.x;

		if (dif > 0.0f) {
			wasp->direction = LEFT;
		} else {
			wasp->direction = RIGHT;
		}
	// bug should fall off the screen in case of jump death
	} else {			
		wasp->velocity.y = WASP_VELOCITY_DEATH_FALL; 
		wasp->position.y += wasp->velocity.y * frameTime;	
		wasp->status = DEAD;
		froggy->score += 5;
	}	
}

// velocity frog
void apply_velocity(Frog *froggy, float frameTime) {	
	froggy->position.x += froggy->velocity.x * frameTime;
	froggy->position.y += froggy->velocity.y * frameTime;
}	

// clean up bugs too far away from the froggy 
void deactivate_bug(Bug *bug, Frog *froggy) {	
	// deactivate bug at y distance difference			
	if (bug->isActive && (bug->position.y > froggy->position.y + 1000.0f)) {
		bug->isActive = false;
		bug->isBuzzing = false;
	}

	// and x difference (these numbers need to be further than where the bugs spawn)
	if (bug->isActive && ((bug->direction == RIGHT && bug->position.x > 1500.0f) || (bug->direction == LEFT && bug->position.x < -900.0f))) {
		bug->isActive = false;
		bug->isBuzzing = false;
	}	
}

// helper function for bug death animation after getting jumped on
void bug_spit_death(Bug *bug, float frameTime, Frog *froggy) {
	if (bug->status == DEAD) {
		if (bug->type == "wasp") {
			bug->velocity.y = WASP_VELOCITY_DEATH_FALL; 
			froggy->score += 5;
		} else if (bug->type == "mosquito") {
			bug->velocity.y = MOSQUITO_VELOCITY_DEATH_FALL; 
			froggy->score++;
		}		
		bug->position.y += bug->velocity.y * frameTime;
	}
}

// you will eat the bugs
void eat_bug(Frog *froggy, Bug *bug, float frameTime) {
    // start pulling the bug toward the froggy when the tongue starts retracting
    if (froggy->status == ALIVE && bug->isEaten && froggy->tongueTimer >= FROGGY_TONGUE_TIMER / 2) { 
		// update length based on retraction progress
		float currentLength = bug->caughtTongueLength * froggy->tongueProgress;
		float angle = froggy->tongueAngle * DEG2RAD;

		// update bug position		
        bug->position.x = froggy->mouthPosition.x + cosf(angle) * currentLength;
        bug->position.y = froggy->mouthPosition.y + sinf(angle) * currentLength;		

		// force bug collision at end of tongue retraction   
		if (froggy->tongueProgress < EPSILON) {
			bug->hitbox = (Rectangle){
				froggy->hitbox.x,
				froggy->hitbox.y,
				300.0f,
				300.0f
			};
		}

		// increase in size from eating bug
		if (CheckCollisionRecs(froggy->hitbox, bug->hitbox) || CheckCollisionRecs(froggy->mouthPosition, bug->hitbox)) {
			froggy->size += 0.10f;			
			bug->isActive = false;
			bug->status = DEAD;
			bug->isBuzzing = false;
			froggy->bugsEaten++;
		}	
    }
}

// TODO: make either this or the other eat function generic
void froggy_eat_heart(Frog *froggy, Heart *hearty, float frameTime) {
	// drag the heart over to the frog by the tongue
	if (hearty->isEaten && froggy->tongueTimer >= FROGGY_TONGUE_TIMER / 2) { 

		// update length based on retraction progress
		float currentLength = hearty->caughtTongueLength * froggy->tongueProgress;
		float angle = froggy->tongueAngle * DEG2RAD;

		// update hearty position		
		hearty->position.x = froggy->mouthPosition.x + cosf(angle) * currentLength;
		hearty->position.y = froggy->mouthPosition.y + sinf(angle) * currentLength;		

		// force hearty collision at end of tongue retraction   
		if (froggy->tongueProgress < EPSILON) {
			hearty->hitbox = (Rectangle){
				froggy->hitbox.x,
				froggy->hitbox.y,
				300.0f,
				300.0f
			};
		}	
	}
}

// TODO: maybe split these into 2, 1 for mosquito and 1 for wasps
void hitbox_bug(Bug *bug) {
	// mosquito hitbox
	if (bug->type == "mosquito") {	
		bug->hitbox = (Rectangle){
			.x = bug->position.x + 13.0f,
			.y = bug->position.y + 1.0f,
			.width = 50.0f,
			.height = 50.0f
		};
	}

	// wasp hitbox
	if (bug->type == "wasp") {		
		bug->hitbox = (Rectangle) {
			.x = bug->position.x + 10.0f,
			.y = bug->position.y + 1.0f,
			.width = 100.0f,
			.height = 100.0f
		};
	}
}

void collision_check_bugs(Frog *froggy, Bug *bug, float frameTime) {
	// when the tongue hits a mosquito, eat it
	if (CheckCollisionRecs(froggy->tongueHitbox, bug->hitbox) && bug->type == "mosquito" && !bug->isEaten) {
		bug->isEaten = true;
		// set current length where mosquito got caught
		bug->caughtTongueLength = FROGGY_TONGUE_LENGTH * (froggy->tongueTimer / (froggy->attackDuration / 2));
		// update score
		froggy->score++;
	}	
	
	// TODO: this is a mess
	// froggy bug collision
	if (CheckCollisionRecs(froggy->hitbox, bug->hitbox) && bug->status == ALIVE && froggy->status == ALIVE && !bug->isEaten) {
		// froggy is hitting the bug from the bottom
		if (froggy->position.y > bug->position.y) {		
			if (froggy->health >= 0.0) {
				if (bug->type == "mosquito") froggy->health -= 2.0f;					
				if (bug->type == "wasp") {
					froggy->health -= 1.0f;
					froggy->isPoisoned = true;
				}
			}	
			// bump froggy down when trying to jump through a bug
			// TODO: instead of bump down, slow velocity
			// froggy->velocity.y *= 0.50;

			// TODO: horizontal bumps do not work
			// froggy is further left than bug
			// if (froggy->position.x < bug->position.x) {
			// 	froggy->velocity.x = -FROGGY_BUMP_VELOCITY_X;
			// } 

			// // froggy is further right
			// if (froggy->position.x > bug->position.x) {
			// 	froggy->velocity.x = FROGGY_BUMP_VELOCITY_X;
			// } 

		// froggy is on TOP wew!!!!	
		} else if (froggy->position.y < bug->position.y && (froggy->isJumping || froggy->velocity.y <= FROGGY_FALL_VELOCITY)) {
			// allow the frog to bounce off bug for a boost and kill the bug						
			if (bug->status == ALIVE) {			
				froggy->velocity.y = -FROGGY_JUMP_VELOCITY_Y * 0.65f;	
				bug->status = DEAD;		
				froggy->score++;		
			}
		} 
	}	
}

// frog dies when health goes to 0 . . . .	
void froggy_death(Frog *froggy) {	
	if (froggy->health <= 0.0f) {
		froggy->velocity.x = 0.0f;
		froggy->health = 0.0f;
		froggy->status = DEAD;	
		froggy->frame = 5;		
	}
}

void bug_death(Bug *bug) {
	if (bug->health <= 0.0f) {		
		bug->health = 0.0f;
		bug->status = DEAD;				
	}
}

// collision with lilypads
void collision_check_pads(Frog *froggy, Lilypad *pad) {	
	// lilypad hitbox(es)
	switch (pad->frame) {
		case 0:
			pad->hitbox = (Rectangle){
				.x = pad->position.x + 5.0f,
				.y = pad->position.y + 15.0f,
				.width = 90.0f,
				.height = 3.0f
			};
			break;
		case 1:
			pad->hitbox = (Rectangle){
				.x = pad->position.x + 5.0f,
				.y = pad->position.y + 15.0f,
				.width = 85.0f,
				.height = 3.0f
			};
			break;
		case 2:
			pad->hitbox = (Rectangle){
				.x = pad->position.x + 5.0f,
				.y = pad->position.y + 15.0f,
				.width = 60.0f,
				.height = 3.0f
			};
			break;
		case 3:
			pad->hitbox = (Rectangle){
				.x = pad->position.x + 0.0f,
				.y = pad->position.y + 15.0f,
				.width = 50.0f,
				.height = 3.0f
			};
			break;
		default:
			break;
	}

    //TODO: frog lilypad collision is REAL nasty atm, look INTO it

	// froggy is below the pad
	if (froggy->position.y > pad->position.y && pad->isActive) {
		if (CheckCollisionRecs(froggy->hitbox, pad->hitbox) && !froggy->dropDown) {									
			froggy->position.y += (pad->position.y - froggy->position.y) * 0.8f;	

			// froggy is not moving up or down vertically (or affected by max gravity)
			if (froggy->velocity.y == FROGGY_FALL_VELOCITY && froggy->status == ALIVE) {
				froggy->frame = 1;
				froggy->isJumping = false;	
			}
		}
	}
}

// fill up the buzzes array with usable buzzes
void create_mosquito_buzz_instance(const char *sound_path, Sound buzzes[], int *activeBuzzes) {	
	buzzes[*activeBuzzes] = LoadSound(sound_path); 
	(*activeBuzzes)++;  
}

// free the sound and set in use to false so it can be reused by a freshly spawned mosquito.
void free_buzz(Bug *bug, Sound buzzes[]) {
    if (bug->status == DEAD || !bug->isActive) {
        if (bug->buzzIndex >= 0 && bug->buzzIndex < MAX_MOSQUITOES) {
            StopSound(buzzes[bug->buzzIndex]);  
            buzzInUse[bug->buzzIndex] = false;  
            bug->buzzIndex = -1;  
        }
    }
}

// proximity based buzzing
void buzz_volume_control(Bug *bug, Frog *froggy, float maxDistance) {
	// euclidian distance between frog n bug
	float distance = sqrt(pow(bug->position.x - froggy->position.x, 2) + pow(bug->position.y - froggy->position.y, 2));

	// only play sounds when the mosquito is in field of view. (-50, 1280)
	if (bug->position.x > -50.0f && bug->position.x < SCREEN_WIDTH) {		
		// normalize distance * magic number for enhanced effect
		float normDist = 1.0f - (distance / maxDistance * 2.5); 

		// set volume based on distance
		float volume = normDist;

		// max = 1.0, min 0
		if (volume > MAX_VOLUME) volume = MAX_VOLUME;		
		if (volume < MIN_VOLUME) volume = MIN_VOLUME;	

		// change the pitch and volume when bug is caught on the tongue
		if (bug->isEaten) {
			volume = 0.9f;
			SetSoundPitch(bug->sound, 0.9f);
		}		
		SetSoundVolume(bug->sound, volume);	
	// offscreen volume
	} else {
		SetSoundVolume(bug->sound, 0.0f);
	}
}

// if bug is to the left of froggy, pan sound left and vice versa
void volume_panning(Bug *bug, Frog *froggy) {	
		float pan = (bug->position.x - froggy->position.x) / SCREEN_WIDTH;
		// normalize pan 
		pan = (pan + 1.0f) / 2.0f;
		SetSoundPan(bug->sound, pan);
}

// checks if a buzz sound is already in use
int get_buzz_index() {
    for (int i = 0; i < MAX_MOSQUITOES; i++) {
        if (!buzzInUse[i]) {  
            buzzInUse[i] = true;
            return i;
        }
    }
	// no available buzz index slot
    return -1; 
}

// initialize mosquito(es)
void spawn_mosquito(Bug *mosquito, Frog *froggy, Texture2D mosquito_texture, Sound buzzes[]) {		
	// buzz related
	mosquito->buzzIndex = get_buzz_index();
	if (mosquito->buzzIndex >= 0 && mosquito->buzzIndex < MAX_MOSQUITOES) {
		mosquito->sound = buzzes[mosquito->buzzIndex];
		SetSoundVolume(mosquito->sound, 0.0f);	
		PlaySound(mosquito->sound);
	}		

    mosquito->texture = mosquito_texture;
	mosquito->position = (Rectangle){	
		// either spawn on the left or the right side just past the window's edge	
		640.0f + (GetRandomValue(0, 1) == 0 ? -1440.0f : 800.0f),
		froggy->position.y + GetRandomValue(-500, -400), 
		0.0f, 
		0.0f
	};	

	mosquito->spawnPosition = (Vector2){ mosquito->position.x, mosquito->position.y };
    if (mosquito->spawnPosition.x < SCREEN_WIDTH / 2) { 
        mosquito->direction = RIGHT;          
    } else {
        mosquito->direction = LEFT;         
    }
    mosquito->frame = 0;
	mosquito->status = ALIVE;
	mosquito->isActive = true;
	mosquito->type = "mosquito";
	mosquito->isEaten = false;
	mosquito->health = 1.0f;	
	mosquito->isBuzzing = true;	
}

// initialize wasps
void spawn_wasp(Bug *wasp, Frog *froggy, Texture2D wasp_texture) {		
    wasp->texture = wasp_texture;
    wasp->position = (Rectangle){
		froggy->position.x + (float)GetRandomValue(-250, 250), 
		froggy->position.y + (float)GetRandomValue(200, 300), 
		0.0f, 
		0.0f
	};

	wasp->spawnPosition = (Vector2){ wasp->position.x, wasp->position.y };
    wasp->angle = 0.0f;
    wasp->radius = (float)GetRandomValue(WASP_RADIUS_MIN, WASP_RADIUS_MAX);	
	// random direction
	int randomDirection = GetRandomValue(0,1);
	wasp->direction = (randomDirection == 0) ? LEFT : RIGHT;
    wasp->spiralSpeed = (float)GetRandomValue(WASP_SPIRAL_SPEED_MIN, WASP_SPIRAL_SPEED_MAX) * wasp->direction;
    wasp->convergence = (float)GetRandomValue(WASP_CONVERGENCE_MIN, WASP_CONVERGENCE_MAX);
    wasp->minRadius = WASP_CLOSEST_RADIUS;	
	wasp->status = ALIVE;
	wasp->isActive = true;
	wasp->type = "wasp";
	wasp->isEaten = false;
	wasp->frameTimer = 0;
	wasp->health = 10.0;
}

// initialize the platforms	
void make_lilypads(Lilypad *pad, Texture2D lilypad_texture, Frog *froggy) {		
	pad->texture = lilypad_texture;
	pad->position = (Rectangle){
		.x = froggy->position.x + (float)GetRandomValue(-700, 600),
		.y = froggy->position.y + (float)GetRandomValue(-1600, 300),
		.width = 0.0f,
		.height = 36.0f,
	};
	pad->frame = GetRandomValue(0,3);
	pad->isActive = true;	
	pad->hasFishTrap = false;
	pad->despawnTimer = 0.0f;
}

void make_lilypads_offscreen(Lilypad *pad, Texture2D lilypad_texture, Frog *froggy) {
	pad->texture = lilypad_texture;
	pad->position = (Rectangle){
		.x = froggy->position.x + (float)GetRandomValue(-600, 600),
		.y = froggy->position.y + (float)GetRandomValue(-800, -1000),
		.width = 0.0f,
		.height = 36.0f,
	};	
	pad->frame = GetRandomValue(0,3);
	pad->isActive = true;
	pad->hasFishTrap = false;
	pad->despawnTimer = 0.0f;
}

void update_froggy_highest_position(Frog *froggy) {
	// get the highest y value visited
	if (froggy->position.y < froggy->highestPosition) {
        froggy->highestPosition = froggy->position.y;
    }
}

void deactivate_lilypads(Lilypad *pad, Frog *froggy) {
	// remove lilypads when froggy has climbed certain distance
    if (pad->isActive && pad->position.y > froggy->highestPosition + 1500.0f) {
        pad->isActive = false;
    }	
}

// remove lilypads when froggy is staying on it too long
void deactivate_lilypad_collision(Lilypad *pad, Frog *froggy, float frameTime) {
	float timer = (float)GetRandomValue(2,4);

	// increment timer when frog makes contact with lilypad
	if (CheckCollisionRecs(pad->hitbox, froggy->hitbox)) {
		pad->despawnTimer += frameTime;
	}	

	// warning sound right before it despawns
	// if (pad->despawnTimer >= (0.75 * timer)) {
	// 	playSound(warning!);
	// }

	// when timer exceeds certain value, deactivate pad
	if (pad->despawnTimer >= timer) {
		pad->isActive = false;		
	}
}

// once fish is active, animate it (once).
void animate_fish(Fish *fishy, float frameTime, int maxFramesFish) {	
	if (fishy->isAttacking) {
		// divided by the number of frames on the sprite sheet
		float frameDuration = FISH_ATTACK_DURATION / 8;
		fishy->frameTimer += frameTime;

		if (fishy->frameTimer >= frameDuration) {
			fishy->frameTimer = 0.0f;
			fishy->frame++;
			fishy->frame %= maxFramesFish;
		}	
	}
}

// fish lays (swims) in ambush at location of the lilypads
void spawn_fish(Fish *fishy, Texture2D fish_texture, Lilypad *pads, int activePads) {
	// pick a random lilypad between 0 and the max active pads for a fish to spawn under
	int randomValue = GetRandomValue(0, activePads - 1);	

	// TODO: i don't like the way it looks when fish spawn too close to eachother, worth looking into
	// don't double spawn fish on lilypads
	if (!pads[randomValue].hasFishTrap) {
		fishy->position = pads[randomValue].position;	
		fishy->isActive = true;
		pads[randomValue].hasFishTrap = true;
	}

	fishy->frameTimer = 0.0f;
	fishy->texture = fish_texture;
	fishy->isAttacking = false;
	fishy->attackDuration = FISH_ATTACK_DURATION;	
	fishy->isActive = true;
	fishy->attackTimer = 0.0f;
	fishy->frame = 0;

	// get random size
	fishy->size = GetRandomValue(FISH_MIN_SIZE, FISH_MAX_SIZE);
	// get random direction
	int randomDirection = GetRandomValue(0,1);
	fishy->direction = (randomDirection == 0) ? RIGHT : LEFT; 	
}

// trigger the fishy ambush by stepping on its initial hitbox 
void activate_fish(Fish *fishy, Frog *froggy, Sound fish_splash) {
	if (!fishy->isAttacking) {
		// fish hitbox 
		fishy->hitbox = (Rectangle){
			.x = fishy->position.x,
			.y = fishy->position.y,
			.width = 60.0f,
			.height = 20.0f
		};	

		// different splash volumes for size fish
		switch (fishy->size) {
		case 1:
			SetSoundVolume(fish_splash, 0.4f);
			break;
		case 2:
			SetSoundVolume(fish_splash, 0.7f);
			break;
		case 3:
			SetSoundVolume(fish_splash, 1.0f);
			break;		
		default:
			break;
		}

		if (CheckCollisionRecs(fishy->hitbox, froggy->hitbox) && fishy->isActive) {
			fishy->isAttacking = true;
			SetSoundVolume(fish_splash, 1.0f);
			PlaySound(fish_splash);
		}
	}
}

// after fish has been activated, change hitbox based on the frame
void attacking_fish_collision(Fish *fishy, Frog *froggy) {
	switch(fishy->frame) {
		case 0:
			fishy->hitbox = (Rectangle){			
				.x = fishy->position.x,
				.y = fishy->position.y,
				.width = 60.0f * fishy->size,
				.height = 20.0f * fishy->size
			};
			break;
		case 1:
			fishy->hitbox = (Rectangle){			
				.x = fishy->position.x,
				.y = fishy->position.y + 20.0f,
				.width = 60.0f * fishy->size,
				.height = 30.0f * fishy->size
			};		
			break;	
		case 2:
			fishy->hitbox = (Rectangle){			
				.x = fishy->position.x,
				.y = fishy->position.y + 15.0f,
				.width = 60.0f * fishy->size,
				.height = 40.0f * fishy->size
			};
			break;
		case 3:
			fishy->hitbox = (Rectangle){			
				.x = fishy->position.x,
				.y = fishy->position.y - 10.0f,
				.width = 60.0f * fishy->size,
				.height = 60.0f * fishy->size
			};
			break;
		case 4:
			fishy->hitbox = (Rectangle){			
				.x = fishy->position.x,
				.y = fishy->position.y - 40.0f,
				.width = 60.0f * fishy->size,
				.height = 80.0f * fishy->size
			};
			break;
		case 5:
			fishy->hitbox = (Rectangle){			
				.x = fishy->position.x,
				.y = fishy->position.y - 30.0f,
				.width = 60.0f * fishy->size,
				.height = 70.0f * fishy->size
			};
			break;	
		case 6:
			fishy->hitbox = (Rectangle){			
				.x = fishy->position.x,
				.y = fishy->position.y - 20.0f,
				.width = 60.0f * fishy->size,
				.height = 40.0f * fishy->size
			};
			break;
		case 7:
			fishy->hitbox = (Rectangle){			
				.x = fishy->position.x,
				.y = fishy->position.y - 20.0f,
				.width = 60.0f * fishy->size,
				.height = 1.0f * fishy->size
			};
			break;
		default:
			break;
	}

	if (fishy->isAttacking && fishy->frame > 1) {
		if (CheckCollisionRecs(fishy->hitbox, froggy->hitbox)) {
			froggy->health -= FISH_ATTACK_DAMAGE * fishy->size;
		}
	}
}

void deactivate_fish(Fish *fishy, Frog *froggy, float frameTime) {	
	// deactivate fish at y distance difference			
	if (fishy->isActive && (fishy->position.y > froggy->position.y + 1000.0f)) {
		fishy->isActive = false;
		fishy->isAttacking = false;
	}

	// deactivate after 1 attack
	if (fishy->isAttacking) {
		fishy->attackTimer += frameTime;
		
		if (fishy->attackTimer >= fishy->attackDuration) {
			fishy->isActive = false;
			fishy->isAttacking = false;
			fishy->attackTimer = 0.0f; 
		}
	}
}

void spawn_healing_heart(Heart *hearty, Texture2D heart_texture, Lilypad *pads, int activePads, Frog *froggy) {
	hearty->texture = heart_texture;

	// hearty->frame = 0;
	// spawn at (somewhat) random lily location 
	int padsAboveFroggy[MAX_LILLYPADS];
	int count = 0;

	for (int i = 0; i < activePads; i++) {
		// (must be ABOVE the frog)
		if (pads[i].position.y < froggy->highestPosition) {
			padsAboveFroggy[count++] = i;
		}
	}

	// choose 1 pad from the pads above froggy array to spawn a heart on
	if (count > 0) {
		// random index based on how many pads were above the frog's heighest reached y value
		int index = GetRandomValue(0, count - 1);
		// using the random index in padsAboveFroggy's array of indices to get a valid pads index that is above the frog's heighest visited y value
		int padsIndex = padsAboveFroggy[index];		
		// set the heart's position
		hearty->position = pads[padsIndex].position;
		pads[padsIndex].hasHeart = true;
		hearty->isActive = true; // set to inactive when froggy interacts with it and uses it
		hearty->isEaten = false;
	}
}

// update heart hitbox
void hitbox_heart(Heart *hearty) {
	hearty->hitbox = (Rectangle){
		hearty->position.x,
		hearty->position.y,
		16.0f,
		16.0f
	};
}

// collision frogtongue + heart
void check_collision_tongue_heart(Frog *froggy, Heart *hearty) {
	if (CheckCollisionRecs(froggy->tongueHitbox, hearty->hitbox) && !hearty->isEaten) {
		hearty->isEaten = true;	
		hearty->caughtTongueLength = FROGGY_TONGUE_LENGTH * (froggy->tongueTimer / (froggy->attackDuration / 2));	
	}
}

void deactivate_heart(Heart *hearty, Frog *froggy) {
	// if the heart is no longer reachable (froggy too far removed)
	if (hearty->isActive && (hearty->position.y > froggy->position.y + 1500.0f)) {
		hearty->isActive = false;		
	}

	// if the frog has interacted with it 
	if (CheckCollisionRecs(froggy->hitbox, hearty->hitbox)  || CheckCollisionRecs(froggy->mouthPosition, hearty->hitbox)) {
		hearty->isActive = false;
		froggy->health += 25.0f;
	}

	// // if the pad it spawned on is deactivated
	// if (!pad->isActive) {
	// 	hearty->isActive = false;
	// }
}

void animate_heart(Heart *hearty, float frameTime) {
	// bounce up and down

	// move y posish smoothly
}

// draw heart
void draw_heart(Heart *hearty) {
	DrawTextureRec(
		hearty->texture,
		(Rectangle){
			0.0f,
			0.0f,
			hearty->texture.width,
			hearty->texture.height
		},
		(Vector2){
			hearty->position.x,
			hearty->position.y
		},
		RAYWHITE
	);
}

// duck movement.
void move_duckhorde(Duckhorde *duckies, Frog *froggy, float frameTime) {	
	if (froggy->status == ALIVE) { 
		duckies->position.y -= (duckies->velocity * (float)GetRandomValue(100, 200)) * frameTime;
	}
}

// when froggy is very far upward, update duckhorde position so they'll be near again. . . 
void update_duckhorde_position(Duckhorde *duckies, Frog *froggy) {
	if (froggy->position.y < duckies->position.y - 1200.0f) {
		duckies->position.y = froggy->position.y + 800.0f;
	}
}

// duck hitbox
void hitbox_duckhorde(Duckhorde *duckies) {
	duckies->hitbox = (Rectangle){
		.x = duckies->position.x,
		.y = duckies->position.y + 250.0f,
		.width = 1280.0f,
		.height = 200.0f
	};
}

// draw duck
void draw_duckhorde(Duckhorde *duckies) {
	DrawTexturePro(
		duckies->texture,
		(Rectangle){
			0.0f,
			0.0f,
			duckies->texture.width,
			duckies->texture.height
		},
		(Rectangle){
			duckies->position.x,
			duckies->position.y,
			duckies->texture.width,
			duckies->texture.height
		},
		(Vector2){ 0.0f, 0.0f },
		0.0f,  
		RAYWHITE  
	);

	// draw duckhorde hitbox
	DrawRectangleLinesEx(duckies->hitbox, 3.0f, RED); 
}

// p much insta-death when the duckies reach the frog
void froggy_duckhorde_collision(Frog *froggy, Duckhorde *duckies){
	if (CheckCollisionRecs(froggy->hitbox, duckies->hitbox)) {
		froggy->health -= 50.0f;
	}
}

// wave border below the ducks
void draw_duckhorde_surfline(Duckhorde *duckies, float frameTime){
	float screenwidth = GetScreenWidth();
	float wavelength = DUCKHORDE_WAVELENGTH;
	float amplitude = DUCKHORDE_WAVE_AMPLITUDE;

	duckies->wavetime += frameTime;

	// draw a bunch of circles to simulate a wave, thx ai overlords
	for (int x = 0; x < screenwidth; x++) {
		float y = (duckies->position.y + 400.0f) + sinf((x + duckies->wavetime * 100) * (2 * PI / wavelength)) * amplitude;
		// TODO: draw circle is an expensive operation, use a circle texture instead for performance?
		DrawCircle(x, (int)y, 3.0f, DARKBLUE);  
	}
}

void draw_duckhorde_water(Duckhorde *duckies) {
	// below the surf draw big block
	DrawRectangle(0, (int)duckies->position.y + 400, 1280, 800, DARKBLUE);
}

int get_highscore() {    
    int highscore = 0;

    // file doesn't exist, make it
    if (!FileExists("highscore.txt")) {        
        FILE *file = fopen("highscore.txt", "w");
        if (file) {
            fprintf(file, "%d", highscore);
            fclose(file);
        } 
	// file exists, read highscore
    } else {        
        FILE *file = fopen("highscore.txt", "r");
        if (file) {
            fscanf(file, "%d", &highscore);
            fclose(file);
        } 
    }
    return highscore;
}

void update_highscore(Frog *froggy, int highscore) {
    // current_score > highscore
    if (froggy->score > highscore) {
        FILE *file = fopen("highscore.txt", "w");
        if (file) {
            fprintf(file, "%d", froggy->score); 
            fclose(file);
        }    
    }
}

int main () {	
	// Tell the window to use vsync and work on high DPI displays
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	SetTargetFPS(60);

	// Create the window and OpenGL context
	InitWindow(1280, 800, "Froggy");
	// initialize audio
	InitAudioDevice();	

	// Utility function from resource_dir.h to find the resources folder and set it as the current working directory so we can load from it
	SearchAndSetResourceDir("resources");

	int highscore = get_highscore();	

	Texture2D frog_texture = LoadTexture("frog.png");	

	// initialize froggy
	Frog froggy = (Frog){
		.texture = frog_texture,
		.position = (Rectangle){
			.x = 600.0f,
			.y = 800.0f,
			.width = 0.0f,
			.height = 36.0f,
		},						
		.direction = RIGHT,		
		.isJumping = false,
		.frame = 0,
		.jumpTimer = 0.0f,   
		.frameTimer = 0.0f,		
		.health = FROGGY_HEALTH,
		.status = ALIVE,
		.highestPosition = 0.0f,
		.isBouncing = false,
		.tongue = (Rectangle){ 800.0f, 1280.0f, 0.0f, 0.0f },
		.tongueHitbox = (Rectangle){ 800.0f, 1280.0f, 0.0f, 0.0f },
		.attackDuration = FROGGY_ATTACK_DURATION,
		.size = 1.0f,	
		.bugsEaten = 0,
		.isShooting = false,
		.dropDown = false,		
		.spitCooldown = 0.0f,
		.isPoisoned = false,
		.isBurning = false
	};		

	Texture2D duckhorde_texture = LoadTexture("duckhorde.png");

	// initialize duckhorde
	Duckhorde duckies = (Duckhorde){
		.texture = duckhorde_texture,
		.position = (Rectangle){ 
			.x = 0.0f,
			.y = 1600.0f,
			.width = (float)duckhorde_texture.width,
			.height = (float)duckhorde_texture.height
			},
		.velocity = DUCKHORDE_UPWARD_VELOCITY,
		.wavetime = 0.0f
	};

	// 8 pictures on the frog sprite sheet -> 
	const float frameWidth = (float)(frog_texture.width / 8);
	const int maxFrames = (int)frog_texture.width / (int)frameWidth;

	// initialize camera 
	Camera2D camera = {0};
	camera.target = (Vector2){ 0.0f, froggy.position.y};  
	camera.offset = (Vector2){ 0.0f, (float)(GetScreenHeight() / 2)};  
	camera.rotation = 0.0f;
	camera.zoom = 1.0f;

	Texture2D mosquito_texture = LoadTexture("bug.png");
	Texture2D wasp_texture = LoadTexture("WASP.png");
	Texture2D fish_texture = LoadTexture("fish.png");
	Texture2D bugspit_texture = LoadTexture("bugspit.png");
	Texture2D heart_texture = LoadTexture("heart.png");
	// untill i find a better texture, placeholder for testing
	Texture2D flamespitter_texture = LoadTexture("placeholder.png");
	Texture2D flameprojectile_texture = LoadTexture("placeholder_spit.png");
	
	// max distance for sounds
	float maxDistance = sqrt(SCREEN_WIDTH * SCREEN_WIDTH + SCREEN_HEIGHT * SCREEN_HEIGHT); 

	// TODO: add sounds
	const char *sound_path_mosquito_buzz = "mosquito.wav";
	// Sound wasp_buzz = LoadSound("");
	Sound fish_splash = LoadSound("splash.wav");
	// Sound heart_healing = LoadSound("");
	Sound tongue_slurp = LoadSound("lick.wav");
	Sound frog_leap = LoadSound("jump.wav");
	// TODO: what else?
	

	// mosquito (only 2 frames for directions)
	const float frameWidthBug = (float)(mosquito_texture.width / 2);

	// wasp 2 frames for flying animation
	const float frameWidthWasp = (float)(wasp_texture.width / 2);
	const int maxFramesWasp = (int)(wasp_texture.width / frameWidthWasp);

	// fish 8 frames 
	const float frameWidthFish = (float)(fish_texture.width / 8);
	const int maxFramesFish = (int)(fish_texture.width / frameWidthFish);

	// array of mosquitoes / wasps / fish
	Bug mosquitoes[MAX_MOSQUITOES];
	Bug wasps[MAX_WASPS];
	Fish fishies[MAX_FISH];
	Bugspit spitties[FROGGY_MAX_BUG_SPIT];
	Lilypad pads[MAX_LILLYPADS];
	Heart hearties[MAX_HEARTS];
	Sound buzzes[MAX_MOSQUITOES];
	Flamespitter flameys[MAX_FLAMESPITTERS];
	Flameprojectile projectiles[MAX_FLAMEPROJECTILES];

	int activeMosquitoes = 0;	
	int activeWasps = 0;	
	int activeFish = 0;
	int activeSpit = 0;
	int activePads = 0;
	int activeHearts = 0;
	int activeBuzzes = 0;
	int activeFlamespitters = 0;
	int activeFlameprojectiles = 0;

	// prepare the buzz sounds for the mosquitoes	
	while (activeBuzzes < MAX_MOSQUITOES) {
		create_mosquito_buzz_instance(sound_path_mosquito_buzz, buzzes, &activeBuzzes);
	}

	// spawntimers
	float mosquitoSpawnTimer = MOSQUITO_SPAWN_TIMER;		
	float waspSpawnTimer = WASP_SPAWN_TIMER;
	float heartSpawnTimer = HEART_SPAWN_TIMER;

	// lilipad init
	Texture2D lilypad_texture = LoadTexture("lilipads.png");	
	
	// 4 frames in lilypad sprite sheet
	const float frameWidthLilypad = (float)(lilypad_texture.width / 4);	
	
	float nextLilypadSpawn = 0.0f;	

	// TODO: heart spawn timer?	

	// game loop
	while (!WindowShouldClose()) // run the loop untill the user presses ESCAPE or presses the Close button on the window
	{
		// updates
		int fps = GetFPS();
		float frameTime = GetFrameTime();

		mosquitoSpawnTimer += frameTime;
		waspSpawnTimer += frameTime;
		heartSpawnTimer += frameTime;

		// spawn mosquitoes 
		if (mosquitoSpawnTimer >= MOSQUITO_SPAWN_INTERVAL && activeMosquitoes < MAX_MOSQUITOES) {
			spawn_mosquito(&mosquitoes[activeMosquitoes], &froggy, mosquito_texture, buzzes);
			activeMosquitoes++;
			mosquitoSpawnTimer = 0.0f;	
		}		

		// spawn wasp
		if (waspSpawnTimer >= WASP_SPAWN_INTERVAL && activeWasps < MAX_WASPS) {
			spawn_wasp(&wasps[activeWasps], &froggy, wasp_texture);
			activeWasps++;
			waspSpawnTimer = 0.0f;
		}		

		// spawn hearts
		if (heartSpawnTimer >= 0 && activeHearts < MAX_HEARTS) {
			spawn_healing_heart(&hearties[activeHearts], heart_texture, pads, activePads, &froggy);
			activeHearts++;
			// add some randomization to heart spawn timer
			heartSpawnTimer = HEART_SPAWN_TIMER - (float)GetRandomValue(0,10);
		}
	
		// make initial pads to jump on		
		if (activePads < 40) {
			for (int i = 0; i < 40; i++) {
				make_lilypads(&pads[activePads], lilypad_texture, &froggy);
				activePads++;
			}
		}

		// TODO: this needs looked at still
		// keep spawning lilypads, this time offscreen
		if (froggy.position.y < nextLilypadSpawn) {
			for (int i = 0; i < OFFSCREEN_LILYPAD_SPAWN_AMOUNT && i < MAX_LILLYPADS; i++) {
				make_lilypads_offscreen(&pads[activePads], lilypad_texture, &froggy);
				activePads++;				  
			}
			nextLilypadSpawn -= 400.0f;
		}	

		// spawn fish
		if (activeFish < MAX_FISH) {			
			spawn_fish(&fishies[activeFish], fish_texture, pads, activePads);
			activeFish++;		
		}

		// spawn flamespitters
		if (activeFlamespitters < MAX_FLAMESPITTERS) {
			spawn_flamespitter(&flameys[activeFlamespitters], pads, &froggy, activePads, &activeFlamespitters, flamespitter_texture);
		}

		// frog updates
		frog_color(&froggy, frameTime);
		frog_reset_jumpstatus(&froggy);
		hitbox_frog(&froggy);		
		if (froggy.status == ALIVE) {									
			frog_baby_jump(&froggy, maxFrames, frameTime, frog_leap);
			frog_big_jump(&froggy, maxFrames, frameTime, frog_leap);
			move_frog(&froggy);						
		}
		frog_mouth_position(&froggy);
		// lots of function params, kind of N A S T Y 
		frog_attack_params(&froggy, frameTime, camera, bugspit_texture, &activeSpit, spitties, tongue_slurp);			
		frog_prevent_overheal(&froggy);
		screen_flip(&froggy);
		apply_velocity(&froggy, frameTime);	
		apply_gravity(&froggy);
		move_frog_down(&froggy);
		frog_poisoned(&froggy, frameTime);
		froggy_death(&froggy);
		froggy_duckhorde_collision(&froggy, &duckies);
		update_duckhorde_position(&duckies, &froggy);
		update_froggy_highest_position(&froggy);

		// duckhorde updates
		hitbox_duckhorde(&duckies);
		move_duckhorde(&duckies, &froggy, frameTime);	

		// update spitties
		int activeSpitAfterLoop = 0;
		for (int i = 0; i < activeSpit; i++) {
			hitbox_spit(&spitties[i]);
			apply_spit_velocity(&spitties[i], frameTime);
			deactivate_spit(&spitties[i], &froggy);

			if (spitties[i].isActive) {
				spitties[activeSpitAfterLoop++] = spitties[i];
			}
		}
		activeSpit = activeSpitAfterLoop;

		// update hearts
		int activeHeartsAfterLoop = 0;
		for (int i = 0; i < activeHearts; i++) {
			hitbox_heart(&hearties[i]);
			check_collision_tongue_heart(&froggy, &hearties[i]);
			move_caught_heart(&hearties[i], &froggy, frameTime);
			froggy_eat_heart(&froggy, &hearties[i], frameTime);
			deactivate_heart(&hearties[i], &froggy);

			if (hearties[i].isActive) {
				hearties[activeHeartsAfterLoop++] = hearties[i];
			}
		}
		activeHearts = activeHeartsAfterLoop;

		// update lillypads
		int activePadsAfterLoop = 0;		
		for (int i = 0; i < activePads; i++) {	
			collision_check_pads(&froggy, &pads[i]);	
			deactivate_lilypads(&pads[i], &froggy); 
			deactivate_lilypad_collision(&pads[i], &froggy, frameTime);

			// adjust the amount of active pads after without messing up the loop index
			if (pads[i].isActive) {
        		pads[activePadsAfterLoop++] = pads[i];
    		}			
		}	
		activePads = activePadsAfterLoop;

		// update mosquitoes
		int activeMosquitoesAfterLoop = 0;		
		for (int i = 0; i < activeMosquitoes; i++) {	
			move_mosquito(&mosquitoes[i], frameTime);	
			volume_panning(&mosquitoes[i], &froggy);
			buzz_volume_control(&mosquitoes[i], &froggy, maxDistance);
			hitbox_bug(&mosquitoes[i]);
			collision_check_bugs(&froggy, &mosquitoes[i], frameTime);
			eat_bug(&froggy, &mosquitoes[i], frameTime);
			move_caught_bug(&mosquitoes[i], &froggy, frameTime);			

			for (int j = 0; j < activeSpit; j++) {
				collision_check_spit(&spitties[j], &mosquitoes[i]);
			}

			bug_death(&mosquitoes[i]);
			bug_spit_death(&mosquitoes[i], frameTime, &froggy);
			deactivate_bug(&mosquitoes[i], &froggy);
			free_buzz(&mosquitoes[i], buzzes);
			
			// remove inactive mosquitoes after the loop 
			if (mosquitoes[i].isActive) {
				mosquitoes[activeMosquitoesAfterLoop++] = mosquitoes[i];
			}			
		}	
		activeMosquitoes = activeMosquitoesAfterLoop;	

		// update wasps		
		int activeWaspsAfterLoop = 0;
		for (int i = 0; i < activeWasps; i++) {
			animate_wasp(&wasps[i], frameTime, maxFramesWasp); 
			move_wasp(&wasps[i], &froggy, frameTime);
			hitbox_bug(&wasps[i]);
			collision_check_bugs(&froggy, &wasps[i], frameTime);				

			// this works but is it slow as shit?????
			for (int j = 0; j < activeSpit; j++) {
				collision_check_spit(&spitties[j], &wasps[i]);
			}

			bug_death(&wasps[i]);
			bug_spit_death(&wasps[i], frameTime, &froggy);
			deactivate_bug(&wasps[i], &froggy);

			if (wasps[i].isActive) {
				wasps[activeWaspsAfterLoop++] = wasps[i];
			}	
		}
		activeWasps = activeWaspsAfterLoop;

		// update fish
		int activeFishAfterLoop = 0;
		for (int i = 0; i < activeFish; i++) {
			activate_fish(&fishies[i], &froggy, fish_splash);
			animate_fish(&fishies[i], frameTime, maxFramesFish);	
			attacking_fish_collision(&fishies[i], &froggy);		
			deactivate_fish(&fishies[i], &froggy, frameTime);			

			if (fishies[i].isActive) {
				fishies[activeFishAfterLoop++] = fishies[i];
			}	
		}
		activeFish = activeFishAfterLoop;	

		// update flamespitters
		int activeFlamespittersAfterLoop = 0;
		for (int i = 0; i < activeFlamespitters; i++) {	
			update_flamespitter_direction(&flameys[i], &froggy);
			hitbox_flamespitter(&flameys[i]);
			collision_check_flamespitter(&flameys[i], &froggy);	
			flamespitter_death(&flameys[i]);

			for (int j = 0; j < activeSpit; j++) {
				collision_check_spitties_flamespitter(&spitties[j], &flameys[i]);
			}			

			shoot_flameprojectile(&flameys[i], projectiles, &froggy, &activeFlameprojectiles, flameprojectile_texture, frameTime, camera);
			deactivate_flamespitter(&flameys[i], &froggy);

			if (flameys[i].isActive) {
				flameys[activeFlamespittersAfterLoop++] = flameys[i];
			}	
		}
		activeFlamespitters = activeFlamespittersAfterLoop;

		int activeFlameprojectilesAfterLoop = 0;
		for (int i = 0; i < activeFlameprojectiles; i++) {
			apply_flame_projectile_velocity(&projectiles[i], frameTime);
			deactivate_flameprojectile(&projectiles[i], &froggy);
			hitbox_flameprojectile(&projectiles[i]);
			collision_check_flameprojectile(&projectiles[i], &froggy);

			if (projectiles[i].isActive) {
				projectiles[activeFlameprojectilesAfterLoop++] = projectiles[i];
			}	
		}
		activeFlameprojectiles = activeFlameprojectilesAfterLoop;

		// if froggy below ground, put it back on ground
    	if (froggy.position.y > (float)(GetScreenHeight() - froggy.position.height)) {
			froggy.position.y = (float)(GetScreenHeight() - froggy.position.height);
		}

		// update highscore in current game
        if (froggy.score > highscore) {
            highscore = froggy.score;
        }
	
		// drawing
		BeginDrawing();	
		BeginMode2D(camera);		
		
		// follow the froggy's y position
		camera.target.y = froggy.position.y;				
		
		// * smoothening factor
		camera.target.y = camera.target.y + (froggy.position.y - camera.target.y) * 0.1f;

		// prevent camera from going below ground:
		float minY = GetScreenHeight() / 2.0f;
		if (camera.target.y > minY) {
			camera.target.y = minY;
		}

		// debugging: visual hitboxes
		DrawRectangleLinesEx(froggy.hitbox, 1.0f, GREEN); 	
			
		for (int i = 0; i < activePads; i++) {		
			// draw lilypads
			DrawTextureRec(
				pads[i].texture,
				(Rectangle){
					frameWidthLilypad * pads[i].frame,
					0.0f,
					frameWidthLilypad,
					(float)pads[i].texture.height	
				},
				(Vector2){ (float)pads[i].position.x, (float)pads[i].position.y },
				RAYWHITE
			);
			
			// lilypad hitbox visual
			// DrawRectangleLinesEx(pads[i].hitbox, 1.0f, RED); 			
		}

		// Setup the back buffer for drawing (clear color and depth buffers)
		ClearBackground(BLUE);

		// TODO: this approach with stages, new mechanics based on distance travelled vertically
		if (froggy.highestPosition < -4000.0f) {
			ClearBackground(SKYBLUE);
		}
				
		// draw frog
		DrawTexturePro(
			froggy.texture, 
			(Rectangle){
				frameWidth * (float)froggy.frame, 
				0.0f, 
				// flip the texture horizontally depending on direction it's facing
				(froggy.direction == RIGHT) ? -frameWidth: frameWidth,	
				// flip the texture vertically when the frog is dead						
				(float)(froggy.texture.height * ((froggy.status == DEAD) ? -1 : 1)) }, 
			(Rectangle){
				froggy.position.x,
				froggy.position.y,
				frameWidth * froggy.size,					
				(float)(froggy.texture.height * froggy.size) 
			},
			(Vector2){ 
				frameWidth * froggy.size / 2, 
				(float)froggy.texture.height * froggy.size / 2 
   			}, 
			0.0f,
			froggy.color
		);	
			 
		draw_tongue(&froggy);		

		// draw bug spit
		for (int i = 0; i < activeSpit; i++) {
			draw_spit(&spitties[i]);			
		}

		// draw hearts 
		for (int i = 0; i < activeHearts; i++) {
			draw_heart(&hearties[i]);
		}

		// draw fish
		for (int i = 0; i < activeFish; i++) {
			DrawTexturePro(
				fishies[i].texture,  
				(Rectangle){
					frameWidthFish * fishies[i].frame,  
					0.0f,                                  
					frameWidthFish * fishies[i].direction,                      
					(float)fishies[i].texture.height            
				},
				(Rectangle){
					fishies[i].position.x,               
					fishies[i].position.y,   
					// increase size of texture            
					frameWidthFish * fishies[i].size,       
					fishies[i].texture.height * fishies[i].size 
				},
				(Vector2){0, 0},                         
				0.0f,                                    
				RAYWHITE                                 
			);

			// draw fish hitbox
			DrawRectangleLinesEx(fishies[i].hitbox, 1, RED); 
		}

		// draw mosquitoes
		for (int i = 0; i < activeMosquitoes; i++) {				
			// annoying twitching animation by scaling it from 90 / 100% size every frame :thumbs_up:
			DrawTexturePro(
				mosquitoes[i].texture,
				(Rectangle){
					frameWidthBug * mosquitoes[i].frame,
					0,
					(mosquitoes[i].direction == RIGHT) ? -frameWidthBug : frameWidthBug,
					mosquitoes[i].texture.height * mosquitoes[i].status},
				(Rectangle){
					mosquitoes[i].position.x,
					mosquitoes[i].position.y,
					frameWidthBug * GetRandomValue(9,10) / 10.0,					
					(float)mosquitoes[i].texture.height * GetRandomValue(9,10) / 10.0},
				(Vector2){ 0, 0 },
				// draw angle based on froggy tongue when caught
				(mosquitoes[i].isEaten) ? froggy.tongueAngle : 0.0f,
				(mosquitoes[i].status == ALIVE) ? RAYWHITE : RED);		

			DrawRectangleLinesEx(mosquitoes[i].hitbox, 1, RED); 
		}

		// draw wasps
		for (int i = 0; i < activeWasps; i++) {
			DrawTexturePro(
				wasps[i].texture,
				(Rectangle){
					frameWidthWasp * wasps[i].frame,
					0.0f,
					frameWidthWasp * wasps[i].direction,
					(float)(wasps[i].texture.height * wasps[i].status)
				},
				(Rectangle){
					wasps[i].position.x,
					wasps[i].position.y,
					frameWidthWasp * (float)(GetRandomValue(9, 10) / 10.0f),					
					(float)wasps[i].texture.height * (float)(GetRandomValue(9, 10) / 10.0f)},
				(Vector2){ 0.0f, 0.0f },
				0.0f,
				(wasps[i].status == ALIVE) ? RAYWHITE : RED
			);

			// DrawRectangleLinesEx(wasps[i].hitbox, 1, RED); 
		}

		// draw flamespitters
		for (int i = 0; i < activeFlamespitters; i++) {
			draw_flamespitter(&flameys[i]);
			DrawRectangleLinesEx(flameys[i].hitbox, 1, WHITE); 
		}

		// draw flame projectiles
		for (int i = 0; i < activeFlameprojectiles; i++) {			
			draw_flameprojectile(&projectiles[i]);
		}

		// draw duckhorde
		draw_duckhorde_water(&duckies);
		draw_duckhorde(&duckies);
		draw_duckhorde_surfline(&duckies, frameTime);

		EndMode2D();
		
		// TODO: make this N I C E 
		DrawText(TextFormat("Health: %.2f", froggy.health), 500, 0, 40, RED);
		DrawText(TextFormat("Score: %d", froggy.score), 500, 40, 40, WHITE);
		DrawText(TextFormat("Highscore: %d", highscore), 500, 80, 40, YELLOW);
		DrawText(TextFormat("fps: %d", fps), 500, 120, 40, GREEN);
		

		// debug 
		DrawText(TextFormat("Position: %.2f", froggy.position.y), 10, 10, 20, WHITE);
		DrawText(TextFormat("Velocity: %.2f", froggy.velocity.y), 10, 40, 20, WHITE);
		DrawText(TextFormat("lilypads: %d", activePads), 10, 70, 20, WHITE);
		DrawText(TextFormat("mosquitoes: %d", activeMosquitoes), 10, 100, 20, WHITE);
		DrawText(TextFormat("next_spawn: %.2f", nextLilypadSpawn), 10, 130, 20, WHITE);
		DrawText(TextFormat("acivebugs_after_loop: %d", activeMosquitoesAfterLoop), 10, 160, 20, WHITE);
		// DrawText(TextFormat("frog tongue height: %.2f", froggy.tongue.height), 10, 190, 20, WHITE);
		// DrawText(TextFormat("frog tongue width: %.2f", froggy.tongue.width), 10, 220, 20, WHITE);
		// DrawText(TextFormat("frog tongue x: %.2f", froggy.tongue.x), 10, 250, 20, WHITE);
		// DrawText(TextFormat("frog tongue y: %.2f", froggy.tongue.y), 10, 280, 20, WHITE);
		DrawText(TextFormat("acive wasps: %d", activeWasps), 10, 310, 20, WHITE);
		DrawText(TextFormat("tonguetimer: %.2f", froggy.tongueTimer), 10, 340, 20, WHITE);
		DrawText(TextFormat("attackduration: %.2f", froggy.attackDuration), 10, 370, 20, WHITE);
		DrawText(TextFormat("size: %.2f", froggy.size), 10, 400, 20, WHITE);
		DrawText(TextFormat("activeFish: %d", activeFish), 10, 430, 20, WHITE);
		DrawText(TextFormat("bugsEaten: %d", froggy.bugsEaten), 10, 460, 20, WHITE);
		DrawText(TextFormat("isShooting: %d", froggy.isShooting), 10, 490, 20, WHITE);
		DrawText(TextFormat("activeSpit: %d", activeSpit), 10, 520, 20, WHITE);
		DrawText(TextFormat("jumpheight: %.2f", froggy.jumpHeight), 10, 550, 20, WHITE);
		DrawText(TextFormat("activeHearts: %d", activeHearts), 10, 580, 20, WHITE);
		DrawText(TextFormat("tongue prog: %.2f", froggy.tongueProgress), 10, 610, 20, WHITE);
		DrawText(TextFormat("drowdown: %d", froggy.dropDown), 10, 640, 20, WHITE);
		DrawText(TextFormat("isJumping: %d", froggy.isJumping), 10, 670, 20, WHITE);
		DrawText(TextFormat("duckies pos: %.2f", duckies.position.y), 10, 700, 20, WHITE);
		DrawText(TextFormat("active flame proj: %d", activeFlameprojectiles), 10, 730, 20, WHITE);
		DrawText(TextFormat("active flame spitters: %d", activeFlamespitters), 10, 760, 20, WHITE);

		// :-C		
		if (froggy.status == DEAD) {
			DrawText("FROGGY HAS PERISHED", 400, 400, 42, RED);
		}

		// end the frame and get ready for the next one (display frame, poll input, etc...)
		EndDrawing();		
	}

	// update highscore
	highscore = get_highscore();
	update_highscore(&froggy, highscore);

	// cleanup
	// unload our textures so it can be cleaned up
	UnloadTexture(frog_texture);
	UnloadTexture(mosquito_texture);	
	UnloadTexture(lilypad_texture);
	UnloadTexture(fish_texture);
	UnloadTexture(bugspit_texture);
	UnloadTexture(heart_texture);
	UnloadTexture(duckhorde_texture);
	UnloadTexture(flamespitter_texture);
	UnloadTexture(flameprojectile_texture);

	// unload sounds (should be MAX_MOSQUITOES)
	for (int i = 0; i < activeBuzzes; i++) {
		UnloadSound(buzzes[i]);
	}
	// UnloadSound(mosquito_buzz);
	// UnloadSound(wasp_buzz);
	UnloadSound(frog_leap);
	UnloadSound(tongue_slurp);
	// UnloadSound(heart_healing);
	UnloadSound(fish_splash);

	// close audio
	CloseAudioDevice();

	// destroy the window and cleanup the OpenGL context
	CloseWindow();
	return 0;
}