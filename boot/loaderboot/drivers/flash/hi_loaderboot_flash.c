/*
 * Copyright (c) 2020 HiSilicon (Shanghai) Technologies CO., LIMITED.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "hi_loaderboot_flash.h"

#include <adc_drv.h>

hi_spi_flash_ctrl g_flash_drv_ctrl = {
    0,
};

hi_u32 g_dma_buffer[FLASH_DMA_BUF_LEN] = {0};
hi_u8 g_back_buffer[SZ_4KB] = { 0 };

#define HI_FLASH_SUPPORT_REF_VBAT
#define HI_FLASH_VOLTAGE_TH0 280
#define HI_FLASH_VOLTAGE_TH1 310
#define PLL2DBB_192M_MASK    0x3
#define CMU_CLK_SEL_96M      96
#define CMU_CLK_SEL_80M      80
#define CMU_CLK_SEL_48M      48
/* wb flash reg1 need check: srp sec tb bp2 bp1 bp0 , high 6 bits */
#define WB_REG1_CHECK_MSK 0xFC
/* wb flash reg2 need check: cmp lb3 lb2 lb1 qe srl */
#define WB_REG2_CHECK_MSK 0x7B
#define FIRST_DELAY_US  10000
#define NEXT_DELAY_US   1000000
#define WB_REG3_HOLD_OFFSET 7
#define WB_REG3_WPS_OFFSET 2
#define WB_REG2_SRL_OFFSET 0
#define WB_REG1_SRP_OFFSET 7
#define CHECK_PROTECT_REG_SIZE 2
#define MAX_CHECK_REG_TIMES 3
/* wb flash reg3 need check: hold drv1 drv0 s19 wps */
#define WB_CHECK_REG3_MSK 0xEC
#define FLASH_PROTECT_NEED_CHK  0x55
#define FLASH_PROTECT_NOT_CHK   0xAA

hi_u8 g_flash_reg_need_check = FLASH_PROTECT_NOT_CHK;

#ifdef HI_FLASH_SUPPORT_REF_VBAT
typedef struct {
    hi_u8 chip_id[HI_FLASH_CHIP_ID_NUM];
    hi_u8 freq_high : 2;
    hi_u8 freq_midle : 2;
    hi_u8 freq_low : 2;
    hi_u8 voltage : 2;
} flash_vlt_sfc_info;

flash_vlt_sfc_info g_flash_vlt_sfc_info_tbl[HI_FLASH_DEFAULT_TYPE_NUM] = {
    /* 0-->96Mhz 1-->80Mhz 2-->60Mhz 3-->48Mhz */
    {{0,              }, 0x1, 0x1, 0x3, 0x1},
    {{0xef, 0x60, 0x15}, 0x1, 0x1, 0x1, 0x0}, /*  w25q16jw 1.8v */
    /* {{0xef, 0x40, 0x15}, 0x1d}, 00,01,11,01}, w25q16jl 3.3v */
    {{0xef, 0x40, 0x15}, 0x0, 0x1, 0x1, 0x1}, /*  w25q16jl 3.3v */
    {{0xc8, 0x60, 0x15}, 0x1, 0x1, 0x1, 0x0}, /*  gd25le16 1.8v */
    {{0xc8, 0x65, 0x15}, 0x0, 0x1, 0x1, 0x1}, /*  gd25wq16 1.65~3.6v use 2.3~3.6v */
    {{0x1c, 0x38, 0x15}, 0x1, 0x1, 0x1, 0x0}, /*  en25s16 1.8v */
    {{0x1C, 0x70, 0x15}, 0x0, 0x1, 0x1, 0x1}, /*  en25qh16 3.3v */
    {{0x85, 0x60, 0x15}, 0x0, 0x1, 0x1, 0x1}, /*  p25q16 1.65~3.6v use 2.3~3.6V */
};

flash_vlt_sfc_info g_flash_vlt_sfc_info = {
    0,
};
hi_u16 g_voltage = 0;

hi_void sfc_config_set_experience(const hi_u8 *chip_id, flash_vlt_sfc_info *flash_info,
    flash_vlt_sfc_info *flash_info_tbl, hi_u8 tbl_size)
{
    flash_vlt_sfc_info *info = HI_NULL;
    hi_u8 i;
    hi_u8 cur_chip_idx = 0xFF;  /* 0xFF : invalid */
    for (i = 1; i < tbl_size; i++) {
        info = &flash_info_tbl[i];
        if (memcmp(info->chip_id, chip_id, HI_FLASH_CHIP_ID_NUM) == HI_ERR_SUCCESS) {
            cur_chip_idx = i;
        }
    }
    info = (cur_chip_idx == 0xFF) ? &flash_info_tbl[0] : &flash_info_tbl[cur_chip_idx];
    hi_u32 cs = (uintptr_t)flash_info ^ sizeof(flash_vlt_sfc_info) ^ (uintptr_t)info ^ sizeof(flash_vlt_sfc_info);
    if (memcpy_s(flash_info, sizeof(flash_vlt_sfc_info), info, sizeof(flash_vlt_sfc_info), cs) != EOK) {
        return;
    }
}

hi_void sfc_config_update_freq(hi_u32 addr)
{
    hi_u32 ret;
    hi_u16 val;
    hi_u16 reg_val;
    flash_vlt_sfc_info *flash_info = (flash_vlt_sfc_info *)(uintptr_t)addr;
    if (flash_info == HI_NULL) {
        return;
    }
    hi_reg_read16(PMU_CMU_CTL_CMU_CLK_SEL_REG, reg_val);
    reg_val &= ~(PLL2DBB_192M_MASK << 8); /* 8 */
    if (flash_info->voltage == 0) { /* 1.8V flash */
        val = flash_info->freq_high;
        reg_val |= val << 8;  /* 8 */
        hi_reg_write16(PMU_CMU_CTL_CMU_CLK_SEL_REG, reg_val);
        /* Drive Capability Configuration.CNcomment:驱动能力配置 */
        return;
    }
    ret = get_average_ref_vlt(&g_voltage);
    if (ret != HI_ERR_SUCCESS) {
        return;
    }
    /* 2.3V ~ 3.6V, Selecting the SFC frequency */
    if (g_voltage > HI_FLASH_VOLTAGE_TH1) { /* 297*0.01V */
        val = flash_info->freq_high;
    } else if (g_voltage > HI_FLASH_VOLTAGE_TH0) { /* 270*0.01V */
        val = flash_info->freq_midle;
    } else {
        val = flash_info->freq_low;
    }
    reg_val |= val << 8; /* 8 */
    hi_reg_write16(PMU_CMU_CTL_CMU_CLK_SEL_REG, reg_val);
}

hi_void sfc_config_cmu_clk_sel(hi_u8 clk)
{
    if (clk == CMU_CLK_SEL_96M) {
        hi_reg_clrbits(PMU_CMU_CTL_CMU_CLK_SEL_REG, 8, 2);   /* 8, 2 96M */
    } else if (clk == CMU_CLK_SEL_80M) {
        hi_reg_clrbits(PMU_CMU_CTL_CMU_CLK_SEL_REG, 8, 2); /* set 8 left shift 2 */
        hi_reg_setbit(PMU_CMU_CTL_CMU_CLK_SEL_REG, 8);  /*   80M */
    } else if (clk == CMU_CLK_SEL_48M) {
        hi_reg_clrbits(PMU_CMU_CTL_CMU_CLK_SEL_REG, 8, 2); /* set 8 left shift 2 */
        hi_reg_setbit(PMU_CMU_CTL_CMU_CLK_SEL_REG, 8);  /*  48M */
        hi_reg_setbit(PMU_CMU_CTL_CMU_CLK_SEL_REG, 9); /* 9 */
    }
}
#endif
hi_void flash_wb_repair_reg3(const hi_u8 *chip_id);

hi_u32 spi_flash_write_sr_reg(hi_u8 cmd, hi_u8* data, hi_u8 data_len, hi_bool is_volatile)
{
    hi_u32 temp_data = 0;
    hi_u32 ret = HI_ERR_SUCCESS;
    if (data_len > 0) {
        hi_u32 check_sum = (uintptr_t)(&temp_data) ^ (hi_u32)sizeof(temp_data) ^
            (uintptr_t)data ^ data_len;
        if (memcpy_s(&temp_data, sizeof(temp_data), data, data_len, check_sum) != EOK) {
            return HI_ERR_FAILURE;
        }
    }
    if (is_volatile) {
        hisfc_write(SFC_REG_CMD_INS, SPI_CMD_VSR_WREN);
        hisfc_write(SFC_REG_CMD_CONFIG, (hi_u32)(SFC_CMD_CONFIG_SEL_CS | SFC_CMD_CONFIG_START));
        spif_wait_config_start();
    } else {
        ret = spif_write_enable(HI_TRUE);
        if (ret != HI_ERR_SUCCESS) {
            return ret;
        }
    }
    hisfc_write(SFC_REG_CMD_INS, cmd);
    hisfc_write(SFC_REG_CMD_DATABUF1, temp_data);

    hisfc_write(SFC_REG_CMD_CONFIG,
                SFC_CMD_CONFIG_SEL_CS
                | SFC_CMD_CONFIG_DATA_EN
                | sfc_cmd_config_data_cnt(data_len)
                | SFC_CMD_CONFIG_START);

    spif_wait_config_start();
    return ret;
}

hi_u32 flash_protect_read_check_reg(hi_u8 *reg_value, hi_u8 reg_size)
{
    hi_u8 first_rval[CHECK_PROTECT_REG_SIZE] = {0};
    hi_u8 second_rval[CHECK_PROTECT_REG_SIZE] = {0};
    hi_u32 ret;
    if ((reg_value == NULL) || (reg_size < CHECK_PROTECT_REG_SIZE)) {
        return HI_ERR_FLASH_INVALID_PARAMETER;
    }
    ret = spif_wait_ready(HI_TRUE, SPI_CMD_SR_WIPN, SPI_SR_BIT_WIP); /* wait until flash is idle */
    if (ret != HI_ERR_SUCCESS) {
        boot_msg1("spif_wait_ready ret:", ret);
        return ret;
    }
    ret = spi_flash_read_reg(SPI_CMD_RDSR, &first_rval[0], 1);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    ret = spi_flash_read_reg(SPI_CMD_RDSR2, &first_rval[1], 1);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    ret = spi_flash_read_reg(SPI_CMD_RDSR, &second_rval[0], 1);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    ret = spi_flash_read_reg(SPI_CMD_RDSR2, &second_rval[1], 1);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    if ((first_rval[0] & WB_REG1_CHECK_MSK) != (second_rval[0] & WB_REG1_CHECK_MSK)) {
        boot_msg2("check first reg fail:", first_rval[0], second_rval[0]);
        return HI_ERR_FAILURE;
    }
    if ((first_rval[1] & WB_REG2_CHECK_MSK) != (second_rval[1] & WB_REG2_CHECK_MSK)) {
        boot_msg2("check sencond reg fail:", first_rval[1], second_rval[1]);
        return HI_ERR_FAILURE;
    }
    *reg_value = first_rval[0];
    *(reg_value + 1) = first_rval[1];
    return HI_ERR_SUCCESS;
}

hi_u32 flash_protect_set_protect(hi_u8 cmp_bp, hi_bool is_volatile)
{
    hi_u32 ret;
    hi_u8 p_data[2] = {0};   /* 2 */
    hi_u8 cmp = (cmp_bp>>5) & 0x1; /* 5 */
    hi_u8 bp = cmp_bp & 0x1F;
    if (g_flash_reg_need_check == FLASH_PROTECT_NEED_CHK) {
        flash_wb_repair_reg3(g_flash_drv_ctrl.basic_info.chip_id);
    }
    ret = flash_protect_read_check_reg(p_data, sizeof(p_data));
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    if (((p_data[0] & (0x1F<<2)) == (bp<<2)) && ((p_data[1] & (0x1<<6)) == (cmp<<6))) { /* 2 6 */
        return HI_ERR_SUCCESS;
    }
    p_data[0] &= ~(0x1f<<2);      /* 2 */
    p_data[0] |= (hi_u8)(bp<<2);  /* 2 */
    p_data[0] &= ~(0x1 << WB_REG1_SRP_OFFSET);   /* srp need always 0 */
    p_data[1] &= ~(0x1 << WB_REG2_SRL_OFFSET);  /* srl need always 0 */
    p_data[1] &= ~(0x1<<6);       /* 6 */
    p_data[1] |= (hi_u8)(cmp<<6); /* 6 */
    ret = spi_flash_write_sr_reg(SPI_CMD_WRSR1, p_data, 2, is_volatile); /* 2 */
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    ret = spif_wait_ready(HI_TRUE, SPI_CMD_SR_WIPN, SPI_SR_BIT_WIP); /* wait until flash is idle */
    if (ret != HI_ERR_SUCCESS) {
        boot_msg1("spif_wait_ready ret:", ret);
        return ret;
    }
    ret = spi_flash_read_reg(SPI_CMD_RDSR, &p_data[0], 1);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    ret = spi_flash_read_reg(SPI_CMD_RDSR2, &p_data[1], 1);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    if (((p_data[0] & (0x1F<<2)) == (bp<<2)) && ((p_data[1] & (0x1<<6)) == (cmp<<6))) { /* 2 6 */
        return HI_ERR_SUCCESS;
    } else {
        return HI_ERR_FAILURE;
    }
}

hi_u32 flash_wb_read_reg3(hi_u8 *reg_val)
{
    hi_u32 ret;
    hi_u8 reg_first;
    hi_u8 reg_second;
    hi_u8 reg_third;
    if (reg_val == NULL) {
        return HI_ERR_FLASH_INVALID_PARAMETER;
    }
    ret = spif_wait_ready(HI_TRUE, SPI_CMD_SR_WIPN, SPI_SR_BIT_WIP); /* wait until flash is idle */
    if (ret != HI_ERR_SUCCESS) {
        boot_msg1("spif_wait_ready ret:", ret);
        return ret;
    }
    ret = spi_flash_read_reg(SPI_CMD_RDSR3, &reg_first, 1);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    ret = spi_flash_read_reg(SPI_CMD_RDSR3, &reg_second, 1);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    ret = spi_flash_read_reg(SPI_CMD_RDSR3, &reg_third, 1);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    if (((reg_first & WB_CHECK_REG3_MSK) != (reg_second & WB_CHECK_REG3_MSK)) ||
        ((reg_first & WB_CHECK_REG3_MSK) != (reg_third & WB_CHECK_REG3_MSK))) {
        boot_msg1("first read reg3:", reg_first);
        boot_msg1("second read reg3:", reg_second);
        boot_msg1("third read reg3:", reg_third);
        return HI_ERR_FAILURE;
    }
    *reg_val = reg_first;
    return ret;
}

hi_void flash_wb_repair_reg3(const hi_u8 *chip_id)
{
    hi_u8 check_times;
    hi_u8 reg_val;
    hi_u32 ret;
    hi_u32 delay_time = FIRST_DELAY_US;
    if (chip_id == NULL) {
        return;
    }
    /* applies only to the following flash id */
    if (!(((chip_id[0] == 0xEF) && (chip_id[1] == 0x60) && (chip_id[2] == 0x15)) || /* W25Q16JW idx:1 2 */
        ((chip_id[0] == 0xEF) && (chip_id[1] == 0x40) && (chip_id[2] == 0x15)))) { /* W25Q16JL idx:1 2 */
        return;
    }
    for (check_times = 0; check_times < MAX_CHECK_REG_TIMES; check_times++) {
        delay_time += (check_times == 0) ? 0 : NEXT_DELAY_US;
        ret = flash_wb_read_reg3(&reg_val);
        if (ret != HI_ERR_SUCCESS) {
            udelay(delay_time);
            continue;
        }
        /* check hold , wps is 0  */
        if ((((reg_val >> WB_REG3_HOLD_OFFSET) & 0x1) == 0) && (((reg_val >> WB_REG3_WPS_OFFSET) & 0x1) == 0)) {
            g_flash_reg_need_check = FLASH_PROTECT_NOT_CHK;
            return;
        }
        reg_val &= ~((0x1 << WB_REG3_HOLD_OFFSET) | (0x1 << WB_REG3_WPS_OFFSET));
        ret = spi_flash_write_sr_reg(SPI_CMD_WRSR3, &reg_val, 1, HI_FALSE);
        if (ret != HI_ERR_SUCCESS) {
            boot_msg1("write RDSR3 fail:\n", ret);
            udelay(delay_time);
            continue;
        }
        ret = flash_wb_read_reg3(&reg_val);
        if (ret != HI_ERR_SUCCESS) {
            udelay(delay_time);
            continue;
        }
        boot_msg1("wcheck RDSR3:", reg_val);
        /* check hold , wps is 0  */
        if ((((reg_val >> WB_REG3_HOLD_OFFSET) & 0x1) == 0) && (((reg_val >> WB_REG3_WPS_OFFSET) & 0x1) == 0)) {
            g_flash_reg_need_check = FLASH_PROTECT_NOT_CHK;
            return;
        }
        udelay(delay_time);
    }
    g_flash_reg_need_check = FLASH_PROTECT_NEED_CHK;
}

hi_void flash_clk_config(hi_void)
{
    hi_u16 reg_val;
    /* set 0: FPGA 80M, ASIC 96M. */
#ifdef HI_FLASH_SUPPORT_REF_VBAT
    hi_reg_clrbits(PMU_CMU_CTL_CMU_CLK_SEL_REG, 8, 2); /* set 8 left shift 2 */
    hi_reg_setbit(PMU_CMU_CTL_CMU_CLK_SEL_REG, 8);  /* 48M */
    hi_reg_setbit(PMU_CMU_CTL_CMU_CLK_SEL_REG, 9);  /* 9 */
#else
    /* When the VBAT reference voltage is unavailable, set this parameter based on the actual flash model. */
    hi_reg_clrbits(PMU_CMU_CTL_CMU_CLK_SEL_REG, 8, 2); /* set 8 left shift 2 */
    hi_reg_setbit(PMU_CMU_CTL_CMU_CLK_SEL_REG, 8);  /* 8 80M */
#endif
    hi_reg_clrbit(PMU_CMU_CTL_CLK_192M_GT_REG, 0);
    hi_reg_setbit(CLDO_CTL_CLK_SEL_REG, 1);
    hi_reg_clrbits(PMU_CMU_CTL_CMU_CLK_SEL_REG, 4, 3); /* set 4 left shift 3 */

    /* set sfc not div: in fpga, clk is 80M */
    hi_reg_clrbits(CLDO_CTL_CLK_DIV1_REG, 4, 3);       /* 4, 3 */
    /* en flash ldo bypass  */
    hi_reg_read16(PMU_CMU_CTL_FLASHLDO_CFG_1_REG, reg_val);
    reg_val &= ~(0x1 << 6); /* 6 */
    reg_val |= 0x1 << 6;    /* 6 */
    hi_reg_write16(PMU_CMU_CTL_FLASHLDO_CFG_1_REG, reg_val);

#ifdef HI_BOARD_ASIC
    /* Check whether the built-in flash memory exists based on the chip ID. If no built-in flash memory exists,
       disable flash_ldo. */
    hi_u8 chip_id;
    hi_u32 ret;
    ret = hi_efuse_read(HI_EFUSE_CHIP_RW_ID, (hi_u8 *)&chip_id, (hi_u8)sizeof(hi_u8));
    if (ret == HI_ERR_SUCCESS) {
        if (chip_id == HI_CHIP_ID_1131SV200) {
            hi_reg_setbit(PMU_CMU_CTL_PMU_MAN_CLR_0_REG, 8); /* set 1 left shift 8 */
            hi_reg_setbit(CLDO_CTL_CLK_SEL_REG, 0);
        }
    }
#endif
}

hi_u32 hi_flash_erase(hi_u32 flash_addr, hi_u32 flash_erase_size)
{
    hi_u32 ret;
    hi_spi_flash_ctrl *spif_ctrl = &g_flash_drv_ctrl;
    ret = sfc_check_para(spif_ctrl, flash_addr, flash_erase_size, HI_FLASH_CHECK_PARAM_OPT_ERASE);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
#ifdef HI_FLASH_SUPPORT_REF_VBAT
    if ((g_voltage <= HI_FLASH_VOLTAGE_TH0) && (memcmp(g_flash_vlt_sfc_info.chip_id,
        g_flash_vlt_sfc_info_tbl[2].chip_id, HI_FLASH_CHIP_ID_NUM) == HI_ERR_SUCCESS)) { // 2
        sfc_config_cmu_clk_sel(CMU_CLK_SEL_48M);
    }
#endif
    ret = flash_erase_prv(spif_ctrl, flash_addr, flash_erase_size);
#ifdef HI_FLASH_SUPPORT_REF_VBAT
    if ((g_voltage <= HI_FLASH_VOLTAGE_TH0) && (memcmp(g_flash_vlt_sfc_info.chip_id,
        g_flash_vlt_sfc_info_tbl[2].chip_id, HI_FLASH_CHIP_ID_NUM) == HI_ERR_SUCCESS)) { // 2
        sfc_config_cmu_clk_sel(CMU_CLK_SEL_80M);
    }
#endif

    flash_info_print("hi_flash_erase ret:%x addr:%x len:%x\r\n", ret, flash_addr, flash_erase_size);
    return ret;
}

hi_u32 hi_flash_write(hi_u32 flash_addr, hi_u32 flash_write_size, const hi_u8 *flash_write_data, hi_bool do_erase)
{
    hi_u32 ret;
    hi_spi_flash_ctrl *spif_ctrl = &g_flash_drv_ctrl;
    if (flash_write_data == HI_NULL) {
        return HI_ERR_FLASH_INVALID_PARAMETER;
    }
    ret = sfc_check_para(spif_ctrl, flash_addr, flash_write_size, HI_FLASH_CHECK_PARAM_OPT_WRITE);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
#ifdef HI_FLASH_SUPPORT_REF_VBAT
    if ((g_voltage <= HI_FLASH_VOLTAGE_TH0) && (memcmp(g_flash_vlt_sfc_info.chip_id,
        g_flash_vlt_sfc_info_tbl[2].chip_id, HI_FLASH_CHIP_ID_NUM) == HI_ERR_SUCCESS)) { // 2
        sfc_config_cmu_clk_sel(CMU_CLK_SEL_48M);
    }
#endif
    ret = flash_write_prv(spif_ctrl, flash_addr, flash_write_data, flash_write_size, do_erase);
#ifdef HI_FLASH_SUPPORT_REF_VBAT
    if ((g_voltage <= HI_FLASH_VOLTAGE_TH0) && (memcmp(g_flash_vlt_sfc_info.chip_id,
        g_flash_vlt_sfc_info_tbl[2].chip_id, HI_FLASH_CHIP_ID_NUM) == HI_ERR_SUCCESS)) { // 2
        sfc_config_cmu_clk_sel(CMU_CLK_SEL_80M);
    }
#endif
    flash_info_print("hi_flash_write ret:%x addr:%x len:%x\r\n", ret, flash_addr, flash_write_size);
    return ret;
}

hi_u32 hi_flash_read(hi_u32 flash_addr, hi_u32 flash_read_size, hi_u8 *flash_read_data)
{
    hi_u32 ret;
    hi_spi_flash_ctrl *spif_ctrl = &g_flash_drv_ctrl;
    if (flash_read_data == HI_NULL) {
        return HI_ERR_FLASH_INVALID_PARAMETER;
    }
    ret = sfc_check_para(spif_ctrl, flash_addr, flash_read_size, HI_FLASH_CHECK_PARAM_OPT_READ);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
#ifdef HI_FLASH_SUPPORT_REF_VBAT
    if ((g_voltage <= HI_FLASH_VOLTAGE_TH0) && (memcmp(g_flash_vlt_sfc_info.chip_id,
        g_flash_vlt_sfc_info_tbl[2].chip_id, HI_FLASH_CHIP_ID_NUM) == HI_ERR_SUCCESS)) { // 2
        sfc_config_cmu_clk_sel(CMU_CLK_SEL_48M);
    }
#endif
    ret = flash_read_prv(spif_ctrl, flash_addr, flash_read_data, flash_read_size);
#ifdef HI_FLASH_SUPPORT_REF_VBAT
    if ((g_voltage <= HI_FLASH_VOLTAGE_TH0) && (memcmp(g_flash_vlt_sfc_info.chip_id,
        g_flash_vlt_sfc_info_tbl[2].chip_id, HI_FLASH_CHIP_ID_NUM) == HI_ERR_SUCCESS)) { // 2
        sfc_config_cmu_clk_sel(CMU_CLK_SEL_80M);
    }
#endif
    flash_info_print("hi_flash_read ret2:%x addr:%x len:%x\r\n", ret, flash_addr, flash_read_size);
    return ret;
}

hi_u32 flash_init_cfg(hi_spi_flash_ctrl *spif_ctrl, const hi_u8 *chip_id, hi_u32 idlen)
{
    hi_u32 ret;

    if ((spif_ctrl == HI_NULL) || (chip_id == HI_NULL) || (idlen != HI_FLASH_CHIP_ID_NUM)) {
        return HI_ERR_FAILURE;
    }
    flash_clk_config();
#ifdef HI_FLASH_SUPPORT_REF_VBAT
    /* set voltage vs. frequency */
    sfc_config_set_experience(chip_id, &g_flash_vlt_sfc_info, g_flash_vlt_sfc_info_tbl,
        sizeof(g_flash_vlt_sfc_info_tbl) / sizeof(g_flash_vlt_sfc_info_tbl[0]));
    sfc_config_update_freq((hi_u32)(uintptr_t)&g_flash_vlt_sfc_info);
#endif
    /* temply compatible with MX flash chip at FPGA debug phase. */
    if (chip_id[0] == 0xC2 && chip_id[1] == 0x20 && chip_id[2] == 0x19) { /* check chip id 0,1,2 byte */
        ret = spi_flash_enable_quad_mode_mx();
    } else {
        ret = spi_flash_enable_quad_mode();
    }
    hi_io_set_pull(HI_IO_NAME_SFC_IO2, HI_IO_PULL_NONE);
    hi_io_set_pull(HI_IO_NAME_SFC_IO3, HI_IO_PULL_NONE);
    if (ret == HI_ERR_SUCCESS) {
        hi_u32 cs;
#ifdef HI_FLASH_SUPPORT_REF_VBAT
        if ((g_voltage <= HI_FLASH_VOLTAGE_TH0) &&
            (memcmp(chip_id, g_flash_vlt_sfc_info_tbl[2].chip_id, HI_FLASH_CHIP_ID_NUM) == HI_ERR_SUCCESS)) { /* 2 wb */
            cs = ((uintptr_t)&spif_ctrl->opt_read) ^ sizeof(spi_flash_operation) ^
                ((uintptr_t)&g_spi_opt_fast_quad_out_read) ^ sizeof(spi_flash_operation);
            if (memcpy_s(&spif_ctrl->opt_read, sizeof(spi_flash_operation), &g_spi_opt_fast_quad_out_read,
                sizeof(spi_flash_operation), cs) != EOK) {
                return HI_ERR_FAILURE;
            }
        } else {
            cs = ((uintptr_t)&spif_ctrl->opt_read) ^ sizeof(spi_flash_operation) ^
                ((uintptr_t)&g_spi_opt_fast_quad_eb_out_read) ^ sizeof(spi_flash_operation);
            if (memcpy_s(&spif_ctrl->opt_read, sizeof(spi_flash_operation), &g_spi_opt_fast_quad_eb_out_read,
                sizeof(spi_flash_operation), cs) != EOK) {
                return HI_ERR_FAILURE;
            }
        }
#else
        cs = ((uintptr_t)&spif_ctrl->opt_read) ^ sizeof(spi_flash_operation) ^
            ((uintptr_t)&g_spi_opt_fast_quad_eb_out_read) ^ sizeof(spi_flash_operation);
        if (memcpy_s(&spif_ctrl->opt_read, sizeof(spi_flash_operation), &g_spi_opt_fast_quad_eb_out_read,
            sizeof(spi_flash_operation), cs) != EOK) {
            return HI_ERR_FAILURE;
        }
#endif
    }
    /* config flash sfc after flash init. */
    spif_config(&(spif_ctrl->opt_read), spif_ctrl->opt_read.cmd, HI_TRUE);
    return ret;
}

hi_u32 hi_flash_init(hi_void)
{
    hi_u32 ret;
    hi_spi_flash_ctrl *spif_ctrl = &g_flash_drv_ctrl;
    hi_u8 chip_id[HI_FLASH_CHIP_ID_NUM] = {0};
    hi_u32 sfc_clock;

    if (spif_ctrl->is_inited == HI_TRUE) {
        return HI_ERR_FLASH_RE_INIT;
    }
    /* pull up flash hold and wp io */
    hi_io_set_pull(HI_IO_NAME_SFC_IO2, HI_IO_PULL_UP);
    hi_io_set_pull(HI_IO_NAME_SFC_IO3, HI_IO_PULL_UP);

    spif_ctrl->dma_ram_buffer = (hi_u8*)g_dma_buffer;
    spif_ctrl->dma_ram_size = FLASH_DMA_RAM_SIZE;
    spif_ctrl->back_up_buf = (hi_u8 *)g_back_buffer;
    ret = spi_flash_basic_info_probe(spif_ctrl, chip_id, HI_FLASH_CHIP_ID_NUM,
                                     (hi_spi_flash_basic_info*)g_flash_default_info_tbl, HI_FLASH_DEFAULT_TYPE_NUM);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }

    ret = flash_init_cfg(spif_ctrl, chip_id, HI_FLASH_CHIP_ID_NUM);
    if (ret != HI_ERR_SUCCESS) {
        return ret;
    }
    flash_wb_repair_reg3(chip_id);
    spif_ctrl->is_inited = HI_TRUE;

    if (chip_id[0] == 0xC2 && chip_id[1] == 0x20 && chip_id[2] == 0x19) { /* check chip id 0,1,2 byte */
        sfc_clock = hisfc_read(SFC_REG_GLOBAL_CONFIG);
        sfc_clock = sfc_clock & 0xFFFFFFE7;
        sfc_clock = sfc_clock | 0x08;
        hisfc_write(SFC_REG_GLOBAL_CONFIG, sfc_clock);
    }
    hisfc_write(SFC_REG_TIMING, 0x1);

    return ret;
}

