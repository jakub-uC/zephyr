/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <zephyr.h>
#include <logging/log.h>

#include <device.h>
#include <soc.h>

#include <stdio.h>
#include <string.h>
#include <init.h>
#include <random/rand32.h>

#include <nrfs_led.h>
#include <nrfs_pm.h>
#include <nrfs_mts.h>

#define SHM_NODE       DT_CHOSEN(zephyr_ipc_shm)
#define SHM_START_ADDR DT_REG_ADDR(SHM_NODE)
#define SHM_SIZE       DT_REG_SIZE(SHM_NODE)

#define MTS_BUFFER_SIZE 16
#define MTS_SOURCE_ADDR (SHM_START_ADDR + SHM_SIZE - MTS_BUFFER_SIZE)
#define MTS_SINK_ADDR   (MTS_SOURCE_ADDR - MTS_BUFFER_SIZE)

LOG_MODULE_REGISTER(nrf53net, CONFIG_LOG_DEFAULT_LEVEL);

void tx_timer_expiry_fn(struct k_timer *dummy);

static struct k_thread m_tx_thread_cb;
static K_THREAD_STACK_DEFINE(m_tx_thread_stack, 1024);
static K_TIMER_DEFINE(m_tx_timer, tx_timer_expiry_fn, NULL);
static K_SEM_DEFINE(m_sem_irq, 0, 255);
static K_SEM_DEFINE(m_sem_tx, 0, 1);

uint8_t *source_buffer = (uint8_t *)MTS_SOURCE_ADDR;
uint8_t *sink_buffer = (uint8_t *)MTS_SINK_ADDR;

void tx_timer_expiry_fn(struct k_timer *dummy)
{
	uint32_t delay = sys_rand32_get() % 1000;

	k_sem_give(&m_sem_tx);

	LOG_INF("Setting TX timer delay to %d ms", delay);
	k_timer_start(&m_tx_timer, K_MSEC(delay), K_NO_WAIT);
}

static uint32_t context_generate(void)
{
	return sys_rand32_get();
}

static void request_generate(void)
{
	uint32_t ctx = context_generate();
	nrfs_err_t status;

	switch (sys_rand32_get() % 4) {
	case 0:
		LOG_INF("LED: toggle.");
		status = nrfs_led_state_change(NRFS_LED_OP_TOGGLE, sys_rand32_get() % 4);
		break;

	case 1:
		if (sys_rand32_get() % 2 == 0) {
			LOG_INF("RADIO: ON request.");
			status = nrfs_pm_radio_request(500, true, (void *)ctx);
		} else {
			LOG_INF("RADIO: OFF request.");
			status = nrfs_pm_radio_release((void *)ctx);
		}
		break;

	case 2:
		LOG_INF("CLOCK: request.");
		status = nrfs_pm_cpu_clock_request(sys_rand32_get(),
			sys_rand32_get() % NRFS_GPMS_CPU_CLOCK_FREQUENCY_400_MHZ, true, (void *)ctx);
		break;

	case 3:
		LOG_INF("MTS: Copy request.");
		for (size_t i = 0; i < MTS_BUFFER_SIZE; i++) {
			source_buffer[i] = sys_rand32_get() & 0xFF;
		}
		memset(sink_buffer, 0, MTS_BUFFER_SIZE);

		nrfs_mts_copy_request_t req =
		{
			.p_source = source_buffer,
			.p_sink = sink_buffer,
			.size = MTS_BUFFER_SIZE
		};
		status = nrfs_mts_copy_request(&req, true, (void *)ctx);
		break;

	default:
		break;
	}

	if (status != NRFS_SUCCESS) {
		LOG_ERR("Request send failed: %d", status);
	}
}

void tx_thread(void *arg1, void *arg2, void *arg3)
{
	while (1) {
		k_sem_take(&m_sem_tx, K_FOREVER);
		request_generate();
	}
}

void pm_handler(nrfs_pm_evt_t evt, void *context)
{
	switch (evt) {
	case NRFS_PM_EVT_NOTIFICATION:
		LOG_INF("PM handler - notification: 0x%x", (uint32_t)context);
		break;
	case NRFS_PM_EVT_ERROR:
		LOG_INF("PM handler - error: 0x%x", (uint32_t)context);
		break;
	case NRFS_PM_EVT_REJECT:
		LOG_INF("PM handler - request rejected: 0x%x", (uint32_t)context);
		break;
	default:
		LOG_ERR("PM handler - unexpected event: 0x%x", evt);
		break;
	}
}

void led_handler(nrfs_led_evt_t evt, void *p_buffer, size_t size)
{
	switch (evt) {
	case NRFS_LED_EVT_NOTIFICATION:
		LOG_HEXDUMP_INF(p_buffer, size, "LED handler - notification:");
		break;
	case NRFS_LED_EVT_REJECT:
		LOG_INF("LED handler - request rejected");
		break;
	default:
		LOG_ERR("LED handler - unexpected event: 0x%x", evt);
		break;
	}
}

void mts_handler(nrfs_mts_evt_t const *p_evt, void *context)
{
	switch (p_evt->type) {
	case NRFS_MTS_EVT_COPY_DONE:
		LOG_HEXDUMP_INF(p_evt->data.copy_done.p_source,
				p_evt->data.copy_done.size, "MTS handler - copy done - source:");
		LOG_HEXDUMP_INF(p_evt->data.copy_done.p_source,
				p_evt->data.copy_done.size, "MTS handler - copy done - sink:");
		break;
	case NRFS_MTS_EVT_REJECT:
		LOG_INF("MTS handler - request rejected");
		break;
	default:
		LOG_ERR("MTS handler - unexpected event: 0x%x", p_evt->type);
		break;
	}
}

void nrfs_unsolicited_handler(void *p_buffer, size_t size)
{
	LOG_HEXDUMP_INF(p_buffer, size, "Unsolicited notification:");
}

int main(void)
{
	LOG_INF("Local domain.");

	nrfs_err_t status;

	status = nrfs_pm_init(pm_handler);
	if (status != NRFS_SUCCESS) {
		LOG_ERR("PM service init failed: %d", status);
	}

	status = nrfs_led_init(led_handler);
	if (status != NRFS_SUCCESS) {
		LOG_ERR("LED service init failed: %d", status);
	}

	status = nrfs_mts_init(mts_handler);
	if (status != NRFS_SUCCESS) {
		LOG_ERR("MTS service init failed: %d", status);
	}

	k_thread_create(&m_tx_thread_cb, m_tx_thread_stack,
			K_THREAD_STACK_SIZEOF(m_tx_thread_stack), tx_thread, NULL, NULL,
			NULL, 0, 0, K_NO_WAIT);

	k_timer_start(&m_tx_timer, K_MSEC(10), K_NO_WAIT);

	return 0;
}
