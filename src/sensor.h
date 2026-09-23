/********************************** (C) COPYRIGHT *******************************
 * Copyright (c) 2026 createskyblue@outlook.com MIT
 *******************************************************************************/
/**
 ******************************************************************************
 * @file    sensor.h
 * @brief   Dependency-injection IO interface for sensor drivers
 *
 * Platform packages UART read/write and a millisecond clock into sensor_io_t
 * and injects it once at init. Drivers never include platform headers.
 *
 * read / write are non-blocking and return the number of bytes actually
 * transferred. now_ms returns a millisecond tick (wrap-around is OK).
 * If several sensors share one UART, flushing leftover RX bytes after a
 * channel switch is the caller's job — the driver only parses frames.
 ******************************************************************************
 */

#ifndef SENSOR_H_
#define SENSOR_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Platform IO: inject at init, then treat as read-only.
 *
 * Lengths and return values are byte counts as uint32_t.
 * There is no negative error code.
 */
typedef struct sensor_io {
    /*! Read bytes: non-blocking; returns bytes read; 0 = no data now */
    uint32_t (*read)(uint8_t *buf, uint32_t len);
    /*! Write bytes: non-blocking; returns bytes written; < len = short write.
     *  May be NULL for upload-only sensors. */
    uint32_t (*write)(const uint8_t *buf, uint32_t len);
    /*! Millisecond tick: required for timeouts and warm-up */
    uint32_t (*now_ms)(void);
} sensor_io_t;

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_H_ */
