//----------------------------------------------------------------------------------------------------------------
//    This file was generated on the website https://vip-cxema.org/
//    Program version: 1.2.0_03.06.2025
//    File last modified: 20:23 05.11.2025
//----------------------------------------------------------------------------------------------------------------
//    Project home       https://github.com/e2002/yoradio
//    Wiki               https://github.com/e2002/yoradio/wiki
//    Описание на 4PDA   https://4pda.to/forum/index.php?s=&showtopic=1010378&view=findpost&p=112992611
//    Как это прошить?   https://4pda.to/forum/index.php?act=findpost&pid=112992611&anchor=Spoil-112992611-2
//----------------------------------------------------------------------------------------------------------------
#ifndef _my_theme_h
#define _my_theme_h
//----------------------------------------------------------------------------------------------------------------
//    Theming of color displays
//    DSP_ST7735, DSP_ST7789, DSP_ILI9341, DSP_GC9106, DSP_ILI9225, DSP_ST7789_240
//----------------------------------------------------------------------------------------------------------------
//    *    !!! This file must be in the root directory of the sketch !!!    *
//----------------------------------------------------------------------------------------------------------------
//    Uncomment (remove double slash //) from desired line to apply color
//----------------------------------------------------------------------------------------------------------------
#define ENABLE_THEME
#ifdef  ENABLE_THEME
/*----------------------------------------------------------------------------------------------------------------*/
/*       | COLORS             |   values (0-255)  |                                                               */
/*       | color name         |    R    G    B    |                                                               */
/*----------------------------------------------------------------------------------------------------------------*/
#define COLOR_BACKGROUND        0,0,0     /*  background                                                  */
#define COLOR_STATION_NAME        0,0,0     /*  station name                                                */
#define COLOR_STATION_BG         184,121,87     /*  station name background                                     */
#define COLOR_STATION_FILL       184,121,87     /*  station name fill background                                */
#define COLOR_SNG_TITLE_1       255,255,255     /*  first title                                                 */
#define COLOR_SNG_TITLE_2         227,227,227    /*  second title                                                */
#define COLOR_WEATHER           255,126,38     /*  weather string                                              */
#define COLOR_AI_INTERPRETATION  255,200,100    /*  AI interpretation / AI интерпретация                      */
#define COLOR_VU_MAX            255,0,0     /*  max of VU meter                                             */
#define COLOR_VU_MIN            0,255,6     /*  min of VU meter                                             */
#define COLOR_CLOCK              255,255,255     /*  clock color                                                 */
#define COLOR_CLOCK_BG           0,0,0     /*  clock color background                                      */
#define COLOR_SECONDS             236,28,35     /*  seconds color (DSP_ST7789, DSP_ILI9341, DSP_ILI9225)        */
#define COLOR_DAY_OF_W          255,255,255     /*  day of week color (DSP_ST7789, DSP_ILI9341, DSP_ILI9225)    */
#define COLOR_DATE                178,178,178     /*  date color (DSP_ST7789, DSP_ILI9341, DSP_ILI9225)           */
#define COLOR_HEAP              255,168,162     /*  heap string                                                 */
#define COLOR_BUFFER            3,255,0     /*  buffer line                                                 */
#define COLOR_IP                 41,189,207    /*  ip address                                                  */
#define COLOR_VOLUME_VALUE      41,189,207     /*  volume string (DSP_ST7789, DSP_ILI9341, DSP_ILI9225)        */
#define COLOR_RSSI              254,251,0     /*  rssi                                                        */
#define COLOR_VOLBAR_OUT        179,0,27     /*  volume bar outline                                          */
#define COLOR_VOLBAR_IN         179,0,27     /*  volume bar fill                                             */
#define COLOR_DIGITS            255,255,255     /*  volume / station number                                     */
#define COLOR_DIVIDER             255,255,0     /*  divider color (DSP_ST7789, DSP_ILI9341, DSP_ILI9225)        */
#define COLOR_BITRATE           255,255,0     /*  bitrate                                                     */
#define COLOR_PL_CURRENT          0,0,0     /*  playlist current item                                       */
#define COLOR_PL_CURRENT_BG      231,211,90     /*  playlist current item background                            */
#define COLOR_PL_CURRENT_FILL    231,211,90     /*  playlist current item fill background                       */
#define COLOR_PLAYLIST_0        255,255,255     /*  playlist string 0                                           */
#define COLOR_PLAYLIST_1        170,170,170     /*  playlist string 1                                           */
#define COLOR_PLAYLIST_2        140,140,140     /*  playlist string 2                                           */
#define COLOR_PLAYLIST_3          90,90,90     /*  playlist string 3                                           */
#define COLOR_PLAYLIST_4          60,60,60     /*  playlist string 4                                           */


#endif  /* #ifdef  ENABLE_THEME */
#endif  /* #define _my_theme_h  */
