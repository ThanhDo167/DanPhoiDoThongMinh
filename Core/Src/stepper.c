#include "stepper.h"

/* ================================================================
 * stepper.c
 * Non-blocking stepper driver.
 * Stepper_Run() được gọi liên tục trong while(1), mỗi lần chỉ
 * tiến 1 bước nếu đã đủ thời gian STEPPER_DELAY tính bằng tick.
 * ================================================================ */

/* ---------- Chuỗi half-step 8 bước ----------
 * Mỗi hàng = trạng thái [IN1, IN2, IN3, IN4].
 * Half-step cho mô-men xoắn đều hơn full-step và ít rung hơn.
 * ---------------------------------------------------------------- */
static const uint8_t step_seq[8][4] =
{
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1}
};

/* ---------- Biến nội bộ (static = chỉ nhìn thấy trong file này) ---------- */
static int      s_step_index    = 0;   /* Vị trí trong bảng half-step  */
static int      s_step_dir      = 1;   /* +1 hoặc -1                   */
static int      s_step_count    = 0;   /* Số bước đã thực hiện         */
static int      s_step_target   = 0;   /* Tổng số bước cần thực hiện   */
static uint32_t s_last_tick     = 0;   /* Tick lần bước trước          */
static uint8_t  s_motor_running = 0;   /* Cờ đang chạy                 */

/* ----------------------------------------------------------------
 * Stepper_Write (nội bộ)
 * Ghi trực tiếp trạng thái logic lên 4 chân IN1–IN4.
 * ---------------------------------------------------------------- */
static void Stepper_Write(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
    HAL_GPIO_WritePin(IN1_PORT, IN1_PIN, a ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IN2_PORT, IN2_PIN, b ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IN3_PORT, IN3_PIN, c ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IN4_PORT, IN4_PIN, d ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* ----------------------------------------------------------------
 * Stepper_Start
 * Đặt thông số và kích hoạt cờ chạy. Không tự chạy — cần gọi
 * Stepper_Run() trong vòng lặp chính.
 * ---------------------------------------------------------------- */
void Stepper_Start(int steps, int dir)
{
    s_step_target   = steps;
    s_step_count    = 0;
    s_step_dir      = dir;
    s_motor_running = 1;
}

/* ----------------------------------------------------------------
 * Stepper_Run
 * Gọi mỗi vòng while(1). Nếu đã đủ STEPPER_DELAY ms kể từ bước
 * trước, ghi bảng half-step tiếp theo rồi tăng chỉ số.
 * Khi đủ số bước, tự tắt điện và hạ cờ.
 * ---------------------------------------------------------------- */
void Stepper_Run(void)
{
    if (!s_motor_running)
    {
        return;
    }

    if (s_step_count >= s_step_target)
    {
        Stepper_PowerOff();
        s_motor_running = 0;
        return;
    }

    if (HAL_GetTick() - s_last_tick >= STEPPER_DELAY)
    {
        s_last_tick = HAL_GetTick();

        Stepper_Write(
            step_seq[s_step_index][0],
            step_seq[s_step_index][1],
            step_seq[s_step_index][2],
            step_seq[s_step_index][3]
        );

        s_step_index += s_step_dir;

        /* Wrap vòng trong [0, 7] */
        if (s_step_index > 7) { s_step_index = 0; }
        if (s_step_index < 0) { s_step_index = 7; }

        s_step_count++;
    }
}

/* ----------------------------------------------------------------
 * Stepper_IsBusy
 * ---------------------------------------------------------------- */
uint8_t Stepper_IsBusy(void)
{
    return s_motor_running;
}

/* ----------------------------------------------------------------
 * Stepper_PowerOff
 * Tắt tất cả cuộn dây để tránh nóng khi đứng yên.
 * ---------------------------------------------------------------- */
void Stepper_PowerOff(void)
{
    Stepper_Write(0, 0, 0, 0);
}
