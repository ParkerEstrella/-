#include "adc_driver.h"
#include "gd32f4xx.h"

/* ADC 配置 */
#define ADC_GPIO_PORT    GPIOC
#define ADC_GPIO_PIN   GPIO_PIN_0
#define ADC_GPIO_CLK   RCU_GPIOC

#define ADC_PERIPH     ADC0
#define ADC_CLK        RCU_ADC0
#define ADC_CHANNEL    ADC_CHANNEL_10
#define ADC_REF_VOLTAGE 3.3f

void adc_module_init(void)
{
    /* 使能时钟 */
    rcu_periph_clock_enable(ADC_GPIO_CLK);
    rcu_periph_clock_enable(ADC_CLK);
    adc_clock_config(ADC_ADCCK_PCLK2_DIV8);
    
    /* 配置GPIO */
    gpio_mode_set(ADC_GPIO_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, ADC_GPIO_PIN);
    
    /* 配置ADC */
    adc_deinit();
    adc_sync_mode_config(ADC_SYNC_MODE_INDEPENDENT);
    adc_special_function_config(ADC_PERIPH, ADC_SCAN_MODE, DISABLE);
    adc_resolution_config(ADC_PERIPH, ADC_RESOLUTION_12B);
    adc_data_alignment_config(ADC_PERIPH, ADC_DATAALIGN_RIGHT);
    adc_channel_length_config(ADC_PERIPH, ADC_ROUTINE_CHANNEL, 1);
    adc_routine_channel_config(ADC_PERIPH, 0, ADC_CHANNEL, ADC_SAMPLETIME_15);
    adc_external_trigger_config(ADC_PERIPH, ADC_ROUTINE_CHANNEL, EXTERNAL_TRIGGER_DISABLE);
    
    /* 使能并校准 */
    adc_enable(ADC_PERIPH);
    for (volatile u32 i = 0; i < 10000; i++);
    adc_calibration_enable(ADC_PERIPH);
}

u16 adc_read_single(void)
{
    adc_software_trigger_enable(ADC_PERIPH, ADC_ROUTINE_CHANNEL);

    u32 timeout = 100000;
    while (adc_flag_get(ADC_PERIPH, ADC_FLAG_EOC) == RESET) {
        if (--timeout == 0) return 0;
    }
    return adc_routine_data_read(ADC_PERIPH);
}

f32 adc_to_voltage(u16 adc_val)
{
    return (f32)adc_val * ADC_REF_VOLTAGE / 4095.0f;
}
