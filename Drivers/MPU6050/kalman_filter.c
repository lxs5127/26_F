#include "kalman_filter.h"

/**
 * @brief 初始化卡尔曼滤波器
 */
void KalmanFilter_Init(KalmanFilter_t *kf, float Q_angle, float Q_bias, float R_measure)
{
    kf->Q_angle = Q_angle;
    kf->Q_bias  = Q_bias;
    kf->R_measure = R_measure;

    kf->angle = 0.0f;
    kf->bias  = 0.0f;
    kf->rate  = 0.0f;

    /* 初始化协方差矩阵, 初始不确定性较大 */
    kf->P[0][0] = 1.0f;
    kf->P[0][1] = 0.0f;
    kf->P[1][0] = 0.0f;
    kf->P[1][1] = 1.0f;
}

/**
 * @brief 卡尔曼滤波更新一步
 *
 * 状态方程:
 *   angle_k = angle_{k-1} + (gyro - bias) * dt
 *   bias_k  = bias_{k-1}
 *
 * 观测方程:
 *   z = angle + v,  v ~ N(0, R)
 *
 * @param kf          滤波器实例
 * @param accel_angle 加速度计推算的角度 (deg)
 * @param gyro_rate   陀螺仪原始角速度 (deg/s)
 * @param dt          时间间隔 (s)
 * @return 滤波后的角度 (deg)
 */
float KalmanFilter_Update(KalmanFilter_t *kf, float accel_angle, float gyro_rate, float dt)
{
    /* ===== 预测步骤 ===== */
    /* 状态预测: 用去偏后的陀螺仪数据积分角度 */
    float rate_unbiased = gyro_rate - kf->bias;
    kf->angle += rate_unbiased * dt;

    /* 状态转移矩阵 F = [[1, -dt], [0, 1]] */
    /* 协方差预测: P = F * P * F^T + Q */

    /* 先计算 F * P */
    float P00_temp = kf->P[0][0] - dt * kf->P[1][0];
    float P01_temp = kf->P[0][1] - dt * kf->P[1][1];
    /* P[1][0], P[1][1] 不变 (F[1]=[0,1]) */

    /* 再乘以 F^T */
    kf->P[0][0] = P00_temp - dt * P01_temp + kf->Q_angle;
    kf->P[0][1] = P01_temp;
    kf->P[1][0] = kf->P[1][0] - dt * kf->P[1][1];
    kf->P[1][1] = kf->P[1][1] + kf->Q_bias;

    /* ===== 更新步骤 ===== */
    /* 新息 = 测量值 - 预测值 */
    float innovation = accel_angle - kf->angle;

    /* 新息协方差 S = H * P * H^T + R, H = [1, 0] */
    float S = kf->P[0][0] + kf->R_measure;

    /* 卡尔曼增益 K = P * H^T / S */
    float K0 = kf->P[0][0] / S;
    float K1 = kf->P[1][0] / S;

    /* 状态更新 */
    kf->angle += K0 * innovation;
    kf->bias  += K1 * innovation;

    /* 协方差更新: P = (I - K*H) * P */
    float P00 = kf->P[0][0];
    float P01 = kf->P[0][1];

    kf->P[0][0] -= K0 * P00;
    kf->P[0][1] -= K0 * P01;
    kf->P[1][0] -= K1 * P00;
    kf->P[1][1] -= K1 * P01;

    kf->rate = rate_unbiased;

    return kf->angle;
}
