#include "display1.h"
#include "system_state.h"
#include "lcd_i2c.h"
#include <stdio.h>
#include <string.h>

/* ================================================================
 * display.c
 * Hiển thị thông tin lên LCD 16×2, sử dụng cache để giảm giao tiếp
 * I2C không cần thiết (LCD qua I2C khá chậm ở 100 kHz).
 *
 * Dòng 1 — luôn hiển thị:
 *   T:<nhiệt độ> H:<độ ẩm> F:<quạt> E:<số lần sai>
 *   Ví dụ: "T:29 H:65 F:1E2"
 *
 * Dòng 2 — chế độ bình thường:
 *   <mưa>|<sáng/tối>|<trạng thái giá>
 *   Ví dụ: "Mua|Toi|Thu"
 *
 * Dòng 2 — khi đang nhập mật khẩu:
 *   "PASS:***"
 * ================================================================ */

/* ---------- Cache 2 dòng ---------- */
char old_line1[17] = "";
char old_line2[17] = "";

/* ---------- Buffer nội bộ ---------- */
static char line1[17];
static char line2[17];

/* ----------------------------------------------------------------
 * Display_Update
 * ---------------------------------------------------------------- */
void Display_Update(uint8_t isRaining, uint8_t isBright)
{
    /* --- Dòng 1: số liệu cảm biến + trạng thái quạt + lỗi nhập --- */
    snprintf(line1, sizeof(line1), "T:%02d H:%02d F:%dE%d",
             (int)temp,
             (int)humi,
             fan_state,
             wrong_count);

    /* --- Dòng 2 --- */
    if (pass_index > 0)
    {
        /* Đang nhập mật khẩu: hiển thị dấu * */
        memset(line2, 0, sizeof(line2));
        snprintf(line2, sizeof(line2), "PASS:");

        for (uint8_t i = 0; i < pass_index && i < 4; i++)
        {
            line2[5 + i] = '*';
        }

        line2[5 + pass_index] = '\0';
    }
    else
    {
        /* Chế độ bình thường */
        const char *rain_str  = isRaining        ? "Mua"  : "Nang";
        const char *light_str = isBright         ? "Sang" : "Toi";
        const char *state_str = (currentState == STATE_PHOI) ? "Phoi" : "Thu";

        snprintf(line2, sizeof(line2), "%s|%s|%s",
                 rain_str, light_str, state_str);
    }

    /* --- Ghi LCD chỉ khi nội dung thay đổi --- */
    if (strcmp(old_line1, line1) != 0)
    {
        lcd_put_cur(0, 0);
        lcd_send_string("                ");   /* Xóa dòng trước */
        lcd_put_cur(0, 0);
        lcd_send_string(line1);
        strncpy(old_line1, line1, sizeof(old_line1));
    }

    if (strcmp(old_line2, line2) != 0)
    {
        lcd_put_cur(1, 0);
        lcd_send_string("                ");
        lcd_put_cur(1, 0);
        lcd_send_string(line2);
        strncpy(old_line2, line2, sizeof(old_line2));
    }
}
