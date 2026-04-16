#include "PID.h"    
#include <cmath>

// ============ 宏定义 ============


// ============ 构造函数 ============
// 带参数构造函数（使用宏定义的默认值）
PID::PID(float kp , float ki , float kd , int max_val , int min_val)
{
    _kp = kp;
    _ki = ki;
    _kd = kd;
    _last_err = 0;
    _prev_err = 0;
    _integral = 0;
    _last_derivative = 0;
    _MAX = max_val;
    _MIN = min_val;
    _ControlVelocity = 0;
    
    // 使用宏定义的参数
    _derivative_filter_coef = PID_DERIVATIVE_FILTER_COEF;
    _integral_max = PID_INTEGRAL_MAX;
    _integral_min = PID_INTEGRAL_MIN;
    _integral_separation_threshold = PID_INTEGRAL_SEPARATION_THRESHOLD;
}

/************
通用转向环或者角度环：输入目标位置和当前位置
*************/
// 函数：角度环(PID)
// 参数1：当前位置
// 参数2：目标位置
// 返回值：PWM值
int PID::Turn_Pid(int now_position, float tar_position)//当前脉冲，目标脉冲 ,角速度
{
    float Err = tar_position - now_position;//目标脉冲-现在脉冲=误差脉冲
    
    // 角度环绕处理（如果是角度控制，处理0-360度环绕）
    if(Err > 180) {Err -= 360;}
    else if(Err < -180) {Err += 360;} 
    
    // 积分分离：误差较大时停止积分，避免超调
    if(fabs(Err) < _integral_separation_threshold) {
        _integral += Err;
        // 积分限幅（使用独立的积分限制）
        if(_integral > _integral_max) _integral = _integral_max;
        else if(_integral < _integral_min) _integral = _integral_min;
    }
    
    // 微分项（带一阶低通滤波，减少噪声影响）
    float derivative = Err - _last_err;
    derivative = _derivative_filter_coef * derivative + (1.0f - _derivative_filter_coef) * _last_derivative;
    _last_derivative = derivative;
    
    // 计算PID输出
    float output = _kp * Err + _ki * _integral + _kd * derivative;
    
    // 输出限幅
    if(output > _MAX) {
        output = _MAX;
        // 抗积分饱和：输出饱和时，如果误差与积分同号，则停止积分累积
        if(Err * _integral > 0) {
            _integral -= Err; // 回退本次积分
        }
    }
    else if(output < _MIN) {
        output = _MIN;
        // 抗积分饱和
        if(Err * _integral > 0) {
            _integral -= Err; // 回退本次积分
        }
    }
    
    // 更新误差
    _last_err = Err;
    
	// 返回PWM
    return (int)output;
}

/************
增量式速度环
先加i消除误差，再加p消除静态误差
*************/
// 函数：速度环(增量式)
// 参数1：目标速度
// 参数2：当前速度
// 返回值：速度控制值
int PID::FeedbackControl(int TargetVelocity, int CurrentVelocity)
{
    
    float Bias = TargetVelocity - CurrentVelocity; //求速度偏差
    
    // 增量式PID计算: Δu(k) = Kp[e(k)-e(k-1)] + Ki*e(k) + Kd[e(k)-2e(k-1)+e(k-2)]
    // 比例项增量
    float p_increment = _kp * (Bias - _last_err);
    // 积分项
    float i_increment = _ki * Bias;
    // 微分项增量（二阶差分）
    float d_increment = _kd * (Bias - 2.0f * _last_err + _prev_err);
    
    float increment = p_increment + i_increment + d_increment;
    
    _ControlVelocity += increment;  
    
    // 输出限制
    if(_ControlVelocity > _MAX) _ControlVelocity = _MAX;
    else if(_ControlVelocity < _MIN) _ControlVelocity = _MIN;
    
    // 更新误差历史
    _prev_err = _last_err;  // 保存上上次误差
    _last_err = Bias;       // 保存上次误差

    return _ControlVelocity; //返回速度控制值
}

// 函数：位置环(PID)
// 参数1：目标位置
// 参数2：当前位置
// 返回值：速度控制值
int PID::PositionControl(int TargetPosition, int CurrentPosition)
{
    float Bias = TargetPosition - CurrentPosition; //求位置偏差
    
    // 积分分离：误差较大时停止积分，避免超调
    if(fabs(Bias) < _integral_separation_threshold) {
        _integral += Bias;
        // 积分限幅（使用独立的积分限制）
        if(_integral > _integral_max) _integral = _integral_max;
        else if(_integral < _integral_min) _integral = _integral_min;
    }
    
    // 微分项（带一阶低通滤波，减少噪声影响）
    float derivative = Bias - _last_err;
    derivative = _derivative_filter_coef * derivative + (1.0f - _derivative_filter_coef) * _last_derivative;
    _last_derivative = derivative;
    
    // 完整PID控制器：u(k) = Kp*e(k) + Ki*Σe(k) + Kd*[e(k)-e(k-1)]
    float output = _kp * Bias + _ki * _integral + _kd * derivative;
    
    // 输出限幅
    if(output > _MAX) {
        output = _MAX;
        // 抗积分饱和：输出饱和时，如果误差与积分同号，则停止积分累积
        if(Bias * _integral > 0) {
            _integral -= Bias; // 回退本次积分
        }
    }
    else if(output < _MIN) {
        output = _MIN;
        // 抗积分饱和
        if(Bias * _integral > 0) {
            _integral -= Bias; // 回退本次积分
        }
    }
    
    // 更新误差
    _last_err = Bias;
    
    return (int)output; //返回位置控制值
}

// ============ 辅助函数 ============

// 重置PID控制器状态
void PID::Reset()
{
    _last_err = 0;
    _prev_err = 0;
    _integral = 0;
    _last_derivative = 0;
    _ControlVelocity = 0;
}

// 设置积分限幅
void PID::SetIntegralLimit(float max_integral, float min_integral)
{
    _integral_max = max_integral;
    _integral_min = min_integral;
}

// 设置积分分离阈值
void PID::SetIntegralSeparation(float threshold)
{
    _integral_separation_threshold = threshold;
}