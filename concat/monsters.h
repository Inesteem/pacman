#ifndef MONSTERS_H
#define MONSTERS_H

#include <stdint.h>

#define MIN_SPEED 1.0
#define MAX_SPEED 0.1
#define MAX_DEV_VAL 0x3FF
#define RIGHT 0
#define DOWN 1
#define LEFT 2
#define UP 3

#define EVENT_CHANGE_DIR    1
#define EVENT_ACT_P_POS     2
#define EVENT_ACT_G_POS     4
#define EVENT_WAIT          5



typedef enum { 
        NORMAL,
        ROTATE_1,
        ROTATE_2,
        ROTATE_3, 
        MIRROR_V,
        MIRROR_H
    } BIAS;





typedef enum { 
        START,
        HALT, 
        PLAY,
        END
    } STATE;




typedef struct {
    int8_t dir;
    int8_t next_dir;
    uint8_t px; 
    uint8_t py;
    void *move;
} position;

inline void get_tile_from_pos(uint8_t *page, uint8_t *col, position *pos){
    *page = pos->py/8;
    *col =  pos->px;
} 

inline int8_t next_pos(int8_t steps, position *pos){

    switch(pos->dir){
        case RIGHT: 
            pos->px += steps;
            break;
        case DOWN:
            pos->py += steps;
            break;
        case LEFT:
            pos->px -= steps;
            break;
        case UP:
            pos->py -= steps;
            break;
        default: return -1;
    }       
    return 0; 
            
}    

static const __flash uint8_t wall[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF};
 
static const __flash uint8_t dot[] = {
    0x00, 0x18, 0x3C, 0x66, 
    0x66, 0x3C, 0x18, 0x00};

static const __flash uint8_t dot_filled[] = {
    0x00, 0x18, 0x3C, 0x7E, 
    0x7E, 0x3C, 0x18, 0x00};

//static const __flash uint8_t pacman[] = {
//    0x3C, 0x7E, 0xFF, 0xFF, 
//    0xFF, 0xFF, 0x76, 0x34};

static const __flash char pacman_bm_1[] = 
   "00111100"
   "01111110"
   "11111111"
   "11111111"
   "11111111"
   "11111111"
   "01111110"
   "00111100";


static const __flash char pacman_bm_2[] = 
   "00111100"
   "01111110"
   "11111111"
   "11000000"
   "11100000"
   "11111111"
   "01111110"
   "00111100";



static const __flash char pacman_bm_3[] = 
   "00111100"
   "01111110"
   "11100000"
   "11000000"
   "11100000"
   "11111111"
   "01111110"
   "00111100";


static const __flash char pacman_bm_4[] = 
   "00111100"
   "01111111"
   "11110000"
   "11000000"
   "11100000"
   "11110000"
   "01111111"
   "00111100";




static const __flash char ghost_bm_1[] = 
   "00111100"
   "01111110"
   "11101101"
   "11001001"
   "11111111"
   "11011101"
   "11001101"
   "10001001";

static const __flash char ghost_bm_up[] = 
   "00111100"
   "01011010"
   "10011001"
   "11111111"
   "11111111"
   "11011011"
   "10011011"
   "10001001";


static const __flash char ghost_bm_down[] = 
   "00111100"
   "01111110"
   "11011011"
   "10011001"
   "11111111"
   "11011011"
   "10011011"
   "10010001";




  
void render_tile(position *pos, uint8_t *bm);
void render_ghost();
void render_pacman();
void calc_bitmap(const __flash char *bm, uint8_t *display_bm, uint8_t orient);
void set_speed();
void invoke_ghost();
void calc_next_ghost_dir();
#endif
