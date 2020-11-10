/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _LED_EVENT_H_
#define _LED_EVENT_H_

/**
 * @brief Led Event
 * @defgroup led_event Led Event
 * @{
 */

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

struct led_event {
	struct event_header header;
	u8_t domain;
};

EVENT_TYPE_DECLARE(led_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _LED_EVENT_H_ */
