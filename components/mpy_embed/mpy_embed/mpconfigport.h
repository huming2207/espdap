/* This file is part of the MicroPython project, http://micropython.org/
 * The MIT License (MIT)
 * Copyright (c) 2022-2023 Damien P. George
 */

// Include common MicroPython embed configuration.
#include <port/mpconfigport_common.h>

// Use the minimal starting configuration (disables all optional features).
#define MICROPY_CONFIG_ROM_LEVEL                (MICROPY_CONFIG_ROM_LEVEL_MINIMUM)

// MicroPython configuration.
#define MICROPY_ENABLE_COMPILER                 (1)
#define MICROPY_ENABLE_GC                       (1)
#define MICROPY_PY_GC                           (1)
#define MICROPY_GCREGS_SETJMP (1)
#define MICROPY_COMP_CONST_FOLDING_COMPILER_WORKAROUND (1)
#define MICROPY_PY_ERRNO                        (1)
#define MICROPY_PY_STRUCT                       (1)
#define MICROPY_PY_MATH                         (1)
#define MICROPY_PY_BUILTINS_SET                 (1)
#define MICROPY_PY_ATTRTUPLE                    (1)
#define MICROPY_PY_COLLECTIONS                  (1)
#define MICROPY_FLOAT_IMPL                      (2)
#define MICROPY_HW_BOARD_NAME "soulinjector"
#define MICROPY_HW_MCU_NAME   "soulplatform"