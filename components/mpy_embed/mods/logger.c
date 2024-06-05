#ifndef NO_QSTR
#include <esp_log.h>
#endif
#include "py/runtime.h"

static mp_obj_t log_debug(mp_obj_t tag, mp_obj_t log)
{
    const char *tag_str = mp_obj_is_str(tag) ? mp_obj_str_get_str(tag) : "mpy_default";
    const char *log_str = mp_obj_is_str(log) ? mp_obj_str_get_str(log) : "";
    esp_log_write(ESP_LOG_DEBUG, tag_str, "%s", log_str);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(logger_log_debug, log_debug);

static mp_obj_t log_info(mp_obj_t tag, mp_obj_t log)
{
    const char *tag_str = mp_obj_is_str(tag) ? mp_obj_str_get_str(tag) : "mpy_default";
    const char *log_str = mp_obj_is_str(log) ? mp_obj_str_get_str(log) : "";
    esp_log_write(ESP_LOG_INFO, tag_str, "%s", log_str);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(logger_log_info, log_info);

static mp_obj_t log_warn(mp_obj_t tag, mp_obj_t log)
{
    const char *tag_str = mp_obj_is_str(tag) ? mp_obj_str_get_str(tag) : "mpy_default";
    const char *log_str = mp_obj_is_str(log) ? mp_obj_str_get_str(log) : "";
    esp_log_write(ESP_LOG_WARN, tag_str, "%s", log_str);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(logger_log_warn, log_warn);

static mp_obj_t log_error(mp_obj_t tag, mp_obj_t log)
{
    const char *tag_str = mp_obj_is_str(tag) ? mp_obj_str_get_str(tag) : "mpy_default";
    const char *log_str = mp_obj_is_str(log) ? mp_obj_str_get_str(log) : "";
    esp_log_write(ESP_LOG_ERROR, tag_str, "%s", log_str);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(logger_log_error, log_error);

static const mp_rom_map_elem_t logger_module_globals_table[] = {
        { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_logger) },
        { MP_ROM_QSTR(MP_QSTR_debug), MP_ROM_PTR(&logger_log_debug) },
        { MP_ROM_QSTR(MP_QSTR_info), MP_ROM_PTR(&logger_log_info) },
        { MP_ROM_QSTR(MP_QSTR_warn), MP_ROM_PTR(&logger_log_warn) },
        { MP_ROM_QSTR(MP_QSTR_error), MP_ROM_PTR(&logger_log_error) },
};
static MP_DEFINE_CONST_DICT(logger_module_globals, logger_module_globals_table);

const mp_obj_module_t logger_module = {
        .base = { &mp_type_module },
        .globals = (mp_obj_dict_t *)&logger_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_logger, logger_module);