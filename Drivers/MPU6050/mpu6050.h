#include "kalman_filter.h"

void mpu6050_init(void);
int Read_MPU6050(void);

extern float pitch, roll, yaw;
extern float gz_dps;                     /* Z轴角速度 (deg/s), 供巡线融合 */
extern KalmanFilter_t kf_pitch, kf_roll;

/* 加速度计原始值 (g), 供管道平衡前馈使用
 * Read_MPU6050() 中每帧更新, 200Hz */
extern float g_mpu_accel_ax;             /* X轴加速度 (g) */
extern float g_mpu_accel_ay;             /* Y轴加速度 (g) */
extern float g_mpu_accel_az;             /* Z轴加速度 (g) */