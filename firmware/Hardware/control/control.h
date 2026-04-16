#ifndef CONTROL_H
#define CONTROL_H

#include "driver/gpio.h"
#include "esp_log.h"


// 电机类
struct Control {
    gpio_num_t pin_just;
    gpio_num_t pin_forward;
    // 模式
    ledc_mode_t pwm_mode;
    // PWM引脚
    gpio_num_t pwm_pin;

    // 初始化
    Control(gpio_num_t pin_just, gpio_num_t pin_forward, ledc_mode_t pwm_mode, gpio_num_t pwm_pin);

    // 设置方向
    void set_direction(bool direction);
    
    // 设置刹车
    void set_brake(bool brake);

    // Start of Selection
    // PWM运算符重载，直接传入PWM值，根据正负控制正反转
    void operator=(int32_t pwm_value);
};


#endif