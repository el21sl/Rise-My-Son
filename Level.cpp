#include "Level.h"
#include "PinNames.h"
#include "Sprites.h"
#include <iostream>

Level::Level() {}

// initialise physics variables
void Level::init_phys(InitPhys init_phys) {
    _player.set_grav(init_phys.grav); // 0.1
    _player.set_term_vel(init_phys.tvel); // 1
    _player.set_drag(init_phys.drag); //0.008
}

// initialise player variables
void Level::init_player(InitPlayer init_player) {
    _player.set_x(init_player.x);
    _player.set_y(init_player.y);
    _player.set_vx(init_player.vx);
    _player.set_vy(init_player.vy);
    _floor = init_player.floor;
    _player.set_floor(init_player.floor);
    _player.init();
}

// detect and change active floor the player is on
void Level::floor_change(N5110 &lcd) {
    // if at the top and jumping up
    if (_player.get_pos().y < 0 && _player.get_vel().y < 0) {
        _floor ++; // increment floor
        _player.set_y(42); // set player y value to bottom

        // if on level 5, invert lcd mode
        if(_floor == 4) {lcd.inverseMode();}
        else {lcd.normalMode();}

    // if at bottom and going down
    } else if (_player.get_pos().y > 48 && _player.get_vel().y > 0) {
        _floor --; // decrement floor
        _player.set_y(1); // set player y coord to top

        if(_floor == 4) {lcd.inverseMode();}
        else {lcd.normalMode();}
        // cant go lower than ground floor
        if (_floor < 0) {_floor = 0;}
    }
}

// calculates how close the player is to the end as a percentage
// returns progress as 2 integers (tens,ones,%) as position 2d, this is so i dont have to use a new struct
Position2D Level::progress() {
    float y_pos;
    float elevation;
    float max_height;
    int progress; // percentage in 2 numbers e.g. 34%
    int progress_tens; // first digit e.g. 3
    int progress_ones; // second digit e.g. 4

    y_pos = 48 - _player.get_pos().y; // y increases downwards normally so flip
    elevation = y_pos+(_floor*48); // total height of player
    max_height = 6*48; // 6 floors, with 48 pixels in each so max = 288
    progress = (elevation/max_height) * 100;
    progress_tens = progress/10;
    progress_ones = progress%10;

    return {progress_tens, progress_ones};
}

// combine methods from player class to update player position, speed...
void Level::update(UserInput input, int jump) {
    _player.set_floor(_floor);
    _player.walk(input);
    _player.collisions();
    _player.jump(jump);
    _player.update();
    if (_player.get_grounded() == true) {_player.animate_walk();} //only animate when grounded
}

// draw sprites to lcd
void Level::render(N5110 &lcd) {
    lcd.drawSprite(0, 0, 48, 84, (int*) spr_backgrounds[_floor]); // draw background to lcd

    // draw tree trunks for the respective level
    if (_player.get_pos().x > 13) {
        if (_floor == 0) {
            lcd.drawSprite(0, 0, 27, 14, (int*) spr_trunk_1);
        } else if (_floor == 1) {
            lcd.drawSprite(0, 0, 48, 13, (int*) spr_trunk_2);
        } else if (_floor == 2) {
            lcd.drawSprite(0, 0, 48, 13, (int*) spr_trunk_3);
        } else if (_floor == 3) {
            lcd.drawSprite(0, 37, 11, 15, (int*) spr_trunk_4);
        } 
    }

    // if on the ground floor and goes into cave
    if (_floor == 0) {
        if(_player.get_pos().x > 51 && _player.get_pos().y >19) {
            lcd.inverseMode();  // inverse lcd mode
            lcd.drawSprite(0, 0, 48, 84, (int*) spr_cave); // draw cave background
        } else {lcd.normalMode();}
    }

    _player.render(lcd); // draw player last so on top of everything
}

bool Level::trigger_finale() {return _player.get_trigger_finale();}

int Level::get_floor() {return _floor;}

bool Level::get_player_grounded() {return _player.get_grounded();}

Position2D Level::get_player_pos() {return _player.get_pos();}