#include <hi_types_base.h>
#include <hi_i2c.h>
#include <app_demo_sgp30.h>
#include <hi_early_debug.h>
#include <hi_task.h>
#include <hi_time.h>
#include <string.h>
#include <app_demo_multi_sample.h>
#include <hi_mux.h>
#include <hi_io.h>

#define  SGP_REG_ARRAY_LEN              (6)     //这里为6是因为CO2和TVOC各有高低八位，还有两个ACK包，分布为CO2高、CO2低、ACK,TVOC高、TVOC低、ACK
#define  SGP_OC_ARRAY_LEN               (6)
#define  SGP_SNED_CMD_LEN               (2)     //传输的指令有3个部分

hi_u8 g_sgp30_tvoc_buff[6] = {0};
hi_u8 g_sgp30_co2_buff[6] = {0};
hi_s32 sgp_tvoc =0;
hi_s32 sgp_co2  =0;
extern hi_u8 g_menu_select;
extern hi_u8 g_menu_mode ;
extern hi_u32 g_mux_id;
extern hi_u8  *flaot_to_string(hi_double d, hi_u8 *str );

extern hi_u32 hi_io_set_func(hi_io_name id, hi_u8 val);
extern hi_u8  flaot1_to_string(hi_double d, hi_u8 *str );

hi_u32 sgp30_check_and_init()
{
    hi_u32 status =0;
    hi_i2c_idx id = HI_I2C_IDX_0;

    hi_i2c_data sgp30_i2c_huanxing ={0};
    hi_i2c_data sgp30_i2c_init_air_quality={0};
    // hi_i2c_data sgp30_i2c_measure_air_quality={0};

    hi_u8 huanxing[SGP_SNED_CMD_LEN]={0x36,0x82};        //SPG30的地址为0x58。0x36,0x82为唤醒SPG30模块命令。
    hi_u8 init_air_quality[SGP_SNED_CMD_LEN]={0x20,0x03};
    // hi_u8 measure_air_quality={0x58<<1,0x20,0x08};

    sgp30_i2c_huanxing.send_buf=huanxing;
    sgp30_i2c_huanxing.send_len=SGP_SNED_CMD_LEN;
    sgp30_i2c_init_air_quality.send_buf=init_air_quality;
    sgp30_i2c_init_air_quality.send_len=SGP_SNED_CMD_LEN;
    // sgp30_i2c_measure_air_quality.send_buf=measure_air_quality;
    // sgp30_i2c_measure_air_quality.send_len=SGP_SNED_CMD_LEN;

    status=hi_i2c_write(id,(SGP_DEVICE_ADDR<<1)|0x00,&sgp30_i2c_huanxing);      //唤醒SGP30

    hi_udelay(SGP_DELAY_10MS);
    status=hi_i2c_write(id,(SGP_DEVICE_ADDR<<1)|0x00,&sgp30_i2c_init_air_quality);  //初始化空气质量检测
    
    // status=hi_i2c_write(id,(SGP_DEVICE_ADDR<<1)|0x00,&sgp30_i2c_measure_air_quality); //空气质量检测
    return HI_ERR_SUCCESS;
}

hi_void sgp30_measure_test(void)
{
    hi_u32 status=0;
    hi_i2c_idx id = 0;
    hi_i2c_data sgp30_test_cmd ={0};
    hi_u8 _send_test_cmd[2] = {0x20,0x32};
    hi_u8 result[3]={0};
    hi_u32 recv_data[3] = { 0 };
    hi_i2c_data sgp30_i2c_write_cmd_addr ={0};
    hi_i2c_data sgp30_i2c_data = { 0 };

    sgp30_i2c_write_cmd_addr.send_buf = _send_test_cmd;
    sgp30_i2c_write_cmd_addr.send_len = 2;

    memset(&recv_data, 0x0, sizeof(recv_data));
    memset(&sgp30_i2c_data, 0x0, sizeof(hi_i2c_data));

    sgp30_i2c_data.receive_buf = recv_data;
    sgp30_i2c_data.receive_len = 3;


    status = hi_i2c_write(id, 0xB0, &sgp30_i2c_write_cmd_addr);

    hi_udelay(SGP_DELAY_500MS);

    status = hi_i2c_read(id,0xB1,&sgp30_i2c_data);
    // hi_i2c_writeread(id,0x58,&sgp30_test_cmd);
    if(recv_data[1]==NULL)
    {
        oled_show_str(0,6, "      yes       ",1);
    }
    else
    {
        oled_show_str(0,6,recv_data[1],1);
    }
}

hi_u32 sgp30_write(hi_u8 high_8,hi_u8 low_8)
{
    hi_u32 status =0;
    hi_i2c_idx id = 0;
    hi_i2c_data sgp30_i2c_write_cmd_addr ={0};
    hi_u8 _send_user_cmd[3] = {high_8, low_8};

    sgp30_i2c_write_cmd_addr.send_buf = _send_user_cmd;
    sgp30_i2c_write_cmd_addr.send_len = 2;

    status = hi_i2c_write(id, (SGP_DEVICE_ADDR<<1)|0x00, &sgp30_i2c_write_cmd_addr);        //这里的SGP_DEVICE_ADDR是I2C从机地址0x58,(SGP_DEVICE_ADDR<<1)|0x00=0xB0

    return HI_ERR_SUCCESS;
}


hi_u32 sgp30_read(hi_u32 recv_len, hi_u8 type)
{
    hi_u32 status=0;
    hi_i2c_idx id  = HI_I2C_IDX_0;
    hi_u8 recv_data[6] = { 0 };
    hi_i2c_data sgp30_i2c_data = { 0 };
    hi_s32 tvoc =0;
    hi_s32 co2 =0;
    //hi_u8 send[1]={0xB1};

    /* Request memory space */
    memset(&recv_data, 0x0, sizeof(recv_data));
    memset(&sgp30_i2c_data, 0x0, sizeof(hi_i2c_data));

    sgp30_i2c_data.receive_buf = recv_data;
    sgp30_i2c_data.receive_len = recv_len;

    hi_i2c_data sgp30_i2c_write_cmd_addr ={0};
    hi_u8 _send_user_cmd[2] = {SGP_DEVICE_PARAM_HIGH_BYTE, SGP_DEVICE_PARAM_LOW_BYTE};

    sgp30_i2c_write_cmd_addr.send_buf = _send_user_cmd;
    sgp30_i2c_write_cmd_addr.send_len = 2;

    status = hi_i2c_write(id, (SGP_DEVICE_ADDR<<1)|0x00, &sgp30_i2c_write_cmd_addr);        //这里的SGP_DEVICE_ADDR是I2C从机地址0x58,(SGP_DEVICE_ADDR<<1)|0x00=0xB0


    hi_udelay(SGP_DELAY_500MS);//500ms等待测量完成
    
    status = hi_i2c_read(id, 0xB1, &sgp30_i2c_data);       //(SGP_DEVICE_ADDR<<1)|0x01=0xB1，读完的存在sgp30_i2c_data里面了
    

    if(recv_data[0]==NULL)
    {
        oled_show_str(0,3, "      ERROR     ",1);
    }
 

    if(type==SGP_TVOC)
    {
        tvoc=(hi_float)((recv_data[3]<<8|recv_data[4]));
        sgp_tvoc=tvoc;
        flaot1_to_string(sgp_tvoc,g_sgp30_tvoc_buff);
        return HI_ERR_SUCCESS;
    }

    if(type==SGP_CO2)
    {
        co2=(hi_float)((recv_data[0]<<8|recv_data[1]));
        sgp_co2=co2;
        flaot1_to_string(sgp_co2,g_sgp30_co2_buff);
        //oled_show_str(55,5,g_sgp30_co2_buff,1);
        return HI_ERR_SUCCESS;
    }
}


hi_void get_sgp30_data(hi_void)
{
    hi_i2c_idx id =0;
    hi_u32 status =0;


    hi_i2c_init(id, BAUDRATE_INIT); /* baudrate: 400000 */
    hi_i2c_set_baudrate(id, BAUDRATE_INIT);

    //hi_udelay(SGP_DELAY_1S);  //初始化需要时间间隔

    //status = sgp30_write(SGP_DEVICE_PARAM_HIGH_BYTE, SGP_DEVICE_PARAM_LOW_BYTE);//temperature
    //status = sgp30_write(SGP_DEVICE_PARAM_HIGH_BYTE, SGP_DEVICE_PARAM_LOW_BYTE);//temperature
    
    status = sgp30_read(SGP_REG_ARRAY_LEN, SGP_CO2);
    status = sgp30_read(SGP_REG_ARRAY_LEN, SGP_TVOC);
}