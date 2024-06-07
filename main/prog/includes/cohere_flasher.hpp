#pragma once

#include <freertos/FreeRTOS.h>
#include <esp_err.h>

class cohere_flasher
{
public:
    static cohere_flasher *instance()
    {
        static cohere_flasher instance;
        return &instance;
    }

    cohere_flasher(cohere_flasher const &) = delete;
    void operator=(cohere_flasher const &) = delete;

public:
    enum cohere_states : uint32_t {
        STATE_INIT = 0,
        STATE_CHECK_BINARY,
        STATE_FETCH_BIN_FW,
        STATE_FETCH_BIN_ALGO,
        STATE_EXEC_IDENT,
        STATE_EXEC_SELF_TEST,
        STATE_EXEC_EXTERN_TEST,
        STATE_EXEC_ERASE,
        STATE_EXEC_FW_PROG,
        STATE_EXEC_FW_VERIFY,
        STATE_EXEC_DONE,
        STATE_ERROR,
    };

public:
    esp_err_t init();
    esp_err_t decode_message(const char *topic, uint8_t *buf, size_t len);

private:
    esp_err_t on_check_binary();
    esp_err_t on_fetch_firmware();
    esp_err_t on_fetch_flash_algo();
    esp_err_t on_exec_ident();
    esp_err_t on_exec_self_test();
    esp_err_t on_exec_extern_test();
    esp_err_t on_exec_erase();
    esp_err_t on_exec_fw_prog();
    esp_err_t on_exec_fw_verify();
    esp_err_t on_exec_done();
    esp_err_t on_error();

private:
    cohere_flasher() = default;
};
