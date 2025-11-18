#include "tl_common.h"
#include "zcl_include.h"

#include "app_main.h"

static void led_on(uint32_t pin)
{
    drv_gpio_write(pin, LED_ON);
}

static void led_off(uint32_t pin)
{
    drv_gpio_write(pin, LED_OFF);
}

static void led_set_control() {

//    printf("led_control: %d\r\n", relay_settings.led_control);

    switch(relay_settings.led_control) {
        case CONTROL_LED_OFF:
            led_off(LED_GPIO);
            break;
        case CONTROL_LED_ON:
            led_on(LED_GPIO);
            break;
        case CONTROL_LED_ON_OFF:
            if (get_relay_status(0)) led_on(LED_GPIO);
            else led_off(LED_GPIO);
            break;
        default:
            break;
    }
}

void light_on(void)
{
    if(!g_appCtx.timerLedEvt && relay_settings.led_control != CONTROL_LED_OFF) led_on(LED_GPIO);
}

void light_off(void)
{
    if(!g_appCtx.timerLedEvt && relay_settings.led_control != CONTROL_LED_ON) led_off(LED_GPIO);
}

void light_init(void)
{
    led_off(LED_GPIO);

}

int32_t zclLightTimerCb(void *arg)
{
    uint32_t interval = 0;

    if(g_appCtx.sta == g_appCtx.oriSta){
        g_appCtx.times--;
        if(g_appCtx.times <= 0){
            led_set_control();
            g_appCtx.timerLedEvt = NULL;
            return -1;
        }
    }

    g_appCtx.sta = !g_appCtx.sta;
    if(g_appCtx.sta){
        led_on(LED_GPIO);
//        light_on();
        interval = g_appCtx.ledOnTime;
    }else{
        led_off(LED_GPIO);
//        light_off();
        interval = g_appCtx.ledOffTime;
    }

    return interval;
}

void light_blink_start(uint8_t times, uint16_t ledOnTime, uint16_t ledOffTime) {

//    printf("light_blink_start. times: %d\r\n", times);

    uint32_t interval = 0;
    g_appCtx.times = times;
    zcl_onOffAttr_t *pOnoff = zcl_onOffAttrsGet();

    if(!g_appCtx.timerLedEvt){
        g_appCtx.oriSta = pOnoff->onOff;
        if(g_appCtx.oriSta){
            led_off(LED_GPIO);
//            light_off();
            g_appCtx.sta = 0;
            interval = ledOffTime;
        }else{
            led_on(LED_GPIO);
//            light_on();
            g_appCtx.sta = 1;
            interval = ledOnTime;
        }
        g_appCtx.ledOnTime = ledOnTime;
        g_appCtx.ledOffTime = ledOffTime;

        g_appCtx.timerLedEvt = TL_ZB_TIMER_SCHEDULE(zclLightTimerCb, NULL, interval);
    }
}

void light_blink_stop(void) {

//    printf("light_blink_stop\r\n");

    if(g_appCtx.timerLedEvt){
        TL_ZB_TIMER_CANCEL(&g_appCtx.timerLedEvt);
        g_appCtx.times = 0;
    }

    led_set_control();
}
