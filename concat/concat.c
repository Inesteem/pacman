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
typedef void(* alarmcallback_t )(void);


//0 = RIGHT, 1 = DOWN, 2 = LEFT, 3 UP

static volatile uint8_t event = 0;
float pacman_speed = MIN_SPEED;
float ghost_speed = MIN_SPEED;
position player_pos;
position ghost_pos;
static volatile STATE state = START;
uint8_t points = 0;


void render_tile(position *pos, const __flash char *bm, uint8_t pacman);
void calc_bitmap(const __flash char *bm, uint8_t *display_bm, uint8_t orient);

void demask_buttons(){

    EIMSK |= (1 << INT0); /* demaskiere Interrupt 0 */
    EIMSK |= (1 << INT1); /* demaskiere Interrupt 1 */

}
void mask_buttons(){

    EIMSK &= ~(1 << INT0); /* maskiere Interrupt 0 */
    EIMSK &= ~(1 << INT1); /* maskiere Interrupt 1 */

}
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

    for(uint8_t i = 0; i < sizeof(board); ++i){
        for(uint8_t bit = 0; bit < 8; ++bit){
            uint8_t page, col;
            get_tile(&page, &col, bit, i);
            if(i*8 + bit == POS_PACMAN){
                player_pos.px = (i * 64 + bit * 8) % 128; 
                player_pos.py = (i * 64 + bit * 8) / 128 * 8; 
                player_pos.next_dir = 0;
                player_pos.dir = RIGHT;
                player_pos.move = 0; 
                render_tile(&player_pos, pacman_bm, 1);
                erase_dot(player_pos.px, player_pos.py);
                //sb_display_drawBitmapFromFlash(page, col, 1, TILE_SIZE, pacman); 
            } else if(board[i] & (1 << bit)){
                sb_display_drawBitmapFromFlash(page, col, 1, TILE_SIZE, wall); 
            } else if(dots[i] & (1 << bit)){
                sb_display_drawBitmapFromFlash(page, col, 1, TILE_SIZE, dot); 
                ++points;
            }
            if(i*8 + bit == POS_GHOST){
                ghost_pos.px = (i * 64 + bit * 8) % 128; 
                ghost_pos.py = (i * 64 + bit * 8) / 128 * 8; 
                ghost_pos.next_dir = 0;
                ghost_pos.dir = LEFT;
                ghost_pos.move = 0; 
                render_tile(&ghost_pos, ghost_bm, 0);

            }
        }

    }
}

void set_speed(){

 
     int16_t poti = sb_adc_read(POTI);
 
     float bn = 1.0 - (float)(poti)/(float)(MAX_DEV_VAL);
     pacman_speed = MAX_SPEED + (MIN_SPEED - MAX_SPEED) * bn;
     ghost_speed = MAX_SPEED + 100 + (MIN_SPEED - MAX_SPEED) * bn;
}

ISR(ADC_vect){
    event = 72;
    set_speed();
}

ISR(INT0_vect) { 
    event = 1;
    if(state == START){
        sb_led_off(RED0);
        state = HALT;
        return;
    }
    player_pos.next_dir += 1;
    player_pos.next_dir %= 4;
}

ISR(INT1_vect) { 
    event = 1;
    if(state == START){
        sb_led_off(RED0);
        state = HALT;
        return;
    }
    player_pos.next_dir -= 1;
    if(player_pos.next_dir < 0) player_pos.next_dir += 4;
}

void actualize_ghost_pos(void){
    event = 4;
}




void actualize_player_pos(void){
    event = 2;
}

uint8_t next_pos_free(position *pos){
    switch(pos->dir){
        case RIGHT:
            if(pos->px + TILE_SIZE >= 127) return 0; 
            break;
        case DOWN:
            if(pos->py + TILE_SIZE >= 63) return 0; 
            break;
        case LEFT:
            if(pos->px < TILE_SIZE ) return 0; 
            break;
        case UP:
            if(pos->py < TILE_SIZE) return 0; 
            break;
        default: return -1;

    }

    if(next_pos(8, pos)){
        sb_7seg_showString("cf");
        while(1);
    }
    uint8_t ret = !get_board_cont_from_px(pos->px, pos->py);
    if(next_pos(-8, pos)){
        sb_7seg_showString("CF");
        while(1);
    }
    return ret;
    
}

void render_tile(position *pos, const __flash char *bm, uint8_t pacman){

   uint8_t tile[TILE_SIZE];
   if(pacman == 1) calc_bitmap(bm, tile, pos->dir); 
   else calc_bitmap(bm, tile, RIGHT); 

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
inline void print_dir_od(void){
    if(player_pos.dir == 0) sb_7seg_showString("RE");
    else if(player_pos.dir == 1) sb_7seg_showString("DO");
    else if(player_pos.dir == 2) sb_7seg_showString("LE");
    else if(player_pos.dir == 3) sb_7seg_showString("UP");
    else sb_7seg_showString("??");
}

void update_board(void){
    set_speed();
    mask_buttons();
    uint8_t page, col;
    switch(event){
        //changed dir
        case 1: 
            if(state != HALT){
                break; 
            }
        //move
        case 2:
           if(state == PLAY){
                render_tile(&player_pos, 0, 1);
                if(next_pos(1, &player_pos)){
                    sb_7seg_showString("UB");
                    while(1);
                }
               render_tile(&player_pos, pacman_bm,1);
            }
    //        get_tile(&page, &col, bit, i);
              state = PLAY;         
              if( !(player_pos.px % 8) && !(player_pos.py % 8)){
                    points -= erase_dot(player_pos.px, player_pos.py);
                    sb_7seg_showNumber(points);
                    
                    player_pos.dir = player_pos.next_dir;
                    //print_dir_od();
                    if(next_pos_free(&player_pos)){
                        player_pos.move = (ALARM *)sb_timer_setAlarm((alarmcallback_t) actualize_player_pos, 1000.0 * (pacman_speed/(float)TILE_SIZE), 0);
                    } else { 
                        state = HALT;
                        render_tile(&player_pos, pacman_bm,1);
                        //sb_7seg_showString("ha");
                    }


              } else { 
                     player_pos.move = (ALARM *) sb_timer_setAlarm((alarmcallback_t) actualize_player_pos, 1000.0 * (pacman_speed/(float)TILE_SIZE), 0);
              }

                //GHOST

                render_tile(&ghost_pos, 0, 1);
                render_tile(&ghost_pos, ghost_bm, 1);
              if( !(ghost_pos.px % 8) && !(ghost_pos.py % 8)){
//TODO:::::
}

              break;

        default:;

    };

    demask_buttons();
}
// active-low: low-Pegel (logisch 0; GND am Pin) → LED leuchtet


void calc_bitmap(const __flash char *bm, uint8_t *display_bm, uint8_t orient){
    for(uint8_t i = 0; i < TILE_SIZE; ++i) display_bm[i] = 0;
    

    for(uint8_t x = 0; x < TILE_SIZE; ++x){ 
        for(uint8_t y = 0; y < TILE_SIZE; ++y){
            if(orient == DOWN){
                //bm_tmp[y * TILE_SIZE + x] = bm[(TILE_SIZE - 1 - x)*TILE_SIZE + y];
                if(bm[(TILE_SIZE - 1 - x)*TILE_SIZE + y] == '1')
                    display_bm[x] |= (1 << y);
            } else if(orient == LEFT){ 
                //bm_tmp[y * TILE_SIZE + x] = bm[(TILE_SIZE - 1 - y)*TILE_SIZE + TILE_SIZE| -x-1];
                if(bm[(TILE_SIZE - 1 - y)*TILE_SIZE + TILE_SIZE-x-1] == '1')
                    display_bm[x] |= (1 << y);
            } else if(orient == UP){
                //bm_tmp[y * TILE_SIZE + x] = bm[x*TILE_SIZE + TILE_SIZE - 1 - y];
                if(bm[x*TILE_SIZE + TILE_SIZE - 1 - y] == '1')
                    display_bm[x] |= (1 << y);
            } else {
                //bm_tmp[y * TILE_SIZE + x] = bm[y*TILE_SIZE + x];
                if(bm[y * TILE_SIZE + x] == '1')
                    display_bm[x] |= (1 << y);
            } 
        }
    } 


}
void wait(void){}
void show_cursor(uint8_t leds, uint8_t cursor){
    sb_led_setMask(leds ^ cursor);
}

void show_win(void){
    uint8_t set_leds = 0xFF;
    uint32_t time = 500;
    for(int i = 0; i <= 8; ++i){
        sb_led_setMask(set_leds);
        set_leds = set_leds << 1;
        sb_timer_setAlarm((alarmcallback_t) wait, time, 0);
    }
    uint8_t cursor = 7;
    for(uint8_t i = 0; i < 15; ++i){
        show_cursor(set_leds, (1 << cursor));

        if(i < 7) --cursor;
        else ++cursor;
        sb_timer_setAlarm((alarmcallback_t) wait, time, 0);
    }
    uint8_t leds[] = {0x00, 0x81, 0xC3, 0xE7, 0xFF};
    for(int8_t i = 0; i < sizeof(leds); ++i){

        sb_led_setMask(leds[i]); //1000 0001
        sb_timer_setAlarm((alarmcallback_t) wait, time, 0);
    }
    for(int8_t i = sizeof(leds) - 2; i >= 0; --i){

        sb_led_setMask(leds[i]); //1000 0001
        sb_timer_setAlarm((alarmcallback_t) wait, time, 0);
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
    sb_7seg_showString("ok");
    sb_display_fillScreen(NULL); // Clear display
    set_speed();
    //ADCSRA = 0x8F;          // Enable the ADC and its interrupt feature
   // ADCSRA |= 1<<ADSC;  
    init();
/*
    while(1) {
    player_pos.px = 0;
    player_pos.py = 0;
    uint8_t bm[TILE_SIZE];
    
    for(volatile uint8_t i = 0; i < 64 * 2 - 8; ++i){
        sb_7seg_showNumber(i);
        render_tile(&player_pos, 0);
        if(i < 64 - 8 ) player_pos.py += 1;
        else player_pos.py -= 1;
        calc_bitmap(pacman_bm, bm, ); 
        render_tile(&player_pos, bm);
        for(volatile uint32_t j = 0; j < 100000; ++j);
    }
    }
*/



    sb_7seg_showString("00");
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
        if(!points){
            show_win();
            init();
        }
        event = 0;
    }
    sleep_disable();





    while(1); // Stop loop
}    
