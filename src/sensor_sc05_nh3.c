/********************************** (C) COPYRIGHT *******************************
 * Copyright (c) 2026 createskyblue@outlook.com MIT
 *******************************************************************************/
/**
 ******************************************************************************
 * @file    sensor_sc05_nh3.c
 * @brief   Shenchen SC05-NH3 NH3 module driver implementation
 *
 * Vendor: Shenzhen Shenchen Technology Co., Ltd
 * Model:  SC05-NH3 (electrochemical NH3)
 *
 * Byte-at-a-time do-while with a SEEK → RECV → VERIFY state machine.
 * Deadline is fixed for the whole call. Results are written only to the
 * caller's out buffer (no internal cache). This driver does not flush UART.
 ******************************************************************************
 */

#include "sensor_sc05_nh3.h"

/* Frame field indices */
#define NH3_IDX_START      0u
#define NH3_IDX_GAS_NAME   1u
#define NH3_IDX_UNIT       2u
#define NH3_IDX_DECIMALS   3u
#define NH3_IDX_CONC_HI    4u
#define NH3_IDX_CONC_LO    5u
#define NH3_IDX_FS_HI      6u
#define NH3_IDX_FS_LO      7u
#define NH3_IDX_CHECK      8u

/* Receive state machine (local to this call) */
typedef enum {
    NH3_ST_SEEK = 0,    /*!< Hunt for 0xFF */
    NH3_ST_RECV,        /*!< Collect Byte1..Byte8 */
    NH3_ST_VERIFY       /*!< 9 bytes in; checksum */
} nh3_state_t;

void sensor_nh3_init(sensor_nh3_t *ctx, const sensor_io_t *io)
{
    ctx->io = io;
    ctx->warmup_start_ms = (io != NULL && io->now_ms != NULL) ? io->now_ms() : 0u;
}

uint32_t sensor_nh3_get(sensor_nh3_t *ctx, sensor_nh3_data_t *out, uint32_t wait_ms)
{
    uint8_t      frame[NH3_FRAME_LEN];
    uint8_t      idx = 0u;
    uint8_t      sum;
    uint8_t      i;
    nh3_state_t  state = NH3_ST_SEEK;
    uint32_t     deadline;

    if (ctx == NULL || out == NULL || ctx->io == NULL ||
        ctx->io->read == NULL || ctx->io->now_ms == NULL) {
        return NH3_ERR_IO;
    }

    deadline = ctx->io->now_ms() + wait_ms;

    do {
        uint8_t byte;

        if (ctx->io->read(&byte, 1u) != 1u) {
            continue;
        }

        switch (state) {
        case NH3_ST_SEEK:
            if (byte != NH3_START_BYTE) {
                break;
            }
            frame[0] = byte;
            idx      = 1u;
            state    = NH3_ST_RECV;
            break;

        case NH3_ST_RECV:
            frame[idx++] = byte;
            if (idx < NH3_FRAME_LEN) {
                break;
            }
            state = NH3_ST_VERIFY;
            /* FALLTHRU */

        case NH3_ST_VERIFY:
            /* Checksum ~(Byte1 + ... + Byte7) + 1 */
            sum = 0u;
            for (i = NH3_IDX_GAS_NAME; i <= NH3_IDX_FS_LO; i++) {
                sum = (uint8_t)(sum + frame[i]);
            }
            if ((uint8_t)((uint8_t)(~sum) + 1u) != frame[NH3_IDX_CHECK]) {
                state = NH3_ST_SEEK;
                idx   = 0u;
                break;
            }
            /* Frame accepted — warm-up here so it is not confused with I/O errors */
            if ((uint32_t)(ctx->io->now_ms() - ctx->warmup_start_ms) < NH3_WARMUP_MS) {
                return NH3_ERR_WARMUP;
            }
            out->gas       = frame[NH3_IDX_GAS_NAME];
            out->decimals  = frame[NH3_IDX_DECIMALS];
            out->conc_ppm  = (float)(((uint16_t)frame[NH3_IDX_CONC_HI] << 8) |
                                      (uint16_t)frame[NH3_IDX_CONC_LO]) / 10.0f;
            out->range_ppm = (float)(((uint16_t)frame[NH3_IDX_FS_HI] << 8) |
                                      (uint16_t)frame[NH3_IDX_FS_LO]) / 10.0f;
            out->valid     = 1u;
            return NH3_OK;
        }
    } while ((int32_t)(ctx->io->now_ms() - deadline) < 0);

    return NH3_ERR_IO;
}
