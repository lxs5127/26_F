#ifndef _KALMAN_FILTER_H_
#define _KALMAN_FILTER_H_

/**
 * @brief 一维卡尔曼滤波器结构体
 * 状态向量: [angle, gyro_bias]^T
 * 预测: 陀螺仪积分
 * 观测: 加速度计推算角度
 */
typedef struct {
    float angle;       // 滤波后的角度 (deg)
    float bias;        // 陀螺仪零偏 (deg/s)
    float rate;        // 去偏后的角速度 (deg/s)

    float P[2][2];     // 误差协方差矩阵

    /* 可调参数 */
    float Q_angle;     // 角度过程噪声
    float Q_bias;      // 零偏过程噪声
    float R_measure;   // 测量噪声
} KalmanFilter_t;

/**
 * @brief 初始化卡尔曼滤波器
 * @param kf    滤波器实例
 * @param Q_angle   角度过程噪声 (推荐 0.001)
 * @param Q_bias    零偏过程噪声 (推荐 0.003)
 * @param R_measure 测量噪声     (推荐 0.03 ~ 0.1)
 */
void KalmanFilter_Init(KalmanFilter_t *kf, float Q_angle, float Q_bias, float R_measure);

/**
 * @brief 卡尔曼滤波更新一步
 * @param kf          滤波器实例
 * @param accel_angle 加速度计推算的角度 (deg)
 * @param gyro_rate   陀螺仪角速度 (deg/s)
 * @param dt          时间间隔 (s)
 * @return 滤波后的角度 (deg)
 */
float KalmanFilter_Update(KalmanFilter_t *kf, float accel_angle, float gyro_rate, float dt);

#endif /* _KALMAN_FILTER_H_ */
