#ifndef LEVEL_H
#define LEVEL_H

#include "mbed.h"
#include "N5110.h"
#include "Player.h"
#include "Joystick.h"

// used to set player location
struct InitPlayer {
    float x;
    float y;
    float vx;
    float vy;
    int floor;
};

// used to set level physics
struct InitPhys {
    float grav;
    float tvel;
    float drag;
};

class Level {
    
public:
    Level();
    void init_phys(InitPhys init_phys); // set desired physics values
    void init_player(InitPlayer init_player); // set desired player location
    void floor_change(N5110 &lcd); //  check whether to change what floor player is on
    Position2D progress(); // calculate elevation as a perventage
    void update(UserInput input, int jump); // update player
    void render(N5110 &lcd); // draw to lcd
    bool trigger_finale(); // check if in right pos to trigger finale

    int get_floor(); // outputs current floor
    bool get_player_grounded(); // outputs player grounded status
    Position2D get_player_pos(); // outputs player coords

private:
    Player _player;
    int _floor;
};

#endif