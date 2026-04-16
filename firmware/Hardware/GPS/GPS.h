#ifndef GPS_H
#define GPS_H

#include "driver/uart.h"
#include "esp_log.h"

// GPS模块使用的UART端口号（使用UART1）
#define GPS_UART_NUM UART_NUM_1

// GPS模块的GPIO引脚定义（可根据实际硬件连接修改）
#define GPS_TX_PIN 17  // ESP32的TX引脚，连接到GPS模块的RX
#define GPS_RX_PIN 16  // ESP32的RX引脚，连接到GPS模块的TX

// UART缓冲区大小定义
#define GPS_UART_BUF_SIZE 1024  // 接收缓冲区大小

class GPS {
public:
    GPS();
    ~GPS();
    void init(int baudrate);  // 初始化串口，波特率作为参数传入
    void deinit();

    // 通用串口发送函数
    int sendByte(uint8_t byte);  // 发送单个字节，返回发送的字节数（成功返回1，失败返回-1）
    int sendString(const char* str);  // 发送字符串，返回发送的字节数（失败返回-1）
    int sendData(const uint8_t* data, size_t len);  // 发送数组，返回实际发送的字节数（失败返回-1）

    // 通用串口接收函数
    int readByte(uint8_t* byte, uint32_t timeout_ms);  // 接收单个字节，返回接收的字节数（成功返回1，超时返回0，失败返回-1）
    int readData(uint8_t* buffer, size_t len, uint32_t timeout_ms);  // 接收指定长度数据，返回实际接收的字节数（失败返回-1）
    int available();  // 查询接收缓冲区可读字节数，返回可读字节数（失败返回-1）

    void read();
    void write();
    void flush();
    void close();
private:
    uart_config_t uart_config; // UART配置
    uart_handle_t uart_handle; // UART句柄
    gps_data_t gps_data; // GPS数据
};

#endif