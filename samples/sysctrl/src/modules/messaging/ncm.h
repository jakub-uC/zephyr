/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _NCM_H_
#define _NCM_H_

/**
 * @brief Notification Context Manager
 * @defgroup ncm Notification Context Manager
 * @{
 */

#include <internal/nrfs_hdr.h>
#include <internal/nrfs_ctx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Single context entry.
 * Used internally by the services that holds the context of a request for the notification.
 */
struct ncm_ctx {
	uint8_t domain_id;
	uint8_t ept_id;
	nrfs_hdr_t hdr;
	nrfs_ctx_t app_ctx;
};

/**
 * @brief Single context entry for unsolicited notifications.
 * Used for describing unsolicited notification contents and recipient.
 */
struct ncm_unsolicited_ctx {
	uint8_t domain_id;
	uint8_t ept_id;
	uint16_t notif_id;
};

/**
 * @brief Fill single context entry with the details about the sender of the request.
 *
 * This context is later used to send a notification to the sender of the request.
 *
 * @param p_ctx Pointer to the context entry to be filled.
 * @param p_msg Pointer to the message with request.
 */
void ncm_fill(struct ncm_ctx *p_ctx, nrfs_phy_t *p_msg);

/**
 * @brief Get the request ID associated with the specified context.
 *
 * @param p_ctx Pointer to the context entry to get the request ID from.
 *
 * @return Request ID.
 */
uint32_t ncm_req_id_get(struct ncm_ctx *p_ctx);

/**
 * @brief Allocate memory for the payload of the notification.
 *
 * Allocated space should be filled with the notification details
 * and then passed to @ref ncm_notify() or @ref ncm_unsolicited_notify().
 *
 * @param bytes Number of bytes to allocate.
 *
 * @return Pointer to the buffer for notification details.
 */
void *ncm_alloc(size_t bytes);

/**
 * @brief Free memory that was earlier allocated for the notification payload.
 *
 * @warning Use this function only if notification is to be abandoned
 *          and payload allocated with @ref ncm_alloc() is no longer needed.
 * @warning This function must not be used with the allocated payload
 *          after using it in  @ref ncm_notify() or @ref ncm_unsolicited_notify().
 *
 * @param p_data Pointer to the buffer for notification details.
 */
void ncm_abandon(void *app_payload);

/**
 * @brief Send notification associated with specified context.
 *
 * @param p_ctx       Pointer to the context entry associated with the notification to be send.
 * @param app_payload Pointer to the memory returned by @ref ncm_alloc()
 *                    and filled with notification details.
 * @param size        Size of the notification details.
 */
void ncm_notify(struct ncm_ctx *p_ctx, void *app_payload, size_t size);

/**
 * @brief Send unsolicited notification.
 *
 * Unsolicited notifications are not associated with any previous request.
 *
 * @param p_ctx       Pointer to the context entry associated with the notification to be send.
 * @param app_payload Pointer to the memory returned by @ref ncm_alloc()
 *                    and filled with notification details.
 * @param size        Size of the notification details.
 */
void ncm_unsolicited_notify(struct ncm_unsolicited_ctx *p_ctx, void *app_payload, size_t size);

/**
 * @brief Free memory that was earlier allocated for the notification payload.
 *
 * @warning This function must be called by the transport backend
 *          that performed the notification.
 *
 * @param p_data Notification payload.
 */
void ncm_free(void *p_data);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _NCM_H_ */
