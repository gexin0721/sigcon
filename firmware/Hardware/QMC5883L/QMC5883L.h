#ifndef QMC5883L_H
#define QMC5883L_H

// ============ 头文件 ============
#include <stdint.h>
#include <stdbool.h>

// ================ I2C回调函数类型定义 ================
// I2C写单字节回调函数类型
// 参数: 设备地址, 寄存器地址, 数据
// 返回: 成功返回true, 失败返回false
typedef bool (*i2c_write_byte_cb_t)(uint8_t dev_addr, uint8_t reg_addr, uint8_t data);

// I2C读多字节回调函数类型
// 参数: 设备地址, 寄存器地址, 数据缓冲区指针, 读取长度
// 返回: 成功返回true, 失败返回false
typedef bool (*i2c_read_bytes_cb_t)(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint8_t length);

// ================ QMC5883L寄存器地址定义 ================
#define QMC5883L_ADDR           0x0D    // QMC5883L的I2C设备地址
#define QMC5883L_REG_DATA_X_LSB 0x00    // X轴数据低字节寄存器
#define QMC5883L_REG_DATA_X_MSB 0x01    // X轴数据高字节寄存器
#define QMC5883L_REG_DATA_Y_LSB 0x02    // Y轴数据低字节寄存器
#define QMC5883L_REG_DATA_Y_MSB 0x03    // Y轴数据高字节寄存器
#define QMC5883L_REG_DATA_Z_LSB 0x04    // Z轴数据低字节寄存器
#define QMC5883L_REG_DATA_Z_MSB 0x05    // Z轴数据高字节寄存器
#define QMC5883L_REG_STATUS     0x06    // 状态寄存器
#define QMC5883L_REG_TEMP_LSB   0x07    // 温度数据低字节寄存器
#define QMC5883L_REG_TEMP_MSB   0x08    // 温度数据高字节寄存器
#define QMC5883L_REG_CONTROL1   0x09    // 控制寄存器1 (模式|ODR|量程|过采样)
#define QMC5883L_REG_CONTROL2   0x0A    // 控制寄存器2 (软复位等)
#define QMC5883L_REG_SET_RESET  0x0B    // SET/RESET周期寄存器
#define QMC5883L_REG_CHIP_ID    0x0D    // 芯片ID寄存器

// ================ 状态寄存器位定义 ================
#define QMC5883L_STATUS_DRDY    0x01    // 数据准备好标志
#define QMC5883L_STATUS_OVL     0x02    // 溢出标志
#define QMC5883L_STATUS_DOR     0x04    // 数据跳过标志

// ================ 工作模式定义 ================
#define QMC5883L_MODE_STANDBY   0x00    // 待机模式
#define QMC5883L_MODE_CONTINUOUS 0x01   // 连续测量模式

// ================ 输出数据率(ODR)定义 ================
#define QMC5883L_ODR_10HZ       0x00    // 输出数据率 10Hz
#define QMC5883L_ODR_50HZ       0x04    // 输出数据率 50Hz
#define QMC5883L_ODR_100HZ      0x08    // 输出数据率 100Hz
#define QMC5883L_ODR_200HZ      0x0C    // 输出数据率 200Hz

// ================ 量程(RNG)定义 ================
#define QMC5883L_RNG_2G         0x00    // 量程 ±2 高斯
#define QMC5883L_RNG_8G         0x10    // 量程 ±8 高斯

// ================ 过采样率(OSR)定义 ================
#define QMC5883L_OSR_512        0x00    // 过采样率 512
#define QMC5883L_OSR_256        0x40    // 过采样率 256
#define QMC5883L_OSR_128        0x80    // 过采样率 128
#define QMC5883L_OSR_64         0xC0    // 过采样率 64

// ================ 控制寄存器2定义 ================
#define QMC5883L_SOFT_RESET     0x80    // 软复位命令

// ================ QMC5883L类定义 ================
class QMC5883L {
public:
    // ================ 构造函数 ================
    // 功能: 创建QMC5883L对象
    // 参数: write_cb - I2C写回调函数指针(可选)
    //       read_cb - I2C读回调函数指针(可选)
    QMC5883L(i2c_write_byte_cb_t write_cb = nullptr, i2c_read_bytes_cb_t read_cb = nullptr);

    // ================ 析构函数 ================
    ~QMC5883L();

    // ================ 设置I2C回调函数 ================
    // 功能: 设置I2C读写操作的回调函数
    // 参数: write_cb - I2C写单字节回调函数
    //       read_cb - I2C读多字节回调函数
    // 返回: 无
    // 说明: 必须在调用init()之前设置回调函数
    void setI2CCallbacks(i2c_write_byte_cb_t write_cb, i2c_read_bytes_cb_t read_cb);

    // ================ 初始化传感器 ================
    // 功能: 初始化QMC5883L传感器,配置为默认工作模式
    // 参数: 无
    // 返回: 成功返回true, 失败返回false
    // 说明: 默认配置为连续模式、100Hz输出、8G量程、512过采样率
    //       调用此函数前必须先设置I2C回调函数
    bool init();

    // ================ 软复位传感器 ================
    // 功能: 对QMC5883L传感器执行软件复位
    // 参数: 无
    // 返回: 成功返回true, 失败返回false
    // 说明: 复位后需要重新调用init()进行初始化
    bool reset();

    // ================ 校准传感器 ================
    // 功能: 校准QMC5883L传感器
    // 参数: 无
    // 返回: 成功返回true, 失败返回false
    // 说明: 校准后需要重新调用init()进行初始化
    bool calibrate();

    // ================ 设置工作模式 ================
    // 功能: 配置QMC5883L的工作模式
    // 参数: mode - 工作模式 (QMC5883L_MODE_STANDBY 或 QMC5883L_MODE_CONTINUOUS)
    //       odr - 输出数据率 (QMC5883L_ODR_10HZ ~ QMC5883L_ODR_200HZ)
    //       rng - 量程 (QMC5883L_RNG_2G 或 QMC5883L_RNG_8G)
    //       osr - 过采样率 (QMC5883L_OSR_512 ~ QMC5883L_OSR_64)
    // 返回: 成功返回true, 失败返回false
    bool setMode(uint8_t mode, uint8_t odr, uint8_t rng, uint8_t osr);

    // ================ 读取三轴磁场数据 ================
    // 功能: 从QMC5883L读取X、Y、Z三轴的原始磁场数据
    // 参数: 无
    // 返回: 成功返回true, 失败返回false
    // 说明: 读取的数据保存在内部变量中,通过getX/Y/Z()获取
    bool read();

    // ================ 获取X轴数据 ================
    // 功能: 获取X轴磁场强度数据
    // 参数: 无
    // 返回: X轴磁场强度(16位有符号整数)
    // 说明: 需要先调用read()读取数据
    int16_t getX();

    // ================ 获取Y轴数据 ================
    // 功能: 获取Y轴磁场强度数据
    // 参数: 无
    // 返回: Y轴磁场强度(16位有符号整数)
    // 说明: 需要先调用read()读取数据
    int16_t getY();

    // ================ 获取Z轴数据 ================
    // 功能: 获取Z轴磁场强度数据
    // 参数: 无
    // 返回: Z轴磁场强度(16位有符号整数)
    // 说明: 需要先调用read()读取数据
    int16_t getZ();

    // ================ 获取三轴原始数据 ================
    // 功能: 一次性获取X、Y、Z三轴的磁场数据
    // 参数: x - X轴数据输出引用
    //       y - Y轴数据输出引用
    //       z - Z轴数据输出引用
    // 返回: 无
    // 说明: 需要先调用read()读取数据
    void getRawData(int16_t &x, int16_t &y, int16_t &z);

    // ================ 验证芯片ID ================
    // 功能: 验证QMC5883L芯片是否正确连接
    // 参数: 无
    // 返回: 成功返回true, 失败返回false
    bool verifyChipID();

    // ================ 检查数据是否准备好 ================
    // 功能: 检查QMC5883L是否有新的数据可读
    // 参数: 无
    // 返回: 数据准备好返回true, 否则返回false
    bool isDataReady();

    // ================ 获取方位角 ================
    // 功能: 计算磁场方向的方位角(0-360度)
    // 参数: 无
    // 返回: 方位角(0-360度), 0度为北方
    // 说明: 需要先调用read()读取数据，建议先进行校准
    float getAzimuth();

    // ================ 设置磁偏角 ================
    // 功能: 设置磁偏角补偿(磁北与真北的偏差)
    // 参数: degrees - 磁偏角度数
    //       minutes - 磁偏角分数(0-59)
    // 返回: 无
    // 说明: 不同地区的磁偏角不同，可从地图或GPS获取
    void setMagneticDeclination(float degrees, uint8_t minutes = 0);

    // ================ 设置校准参数(最小最大值) ================
    // 功能: 通过最小最大值设置校准参数
    // 参数: x_min, x_max - X轴最小最大值
    //       y_min, y_max - Y轴最小最大值
    //       z_min, z_max - Z轴最小最大值
    // 返回: 无
    // 说明: 通过旋转传感器360度采集数据，记录各轴最小最大值
    void setCalibration(int16_t x_min, int16_t x_max,
                        int16_t y_min, int16_t y_max,
                        int16_t z_min, int16_t z_max);

    // ================ 设置校准偏移量 ================
    // 功能: 设置校准偏移量(用于消除硬铁干扰)
    // 参数: x_offset, y_offset, z_offset - 各轴偏移量
    // 返回: 无
    void setCalibrationOffsets(float x_offset, float y_offset, float z_offset);

    // ================ 设置校准缩放因子 ================
    // 功能: 设置校准缩放因子(用于消除软铁干扰)
    // 参数: x_scale, y_scale, z_scale - 各轴缩放因子
    // 返回: 无
    void setCalibrationScales(float x_scale, float y_scale, float z_scale);

    // ================ 获取校准偏移量 ================
    // 功能: 获取指定轴的校准偏移量
    // 参数: index - 轴索引(0=X, 1=Y, 2=Z)
    // 返回: 校准偏移量
    float getCalibrationOffset(uint8_t index);

    // ================ 获取校准缩放因子 ================
    // 功能: 获取指定轴的校准缩放因子
    // 参数: index - 轴索引(0=X, 1=Y, 2=Z)
    // 返回: 校准缩放因子
    float getCalibrationScale(uint8_t index);

    // ================ 清除校准参数 ================
    // 功能: 清除所有校准参数，恢复默认值
    // 参数: 无
    // 返回: 无
    void clearCalibration();

    // ================ 获取校准后的数据 ================
    // 功能: 获取经过校准处理的三轴磁场数据
    // 参数: x - X轴数据输出引用
    //       y - Y轴数据输出引用
    //       z - Z轴数据输出引用
    // 返回: 无
    // 说明: 需要先调用read()读取数据
    void getCalibratedData(float &x, float &y, float &z);

    // ================ 获取温度 ================
    // 功能: 读取QMC5883L内部温度传感器数据
    // 参数: 无
    // 返回: 温度值(℃)
    float getTemperature();

private:
    // ================ I2C回调函数指针 ================
    i2c_write_byte_cb_t write_byte_cb;  // I2C写单字节回调函数指针
    i2c_read_bytes_cb_t read_bytes_cb;  // I2C读多字节回调函数指针

    // ================ 原始数据缓冲区 ================
    int16_t raw_x;  // X轴原始数据
    int16_t raw_y;  // Y轴原始数据
    int16_t raw_z;  // Z轴原始数据

    // ================ 校准参数 ================
    float calibration_offset[3];  // 校准偏移量 [X, Y, Z]
    float calibration_scale[3];   // 校准缩放因子 [X, Y, Z]
    float calibrated_x;           // 校准后的X轴数据
    float calibrated_y;           // 校准后的Y轴数据
    float calibrated_z;           // 校准后的Z轴数据

    // ================ 磁偏角补偿 ================
    float magnetic_declination;   // 磁偏角(度数)

    // ================ 内部寄存器写入方法 ================
    // 功能: 向QMC5883L的寄存器写入单字节数据
    // 参数: reg_addr - 寄存器地址
    //       data - 要写入的数据
    // 返回: 成功返回true, 失败返回false
    bool writeRegister(uint8_t reg_addr, uint8_t data);

    // ================ 内部寄存器读取方法 ================
    // 功能: 从QMC5883L的寄存器读取多字节数据
    // 参数: reg_addr - 起始寄存器地址
    //       data - 数据缓冲区指针
    //       length - 读取长度
    // 返回: 成功返回true, 失败返回false
    bool readRegister(uint8_t reg_addr, uint8_t *data, uint8_t length);

    // ================ 应用校准参数 ================
    // 功能: 对原始数据应用校准参数(偏移量和缩放因子)
    // 参数: 无
    // 返回: 无
    // 说明: 内部方法，在read()后自动调用
    void applyCalibration();
};

#endif // QMC5883L_H
