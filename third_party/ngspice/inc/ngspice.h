/*
 * File: ngspice.h
 *
 * All common top level ngspice interfaces, regardless of how the backend does the work....
 */

#ifndef NGSPICE_H
#define NGSPICE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NGSPICE_ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

/* Max data transfer size */
// 6kB = max mem32_read block, 8kB sram
// #define Q_BUF_LEN    96
#define Q_BUF_LEN (1024 * 100)

/* Statuses of core */
enum target_state {
    TARGET_UNKNOWN = 0,
    TARGET_RUNNING = 1,
    TARGET_HALTED = 2,
    TARGET_RESET = 3,
    TARGET_DEBUG_RUNNING = 4,
};

#define NGSPICE_CORE_RUNNING             0x80
#define NGSPICE_CORE_HALTED              0x81

/* NGSPICE modes */
#define NGSPICE_DEV_DFU_MODE             0x00
#define NGSPICE_DEV_MASS_MODE            0x01
#define NGSPICE_DEV_DEBUG_MODE           0x02
#define NGSPICE_DEV_UNKNOWN_MODE           -1

/* NRST pin states */
#define NGSPICE_DEBUG_APIV2_DRIVE_NRST_LOW  0x00
#define NGSPICE_DEBUG_APIV2_DRIVE_NRST_HIGH 0x01

/* Baud rate divisors for SWDCLK */
#define NGSPICE_SWDCLK_4MHZ_DIVISOR        0
#define NGSPICE_SWDCLK_1P8MHZ_DIVISOR      1
#define NGSPICE_SWDCLK_1P2MHZ_DIVISOR      2
#define NGSPICE_SWDCLK_950KHZ_DIVISOR      3
#define NGSPICE_SWDCLK_480KHZ_DIVISOR      7
#define NGSPICE_SWDCLK_240KHZ_DIVISOR     15
#define NGSPICE_SWDCLK_125KHZ_DIVISOR     31
#define NGSPICE_SWDCLK_100KHZ_DIVISOR     40
#define NGSPICE_SWDCLK_50KHZ_DIVISOR      79
#define NGSPICE_SWDCLK_25KHZ_DIVISOR     158
#define NGSPICE_SWDCLK_15KHZ_DIVISOR     265
#define NGSPICE_SWDCLK_5KHZ_DIVISOR      798

#define NGSPICE_SERIAL_LENGTH             24
#define NGSPICE_SERIAL_BUFFER_SIZE        (NGSPICE_SERIAL_LENGTH + 1)

#define NGSPICE_V3_MAX_FREQ_NB            10

#define NGSPICE_V2_TRACE_BUF_LEN            2048
#define NGSPICE_V3_TRACE_BUF_LEN            8192
#define NGSPICE_V2_MAX_TRACE_FREQUENCY   2000000
#define NGSPICE_V3_MAX_TRACE_FREQUENCY  24000000
#define NGSPICE_DEFAULT_TRACE_FREQUENCY  2000000

/* Map the relevant features, quirks and workaround for specific firmware version of ngspice */
#define NGSPICE_F_HAS_TRACE              (1 << 0)
#define NGSPICE_F_HAS_SWD_SET_FREQ       (1 << 1)
#define NGSPICE_F_HAS_JTAG_SET_FREQ      (1 << 2)
#define NGSPICE_F_HAS_MEM_16BIT          (1 << 3)
#define NGSPICE_F_HAS_GETLASTRWSTATUS2   (1 << 4)
#define NGSPICE_F_HAS_DAP_REG            (1 << 5)
#define NGSPICE_F_QUIRK_JTAG_DP_READ     (1 << 6)
#define NGSPICE_F_HAS_AP_INIT            (1 << 7)
#define NGSPICE_F_HAS_DPBANKSEL          (1 << 8)
#define NGSPICE_F_HAS_RW8_512BYTES       (1 << 9)

/* Error code */
#define NGSPICE_DEBUG_ERR_OK              0x80
#define NGSPICE_DEBUG_ERR_FAULT           0x81
#define NGSPICE_DEBUG_ERR_WRITE           0x0c
#define NGSPICE_DEBUG_ERR_WRITE_VERIFY    0x0d
#define NGSPICE_DEBUG_ERR_AP_WAIT         0x10
#define NGSPICE_DEBUG_ERR_AP_FAULT       0x11
#define NGSPICE_DEBUG_ERR_AP_ERROR       0x12
#define NGSPICE_DEBUG_ERR_DP_WAIT        0x14
#define NGSPICE_DEBUG_ERR_DP_FAULT       0x15
#define NGSPICE_DEBUG_ERR_DP_ERROR       0x16

#define CMD_CHECK_NO         0
#define CMD_CHECK_REP_LEN    1
#define CMD_CHECK_STATUS     2
#define CMD_CHECK_RETRY      3 /* check status and retry if wait error */

#define C_BUF_LEN 32

struct ngspice_reg {
    uint32_t r[16];
    uint32_t s[32];
    uint32_t xpsr;
    uint32_t main_sp;
    uint32_t process_sp;
    uint32_t rw;
    uint32_t rw2;
    uint8_t control;
    uint8_t faultmask;
    uint8_t basepri;
    uint8_t primask;
    uint32_t fpscr;
};

typedef uint32_t ngspice_addr_t;

typedef struct ngspice_version_ {
    uint32_t major;
    uint32_t minor;
    uint32_t release;
} ngspice_version_t;

enum transport_type {
    TRANSPORT_TYPE_ZERO = 0,
    TRANSPORT_TYPE_LIBSG,
    TRANSPORT_TYPE_LIBUSB,
    TRANSPORT_TYPE_INVALID
};

enum connect_type {
    CONNECT_HOT_PLUG = 0,
    CONNECT_NORMAL = 1,
    CONNECT_UNDER_RESET = 2,
};

enum reset_type {
    RESET_AUTO = 0,
    RESET_HARD = 1,
    RESET_SOFT = 2,
    RESET_SOFT_AND_HALT = 3,
};

enum run_type {
    RUN_NORMAL = 0,
    RUN_FLASH_LOADER = 1,
};

typedef struct _ngspice ngspice_t;

#include <backend.h>

struct _ngspice {
    struct _ngspice_backend *backend;
    void *backend_data;

    // room for the command header
    unsigned char c_buf[C_BUF_LEN];
    // data transferred from or to device
    unsigned char q_buf[Q_BUF_LEN];
    int32_t q_len;

    // transport layer verboseness: 0 for no debug info, 10 for lots
    int32_t verbose;

    char serial[NGSPICE_SERIAL_BUFFER_SIZE];
    int32_t freq;

    struct ngspice_version_ version;

    uint32_t chip_flags;         // stlink_chipid_params.flags, set by stlink_load_device_params(), values: CHIP_F_xxx

    uint32_t max_trace_freq;     // set by stlink_open_usb()
};

ngspice_t* ngspice_open_udp(uint32_t ip, uint32_t port);

/* Functions defined in common.c */

int32_t ngspice_enter_swd_mode(ngspice_t *sl);
int32_t ngspice_enter_jtag_mode(ngspice_t *sl);
int32_t ngspice_exit_debug_mode(ngspice_t *sl);
int32_t ngspice_exit_dfu_mode(ngspice_t *sl);
void ngspice_close(ngspice_t *sl);
int32_t ngspice_core_id(ngspice_t *sl);
int32_t ngspice_reset(ngspice_t *sl, enum reset_type type);
int32_t ngspice_run(ngspice_t *sl, enum run_type type);
int32_t ngspice_status(ngspice_t *sl);
int32_t ngspice_version(ngspice_t *sl);
int32_t ngspice_read_debug32(ngspice_t *sl, uint32_t addr, uint32_t *data);
int32_t ngspice_read_mem32(ngspice_t *sl, uint32_t addr, uint16_t len);
int32_t ngspice_write_debug32(ngspice_t *sl, uint32_t addr, uint32_t data);
int32_t ngspice_write_mem32(ngspice_t *sl, uint32_t addr, uint16_t len);
int32_t ngspice_write_mem8(ngspice_t *sl, uint32_t addr, uint16_t len);
int32_t ngspice_read_all_regs(ngspice_t *sl, struct ngspice_reg *regp);
int32_t ngspice_read_all_unsupported_regs(ngspice_t *sl, struct ngspice_reg *regp);
int32_t ngspice_read_reg(ngspice_t *sl, int32_t r_idx, struct ngspice_reg *regp);
int32_t ngspice_read_unsupported_reg(ngspice_t *sl, int32_t r_idx, struct ngspice_reg *regp);
int32_t ngspice_write_unsupported_reg(ngspice_t *sl, uint32_t value, int32_t r_idx, struct ngspice_reg *regp);
int32_t ngspice_write_reg(ngspice_t *sl, uint32_t reg, int32_t idx);
int32_t ngspice_step(ngspice_t *sl);
int32_t ngspice_current_mode(ngspice_t *sl);
int32_t ngspice_force_debug(ngspice_t *sl);
int32_t ngspice_target_voltage(ngspice_t *sl);
int32_t ngspice_set_swdclk(ngspice_t *sl, int32_t freq_khz);
int32_t ngspice_trace_enable(ngspice_t* sl, uint32_t frequency);
int32_t ngspice_trace_disable(ngspice_t* sl);
int32_t ngspice_trace_read(ngspice_t* sl, uint8_t* buf, uint32_t size);
int32_t ngspice_parse_ihex(const char* path, uint8_t erased_pattern, uint8_t * * mem, uint32_t * size, uint32_t * begin);
uint8_t ngspice_get_erased_pattern(ngspice_t *sl);
int32_t ngspice_mwrite_sram(ngspice_t *sl, uint8_t* data, uint32_t length, ngspice_addr_t addr);

uint16_t read_uint16(const unsigned char *c, const int32_t pt);
//void ngspice_core_stat(ngspice_t *sl);
//void ngspice_print_data(ngspice_t *sl);
uint32_t is_bigendian(void);
uint32_t read_uint32(const unsigned char *c, const int32_t pt);

void write_uint32(unsigned char* buf, uint32_t ui);
void write_uint16(unsigned char* buf, uint16_t ui);

bool ngspice_is_core_halted(ngspice_t *sl);

int32_t ngspice_fread(ngspice_t* sl, const char* path, bool is_ihex, ngspice_addr_t addr, uint32_t size);
int32_t ngspice_load_device_params(ngspice_t *sl);
int32_t ngspice_target_connect(ngspice_t *sl, enum connect_type connect);

#ifdef __cplusplus
}
#endif

#endif // NGSPICE_H
