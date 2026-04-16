#ifndef BMP280_H
#define BMP280_H

#include <stdint.h>
#include <stdbool.h>

// ================ BMP280 I2C地址 ================
#define BMP280_ADDR 0x76  // 默认地址，可通过SDO引脚改为0x76

// ================ BMP280 寄存器地址 ================
#define BMP280_REG_CHIP_ID          0xD0
#define BMP280_REG_RESET            0xE0
#define BMP280_REG_STATUS           0xF3
#define BMP280_REG_CTRL_MEAS        0xF4
#define BMP280_REG_CONFIG           0xF5
#define BMP280_REG_PRESS_MSB        0xF7
#define BMP280_REG_PRESS_LSB        0xF8
#define BMP280_REG_PRESS_XLSB       0xF9
#define BMP280_REG_TEMP_MSB         0xFA
#define BMP280_REG_TEMP_LSB         0xFB
#define BMP280_REG_TEMP_XLSB        0xFC
#define BMP280_REG_CALIB_T1_LSB     0x88
#define BMP280_REG_CALIB_T1_MSB     0x89

// ================ BMP280 芯片ID ================
#define BMP280_CHIP_ID              0x58

// ================ BMP280 复位命令 ================
#define BMP280_RESET_CODE           0xB6

// ================ 过采样率配置 ================
typedef enum {
    BMP280_OVERSAMPLING_SKIP = 0x00,  // 跳过测量
    BMP280_OVERSAMPLING_X1   = 0x01,  // 1倍过采样
    BMP280_OVERSAMPLING_X2   = 0x02,  // 2倍过采样
    BMP280_OVERSAMPLING_X4   = 0x03,  // 4倍过采样
    BMP280_OVERSAMPLING_X8   = 0x04,  // 8倍过采样
    BMP280_OVERSAMPLING_X16  = 0x05   // 16倍过采样
} BMP280_Oversampling;

// ================ 工作模式 ================
typedef enum {
    BMP280_MODE_SLEEP   = 0x00,  // 睡眠模式
    BMP280_MODE_FORCED  = 0x01,  // 强制模式(单次测量)
    BMP280_MODE_NORMAL  = 0x03   // 正常模式(连续测量)
} BMP280_Mode;

// ================ IIR滤波系数 ================
typedef enum {
    BMP280_FILTER_OFF    = 0x00,  // 无滤波
    BMP280_FILTER_X2     = 0x01,  // 系数2
    BMP280_FILTER_X4     = 0x02,  // 系数4
    BMP280_FILTER_X8     = 0x03,  // 系数8
    BMP280_FILTER_X16    = 0x04   // 系数16
} BMP280_Filter;

// ================ 待机时间 ================
typedef enum {
    BMP280_STANDBY_0_5MS   = 0x00,  // 0.5ms
    BMP280_STANDBY_62_5MS  = 0x01,  // 62.5ms
    BMP280_STANDBY_125MS   = 0x02,  // 125ms
    BMP280_STANDBY_250MS   = 0x03,  // 250ms
    BMP280_STANDBY_500MS   = 0x04,  // 500ms
    BMP280_STANDBY_1000MS  = 0x05,  // 1000ms
    BMP280_STANDBY_2000MS  = 0x06,  // 2000ms
    BMP280_STANDBY_4000MS  = 0x07   // 4000ms
} BMP280_Standby;

// ================ 校准参数结构体 ================
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
    int32_t  t_fine;  // 温度补偿中间值
} BMP280_Calib;

// ================ I2C回调函数类型定义 ================
typedef bool (*i2c_write_byte_cb_t)(uint8_t addr, uint8_t reg, uint8_t data);
typedef bool (*i2c_read_bytes_cb_t)(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t length);

// ================ BMP280驱动类 ================
class BMP280 {
public:
    // ================ 构造函数 ================
    BMP280(i2c_write_byte_cb_t write_cb = nullptr, i2c_read_bytes_cb_t read_cb = nullptr);
    ~BMP280();

    // ================ I2C回调函数设置 ================
    void setI2CCallbacks(i2c_write_byte_cb_t write_cb, i2c_read_bytes_cb_t read_cb);

    // ================ 初始化和控制 ================
    bool init();
    bool reset();
    bool verifyChipID();

    // ================ 配置函数 ================
    bool setMode(BMP280_Mode mode, 
                 BMP280_Oversampling press_osr = BMP280_OVERSAMPLING_X16,
                 BMP280_Oversampling temp_osr = BMP280_OVERSAMPLING_X2,
                 BMP280_Filter filter = BMP280_FILTER_X16,
                 BMP280_Standby standby = BMP280_STANDBY_0_5MS);
    
    bool setFilter(BMP280_Filter filter);
    bool setStandby(BMP280_Standby standby);

    // ================ 数据读取 ================
    bool read();
    bool getTemperature(float &temperature);
    bool getPressure(float &pressure);
    bool getTempPres(float &temperature, float &pressure);
    bool getAltitude(float &altitude);
    bool getMeasurements(float &temperature, float &pressure, float &altitude);

    // ================ 原始数据获取 ================
    int16_t getRawTemperature() const { return raw_temperature; }
    int32_t getRawPressure() const { return raw_pressure; }

    // ================ 海平面气压设置(用于高度计算) ================
    void setSeaLevelPressure(float pressure) { sea_level_pressure = pressure; }
    float getSeaLevelPressure() const { return sea_level_pressure; }

private:
    // ================ I2C回调函数 ================
    i2c_write_byte_cb_t write_byte_cb;
    i2c_read_bytes_cb_t read_bytes_cb;

    // ================ 校准参数 ================
    BMP280_Calib calib;

    // ================ 原始数据 ================
    int32_t raw_temperature;
    int32_t raw_pressure;

    // ================ 海平面气压(用于高度计算) ================
    float sea_level_pressure;

    // ================ 内部寄存器操作 ================
    bool writeRegister(uint8_t reg_addr, uint8_t data);
    bool readRegister(uint8_t reg_addr, uint8_t *data, uint8_t length);

    // ================ 校准数据读取 ================
    bool readCalibration();

    // ================ Bosch补偿算法 ================
    int32_t compensateTemperature(int32_t adc_T);
    uint32_t compensatePressure(int32_t adc_P);
};

#endif // BMP280_H
