#include "QMC5883L.h"
#include <math.h>
#include "esp_timer.h"

// ================ 构造函数 ================
// 功能: 创建QMC5883L对象，可选地设置I2C回调函数
// 参数: write_cb - I2C写回调函数指针(可选)
//       read_cb - I2C读回调函数指针(可选)
QMC5883L::QMC5883L(i2c_write_byte_cb_t write_cb, i2c_read_bytes_cb_t read_cb) {
    // 保存回调函数指针
    write_byte_cb = write_cb;
    read_bytes_cb = read_cb;

    // 初始化原始数据为0
    raw_x = 0;
    raw_y = 0;
    raw_z = 0;

    // 初始化校准参数为默认值(无校准)
    calibration_offset[0] = 0.0f;  // X轴偏移量
    calibration_offset[1] = 0.0f;  // Y轴偏移量
    calibration_offset[2] = 0.0f;  // Z轴偏移量
    calibration_scale[0] = 1.0f;   // X轴缩放因子
    calibration_scale[1] = 1.0f;   // Y轴缩放因子
    calibration_scale[2] = 1.0f;   // Z轴缩放因子

    // 初始化校准后的数据
    calibrated_x = 0.0f;
    calibrated_y = 0.0f;
    calibrated_z = 0.0f;

    // 初始化磁偏角为0
    magnetic_declination = 0.0f;
}

// ================ 析构函数 ================
QMC5883L::~QMC5883L() {
    // 无需特殊清理操作
}

// ================ 设置I2C回调函数 ================
// 功能: 设置I2C读写操作的回调函数
// 参数: write_cb - I2C写单字节回调函数
//       read_cb - I2C读多字节回调函数
void QMC5883L::setI2CCallbacks(i2c_write_byte_cb_t write_cb, i2c_read_bytes_cb_t read_cb) {
    write_byte_cb = write_cb;
    read_bytes_cb = read_cb;
}

// ================ 内部寄存器写入方法 ================
// 功能: 向QMC5883L的寄存器写入单字节数据
// 参数: reg_addr - 寄存器地址
//       data - 要写入的数据
// 返回: 成功返回true, 失败返回false
bool QMC5883L::writeRegister(uint8_t reg_addr, uint8_t data) {
    // 检查回调函数是否已设置
    if (write_byte_cb == nullptr) {
        return false;
    }

    // 调用回调函数执行I2C写操作
    return write_byte_cb(QMC5883L_ADDR, reg_addr, data);
}

// ================ 内部寄存器读取方法 ================
// 功能: 从QMC5883L的寄存器读取多字节数据
// 参数: reg_addr - 起始寄存器地址
//       data - 数据缓冲区指针
//       length - 读取长度
// 返回: 成功返回true, 失败返回false
bool QMC5883L::readRegister(uint8_t reg_addr, uint8_t *data, uint8_t length) {
    // 检查回调函数是否已设置
    if (read_bytes_cb == nullptr) {
        return false;
    }

    // 检查数据缓冲区指针是否有效
    if (data == nullptr) {
        return false;
    }

    // 调用回调函数执行I2C读操作
    return read_bytes_cb(QMC5883L_ADDR, reg_addr, data, length);
}

// ================ 软复位传感器 ================
// 功能: 对QMC5883L传感器执行软件复位
// 参数: 无
// 返回: 成功返回true, 失败返回false
// 说明: 复位后需要重新调用init()进行初始化
bool QMC5883L::reset() {
    // 向控制寄存器2写入软复位命令
    return writeRegister(QMC5883L_REG_CONTROL2, QMC5883L_SOFT_RESET);
}

// ================ 校准传感器 ================
// 功能: 校准QMC5883L传感器
// 参数: 无
// 返回: 成功返回true, 失败返回false
// 说明: 校准后需要重新调用init()进行初始化
bool QMC5883L::calibrate() {
	clearCalibration();  // 清除之前的校准数据
	// 初始化校准数据数组，存储每个轴的最小值和最大值
	long calibrationData[3][2] = {{65000, -65000}, {65000, -65000}, {65000, -65000}};
  	long	x = calibrationData[0][0] = calibrationData[0][1] = getX();  // 初始化X轴数据
  	long	y = calibrationData[1][0] = calibrationData[1][1] = getY();  // 初始化Y轴数据
  	long	z = calibrationData[2][0] = calibrationData[2][1] = getZ();  // 初始化Z轴数据

	unsigned long startTime = (unsigned long)(esp_timer_get_time() / 1000);  // 记录开始时间(ms)

	// 在10秒内循环读取传感器数据
	while(((unsigned long)(esp_timer_get_time() / 1000) - startTime) < 10000) {
		read();  // 读取传感器数据

  		x = getX();  // 获取X轴值
  		y = getY();  // 获取Y轴值
  		z = getZ();  // 获取Z轴值

		// 记录X轴的最小值
		if(x < calibrationData[0][0]) {
			calibrationData[0][0] = x;
		}
		// 记录X轴的最大值
		if(x > calibrationData[0][1]) {
			calibrationData[0][1] = x;
		}

		// 记录Y轴的最小值
		if(y < calibrationData[1][0]) {
			calibrationData[1][0] = y;
		}
		// 记录Y轴的最大值
		if(y > calibrationData[1][1]) {
			calibrationData[1][1] = y;
		}

		// 记录Z轴的最小值
		if(z < calibrationData[2][0]) {
			calibrationData[2][0] = z;
		}
		// 记录Z轴的最大值
		if(z > calibrationData[2][1]) {
			calibrationData[2][1] = z;
		}
	}

	// 使用收集到的最大值和最小值进行校准
	setCalibration(
		calibrationData[0][0],
		calibrationData[0][1],
		calibrationData[1][0],
		calibrationData[1][1],
		calibrationData[2][0],
		calibrationData[2][1]
	);
	return true;
}


// ================ 设置工作模式 ================
// 功能: 配置QMC5883L的工作模式
// 参数: mode - 工作模式 (QMC5883L_MODE_STANDBY 或 QMC5883L_MODE_CONTINUOUS)
//       odr - 输出数据率 (QMC5883L_ODR_10HZ ~ QMC5883L_ODR_200HZ)
//       rng - 量程 (QMC5883L_RNG_2G 或 QMC5883L_RNG_8G)
//       osr - 过采样率 (QMC5883L_OSR_512 ~ QMC5883L_OSR_64)
// 返回: 成功返回true, 失败返回false
bool QMC5883L::setMode(uint8_t mode, uint8_t odr, uint8_t rng, uint8_t osr) {
    // 组合配置参数: 模式 | 输出数据率 | 量程 | 过采样率
    uint8_t config = mode | odr | rng | osr;

    // 写入控制寄存器1
    return writeRegister(QMC5883L_REG_CONTROL1, config);
}

// ================ 初始化传感器 ================
// 功能: 初始化QMC5883L传感器,配置为默认工作模式
// 参数: 无
// 返回: 成功返回true, 失败返回false
// 说明: 默认配置为连续模式、100Hz输出、8G量程、512过采样率
//       调用此函数前必须先设置I2C回调函数
bool QMC5883L::init() {
    // 检查回调函数是否已设置
    if (write_byte_cb == nullptr || read_bytes_cb == nullptr) {
        return false;
    }

    // // 步骤1: 验证芯片ID
    // if (!verifyChipID()) {
    //     return false;
    // }

    // 步骤2: 设置SET/RESET周期寄存器
    // 推荐值为0x01，用于周期性地对传感器进行SET/RESET操作，提高测量精度
    if (!writeRegister(QMC5883L_REG_SET_RESET, 0x01)) {
        return false;
    }

    // 步骤3: 配置工作模式
    // 连续测量模式 | 100Hz输出 | 8G量程 | 512过采样率
    // 0x01 | 0x08 | 0x10 | 0x00 = 0x19
    if (!setMode(QMC5883L_MODE_CONTINUOUS,
                 QMC5883L_ODR_100HZ,
                 QMC5883L_RNG_8G,
                 QMC5883L_OSR_512)) {
        return false;
    }
    
    return true;
}

// ================ 读取三轴磁场数据 ================
// 功能: 从QMC5883L读取X、Y、Z三轴的原始磁场数据
// 参数: 无
// 返回: 成功返回true, 失败返回false
// 说明: 读取的数据保存在内部变量中,通过getX/Y/Z()获取
bool QMC5883L::read() {
    uint8_t buffer[6];  // 数据缓冲区，存储6字节数据(X_LSB, X_MSB, Y_LSB, Y_MSB, Z_LSB, Z_MSB)

    // 从寄存器0x00开始连续读取6字节数据
    if (!readRegister(QMC5883L_REG_DATA_X_LSB, buffer, 6)) {
        return false;
    }

    // 组合LSB和MSB为16位有符号整数
    // QMC5883L数据格式: LSB在前，MSB在后
    raw_x = (int16_t)(buffer[0] | (buffer[1] << 8));
    raw_y = (int16_t)(buffer[2] | (buffer[3] << 8));
    raw_z = (int16_t)(buffer[4] | (buffer[5] << 8));

    // 应用校准参数
    applyCalibration();

    return true;
}

// ================ 获取X轴数据 ================
// 功能: 获取X轴磁场强度数据
// 参数: 无
// 返回: X轴磁场强度(16位有符号整数)
// 说明: 需要先调用read()读取数据
int16_t QMC5883L::getX() {
    return raw_x;
}

// ================ 获取Y轴数据 ================
// 功能: 获取Y轴磁场强度数据
// 参数: 无
// 返回: Y轴磁场强度(16位有符号整数)
// 说明: 需要先调用read()读取数据
int16_t QMC5883L::getY() {
    return raw_y;
}

// ================ 获取Z轴数据 ================
// 功能: 获取Z轴磁场强度数据
// 参数: 无
// 返回: Z轴磁场强度(16位有符号整数)
// 说明: 需要先调用read()读取数据
int16_t QMC5883L::getZ() {
    return raw_z;
}

// ================ 获取三轴原始数据 ================
// 功能: 一次性获取X、Y、Z三轴的磁场数据
// 参数: x - X轴数据输出引用
//       y - Y轴数据输出引用
//       z - Z轴数据输出引用
// 返回: 无
// 说明: 需要先调用read()读取数据
void QMC5883L::getRawData(int16_t &x, int16_t &y, int16_t &z) {
    x = raw_x;
    y = raw_y;
    z = raw_z;
}

// ================ 验证芯片ID ================
bool QMC5883L::verifyChipID() {
    uint8_t chip_id = 0;
    
    // 读取芯片ID寄存器
    if (!readRegister(QMC5883L_REG_CHIP_ID, &chip_id, 1)) {
        return false;
    }
    
    // QMC5883L的芯片ID应该是0xFF
    return (chip_id == 0xFF);
}

// ================ 检查数据是否准备好 ================
bool QMC5883L::isDataReady() {
    uint8_t status = 0;
    
    // 读取状态寄存器
    if (!readRegister(QMC5883L_REG_STATUS, &status, 1)) {
        return false;
    }
    
    // 检查DRDY标志位
    return (status & QMC5883L_STATUS_DRDY) != 0;
}

// ================ 获取方位角 ================
float QMC5883L::getAzimuth() {
    // 使用atan2计算方位角
    // atan2返回值范围: -π 到 π
    // 转换为 0-360度范围
    float azimuth = atan2f((float)calibrated_y, (float)calibrated_x) * 180.0f / 3.14159265f;
    
    // 加上磁偏角补偿
    azimuth += magnetic_declination;
    
    // 转换为0-360度范围
    if (azimuth < 0) {
        azimuth += 360.0f;
    } else if (azimuth >= 360.0f) {
        azimuth -= 360.0f;
    }
    
    return azimuth;
}

// ================ 设置磁偏角 ================
void QMC5883L::setMagneticDeclination(float degrees, uint8_t minutes) {
    // 将分数转换为度数 (1度 = 60分)
    magnetic_declination = degrees + (float)minutes / 60.0f;
}

// ================ 设置校准参数(最小最大值) ================
void QMC5883L::setCalibration(int16_t x_min, int16_t x_max,
                               int16_t y_min, int16_t y_max,
                               int16_t z_min, int16_t z_max) {
    // 计算偏移量(中心点)
    calibration_offset[0] = (float)(x_min + x_max) / 2.0f;
    calibration_offset[1] = (float)(y_min + y_max) / 2.0f;
    calibration_offset[2] = (float)(z_min + z_max) / 2.0f;
    
    // 计算缩放因子(使用最大范围作为参考)
    float x_range = (float)(x_max - x_min) / 2.0f;
    float y_range = (float)(y_max - y_min) / 2.0f;
    float z_range = (float)(z_max - z_min) / 2.0f;
    
    // 找到最大范围
    float max_range = x_range;
    if (y_range > max_range) max_range = y_range;
    if (z_range > max_range) max_range = z_range;
    
    // 计算缩放因子(使各轴范围相等)
    if (x_range > 0) calibration_scale[0] = max_range / x_range;
    if (y_range > 0) calibration_scale[1] = max_range / y_range;
    if (z_range > 0) calibration_scale[2] = max_range / z_range;
}

// ================ 设置校准偏移量 ================
void QMC5883L::setCalibrationOffsets(float x_offset, float y_offset, float z_offset) {
    calibration_offset[0] = x_offset;
    calibration_offset[1] = y_offset;
    calibration_offset[2] = z_offset;
}

// ================ 设置校准缩放因子 ================
void QMC5883L::setCalibrationScales(float x_scale, float y_scale, float z_scale) {
    calibration_scale[0] = x_scale;
    calibration_scale[1] = y_scale;
    calibration_scale[2] = z_scale;
}

// ================ 获取校准偏移量 ================
float QMC5883L::getCalibrationOffset(uint8_t index) {
    if (index < 3) {
        return calibration_offset[index];
    }
    return 0.0f;
}

// ================ 获取校准缩放因子 ================
float QMC5883L::getCalibrationScale(uint8_t index) {
    if (index < 3) {
        return calibration_scale[index];
    }
    return 1.0f;
}

// ================ 清除校准参数 ================
void QMC5883L::clearCalibration() {
    // 重置为默认值(无校准)
    calibration_offset[0] = 0.0f;
    calibration_offset[1] = 0.0f;
    calibration_offset[2] = 0.0f;
    calibration_scale[0] = 1.0f;
    calibration_scale[1] = 1.0f;
    calibration_scale[2] = 1.0f;
    
    // 重置磁偏角
    magnetic_declination = 0.0f;
}

// ================ 获取校准后的数据 ================
void QMC5883L::getCalibratedData(float &x, float &y, float &z) {
    x = calibrated_x;
    y = calibrated_y;
    z = calibrated_z;
}

// ================ 获取温度 ================
float QMC5883L::getTemperature() {
    uint8_t buffer[2];
    
    // 读取温度寄存器(LSB和MSB)
    if (!readRegister(QMC5883L_REG_TEMP_LSB, buffer, 2)) {
        return 0.0f;
    }
    
    // 组合LSB和MSB为16位有符号整数
    int16_t temp_raw = (int16_t)(buffer[0] | (buffer[1] << 8));
    
    // 转换为温度值
    // QMC5883L温度分辨率: 1/256 ℃ per LSB
    // 参考温度: 25℃ 对应 0x0000
    float temperature = 25.0f + (float)temp_raw / 256.0f;
    
    return temperature;
}

// ================ 应用校准参数 ================
void QMC5883L::applyCalibration() {
    // 应用偏移量和缩放因子
    calibrated_x = ((float)raw_x - calibration_offset[0]) * calibration_scale[0];
    calibrated_y = ((float)raw_y - calibration_offset[1]) * calibration_scale[1];
    calibrated_z = ((float)raw_z - calibration_offset[2]) * calibration_scale[2];
}
