#include "own_led.h"
#include <avr/io.h>
// active-low: low-Pegel (logisch 0; GND am Pin) → LED leuchtet

volatile uint8_t *dir_ports[] = { &DDRD, &DDRD, &DDRD, &DDRD, &DDRB, &DDRB, &DDRC ,&DDRC };
volatile uint8_t *data_ports[] = { &PORTD, &PORTD, &PORTD, &PORTD, &PORTB, &PORTB, &PORTC ,&PORTC };
uint8_t pins[] = {PD6, PD5, PD4, PD7, PB0, PB1, PC3, PC2};

static void init(void){
    static uint8_t initDone = 0;
    if(initDone == 1) return;
    initDone = 1;    
    for(uint8_t p = 0; p < sizeof(dir_ports)/sizeof(uint8_t *); ++p){
        *dir_ports[p] |= (1 << pins[p]); /* =0x08; PC3 als Ausgang nutzen... */
        *data_ports[p] |= (1 << pins[p]); 
    }
}

uint8_t sb_led_on(LED led){
    init();
    if(led > BLUE1) return -1;
    *data_ports[led] &= ~(1 << pins[led]); 
    return 0;
}

uint8_t sb_led_off(LED led){
    init();
    if(led > BLUE1) return -1;
    *data_ports[led] |= (1 << pins[led]); 
    return 0;
}


void sb_led_setAll(uint8_t setting ){
    init();
    for(uint8_t p = 0; p < 8; ++p){
        if(setting  & (1 << p)){
            *data_ports[p] &= ~(1 << pins[p]); 
        } else {
            *data_ports[p] |= (1 << pins[p]); 
        }
    }
}

uint8_t sb_led_toggle(uint8_t led ){
    init();
    if(led > BLUE1) return -1;
    *data_ports[led] ^=  (1 << pins[led]);
    return 0;
}


void main(void){
/*    for(uint8_t i = 0; i < 8; ++i){
        sb_led_on(i);
        for(volatile uint32_t j = 0; j < 20000; ++j); 
    }
    for(uint8_t i = 0; i < 8; ++i){
        sb_led_off(i);
        for(volatile uint32_t j = 0; j < 20000; ++j); 
    }*/
    sb_led_setAll(0x81);
    sb_led_toggle(0);
    sb_led_toggle(1);
    sb_led_toggle(2);
//    DDRC |= (1 << PC2); /* =0x08; PC3 als Ausgang nutzen... */
//    PORTC |= (1 << PC2); /* ...und auf 1 (=high) setzen */
//    PORTC &= ~(1 << PC2); /* ...und auf 1 (=high) setzen */
    while(1);
}

