#ifndef SRC_INCLUDE_APP_LED_H_
#define SRC_INCLUDE_APP_LED_H_

typedef enum {
    CONTROL_LED_OFF = 0,
    CONTROL_LED_ON,
    CONTROL_LED_ON_OFF
} control_led_t;

void light_blink_start(uint8_t times, uint16_t ledOnTime, uint16_t ledOffTime);
void light_blink_stop(void);

void light_init(void);
void light_on(void);
void light_off(void);

//void led_on(uint32_t pin);
//void led_off(uint32_t pin);

#endif /* SRC_INCLUDE_APP_LED_H_ */
