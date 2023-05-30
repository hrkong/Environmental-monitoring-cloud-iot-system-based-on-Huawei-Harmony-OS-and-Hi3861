#include <hi_types_base.h>
#include <hi_i2c.h>
#include <hi_early_debug.h>
#include <hi_task.h>
#include <hi_time.h>
#include <iot_log.h>
#include <string.h>
#include <app_demo_multi_sample.h>
#include <hi_mux.h>
#include <hi_io.h>
#include <hi3861_platform.h>
#include <hi_gpio.h>

#define     BEEP_ON_OFF_DELAY_500MS             (500000)
#define     BEEP_CONTROL_TASK_DELAY_20MS             (20000)
#define     BEEP_CONTROL_TASK_SIZE    (1024*2)
#define     BEEP_CONTROL_TASK_PRIO     (25)

hi_u32 g_beep_control_id = 0;

hi_void beep_control(hi_void)
{
    hi_pwm_init(HI_PWM_PORT_PWM0);
    hi_pwm_set_clock(PWM_CLK_160M);
    hi_pwm_start(HI_PWM_PORT_PWM0, PWM_DUTY, PWM_FULL_DUTY);
    hi_udelay(BEEP_ON_OFF_DELAY_500MS);
    hi_pwm_start(HI_PWM_PORT_PWM0, PWM_LOW_DUTY, PWM_FULL_DUTY);
    hi_udelay(BEEP_ON_OFF_DELAY_500MS);
}
hi_void *beep_control_demo(hi_void *param)
{
    hi_u32 ret;
    hi_unuse_param(param);
    while(1)
    {
        beep_control();
        hi_sleep(BEEP_CONTROL_TASK_DELAY_20MS);
    }
    ret=hi_task_delete(g_beep_control_id);
    if(ret!=HI_ERR_SUCCESS){
        printf("Failed to delete beep demo task\r\n");
        return HI_ERR_FAILURE;
    }
    return HI_ERR_SUCCESS;
}
hi_u32 app_demo_beep_control_task(hi_void)
{
    hi_u32 ret;
    hi_task_attr beep_control_attr={0};

    beep_control_attr.stack_size=BEEP_CONTROL_TASK_SIZE;
    beep_control_attr.task_prio=BEEP_CONTROL_TASK_PRIO;
    beep_control_attr.task_name=(hi_char*) "beep control demo";

    ret=hi_task_create(&g_beep_control_id,&beep_control_attr,beep_control_demo,HI_NULL);
    if(ret!=HI_ERR_SUCCESS){
        printf("Failed to create beep demo task\r\n");
        return HI_ERR_FAILURE;
    }
    return HI_ERR_SUCCESS;
}