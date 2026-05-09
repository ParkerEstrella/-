#ifndef ADC_DRIVER_H
#define ADC_DRIVER_H

#include "common_types.h"
#include "filter.h"

void adc_module_init(void);
u16 adc_read_single(void);
f32 adc_to_voltage(u16 adc_val);

#endif /* ADC_DRIVER_H */
