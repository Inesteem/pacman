#include "ampel.h"
#include <7seg.h>
#include <timer.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
// active-low: low-Pegel (logisch 0; GND am Pin) → LED leuchtet



typedef void(* alarmcallback_t )(void);

static volatile uint8_t event = 0;
static volatile uint8_t zaehler = 0;
static STATE state = START;
static ALARM *alrm;    


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

    event = 0;
    zaehler = 42;
}

    
//ISR(INT0_vect) { event = 1; } //FG_Button
ISR(INT0_vect) { 
    event = 42;  

    if(alrm){
        if(sb_timer_cancelAlarm(alrm) == -1){
            event = 50;
        }   
    }
    switch(state){
        case FG_ROT:
            state = FG_ROT_W;
            break;
        case AUTO_GELBROT:
            state = AUTO_GELBROT_W;
            break;
        case AUTO_GRUEN:
            state = AUTO_GRUEN_W;
            break;
        case AUTO_GRUEN_F:
            state = FG_WARTEN;
            break;

        default: event = 58;
    }
} //FG_Button

ISR(INT1_vect) { 
    if(state != START){
        event = 99;
        return;
    }
    event = 1; 
    state = FG_ROT; 
} //Zaehler_Button

//ISR(TIMER1_COMPA_vect) { event = 1; }


//ALARM * sb_timer_setAlarm (alarmcallback_t callback, uint16_t alarmtime, uint16_t cycle);

//int8_t sb_timer_cancelAlarm (ALARM *alrm);

void FGR_TO_AGR(void){
    if(state != FG_ROT){
        event = 98;
        return;
    }
    state = AUTO_GELBROT;
    event = 2;
}

void AGR_TO_AG(void){
    if(state != AUTO_GELBROT){
        event = 97;
        return;
    }
    state = AUTO_GRUEN;
    event = 3;
}

void AG_TO_AGF(void){
    if(state != AUTO_GRUEN){ 
        event = 96;
        return;
    }
    state = AUTO_GRUEN_F;
    event = 4;
}

void FGRW_TO_AGRW(void){
    if(state != FG_ROT_W){ 
        event = 95;
        return;
    }
    state = AUTO_GELBROT_W;
    event = 5;
}

void AGRW_TO_AGW(void){
    if(state != AUTO_GELBROT_W){ 
        event = 94;
        return;
    }
    state = AUTO_GRUEN_W;
    event = 6;
}

void AGW_TO_FGW(void){
    if(state != AUTO_GRUEN_W){ 
        event = 92;
        return;
    }
    state = FG_WARTEN;
    event = 8;
}


void FGW_TO_AG(void){
    if(state != FG_WARTEN){ 
        event = 93;
        return;
    }
    state = AUTO_GELB;
    event = 7;
}


void AG_TO_AR(void){
    if(state != AUTO_GELB){ 
        event = 91;
        return;
    }
    state = AUTO_ROT;
    event = 9;
}

void AR_TO_FGG(void){
    if(state != AUTO_ROT){ 
        event = 90;
        return;
    }
    state = FG_GRUEN;
    event = 10;
}


void FGG_TO_FGR(void){
    if(state != FG_GRUEN){ 
        event = 89;
        return;
    }
    state = FG_ROT;
    event = 11;
}



void set_state(void){
    sb_led_setMask(0x0);
    alrm = 0;
    switch(state){
        case FG_ROT:
            sb_led_setMask((1 << A_LED_ROT) | (1 << FG_LED_ROT));
            alrm = sb_timer_setAlarm((alarmcallback_t) FGR_TO_AGR, 2000, 0); 
            break;
        case FG_ROT_W:
            sb_led_setMask((1 << A_LED_ROT) | (1 << FG_LED_ROT) | (1 << FG_LED_SIGNAL));
            alrm = sb_timer_setAlarm((alarmcallback_t) FGRW_TO_AGRW, 2000, 0); 
            break;
        case AUTO_GELBROT:
            sb_led_setMask((1 << A_LED_ROT) | (1 << A_LED_GELB) | (1 << FG_LED_ROT));
            alrm = sb_timer_setAlarm((alarmcallback_t) AGR_TO_AG, 1000, 0); 
            break;
        case AUTO_GELBROT_W:
            sb_led_setMask((1 << A_LED_ROT) | (1 << A_LED_GELB) | (1 << FG_LED_ROT) | (1 << FG_LED_SIGNAL));
            alrm = sb_timer_setAlarm((alarmcallback_t) AGRW_TO_AGW, 1000, 0); 
            break;
        case AUTO_GRUEN:
            sb_led_setMask((1 << A_LED_GRUEN) | (1 << FG_LED_ROT));
            alrm = sb_timer_setAlarm((alarmcallback_t) AG_TO_AGF, 5000, 0); 
            break;
        case AUTO_GRUEN_W:
            sb_led_setMask((1 << A_LED_GRUEN) | (1 << FG_LED_ROT) | (1 << FG_LED_SIGNAL));
            alrm = sb_timer_setAlarm((alarmcallback_t) AGW_TO_FGW, 5000, 0); 
            break;
        case AUTO_GRUEN_F:
            sb_led_setMask((1 << A_LED_GRUEN) | (1 << FG_LED_ROT));
            break;
        case FG_WARTEN:
            sb_led_setMask((1 << A_LED_GRUEN) | (1 << FG_LED_ROT) | (1 << FG_LED_SIGNAL));
            alrm = sb_timer_setAlarm((alarmcallback_t) FGW_TO_AG, 5000, 0); 
            break;
        case AUTO_GELB:
            sb_led_setMask((1 << A_LED_GELB) | (1 << FG_LED_ROT));
            alrm = sb_timer_setAlarm((alarmcallback_t) AG_TO_AR, 1000, 0); 
            break;
        case AUTO_ROT:
            sb_led_setMask((1 << A_LED_ROT) | (1 << FG_LED_ROT));
            alrm = sb_timer_setAlarm((alarmcallback_t) AR_TO_FGG, 2000, 0); 
            break;
        case FG_GRUEN:
            sb_led_setMask((1 << A_LED_ROT) | (1 << FG_LED_GRUEN));
            alrm = sb_timer_setAlarm((alarmcallback_t) FGG_TO_FGR, 5000, 0); 
            break;
        default:;
    }

}


void main(void){
    init();
    sei();
    sb_7seg_showNumber(42); 
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
        sb_7seg_showNumber(event); 
        set_state();
        event = 0;
    }
    sleep_disable();
}


/*
volatile uint8_t *dir_ports[] = { &DDRD, &DDRD, &DDRD, &DDRD, &DDRB, &DDRB, &DDRC ,&DDRC };
volatile uint8_t *data_ports[] = { &PORTD, &PORTD, &PORTD, &PORTD, &PORTB, &PORTB, &PORTC ,&PORTC };
uint8_t pins[] = {PD6, PD5, PD4, PD7, PB0, PB1, PC3, PC2};

static void init(void){
    static uint8_t initDone = 0;
    if(initDone == 1) return;
    initDone = 1;    
    for(uint8_t p = 0; p < sizeof(dir_ports)/sizeof(uint8_t *); ++p){
        *dir_ports[p] |= (1 << pins[p]); 
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
    sb_led_setAll(0x81);
    sb_led_toggle(0);
    sb_led_toggle(1);
    sb_led_toggle(2);
    while(1);
}
*/
