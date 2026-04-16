#include "GPS.h"

// 定义日志标签，用于ESP32的日志系统输出调试信息
static const char *TAG = "GPS";

// GPS类的构造函数
// 功能：创建GPS对象时被调用，目前不需要做特殊初始化
GPS::GPS()
{

}

// GPS类的析构函数
// 功能：销毁GPS对象时被调用，确保释放串口资源
GPS::~GPS()
{
    deinit();  // 调用反初始化函数，释放UART资源
}

// 初始化GPS模块的串口通信
// 参数：baudrate - 波特率，常见值如9600、38400、115200等
// 功能：配置并启动UART1串口，用于与GPS模块通信
void GPS::init(int baudrate)
{
    // 第一步：配置UART的基本参数
    uart_config.baud_rate = baudrate;              // 设置波特率（由用户传入）
    uart_config.data_bits = UART_DATA_8_BITS;      // 数据位：8位（GPS模块标准配置）
    uart_config.parity = UART_PARITY_DISABLE;      // 校验位：无校验（GPS模块通常不需要校验）
    uart_config.stop_bits = UART_STOP_BITS_1;      // 停止位：1位（标准配置）
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;  // 流控：禁用硬件流控（GPS模块一般不需要）
    uart_config.source_clk = UART_SCLK_DEFAULT;    // 时钟源：使用默认时钟源

    // 第二步：安装UART驱动程序
    // 参数说明：
    // - GPS_UART_NUM: 使用的UART端口号（这里是UART1）
    // - &uart_config: UART配置参数的指针
    // - GPS_UART_BUF_SIZE: 接收缓冲区大小（1024字节）
    // - GPS_UART_BUF_SIZE: 发送缓冲区大小（1024字节）
    // - 0: 事件队列大小（0表示不使用事件队列）
    // - NULL: 事件队列句柄（不使用所以传NULL）
    // - 0: 中断分配标志（使用默认值）
    esp_err_t ret = uart_driver_install(GPS_UART_NUM, GPS_UART_BUF_SIZE, GPS_UART_BUF_SIZE, 0, NULL, 0);
    if (ret != ESP_OK) {
        // 如果安装驱动失败，输出错误日志
        ESP_LOGE(TAG, "UART驱动安装失败，错误码: %d", ret);
        return;
    }

    // 第三步：应用UART配置参数
    ret = uart_param_config(GPS_UART_NUM, &uart_config);
    if (ret != ESP_OK) {
        // 如果配置失败，输出错误日志并卸载驱动
        ESP_LOGE(TAG, "UART参数配置失败，错误码: %d", ret);
        uart_driver_delete(GPS_UART_NUM);
        return;
    }

    // 第四步：设置UART的GPIO引脚
    // 参数说明：
    // - GPS_UART_NUM: UART端口号
    // - GPS_TX_PIN: ESP32的TX引脚（连接到GPS模块的RX）
    // - GPS_RX_PIN: ESP32的RX引脚（连接到GPS模块的TX）
    // - UART_PIN_NO_CHANGE: 不使用RTS引脚
    // - UART_PIN_NO_CHANGE: 不使用CTS引脚
    ret = uart_set_pin(GPS_UART_NUM, GPS_TX_PIN, GPS_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        // 如果引脚配置失败，输出错误日志并卸载驱动
        ESP_LOGE(TAG, "UART引脚配置失败，错误码: %d", ret);
        uart_driver_delete(GPS_UART_NUM);
        return;
    }

    // 初始化成功，输出日志信息
    ESP_LOGI(TAG, "GPS串口初始化成功 - 端口: UART%d, 波特率: %d, TX: GPIO%d, RX: GPIO%d",
             GPS_UART_NUM, baudrate, GPS_TX_PIN, GPS_RX_PIN);
}

// 反初始化GPS模块的串口通信
// 功能：释放UART资源，停止串口通信
void GPS::deinit()
{
    // 删除UART驱动，释放相关资源
    // 这会停止串口通信并释放内存
    esp_err_t ret = uart_driver_delete(GPS_UART_NUM);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "GPS串口资源已释放");
    } else {
        ESP_LOGE(TAG, "GPS串口资源释放失败，错误码: %d", ret);
    }
}

// ==================== 通用串口发送函数 ====================

// 发送单个字节
// 参数：byte - 要发送的字节数据
// 返回值：成功返回1（发送了1个字节），失败返回-1
// 功能：通过串口发送一个字节的数据到GPS模块
int GPS::sendByte(uint8_t byte)
{
    // 使用uart_write_bytes函数发送数据
    // 参数说明：
    // - GPS_UART_NUM: UART端口号
    // - &byte: 要发送的数据的地址
    // - 1: 发送的字节数（这里是1个字节）
    int len = uart_write_bytes(GPS_UART_NUM, &byte, 1);

    // 检查发送结果
    if (len < 0) {
        // 发送失败，输出错误日志
        ESP_LOGE(TAG, "发送字节失败");
        return -1;
    }

    // 返回实际发送的字节数
    return len;
}

// 发送字符串
// 参数：str - 要发送的字符串（以'\0'结尾的C字符串）
// 返回值：成功返回发送的字节数，失败返回-1
// 功能：通过串口发送一个字符串到GPS模块（不包含结尾的'\0'）
int GPS::sendString(const char* str)
{
    // 检查字符串指针是否有效
    if (str == NULL) {
        ESP_LOGE(TAG, "字符串指针为空");
        return -1;
    }

    // 计算字符串长度（不包含'\0'）
    size_t len = strlen(str);

    // 使用uart_write_bytes函数发送字符串
    // 参数说明：
    // - GPS_UART_NUM: UART端口号
    // - str: 字符串的地址
    // - len: 字符串的长度
    int sent = uart_write_bytes(GPS_UART_NUM, str, len);

    // 检查发送结果
    if (sent < 0) {
        // 发送失败，输出错误日志
        ESP_LOGE(TAG, "发送字符串失败");
        return -1;
    }

    // 返回实际发送的字节数
    return sent;
}

// 发送数据数组
// 参数：data - 要发送的数据数组的指针
//       len - 要发送的数据长度（字节数）
// 返回值：成功返回实际发送的字节数，失败返回-1
// 功能：通过串口发送指定长度的数据数组到GPS模块
int GPS::sendData(const uint8_t* data, size_t len)
{
    // 检查数据指针是否有效
    if (data == NULL) {
        ESP_LOGE(TAG, "数据指针为空");
        return -1;
    }

    // 检查数据长度是否有效
    if (len == 0) {
        ESP_LOGW(TAG, "数据长度为0，无需发送");
        return 0;
    }

    // 使用uart_write_bytes函数发送数据
    // 参数说明：
    // - GPS_UART_NUM: UART端口号
    // - data: 数据数组的地址
    // - len: 数据的长度
    int sent = uart_write_bytes(GPS_UART_NUM, data, len);

    // 检查发送结果
    if (sent < 0) {
        // 发送失败，输出错误日志
        ESP_LOGE(TAG, "发送数据失败");
        return -1;
    }

    // 返回实际发送的字节数
    return sent;
}

// ==================== 通用串口接收函数 ====================

// 接收单个字节
// 参数：byte - 用于存储接收到的字节的指针
//       timeout_ms - 超时时间（毫秒），0表示不等待，立即返回
// 返回值：成功返回1（接收到1个字节），超时返回0，失败返回-1
// 功能：从串口接收一个字节的数据
int GPS::readByte(uint8_t* byte, uint32_t timeout_ms)
{
    // 检查指针是否有效
    if (byte == NULL) {
        ESP_LOGE(TAG, "字节指针为空");
        return -1;
    }

    // 使用uart_read_bytes函数接收数据
    // 参数说明：
    // - GPS_UART_NUM: UART端口号
    // - byte: 存储接收数据的缓冲区地址
    // - 1: 要接收的字节数（这里是1个字节）
    // - timeout_ms / portTICK_PERIOD_MS: 超时时间（转换为系统时钟周期数）
    int len = uart_read_bytes(GPS_UART_NUM, byte, 1, timeout_ms / portTICK_PERIOD_MS);

    // 检查接收结果
    if (len < 0) {
        // 接收失败，输出错误日志
        ESP_LOGE(TAG, "接收字节失败");
        return -1;
    }

    // 返回实际接收的字节数（0表示超时，1表示成功）
    return len;
}

// 接收指定长度的数据
// 参数：buffer - 用于存储接收数据的缓冲区指针
//       len - 要接收的数据长度（字节数）
//       timeout_ms - 超时时间（毫秒），0表示不等待，立即返回
// 返回值：成功返回实际接收的字节数，失败返回-1
// 功能：从串口接收指定长度的数据，如果超时则返回已接收的字节数
int GPS::readData(uint8_t* buffer, size_t len, uint32_t timeout_ms)
{
    // 检查缓冲区指针是否有效
    if (buffer == NULL) {
        ESP_LOGE(TAG, "缓冲区指针为空");
        return -1;
    }

    // 检查长度是否有效
    if (len == 0) {
        ESP_LOGW(TAG, "接收长度为0");
        return 0;
    }

    // 使用uart_read_bytes函数接收数据
    // 参数说明：
    // - GPS_UART_NUM: UART端口号
    // - buffer: 存储接收数据的缓冲区地址
    // - len: 要接收的字节数
    // - timeout_ms / portTICK_PERIOD_MS: 超时时间（转换为系统时钟周期数）
    int received = uart_read_bytes(GPS_UART_NUM, buffer, len, timeout_ms / portTICK_PERIOD_MS);

    // 检查接收结果
    if (received < 0) {
        // 接收失败，输出错误日志
        ESP_LOGE(TAG, "接收数据失败");
        return -1;
    }

    // 返回实际接收的字节数
    return received;
}

// 查询接收缓冲区可读字节数
// 返回值：返回接收缓冲区中可读的字节数，失败返回-1
// 功能：查询串口接收缓冲区中有多少字节的数据可以读取
// 用途：在调用readByte或readData之前，可以先用这个函数检查是否有数据可读
int GPS::available()
{
    size_t available_bytes = 0;

    // 使用uart_get_buffered_data_len函数查询缓冲区数据长度
    // 参数说明：
    // - GPS_UART_NUM: UART端口号
    // - &available_bytes: 用于存储可读字节数的变量地址
    esp_err_t ret = uart_get_buffered_data_len(GPS_UART_NUM, &available_bytes);

    // 检查查询结果
    if (ret != ESP_OK) {
        // 查询失败，输出错误日志
        ESP_LOGE(TAG, "查询缓冲区数据长度失败，错误码: %d", ret);
        return -1;
    }

    // 返回可读字节数
    return (int)available_bytes;
}