#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0

#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (96U * 1024U)

#define LV_DISP_DEF_REFR_PERIOD 16
#define LV_INDEV_DEF_READ_PERIOD 30

#define LV_DPI_DEF 160

#define LV_USE_LOG 0
#define LV_USE_ASSERT_NULL 1

#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0

#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_22 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_48 1
#define LV_FONT_CUSTOM_DECLARE \
  LV_FONT_DECLARE(bike_mb_font_speed_140); \
  LV_FONT_DECLARE(bike_mb_font_speed_decimal_96); \
  LV_FONT_DECLARE(bike_mb_font_output_80)

#endif
