#include "rgb_led_module.hpp"
#include "led_ctrl.hpp"

esp_err_t rgb_led_module::init()
{
    bool ret = wasm_runtime_register_natives(name(), const_cast<NativeSymbol *>(symbols), func_count());
    return ret ? ESP_OK : ESP_FAIL;
}

esp_err_t rgb_led_module::deinit()
{
    return ESP_OK;
}

const char *rgb_led_module::name() const
{
    return "rgb_led";
}

size_t rgb_led_module::func_count() const
{
    return sizeof(symbols) / sizeof(NativeSymbol);
}

void rgb_led_module::set_color(uint8_t r, uint8_t g, uint8_t b)
{
    led_ctrl::instance().set_color(r, g, b, 50);
}
