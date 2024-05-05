#include "Player.h"
#include "Sprites.h"
#include "mbed_thread.h"
#include <iostream>
#include <cmath>
#include <string>

Player::Player() {}

// ===================SET====================

void Player::set_x(float x) {_float_x = x;}
void Player::set_y(float y) {_float_y = y;}
void Player::set_vx(float vx) {_vx = vx;}
void Player::set_vy(float vy) {_vy = vy;}
void Player::set_grav(float grav) {_gravity = grav;}
void Player::set_term_vel(float term_vel) {_term_vel = term_vel;}
void Player::set_drag(float drag) {_drag = drag;}
void Player::set_floor(int floor) {_floor = floor;}

//====================GET====================

Position2D Player::get_pos() {return {_x,_y};}
Vector2D Player::get_vel() {return {_vx, _vy};}
bool Player::get_grounded() {return _grounded;}
bool Player::get_trigger_finale() {return _trigger_finale;}

//===================OTHER===================

// initialises player status flags
void Player::init(){
    _direction = false; // false == facing right
    _anim_walk = false; // false == 2 feet
    _grounded = true; // player is colliding with ground
    _fall_through = false; // player has clipped through a collision
    _trigger_finale = false;
}

// uses input from joystick to set x velocites
void Player::walk(UserInput input) {
    // previous x used to check if it should animate
    _prev_x = _x;
    // update y value depending on direction of movement
    if (_grounded == true) {
        if (input.d == E) { 
            _vx = 0.5; // set x velocity to 0.4 to the right
            _direction = false; // face right
        } else if (input.d == W) {
            _vx = -0.5; // set x velocity to 0.4 to the left
            _direction = true; // face left
        // if joystick is central reset velocity
        } else if (input.d == CENTRE) {_vx = 0;}    
    }
}

// checks the coords around player to see if colliding with obstacles on collision map
void Player::collisions() {
    //================================== TOP & BOTTOM COLLISION =========================================
    // temp flags (to compare if any coordinate is an obstacle)
    bool clipped = false;
    bool ground_col = false;
    bool head_col = false;
    // iterate through 5 middle coords in line with feet, not 7
    // because otherwise looks like player is floating on edge of platforms 
    for (int i = 0; i<5; i++) {
        // check coordinates on collision map, if there is an obstacle, set clipped flag
        if (collision_maps[_floor][_y + 4][_x + 1 + i] == 1) {
            clipped = true;
        // check coordinates below player, if obstacle is present set ground collision flag
        }else if (collision_maps[_floor][_y + 5][_x + 1 + i] == 1 && _y != 43) {
            ground_col = true;
        }
        // if colliding above, then set head collision flag (bounce down) 
        if (collision_maps[_floor][_y-1][_x+i] == 1 && _y > 0 && _vy < 0) {head_col = true;}
    }

    // if first 2 are true, set the actual flags, if neither then no collision so not grounded
    if (clipped == true) {_fall_through = true;
    } else if (ground_col == true) {_grounded = true;
    } else { _grounded = false;}
    
    // if head collision flip sign on velocity to bounce down
    if (head_col == true) {_vy = -_vy;}

    //================================== SIDE COLLISION =========================================
    // temp flags (to compare if any coordinate is an obstacle)
    bool left_col = false;
    bool right_col = false;
    // iterate through height
    for (int j = 0; j<5; j++) {
        // only collide if within vertical limits of screen
        if (_y+j >= 2 && _y+j < 48) {
        // check coords either side and set flags if collisions
            if (_direction == false && (collision_maps[_floor][_y+j][_x+7] == 1 || _x == 77)) {right_col = true;}
            if (_direction == true && (collision_maps[_floor][_y+j][_x-1] == 1 || _x == 0)) {left_col = true;}
        }
    }
    // if collision occurs
    if (right_col == true || left_col == true) {
        if (_grounded == true) {
            // if player is grounded and tries to move in direction of collision, stop player
            if ((right_col == true && _vx > 0) || (left_col == true && _vx < 0)) {_vx = 0;}
        // if not grounded but collision, bounce
        } else if (_vx != 0) {
            _vx = -_vx;
            _direction = !_direction;
        }
    }
}

// uses input from button to jump
void Player::jump(int jump) {
    // jump == 0 means button has been pressed
    if (_grounded == true && jump == 0) {
        if (_floor == 5 && _y < 14 && _grounded == true) {
            _vy = -0.5;
            _trigger_finale = true;
        } else if (_floor == 4 && _y < 4) {
            _vy = -2.2;
        } else {
        _vy = -1.8;   // if player is grounded and jump button has been pressed, give upwards velocity
        }
        _grounded = false; // will not be on the ground so change status of flag
    }
}

// update position of player
void Player::update() {

    if (_grounded == false) { 
        // gravity, downwards acceleration
        // if in the air and not reached term vel, increase downwards velocity
        if(_vy <= _term_vel) {
            _vy += _gravity;
            
        } else {
            if (_vx > 0) {_vx -= _drag;}
            else if (_vx < 0) {_vx += _drag;}
        }
        // while in the air slow down gradually
    }

    if (_grounded == true) {_vy = 0;} // if grounded set y velocity to 0
        _float_x += _vx; // float x is used as an intermediate to set int x, if vx was rounded it would never change the pos
        _x = round(_float_x);
        _float_y += _vy;
        _y = round(_float_y);
        
    // if player has clipped through collision, move player upwards then reset flag
    if (_fall_through == true) {
        _y -= 2;
        _fall_through = false;
    }
}

// draw player on lcd
void Player::render(N5110 &lcd) {
    // select which walking animation cycle render
    int n;
    if (_anim_walk == false) {
        n = 0; // switch between walking sprites
    } else if (_anim_walk == true) {
        n = 1;
    }

    if (_direction == false){   // switch between direction sprites
        lcd.drawSprite(_x, _y, 5, 7, (int*) fletcher[n]);
    } else if (_direction == true){
        lcd.drawReverseSprite(_x, _y, 5, 7, (int*) fletcher[n]);
    }
}

// if moving _x pos changes change walk anim
void Player::animate_walk() {if (_x != _prev_x) {_anim_walk = !_anim_walk;}} 