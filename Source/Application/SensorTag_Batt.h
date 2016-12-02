/******************************************************************************
  Filename:       SensorTag_Batt.h

  Description:    This file contains the Sensor Tag Battery info.
****************************************************************************/

#ifndef SENSORTAGBATT_H
#define SENSORTAGBATT_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
#include "SensorTag.h"

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Initialization for the SensorTag movement task
 */
extern void SensorTagBatt_init(void);

/*
 * Task Event Processor for SensorTag movement
 */
extern void SensorTagBatt_processSensorEvent(void);

/*
 * Reset function for the Movement task
 */
extern void SensorTagBatt_reset(void);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* SENSORTAGBATT_H */
