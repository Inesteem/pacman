#ifndef AMPEL_H
#define AMPEL_H
/* fixed-width Datentypen einbinden (im Header verwendet) */
#include <stdint.h>
#include <led.h>

#define A_LED_ROT RED0
#define A_LED_GELB YELLOW0
#define A_LED_GRUEN GREEN0

#define FG_LED_ROT RED1
#define FG_LED_GRUEN GREEN1
#define FG_LED_SIGNAL BLUE1

#define FG_BUTTON BUTTON0

typedef enum { 
        START, 
        AUTO_ROT, 
        AUTO_GELBROT, 
        AUTO_GELBROT_W, 
        AUTO_GELB, 
        AUTO_GRUEN, 
        AUTO_GRUEN_W, 
        AUTO_GRUEN_F, 
        FG_WARTEN, 
        FG_ROT, 
        FG_ROT_W, 
        FG_GRUEN, 
        END
    } STATE;











/* LED-Typ */
/*typedef enum { RED0=0, YELLOW0=1, GREEN0=2, BLUE0=3, RED1=4 , YELLOW1=5,  GREEN1=6,  BLUE1=7  } LED;
uint8_t sb_led_on(LED led);

static void init(void);
*/

#endif
