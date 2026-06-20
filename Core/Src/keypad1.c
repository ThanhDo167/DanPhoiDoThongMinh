#include "keypad1.h"
#include "system_state.h"
#include "actuator.h"
#include "logic.h"
#include "lcd_i2c.h"
#include <string.h>

/* ================================================================
 * keypad.c
 * Quét ma trận 4×4 và xử lý toàn bộ input từ người dùng.
 * LCD được cập nhật ngay tại đây để phản hồi tức thì khi nhấn phím.
 * ================================================================ */

/* ---------- Bảng ký tự bàn phím ---------- */
static const char keymap[4][4] =
{
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

/* Khai báo tham chiếu tới old_line (để display module biết cần vẽ lại) */
extern char old_line1[17];
extern char old_line2[17];

/* ---------- Pin map ---------- */
uint16_t row_pins[4] = {GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_10, GPIO_PIN_11};
uint16_t col_pins[4] = {GPIO_PIN_12, GPIO_PIN_13, GPIO_PIN_14, GPIO_PIN_15};

/* ----------------------------------------------------------------
 * Keypad_Read
 * Kéo từng hàng xuống LOW rồi quét các cột.
 * Debounce: đọc lại sau 20 ms trước khi chấp nhận.
 * Chờ nhả phím (wait-for-release) để tránh lặp lệnh.
 * ---------------------------------------------------------------- */
char Keypad_Read(void)
{
    for (int r = 0; r < 4; r++)
    {
        /* Kéo tất cả hàng lên HIGH trước */
        for (int i = 0; i < 4; i++)
        {
            HAL_GPIO_WritePin(GPIOB, row_pins[i], GPIO_PIN_SET);
        }

        /* Kéo hàng đang xét xuống LOW */
        HAL_GPIO_WritePin(GPIOB, row_pins[r], GPIO_PIN_RESET);

        for (int c = 0; c < 4; c++)
        {
            if (HAL_GPIO_ReadPin(GPIOB, col_pins[c]) == GPIO_PIN_RESET)
            {
                HAL_Delay(20);   /* Debounce */

                if (HAL_GPIO_ReadPin(GPIOB, col_pins[c]) == GPIO_PIN_RESET)
                {
                    /* Chờ nhả phím */
                    while (HAL_GPIO_ReadPin(GPIOB, col_pins[c]) == GPIO_PIN_RESET) {}

                    HAL_GPIO_WritePin(GPIOB, row_pins[r], GPIO_PIN_SET);
                    return keymap[r][c];
                }
            }
        }

        HAL_GPIO_WritePin(GPIOB, row_pins[r], GPIO_PIN_SET);
    }

    return 0;
}

/* Thời gian giữ thông báo trên LCD để người dùng đọc kịp */
#define LCD_FLASH_HOLD_MS  1500U

/* ----------------------------------------------------------------
 * Helper: hiển thị thông báo tức thì lên LCD, giữ trong
 * LCD_FLASH_HOLD_MS để người dùng đọc kịp, rồi xóa cache display
 * để Display_Update() vẽ lại nội dung bình thường ở chu kỳ kế tiếp.
 *
 * LƯU Ý: HAL_Delay() ở đây sẽ chặn toàn bộ vòng lặp chính trong
 * 1.5 giây — bao gồm cả Stepper_Run(). Động cơ đang quay sẽ tạm
 * dừng tiến bước trong khoảng thời gian này, nhưng sẽ tự tiếp tục
 * đúng vị trí ngay khi LCD_Flash() trả về, vì s_step_count không
 * bị mất. Đây là đánh đổi hợp lý cho một thông báo ngắn người dùng
 * chủ động kích hoạt (nhấn phím), không xảy ra liên tục.
 * ---------------------------------------------------------------- */
static void LCD_Flash(const char *l1, const char *l2)
{
    lcd_clear();
    lcd_put_cur(0, 0);
    lcd_send_string(l1);

    if (l2)
    {
        lcd_put_cur(1, 0);
        lcd_send_string(l2);
    }

    /* Giữ thông báo đủ lâu để đọc được trước khi bị ghi đè */
    HAL_Delay(LCD_FLASH_HOLD_MS);

    /* Buộc display module vẽ lại ở lần cập nhật tiếp theo */
    old_line1[0] = '\0';
    old_line2[0] = '\0';
}

/* ----------------------------------------------------------------
 * Keypad_Handle
 * Xử lý phím theo bảng:
 *   1#  → Bật MANUAL mode (mở khóa A/B/C/D)
 *         BỊ TỪ CHỐI nếu đang mưa (last_isRaining = 1) — hiện cảnh báo
 *         "DANG MUA! / KHONG VAO MANUAL" và giữ nguyên AUTO mode.
 *   0#  → Thoát MANUAL mode, quay về AUTO (tính lại targetState ngay)
 *   A   → (chỉ khi manual) Thu vào — BẮT BUỘC thu, set targetState
 *         ngay lập tức, KHÔNG bị huỷ hay chuyển ngược về AUTO.
 *   B   → (chỉ khi manual) Phơi ra
 *         BỊ TỪ CHỐI nếu đang mưa, NGAY CẢ KHI đã ở MANUAL từ trước
 *         (ví dụ vào manual lúc trời chưa mưa, rồi mưa mới bắt đầu).
 *         Hiện cảnh báo "DANG MUA! / KHONG THE PHOI".
 *   C   → (chỉ khi manual) Bật quạt
 *   D   → (chỉ khi manual) Tắt quạt
 *   0–9 → Nhập mã (tối đa 4 ký tự)
 *   *   → Xóa mã đang nhập
 *   #   → Xác nhận mã đã nhập:
 *           "1"    → Bật MANUAL mode (trừ khi đang mưa)
 *           "0"    → Thoát MANUAL mode (về AUTO)
 *           "1234" → Tắt còi — LUÔN hoạt động, không bị chặn bởi mưa
 *           Sai mã → tăng wrong_count; đủ MAX → khóa hệ thống
 *
 * MANUAL LÀ QUYỀN CAO NHẤT: một khi đã vào MANUAL (1#), hệ thống sẽ
 * KHÔNG tự ý quay về AUTO theo thời gian (không có timeout). Lệnh
 * Thu/Phơi/Quạt do người dùng đặt trong MANUAL sẽ được giữ nguyên
 * cho tới khi người dùng chủ động thoát bằng 0#. Ngoại lệ duy nhất:
 * khi đang mưa, hệ thống vẫn ép giá về STATE_THU để bảo vệ quần áo
 * (an toàn vật lý được ưu tiên trên cả MANUAL), nhưng việc này KHÔNG
 * làm tắt cờ manual_mode — người dùng vẫn đang ở MANUAL, chỉ là
 * targetState bị ép về THU.
 * ---------------------------------------------------------------- */
void Keypad_Handle(void)
{
    char key = Keypad_Read();

    if (!key)
    {
        return;
    }

    HAL_Delay(150);   /* Chống rung thêm sau khi nhả */

    /* Hệ thống đang bị khóa: chỉ hiển thị thông báo, bỏ qua mọi phím */
    if (system_locked)
    {
        LCD_Flash("BI KHOA!", NULL);
        return;
    }

    /* --- Phím chức năng — chỉ hoạt động khi đã ở MANUAL mode --- */
    if (key == 'A')
    {
        if (!manual_mode)
        {
            LCD_Flash("NHAP 1# TRUOC", "DE VAO MANUAL");
        }
        else
        {
            targetState = STATE_THU;
            LCD_Flash("MANUAL MODE", "DANG THU VAO");
        }
    }
    else if (key == 'B')
    {
        if (!manual_mode)
        {
            LCD_Flash("NHAP 1# TRUOC", "DE VAO MANUAL");
        }
        else if (last_isRaining)
        {
            /* Đã ở MANUAL từ trước khi mưa bắt đầu — vẫn chặn việc
             * cố tình đưa đồ ra ngoài lúc trời đang mưa, dù người
             * dùng đang ở chế độ thủ công. */
            LCD_Flash("DANG MUA!", "KHONG THE PHOI");
        }
        else
        {
            targetState = STATE_PHOI;
            LCD_Flash("MANUAL MODE", "DANG PHOI RA");
        }
    }
    else if (key == 'C')
    {
        if (!manual_mode)
        {
            LCD_Flash("NHAP 1# TRUOC", "DE VAO MANUAL");
        }
        else
        {
            fan_manual = 1;
            Actuator_SetFan(1);
            LCD_Flash("QUAT: ON", NULL);
        }
    }
    else if (key == 'D')
    {
        if (!manual_mode)
        {
            LCD_Flash("NHAP 1# TRUOC", "DE VAO MANUAL");
        }
        else
        {
            fan_manual = 1;
            Actuator_SetFan(0);
            LCD_Flash("QUAT: OFF", NULL);
        }
    }
    /* --- Nhập mã số (dùng chung cho cả "1" và "1234") --- */
    else if (key >= '0' && key <= '9')
    {
        if (pass_index < 4)
        {
            input_pass[pass_index++] = key;
            input_pass[pass_index]   = '\0';
        }
    }
    else if (key == '*')
    {
        /* Xóa toàn bộ mã đang nhập */
        pass_index = 0;
        memset(input_pass, 0, sizeof(input_pass));
    }
    else if (key == '#')
    {
        /* --- Trường hợp 1: mã "1" → bật MANUAL mode ---
         * NHƯNG: nếu đang mưa, từ chối vào MANUAL để bảo vệ quần áo.
         * Người dùng vẫn phải chờ hết mưa mới có thể chuyển sang
         * thủ công và tự ý kéo đồ ra ngoài. */
        if (strcmp(input_pass, MANUAL_ENTER_CODE) == 0)
        {
            if (last_isRaining)
            {
                LCD_Flash("DANG MUA!", "KHONG VAO MANUAL");
            }
            else
            {
                manual_mode = 1;
                LCD_Flash("MANUAL MODE", "AN A/B/C/D");
            }
        }
        /* --- Trường hợp 2: mã "0" → thoát MANUAL, về AUTO --- */
        else if (strcmp(input_pass, MANUAL_EXIT_CODE) == 0)
        {
            manual_mode = 0;
            fan_manual  = 0;

            /* Tính lại targetState NGAY bằng dữ liệu cảm biến gần nhất,
             * tránh kẹt ở trạng thái cũ do người dùng đặt thủ công
             * trước đó (cùng nguyên nhân bug đã sửa trước). */
            Logic_UpdateRackState(last_isRaining, last_isBright, humi);

            LCD_Flash("AUTO MODE", "DA KICH HOAT");
        }
        /* --- Trường hợp 3: mã "1234" → tắt còi, KHÔNG đổi manual_mode --- */
        else if (strcmp(input_pass, CORRECT_PASS) == 0)
        {
            wrong_count = 0;
            buzzer_off  = 1;
            LCD_Flash("DA TAT COI", NULL);
        }
        /* --- Trường hợp 4: mã sai --- */
        else
        {
            wrong_count++;
            LCD_Flash("SAI MA SO", NULL);

            if (wrong_count >= MAX_WRONG_PASS)
            {
                system_locked = 1;
                /* Tắt còi khi khóa để tránh tiếng kêu liên tục */
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
            }
        }

        pass_index = 0;
        memset(input_pass, 0, sizeof(input_pass));
    }
}
