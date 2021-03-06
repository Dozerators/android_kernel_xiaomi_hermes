/*
 * Reversed by LazyC0DEr
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of this archive for
 * more details.
 */

#include <mach/mt_pm_ldo.h>
#include <mach/mt_gpio.h>
#include <cust_gpio_usage.h>
#include <cust_i2c.h>

#include "lcm_drv.h"
#include "tps65132_i2c.h"

static LCM_UTIL_FUNCS lcm_util;
static raw_spinlock_t tianma_SpinLock;
static int lcm_intialized;

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define read_reg_v2(cmd, buffer, buffer_size)			lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#define MDELAY(n)						(lcm_util.mdelay(n))
#define UDELAY(n)						(lcm_util.udelay(n))

#define REGFLAG_DELAY						0xFC
#define REGFLAG_END_OF_TABLE					0xFD

struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static struct LCM_setting_table lcm_backlight_level_setting[] = {
    {0xFF, 1, {0x00}},
    {0xFB, 1, {0x01}},
    {0x51, 1, {0xFF}},
    {REGFLAG_END_OF_TABLE, 0, {0x00}}
};

static struct LCM_setting_table lcm_backlight_disable[] = {
    {0x55, 1, {0x00}},
    {REGFLAG_END_OF_TABLE, 0, {0x00}}
};

static struct LCM_setting_table lcm_backlight_enable[] = {
    {0x55, 1, {0x01}},
    {REGFLAG_END_OF_TABLE, 0, {0x00}}
};

static struct LCM_setting_table lcm_suspend_setting[] = {
    {0x51, 1, {0x00}},
    {0x28, 0, {0x00}},
    {REGFLAG_DELAY, 20, {0x00}},
    {0x10, 0, {0x00}},
    {REGFLAG_DELAY, 120, {0x00}},
    {REGFLAG_END_OF_TABLE, 0, {0x00}}
};

static struct LCM_setting_table lcm_resume_setting[] = {
    {0xFF, 1, {0xEE}},
    {0xFB, 1, {0x01}},
    {0x18, 1, {0x40}},
    {REGFLAG_DELAY, 10, {0x00}},
    {0x18, 1, {0x00}},
    {REGFLAG_DELAY, 20, {0x00}},
    {0x7C, 1, {0x31}},
    {0xFF, 1, {0x01}},
    {0xFB, 1, {0x01}},
    {0x00, 1, {0x01}},
    {0x01, 1, {0x55}},
    {0x02, 1, {0x40}},
    {0x05, 1, {0x40}},
    {0x06, 1, {0x0A}},
    {0x07, 1, {0x14}},
    {0x08, 1, {0x0C}},
    {0x0B, 1, {0x7D}},
    {0x0C, 1, {0x7D}},
    {0x0E, 1, {0xAB}},
    {0x0F, 1, {0xA4}},
    {0x14, 1, {0x14}},
    {0x15, 1, {0x13}},
    {0x16, 1, {0x13}},
    {0x18, 1, {0x00}},
    {0x19, 1, {0x77}},
    {0x1A, 1, {0x55}},
    {0x1B, 1, {0x13}},
    {0x1C, 1, {0x00}},
    {0x1D, 1, {0x00}},
    {0x1E, 1, {0x13}},
    {0x1F, 1, {0x00}},
    {0x35, 1, {0x00}},
    {0x66, 1, {0x00}},
    {0x58, 1, {0x81}},
    {0x59, 1, {0x01}},
    {0x5A, 1, {0x01}},
    {0x5B, 1, {0x01}},
    {0x5C, 1, {0x82}},
    {0x5D, 1, {0x82}},
    {0x5E, 1, {0x02}},
    {0x5F, 1, {0x02}},
    {0x6D, 1, {0x22}},
    {0x72, 1, {0x31}},
    {0xFF, 1, {0x05}},
    {0xFB, 1, {0x01}},
    {0x00, 1, {0x00}},
    {0x01, 1, {0x00}},
    {0x02, 1, {0x03}},
    {0x03, 1, {0x04}},
    {0x04, 1, {0x00}},
    {0x05, 1, {0x11}},
    {0x06, 1, {0x0C}},
    {0x07, 1, {0x0B}},
    {0x08, 1, {0x01}},
    {0x09, 1, {0x00}},
    {0x0A, 1, {0x18}},
    {0x0B, 1, {0x16}},
    {0x0C, 1, {0x14}},
    {0x0D, 1, {0x17}},
    {0x0E, 1, {0x15}},
    {0x0F, 1, {0x13}},
    {0x10, 1, {0x00}},
    {0x11, 1, {0x00}},
    {0x12, 1, {0x03}},
    {0x13, 1, {0x04}},
    {0x14, 1, {0x00}},
    {0x15, 1, {0x11}},
    {0x16, 1, {0x0C}},
    {0x17, 1, {0x0B}},
    {0x18, 1, {0x01}},
    {0x19, 1, {0x00}},
    {0x1A, 1, {0x18}},
    {0x1B, 1, {0x16}},
    {0x1C, 1, {0x14}},
    {0x1D, 1, {0x17}},
    {0x1E, 1, {0x15}},
    {0x1F, 1, {0x13}},
    {0x20, 1, {0x00}},
    {0x21, 1, {0x02}},
    {0x22, 1, {0x09}},
    {0x23, 1, {0x67}},
    {0x24, 1, {0x06}},
    {0x25, 1, {0x1D}},
    {0x29, 1, {0x58}},
    {0x2A, 1, {0x11}},
    {0x2B, 1, {0x04}},
    {0x2F, 1, {0x02}},
    {0x30, 1, {0x01}},
    {0x31, 1, {0x49}},
    {0x32, 1, {0x23}},
    {0x33, 1, {0x01}},
    {0x34, 1, {0x03}},
    {0x35, 1, {0x6B}},
    {0x36, 1, {0x00}},
    {0x37, 1, {0x1D}},
    {0x38, 1, {0x00}},
    {0x5D, 1, {0x23}},
    {0x61, 1, {0x15}},
    {0x65, 1, {0x00}},
    {0x69, 1, {0x04}},
    {0x6C, 1, {0x51}},
    {0x7A, 1, {0x00}},
    {0x7B, 1, {0x80}},
    {0x7C, 1, {0xD8}},
    {0x7D, 1, {0x10}},
    {0x7E, 1, {0x06}},
    {0x7F, 1, {0x1B}},
    {0x81, 1, {0x06}},
    {0x82, 1, {0x02}},
    {0x8A, 1, {0x33}},
    {0x93, 1, {0x06}},
    {0x94, 1, {0x06}},
    {0x9B, 1, {0x0F}},
    {0xA4, 1, {0x0F}},
    {0xE7, 1, {0x80}},
    {0xFF, 1, {0x01}},
    {0xFB, 1, {0x01}},
    {0x75, 1, {0x00}},
    {0x76, 1, {0x00}},
    {0x77, 1, {0x00}},
    {0x78, 1, {0x21}},
    {0x79, 1, {0x00}},
    {0x7A, 1, {0x4A}},
    {0x7B, 1, {0x00}},
    {0x7C, 1, {0x66}},
    {0x7D, 1, {0x00}},
    {0x7E, 1, {0x7F}},
    {0x7F, 1, {0x00}},
    {0x80, 1, {0x94}},
    {0x81, 1, {0x00}},
    {0x82, 1, {0xA7}},
    {0x83, 1, {0x00}},
    {0x84, 1, {0xB8}},
    {0x85, 1, {0x00}},
    {0x86, 1, {0xC7}},
    {0x87, 1, {0x00}},
    {0x88, 1, {0xFB}},
    {0x89, 1, {0x01}},
    {0x8A, 1, {0x25}},
    {0x8B, 1, {0x01}},
    {0x8C, 1, {0x61}},
    {0x8D, 1, {0x01}},
    {0x8E, 1, {0x94}},
    {0x8F, 1, {0x01}},
    {0x90, 1, {0xE2}},
    {0x91, 1, {0x02}},
    {0x92, 1, {0x20}},
    {0x93, 1, {0x02}},
    {0x94, 1, {0x22}},
    {0x95, 1, {0x02}},
    {0x96, 1, {0x5C}},
    {0x97, 1, {0x02}},
    {0x98, 1, {0x9E}},
    {0x99, 1, {0x02}},
    {0x9A, 1, {0xC9}},
    {0x9B, 1, {0x03}},
    {0x9C, 1, {0x01}},
    {0x9D, 1, {0x03}},
    {0x9E, 1, {0x28}},
    {0x9F, 1, {0x03}},
    {0xA0, 1, {0x55}},
    {0xA2, 1, {0x03}},
    {0xA3, 1, {0x62}},
    {0xA4, 1, {0x03}},
    {0xA5, 1, {0x6F}},
    {0xA6, 1, {0x03}},
    {0xA7, 1, {0x7E}},
    {0xA9, 1, {0x03}},
    {0xAA, 1, {0x8F}},
    {0xAB, 1, {0x03}},
    {0xAC, 1, {0x9C}},
    {0xAD, 1, {0x03}},
    {0xAE, 1, {0xA2}},
    {0xAF, 1, {0x03}},
    {0xB0, 1, {0xAB}},
    {0xB1, 1, {0x03}},
    {0xB2, 1, {0xB2}},
    {0xB3, 1, {0x00}},
    {0xB4, 1, {0x00}},
    {0xB5, 1, {0x00}},
    {0xB6, 1, {0x21}},
    {0xB7, 1, {0x00}},
    {0xB8, 1, {0x4A}},
    {0xB9, 1, {0x00}},
    {0xBA, 1, {0x66}},
    {0xBB, 1, {0x00}},
    {0xBC, 1, {0x7F}},
    {0xBD, 1, {0x00}},
    {0xBE, 1, {0x94}},
    {0xBF, 1, {0x00}},
    {0xC0, 1, {0xA7}},
    {0xC1, 1, {0x00}},
    {0xC2, 1, {0xB8}},
    {0xC3, 1, {0x00}},
    {0xC4, 1, {0xC7}},
    {0xC5, 1, {0x00}},
    {0xC6, 1, {0xFB}},
    {0xC7, 1, {0x01}},
    {0xC8, 1, {0x25}},
    {0xC9, 1, {0x01}},
    {0xCA, 1, {0x61}},
    {0xCB, 1, {0x01}},
    {0xCC, 1, {0x94}},
    {0xCD, 1, {0x01}},
    {0xCE, 1, {0xE2}},
    {0xCF, 1, {0x02}},
    {0xD0, 1, {0x20}},
    {0xD1, 1, {0x02}},
    {0xD2, 1, {0x22}},
    {0xD3, 1, {0x02}},
    {0xD4, 1, {0x5C}},
    {0xD5, 1, {0x02}},
    {0xD6, 1, {0x9E}},
    {0xD7, 1, {0x02}},
    {0xD8, 1, {0xC9}},
    {0xD9, 1, {0x03}},
    {0xDA, 1, {0x01}},
    {0xDB, 1, {0x03}},
    {0xDC, 1, {0x28}},
    {0xDD, 1, {0x03}},
    {0xDE, 1, {0x55}},
    {0xDF, 1, {0x03}},
    {0xE0, 1, {0x62}},
    {0xE1, 1, {0x03}},
    {0xE2, 1, {0x6F}},
    {0xE3, 1, {0x03}},
    {0xE4, 1, {0x7E}},
    {0xE5, 1, {0x03}},
    {0xE6, 1, {0x8F}},
    {0xE7, 1, {0x03}},
    {0xE8, 1, {0x9C}},
    {0xE9, 1, {0x03}},
    {0xEA, 1, {0xA2}},
    {0xEB, 1, {0x03}},
    {0xEC, 1, {0xAB}},
    {0xED, 1, {0x03}},
    {0xEE, 1, {0xB2}},
    {0xEF, 1, {0x00}},
    {0xF0, 1, {0x00}},
    {0xF1, 1, {0x00}},
    {0xF2, 1, {0x21}},
    {0xF3, 1, {0x00}},
    {0xF4, 1, {0x4A}},
    {0xF5, 1, {0x00}},
    {0xF6, 1, {0x66}},
    {0xF7, 1, {0x00}},
    {0xF8, 1, {0x7F}},
    {0xF9, 1, {0x00}},
    {0xFA, 1, {0x94}},
    {0xFF, 1, {0x02}},
    {0xFB, 1, {0x01}},
    {0x00, 1, {0x00}},
    {0x01, 1, {0xA7}},
    {0x02, 1, {0x00}},
    {0x03, 1, {0xB8}},
    {0x04, 1, {0x00}},
    {0x05, 1, {0xC7}},
    {0x06, 1, {0x00}},
    {0x07, 1, {0xFB}},
    {0x08, 1, {0x01}},
    {0x09, 1, {0x25}},
    {0x0A, 1, {0x01}},
    {0x0B, 1, {0x61}},
    {0x0C, 1, {0x01}},
    {0x0D, 1, {0x94}},
    {0x0E, 1, {0x01}},
    {0x0F, 1, {0xE2}},
    {0x10, 1, {0x02}},
    {0x11, 1, {0x20}},
    {0x12, 1, {0x02}},
    {0x13, 1, {0x22}},
    {0x14, 1, {0x02}},
    {0x15, 1, {0x5C}},
    {0x16, 1, {0x02}},
    {0x17, 1, {0x9E}},
    {0x18, 1, {0x02}},
    {0x19, 1, {0xC9}},
    {0x1A, 1, {0x03}},
    {0x1B, 1, {0x01}},
    {0x1C, 1, {0x03}},
    {0x1D, 1, {0x28}},
    {0x1E, 1, {0x03}},
    {0x1F, 1, {0x55}},
    {0x20, 1, {0x03}},
    {0x21, 1, {0x62}},
    {0x22, 1, {0x03}},
    {0x23, 1, {0x6F}},
    {0x24, 1, {0x03}},
    {0x25, 1, {0x7E}},
    {0x26, 1, {0x03}},
    {0x27, 1, {0x8F}},
    {0x28, 1, {0x03}},
    {0x29, 1, {0x9C}},
    {0x2A, 1, {0x03}},
    {0x2B, 1, {0xA2}},
    {0x2D, 1, {0x03}},
    {0x2F, 1, {0xAB}},
    {0x30, 1, {0x03}},
    {0x31, 1, {0xB2}},
    {0x32, 1, {0x00}},
    {0x33, 1, {0x00}},
    {0x34, 1, {0x00}},
    {0x35, 1, {0x21}},
    {0x36, 1, {0x00}},
    {0x37, 1, {0x4A}},
    {0x38, 1, {0x00}},
    {0x39, 1, {0x66}},
    {0x3A, 1, {0x00}},
    {0x3B, 1, {0x7F}},
    {0x3D, 1, {0x00}},
    {0x3F, 1, {0x94}},
    {0x40, 1, {0x00}},
    {0x41, 1, {0xA7}},
    {0x42, 1, {0x00}},
    {0x43, 1, {0xB8}},
    {0x44, 1, {0x00}},
    {0x45, 1, {0xC7}},
    {0x46, 1, {0x00}},
    {0x47, 1, {0xFB}},
    {0x48, 1, {0x01}},
    {0x49, 1, {0x25}},
    {0x4A, 1, {0x01}},
    {0x4B, 1, {0x61}},
    {0x4C, 1, {0x01}},
    {0x4D, 1, {0x94}},
    {0x4E, 1, {0x01}},
    {0x4F, 1, {0xE2}},
    {0x50, 1, {0x02}},
    {0x51, 1, {0x20}},
    {0x52, 1, {0x02}},
    {0x53, 1, {0x22}},
    {0x54, 1, {0x02}},
    {0x55, 1, {0x5C}},
    {0x56, 1, {0x02}},
    {0x58, 1, {0x9E}},
    {0x59, 1, {0x02}},
    {0x5A, 1, {0xC9}},
    {0x5B, 1, {0x03}},
    {0x5C, 1, {0x01}},
    {0x5D, 1, {0x03}},
    {0x5E, 1, {0x28}},
    {0x5F, 1, {0x03}},
    {0x60, 1, {0x55}},
    {0x61, 1, {0x03}},
    {0x62, 1, {0x62}},
    {0x63, 1, {0x03}},
    {0x64, 1, {0x6F}},
    {0x65, 1, {0x03}},
    {0x66, 1, {0x7E}},
    {0x67, 1, {0x03}},
    {0x68, 1, {0x8F}},
    {0x69, 1, {0x03}},
    {0x6A, 1, {0x9C}},
    {0x6B, 1, {0x03}},
    {0x6C, 1, {0xA2}},
    {0x6D, 1, {0x03}},
    {0x6E, 1, {0xAB}},
    {0x6F, 1, {0x03}},
    {0x70, 1, {0xB2}},
    {0x71, 1, {0x00}},
    {0x72, 1, {0x00}},
    {0x73, 1, {0x00}},
    {0x74, 1, {0x1E}},
    {0x75, 1, {0x00}},
    {0x76, 1, {0x48}},
    {0x77, 1, {0x00}},
    {0x78, 1, {0x57}},
    {0x79, 1, {0x00}},
    {0x7A, 1, {0x6A}},
    {0x7B, 1, {0x00}},
    {0x7C, 1, {0x80}},
    {0x7D, 1, {0x00}},
    {0x7E, 1, {0x90}},
    {0x7F, 1, {0x00}},
    {0x80, 1, {0xA0}},
    {0x81, 1, {0x00}},
    {0x82, 1, {0xAE}},
    {0x83, 1, {0x00}},
    {0x84, 1, {0xE3}},
    {0x85, 1, {0x01}},
    {0x86, 1, {0x0E}},
    {0x87, 1, {0x01}},
    {0x88, 1, {0x50}},
    {0x89, 1, {0x01}},
    {0x8A, 1, {0x88}},
    {0x8B, 1, {0x01}},
    {0x8C, 1, {0xDA}},
    {0x8D, 1, {0x02}},
    {0x8E, 1, {0x19}},
    {0x8F, 1, {0x02}},
    {0x90, 1, {0x1B}},
    {0x91, 1, {0x02}},
    {0x92, 1, {0x58}},
    {0x93, 1, {0x02}},
    {0x94, 1, {0x9C}},
    {0x95, 1, {0x02}},
    {0x96, 1, {0xC6}},
    {0x97, 1, {0x03}},
    {0x98, 1, {0x01}},
    {0x99, 1, {0x03}},
    {0x9A, 1, {0x28}},
    {0x9B, 1, {0x03}},
    {0x9C, 1, {0x55}},
    {0x9D, 1, {0x03}},
    {0x9E, 1, {0x62}},
    {0x9F, 1, {0x03}},
    {0xA0, 1, {0x6F}},
    {0xA2, 1, {0x03}},
    {0xA3, 1, {0x7E}},
    {0xA4, 1, {0x03}},
    {0xA5, 1, {0x8F}},
    {0xA6, 1, {0x03}},
    {0xA7, 1, {0x9C}},
    {0xA9, 1, {0x03}},
    {0xAA, 1, {0xA2}},
    {0xAB, 1, {0x03}},
    {0xAC, 1, {0xAB}},
    {0xAD, 1, {0x03}},
    {0xAE, 1, {0xB2}},
    {0xAF, 1, {0x00}},
    {0xB0, 1, {0x00}},
    {0xB1, 1, {0x00}},
    {0xB2, 1, {0x1E}},
    {0xB3, 1, {0x00}},
    {0xB4, 1, {0x48}},
    {0xB5, 1, {0x00}},
    {0xB6, 1, {0x57}},
    {0xB7, 1, {0x00}},
    {0xB8, 1, {0x6A}},
    {0xB9, 1, {0x00}},
    {0xBA, 1, {0x80}},
    {0xBB, 1, {0x00}},
    {0xBC, 1, {0x90}},
    {0xBD, 1, {0x00}},
    {0xBE, 1, {0xA0}},
    {0xBF, 1, {0x00}},
    {0xC0, 1, {0xAE}},
    {0xC1, 1, {0x00}},
    {0xC2, 1, {0xE3}},
    {0xC3, 1, {0x01}},
    {0xC4, 1, {0x0E}},
    {0xC5, 1, {0x01}},
    {0xC6, 1, {0x50}},
    {0xC7, 1, {0x01}},
    {0xC8, 1, {0x88}},
    {0xC9, 1, {0x01}},
    {0xCA, 1, {0xDA}},
    {0xCB, 1, {0x02}},
    {0xCC, 1, {0x19}},
    {0xCD, 1, {0x02}},
    {0xCE, 1, {0x1B}},
    {0xCF, 1, {0x02}},
    {0xD0, 1, {0x58}},
    {0xD1, 1, {0x02}},
    {0xD2, 1, {0x9C}},
    {0xD3, 1, {0x02}},
    {0xD4, 1, {0xC6}},
    {0xD5, 1, {0x03}},
    {0xD6, 1, {0x01}},
    {0xD7, 1, {0x03}},
    {0xD8, 1, {0x28}},
    {0xD9, 1, {0x03}},
    {0xDA, 1, {0x55}},
    {0xDB, 1, {0x03}},
    {0xDC, 1, {0x62}},
    {0xDD, 1, {0x03}},
    {0xDE, 1, {0x6F}},
    {0xDF, 1, {0x03}},
    {0xE0, 1, {0x7E}},
    {0xE1, 1, {0x03}},
    {0xE2, 1, {0x8F}},
    {0xE3, 1, {0x03}},
    {0xE4, 1, {0x9C}},
    {0xE5, 1, {0x03}},
    {0xE6, 1, {0xA2}},
    {0xE7, 1, {0x03}},
    {0xE8, 1, {0xAB}},
    {0xE9, 1, {0x03}},
    {0xEA, 1, {0xB2}},
    {0xFF, 1, {0x05}},
    {0xFB, 1, {0x01}},
    {0xE7, 1, {0x00}},
    {0xFF, 1, {0x04}},
    {0xFB, 1, {0x01}},
    {0x08, 1, {0x06}},
    {0xFF, 1, {0x00}},
    {0xFB, 1, {0x01}},
    {0xD3, 1, {0x06}},
    {0xD4, 1, {0x16}},
    {0x51, 1, {0x20}},
    {0x53, 1, {0x24}},
    {0x55, 1, {0x01}},
    {0x11, 0, {0x00}},
    {REGFLAG_DELAY, 120, {0x00}},
    {0x29, 0, {0x00}},
    {REGFLAG_DELAY, 20, {0x00}},
    {REGFLAG_END_OF_TABLE, 0, {0x00}}
};

static struct LCM_setting_table lcm_initialization_setting[] = {
    {0xFF, 1, {0xEE}},
    {0xFB, 1, {0x01}},
    {0x18, 1, {0x40}},
    {REGFLAG_DELAY, 10, {0x00}},
    {0x18, 1, {0x00}},
    {REGFLAG_DELAY, 20, {0x00}},
    {0x7C, 1, {0x31}},
    {0xFF, 1, {0x01}},
    {0xFB, 1, {0x01}},
    {0x00, 1, {0x01}},
    {0x01, 1, {0x55}},
    {0x02, 1, {0x40}},
    {0x05, 1, {0x40}},
    {0x06, 1, {0x0A}},
    {0x07, 1, {0x14}},
    {0x08, 1, {0x0C}},
    {0x0B, 1, {0x7D}},
    {0x0C, 1, {0x7D}},
    {0x0E, 1, {0xAB}},
    {0x0F, 1, {0xA4}},
    {0x14, 1, {0x14}},
    {0x15, 1, {0x13}},
    {0x16, 1, {0x13}},
    {0x18, 1, {0x00}},
    {0x19, 1, {0x77}},
    {0x1A, 1, {0x55}},
    {0x1B, 1, {0x13}},
    {0x1C, 1, {0x00}},
    {0x1D, 1, {0x00}},
    {0x1E, 1, {0x13}},
    {0x1F, 1, {0x00}},
    {0x35, 1, {0x00}},
    {0x66, 1, {0x00}},
    {0x58, 1, {0x81}},
    {0x59, 1, {0x01}},
    {0x5A, 1, {0x01}},
    {0x5B, 1, {0x01}},
    {0x5C, 1, {0x82}},
    {0x5D, 1, {0x82}},
    {0x5E, 1, {0x02}},
    {0x5F, 1, {0x02}},
    {0x6D, 1, {0x22}},
    {0x72, 1, {0x31}},
    {0xFF, 1, {0x05}},
    {0xFB, 1, {0x01}},
    {0x00, 1, {0x00}},
    {0x01, 1, {0x00}},
    {0x02, 1, {0x03}},
    {0x03, 1, {0x04}},
    {0x04, 1, {0x00}},
    {0x05, 1, {0x11}},
    {0x06, 1, {0x0C}},
    {0x07, 1, {0x0B}},
    {0x08, 1, {0x01}},
    {0x09, 1, {0x00}},
    {0x0A, 1, {0x18}},
    {0x0B, 1, {0x16}},
    {0x0C, 1, {0x14}},
    {0x0D, 1, {0x17}},
    {0x0E, 1, {0x15}},
    {0x0F, 1, {0x13}},
    {0x10, 1, {0x00}},
    {0x11, 1, {0x00}},
    {0x12, 1, {0x03}},
    {0x13, 1, {0x04}},
    {0x14, 1, {0x00}},
    {0x15, 1, {0x11}},
    {0x16, 1, {0x0C}},
    {0x17, 1, {0x0B}},
    {0x18, 1, {0x01}},
    {0x19, 1, {0x00}},
    {0x1A, 1, {0x18}},
    {0x1B, 1, {0x16}},
    {0x1C, 1, {0x14}},
    {0x1D, 1, {0x17}},
    {0x1E, 1, {0x15}},
    {0x1F, 1, {0x13}},
    {0x20, 1, {0x00}},
    {0x21, 1, {0x02}},
    {0x22, 1, {0x09}},
    {0x23, 1, {0x67}},
    {0x24, 1, {0x06}},
    {0x25, 1, {0x1D}},
    {0x29, 1, {0x58}},
    {0x2A, 1, {0x11}},
    {0x2B, 1, {0x04}},
    {0x2F, 1, {0x02}},
    {0x30, 1, {0x01}},
    {0x31, 1, {0x49}},
    {0x32, 1, {0x23}},
    {0x33, 1, {0x01}},
    {0x34, 1, {0x03}},
    {0x35, 1, {0x6B}},
    {0x36, 1, {0x00}},
    {0x37, 1, {0x1D}},
    {0x38, 1, {0x00}},
    {0x5D, 1, {0x23}},
    {0x61, 1, {0x15}},
    {0x65, 1, {0x00}},
    {0x69, 1, {0x04}},
    {0x6C, 1, {0x51}},
    {0x7A, 1, {0x00}},
    {0x7B, 1, {0x80}},
    {0x7C, 1, {0xD8}},
    {0x7D, 1, {0x10}},
    {0x7E, 1, {0x06}},
    {0x7F, 1, {0x1B}},
    {0x81, 1, {0x06}},
    {0x82, 1, {0x02}},
    {0x8A, 1, {0x33}},
    {0x93, 1, {0x06}},
    {0x94, 1, {0x06}},
    {0x9B, 1, {0x0F}},
    {0xA4, 1, {0x0F}},
    {0xE7, 1, {0x80}},
    {0xFF, 1, {0x01}},
    {0xFB, 1, {0x01}},
    {0x75, 1, {0x00}},
    {0x76, 1, {0x00}},
    {0x77, 1, {0x00}},
    {0x78, 1, {0x21}},
    {0x79, 1, {0x00}},
    {0x7A, 1, {0x4A}},
    {0x7B, 1, {0x00}},
    {0x7C, 1, {0x66}},
    {0x7D, 1, {0x00}},
    {0x7E, 1, {0x7F}},
    {0x7F, 1, {0x00}},
    {0x80, 1, {0x94}},
    {0x81, 1, {0x00}},
    {0x82, 1, {0xA7}},
    {0x83, 1, {0x00}},
    {0x84, 1, {0xB8}},
    {0x85, 1, {0x00}},
    {0x86, 1, {0xC7}},
    {0x87, 1, {0x00}},
    {0x88, 1, {0xFB}},
    {0x89, 1, {0x01}},
    {0x8A, 1, {0x25}},
    {0x8B, 1, {0x01}},
    {0x8C, 1, {0x61}},
    {0x8D, 1, {0x01}},
    {0x8E, 1, {0x94}},
    {0x8F, 1, {0x01}},
    {0x90, 1, {0xE2}},
    {0x91, 1, {0x02}},
    {0x92, 1, {0x20}},
    {0x93, 1, {0x02}},
    {0x94, 1, {0x22}},
    {0x95, 1, {0x02}},
    {0x96, 1, {0x5C}},
    {0x97, 1, {0x02}},
    {0x98, 1, {0x9E}},
    {0x99, 1, {0x02}},
    {0x9A, 1, {0xC9}},
    {0x9B, 1, {0x03}},
    {0x9C, 1, {0x01}},
    {0x9D, 1, {0x03}},
    {0x9E, 1, {0x28}},
    {0x9F, 1, {0x03}},
    {0xA0, 1, {0x55}},
    {0xA2, 1, {0x03}},
    {0xA3, 1, {0x62}},
    {0xA4, 1, {0x03}},
    {0xA5, 1, {0x6F}},
    {0xA6, 1, {0x03}},
    {0xA7, 1, {0x7E}},
    {0xA9, 1, {0x03}},
    {0xAA, 1, {0x8F}},
    {0xAB, 1, {0x03}},
    {0xAC, 1, {0x9C}},
    {0xAD, 1, {0x03}},
    {0xAE, 1, {0xA2}},
    {0xAF, 1, {0x03}},
    {0xB0, 1, {0xAB}},
    {0xB1, 1, {0x03}},
    {0xB2, 1, {0xB2}},
    {0xB3, 1, {0x00}},
    {0xB4, 1, {0x00}},
    {0xB5, 1, {0x00}},
    {0xB6, 1, {0x21}},
    {0xB7, 1, {0x00}},
    {0xB8, 1, {0x4A}},
    {0xB9, 1, {0x00}},
    {0xBA, 1, {0x66}},
    {0xBB, 1, {0x00}},
    {0xBC, 1, {0x7F}},
    {0xBD, 1, {0x00}},
    {0xBE, 1, {0x94}},
    {0xBF, 1, {0x00}},
    {0xC0, 1, {0xA7}},
    {0xC1, 1, {0x00}},
    {0xC2, 1, {0xB8}},
    {0xC3, 1, {0x00}},
    {0xC4, 1, {0xC7}},
    {0xC5, 1, {0x00}},
    {0xC6, 1, {0xFB}},
    {0xC7, 1, {0x01}},
    {0xC8, 1, {0x25}},
    {0xC9, 1, {0x01}},
    {0xCA, 1, {0x61}},
    {0xCB, 1, {0x01}},
    {0xCC, 1, {0x94}},
    {0xCD, 1, {0x01}},
    {0xCE, 1, {0xE2}},
    {0xCF, 1, {0x02}},
    {0xD0, 1, {0x20}},
    {0xD1, 1, {0x02}},
    {0xD2, 1, {0x22}},
    {0xD3, 1, {0x02}},
    {0xD4, 1, {0x5C}},
    {0xD5, 1, {0x02}},
    {0xD6, 1, {0x9E}},
    {0xD7, 1, {0x02}},
    {0xD8, 1, {0xC9}},
    {0xD9, 1, {0x03}},
    {0xDA, 1, {0x01}},
    {0xDB, 1, {0x03}},
    {0xDC, 1, {0x28}},
    {0xDD, 1, {0x03}},
    {0xDE, 1, {0x55}},
    {0xDF, 1, {0x03}},
    {0xE0, 1, {0x62}},
    {0xE1, 1, {0x03}},
    {0xE2, 1, {0x6F}},
    {0xE3, 1, {0x03}},
    {0xE4, 1, {0x7E}},
    {0xE5, 1, {0x03}},
    {0xE6, 1, {0x8F}},
    {0xE7, 1, {0x03}},
    {0xE8, 1, {0x9C}},
    {0xE9, 1, {0x03}},
    {0xEA, 1, {0xA2}},
    {0xEB, 1, {0x03}},
    {0xEC, 1, {0xAB}},
    {0xED, 1, {0x03}},
    {0xEE, 1, {0xB2}},
    {0xEF, 1, {0x00}},
    {0xF0, 1, {0x00}},
    {0xF1, 1, {0x00}},
    {0xF2, 1, {0x21}},
    {0xF3, 1, {0x00}},
    {0xF4, 1, {0x4A}},
    {0xF5, 1, {0x00}},
    {0xF6, 1, {0x66}},
    {0xF7, 1, {0x00}},
    {0xF8, 1, {0x7F}},
    {0xF9, 1, {0x00}},
    {0xFA, 1, {0x94}},
    {0xFF, 1, {0x02}},
    {0xFB, 1, {0x01}},
    {0x00, 1, {0x00}},
    {0x01, 1, {0xA7}},
    {0x02, 1, {0x00}},
    {0x03, 1, {0xB8}},
    {0x04, 1, {0x00}},
    {0x05, 1, {0xC7}},
    {0x06, 1, {0x00}},
    {0x07, 1, {0xFB}},
    {0x08, 1, {0x01}},
    {0x09, 1, {0x25}},
    {0x0A, 1, {0x01}},
    {0x0B, 1, {0x61}},
    {0x0C, 1, {0x01}},
    {0x0D, 1, {0x94}},
    {0x0E, 1, {0x01}},
    {0x0F, 1, {0xE2}},
    {0x10, 1, {0x02}},
    {0x11, 1, {0x20}},
    {0x12, 1, {0x02}},
    {0x13, 1, {0x22}},
    {0x14, 1, {0x02}},
    {0x15, 1, {0x5C}},
    {0x16, 1, {0x02}},
    {0x17, 1, {0x9E}},
    {0x18, 1, {0x02}},
    {0x19, 1, {0xC9}},
    {0x1A, 1, {0x03}},
    {0x1B, 1, {0x01}},
    {0x1C, 1, {0x03}},
    {0x1D, 1, {0x28}},
    {0x1E, 1, {0x03}},
    {0x1F, 1, {0x55}},
    {0x20, 1, {0x03}},
    {0x21, 1, {0x62}},
    {0x22, 1, {0x03}},
    {0x23, 1, {0x6F}},
    {0x24, 1, {0x03}},
    {0x25, 1, {0x7E}},
    {0x26, 1, {0x03}},
    {0x27, 1, {0x8F}},
    {0x28, 1, {0x03}},
    {0x29, 1, {0x9C}},
    {0x2A, 1, {0x03}},
    {0x2B, 1, {0xA2}},
    {0x2D, 1, {0x03}},
    {0x2F, 1, {0xAB}},
    {0x30, 1, {0x03}},
    {0x31, 1, {0xB2}},
    {0x32, 1, {0x00}},
    {0x33, 1, {0x00}},
    {0x34, 1, {0x00}},
    {0x35, 1, {0x21}},
    {0x36, 1, {0x00}},
    {0x37, 1, {0x4A}},
    {0x38, 1, {0x00}},
    {0x39, 1, {0x66}},
    {0x3A, 1, {0x00}},
    {0x3B, 1, {0x7F}},
    {0x3D, 1, {0x00}},
    {0x3F, 1, {0x94}},
    {0x40, 1, {0x00}},
    {0x41, 1, {0xA7}},
    {0x42, 1, {0x00}},
    {0x43, 1, {0xB8}},
    {0x44, 1, {0x00}},
    {0x45, 1, {0xC7}},
    {0x46, 1, {0x00}},
    {0x47, 1, {0xFB}},
    {0x48, 1, {0x01}},
    {0x49, 1, {0x25}},
    {0x4A, 1, {0x01}},
    {0x4B, 1, {0x61}},
    {0x4C, 1, {0x01}},
    {0x4D, 1, {0x94}},
    {0x4E, 1, {0x01}},
    {0x4F, 1, {0xE2}},
    {0x50, 1, {0x02}},
    {0x51, 1, {0x20}},
    {0x52, 1, {0x02}},
    {0x53, 1, {0x22}},
    {0x54, 1, {0x02}},
    {0x55, 1, {0x5C}},
    {0x56, 1, {0x02}},
    {0x58, 1, {0x9E}},
    {0x59, 1, {0x02}},
    {0x5A, 1, {0xC9}},
    {0x5B, 1, {0x03}},
    {0x5C, 1, {0x01}},
    {0x5D, 1, {0x03}},
    {0x5E, 1, {0x28}},
    {0x5F, 1, {0x03}},
    {0x60, 1, {0x55}},
    {0x61, 1, {0x03}},
    {0x62, 1, {0x62}},
    {0x63, 1, {0x03}},
    {0x64, 1, {0x6F}},
    {0x65, 1, {0x03}},
    {0x66, 1, {0x7E}},
    {0x67, 1, {0x03}},
    {0x68, 1, {0x8F}},
    {0x69, 1, {0x03}},
    {0x6A, 1, {0x9C}},
    {0x6B, 1, {0x03}},
    {0x6C, 1, {0xA2}},
    {0x6D, 1, {0x03}},
    {0x6E, 1, {0xAB}},
    {0x6F, 1, {0x03}},
    {0x70, 1, {0xB2}},
    {0x71, 1, {0x00}},
    {0x72, 1, {0x00}},
    {0x73, 1, {0x00}},
    {0x74, 1, {0x1E}},
    {0x75, 1, {0x00}},
    {0x76, 1, {0x48}},
    {0x77, 1, {0x00}},
    {0x78, 1, {0x57}},
    {0x79, 1, {0x00}},
    {0x7A, 1, {0x6A}},
    {0x7B, 1, {0x00}},
    {0x7C, 1, {0x80}},
    {0x7D, 1, {0x00}},
    {0x7E, 1, {0x90}},
    {0x7F, 1, {0x00}},
    {0x80, 1, {0xA0}},
    {0x81, 1, {0x00}},
    {0x82, 1, {0xAE}},
    {0x83, 1, {0x00}},
    {0x84, 1, {0xE3}},
    {0x85, 1, {0x01}},
    {0x86, 1, {0x0E}},
    {0x87, 1, {0x01}},
    {0x88, 1, {0x50}},
    {0x89, 1, {0x01}},
    {0x8A, 1, {0x88}},
    {0x8B, 1, {0x01}},
    {0x8C, 1, {0xDA}},
    {0x8D, 1, {0x02}},
    {0x8E, 1, {0x19}},
    {0x8F, 1, {0x02}},
    {0x90, 1, {0x1B}},
    {0x91, 1, {0x02}},
    {0x92, 1, {0x58}},
    {0x93, 1, {0x02}},
    {0x94, 1, {0x9C}},
    {0x95, 1, {0x02}},
    {0x96, 1, {0xC6}},
    {0x97, 1, {0x03}},
    {0x98, 1, {0x01}},
    {0x99, 1, {0x03}},
    {0x9A, 1, {0x28}},
    {0x9B, 1, {0x03}},
    {0x9C, 1, {0x55}},
    {0x9D, 1, {0x03}},
    {0x9E, 1, {0x62}},
    {0x9F, 1, {0x03}},
    {0xA0, 1, {0x6F}},
    {0xA2, 1, {0x03}},
    {0xA3, 1, {0x7E}},
    {0xA4, 1, {0x03}},
    {0xA5, 1, {0x8F}},
    {0xA6, 1, {0x03}},
    {0xA7, 1, {0x9C}},
    {0xA9, 1, {0x03}},
    {0xAA, 1, {0xA2}},
    {0xAB, 1, {0x03}},
    {0xAC, 1, {0xAB}},
    {0xAD, 1, {0x03}},
    {0xAE, 1, {0xB2}},
    {0xAF, 1, {0x00}},
    {0xB0, 1, {0x00}},
    {0xB1, 1, {0x00}},
    {0xB2, 1, {0x1E}},
    {0xB3, 1, {0x00}},
    {0xB4, 1, {0x48}},
    {0xB5, 1, {0x00}},
    {0xB6, 1, {0x57}},
    {0xB7, 1, {0x00}},
    {0xB8, 1, {0x6A}},
    {0xB9, 1, {0x00}},
    {0xBA, 1, {0x80}},
    {0xBB, 1, {0x00}},
    {0xBC, 1, {0x90}},
    {0xBD, 1, {0x00}},
    {0xBE, 1, {0xA0}},
    {0xBF, 1, {0x00}},
    {0xC0, 1, {0xAE}},
    {0xC1, 1, {0x00}},
    {0xC2, 1, {0xE3}},
    {0xC3, 1, {0x01}},
    {0xC4, 1, {0x0E}},
    {0xC5, 1, {0x01}},
    {0xC6, 1, {0x50}},
    {0xC7, 1, {0x01}},
    {0xC8, 1, {0x88}},
    {0xC9, 1, {0x01}},
    {0xCA, 1, {0xDA}},
    {0xCB, 1, {0x02}},
    {0xCC, 1, {0x19}},
    {0xCD, 1, {0x02}},
    {0xCE, 1, {0x1B}},
    {0xCF, 1, {0x02}},
    {0xD0, 1, {0x58}},
    {0xD1, 1, {0x02}},
    {0xD2, 1, {0x9C}},
    {0xD3, 1, {0x02}},
    {0xD4, 1, {0xC6}},
    {0xD5, 1, {0x03}},
    {0xD6, 1, {0x01}},
    {0xD7, 1, {0x03}},
    {0xD8, 1, {0x28}},
    {0xD9, 1, {0x03}},
    {0xDA, 1, {0x55}},
    {0xDB, 1, {0x03}},
    {0xDC, 1, {0x62}},
    {0xDD, 1, {0x03}},
    {0xDE, 1, {0x6F}},
    {0xDF, 1, {0x03}},
    {0xE0, 1, {0x7E}},
    {0xE1, 1, {0x03}},
    {0xE2, 1, {0x8F}},
    {0xE3, 1, {0x03}},
    {0xE4, 1, {0x9C}},
    {0xE5, 1, {0x03}},
    {0xE6, 1, {0xA2}},
    {0xE7, 1, {0x03}},
    {0xE8, 1, {0xAB}},
    {0xE9, 1, {0x03}},
    {0xEA, 1, {0xB2}},
    {0xFF, 1, {0x05}},
    {0xFB, 1, {0x01}},
    {0xE7, 1, {0x00}},
    {0xFF, 1, {0x04}},
    {0xFB, 1, {0x01}},
    {0x08, 1, {0x06}},
    {0xFF, 1, {0x00}},
    {0xFB, 1, {0x01}},
    {0xD3, 1, {0x06}},
    {0xD4, 1, {0x16}},
    {0x51, 1, {0x20}},
    {0x53, 1, {0x24}},
    {0x55, 1, {0x00}},
    {0x11, 0, {0x00}},
    {REGFLAG_DELAY, 120, {0x00}},
    {0x29, 0, {0x00}},
    {REGFLAG_DELAY, 20, {0x00}},
    {REGFLAG_END_OF_TABLE, 0, {0x00}}
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;
    for (i = 0; i < count; i++) {
        unsigned cmd;
        cmd = table[i].cmd;

        switch (cmd) {
            case REGFLAG_DELAY:
                if (table[i].count <= 10)
                    MDELAY(table[i].count);
                else
                    MDELAY(table[i].count);
                break;
            case REGFLAG_END_OF_TABLE:
                break;
            default:
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
        }
    }
}

static void tps65132_enable(bool enable){
    int i;
    mt_set_gpio_mode(GPIO_MHL_RST_B_PIN, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_MHL_RST_B_PIN, GPIO_DIR_OUT);
    mt_set_gpio_mode(GPIO_MHL_EINT_PIN, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_MHL_EINT_PIN, GPIO_DIR_OUT);
    if (enable){
        mt_set_gpio_out(GPIO_MHL_EINT_PIN, GPIO_OUT_ONE);
        MDELAY(12);
        mt_set_gpio_out(GPIO_MHL_RST_B_PIN, GPIO_OUT_ONE);
        MDELAY(12);
        for (i = 0; i < 3; i++) {
            if ((tps65132_write_bytes(0, 0xF) & 0x80000000) == 0)
                break;
            MDELAY(5);
        }
        for (i = 0; i < 3; i++) {
            if ((tps65132_write_bytes(1, 0xF) & 0x80000000) == 0)
                break;
            MDELAY(5);
        }
    } else {
        mt_set_gpio_out(GPIO_MHL_RST_B_PIN, GPIO_OUT_ZERO);
        MDELAY(12);
        mt_set_gpio_out(GPIO_MHL_EINT_PIN, GPIO_OUT_ZERO);
        MDELAY(12);
    }
}

static void tianma_senddata(unsigned char value){
    int i;
    raw_spin_lock_irq(&tianma_SpinLock);
    mt_set_gpio_out(GPIO_MHL_POWER_CTRL_PIN, GPIO_OUT_ONE);
    UDELAY(15);
    for (i = 7; i >= 0; i--) {
        if ((value >> i) & 1) {
            mt_set_gpio_out(GPIO_MHL_POWER_CTRL_PIN, GPIO_OUT_ZERO);
            UDELAY(10);
            mt_set_gpio_out(GPIO_MHL_POWER_CTRL_PIN, GPIO_OUT_ONE);
            UDELAY(30);
        } else {
            mt_set_gpio_out(GPIO_MHL_POWER_CTRL_PIN, GPIO_OUT_ZERO);
            UDELAY(30);
            mt_set_gpio_out(GPIO_MHL_POWER_CTRL_PIN, GPIO_OUT_ONE);
            UDELAY(10);
        }
    }
    mt_set_gpio_out(GPIO_MHL_POWER_CTRL_PIN, GPIO_OUT_ZERO);
    UDELAY(15);
    mt_set_gpio_out(GPIO_MHL_POWER_CTRL_PIN, GPIO_OUT_ONE);
    UDELAY(350);
    raw_spin_unlock_irq(&tianma_SpinLock);
}

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS *params)
{
    memset(params, 0, sizeof(LCM_PARAMS));

    params->dsi.LANE_NUM = 4;
    params->dsi.vertical_backporch = 4;
    params->dsi.vertical_frontporch = 22;
    params->dsi.horizontal_sync_active = 4;
    params->physical_width = 68;
    params->physical_height = 121;
    params->dsi.mode = 3;
    params->dsi.PLL_CLOCK = 475;
    params->dsi.lcm_esd_check_table[0].cmd = 0xA;
    params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;
    params->type = 2;
    params->dsi.data_format.format = 2;
    params->dsi.PS = 2;
    params->dsi.vertical_sync_active = 2;
    params->width = 1080;
    params->dsi.horizontal_active_pixel = 1080;
    params->height = 1920;
    params->dsi.vertical_active_line = 1920;
    params->dsi.switch_mode_enable = 0;
    params->dsi.data_format.color_order = 0;
    params->dsi.data_format.trans_seq = 0;
    params->dsi.data_format.padding = 0;
    params->dsi.noncont_clock = 1;
    params->dsi.esd_check_enable = 1;
    params->dsi.customization_esd_check_enable = 1;
    params->dsi.lcm_esd_check_table[0].count = 1;
    params->dsi.horizontal_backporch = 72;
    params->dsi.horizontal_frontporch = 72;
}

static void lcm_init(void)
{
    tps65132_enable(1);
    MDELAY(20);
    mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
    MDELAY(20);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
    MDELAY(20);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
    MDELAY(50);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
    MDELAY(50);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
    MDELAY(50);
    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
    MDELAY(20);
    lcm_intialized = 1;
}

static void lcm_suspend(void)
{
    push_table(lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
    mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
    MDELAY(10);
    tps65132_enable(0);
}

static void lcm_resume(void)
{
    tps65132_enable(1);
    MDELAY(15);
    mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
    MDELAY(3);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
    MDELAY(3);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
    MDELAY(3);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
    MDELAY(3);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
    MDELAY(22);
    if (lcm_intialized)
        push_table(lcm_resume_setting, sizeof(lcm_resume_setting) / sizeof(struct LCM_setting_table), 1);
    else
        push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}

static unsigned int lcm_compare_id(void)
{
    unsigned char LCD_ID_value;
    LCD_ID_value = which_lcd_module_triple();
    if (LCD_ID_value == 2) {
        unsigned int id;
        unsigned char buffer[2];
        unsigned int array[16];

        mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
        mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
        mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
        MDELAY(1);
        mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
        MDELAY(10);
        mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
        MDELAY(10);

        array[0] = 0x23700;
        dsi_set_cmdq(array, 1, 1);
        array[0] = 0xFF1500;
        dsi_set_cmdq(array, 1, 1);
        array[0] = 0x1FB1500;
        dsi_set_cmdq(array, 1, 1);
        MDELAY(10);
        read_reg_v2(0xF4, buffer, 1);
        MDELAY(20);
        id = buffer[0];
        return (id == 0x96)?1:0;
    } else
        return 0;
}

static void lcm_setbacklight_cmdq(void* handle, unsigned int level)
{
    static int tianma_value_0 = 0, tianma_value_1 = 0, tianma_value_2 = 0;
    if (level != tianma_value_0) {
        tianma_value_0 = level;
        tianma_value_1 = level;
        if (tianma_value_1 != 0 || tianma_value_2 != 0) {
            mt_set_gpio_mode(GPIO_MHL_POWER_CTRL_PIN, GPIO_MODE_00);
            mt_set_gpio_dir(GPIO_MHL_POWER_CTRL_PIN, GPIO_DIR_OUT);
            if (level) {
                if (level - 1 > 2) {
                    if (level > 255)
                        level = 255;
                } else
                    level = 3;

                if (level < 32)
                    tianma_senddata(64 - level * 2);
                else {
                    if (tianma_value_1 >= 32) {
                        if (tianma_value_2 >= 32) {
                            mt_set_gpio_out(GPIO_MHL_POWER_CTRL_PIN, GPIO_OUT_ONE);
                            MDELAY(10);
                        } else
                            tianma_senddata(0);
                    }
                }
            } else {
                mt_set_gpio_out(GPIO_MHL_POWER_CTRL_PIN, GPIO_OUT_ZERO);
                MDELAY(30);
            }

            if (level < 32)
                lcm_backlight_level_setting[2].para_list[0] = 32;
            else
                lcm_backlight_level_setting[2].para_list[0] = (unsigned char)level;
            push_table(lcm_backlight_level_setting, sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);
            tianma_value_2 = tianma_value_1;
        }
    }
}

static void lcm_cabc_enable_cmdq(unsigned int mode)
{
    if (mode)
        push_table(lcm_backlight_enable, sizeof(lcm_backlight_enable) / sizeof(struct LCM_setting_table), 1);
    else
        push_table(lcm_backlight_disable, sizeof(lcm_backlight_disable) / sizeof(struct LCM_setting_table), 1);
}

LCM_DRIVER nt35596_fhd_tianma_phantom_lcm_drv = {
    .name = "nt35596_fhd_tianma_phantom",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params = lcm_get_params,
    .init = lcm_init,
    .suspend = lcm_suspend,
    .resume = lcm_resume,
    .compare_id = lcm_compare_id,
    .set_backlight_cmdq = lcm_setbacklight_cmdq,
    .set_pwm = lcm_cabc_enable_cmdq
};
