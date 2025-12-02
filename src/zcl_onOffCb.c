/********************************************************************************************************
 * @file    zcl_onOffCb.c
 *
 * @brief   This is the source file for zcl_onOffCb
 *
 * @author  Zigbee Group
 * @date    2021
 *
 * @par     Copyright (c) 2021, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *			All rights reserved.
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/

/**********************************************************************
 * INCLUDES
 */
#include "app_main.h"

/**********************************************************************
 * LOCAL CONSTANTS
 */
#define ZCL_ONOFF_TIMER_INTERVAL        100 //the timer interval to change the offWaitTime/onTime attribute of the ONOFF cluster

/**********************************************************************
 * LOCAL VARIABLES
 */
static ev_timer_event_t *onWithTimedOffTimerEvt = NULL;

/**********************************************************************
 * FUNCTIONS
 */

void app_onOffUpdate(u8 ep, u8 cmd);

/*********************************************************************
 * @fn      app_onOffInit
 *
 * @brief
 *
 * @param   None
 *
 * @return  None
 */
void app_onOffInit()
{
    zcl_onOffAttr_t *pOnOff = zcl_onOffAttrsGet();

    for(uint8_t i = 0; i < AMT_RELAY; i++) {
        pOnOff += i;
        app_onOffUpdate(i+1, pOnOff->onOff);
    }

}

/*********************************************************************
 * @fn      app_onOffUpdate
 *
 * @brief
 *
 * @param   ZCL_CMD_ONOFF_ON / ZCL_ONOFF_STATUS_OFF / ZCL_CMD_ONOFF_TOGGLE
 *
 * @return  None
 */
void app_onOffUpdate(u8 ep, u8 cmd)
{
    zcl_onOffAttr_t *pOnOff = zcl_onOffAttrsGet();
    pOnOff += (ep-1);
    bool onOff = ZCL_ONOFF_STATUS_ON;

    if (cmd == ZCL_CMD_ONOFF_ON) {
        onOff = ZCL_ONOFF_STATUS_ON;
    } else if (cmd == ZCL_CMD_ONOFF_OFF) {
        onOff = ZCL_ONOFF_STATUS_OFF;
    } else if (cmd == ZCL_CMD_ONOFF_TOGGLE) {
        onOff = (pOnOff->onOff == ZCL_ONOFF_STATUS_ON) ? ZCL_ONOFF_STATUS_OFF
                                                       : ZCL_ONOFF_STATUS_ON;
    } else {
        return;
    }

    //update attributes
    if (onOff == ZCL_ONOFF_STATUS_ON) {
        pOnOff->globalSceneControl = TRUE;
        pOnOff->onOff = ZCL_ONOFF_STATUS_ON;
        if (pOnOff->onTime == 0) {
            pOnOff->offWaitTime = 0;
        }
    } else {
        pOnOff->onOff = ZCL_ONOFF_STATUS_OFF;
        pOnOff->onTime = 0;
    }

#ifdef ZCL_SCENE
    zcl_sceneAttr_t *pScene = zcl_sceneAttrGet();
    pScene->sceneValid = 0;
#endif

    //TODO: On and Off

//    light_refresh(LIGHT_STA_ON_OFF);
}

/*********************************************************************
 * @fn      app_onWithTimedOffTimerCb
 *
 * @brief   timer event to process the ON_WITH_TIMED_OFF command
 *
 * @param   arg
 *
 * @return  0: timer continue on; -1: timer will be canceled
 */
static s32 app_onWithTimedOffTimerCb(void *arg)
{
    zcl_onOffAttr_t *pOnOff = zcl_onOffAttrsGet();
    u8 ep = 1;

    if ((pOnOff->onOff == ZCL_ONOFF_STATUS_ON) && pOnOff->onTime) {
        pOnOff->onTime--;
        if (pOnOff->onTime <= 0) {
            pOnOff->offWaitTime = 0;
            app_onOffUpdate(ep, ZCL_CMD_ONOFF_OFF);
        }
    }

    if ((pOnOff->onOff == ZCL_ONOFF_STATUS_OFF) && pOnOff->offWaitTime) {
        pOnOff->offWaitTime--;
        if (pOnOff->offWaitTime <= 0) {
            onWithTimedOffTimerEvt = NULL;
            return -1;
        }
    }

    if (pOnOff->onTime || pOnOff->offWaitTime) {
        return 0;
    } else {
        onWithTimedOffTimerEvt = NULL;
        return -1;
    }
}

/*********************************************************************
 * @fn      app_onWithTimedOffTimerStart
 *
 * @brief   start the onWithTimedOff timer
 *
 * @param
 *
 * @return
 */
static void app_onWithTimedOffTimerStart(void)
{
    if (onWithTimedOffTimerEvt) {
        TL_ZB_TIMER_CANCEL(&onWithTimedOffTimerEvt);
    }
    onWithTimedOffTimerEvt = TL_ZB_TIMER_SCHEDULE(app_onWithTimedOffTimerCb, NULL, ZCL_ONOFF_TIMER_INTERVAL);
}

/*********************************************************************
 * @fn      app_onoff_onWithTimedOffProcess
 *
 * @brief
 *
 * @param   cmd
 *
 * @return  None
 */
static void app_onoff_onWithTimedOffProcess(u8 ep, zcl_onoff_onWithTimeOffCmd_t *cmd)
{
    zcl_onOffAttr_t *pOnOff = zcl_onOffAttrsGet();
    pOnOff += (ep-1);

    if (cmd->onOffCtrl.bits.acceptOnlyWhenOn && (pOnOff->onOff == ZCL_ONOFF_STATUS_OFF)) {
        return;
    }

    if (pOnOff->offWaitTime && (pOnOff->onOff == ZCL_ONOFF_STATUS_OFF)) {
        pOnOff->offWaitTime = min2(pOnOff->offWaitTime, cmd->offWaitTime);
    } else {
        pOnOff->onTime = max2(pOnOff->onTime, cmd->onTime);
        pOnOff->offWaitTime = cmd->offWaitTime;
        app_onOffUpdate(ZCL_CMD_ONOFF_ON, ep);
    }

    if ((pOnOff->onTime < 0xFFFF) && (pOnOff->offWaitTime < 0xFFFF)) {
        if(pOnOff->onTime || pOnOff->offWaitTime){
            app_onWithTimedOffTimerStart();
        }
    }
}

/*********************************************************************
 * @fn      app_onoff_offWithEffectProcess
 *
 * @brief
 *
 * @param   cmd
 *
 * @return  None
 */
static void app_onoff_offWithEffectProcess(u8 ep, zcl_onoff_offWithEffectCmd_t *cmd)
{
    zcl_onOffAttr_t *pOnOff = zcl_onOffAttrsGet();
    pOnOff += (ep-1);
    pOnOff->globalSceneControl = FALSE;

    //TODO:

    app_onOffUpdate(ZCL_CMD_ONOFF_OFF, ep);
}

/*********************************************************************
 * @fn      app_onoff_onWithRecallGlobalSceneProcess
 *
 * @brief
 *
 * @param   cmd
 *
 * @return  None
 */
static void app_onoff_onWithRecallGlobalSceneProcess(void)
{
    zcl_onOffAttr_t *pOnOff = zcl_onOffAttrsGet();
    pOnOff->globalSceneControl = TRUE;

    //TODO:
}

/*********************************************************************
 * @fn      app_onOffCb
 *
 * @brief   Handler for ZCL ONOFF command. This function will set ONOFF attribute first.
 *
 * @param	pAddrInfo
 * @param   cmdId - onoff cluster command id
 * @param	cmdPayload
 *
 * @return  status_t
 */
status_t app_onOffCb(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload) {

    printf("app_onOffCb\r\n");

    if (pAddrInfo->dstEp == APP_ENDPOINT1) {
        switch (cmdId) {
        case ZCL_CMD_ONOFF_ON:
        case ZCL_CMD_ONOFF_OFF:
        case ZCL_CMD_ONOFF_TOGGLE:
            app_onOffUpdate(pAddrInfo->dstEp, cmdId);
            break;
        case ZCL_CMD_OFF_WITH_EFFECT:
            app_onoff_offWithEffectProcess(pAddrInfo->dstEp , (zcl_onoff_offWithEffectCmd_t *)cmdPayload);
            break;
        case ZCL_CMD_ON_WITH_RECALL_GLOBAL_SCENE:
            app_onoff_onWithRecallGlobalSceneProcess();
            break;
        case ZCL_CMD_ON_WITH_TIMED_OFF:
            app_onoff_onWithTimedOffProcess(pAddrInfo->dstEp , (zcl_onoff_onWithTimeOffCmd_t *)cmdPayload);
            break;
        default:
            break;
        }
    }

    return ZCL_STA_SUCCESS;
}

