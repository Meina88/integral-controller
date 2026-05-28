#include "calibration.h"
#include "storage/nvs/storage_nvs.h"

#define CAL_FACTOR_MIN 0.8f
#define CAL_FACTOR_MAX 1.2f

static float s_factor = 1.0f;

void calibration_init(void)
{
    storage_nvs_load_cal_factor(&s_factor);
    if (s_factor < CAL_FACTOR_MIN) s_factor = CAL_FACTOR_MIN;
    if (s_factor > CAL_FACTOR_MAX) s_factor = CAL_FACTOR_MAX;
}

float calibration_get_factor(void)
{
    return s_factor;
}

void calibration_set_factor(float factor)
{
    if (factor < CAL_FACTOR_MIN) factor = CAL_FACTOR_MIN;
    if (factor > CAL_FACTOR_MAX) factor = CAL_FACTOR_MAX;
    s_factor = factor;
    storage_nvs_save_cal_factor(factor);
}
