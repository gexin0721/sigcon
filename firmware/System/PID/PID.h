#ifndef PID_H
#define PID_H

// ============ PID参数宏定义 ============
// 输出限幅（电机PWM范围）
#define PID_OUTPUT_MAX      255
#define PID_OUTPUT_MIN      -255

// 默认PID系数（可根据实际调试修改）
#define PID_DEFAULT_KP      2.0f
#define PID_DEFAULT_KI      0.1f
#define PID_DEFAULT_KD      0.5f

// 积分限幅（默认为输出限幅的80%）
#define PID_INTEGRAL_MAX    (PID_OUTPUT_MAX * 0.8f)
#define PID_INTEGRAL_MIN    (PID_OUTPUT_MIN * 0.8f)

// 积分分离阈值（误差大于此值时停止积分）
#define PID_INTEGRAL_SEPARATION_THRESHOLD   ((PID_OUTPUT_MAX - PID_OUTPUT_MIN) * 0.3f)

// 微分滤波系数（0-1，越小滤波越强，建议0.6-0.8）
#define PID_DERIVATIVE_FILTER_COEF  0.7f

// PID控制器
class PID
{
private:

    float _last_err ;      // 上次误差 e(k-1)
    float _prev_err ;      // 上上次误差 e(k-2)，用于增量式PID
    float _integral ;      // 积分项累积
    float _last_derivative; // 上次微分项，用于微分滤波
    int _MAX ;             // 最大值
    int _MIN ;             // 最小值
    int _ControlVelocity ; // 控制速度
    float _integral_max ;  // 积分上限
    float _integral_min ;  // 积分下限
    float _integral_separation_threshold; // 积分分离阈值

public:
    // 外部可修改 PID参数
    float _kp ;       // 比例系数
    float _ki ;       // 积分系数
    float _kd ;       // 微分系数
    float _derivative_filter_coef; // 微分滤波系数 (0-1, 越小滤波越强)
    
    // 构造函数：使用宏定义的默认值
    PID(float kp = PID_DEFAULT_KP, 
        float ki = PID_DEFAULT_KI, 
        float kd = PID_DEFAULT_KD, 
        int max_val = PID_OUTPUT_MAX, 
        int min_val = PID_OUTPUT_MIN);
    
    // 角度环(PID)
    int Turn_Pid(int now_position, float tar_position);
    // 速度环(增量式)
    int FeedbackControl(int TargetVelocity, int CurrentVelocity);
    // 位置环(PID)
    int PositionControl(int TargetPosition, int CurrentPosition);
    // 重置PID状态
    void Reset();
    // 设置积分限幅
    void SetIntegralLimit(float max_integral, float min_integral);
    // 设置积分分离阈值
    void SetIntegralSeparation(float threshold);

};


#endif