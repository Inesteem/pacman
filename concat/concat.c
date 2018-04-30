#include "board.h"
#include "monsters.h"
#include <stdlib.h>
#include <stddef.h>
#include <display.h>
#include <7seg.h>
#include <timer.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>


// active-low: low-Pegel (logisch 0; GND am Pin) → LED leuchtet

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
    // Square Bitmap
    //sb_display_fillScreenFromFlash(board);
    /*
    for(uint8_t h = 0; h < BOARD_HEIGHT; ++h){
        for(uint8_t w = 0; w < BOARD_WIDTH; ++w){
            if(board[h * BOARD_WIDTH + w] == '1')
                sb_display_drawBitmapFromFlash(h, w * TILE_SIZE, 1, TILE_SIZE, wall);
                  
        } 
    }
    */
    for(uint8_t i = 0; i < sizeof(board); ++i){
        for(uint8_t bit = 0; bit < 8; ++bit){
            if(board[i] & (1 << bit)){
                uint8_t page = (i * 8 * TILE_SIZE + bit * TILE_SIZE)/(BOARD_WIDTH * TILE_SIZE);
                uint8_t col =  (i * 8 * TILE_SIZE + bit * TILE_SIZE)%(BOARD_WIDTH * TILE_SIZE);
                sb_display_drawBitmapFromFlash(page, col, 1, TILE_SIZE, wall); 

            }
        }

    }
    sb_7seg_showString("fi");
   
    //sb_display_drawBitmapFromFlash(0, 5, 2, 8, bitmap);
/*
int8_t sb_display_drawBitmap    (   uint8_t     pageStart,
        uint8_t     colStart,
        uint8_t     pageCount,
        uint8_t     colCount,
        const uint8_t *     contents 
    )   
*/
    while(1); // Stop loop
}    
