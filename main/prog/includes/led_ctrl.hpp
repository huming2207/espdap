#pragma once

class led_ctrl
{
public:
    static led_ctrl& instance()
    {
        static led_ctrl instance;
        return instance;
    }

    led_ctrl(manifest_mgr const &) = delete;
    void operator=(led_ctrl const &) = delete;

private:
    led_ctrl() = default;
};

