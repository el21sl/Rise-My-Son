/* "Rise My Son" Game
ELEC2645 Unit 4 Assessment
-------------------------------------------------------------------------------
Description: Jump based platformer with the aim of ascending to the top
Author: Sacha Laurent (el21sl)
Student ID: 201520984
Date: 04/2024
-------------------------------------------------------------------------------
MBED Studio Version: 1.4.1
MBED OS Version: 6.12.0
Board: NUCLEO-L476RG  */

//================Include================//
#include "PinNamesTypes.h"
#include "ThisThread.h"
#include "mbed_thread.h"
#include "mbed.h"
#include "N5110.h"
#include "Joystick.h"
#include "Sprites.h"
#include "Player.h"
#include "Level.h"
#include <iostream>
//========= Function Prototypes =========//
void init(float brightness, float contrast);
void menu();
void trigger_pause(float &bright, float &cont);
void pause(float &bright, float &cont);
void pause_levels(float &bright, float &cont);
void pause_options(float &bright, float &cont);
void button_start_isr();
void timer_isr();
void intro();
void sage_dialogue(int &jump);
void finale(int &count, int &final_pulse, bool &lcd_mode, bool &end);
//=========== Interrupt Flags ===========//
volatile int button_start_flag = 0;
volatile int timer_start_flag = 0;
//=============== Objects ===============//
N5110 lcd(PC_7, PA_9, PB_10, PB_5, PB_3, PA_10);
Joystick joystick(PC_3, PC_2);
InterruptIn button_start(BUTTON1);
DigitalIn stick_but(PC_10);
Ticker ticker;  // for timer ISR
Level level; // create level class instance

int main() {
    printf("-------------------------------------------\n");
    float brightness = 0.5;//initialisation brightness
    float contrast = 0.5; //initialisation contrast
    int count = 0;
    int final_pulse = 7;
    bool lcd_mode = false; //normal mode
    bool end = false;
    init(brightness, contrast); // initialise game
    menu(); // render the main menu
    intro(); // play intro cutscene

    while (end == false) {
        trigger_pause(brightness, contrast); // pause interrupt

        // user inputs
        UserInput input = {joystick.get_direction(),joystick.get_mag()};
        int jump = stick_but.read();

        sage_dialogue(jump);

        level.floor_change(lcd); //update players floor
        level.update(input, jump); //update level (player pos, collisions...)

        lcd.clear();
        level.render(lcd); // draw everything to lcd
        finale(count, final_pulse, lcd_mode, end); // if triggered, play outro cutscene
        lcd.refresh();
        
        thread_sleep_for(20); // delay between frames
    }
}

// Initialise paremeters
void init(float brightness, float contrast) {

    // initialise components (lcd, joystick)
    lcd.init(LPH7366_1);
    lcd.setContrast(brightness);
    lcd.setBrightness(contrast);
    joystick.init();

    // initialise game physics
    InitPhys phys = {0.1,1,0.008};
    level.init_phys(phys);

    // initialise player position
    InitPlayer init_player = {43,38,0,0,0};
    level.init_player(init_player);

    button_start.rise(&button_start_isr);  // on release of button, trigger interrupt
    button_start.mode(PullUp);  // button 1->0 so set internal resistor to pull-up
    stick_but.mode(PullUp); // jump button
    printf("INTIALISED\n");
}

// Initial menu screen, press blue button to proceed
void menu() {
    printf("MENU\n");
    bool show_play = false;  // boolean to alternate between showing 'play' and not showing
    ticker.attach(&timer_isr,750ms);  // interrupt every 750ms
    while(1) {
        // event triggered interrupt to break from loop and proceed from menu
        if(button_start_flag) {
            button_start_flag = 0;
            printf("PLAY\n");
            break;
        }
        // time triggered interrupt for display of flashing 'play'
        if(timer_start_flag) {
            timer_start_flag = 0;

            lcd.clear();
            lcd.drawSprite(0,0,48,84,(int*)spr_menu);  // menu sprite does not contain 'play'
            if(show_play) {lcd.drawSprite(33,36,5,18,(int*)spr_menu_play);}
            lcd.refresh();

            show_play = !show_play;  // show 'play' every other interrupt
        }
        sleep();
    }
}

// Interrupt triggers pause menu screen, triggered by pressing blue button
void trigger_pause(float &bright, float &cont) {  
    // if button 1 pressed, trigger pause menu
    if(button_start_flag) {
        button_start_flag = 0;
        printf("PAUSED\n");  
        pause(bright, cont);
    }
}

// pause menu screen
void pause(float &bright, float &cont) {
     
    bool levels = false;
    bool options = false;
    bool exit = false;
    int select_pos = 0; // 0 = levels, 1 = options, 2 = exit

    // run progress function 
    Position2D progress = level.progress();

    while (exit == false) {

        int input_p = joystick.get_direction(); // joystick inputs
        int select = stick_but.read(); // joystick button input, idle 1

        lcd.clear();
        lcd.drawSprite(2,1,46,80, (int*) spr_pause); // PAUSE + BACKGROUND

        lcd.drawSprite(65,7,6,3, (int*) spr_numbers[progress.x]);
        lcd.drawSprite(69,7,6,3, (int*) spr_numbers[progress.y]); // progress tracker
        lcd.drawSprite(73,7,6,3, (int*) spr_numbers[10]); // % sign

        lcd.drawSprite(25,18,6,35, (int*) spr_p_levels); // LEVELS
        lcd.drawSprite(22,26,6,41, (int*) spr_p_options); // OPTIONS
        lcd.drawSprite(31,34,6,23, (int*) spr_p_exit); // EXIT

        // change select pos value based on joystick input
        if (input_p == 5 && select_pos < 2) {
            // wait for joystick direction to change so only 1 increment/decrement
            while (input_p == 5) {input_p = joystick.get_direction();}
            select_pos++;
        } else if (input_p == 1 && select_pos > 0) {
            while (input_p == 1) {input_p = joystick.get_direction();}
            select_pos--;
        }

        if (select_pos == 0) { // LEVELS
            lcd.drawReverseSprite(wing_locations[0][0],wing_locations[0][1],8,11, (int*) spr_p_wing); // left wing
            lcd.drawSprite(wing_locations[0][2],wing_locations[0][3],8,11, (int*) spr_p_wing); // right wing
            // if button pressed, wait for button to be released then set flags
            if (select == 0) {
                while (select == 0) {select = stick_but.read();}
                printf("PAUSE - LEVELS\n");
                levels = true;
                exit = true;
                }
        } else if (select_pos == 1) { // OPTIONS
            lcd.drawReverseSprite(wing_locations[1][0],wing_locations[1][1],8,11, (int*) spr_p_wing); // left wing
            lcd.drawSprite(wing_locations[1][2],wing_locations[1][3],8,11, (int*) spr_p_wing); // right wing
            if (select == 0) {
                while (select == 0) {select = stick_but.read();}
                printf("PAUSE - OPTIONS\n");
                options = true;
                exit = true;
            }
        } else if (select_pos == 2) { // EXIT
            lcd.drawReverseSprite(wing_locations[2][0],wing_locations[2][1],8,11, (int*) spr_p_wing); // left wing
            lcd.drawSprite(wing_locations[2][2],wing_locations[2][3],8,11, (int*) spr_p_wing); // right wing
            if (select == 0) {
                while (select == 0) {select = stick_but.read();}
                printf("RESUME\n");
                exit = true;
            }
        }
        lcd.refresh();
    }
    
    // check flags and run associated functions
    if (levels == true) {
        pause_levels(bright, cont);
    } else if (options == true) {
        pause_options(bright, cont);
    }
}

// Pause --> Levels menu screen
void pause_levels(float &bright, float &cont) {

    bool exit = false; // flag to exit back to pause
    bool mega_exit = false;
    int select_pos = 0; // vertical position
    int select_level = 1; // horizontal position

    while(exit == false && mega_exit == false) {

        int input_p = joystick.get_direction(); // joystick inputs
        int select = stick_but.read(); // joystick button input, idle 1

        lcd.clear();
        lcd.drawSprite(2,1,46,80, (int*) spr_pause); // BACKGROUND
        lcd.drawSprite(25,7,6,35, (int*) spr_p_levels); // LEVELS

        // change select pos value based on vertical joystick input
        if (input_p == 5 && select_pos < 1) {
            // wait for joystick direction to change so only 1 increment/decrement
            while (input_p == 5) {input_p = joystick.get_direction();}
            select_pos++;
        } else if (input_p == 1 && select_pos > 0) {
            while (input_p == 1) {input_p = joystick.get_direction();}
            select_pos--;
        } 
        
        // if numbers selected, change select_level based on horizontal joystick inputs
        if (select_pos == 0) {
            if (input_p == 3 && select_level < 6) {
                // wait for joystick direction to change so only 1 increment/decrement
                while (input_p == 3) {input_p = joystick.get_direction();}
                select_level++;
            } else if (input_p == 7 && select_level > 1) {
                while (input_p == 7) {input_p = joystick.get_direction();}
                select_level--;
            }
        }

        if (select_pos == 0) { // LEVELS
            if (select_level == 1) {
                if (select == 0) {
                while (select == 0) {select = stick_but.read();}
                printf("LEVEL 1 SELECTED\n");
                InitPlayer init_player = {42,38,0,0,0};
                level.init_player(init_player);
                lcd.normalMode();
                mega_exit = true;
                }
            } else if (select_level == 2) {
                if (select == 0) {
                while (select == 0) {select = stick_but.read();}
                printf("LEVEL 2 SELECTED\n");
                InitPlayer init_player = {72,35,0,0,1};
                level.init_player(init_player);
                lcd.normalMode();
                mega_exit = true;
                }
            } else if (select_level == 3) {
                if (select == 0) {
                while (select == 0) {select = stick_but.read();}
                printf("LEVEL 3 SELECTED\n");
                InitPlayer init_player = {50,32,0,0,2};
                level.init_player(init_player);
                lcd.normalMode();
                mega_exit = true;
                }
            } else if (select_level == 4) {
                if (select == 0) {
                while (select == 0) {select = stick_but.read();}
                printf("LEVEL 4 SELECTED\n");
                InitPlayer init_player = {20,37,0,0,3};
                level.init_player(init_player);
                lcd.normalMode();
                mega_exit = true;

                }
            } else if (select_level == 5) {
                if (select == 0) {
                while (select == 0) {select = stick_but.read();}
                printf("LEVEL 5 SELECTED\n");
                InitPlayer init_player = {40,32,0,0,4};
                level.init_player(init_player);
                lcd.inverseMode();
                mega_exit = true;
                }
            } else if (select_level == 6) {
                if (select == 0) {
                while (select == 0) {select = stick_but.read();}
                printf("LEVEL 6 SELECTED\n");
                InitPlayer init_player = {9,24,0,0,5};
                level.init_player(init_player);
                lcd.normalMode();
                mega_exit = true;
                }
            }
        } else if (select_pos == 1) { // EXIT
            lcd.drawReverseSprite(wing_locations[2][0],wing_locations[2][1],8,11, (int*) spr_p_wing); // left wing
            lcd.drawSprite(wing_locations[2][2],wing_locations[2][3],8,11, (int*) spr_p_wing); // right wing
            if (select == 0) {
                while (select == 0) {select = stick_but.read();}
                printf("EXIT LEVELS\n");
                exit = true;
            }
        }

        // if on numbers, draw level indicator based on which number is being hovered over
        if (select_pos == 0) {lcd.drawSprite(12+(select_level*8),16,14,5, (int*) spr_number_select);} 
        for (int i = 1; i < 7; i++) {lcd.drawSprite(13+(i*8),20,6,3, (int*) spr_numbers[i]);} // NUMBERS 1-6
        lcd.drawSprite(31,34,6,23, (int*) spr_p_back); // BACK
        lcd.refresh();
    }
    // exiting returns to main pause function
    if (exit == true) {pause(bright, cont);}
}

// Pause --> Options menu screen
void pause_options(float &bright, float &cont) {

    // initialise flags
    bool exit = false;
    int select_pos = 0;

    while(exit == false) {

        // update user inputs
        int input_p = joystick.get_direction(); // joystick inputs
        int select = stick_but.read(); // joystick button input, idle 1

        lcd.clear();
        lcd.drawSprite(2,1,46,80, (int*) spr_pause); // BACKGROUND
        lcd.drawSprite(22,7,6,41, (int*) spr_p_options); // OPTIONS

        // change select pos value based on joystick input
        if (input_p == 5 && select_pos < 2) {
            // wait for joystick direction to change so only 1 increment/decrement
            while (input_p == 5) {input_p = joystick.get_direction();}
            select_pos++;
        } else if (input_p == 1 && select_pos > 0) {
            while (input_p == 1) {input_p = joystick.get_direction();}
            select_pos--;
        } 

        // Brightness
        if (select_pos == 0) {
            lcd.drawSprite(67,19,6,6, (int*) spr_signs[0]); // PLUS
            lcd.drawSprite(11,19,6,6, (int*) spr_signs[1]); // MINUS
            if (input_p == 3 && bright < 1) {
                while (input_p == 3) {input_p = joystick.get_direction();}
                bright += 0.1; // increase brightness and pass by reference
                lcd.setBrightness(bright);
            } else if (input_p == 7 && bright > 0) {
                while (input_p == 7) {input_p = joystick.get_direction();}
                bright -= 0.1;
                lcd.setBrightness(bright);
            }
        // Contrast
        } else if (select_pos == 1) {
            lcd.drawSprite(64,27,6,6, (int*) spr_signs[0]); // PLUS
            lcd.drawSprite(14,27,6,6, (int*) spr_signs[1]); // MINUS
            if (input_p == 3 && cont < 1) {
                while (input_p == 3) {input_p = joystick.get_direction();}
                cont += 0.05;// increase contrast and pass by reference
                lcd.setContrast(cont);
            } else if (input_p == 7 && cont > 0) {
                while (input_p == 7) {input_p = joystick.get_direction();}
                cont -= 0.05;
                lcd.setContrast(cont);
            }
        // EXIT
        } else if (select_pos == 2) { // EXIT
            lcd.drawReverseSprite(wing_locations[2][0],wing_locations[2][1]+1,8,11, (int*) spr_p_wing); // left wing
            lcd.drawSprite(wing_locations[2][2],wing_locations[2][3]+1,8,11, (int*) spr_p_wing); // right wing
            if (select == 0) {
                while (select == 0) {select = stick_but.read();}
                printf("EXIT OPTIONS\n");
                exit = true;
            }
        }
        lcd.drawSprite(19,19,6,46, (int*) spr_p_opt_bright); // BRIGHTNESS
        lcd.drawSprite(22,27,6,40, (int*) spr_p_opt_contrast); // CONTRAST
        lcd.drawSprite(31,35,6,23, (int*) spr_p_back); // BACK
        lcd.refresh();
    }
    // exiting returns to main pause function
    if (exit == true) {pause(bright, cont);}
}

// set event flag in ISR
void button_start_isr() {button_start_flag = 1;}   

// set timer flag in ISR
void timer_isr() {timer_start_flag = 1;}   

// bird falls from sky and thumps on ground, intro cutscene
void intro () {
    // bird falling
    for (int i=-10; i<40 ;i++){
        lcd.clear();
        lcd.drawSprite(0, 0, 48, 84, (int*) spr_backgrounds[0]);
        lcd.drawSprite(0, 0, 27, 14, (int*) spr_trunk_1);
        lcd.drawSprite(43, i, 5, 7, (int*) fletcher[0]);
        lcd.refresh();
        thread_sleep_for(30);
    }
    // landing animation
    for (int j=0; j<5; j++) {
        lcd.clear();
        lcd.drawSprite(0, 0, 48, 84, (int*) spr_backgrounds[0]);
        lcd.drawSprite(0, 0, 27, 14, (int*) spr_trunk_1);
        lcd.drawSprite(38, 39, 6, 17,(int*) start_anim[j]);
        if(j>=2) {lcd.drawSprite(67,28,12,6, (int*) spr_exclamation);}
        lcd.refresh();
        thread_sleep_for(130);
    }
    // bird sitting down
    lcd.clear();
    lcd.drawSprite(0, 0, 48, 84, (int*) spr_backgrounds[0]);
    lcd.drawSprite(0, 0, 27, 14, (int*) spr_trunk_1);
    lcd.drawSprite(67,28,12,6, (int*) spr_exclamation);
    lcd.drawSprite(43, 39, 5, 7, (int*) fletcher[2]);
    lcd.refresh();
    thread_sleep_for(1000);

}

// interaction with the sage
void sage_dialogue(int &jump) {
    // if in correct position
    if(level.get_floor() == 0) {
        if (level.get_player_pos().x == 59 && level.get_player_pos().y == 42 && jump == 0) {
            // wait for button to be released
            while(jump == 0) {jump = stick_but.read();}
            lcd.drawSprite(2,24,9,52,(int*) spr_sage_text1); // draw text 1 to lcd
            lcd.refresh();
            thread_sleep_for(100);

            // wait for button to be pressed then released
            while(jump == 1) {jump = stick_but.read();}
            while(jump == 0) {jump = stick_but.read();}

            level.render(lcd);
            lcd.drawSprite(3,20,16,50,(int*) spr_sage_text2); //draw text 2 to lcd
            lcd.refresh();
            thread_sleep_for(100);

            // wait for button to be pressed then released
            while(jump == 1) {jump = stick_but.read();}
            while(jump == 0) {jump = stick_but.read();}

            level.render(lcd);
            lcd.drawSprite(1,22,14,54,(int*) spr_sage_text3); //draw text 3 to lcd
            lcd.refresh();
            thread_sleep_for(100);
            
            // wait for button to be pressed then released
            while(jump == 1) {jump = stick_but.read();}
            while(jump == 0) {jump = stick_but.read();}
            // manually set jump to not jumping so player doesnt jump after leaving interaction
            jump = 1;
        }
    }
}

// bird ascends, final cutscene
void finale(int &count, int &final_pulse, bool &lcd_mode, bool &end) {
    if (level.trigger_finale() == true) {
        level.init_phys({0.006,0,0}); //set gravity to 0.006 (much lower than before)
        // switch between lcd modes with an increasing frequency, increment count till it reaches threshold, then change mode and decrease threshold
        if (count == final_pulse) {
            lcd_mode = !lcd_mode; // switch modes
            if (final_pulse != 1) {final_pulse-= 1;} // decrease threshold
            count = 0; // reset counter
        }
        count++;
        if (lcd_mode == true) {lcd.inverseMode();}
        else {lcd.normalMode();}
    }
    
    // once reached the next floor, turn screen black for 2 sec
    if (level.get_floor() == 6) {
        lcd.clear();
        lcd.inverseMode();
        lcd.refresh();
        thread_sleep_for(2000);

        lcd.normalMode();
        // final background comes down, makes it look like camera is panning up to them
        for (int i = -50; i < 1; i++) {
            lcd.drawSprite(6,i,48,84, (int*) spr_final);
            lcd.refresh();
        }
        // display text (my son, you made it)
        thread_sleep_for(700);
        lcd.drawSprite(7,33,9,14, (int*) spr_fin_text1);
        lcd.refresh();
        thread_sleep_for(700);
        lcd.drawSprite(60,30,14,20, (int*) spr_fin_text2);
        lcd.refresh();
        thread_sleep_for(2000);
        
        // display next text (I never doubted you, not for a second)
        lcd.clear();
        lcd.drawSprite(6,0,48,84, (int*) spr_final);
        lcd.refresh();

        thread_sleep_for(700);
        lcd.drawSprite(1,33,15,32, (int*) spr_fin_text3);
        lcd.refresh();
        thread_sleep_for(700);
        lcd.drawSprite(52,33,14,31, (int*) spr_fin_text4);
        lcd.refresh();

        // trigger end of main while loop
        end = true;
    }
}