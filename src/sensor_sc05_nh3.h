/********************************** (C) COPYRIGHT *******************************
 * Copyright (c) 2026 createskyblue@outlook.com MIT
 *******************************************************************************/
/**
 ******************************************************************************
 * @file    sensor_sc05_nh3.h
 * @brief   Platform-independent driver for Shenchen SC05-NH3 NH3 module
 *
 * Vendor:  Shenzhen Shenchen Technology Co., Ltd
 * Model:   SC05-NH3 (electrochemical NH3)
 * Link:    UART 9600/8N1, factory auto-upload, fixed 9-byte frames
 *
 * One-way (sensor uploads only) — no TX path in this driver.
 * IO is injected via sensor_io_t; the driver never includes platform headers.
 ******************************************************************************
 */

#ifndef SENSOR_SC05_NH3_H_
#define SENSOR_SC05_NH3_H_

#include "sensor.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Protocol constants */
#define NH3_FRAME_LEN     9u      /*!< Frame length including header and checksum */
#define NH3_START_BYTE    0xFFu   /*!< Frame header */
#define NH3_GAS_NH3       0x17u   /*!< Gas id: ammonia */

/*! Module uploads about once per second; leave a little slack above one frame */
#ifndef NH3_WAIT_MS_TYPICAL
#define NH3_WAIT_MS_TYPICAL 1500u
#endif

/*! Warm-up after init: 60s (checked only after a successful parse) */
#ifndef NH3_WARMUP_MS
#define NH3_WARMUP_MS       60000u
#endif

/* get() results: parse first, then warm-up — avoids mixing with I/O errors */
#define NH3_OK              0u  /*!< Parsed OK and warm-up done; out written */
#define NH3_ERR_IO          1u  /*!< Bad args / no frame; out untouched */
#define NH3_ERR_WARMUP      2u  /*!< Frame OK but warm-up not finished; out untouched */

/*! Parsed sample (driver fills data only; no string formatting) */
typedef struct {
    uint8_t  valid;           /*!< 1 = at least one good frame received */
    uint8_t  gas;             /*!< Gas name byte (NH3_GAS_NH3) */
    uint8_t  decimals;        /*!< Decimal places from the frame */
    float    conc_ppm;        /*!< Concentration in PPM */
    float    range_ppm;       /*!< Full scale in PPM */
} sensor_nh3_data_t;

/*!
 * Driver context. No internal sample cache: results go straight to the
 * caller's out buffer. Warm-up is judged after a successful parse.
 */
typedef struct {
    const sensor_io_t *io;    /*!< Injected IO (read / now_ms) */
    uint32_t warmup_start_ms; /*!< now_ms() at init */
} sensor_nh3_t;

/**
 * @brief Inject IO and record warm-up start time
 */
void sensor_nh3_init(sensor_nh3_t *ctx, const sensor_io_t *io);

/**
 * @brief Read one sample
 * @param ctx      Driver context
 * @param out      Written only on NH3_OK; left unchanged otherwise. Must not be NULL
 * @param wait_ms  Hard time budget for the whole call (ms); 0 = do not wait
 * @retval NH3_OK (0) — frame received, checksum OK, warm-up finished
 * @retval NH3_ERR_IO (1) — no frame or bad arguments
 * @retval NH3_ERR_WARMUP (2) — frame OK but still within 60s warm-up
 *
 * State machine SEEK → RECV → VERIFY. Warm-up is checked after VERIFY
 * and before writing out, so WARMUP means the link and module are alive.
 * Flushing a shared UART is the caller's responsibility.
 */
uint32_t sensor_nh3_get(sensor_nh3_t *ctx, sensor_nh3_data_t *out, uint32_t wait_ms);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_SC05_NH3_H_ */
