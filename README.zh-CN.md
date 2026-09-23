**语言 / Language:** [English](README.md) | [中文](README.zh-CN.md)

# 深晨 SC05-NH3 · 电化学氨气模组驱动

**厂商：** 深圳市深晨科技有限公司（Shenzhen Shenchen Technology Co., Ltd）  
**型号：** SC05-NH3  
**类型：** 电化学氨气 (NH3)，UART 主动上传  

面向深晨 **SC05-NH3** 电化学氨气模组的平台无关 C 驱动。

![SC05-NH3 电化学氨气模组](img/PixPin_2026-09-24_03-57-02.jpg)

*SC05-NH3 电化学氨气模组 · 深晨科技*

---

## 特性

- 平台无关：通过 `sensor_io_t` 注入读写与时钟，不依赖具体 MCU HAL
- 主动上传模组：调用 `get()` 即可，无需下发命令
- 浓度以 `float` PPM 输出（`conc_ppm` / `range_ppm`）
- 预热 60s：**解析成功后**才返回 `NH3_ERR_WARMUP`，与通讯失败区分
- 返回值：`0` 成功 / `1` 无帧或参数错 / `2` 预热中；仅成功时写 `out`

## 需要实现的 IO 接口

驱动不碰 HAL，只依赖你在 `init` 时注入的三个函数指针（见 `src/sensor.h`）：

```c
typedef struct sensor_io {
    /* 读串口：非阻塞，立即返回本次真正读到的字节数；0 = 暂无数据 */
    uint32_t (*read)(uint8_t *buf, uint32_t len);

    /* 写串口：本模组是单向上传，可填 NULL */
    uint32_t (*write)(const uint8_t *buf, uint32_t len);

    /* 毫秒时钟：必填；驱动靠它算超时与预热 */
    uint32_t (*now_ms)(void);
} sensor_io_t;
```

约定：

1. **`read` / `write` 一律非阻塞**——不能在里面 `delay`、等 UART 发完；读不到就返回 0，驱动自己重试到超时。
2. **`now_ms` 返回开机以来的毫秒数**（允许回绕），预热与 `wait_ms` 预算都用它。
3. 多传感器共用一条串口时，**切通道后清缓冲是调用方的事**，驱动不清 FIFO。

驱动保持平台无关：`init` 时注入 `io` 即可，不绑定任何具体 MCU 或 RTOS。

## 用法

```c
#include "sensor_sc05_nh3.h"

sensor_nh3_t ctx;
sensor_nh3_data_t data = {0};

sensor_nh3_init(&ctx, &io);            /* 注入 IO，记录预热起点 */

uint32_t err = sensor_nh3_get(&ctx, &data, NH3_WAIT_MS_TYPICAL);
if (err == NH3_OK) {
    /* data.conc_ppm / data.range_ppm 已是 float PPM */
} else if (err == NH3_ERR_WARMUP) {
    /* 链路正常，仍在 60s 预热内 */
} else {
    /* 超时 / 校验失败 / 参数错误 */
}
```

编译：把 `src/*.c` 加入工程，`src/` 加入 include 路径。

## 文件结构

| 路径 | 说明 |
|------|------|
| `src/sensor.h` | 依赖注入（IO 接口） |
| `src/sensor_sc05_nh3.h/.c` | 本模组驱动 |
| `img/` | 产品图 |
| `docs/电化学氨气模组_SC05-NH3.pdf` | 厂家用户手册 |

---

**License:** MIT  
**规格书:** 见 `docs/`
