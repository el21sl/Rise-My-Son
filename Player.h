#ifndef PLAYER_H
#define PLAYER_H

#include "mbed.h"
#include "N5110.h"
#include "Joystick.h"

class Player{

    public:
        Player();
        void init(); // initialise player flags
        void walk(UserInput input); // check joystick pos, increase respective velocity
        void jump(int jump); // check joysitck button press, increase y velocity
        void update(); // add velocity to pos and check whether to apply gravity
        void collisions(); // check coordinates around player on collision map and set velocities to 0 if colliding
        void animate_walk(); // check what walk cycle the player is in
        void render(N5110 &lcd);
        //=========SET============
        void set_y(float y); // set x and y coords
        void set_x(float x);
        void set_vy(float vy); // set x and y velocities
        void set_vx(float vx);
        void set_grav(float grav); // set gravity and other physics
        void set_term_vel(float term_vel);
        void set_drag(float drag);
        void set_floor(int floor); // set the floor the player is on
        //=========GET==========
        Position2D get_pos(); // get player coordinates
        Vector2D get_vel(); // get player velocity
        bool get_grounded(); // get player grounded status
        bool get_trigger_finale(); // get finale flag
        
    private:
        // player coord and velocities
        int _x;
        int _y;
        float _float_x;
        float _float_y;
        float _prev_x;
        float _vx;
        float _vy;
        int _floor;

        // physics
        float _gravity;
        float _term_vel;
        float _drag;

        bool _direction; // direction player facing
        bool _anim_walk; // walk cycle

        // collision
        bool _grounded;
        bool _fall_through;

        bool _trigger_finale; 
};

#endif