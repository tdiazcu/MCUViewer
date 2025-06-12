#ifndef BACKEND_H
#define BACKEND_H

#include <stdint.h>

    typedef struct _ngspice_backend {
        void (*close) (ngspice_t * sl);
        int32_t (*exit_debug_mode) (ngspice_t * sl);
        int32_t (*enter_swd_mode) (ngspice_t * sl);
        int32_t (*enter_jtag_mode) (ngspice_t * stl);
        int32_t (*exit_dfu_mode) (ngspice_t * stl);
        int32_t (*core_id) (ngspice_t * stl);
        int32_t (*reset) (ngspice_t * stl);
        int32_t (*jtag_reset) (ngspice_t * stl, int32_t value);
        int32_t (*run) (ngspice_t * stl, enum run_type type);
        int32_t (*status) (ngspice_t * stl);
        int32_t (*version) (ngspice_t *sl);
        int32_t (*read_debug32) (ngspice_t *sl, uint32_t addr, uint32_t *data);
        int32_t (*read_mem32) (ngspice_t *sl, uint32_t addr, uint16_t len);
        int32_t (*write_debug32) (ngspice_t *sl, uint32_t addr, uint32_t data);
        int32_t (*write_mem32) (ngspice_t *sl, uint32_t addr, uint16_t len);
        int32_t (*write_mem8) (ngspice_t *sl, uint32_t addr, uint16_t len);
        int32_t (*read_all_regs) (ngspice_t *sl, struct ngspice_reg * regp);
        int32_t (*read_reg) (ngspice_t *sl, int32_t r_idx, struct ngspice_reg * regp);
        int32_t (*read_all_unsupported_regs) (ngspice_t *sl, struct ngspice_reg *regp);
        int32_t (*read_unsupported_reg) (ngspice_t *sl, int32_t r_idx, struct ngspice_reg *regp);
        int32_t (*write_unsupported_reg) (ngspice_t *sl, uint32_t value, int32_t idx, struct ngspice_reg *regp);
        int32_t (*write_reg) (ngspice_t *sl, uint32_t reg, int32_t idx);
        int32_t (*step) (ngspice_t * stl);
        int32_t (*current_mode) (ngspice_t * stl);
        int32_t (*force_debug) (ngspice_t *sl);
        int32_t (*target_voltage) (ngspice_t *sl);
        int32_t (*set_swdclk) (ngspice_t * stl, int32_t freq_khz);
        int32_t (*trace_enable) (ngspice_t * sl, uint32_t frequency);
        int32_t (*trace_disable) (ngspice_t * sl);
        int32_t (*trace_read) (ngspice_t * sl, uint8_t* buf, uint32_t size);
    } ngspice_backend_t;

#endif // BACKEND_H
