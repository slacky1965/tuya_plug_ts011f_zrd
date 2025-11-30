#include "app_main.h"

relay_settings_t relay_settings;
dev_relay_t      dev_relay;

static uint8_t checksum(uint8_t *data, uint16_t length) {

    uint8_t crc8 = 0;

    for(uint8_t i = 0; i < (length - 1); i++) {
        crc8 += data[i];
    }

    return crc8;
}

static void check_first_start(uint8_t i) {

    switch(relay_settings.startUpOnOff[i]) {
        case ZCL_START_UP_ONOFF_SET_ONOFF_TO_PREVIOUS:
            if (relay_settings.status_onoff[i]) cmdOnOff_on(dev_relay.unit_relay[i].ep);
            else cmdOnOff_off(dev_relay.unit_relay[i].ep);
            break;
        case ZCL_START_UP_ONOFF_SET_ONOFF_TOGGLE:
            cmdOnOff_toggle(dev_relay.unit_relay[i].ep);
            break;
        case ZCL_START_UP_ONOFF_SET_ONOFF_TO_ON:
            cmdOnOff_on(dev_relay.unit_relay[i].ep);
            break;
        case ZCL_START_UP_ONOFF_SET_ONOFF_TO_OFF:
            cmdOnOff_off(dev_relay.unit_relay[i].ep);
            break;
        default:
            cmdOnOff_off(dev_relay.unit_relay[i].ep);
            break;
    }
}



bool get_relay_status(uint8_t i) {
    return drv_gpio_read(dev_relay.unit_relay[i].rl)?true:false;
}

void set_relay_status(uint8_t i, uint8_t status) {
//    printf("set_relay_status(i = %d, status = %d). GPIO: %d\r\n", i, status, dev_relay.unit_relay[i].rl);
    if (status == RELAY_ON) {
        light_on();
#if !WITHOUT_MONITORING
        clear_auto_restart();
#endif
    } else {
        light_off();
    }
    drv_gpio_write(dev_relay.unit_relay[i].rl, status);
}

#if UART_PRINTF_MODE && DEBUG_SAVE
static void print_setting_sr(nv_sts_t st, relay_settings_t *relay_settings_tmp, bool save) {

    printf("Settings %s. Return: %s\r\n", save?"saved":"restored", st==NV_SUCC?"Ok":"Error");

    printf("status_onoff1:      0x%02x\r\n", relay_settings_tmp->status_onoff[0]);
    printf("startUpOnOff1:      0x%02x\r\n", relay_settings_tmp->startUpOnOff[0]);
    printf("current_max:        0x%04x\r\n", relay_settings_tmp->current_max);
    printf("power_max:          0x%04x\r\n", relay_settings_tmp->power_max);
    printf("voltage_min:        0x%04x\r\n", relay_settings_tmp->voltage_min);
    printf("voltage_max:        0x%04x\r\n", relay_settings_tmp->voltage_max);
    printf("time_reload:        0x%04x\r\n", relay_settings_tmp->time_reload);
    printf("protect_control:    0x%02x\r\n", relay_settings_tmp->protect_control);
    printf("auto_restart:       0x%02x\r\n", relay_settings_tmp->auto_restart);
    printf("key_lock:           0x%02x\r\n", relay_settings_tmp->key_lock);
    printf("led_control:        0x%02x\r\n", relay_settings_tmp->led_control);

}
#endif

nv_sts_t relay_settings_save() {
    nv_sts_t st = NV_SUCC;

#if NV_ENABLE

#if UART_PRINTF_MODE
    printf("Saved relay settings\r\n");
#endif

    relay_settings.crc = checksum((uint8_t*)&relay_settings, sizeof(relay_settings_t));
    st = nv_flashWriteNew(1, NV_MODULE_APP,  NV_ITEM_APP_USER_CFG, sizeof(relay_settings_t), (uint8_t*)&relay_settings);

#else
    st = NV_ENABLE_PROTECT_ERROR;
#endif

    return st;
}

nv_sts_t relay_settings_restore() {
    nv_sts_t st = NV_SUCC;

#if NV_ENABLE

    relay_settings_t relay_settings_tmp;

    st = nv_flashReadNew(1, NV_MODULE_APP,  NV_ITEM_APP_USER_CFG, sizeof(relay_settings_t), (uint8_t*)&relay_settings_tmp);

    if (st == NV_SUCC && relay_settings_tmp.crc == checksum((uint8_t*)&relay_settings_tmp, sizeof(relay_settings_t))) {

#if UART_PRINTF_MODE
        printf("Restored relay settings\r\n");
#if DEBUG_SAVE
        print_setting_sr(st, &relay_settings_tmp, false);
#endif
#endif

    } else {
        /* default config */
#if UART_PRINTF_MODE
        printf("Default relay settings \r\n");
#endif
        relay_settings_tmp.startUpOnOff[0] = ZCL_START_UP_ONOFF_SET_ONOFF_TO_OFF;
        relay_settings_tmp.status_onoff[0] = ZCL_ONOFF_STATUS_OFF;
        relay_settings_tmp.current_max = DEFAULT_CURRENT_MAX;
        relay_settings_tmp.power_max = DEFAULT_POWER_MAX;
        relay_settings_tmp.voltage_min = DEFAULT_VOLTAGE_MIN;
        relay_settings_tmp.voltage_max = DEFAULT_VOLTAGE_MAX;
        relay_settings_tmp.time_reload = DEFAULT_TIME_RELOAD;
        relay_settings_tmp.protect_control = DEFAULT_PROTECT_CONTROL;
        relay_settings_tmp.auto_restart = DEFAULT_AUTORESTART;
        relay_settings_tmp.key_lock = DEFAULT_KEY_LOCK;
        relay_settings_tmp.led_control = DEFAULT_LED_CONTROL;
    }

    memcpy(&relay_settings, &relay_settings_tmp, (sizeof(relay_settings_t)));
    g_zcl_onOffAttrs[0].onOff = relay_settings.status_onoff[0];
    g_zcl_onOffAttrs[0].startUpOnOff = relay_settings.startUpOnOff[0];
    g_zcl_onOffAttrs[0].key_lock = relay_settings.key_lock;
    g_zcl_onOffAttrs[0].led_control = relay_settings.led_control;
#if !WITHOUT_MONITORING
    g_zcl_msAttrs.current_max = relay_settings.current_max;
    g_zcl_msAttrs.power_max = relay_settings.power_max;
    g_zcl_msAttrs.voltage_min = relay_settings.voltage_min;
    g_zcl_msAttrs.voltage_max = relay_settings.voltage_max;
    g_zcl_msAttrs.time_reload = relay_settings.time_reload;
    g_zcl_msAttrs.protect_control = relay_settings.protect_control;
    g_zcl_msAttrs.auto_restart = relay_settings.auto_restart;
#endif
#else
    st = NV_ENABLE_PROTECT_ERROR;
#endif

    return st;
}

void relay_settints_default() {
    relay_settings.startUpOnOff[0] = ZCL_START_UP_ONOFF_SET_ONOFF_TO_OFF;
    relay_settings.status_onoff[0] = ZCL_ONOFF_STATUS_OFF;
    relay_settings.current_max = DEFAULT_CURRENT_MAX;
    relay_settings.power_max = DEFAULT_POWER_MAX;
    relay_settings.voltage_min = DEFAULT_VOLTAGE_MIN;
    relay_settings.voltage_max = DEFAULT_VOLTAGE_MAX;
    relay_settings.time_reload = DEFAULT_TIME_RELOAD;
    relay_settings.protect_control = DEFAULT_PROTECT_CONTROL;
    relay_settings.auto_restart = DEFAULT_AUTORESTART;
    relay_settings.key_lock = DEFAULT_KEY_LOCK;
    relay_settings.led_control = DEFAULT_LED_CONTROL;

    relay_settings_save();

    g_zcl_onOffAttrs[0].onOff = relay_settings.status_onoff[0];
    g_zcl_onOffAttrs[0].startUpOnOff = relay_settings.startUpOnOff[0];
    g_zcl_onOffAttrs[0].key_lock = relay_settings.key_lock;
    g_zcl_onOffAttrs[0].led_control = relay_settings.led_control;
#if !WITHOUT_MONITORING
    g_zcl_msAttrs.current_max = relay_settings.current_max;
    g_zcl_msAttrs.power_max = relay_settings.power_max;
    g_zcl_msAttrs.voltage_min = relay_settings.voltage_min;
    g_zcl_msAttrs.voltage_max = relay_settings.voltage_max;
    g_zcl_msAttrs.time_reload = relay_settings.time_reload;
    g_zcl_msAttrs.protect_control = relay_settings.protect_control;
    g_zcl_msAttrs.auto_restart = relay_settings.auto_restart;
#endif
    cmdOnOff_off(APP_ENDPOINT1);
}

void dev_relay_init() {
    dev_relay.amt = AMT_RELAY;
    dev_relay.unit_relay[0].ep = APP_ENDPOINT1;
    dev_relay.unit_relay[0].rl = RELAY1_GPIO;

    if (relay_settings.led_control == CONTROL_LED_ON) light_on();

    check_first_start(0);
}
