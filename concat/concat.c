#include "board.h"
#include "monsters.h"
#include <stdlib.h>
#include <stddef.h>
#include <display.h>
#include <7seg.h>
#include <timer.h>
#include <button.h>
#include <led.h>
#include <adc.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

typedef void(* alarmcallback_t )(void);

//needs to be volatile since it is set in multiple ISRs
static volatile uint8_t event;
static volatile STATE state;

uint16_t pacman_speed;
uint16_t ghost_speed;
uint8_t ghost_vulnerable;

position player_pos;
position ghost_pos;

uint8_t points;
uint8_t dots_left;

// global variable to count the number of overflows
uint16_t ghost_of;
uint16_t pacmn_of;

// since button1 is not debounced in hw the simples solution is to use the spiclib  functions
BUTTONSTATE button1_last_state;
BUTTONSTATE button0_last_state;

void be_vulnerable(){
    if(ghost_pos.dead) return;
    sb_led_setMask(ghost_vulnerable);
    if(ghost_vulnerable){ 
        ghost_vulnerable = ghost_vulnerable >> 1;
        sb_timer_setAlarm((alarmcallback_t) be_vulnerable, 800 , 0);
    }
}


// TIMER0 overflow interrupt service routine
// called whenever TCNT0 overflows
ISR(TIMER0_OVF_vect) {
     BUTTONSTATE bs = sb_button_getState(BUTTON1);
     if(button1_last_state == PRESSED){
        button1_last_state = bs;
     } else if(button1_last_state != PRESSED && bs == PRESSED){      
         button1_last_state = PRESSED; 
         event = EVENT_CHANGE_DIR;
         if(state == START){
             state = HALT;
             return;
         }
         player_pos.next_dir -= 1;
         if(player_pos.next_dir < 0) player_pos.next_dir += 4;
         return;
    }
    bs = sb_button_getState(BUTTON0);
    if(button0_last_state == PRESSED){
        button0_last_state = bs;
    } else if(button0_last_state != PRESSED && bs == PRESSED){      
         button0_last_state = PRESSED; 
        event = EVENT_CHANGE_DIR;
        if(state == START){
            state = HALT;
            return;
        }
        player_pos.next_dir += 1;
        player_pos.next_dir %= 4;
        return;
    }

    if(state == START) return;

     // keep a track of number of overflows
     ++pacmn_of;
     ++ghost_of;

     if (pacmn_of >= pacman_speed){
         TCNT0 = 0;            // reset counter
         pacmn_of = 0;     // reset overflow counter
         event = EVENT_ACT_P_POS;
         return;
     }

     if(ghost_pos.dead) return; 
     if (ghost_of >= ghost_speed){
         TCNT0 = 0;            // reset counter
         ghost_of = 0;     // reset overflow counter
         event = EVENT_ACT_G_POS;
     }

}

void deaktivate_timers(){
    TIMSK0 = 0;
}

void timer0_init(){
    //TIMER0
    // set up timer with no prescaling
    // TCCR0B |= (1 << CS00);
 
   // set up timer with prescaler = 1024
    //TCCR0B |= (1 << CS02)|(1 << CS00);
    TCCR0B |= (1 << CS01);

    // initialize counter
    TCNT0 = 0;

    // enable overflow interrupt
    TIMSK0 |= (1 << TOIE0); 
}


void init(){
    button1_last_state = UNKNOWN;
    button0_last_state = UNKNOWN;
    dots_left = 0;
    ghost_of = 0;
    pacmn_of = 0;

    event = 0;
    ghost_vulnerable = 0;
    state = START;
    dots_left = 0;
    points = 0;
    sb_7seg_showNumber(points);
    set_speed();
    sb_display_fillScreen(NULL); // Clear display
    for(uint8_t i = 0; i < sizeof(board); ++i){
        for(uint8_t bit = 0; bit < 8; ++bit){
            uint8_t page, col;
            get_tile(&page, &col, bit, i);

            if(board[i] & (1 << bit)){
                sb_display_drawBitmapFromFlash(page, col, 1, TILE_SIZE, wall); 
            } else {
                dots[i] |= (1 << bit);
                if(special_dots[i] & (1 << bit) ) {
                    sb_display_drawBitmapFromFlash(page, col, 1, TILE_SIZE, dot_filled); 
                } else {
                    sb_display_drawBitmapFromFlash(page, col, 1, TILE_SIZE, dot); 
                }
                ++dots_left;
            }

            if(i*8 + bit == POS_PACMAN){
                player_pos.px = (i * 64 + bit * 8) % 128; 
                player_pos.py = (i * 64 + bit * 8) / 128 * 8; 
                player_pos.next_dir = 0;
                player_pos.dir = RIGHT;
                player_pos.dead = false;
                render_pacman();
                erase_dot(player_pos.px, player_pos.py);
                --dots_left;
                //sb_display_drawBitmapFromFlash(page, col, 1, TILE_SIZE, pacman); 
            } else if(i*8 + bit == POS_GHOST){
                ghost_pos.px = (i * 64 + bit * 8) % 128; 
                ghost_pos.py = (i * 64 + bit * 8) / 128 * 8; 
                ghost_pos.next_dir = 0;
                ghost_pos.dir = LEFT;
                ghost_pos.dead = false;
                render_ghost();
                calc_next_ghost_dir();
            }
        }

    }
}

void set_speed(){

     int16_t poti = sb_adc_read(POTI);
     float bn = 1.0 - (float)(poti)/(float)(MAX_DEV_VAL);

     pacman_speed = MAX_SPEED + (MIN_SPEED - MAX_SPEED) * bn;
     ghost_speed = pacman_speed * 1.2;
}

ISR(ADC_vect){
    event = 72;
    set_speed();
}

// BOARD ACCESS: HELPER FUNCTIONS

uint8_t next_pos_in_board(position *pos){
    switch(pos->dir){
        case RIGHT:
            if(pos->px + TILE_SIZE >= 127) return false; 
            break;
        case DOWN:
            if(pos->py + TILE_SIZE >= 63) return false; 
            break;
        case LEFT:
            if(pos->px < TILE_SIZE ) return false; 
            break;
        case UP:
            if(pos->py < TILE_SIZE) return false; 
            break;
        default: 
            sb_7seg_showString("di");
            while(1);

    }
    return true;
}

uint8_t pos_free(position *pos){
    return !get_board_cont_from_px(pos->px, pos->py);
}

uint8_t next_pos_free(position *pos){
    if(!next_pos_in_board(pos)) return false;

    if(next_pos(8, pos)){
        sb_7seg_showString("cf");
        while(1);
    }
    uint8_t ret = pos_free(pos);

    if(next_pos(-8, pos)){
        sb_7seg_showString("CF");
        while(1);
    }
    return ret;
    
}

// RENDER FUNCTIONS

void render_pacman(){
    uint8_t tile[TILE_SIZE];

    uint8_t pos_lag = (player_pos.px % 8) + (player_pos.py % 8);
    uint8_t bias;
 
    switch(player_pos.dir){
        case DOWN: bias = ROTATE_1; break;
        //case LEFT: bias = ROTATE_2; break;
        case LEFT: bias = MIRROR_V; break;
        case UP:   bias = ROTATE_3; break;
        default:   bias = NORMAL;
    }
   

    switch(pos_lag){
        case 1:
        case 6: calc_bitmap(pacman_bm_2, tile, bias); break;
        case 2:
        case 5: calc_bitmap(pacman_bm_3, tile, bias); break;
        case 3:
        case 4: calc_bitmap(pacman_bm_4, tile, bias); break;
        default:  calc_bitmap(pacman_bm_1, tile, bias); 

    }
    render_tile(&player_pos, tile);
}
void render_ghost(){
    uint8_t tile[TILE_SIZE];
    switch(ghost_pos.dir){
        case LEFT: calc_bitmap(ghost_bm_1, tile, MIRROR_V); break; 
        case UP: calc_bitmap(ghost_bm_up, tile, NORMAL); break; 
        case DOWN: calc_bitmap(ghost_bm_down, tile, NORMAL); break; 
        default: calc_bitmap(ghost_bm_1, tile, NORMAL); 
    }
    render_tile(&ghost_pos, tile);
}

inline void clear_pacman_pos(){
    render_tile(&player_pos, 0);
}

inline void clear_ghost_pos(){
    render_tile(&ghost_pos, 0);
}

void render_tile(position *pos, uint8_t *tile){

    uint8_t page_start = (pos->py) % 8;
    if(page_start){
        if(tile){
            uint8_t page_cont[TILE_SIZE];
            for(int i = 0; i < sizeof(page_cont); ++i){
                page_cont[i] = 0x0 | (tile[i] << (page_start));
            }
            sb_display_drawBitmap(pos->py/8,pos->px,1, TILE_SIZE, page_cont);

            for(int i = 0; i < sizeof(page_cont); ++i){
                page_cont[i] = 0x0 | (tile[i] >> (8 - page_start));
            }
            sb_display_drawBitmap(pos->py/8+1,pos->px,1, TILE_SIZE, page_cont);
         } else {
            sb_display_drawBitmap(pos->py/8,pos->px,1, TILE_SIZE, 0);
            sb_display_drawBitmap((pos->py/8)+1,pos->px,1, TILE_SIZE, 0);
         }
    } else {
        sb_display_drawBitmap((pos->py/8),pos->px,1, TILE_SIZE, tile);
    }

}

// ACTUALIZE GHOST DIR

static inline uint16_t euclidian_distance_sqr(uint8_t px1, uint8_t py1, uint8_t px2, uint8_t py2){
    uint16_t dx = px1 - px2;
    uint16_t dy = py1 - py2;
    return  dx * dx + dy * dy;
}

static inline uint16_t distance_ghost_pacman(){
    return euclidian_distance_sqr(ghost_pos.px, ghost_pos.py, player_pos.px, player_pos.py);
}

void calc_next_ghost_dir(){
    uint16_t distance = -1;
    if(ghost_vulnerable) distance = 0;
    uint8_t best_dir = 0;
    for(int8_t dir = 0; dir < 4; ++dir){
        ghost_pos.dir = dir;
        if(!next_pos_in_board(&ghost_pos)) continue;
        next_pos(8, &ghost_pos);
        if(pos_free(&ghost_pos)){
            uint16_t tmp = distance_ghost_pacman();
            if(!ghost_vulnerable && tmp < distance){
                distance = tmp;
                best_dir = dir;
            } else if(ghost_vulnerable && tmp > distance){
                distance = tmp;
                best_dir = dir;
            } 
        }
        next_pos(-8, &ghost_pos);
    }
    ghost_pos.dir = best_dir;
}

void update_board(void){
    set_speed();

    uint8_t page, col;
    switch(event){

        case EVENT_CHANGE_DIR: 
            if(state != HALT){
                break; 
            }

        case EVENT_ACT_P_POS:

           if(state == PLAY){
               clear_pacman_pos();
               if(next_pos(1, &player_pos)){
                    sb_7seg_showString("PA");
                    while(1);
                }
                render_pacman();
           } 

          state = PLAY;         
          //new tile reached
          if( !(player_pos.px % 8) && !(player_pos.py % 8)){
                //eat dot
                uint8_t dot= erase_dot(player_pos.px, player_pos.py);
                if(dot){
                    points += dot;
                    dots_left -= dot;
                    if( is_special_dot(player_pos.px, player_pos.py) ){
                        ghost_vulnerable = 0xFF;
                        be_vulnerable();
                    }
    
                }
                sb_7seg_showNumber(points);
                //set next dir
                player_pos.dir = player_pos.next_dir;

                if(next_pos_free(&player_pos)){
                } else { 
                    //sb_led_on(RED0);
                    state = HALT;
                    render_pacman();
                }

          }  
        break;

        case EVENT_ACT_G_POS: 
            clear_ghost_pos();
            if(next_pos(1, &ghost_pos)){
                sb_7seg_showString("GO");
                while(1);
            }
            render_ghost();

            if( !(ghost_pos.px % 8) && !(ghost_pos.py % 8)){
                uint8_t dot = erase_dot(ghost_pos.px, ghost_pos.py);
                dots_left -= dot;
                calc_next_ghost_dir();
            }

              break;

        default:;

    };

   // demask_buttons();
}
// active-low: low-Pegel (logisch 0; GND am Pin) =>  LED leuchtet

void calc_bitmap(const __flash char *bm, uint8_t *display_bm, uint8_t orient){
    for(uint8_t i = 0; i < TILE_SIZE; ++i) display_bm[i] = 0;
    

    for(uint8_t x = 0; x < TILE_SIZE; ++x){ 
        for(uint8_t y = 0; y < TILE_SIZE; ++y){
            if(orient == ROTATE_1){
                if(bm[(TILE_SIZE - 1 - x)*TILE_SIZE + y] == '1')
                    display_bm[x] |= (1 << y);
            } else if(orient == ROTATE_2){ 
                if(bm[(TILE_SIZE - 1 - y)*TILE_SIZE + TILE_SIZE-x-1] == '1')
                    display_bm[x] |= (1 << y);
            } else if(orient == ROTATE_3){
                if(bm[x*TILE_SIZE + TILE_SIZE - 1 - y] == '1')
                    display_bm[x] |= (1 << y);
            } else if(orient == MIRROR_V){
                if(bm[y * TILE_SIZE + (TILE_SIZE - 1 - x)] == '1')
                    display_bm[x] |= (1 << y);
            } else if(orient == MIRROR_H){
                if(bm[(TILE_SIZE -1 - y) * TILE_SIZE + x] == '1')
                    display_bm[x] |= (1 << y);
            } else {
                if(bm[y * TILE_SIZE + x] == '1')
                    display_bm[x] |= (1 << y);
            } 
        }
    } 


}

//ON WIN / ON LOOSE

void wait(){
    event = EVENT_WAIT;
}

void wait_time(uint16_t time){
     sb_timer_setAlarm((alarmcallback_t) wait, time, 0);
     cli();
     while( !event ) {
         sei();
         sleep_cpu();
         cli();
     }
     sei();
     event = 0;
    
}

void show_win(void){
    uint8_t set_leds = 0xFF;
    uint16_t time = 100;
    for(int i = 0; i <= 8; ++i){
        sb_led_setMask(set_leds);
        set_leds = set_leds << 1;
        wait_time(time);
    }
    uint8_t cursor = 7;
    for(uint8_t i = 0; i < 15; ++i){
        sb_led_setMask(set_leds ^ (1 << cursor));

        if(i < 7) --cursor;
        else ++cursor;
        wait_time(time);
    }
    uint8_t leds[] = {0x00, 0x81, 0xC3, 0xE7, 0xFF};
    for(int8_t i = 0; i < sizeof(leds); ++i){

        sb_led_setMask(leds[i]); //1000 0001
        wait_time(time);
    }
    for(int8_t i = sizeof(leds) - 2; i >= 0; --i){

        sb_led_setMask(leds[i]); //1000 0001
        wait_time(time);
    }


}

void show_loose(void){
    uint8_t set_leds = 0;
    uint16_t time = 500;
    sb_led_setMask(set_leds);
    const char loser[] = "LOSER";
    sb_display_fillScreen(NULL); // Clear display
    for(uint8_t i = 0; i < 20; ++i){
        sb_led_toggle(RED0);
        sb_led_toggle(RED1);
        sb_led_toggle(YELLOW0);
        sb_led_toggle(YELLOW1);
        if(!(i % 4)  || !((i+1)%4)) sb_display_fillScreen(NULL); // Clear display
        else sb_display_showStringWide(3,128/2-20,loser);
        wait_time(time);

    }

}



void main(void){
    sei();

    if(sb_display_available() != 0){
        sb_7seg_showString("--");
        while(1);
    }

    if(sb_display_enable() != 0){
        sb_7seg_showString("++");
        while(1);
    }

    init();

    set_sleep_mode(SLEEP_MODE_IDLE); /* Idle-Modus verwenden */
    sleep_enable();

    while(1){
        timer0_init();
        cli();
        while( !event ) {
            sei();
            sleep_cpu();
            cli();
        }
        sei();
        update_board();
        if(!ghost_pos.dead && distance_ghost_pacman() < DEADLY_DISTANCE){
            if(!ghost_vulnerable){
                deaktivate_timers();
                show_loose();
                init();
            } else {
                ghost_pos.dead = true;
                clear_ghost_pos();
                render_pacman();
                points += 20;
                sb_led_setMask(0);
            }
        } else if(!dots_left ){
            deaktivate_timers();
            show_win();
            init();
        }
        event = 0;
    }
    sleep_disable();

    while(1); // Stop loop
}    
