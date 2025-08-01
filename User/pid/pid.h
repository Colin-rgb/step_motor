#ifndef __PID_H__
#define __PID_H__

typedef struct
{
    float kp;
    float ki;
    float kd;

    float integral;
    float last_error;
    float imax;
} PID_t;

extern PID_t pid_x;
extern PID_t pid_y;

void PID_Init(PID_t* pid, float kp, float ki, float kd, float imax);
float PID_Compute(PID_t* pid, float error);
void StepMotor_PID_Update(float err_x, float err_y, int dect_flag);

#endif
