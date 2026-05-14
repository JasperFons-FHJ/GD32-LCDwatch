#include "spi.h"

void spi_lcd_init(void)
{
    spi_parameter_struct spi_init_struct;

    rcu_periph_clock_enable(RCU_AF);
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_SPI2);

    gpio_pin_remap_config(GPIO_SWJ_SWDPENABLE_REMAP, ENABLE);

    gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_3 | GPIO_PIN_5);
    gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_4 | GPIO_PIN_6 | GPIO_PIN_7);
    gpio_bit_set(GPIOB, GPIO_PIN_4);
    gpio_bit_set(GPIOB, GPIO_PIN_6);
    gpio_bit_reset(GPIOB, GPIO_PIN_7);

    gpio_init(GPIOA, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_15);
    gpio_bit_reset(GPIOA, GPIO_PIN_15);

    spi_i2s_deinit(SPI2);
    spi_init_struct.trans_mode = SPI_TRANSMODE_BDTRANSMIT;
    spi_init_struct.device_mode = SPI_MASTER;
    spi_init_struct.frame_size = SPI_FRAMESIZE_8BIT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_HIGH_PH_2EDGE;
    spi_init_struct.nss = SPI_NSS_SOFT;
    spi_init_struct.prescale = SPI_PSC_8;
    spi_init_struct.endian = SPI_ENDIAN_MSB;
    spi_init(SPI2, &spi_init_struct);
    spi_enable(SPI2);
}

void SPI2_WriteBytes(uint8_t *pbuffer, uint32_t length)
{
    gpio_bit_reset(GPIOA, GPIO_PIN_15);

    while (length-- != 0U) {
        while (spi_i2s_flag_get(SPI2, SPI_FLAG_TBE) == RESET) {
        }

        spi_i2s_data_transmit(SPI2, *pbuffer);

        while (spi_i2s_flag_get(SPI2, SPI_STAT_TRANS) == SET) {
        }

        pbuffer++;
    }
}
