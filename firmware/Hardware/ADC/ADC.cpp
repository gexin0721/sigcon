#include "ADC.h"
#include "esp_log.h"

static const char *TAG = "ADC_Module";

// ================ 构造函数 ================
// 功能: 初始化所有成员变量为默认值，不执行硬件操作
ADC_Module::ADC_Module()
    : adc_handle(nullptr),
      cali_handle(nullptr),
      adc_unit(ADC_UNIT_1),
      adc_channel(ADC_CHANNEL_0),
      adc_atten(ADC_ATTEN_DB_12),
      initialized(false),
      calibrated(false)
{
}

// ================ 析构函数 ================
// 功能: 释放 ADC oneshot 句柄和校准句柄
ADC_Module::~ADC_Module()
{
    // 释放校准句柄
    if (calibrated && cali_handle != nullptr) {
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
        adc_cali_delete_scheme_curve_fitting(cali_handle);
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
        adc_cali_delete_scheme_line_fitting(cali_handle);
#endif
        cali_handle = nullptr;
    }

    // 释放 ADC oneshot 句柄
    if (adc_handle != nullptr) {
        adc_oneshot_del_unit(adc_handle);
        adc_handle = nullptr;
    }
}

// ================ 初始化 ================
// 功能: 创建 ADC oneshot 句柄，配置通道，并尝试建立校准
// 参数: unit    - ADC单元
//       channel - ADC通道
//       atten   - 衰减系数（量程选择）
// 返回: 成功返回 true，失败返回 false
bool ADC_Module::init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten)
{
    adc_unit    = unit;
    adc_channel = channel;
    adc_atten   = atten;

    // ---- 步骤1: 创建 ADC oneshot 单元句柄 ----
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = adc_unit,
    };
    esp_err_t ret = adc_oneshot_new_unit(&unit_cfg, &adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_new_unit failed: %s", esp_err_to_name(ret));
        return false;
    }

    // ---- 步骤2: 配置通道（位宽固定12位，衰减由参数指定） ----
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = adc_atten,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ret = adc_oneshot_config_channel(adc_handle, adc_channel, &chan_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_config_channel failed: %s", esp_err_to_name(ret));
        adc_oneshot_del_unit(adc_handle);
        adc_handle = nullptr;
        return false;
    }

    initialized = true;
    ESP_LOGI(TAG, "ADC unit=%d channel=%d atten=%d initialized", unit, channel, atten);

    // ---- 步骤3: 尝试建立硬件校准（可选，失败不影响基本读值） ----
    initCalibration();

    return true;
}

// ================ 内部方法: 初始化校准 ================
// 功能: 依次尝试曲线拟合校准和线性校准方案
// 返回: 成功建立任一方案返回 true
bool ADC_Module::initCalibration()
{
    calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    // 曲线拟合校准（ESP32-S3 支持）
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id  = adc_unit,
        .chan     = adc_channel,
        .atten    = adc_atten,
        .bitwidth = ADC_BITWIDTH_12,
    };
    esp_err_t ret = adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali_handle);
    if (ret == ESP_OK) {
        calibrated = true;
        ESP_LOGI(TAG, "ADC calibration: curve fitting scheme");
        return true;
    }
    ESP_LOGW(TAG, "Curve fitting calibration failed: %s", esp_err_to_name(ret));
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    // 线性拟合校准（ESP32 / ESP32-S2 等）
    adc_cali_line_fitting_config_t line_cfg = {
        .unit_id       = adc_unit,
        .atten         = adc_atten,
        .bitwidth      = ADC_BITWIDTH_12,
    };
    esp_err_t ret2 = adc_cali_create_scheme_line_fitting(&line_cfg, &cali_handle);
    if (ret2 == ESP_OK) {
        calibrated = true;
        ESP_LOGI(TAG, "ADC calibration: line fitting scheme");
        return true;
    }
    ESP_LOGW(TAG, "Line fitting calibration failed: %s", esp_err_to_name(ret2));
#endif

    ESP_LOGW(TAG, "ADC running without hardware calibration");
    return false;
}

// ================ 读取原始ADC值 ================
// 功能: 单次读取原始ADC值
// 返回: 原始值(0~4095)，失败返回 -1
int ADC_Module::readRaw()
{
    if (!initialized) {
        ESP_LOGE(TAG, "ADC not initialized");
        return -1;
    }

    int raw = 0;
    esp_err_t ret = adc_oneshot_read(adc_handle, adc_channel, &raw);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_read failed: %s", esp_err_to_name(ret));
        return -1;
    }
    return raw;
}

// ================ 读取平均原始ADC值 ================
// 功能: 多次采样取算术平均，抑制高频噪声
// 参数: samples - 采样次数
// 返回: 平均原始值，失败返回 -1
int ADC_Module::readRawAvg(int samples)
{
    if (!initialized) {
        ESP_LOGE(TAG, "ADC not initialized");
        return -1;
    }
    if (samples <= 0) samples = 1;

    long sum = 0;
    int  valid = 0;
    for (int i = 0; i < samples; i++) {
        int raw = readRaw();
        if (raw >= 0) {
            sum += raw;
            valid++;
        }
    }
    if (valid == 0) return -1;
    return (int)(sum / valid);
}

// ================ 内部方法: 原始值转电压(mV) ================
// 功能: 优先使用硬件校准转换，无校准时使用线性估算
// 参数: raw - 原始ADC值
// 返回: 电压（mV）
float ADC_Module::rawToVoltage(int raw)
{
    if (raw < 0) return ADC_VOLTAGE_INVALID;

    if (calibrated && cali_handle != nullptr) {
        // 使用 ESP-IDF 硬件校准转换
        int voltage_mv = 0;
        esp_err_t ret = adc_cali_raw_to_voltage(cali_handle, raw, &voltage_mv);
        if (ret == ESP_OK) {
            return (float)voltage_mv;
        }
        ESP_LOGW(TAG, "adc_cali_raw_to_voltage failed, fallback to linear");
    }

    // 无校准时的线性估算
    // ADC_ATTEN_DB_12: 满量程约 3100 mV，12位分辨率 4095
    float full_scale_mv = 3100.0f;
    switch (adc_atten) {
        case ADC_ATTEN_DB_0:   full_scale_mv =  950.0f; break;
        case ADC_ATTEN_DB_2_5: full_scale_mv = 1250.0f; break;
        case ADC_ATTEN_DB_6:   full_scale_mv = 1750.0f; break;
        case ADC_ATTEN_DB_12:  full_scale_mv = 3100.0f; break;
        default:               full_scale_mv = 3100.0f; break;
    }
    return ((float)raw / 4095.0f) * full_scale_mv;
}

// ================ 读取校准后电压 ================
// 功能: 多次采样平均后转换为电压（单位 mV）
// 参数: samples - 采样次数
// 返回: 电压（mV），失败返回 ADC_VOLTAGE_INVALID
float ADC_Module::readVoltage(int samples)
{
    int raw = readRawAvg(samples);
    return rawToVoltage(raw);
}

// ================ 读取缩放后的真实电压 ================
// 功能: 读取电压后乘以分压比，换算出真实端点电压
// 参数: scale   - 分压比（真实电压 / ADC端电压），例如两个等阻值分压 scale=2.0
//       samples - 采样次数
// 返回: 真实电压（mV），失败返回 ADC_VOLTAGE_INVALID
float ADC_Module::readVoltageScaled(float scale, int samples)
{
    float voltage = readVoltage(samples);
    if (voltage == ADC_VOLTAGE_INVALID) return ADC_VOLTAGE_INVALID;
    if (scale < 1.0f) scale = 1.0f;    // 分压比最小为1，防止缩小
    return voltage * scale;
}

// ================ 检查是否已初始化 ================
bool ADC_Module::isInitialized() const
{
    return initialized;
}

// ================ 检查校准是否可用 ================
bool ADC_Module::isCalibrated() const
{
    return calibrated;
}
