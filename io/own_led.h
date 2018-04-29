#ifndef OWN_LED_H
#define OWN_LED_H
/* fixed-width Datentypen einbinden (im Header verwendet) */
#include <stdint.h>
/* LED-Typ */
typedef enum { RED0=0, YELLOW0=1, GREEN0=2, BLUE0=3, RED1=4 , YELLOW1=5,  GREEN1=6,  BLUE1=7  } LED;
/* Funktion zum Aktivieren einer bestimmten LED */
uint8_t sb_led_on(LED led);

static void init(void);


#endif
