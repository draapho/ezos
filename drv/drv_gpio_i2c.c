/**
 * \file            drv_gpio_i2c.c
 * \brief           I2C driver source file. IO口模拟 I2C 驱动程序. 主机模式, 7位地址.
 */

#include "drv_gpio_i2c.h"

#ifdef DRV_I2C_NAME_SCL_SDA

/* i2c porting */
typedef struct {
    gpio_hw_t scl;
    gpio_hw_t sda;
} i2c_hw_t;

static const i2c_hw_t i2c_hw[DRV_I2C_NAME_END] = {  // I2C硬件映射表
#define X(name, cport, cpin, dport, dpin) {{cport, cpin}, {dport, dpin}},
    DRV_I2C_NAME_SCL_SDA
#undef X
};

// I2C微秒延时函数, 用于产生软件模拟I2C的SCL信号波特率.
__STATIC_INLINE void i2c_delay_us(int16_t us) {
    delay_us(us);
}

// I2C软件模拟 IO口初始化
__STATIC_INLINE void i2c_port_init(i2c_name_t i2c_name) {
    drv_output_od_init(&i2c_hw[i2c_name].scl);
    drv_output_od_init(&i2c_hw[i2c_name].sda);
}

// I2C软件模拟SCL电平置高, 并等待SCL拉高信号(从设备释放). 返回1, 已置高. 0, 超时.
__STATIC_INLINE uint8_t i2c_scl_high(i2c_name_t i2c_name) {
    int wait_us = 100;  // 该值为, 等待从设备释放SCL信号的最大时间.

    drv_output_high(&i2c_hw[i2c_name].scl);
    do {
        if (drv_input_level(&i2c_hw[i2c_name].scl)) return 1;
        i2c_delay_us(1);
    } while (wait_us-- > 0);
    return 0;
}

// I2C软件模拟SCL电平置低
__STATIC_INLINE void i2c_scl_low(i2c_name_t i2c_name) {
    drv_output_low(&i2c_hw[i2c_name].scl);
}

// I2C软件模拟SDA电平置高
__STATIC_INLINE void i2c_sda_high(i2c_name_t i2c_name) {
    drv_output_high(&i2c_hw[i2c_name].sda);
}

// I2C软件模拟SDA电平置低
__STATIC_INLINE void i2c_sda_low(i2c_name_t i2c_name) {
    drv_output_low(&i2c_hw[i2c_name].sda);
}

// 读取软件模拟I2C的SDA电平值
__STATIC_INLINE uint32_t i2c_sda_level(i2c_name_t i2c_name) {
    return drv_input_level(&i2c_hw[i2c_name].sda);
}

/* variable */
static int16_t i2c_us[DRV_I2C_NAME_END];  // 软件模拟I2C SCL延时时间

/* function */
// 初始化函数. baudrate_khz, 精确的时钟数值: 250, 125, 83, 62, 50. 其它值只能近似.
void i2c_init(i2c_name_t i2c_name, uint16_t baudrate_khz) {
    ASSERT(i2c_name < DRV_I2C_NAME_END);
    if (i2c_name < DRV_I2C_NAME_END) {
        i2c_port_init(i2c_name);
        i2c_us[i2c_name] = 250 / baudrate_khz;  // 1000us 4等分为 250us, 进行时序控制
        if (i2c_us[i2c_name] == 0) i2c_us[i2c_name] = 1;
        i2c_stop(i2c_name);
    }
}

//	I2C起始信号
void i2c_start(i2c_name_t i2c_name) {
    if (i2c_name < DRV_I2C_NAME_END) {
        i2c_sda_high(i2c_name);
        i2c_delay_us(i2c_us[i2c_name]);
        i2c_scl_high(i2c_name);
        i2c_delay_us(i2c_us[i2c_name]);
        i2c_sda_low(i2c_name);
        i2c_delay_us(i2c_us[i2c_name]);
        i2c_scl_low(i2c_name);
        i2c_delay_us(i2c_us[i2c_name]);
    }
}

//	I2C结束信号
void i2c_stop(i2c_name_t i2c_name) {
    if (i2c_name < DRV_I2C_NAME_END) {
        i2c_sda_low(i2c_name);
        i2c_delay_us(i2c_us[i2c_name]);
        i2c_scl_high(i2c_name);
        i2c_delay_us(i2c_us[i2c_name]);
        i2c_sda_high(i2c_name);
    }
}

/**
 * \brief		通过I2C发送一个7bit地址数据, 准备写从设备
 * \param[in]	i2c_name	see i2c_name_t
 * \param[in]	addr		要发送的7bit地址, bit0无效.
 * \return		i2c_ack_t
 *	\arg \c		I2C_ACK		0, I2C 有应答信号
 *	\arg \c		I2C_NO_ACK	1, I2C 无应答信号
 */
i2c_ack_t i2c_tx_addr_wr_slaver(i2c_name_t i2c_name, uint8_t addr) {
    i2c_ack_t ack = I2C_NO_ACK;
    if (i2c_name < DRV_I2C_NAME_END) {
        addr &= 0xFE;  // 标记为写从设备
        ack = i2c_tx_byte(i2c_name, addr);
    }
    return ack;
}

/**
 * \brief		通过I2C发送一个7bit地址数据, 准备读从设备
 * \param[in]	i2c_name	see i2c_name_t
 * \param[in]	addr		要发送的7bit地址, bit0无效.
 * \return		i2c_ack_t
 *	\arg \c		I2C_ACK		0, I2C 有应答信号
 *	\arg \c		I2C_NO_ACK	1, I2C 无应答信号
 */
i2c_ack_t i2c_tx_addr_rd_slaver(i2c_name_t i2c_name, uint8_t addr) {
    i2c_ack_t ack = I2C_NO_ACK;
    if (i2c_name < DRV_I2C_NAME_END) {
        addr |= 0x01;  // 标记为读从设备
        ack = i2c_tx_byte(i2c_name, addr);
    }
    return ack;
}

/**
 * \brief		通过I2C发送一个字节的数据
 * \param[in]	i2c_name	see i2c_name_t
 * \param[in]	data		要发送的数据
 * \return		i2c_ack_t
 *	\arg \c		I2C_ACK		0, I2C 有应答信号
 *	\arg \c		I2C_NO_ACK	1, I2C 无应答信号
 */
i2c_ack_t i2c_tx_byte(i2c_name_t i2c_name, uint8_t data) {
    uint8_t i;
    i2c_ack_t ack = I2C_NO_ACK;
    if (i2c_name >= DRV_I2C_NAME_END) return ack;

    for (i = 0x80; i > 0; i >>= 1) {
        if (data & i) {
            i2c_sda_high(i2c_name);
        } else {
            i2c_sda_low(i2c_name);
        }
        i2c_delay_us(i2c_us[i2c_name]);
        i2c_scl_high(i2c_name);
        i2c_delay_us(i2c_us[i2c_name]);
        i2c_delay_us(i2c_us[i2c_name]);
        i2c_scl_low(i2c_name);
        i2c_delay_us(i2c_us[i2c_name]);
    }

    i2c_sda_high(i2c_name);
    i2c_delay_us(i2c_us[i2c_name]);     // 有些从设备需要充分的时间来控制CLK信号
    if (i2c_scl_high(i2c_name) == 1) {  // 从设备应答周期
        if (i2c_sda_level(i2c_name) == 0) ack = I2C_ACK;
    }
    i2c_delay_us(i2c_us[i2c_name]);
    i2c_delay_us(i2c_us[i2c_name]);
    i2c_scl_low(i2c_name);
    i2c_delay_us(i2c_us[i2c_name]);
    // i2c_delay_us(i2c_us[i2c_name]);    // 每字节额外的延时, 便于示波器观察
    // i2c_delay_us(i2c_us[i2c_name]);
    return ack;
}

/**
 * \brief		通过I2C接收一个字节的数据
 * \param[in]	i2c_name	see i2c_name_t
 * \param[in]	ack			要发送的应答信号
 *	\arg \c		I2C_ACK		0, I2C 有应答信号
 *	\arg \c		I2C_NO_ACK	1, I2C 无应答信号
 * \return		data, 接收到的数据
 */
uint8_t i2c_rx_byte(i2c_name_t i2c_name, i2c_ack_t ack) {
    uint8_t i, data = 0;
    if (i2c_name >= DRV_I2C_NAME_END) return 0xFF;

    i2c_sda_high(i2c_name);  // 使SDA处于输入状态
    for (i = 0; i < 8; i++) {
        i2c_delay_us(i2c_us[i2c_name]);
        i2c_scl_high(i2c_name);
        i2c_delay_us(i2c_us[i2c_name]);
        data <<= 1;
        if (i2c_sda_level(i2c_name)) data++;
        i2c_delay_us(i2c_us[i2c_name]);
        i2c_scl_low(i2c_name);
        i2c_delay_us(i2c_us[i2c_name]);
    }

    if (ack == I2C_ACK) {
        i2c_sda_low(i2c_name);
    } else {
        i2c_sda_high(i2c_name);
    }
    i2c_delay_us(i2c_us[i2c_name] + 1);
    i2c_scl_high(i2c_name);
    i2c_delay_us(i2c_us[i2c_name]);
    i2c_delay_us(i2c_us[i2c_name]);
    i2c_scl_low(i2c_name);
    i2c_delay_us(i2c_us[i2c_name]);
    // i2c_delay_us(i2c_us[i2c_name]);    // 每字节额外的延时, 便于示波器观察
    // i2c_delay_us(i2c_us[i2c_name]);

    return data;
}

#endif /* DRV_I2C_NAME_SCL_SDA */
