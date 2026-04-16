#ifndef ADC_MODULE_H
#define ADC_MODULE_H

// ============ 头文件 ============
#include <stdint.h>
#include <stdbool.h>
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

// ================ 宏定义 ================
#define ADC_DEFAULT_SAMPLES     16      // 默认多次采样取平均次数
#define ADC_VOLTAGE_INVALID     -1.0f   // 无效电压标志值

// ================ ADC类定义 ================
class ADC_Module {
public:
    // ================ 构造函数 ================
    // 功能: 创建ADC_Module对象，所有成员初始化为默认值
    // 参数: 无
    ADC_Module();

    // ================ 析构函数 ================
    // 功能: 释放ADC句柄及校准句柄资源
    ~ADC_Module();

    // ================ 初始化 ================
    // 功能: 初始化指定ADC单元和通道，并尝试建立校准方案
    // 参数: unit    - ADC单元，ADC_UNIT_1 或 ADC_UNIT_2
    //       channel - ADC通道，如 ADC_CHANNEL_0 ~ ADC_CHANNEL_9
    //       atten   - 衰减系数，决定量程:
    //                   ADC_ATTEN_DB_0   ->  0 ~ 950 mV
    //                   ADC_ATTEN_DB_2_5 ->  0 ~ 1250 mV
    //                   ADC_ATTEN_DB_6   ->  0 ~ 1750 mV
    //                   ADC_ATTEN_DB_12  ->  0 ~ 3100 mV  (推荐测电池分压)
    // 返回: 成功返回 true，失败返回 false
    bool init(adc_unit_t unit, adc_channel_t channel,
              adc_atten_t atten = ADC_ATTEN_DB_12);

    // ================ 读取原始ADC值 ================
    // 功能: 读取单次原始ADC值（未校准，范围 0~4095）
    // 参数: 无
    // 返回: 原始ADC整数值，失败返回 -1
    int readRaw();

    // ================ 读取平均原始ADC值 ================
    // 功能: 多次采样后取平均，降低噪声
    // 参数: samples - 采样次数，默认使用 ADC_DEFAULT_SAMPLES
    // 返回: 平均原始ADC整数值，失败返回 -1
    int readRawAvg(int samples = ADC_DEFAULT_SAMPLES);

    // ================ 读取校准后电压 ================
    // 功能: 读取经过ESP-IDF ADC校准后的电压值（单位 mV）
    //       若校准不可用则使用线性公式估算
    // 参数: samples - 内部采样次数，默认 ADC_DEFAULT_SAMPLES
    // 返回: 电压值（mV），失败返回 ADC_VOLTAGE_INVALID
    float readVoltage(int samples = ADC_DEFAULT_SAMPLES);

    // ================ 读取缩放后的真实电压 ================
    // 功能: 在 readVoltage() 基础上乘以分压比，换算出真实电压
    //       适用于通过电阻分压网络检测的高电压，如电池电压:
    //       例: R_up=100k, R_down=100k，则 scale=2.0，
    //           实际电压 = ADC测到电压 * 2.0
    // 参数: scale   - 分压比（真实电压 / ADC端电压），≥1.0
    //       samples - 内部采样次数，默认 ADC_DEFAULT_SAMPLES
    // 返回: 真实电压值（mV），失败返回 ADC_VOLTAGE_INVALID
    float readVoltageScaled(float scale,
                            int samples = ADC_DEFAULT_SAMPLES);

    // ================ 检查是否已初始化 ================
    // 功能: 返回模块是否已成功初始化
    // 参数: 无
    // 返回: 已初始化返回 true，否则返回 false
    bool isInitialized() const;

    // ================ 检查校准是否可用 ================
    // 功能: 返回ADC硬件校准是否成功建立
    // 参数: 无
    // 返回: 校准可用返回 true，否则返回 false
    bool isCalibrated() const;

private:
    // ================ 内部成员变量 ================
    adc_oneshot_unit_handle_t  adc_handle;      // ADC oneshot 句柄
    adc_cali_handle_t          cali_handle;     // 校准句柄
    adc_unit_t                 adc_unit;        // 当前使用的ADC单元
    adc_channel_t              adc_channel;     // 当前使用的ADC通道
    adc_atten_t                adc_atten;       // 当前衰减系数
    bool                       initialized;     // 初始化标志
    bool                       calibrated;      // 校准可用标志

    // ================ 内部方法: 初始化校准 ================
    // 功能: 尝试创建曲线拟合或线性校准方案
    // 参数: 无
    // 返回: 成功返回 true
    bool initCalibration();

    // ================ 内部方法: 原始值转电压(mV) ================
    // 功能: 将原始ADC值通过校准或线性公式转换为电压
    // 参数: raw - 原始ADC值
    // 返回: 电压（mV）
    float rawToVoltage(int raw);
};

#endif // ADC_MODULE_H
