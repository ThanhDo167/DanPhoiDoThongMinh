#include "uart_cli.h"
#include "system_state.h"
#include "logic.h"
#include "actuator.h"
#include <stdio.h>
#include <string.h>

/* ================================================================
 * uart_cli.c  —  UART Monitor (read-only, no flicker)
 *
 * Chỉ hiển thị — KHÔNG nhận lệnh từ PC.
 * Điều khiển qua keypad vật lý.
 *
 * Nguyên tắc chống nhấp nháy:
 *   1. draw_skeleton() gọi \033[2J DUY NHẤT 1 lần lúc init.
 *   2. Mỗi giây update_values() chỉ di chuyển con trỏ tới đúng ô
 *      giá trị rồi overwrite — KHÔNG đụng vào khung hay nhãn.
 *   3. Con trỏ bị ẩn vĩnh viễn (\033[?25l) ngay lúc init.
 *   4. Sau mỗi update con trỏ về row 18 col 1 (dưới dashboard).
 * ================================================================ */

extern UART_HandleTypeDef huart1;

/* ── ANSI escape codes ─────────────────────────────────────────── */
#define ESC          "\033["
#define A_RST        "\033[0m"
#define A_BOLD       "\033[1m"
#define A_RED        "\033[31m"
#define A_GREEN      "\033[32m"
#define A_YELLOW     "\033[33m"
#define A_CYAN       "\033[36m"
#define A_WHITE      "\033[97m"
#define A_GRAY       "\033[90m"
#define A_CURSOR_OFF "\033[?25l"
#define A_ERASE_EOL  "\033[K"

/* ── Layout (cột 1-based, dòng 1-based) ───────────────────────── *
 *
 *  1  ┌──────────────────────────────────────┐
 *  2  │   SMART DRY RACK - GIAM SAT HE THONG │
 *  3  ├──────────── CAM BIEN ─────────────────┤
 *  4  │ Nhiet do  : [val]                     │
 *  5  │ Do am     : [val]                     │
 *  6  │ Anh sang  : [val]                     │
 *  7  │ Mua       : [val]                     │
 *  8  ├──────────── TRANG THAI ───────────────┤
 *  9  │ Che do    : [val]                     │
 * 10  │ Gia phoi  : [val]                     │
 * 11  │ Quat      : [val]                     │
 * 12  │ He thong  : [val]                     │
 * 13  │ Sai ma    : [val]                     │
 * 14  ├──────────── CANH BAO ─────────────────┤
 * 15  │ [val canh bao]                        │
 * 16  └──────────────────────────────────────┘
 * 17  (con trỏ đậu đây — ngoài khung)         */

#define ROW_NHIETDO  4
#define ROW_DOAM     5
#define ROW_ANGSANG  6
#define ROW_MUA      7
#define ROW_CHEDO    9
#define ROW_GIAPHOI  10
#define ROW_QUAT     11
#define ROW_HETHONG  12
#define ROW_SAIMA    13
#define ROW_CAOBAO   15
#define ROW_PARK     17   /* con trỏ "đậu xe" sau update */

/* Cột đầu tiên của vùng GIÁ TRỊ (sau nhãn "│ Nhiet do  : ") */
#define COL_VAL      16

/* Chiều rộng vùng giá trị (đủ ghi đè nội dung cũ dài hơn) */
#define VAL_WIDTH    30

/* ── Hai buffer riêng biệt tránh clobber ──────────────────────── */
static char s_move[32];   /* chỉ chứa escape di chuyển con trỏ */
static char s_val[96];    /* chứa chuỗi giá trị + màu           */

/* ================================================================
 * Gửi UART
 * ================================================================ */
void UART_CLI_Print(const char *str)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)str,
                      (uint16_t)strlen(str), 200);
}

void UART_CLI_Println(const char *str)
{
    UART_CLI_Print(str);
    UART_CLI_Print("\r\n");
}

/* Di chuyển con trỏ tới (row, col) */
static inline void goto_rc(uint8_t row, uint8_t col)
{
    snprintf(s_move, sizeof(s_move), "\033[%d;%dH", row, col);
    UART_CLI_Print(s_move);
}

/* Ghi đè giá trị tại (row, COL_VAL):
 *   - Nhảy tới đúng vị trí
 *   - In nội dung s_val (đã format + màu bên ngoài)
 *   - Xóa phần thừa đến cuối dòng (A_ERASE_EOL)
 *   - Reset màu
 * KHÔNG dùng chung buffer s_val trong hàm này.               */
static void write_val(uint8_t row)
{
    goto_rc(row, COL_VAL);
    UART_CLI_Print(s_val);
    UART_CLI_Print(A_ERASE_EOL);
    UART_CLI_Print(A_RST);
}

/* ================================================================
 * draw_skeleton — gọi DUY NHẤT 1 lần trong UART_CLI_Init()
 * Vẽ toàn bộ khung + nhãn tĩnh.
 * ================================================================ */
static void draw_skeleton(void)
{
    /* Xóa màn hình, về góc trên trái, ẩn con trỏ */
    UART_CLI_Print("\033[2J\033[H" A_CURSOR_OFF);

    UART_CLI_Println(A_CYAN  "+--------------------------------------+" A_RST);
    UART_CLI_Println(A_BOLD A_CYAN
                             "| SMART DRY RACK - GIAM SAT HE THONG   |" A_RST);
    UART_CLI_Println(A_CYAN  "+------------- CAM BIEN ---------------+" A_RST);
    UART_CLI_Println(A_WHITE "| Nhiet do  :                          |" A_RST);
    UART_CLI_Println(A_WHITE "| Do am     :                          |" A_RST);
    UART_CLI_Println(A_WHITE "| Anh sang  :                          |" A_RST);
    UART_CLI_Println(A_WHITE "| Mua       :                          |" A_RST);
    UART_CLI_Println(A_CYAN  "+------------ TRANG THAI --------------+" A_RST);
    UART_CLI_Println(A_WHITE "| Che do    :                          |" A_RST);
    UART_CLI_Println(A_WHITE "| Gia phoi  :                          |" A_RST);
    UART_CLI_Println(A_WHITE "| Quat      :                          |" A_RST);
    UART_CLI_Println(A_WHITE "| He thong  :                          |" A_RST);
    UART_CLI_Println(A_WHITE "| Sai ma    :                          |" A_RST);
    UART_CLI_Println(A_CYAN  "+------------ CANH BAO ----------------+" A_RST);
    UART_CLI_Println(A_WHITE "|                                      |" A_RST);
    UART_CLI_Println(A_CYAN  "+--------------------------------------+" A_RST);
}

/* ================================================================
 * update_values — gọi mỗi 1 giây
 * Chỉ overwrite vùng giá trị — KHÔNG đụng khung/nhãn.
 * ================================================================ */
static void update_values(void)
{
    /* ── Cảm biến ─────────────────────────────────────────────── */

    /* Nhiet do */
    snprintf(s_val, sizeof(s_val),
             A_YELLOW "%5.1f" A_WHITE " C", (double)temp);
    write_val(ROW_NHIETDO);

    /* Do am */
    snprintf(s_val, sizeof(s_val),
             A_YELLOW "%5.1f" A_WHITE " %%", (double)humi);
    write_val(ROW_DOAM);

    /* Anh sang */
    snprintf(s_val, sizeof(s_val),
             "%s%-4s" A_GRAY " (ADC:%4u)",
             last_isBright ? A_YELLOW : A_GRAY,
             last_isBright ? "SANG" : "TOI",
             adcValue);
    write_val(ROW_ANGSANG);

    /* Mua */
    snprintf(s_val, sizeof(s_val),
             "%s%s",
             last_isRaining ? A_RED : A_GREEN,
             last_isRaining ? "DANG MUA  " : "KHONG MUA ");
    write_val(ROW_MUA);

    /* ── Trang thai ───────────────────────────────────────────── */

    /* Che do */
    snprintf(s_val, sizeof(s_val),
             "%s%s",
             manual_mode ? A_YELLOW : A_GREEN,
             manual_mode ? "MANUAL" : "AUTO  ");
    write_val(ROW_CHEDO);

    /* Gia phoi */
    {
        uint8_t moving = (currentState != targetState);
        const char *color = moving ? A_YELLOW : A_GREEN;
        const char *label;
        if (!moving)
            label = (currentState == STATE_PHOI) ? "PHOI RA    " : "THU VAO    ";
        else
            label = (targetState == STATE_PHOI)  ? "DANG PHOI.." : "DANG THU.. ";
        snprintf(s_val, sizeof(s_val), "%s%s", color, label);
        write_val(ROW_GIAPHOI);
    }

    /* Quat */
    snprintf(s_val, sizeof(s_val),
             "%s%-3s" A_GRAY " (%s)",
             fan_state ? A_GREEN : A_GRAY,
             fan_state ? "ON" : "OFF",
             fan_manual ? "Manual" : "Auto  ");
    write_val(ROW_QUAT);

    /* He thong */
    snprintf(s_val, sizeof(s_val),
             "%s%s",
             system_locked ? A_RED : A_GREEN,
             system_locked ? "BI KHOA !" : "HOAT DONG");
    write_val(ROW_HETHONG);

    /* Sai ma */
    snprintf(s_val, sizeof(s_val),
             A_YELLOW "%d" A_WHITE "/%d lan",
             wrong_count, MAX_WRONG_PASS);
    write_val(ROW_SAIMA);

    /* ── Canh bao ─────────────────────────────────────────────── */
    goto_rc(ROW_CAOBAO, 3);

    if (last_isRaining)
    {
        /* Chớp bold/normal mỗi giây mà không xóa màn hình */
        uint32_t s = HAL_GetTick() / 1000u;
        UART_CLI_Print((s & 1u) ? A_BOLD A_RED : A_RED);
        UART_CLI_Print("[!] DANG MUA - TU DONG THU DO VAO");
    }
    else if (system_locked)
    {
        UART_CLI_Print(A_RED "[!] BI KHOA - Nhap dung ma de mo");
    }
    else
    {
        UART_CLI_Print(A_GRAY "Khong co canh bao              ");
    }

    UART_CLI_Print(A_ERASE_EOL A_RST);

    /* Đưa con trỏ về vùng trống dưới dashboard — không hiện chữ */
    goto_rc(ROW_PARK, 1);
}

/* ================================================================
 * API công khai
 * ================================================================ */

void UART_CLI_Init(void)
{
    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = 115200;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX;     /* TX only, không cần RX */
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart1) != HAL_OK) { Error_Handler(); }

    draw_skeleton();   /* khung + nhãn — 1 lần duy nhất */
    update_values();   /* điền giá trị lần đầu ngay lập tức */
}

void UART_CLI_Update(void)
{
    update_values();   /* gọi mỗi 1 giây từ main() */
}
