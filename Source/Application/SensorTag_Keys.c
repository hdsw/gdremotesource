/*******************************************************************************
  Filename:       SensorTag_Keys.c
  Revised:        $Date: 2013-08-23 20:45:31 +0200 (fr, 23 aug 2013) $
  Revision:       $Revision: 35100 $

  Description:    This file contains the Sensor Tag sample application,
                  Keys part, for use with the TI Bluetooth Low 
                  Energy Protocol Stack.

  Copyright 2015  Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
*******************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "gatt.h"
#include "gattservapp.h"
#include "SensorTag_Keys.h"
#include "sensorTag_IO.h"
#include "ioservice.h"

#include "Board.h"
#include "peripheral.h"
#include "simplekeys.h"

#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Semaphore.h>

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */
#define SK_KEY_REED             0x04
#define SK_PUSH_KEYS            0x03

// Key press time-outs (milliseconds)
#define POWER_PRESS_PERIOD      3000
#define RESET_PRESS_PERIOD      6000

/*********************************************************************
 * TYPEDEFS
 */
typedef struct
{
  uint32_t tStart;
  uint32_t tStop;
} KeyPress_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */
extern void restoreFactoryImage(void);

/*********************************************************************
 * LOCAL VARIABLES
 */
static uint32_t totalKeys = 12;
static uint16_t keys;
static KeyPress_t keyStatus[12 + 1]; // aside from bit masks, index at 1
/* static uint32_t keyToPin[] = { 0xFFFFFFFF, // buttons start at 1, keep the map that way
							   IOID_4, IOID_0, IOID_12, IOID_1, IOID_2, IOID_3,
							   IOID_5, IOID_6, IOID_7, IOID_8, IOID_9, IOID_11 }; */
 static uint32_t keyToPin[] = { 0xFFFFFFFF, // buttons start at 1, keep the map that way
							   IOID_0, IOID_1, IOID_2, IOID_3, IOID_4, IOID_5,
							   IOID_6, IOID_7, IOID_8, IOID_9, IOID_10, IOID_11, IOID_12 };

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void processGapStateChange(void);
static uint32_t mapKeyToPin(uint8_t key);
static uint8_t mapPinToKey(uint32_t pin);

/*********************************************************************
 * PROFILE CALLBACKS
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      SensorTagKeys_init
 *
 * @brief   Initialization function for the SensorTag keys
 *
 */
void SensorTagKeys_init(void)
{
  // Add service
  SK_AddService (GATT_ALL_SERVICES);

  // Initialize the module state variables
  SensorTagKeys_reset();
}

/*********************************************************************
 * @fn      SensorTagKeys_processKeyRight
 *
 * @brief   Interrupt handler for BUTTON 1(right)
 *
 */
void SensorTagKeys_processKey(uint32_t pin)
{
  uint8_t key = mapPinToKey(pin);
  if(key != 0xFF) {
	if(PIN_getInputValue(pin)) {
		keys &= ~(1 << (key - 1));
		keyStatus[key].tStop = Clock_getTicks();
	} else {
		keys |= (1 << ( key - 1));
		keyStatus[key].tStart = Clock_getTicks();
	}
  }
  
  // Wake up the application thread
  Semaphore_post(sem);
}

/*********************************************************************
 * @fn      SensorTagKeys_processRelay
 *
 * @brief   Interrupt service routine for reed relay
 *
 */
/*
void SensorTagKeys_processRelay(void)
{
  if (PIN_getInputValue(Board_RELAY))
  {
    keys |= SK_KEY_REED;
  }
  else
  {
    keys &= ~SK_KEY_REED;
  }
  
  // Wake up the application thread
  Semaphore_post(sem);
}
*/

/*********************************************************************
 * @fn      SensorTagKeys_processEvent
 *
 * @brief   SensorTag Keys event processor.  
 *
 */
void SensorTagKeys_processEvent(void)
{
  static uint16_t current_keys = 0;
  
  // Set the value of the keys state to the Simple Keys Profile;
  // This will send out a notification of the keys state if enabled
  if (current_keys != keys)
  {
    SK_SetParameter( SK_KEY_ATTR, sizeof ( uint16_t ), &keys);
    
    // Insert key state into advertising data
    if ( gapProfileState == GAPROLE_ADVERTISING )
    {
      sensorTag_updateAdvertisingData(keys);
    }
    
    // Check if right key was pressed for more than 3 seconds and less than 6
    if ( (current_keys & 1)!=0 && (keys & 1)==0 )
    {
      
      if (gapProfileState == GAPROLE_CONNECTED)
      {
        int duration;
        
        duration = ((keyStatus[1].tStop - keyStatus[1].tStart) * Clock_tickPeriod)
          / 1000;
        
        // Connected: change state after 3 second press (power/right button)
        if (duration > POWER_PRESS_PERIOD && duration < RESET_PRESS_PERIOD)
        {
          processGapStateChange();
        }
      }
      else
      {
        // Not connected; change state immediately (power/right button)
        processGapStateChange();
      } 
    }

    // Check if both keys were pressed for more than 6 seconds
    if ( (current_keys & 3)==3
        && (keys & 3)!=3 )
    {
      int duration;
      uint16_t rel;
      
      // Check which key has been released
      rel = current_keys ^ keys;
      
      if (rel & 1)
      {
        duration = ((keyStatus[1].tStop - keyStatus[1].tStart) * Clock_tickPeriod)
          / 1000;
      }
      else
      {
        duration = ((keyStatus[2].tStop - keyStatus[2].tStart) * Clock_tickPeriod)
          / 1000;
      }
      
      // Both keys have been pressed for 6 seconds -> restore factory image
      if ( duration > RESET_PRESS_PERIOD )
      {
        SensorTag_blinkLed(Board_BUZZER, 10);
        
        // Apply factory image and reboot
        SensorTag_applyFactoryImage();
      }
    }
  }
  
  current_keys = keys;
}

/*********************************************************************
 * @fn      SensorTagKeys_reset
 *
 * @brief   Reset key state to 'not pressed'
 *
 * @param   none
 *
 * @return  none
 */
void SensorTagKeys_reset(void)
{
  memset(keyStatus, 0, sizeof(KeyPress_t) * 16);
  keys = 0;
  SK_SetParameter(SK_KEY_ATTR, sizeof ( uint16_t ), &keys);
}

/*********************************************************************
 * @fn      processGapStateChange
 *
 * @brief   Change the GAP state. 
 *          1. Connected -> disconnect and start advertising
 *          2. Advertising -> stop advertising
 *          3. Disconnected/not advertising -> start advertising
 *
 * @param   none
 *
 * @return  none
 */
static void processGapStateChange(void)
{
  if (gapProfileState != GAPROLE_CONNECTED)
  {
    uint8_t current_adv_enabled_status;
    uint8_t new_adv_enabled_status;
    
    // Find the current GAP advertising status
    GAPRole_GetParameter( GAPROLE_ADVERT_ENABLED, &current_adv_enabled_status);
    
    if( current_adv_enabled_status == FALSE )
    {
      new_adv_enabled_status = TRUE;
    }
    else
    {
      new_adv_enabled_status = FALSE;
    }
    
    // Change the GAP advertisement status to opposite of current status
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof( uint8_t ), 
                         &new_adv_enabled_status);
  }
  
  if (gapProfileState == GAPROLE_CONNECTED)
  {
    uint8_t adv_enabled = TRUE;
    
    // Disconnect
    GAPRole_TerminateConnection();
    
    // Start advertising
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof( uint8_t ), 
                         &adv_enabled);
  }
}

static uint32_t mapKeyToPin(uint8_t key)
{
	if(key <= totalKeys) {
		return keyToPin[key];
	}
	return 0xFFFFFFFF;
}

static uint8_t mapPinToKey(uint32_t pin)
{
	switch(pin) {
		case IOID_4:
			return 1;
		case IOID_0:
			return 2;
		case IOID_12:
			return 3;
		case IOID_1:
			return 4;
		case IOID_2:
			return 5;
		case IOID_3:
			return 6;
		case IOID_5:
			return 7;
		case IOID_6:
			return 8;
		case IOID_7:
			return 9;
		case IOID_8:
			return 10;
		case IOID_9:
			return 11;
		case IOID_11:
			return 12;
		default:
			break;
	};

	return 0xFF;
}
/*********************************************************************
*********************************************************************/

