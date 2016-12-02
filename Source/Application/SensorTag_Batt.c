/*******************************************************************************
  Filename:       SensorTag_Batt.c
  Revised:        $Date: 2013-11-06 17:27:44 +0100 (on, 06 nov 2013) $
  Revision:       $Revision: 35922 $

  Description:    This file contains the Battery Monitor sub-application.
*******************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Queue.h>

#include "gatt.h"
#include "gattservapp.h"

#include "Board.h"
#include "battservice.h"
#include "SensorTag_Batt.h"
#include "sensor.h"
#include "util.h"
#include "string.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS and MACROS
 */
// How often to perform sensor reads (milliseconds)
#define SENSOR_DEFAULT_PERIOD   10000

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
static Clock_Struct periodicClock;
static uint16_t sensorPeriod;
static volatile bool sensorReadScheduled;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void SensorTagBatt_clockHandler(UArg arg);

/*********************************************************************
 * PROFILE CALLBACKS
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      SensorTagBatt_init
 *
 * @brief   Initialization function for the SensorTag movement sub-application
 *
 */
void SensorTagBatt_init( void )
{
  // Add service
  Batt_AddService();

  // Initialize the module state variables
  sensorPeriod = SENSOR_DEFAULT_PERIOD;
  sensorReadScheduled = false;


  // Create continuous clock for internal periodic events.
  Util_constructClock(&periodicClock, SensorTagBatt_clockHandler,
                      1000, sensorPeriod, true, 0);
}

/*********************************************************************
 * @fn      SensorTagBatt_processSensorEvent
 *
 * @brief   SensorTag Gyroscope sensor event processor.
 *
 */
void SensorTagBatt_processSensorEvent(void)
{
  if (sensorReadScheduled)
  {
	Batt_MeasLevel();
    sensorReadScheduled = false;
  }
}

/*********************************************************************
 * @fn      SensorTagBatt_reset
 *
 * @brief   Reset characteristics and disable sensor
 *
 * @param   none
 *
 * @return  none
 */
void SensorTagBatt_reset (void)
{
}


/*********************************************************************
* Private functions
*/

/*********************************************************************
 * @fn      SensorTagBatt_clockHandler
 *
 * @brief   Handler function for clock time-outs.
 *
 * @param   arg - event type
 *
 * @return  none
 */
static void SensorTagBatt_clockHandler(UArg arg)
{
  // Schedule readout periodically
  sensorReadScheduled = true;
  Semaphore_post(sem);
}

/*********************************************************************
*********************************************************************/

