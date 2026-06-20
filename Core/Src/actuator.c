#include "actuator.h"
#include "system_state.h"

/* ================================================================
 * actuator.c
 * Lớp duy nhất được phép ghi GPIO relay và buzzer.
 * Logic bật/tắt đơn giản — quyết định đến từ module logic.
 * ================================================================ */

/* ----------------------------------------------------------------
 * Actuator_SetFan
 * Bật/tắt relay điều khiển quạt và đồng bộ biến fan_state.
 * ---------------------------------------------------------------- */
void Actuator_SetFan(uint8_t state)
{
    fan_state = state ? 1U : 0U;
    HAL_GPIO_WritePin(RELAY_PORT, RELAY_PIN,
                      fan_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* ----------------------------------------------------------------
 * Actuator_HandleBuzzer
 * Quy tắc điều khiển còi:
 *   - Nếu hệ thống đang bị khóa (system_locked) → tắt còi hoàn toàn.
 *   - Nếu đang mưa VÀ người dùng chưa tắt tiếng (buzzer_off = 0) → kêu.
 *   - Ngược lại → tắt còi.
 *   - Khi hết mưa: reset cờ buzzer_off để lần mưa sau còi hoạt động lại.
 * ---------------------------------------------------------------- */
void Actuator_HandleBuzzer(uint8_t isRaining)
{
    if (system_locked)
    {
        HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
        return;
    }

    if (isRaining && !buzzer_off)
    {
        HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
    }

    /* Reset cờ tắt tiếng khi trời tạnh để còi hoạt động lại lần sau */
    if (!isRaining)
    {
        buzzer_off = 0;
    }
}
