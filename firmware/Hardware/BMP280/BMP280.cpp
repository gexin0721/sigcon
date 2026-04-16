#include "BMP280.h"
#include <math.h>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// ================ 构造函数 ================
// 功能: 创建BMP280对象，可选地设置I2C回调函数
// 参数: write_cb - I2C写回调函数指针(可选)
//       read_cb - I2C读回调函数指针(可选)
BMP280::BMP280(i2c_write_byte_cb_t write_cb, i2c_read_bytes_cb_t read_cb) {
    write_byte_cb = write_cb;
    read_bytes_cb = read_cb;

    // 初始化原始数据
    raw_temperature = 0;
    raw_pressure = 0;

    // 初始化校准参数
    memset(&calib, 0, sizeof(BMP280_Calib));

    // 初始化海平面气压为标准值
    sea_level_pressure = 1013.25f;
}

// ================ 析构函数 ================
BMP280::~BMP280() {
    // 无需特殊清理操作
}

// ================ 设置I2C回调函数 ================
// 功能: 设置I2C读写操作的回调函数
// 参数: write_cb - I2C写单字节回调函数
//       read_cb - I2C读多字节回调函数
void BMP280::setI2CCallbacks(i2c_write_byte_cb_t write_cb, i2c_read_bytes_cb_t read_cb) {
    write_byte_cb = write_cb;
    read_bytes_cb = read_cb;
}

// ================ 内部寄存器写入方法 ================
// 功能: 向BMP280的寄存器写入单字节数据
// 参数: reg_addr - 寄存器地址
//       data - 要写入的数据
// 返回: 成功返回true, 失败返回false
bool BMP280::writeRegister(uint8_t reg_addr, uint8_t data) {
    if (write_byte_cb == nullptr) {
        return false;
    }
    return write_byte_cb(BMP280_ADDR, reg_addr, data);
}

// ================ 内部寄存器读取方法 ================
// 功能: 从BMP280的寄存器读取多字节数据
// 参数: reg_addr - 起始寄存器地址
//       data - 数据缓冲区指针
//       length - 读取长度
// 返回: 成功返回true, 失败返回false
bool BMP280::readRegister(uint8_t reg_addr, uint8_t *data, uint8_t length) {
    if (read_bytes_cb == nullptr || data == nullptr) {
        return false;
    }
    return read_bytes_cb(BMP280_ADDR, reg_addr, data, length);
}

// ================ 软复位传感器 ================
// 功能: 对BMP280传感器执行软件复位
// 参数: 无
// 返回: 成功返回true, 失败返回false
bool BMP280::reset() {
    return writeRegister(BMP280_REG_RESET, BMP280_RESET_CODE);
}

// ================ 验证芯片ID ================
bool BMP280::verifyChipID() {
    uint8_t chip_id = 0;
    
    if (!readRegister(BMP280_REG_CHIP_ID, &chip_id, 1)) {
        return false;
    }
    
    return (chip_id == BMP280_CHIP_ID);
}

// ================ 读取校准参数 ================
// 功能: 从BMP280读取出厂校准参数
// 参数: 无
// 返回: 成功返回true, 失败返回false
bool BMP280::readCalibration() {
    uint8_t calib_data[26];
    
    // 读取校准参数(从0x88开始，共26字节)
    if (!readRegister(BMP280_REG_CALIB_T1_LSB, calib_data, 26)) {
        return false;
    }

    // 解析校准参数
    calib.dig_T1 = (uint16_t)(calib_data[1] << 8 | calib_data[0]);
    calib.dig_T2 = (int16_t)(calib_data[3] << 8 | calib_data[2]);
    calib.dig_T3 = (int16_t)(calib_data[5] << 8 | calib_data[4]);
    
    calib.dig_P1 = (uint16_t)(calib_data[7] << 8 | calib_data[6]);
    calib.dig_P2 = (int16_t)(calib_data[9] << 8 | calib_data[8]);
    calib.dig_P3 = (int16_t)(calib_data[11] << 8 | calib_data[10]);
    calib.dig_P4 = (int16_t)(calib_data[13] << 8 | calib_data[12]);
    calib.dig_P5 = (int16_t)(calib_data[15] << 8 | calib_data[14]);
    calib.dig_P6 = (int16_t)(calib_data[17] << 8 | calib_data[16]);
    calib.dig_P7 = (int16_t)(calib_data[19] << 8 | calib_data[18]);
    calib.dig_P8 = (int16_t)(calib_data[21] << 8 | calib_data[20]);
    calib.dig_P9 = (int16_t)(calib_data[23] << 8 | calib_data[22]);

    return true;
}

// ================ 初始化传感器 ================
// 功能: 初始化BMP280传感器，配置为默认工作模式
// 参数: 无
// 返回: 成功返回true, 失败返回false
bool BMP280::init() {
    // // 步骤1: 验证芯片ID
    // if (!verifyChipID()) {
    //     return false;
    // }

    // 步骤2: 软复位
    if (!reset()) {
        return false;
    }
    // 等待复位完成(BMP280数据手册要求约2ms，取10ms保险)
    vTaskDelay(10 / portTICK_PERIOD_MS);

    // 步骤3: 读取校准参数
    if (!readCalibration()) {
        return false;
    }

    // 步骤4: 配置工作模式(默认配置)
    // 正常模式 | 气压16倍过采样 | 温度2倍过采样 | IIR滤波系数16 | 待机0.5ms
    if (!setMode(BMP280_MODE_NORMAL,
                 BMP280_OVERSAMPLING_X16,
                 BMP280_OVERSAMPLING_X2,
                 BMP280_FILTER_X16,
                 BMP280_STANDBY_0_5MS)) {
        return false;
    }

    return true;
}

// ================ 设置工作模式 ================
// 功能: 配置BMP280的工作模式和过采样率
// 参数: mode - 工作模式
//       press_osr - 气压过采样率
//       temp_osr - 温度过采样率
//       filter - IIR滤波系数
//       standby - 待机时间
// 返回: 成功返回true, 失败返回false
bool BMP280::setMode(BMP280_Mode mode,
                     BMP280_Oversampling press_osr,
                     BMP280_Oversampling temp_osr,
                     BMP280_Filter filter,
                     BMP280_Standby standby) {
    // 设置控制测量寄存器(0xF4)
    // bit7-5: 温度过采样 | bit4-2: 气压过采样 | bit1-0: 工作模式
    uint8_t ctrl_meas = (temp_osr << 5) | (press_osr << 2) | mode;
    if (!writeRegister(BMP280_REG_CTRL_MEAS, ctrl_meas)) {
        return false;
    }

    // 设置配置寄存器(0xF5)
    // bit7-5: 待机时间 | bit4-2: IIR滤波系数
    uint8_t config = (standby << 5) | (filter << 2);
    if (!writeRegister(BMP280_REG_CONFIG, config)) {
        return false;
    }

    return true;
}

// ================ 设置IIR滤波 ================
bool BMP280::setFilter(BMP280_Filter filter) {
    uint8_t config = 0;
    if (!readRegister(BMP280_REG_CONFIG, &config, 1)) {
        return false;
    }
    
    config = (config & 0xE3) | (filter << 2);  // 清除bit4-2，设置新值
    return writeRegister(BMP280_REG_CONFIG, config);
}

// ================ 设置待机时间 ================
bool BMP280::setStandby(BMP280_Standby standby) {
    uint8_t config = 0;
    if (!readRegister(BMP280_REG_CONFIG, &config, 1)) {
        return false;
    }
    
    config = (config & 0x1F) | (standby << 5);  // 清除bit7-5，设置新值
    return writeRegister(BMP280_REG_CONFIG, config);
}

// ================ 读取三轴数据 ================
// 功能: 从BMP280读取温度和气压的原始ADC值
// 参数: 无
// 返回: 成功返回true, 失败返回false
bool BMP280::read() {
    uint8_t buffer[6];

    // 从寄存器0xF7开始连续读取6字节数据
    // (气压MSB, LSB, XLSB, 温度MSB, LSB, XLSB)
    if (!readRegister(BMP280_REG_PRESS_MSB, buffer, 6)) {
        return false;
    }

    // 组合气压数据(20位)
    raw_pressure = (int32_t)(buffer[0] << 12 | buffer[1] << 4 | buffer[2] >> 4);

    // 组合温度数据(20位)
    raw_temperature = (int32_t)(buffer[3] << 12 | buffer[4] << 4 | buffer[5] >> 4);

    return true;
}

// ================ 获取温度测量值 ================
// 功能: 获取温度测量值(单位: 摄氏度)
// 参数: temperature - 温度输出引用
// 返回: 成功返回true, 失败返回false
bool BMP280::getTemperature(float &temperature) {
    if (!read()) {
        return false;
    }

    int32_t temp = compensateTemperature(raw_temperature);
    temperature = (float)temp / 100.0f;  // 补偿函数返回值 × 100
    return true;
}

// ================ 获取气压测量值 ================
// 功能: 获取气压测量值(单位: Pa)
// 参数: pressure - 气压输出引用
// 返回: 成功返回true, 失败返回false
bool BMP280::getPressure(float &pressure) {
    float temperature;
    return getTempPres(temperature, pressure);
}

// ================ 同时获取温度和气压 ================
// 功能: 一次读取同时获取温度和气压
// 参数: temperature - 温度输出引用(摄氏度)
//       pressure - 气压输出引用(Pa)
// 返回: 成功返回true, 失败返回false
bool BMP280::getTempPres(float &temperature, float &pressure) {
    if (!read()) {
        return false;
    }

    int32_t temp = compensateTemperature(raw_temperature);
    uint32_t pres = compensatePressure(raw_pressure);

    temperature = (float)temp / 100.0f;
    pressure = (float)pres / 256.0f;  // Q24.8格式转换
    return true;
}

// ================ 获取海拔高度 ================
// 功能: 根据气压计算海拔高度
// 参数: altitude - 高度输出引用(单位: 米)
// 返回: 成功返回true, 失败返回false
bool BMP280::getAltitude(float &altitude) {
    float temperature, pressure;
    return getMeasurements(temperature, pressure, altitude);
}

// ================ 获取所有测量值 ================
// 功能: 获取温度、气压和海拔高度
// 参数: temperature - 温度输出引用(摄氏度)
//       pressure - 气压输出引用(Pa)
//       altitude - 高度输出引用(米)
// 返回: 成功返回true, 失败返回false
bool BMP280::getMeasurements(float &temperature, float &pressure, float &altitude) {
    if (!getTempPres(temperature, pressure)) {
        return false;
    }

    // 使用气压高度公式计算海拔高度
    // h = (T0/L) * ((P0/P)^(R*L/g*M) - 1)
    // 简化为: h = (T + 273.15) / 0.0065 * ((P0/P)^0.190223 - 1)
    float pressure_hpa = pressure / 100.0f;  // 转换为hPa
    altitude = ((float)powf(sea_level_pressure / pressure_hpa, 0.190223f) - 1.0f) * 
               (temperature + 273.15f) / 0.0065f;

    return true;
}

// ================ Bosch温度补偿算法 ================
// 功能: 使用Bosch官方算法补偿温度ADC值
// 参数: adc_T - 温度ADC原始值
// 返回: 补偿后的温度值(×100，单位0.01℃)
// 说明: 返回值5123表示51.23℃
int32_t BMP280::compensateTemperature(int32_t adc_T) {
    int32_t var1, var2, T;

    var1 = ((((adc_T >> 3) - ((int32_t)calib.dig_T1 << 1))) * ((int32_t)calib.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)calib.dig_T1)) * ((adc_T >> 4) - ((int32_t)calib.dig_T1))) >> 12) *
            ((int32_t)calib.dig_T3)) >> 14;
    
    calib.t_fine = var1 + var2;
    T = (calib.t_fine * 5 + 128) >> 8;

    return T;
}

// ================ Bosch气压补偿算法 ================
// 功能: 使用Bosch官方算法补偿气压ADC值
// 参数: adc_P - 气压ADC原始值
// 返回: 补偿后的气压值(Q24.8格式，单位Pa)
// 说明: 返回值24674867表示24674867/256 = 96386.2 Pa
uint32_t BMP280::compensatePressure(int32_t adc_P) {
    int64_t var1, var2, p;

    var1 = ((int64_t)calib.t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)calib.dig_P3) >> 8) + ((var1 * (int64_t)calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calib.dig_P1) >> 33;

    if (var1 == 0) {
        return 0;  // 避免除以零
    }

    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)calib.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)calib.dig_P7) << 4);

    return (uint32_t)p;
}
