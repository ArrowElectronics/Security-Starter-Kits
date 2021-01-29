/******************************************************************************
 * @file    ble_types.h
 * @author  MCD Application Team
 * @date    15 January 2019
 * @brief   Auto-generated file: do not edit!
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */

#ifndef BLUENRG_EVENT_TYPES_H__
#define BLUENRG_EVENT_TYPES_H__

#include <stdint.h>
#include "compiler.h"
#include "ble_const.h"

typedef uint8_t tBleStatus;


typedef tBleStatus (*hci_event_process)(uint8_t *buffer_in);
typedef struct  
{
  uint16_t evt_code;
  hci_event_process process;
} hci_events_table_type, hci_le_meta_events_table_type, hci_vendor_specific_events_table_type;

extern const hci_events_table_type hci_events_table[7];
extern const hci_le_meta_events_table_type hci_le_meta_events_table[10];
extern const hci_vendor_specific_events_table_type hci_vendor_specific_events_table[43];


#endif /* ! BLUENRG_EVENT_TYPES_H__ */
