#include "control.h"

#define MAX_PWM_VALUE 1024
#define MIN_PWM_VALUE -1024


// 初始化
Control::Control(gpio_num_t pin_just, gpio_num_t pin_forward, ledc_mode_t pwm_mode, gpio_num_t pwm_pin)
: pin_just(pin_just), pin_forward(pin_forward), pwm_mode(pwm_mode), pwm_pin(pwm_pin)
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << pin_just) | (1ULL << pin_forward);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
}

// 设置方向
void Control::set_direction(bool direction)
{
    if (direction) {
        gpio_set_level(pin_just, 1);
        gpio_set_level(pin_forward, 0);
    } else {
        gpio_set_level(pin_just, 0);
        gpio_set_level(pin_forward, 1);
    }
}

// 设置刹车
void Control::set_brake(bool brake)
{
    if (brake) {
        gpio_set_level(pin_just, 1);
        gpio_set_level(pin_forward, 1);
    }
}

// Start of Selection
// PWM运算符重载，直接传入PWM值，根据正负控制正反转
void Control::operator=(int32_t pwm_value)
{
    // 根据PWM值的正负设置方向
    if (pwm_value > 0) {
        // 正转
        set_direction(true);
    } else if (pwm_value < 0) {
        // 反转
        set_direction(false);
        pwm_value = -pwm_value; // 取绝对值
    } else if (pwm_value == 0) {
        // PWM为0，刹车
        set_brake(true);
        return;
    }

    // 设置PWM输出（假设PWM分辨率为1024）
    if (pwm_value > MAX_PWM_VALUE) {
        pwm_value = MAX_PWM_VALUE;
    }
    else if (pwm_value < MIN_PWM_VALUE) {
        pwm_value = MIN_PWM_VALUE;
    }
    
    // 设置PWM占空比
    ledc_set_duty(pwm_mode, pwm_pin, pwm_value);
    ledc_update_duty(pwm_mode, pwm_pin);
}