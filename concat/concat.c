#include "board.h"
#include "monsters.h"
#include <stdlib.h>
#include <stddef.h>
#include <display.h>
#include <7seg.h>
#include <timer.h>
#include <led.h>
#include <adc.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include "sqrt.h"

#define false 0
#define true 1
#define DEADLY_DISTANCE (TILE_SIZE * TILE_SIZE / 2)

typedef void(* alarmcallback_t )(void);


static volatile uint8_t event = 0;
uint16_t pacman_speed;
uint16_t ghost_speed;
position player_pos;
position ghost_pos;
static volatile STATE state;
uint8_t points = 0;
uint8_t dots_left = 0;

// global variable to count the number of overflows
volatile uint16_t ghost_of = 0;
volatile uint16_t pacmn_of = 0;

// TIMER0 overflow interrupt service routine
// called whenever TCNT0 overflows
ISR(TIMER0_OVF_vect) {
   // keep a track of number of overflows

     // keep a track of number of overflows
     ++ghost_of;
     ++pacmn_of;
             TCNT0 = 0;            // reset counter
         // check if the timer count reaches 53
         if (pacmn_of >= pacman_speed){
             TCNT0 = 0;            // reset counter
             pacmn_of = 0;     // reset overflow counter
             event = EVENT_ACT_P_POS;
         }
 
         else if (ghost_of >= ghost_speed){
             TCNT0 = 0;            // reset counter
             ghost_of = 0;     // reset overflow counter
             event = EVENT_ACT_G_POS;
         }
   
}

//ISR(TIMER1_OVF_vect) {
//    // keep a track of number of overflows
//    pacman_speed = 1;
//    ghost_speed = 1;  
//    ++ghost_of;
//    ++pacmn_of;
//    sb_led_toggle(YELLOW0);
//            TCNT0 = 0;            // reset counter
//        // check if the timer count reaches 53
//        if (pacmn_of >= pacman_speed){
//            sb_led_toggle(BLUE0);
//            TCNT0 = 0;            // reset counter
//            pacmn_of = 0;     // reset overflow counter
//            event = EVENT_ACT_P_POS;
//        }
//
//        else if (ghost_of >= ghost_speed){
//            sb_led_toggle(BLUE1);
//            TCNT0 = 0;            // reset counter
//            ghost_of = 0;     // reset overflow counter
//            event = EVENT_ACT_G_POS;
//        }
//        
//}

void deaktivate_timers(){

    TIMSK0 = 0;

}

void demask_buttons(){

    EIMSK |= (1 << INT0); /* demaskiere Interrupt 0 */
    EIMSK |= (1 << INT1); /* demaskiere Interrupt 1 */

}
void mask_buttons(){

    EIMSK &= ~(1 << INT0); /* maskiere Interrupt 0 */
    EIMSK &= ~(1 << INT1); /* maskiere Interrupt 1 */

}

void timer0_init(){
    //TIMER0
   // set up timer with no prescaling
//    TCCR0 |= (1 << CS00);
 
   // set up timer with prescaler = 1024
    //TCCR0B |= (1 << CS02)|(1 << CS00);
    TCCR0B |= (1 << CS01);

    // initialize counter
    TCNT0 = 0;

    // enable overflow interrupt
    TIMSK0 |= (1 << TOIE0); 
}


//void timer1_init(){
//   // set up timer with prescaler = 1024
//    TCCR1B |= (1 << CS02)|(1 << CS00);
//
//    // initialize counter
//    TCNT1 = 0;
//
//    // enable overflow interrupt
//    TIMSK1 |= (1 << TOIE1); 
//}



void init(){



    //Configuration auf fallende Flanke
    //Button 0
    DDRD  &= ~(1<< PD2); /* PD2 als Eingang nutzen... */
    PORTD |= (1 << PD2); /* pull-up-Widerstand aktivieren */
    EICRA &= ~(1<< ISC00); /* ISC00 löschen */
    EICRA |= (1 << ISC01); /* ISC10 setzen */
    EIMSK |= (1 << INT0); /* demaskiere Interrupt 0 */
    //Button 1
    DDRD  &= ~(1<< PD3); /* PD2 als Eingang nutzen... */
    PORTD |= (1 << PD3); /* pull-up-Widerstand aktivieren */
    EICRA &= ~(1<< ISC10); /* ISC10 löschen */
    EICRA |= (1 << ISC11); /* ISC11 setzen */
    EIMSK |= (1 << INT1); /* demaskiere Interrupt 1 */    

    state = START;
    dots_left = 0;
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
                sb_display_drawBitmapFromFlash(page, col, 1, TILE_SIZE, dot); 
                ++dots_left;
            }

            if(i*8 + bit == POS_PACMAN){
                player_pos.px = (i * 64 + bit * 8) % 128; 
                player_pos.py = (i * 64 + bit * 8) / 128 * 8; 
                player_pos.next_dir = 0;
                player_pos.dir = RIGHT;
                player_pos.move = 0; 
                render_pacman();
                erase_dot(player_pos.px, player_pos.py);
                --dots_left;
                //sb_display_drawBitmapFromFlash(page, col, 1, TILE_SIZE, pacman); 
            } else if(i*8 + bit == POS_GHOST){
                ghost_pos.px = (i * 64 + bit * 8) % 128; 
                ghost_pos.py = (i * 64 + bit * 8) / 128 * 8; 
                ghost_pos.next_dir = 0;
                ghost_pos.dir = LEFT;
                ghost_pos.move = 0; 
                render_ghost();
                calc_next_ghost_dir();
            }
        }

    }
}

void set_speed(){

 
     int16_t poti = sb_adc_read(POTI);
 
     float bn = 1.0 - (float)(poti)/(float)(MAX_DEV_VAL);
     //pacman_speed = MAX_SPEED + (MIN_SPEED - MAX_SPEED) * bn;
     pacman_speed = (MAX_DEV_VAL - poti)*2;
     ghost_speed = pacman_speed * 1.3;
}

ISR(ADC_vect){
    event = 72;
    set_speed();
}

ISR(INT0_vect) {
    sb_led_toggle(GREEN0); 
    event = EVENT_CHANGE_DIR;
    if(state == START){
        state = HALT;
        timer0_init();
        return;
    }
    player_pos.next_dir += 1;
    player_pos.next_dir %= 4;
}

ISR(INT1_vect) { 
    sb_led_toggle(GREEN1); 
    event = EVENT_CHANGE_DIR;
    if(state == START){
        state = HALT;
        invoke_ghost();
        return;
    }
    player_pos.next_dir -= 1;
    if(player_pos.next_dir < 0) player_pos.next_dir += 4;
}

void actualize_player_pos(void){
    event = EVENT_ACT_P_POS;
}

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

void render_pacman(){
    uint8_t tile[TILE_SIZE];
    switch(player_pos.dir){
        case DOWN: calc_bitmap(pacman_bm, tile, ROTATE_1); break;
        case LEFT: calc_bitmap(pacman_bm, tile, ROTATE_2); break;
        case UP: calc_bitmap(pacman_bm, tile, ROTATE_3); break;
        default:  calc_bitmap(pacman_bm, tile, NORMAL);
    }
    render_tile(&player_pos, tile);
}
void render_ghost(){
    uint8_t tile[TILE_SIZE];
    switch(ghost_pos.dir){
        case LEFT: calc_bitmap(ghost_bm_1, tile, MIRROR_V); break; 
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
    uint8_t best_dir = 0;
    for(int8_t dir = 0; dir < 4; ++dir){
        ghost_pos.dir = dir;
        if(!next_pos_in_board(&ghost_pos)) continue;
        next_pos(8, &ghost_pos);
        if(pos_free(&ghost_pos)){
            uint16_t tmp = distance_ghost_pacman();
            if(tmp < distance){
                sb_led_toggle(YELLOW1);
                distance = tmp;
                best_dir = dir;
            } 
        }
        next_pos(-8, &ghost_pos);
    }
    ghost_pos.dir = best_dir;
}

void invoke_ghost(){
    //timer0_init();

}


void update_board(void){
    set_speed();
    mask_buttons();
    uint8_t page, col;
    switch(event){
        //changed dir
        case EVENT_CHANGE_DIR: 
            if(state != HALT){
                break; 
            }
        //move
        case EVENT_ACT_P_POS:
           sb_led_off(RED0);

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
                    points += dot;
                    dots_left -= dot;
                    sb_7seg_showNumber(points);
                    //set next dir
                    player_pos.dir = player_pos.next_dir;

                    if(next_pos_free(&player_pos)){
                        //player_pos.move = (ALARM *)sb_timer_setAlarm((alarmcallback_t) actualize_player_pos, 1000.0 * (pacman_speed/(float)TILE_SIZE), 0);
                    } else { 
                        sb_led_on(RED0);
                        state = HALT;
                        render_pacman();
                    }

              } else { 
                     //player_pos.move = (ALARM *) sb_timer_setAlarm((alarmcallback_t) actualize_player_pos, 1000.0 * (pacman_speed/(float)TILE_SIZE), 0);
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

    demask_buttons();
}
// active-low: low-Pegel (logisch 0; GND am Pin) =>  LED leuchtet

void calc_bitmap(const __flash char *bm, uint8_t *display_bm, BIAS orient){
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

    sb_led_on(RED0);
    set_sleep_mode(SLEEP_MODE_IDLE); /* Idle-Modus verwenden */
    sleep_enable();

    while(1){
        cli();
        while( !event ) {
            sei();
            sleep_cpu();
            cli();
        }
        sei();
        //sb_7seg_showNumber(event);
        update_board();
        if(!dots_left || distance_ghost_pacman() < DEADLY_DISTANCE){
            deaktivate_timers();
            show_win();
            init();
        }
        event = 0;
    }
    sleep_disable();





    while(1); // Stop loop
}    
