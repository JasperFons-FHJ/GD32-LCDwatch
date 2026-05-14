#include "board.h"
#include "spi.h"
#include "systick.h"
#include "tftlcd.h"

#define WATCH_BG 0x0008U
#define WATCH_PANEL 0x010FU
#define WATCH_PANEL_DARK 0x0004U
#define WATCH_LINE 0x045FU
#define WATCH_ACCENT 0x07FFU
#define WATCH_TEXT 0xFFFFU
#define WATCH_MUTED 0x7D7CU

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t weekday;
} watch_datetime_t;

static uint8_t parse_2_digits(const char *text)
{
    uint8_t high = (text[0] == ' ') ? 0U : (uint8_t)(text[0] - '0');
    uint8_t low = (uint8_t)(text[1] - '0');

    return (uint8_t)(high * 10U + low);
}

static uint16_t parse_4_digits(const char *text)
{
    return (uint16_t)(((uint16_t)(text[0] - '0') * 1000U) +
                      ((uint16_t)(text[1] - '0') * 100U) +
                      ((uint16_t)(text[2] - '0') * 10U) +
                      (uint16_t)(text[3] - '0'));
}

static uint8_t build_month(const char *date)
{
    static const char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

    for (uint8_t month = 0U; month < 12U; month++) {
        const char *name = &months[month * 3U];

        if ((date[0] == name[0]) && (date[1] == name[1]) && (date[2] == name[2])) {
            return (uint8_t)(month + 1U);
        }
    }

    return 1U;
}

static uint8_t is_leap_year(uint16_t year)
{
    if ((year % 400U) == 0U) {
        return 1U;
    }

    if ((year % 100U) == 0U) {
        return 0U;
    }

    return (uint8_t)((year % 4U) == 0U);
}

static uint8_t days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t days[] = {31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U};

    if (month == 2U) {
        return (uint8_t)(28U + is_leap_year(year));
    }

    return days[month - 1U];
}

static uint8_t weekday_from_date(uint16_t year, uint8_t month, uint8_t day)
{
    static const uint8_t offsets[] = {0U, 3U, 2U, 5U, 0U, 3U, 5U, 1U, 4U, 6U, 2U, 4U};
    uint16_t y = year;

    if (month < 3U) {
        y--;
    }

    return (uint8_t)((y + (y / 4U) - (y / 100U) + (y / 400U) + offsets[month - 1U] + day) % 7U);
}

static void watch_time_init(watch_datetime_t *time)
{
    const char *date = __DATE__;
    const char *clock = __TIME__;

    time->month = build_month(date);
    time->day = parse_2_digits(&date[4]);
    time->year = parse_4_digits(&date[7]);
    time->hour = parse_2_digits(&clock[0]);
    time->minute = parse_2_digits(&clock[3]);
    time->second = parse_2_digits(&clock[6]);
    time->weekday = weekday_from_date(time->year, time->month, time->day);
}

static void watch_time_tick(watch_datetime_t *time)
{
    time->second++;
    if (time->second < 60U) {
        return;
    }

    time->second = 0U;
    time->minute++;
    if (time->minute < 60U) {
        return;
    }

    time->minute = 0U;
    time->hour++;
    if (time->hour < 24U) {
        return;
    }

    time->hour = 0U;
    time->weekday = (uint8_t)((time->weekday + 1U) % 7U);
    time->day++;

    if (time->day <= days_in_month(time->year, time->month)) {
        return;
    }

    time->day = 1U;
    time->month++;

    if (time->month <= 12U) {
        return;
    }

    time->month = 1U;
    time->year++;
}

static void two_digits(char *text, uint8_t value)
{
    text[0] = (char)('0' + (value / 10U));
    text[1] = (char)('0' + (value % 10U));
}

static void four_digits(char *text, uint16_t value)
{
    text[0] = (char)('0' + ((value / 1000U) % 10U));
    text[1] = (char)('0' + ((value / 100U) % 10U));
    text[2] = (char)('0' + ((value / 10U) % 10U));
    text[3] = (char)('0' + (value % 10U));
}

static void format_time(char *text, const watch_datetime_t *time)
{
    two_digits(&text[0], time->hour);
    text[2] = ':';
    two_digits(&text[3], time->minute);
    text[5] = '\0';
}

static void format_seconds(char *text, const watch_datetime_t *time)
{
    two_digits(&text[0], time->second);
    text[2] = '\0';
}

static void format_date(char *text, const watch_datetime_t *time)
{
    four_digits(&text[0], time->year);
    text[4] = '-';
    two_digits(&text[5], time->month);
    text[7] = '-';
    two_digits(&text[8], time->day);
    text[10] = '\0';
}

static const char *weekday_name(uint8_t weekday)
{
    static const char *const names[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

    return names[weekday % 7U];
}

static void watch_fill_screen(uint16_t color)
{
    LCD_Fill(0U, 0U, LCD_Width - 1U, LCD_Height - 1U, color);
}

static void watch_draw_frame(void)
{
    watch_fill_screen(WATCH_BG);

    LCD_Fill(10U, 10U, 309U, 161U, WATCH_PANEL_DARK);
    LCD_Fill(17U, 17U, 302U, 154U, WATCH_PANEL);

    POINT_COLOR = WATCH_LINE;
    LCD_DrawRectangle(10U, 10U, 309U, 161U);
    LCD_DrawRectangle(17U, 17U, 302U, 154U);

    POINT_COLOR = WATCH_ACCENT;
    LCD_DrawLine(31U, 52U, 289U, 52U);
    LCD_DrawLine(31U, 129U, 289U, 129U);
    LCD_Fill(287U, 27U, 296U, 36U, WATCH_ACCENT);

    BACK_COLOR = WATCH_PANEL;
    POINT_COLOR = WATCH_MUTED;
    LCD_ShowString(31U, 27U, 120U, 16U, 16U, "HELLO JASPER");

    POINT_COLOR = WATCH_ACCENT;
    LCD_ShowString(251U, 29U, 36U, 12U, 12U, "LIVE");
}

static void watch_draw_time(const watch_datetime_t *time)
{
    char clock_text[6];
    char second_text[3];
    char date_text[11];

    format_time(clock_text, time);
    format_seconds(second_text, time);
    format_date(date_text, time);

    LCD_Fill(60U, 63U, 260U, 111U, WATCH_PANEL);
    BACK_COLOR = WATCH_PANEL;
    POINT_COLOR = WATCH_TEXT;
    LCD_ShowString(102U, 70U, 88U, 32U, 32U, clock_text);

    POINT_COLOR = WATCH_ACCENT;
    LCD_ShowString(194U, 78U, 28U, 24U, 24U, second_text);

    LCD_Fill(55U, 138U, 265U, 150U, WATCH_PANEL);
    POINT_COLOR = WATCH_ACCENT;
    LCD_ShowString(98U, 139U, 24U, 12U, 12U, (char *)weekday_name(time->weekday));

    POINT_COLOR = WATCH_MUTED;
    LCD_ShowString(182U, 139U, 72U, 12U, 12U, date_text);
}

int main(void)
{
    watch_datetime_t time;
    uint32_t last_tick;

    systick_config();
    board_init();
    spi_lcd_init();
    LCD_Init();

    watch_time_init(&time);
    watch_draw_frame();
    watch_draw_time(&time);
    last_tick = systick_millis();

    board_uart_write("GD32F303RCT6 GCC firmware ready\r\n");
    board_uart_write("LCD watch UI: HELLO JASPER\r\n");

    while (1) {
        uint32_t now = systick_millis();

        if ((uint32_t)(now - last_tick) >= 1000U) {
            last_tick += 1000U;
            watch_time_tick(&time);
            watch_draw_time(&time);
            board_led_toggle(BOARD_LED_BLUE);
        }
    }
}
