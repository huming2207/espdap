#include "ui_commander.hpp"
#include "lcd/display_manager.hpp"

esp_err_t ui_commander::init()
{
    task_queue = display_manager::instance()->get_ui_queue();
    if (task_queue == nullptr) {
        ESP_LOGE(TAG, "LVGL needs to be init first");
        return ESP_ERR_INVALID_STATE;
    }

    return ESP_OK;
}

esp_err_t ui_commander::display_init()
{
    if (task_queue == nullptr) {
        ESP_LOGE(TAG, "LVGL needs to be init first");
        return ESP_ERR_INVALID_STATE;
    }

    ui_state::queue_item item = {};
    item.state = ui_state::STATE_INIT;

    xQueueSend(task_queue, &item, portMAX_DELAY);
    return ESP_OK;
}

esp_err_t ui_commander::display_chip_erase()
{
    ui_state::queue_item item = {};
    item.state = ui_state::STATE_ERASE;

    xQueueSend(task_queue, &item, portMAX_DELAY);
    return ESP_OK;
}

esp_err_t ui_commander::display_flash(ui_state::flash_screen *screen)
{
    if (screen == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    ui_state::queue_item item = {};
    item.state = ui_state::STATE_FLASH;
    strncpy(item.comment, screen->subtitle, sizeof(ui_state::queue_item::comment));
    item.comment[sizeof(ui_state::queue_item::comment) - 1] = '\0';

    xQueueSend(task_queue, &item, portMAX_DELAY);
    return ESP_OK;
}

esp_err_t ui_commander::display_test(ui_state::test_screen *screen)
{
    if (screen == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    ui_state::queue_item item = {};
    item.state = ui_state::STATE_TEST;
    item.total_count = screen->total_test;
    item.percentage = screen->done_test;
    strncpy(item.comment, screen->subtitle, sizeof(ui_state::queue_item::comment));
    item.comment[sizeof(ui_state::queue_item::comment) - 1] = '\0';

    xQueueSend(task_queue, &item, portMAX_DELAY);
    return ESP_OK;
}

esp_err_t ui_commander::display_error(ui_state::error_screen *screen)
{
    if (screen == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    ui_state::queue_item item = {};
    item.state = ui_state::STATE_ERROR;
    if (strlen(screen->comment) < 1) {
        strncpy(item.comment, esp_err_to_name(screen->ret), sizeof(ui_state::queue_item::comment));
    } else {
        strncpy(item.comment, screen->comment, sizeof(ui_state::queue_item::comment));
    }

    strncpy(item.qrcode, screen->qrcode, sizeof(ui_state::queue_item::qrcode));

    item.comment[sizeof(ui_state::queue_item::comment) - 1] = '\0';
    item.qrcode[sizeof(ui_state::queue_item::qrcode) - 1] = '\0';

    xQueueSend(task_queue, &item, portMAX_DELAY);
    return ESP_OK;
}

esp_err_t ui_commander::display_done()
{
    ui_state::queue_item item = {};
    item.state = ui_state::STATE_DONE;

    xQueueSend(task_queue, &item, portMAX_DELAY);
    return ESP_OK;
}

esp_err_t ui_commander::display_usb()
{
    ui_state::queue_item item = {};
    item.state = ui_state::STATE_USB;

    xQueueSend(task_queue, &item, portMAX_DELAY);
    return ESP_OK;
}

esp_err_t ui_commander::display_anything(ui_state::queue_item *item)
{
    if (item == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    xQueueSend(task_queue, item, portMAX_DELAY);
    return ESP_OK;
}

esp_err_t ui_commander::display_error(esp_err_t ret, const char *qrcode, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    ui_state::queue_item item = {};
    item.state = ui_state::STATE_ERROR;
    if (fmt == nullptr || strlen(fmt) < 1) {
        snprintf(item.comment, sizeof(ui_state::queue_item::comment), "0x%x\n%s", ret, esp_err_to_name(ret));
    } else {
        if (vsnprintf(item.comment, sizeof(ui_state::queue_item::comment), fmt, args) < 0) {
            return ESP_FAIL;
        }
    }

    strncpy(item.qrcode, qrcode, sizeof(ui_state::queue_item::qrcode));

    item.comment[sizeof(ui_state::queue_item::comment) - 1] = '\0';
    item.qrcode[sizeof(ui_state::queue_item::qrcode) - 1] = '\0';

    xQueueSend(task_queue, &item, portMAX_DELAY);
    va_end(args);
    return ESP_OK;
}
