#include "mpy_bootstrap.h"
#include "port/micropython_embed.h"

esp_err_t mpy_runtime_bootstrap(uint8_t *heap_ptr, size_t heap_size, const char *script)
{
    int stack_top;
    mp_embed_init(heap_ptr, heap_size, &stack_top);

    // Run the example scripts (they will be compiled first).
    mp_embed_exec_str(script);

    // Deinitialise MicroPython.
    mp_embed_deinit();

    return ESP_OK;
}
