#ifndef CONCAT_H
#define CONCAT_H

#include <stdint.h>

#define TILE_SIZE 8
#define BOARD_HEIGHT 8
#define BOARD_WIDTH 16
#define POS_PACMAN 0
#define POS_GHOST 127

#define false 0
#define true 1
#define DEADLY_DISTANCE (TILE_SIZE * TILE_SIZE / 2)


static const __flash uint8_t board[] = {
	0x80, 0x82, 
	0x3e, 0x28, 
	0x82, 0x7b, 
	0xbe, 0x20, 
	0x88, 0xaf, 
	0xbe, 0x28, 
	0x80, 0x8e, 
	0x2a, 0x20};

static const __flash uint8_t special_dots[] = {
	0x00, 0x01, 
	0x00, 0x00, 
	0x00, 0x00, 
	0x00, 0x10, 
	0x00, 0x00, 
	0x00, 0x00, 
	0x01, 0x00, 
	0x00, 0x10
};

uint8_t dots[] = {
	0x7f, 0x7d, 
	0xc1, 0xd7, 
	0x7d, 0x84, 
	0x41, 0xdf, 
	0x77, 0x50, 
	0x41, 0xd7, 
	0x7f, 0x71, 
	0xd5, 0xdf};


inline void get_tile(uint8_t *page, uint8_t *col, uint8_t bit, uint8_t i){
    *page = (i * 8 * TILE_SIZE + bit * TILE_SIZE)/(BOARD_WIDTH * TILE_SIZE);
    *col =  (i * 8 * TILE_SIZE + bit * TILE_SIZE)%(BOARD_WIDTH * TILE_SIZE);
}
//works only for tiles on tile_bordes (divisible / 8)
static inline uint8_t get_board_cont_from_px(uint8_t px, uint8_t py){
    uint16_t idx = ((py * 128)/8  + px);
    uint16_t bit = (uint16_t)((uint16_t)(idx/64)  * 64) ;
    bit = (uint16_t) (idx - bit)/8;   

    if(board[idx/64] & (1 << bit)) return 1;
    return 0;
}

inline uint8_t erase_dot(uint8_t px, uint8_t py){
    uint16_t idx = ((py * 128)/8  + px);
    uint16_t bit = (uint16_t)((uint16_t)(idx/64)  * 64) ;
    bit = (uint16_t) (idx - bit)/8;   
    uint8_t ret = 0;
    if(dots[idx/64] & (1 << bit)) ret =  1;
    dots[idx/64] &= ~(1 << bit);
    return ret;
}

uint8_t is_special_dot(uint8_t px, uint8_t py){
    uint16_t idx = ((py * 128)/8  + px);
    uint16_t bit = (uint16_t)((uint16_t)(idx/64)  * 64) ;
    bit = (uint16_t) (idx - bit)/8;   
    uint8_t ret = 0;
    if(special_dots[idx/64] & (1 << bit)) return true;
    return false;
}


#endif
