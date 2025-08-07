#ifndef COMPONENTS_PERFMON_INCLUDE_PERFMON_H_
#define COMPONENTS_PERFMON_INCLUDE_PERFMON_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

esp_err_t perfmon_start();
uint32_t perfmon_get_cpu_usage(uint8_t core);

#ifdef __cplusplus
}
#endif

#endif /* COMPONENTS_PERFMON_INCLUDE_PERFMON_H_ */
