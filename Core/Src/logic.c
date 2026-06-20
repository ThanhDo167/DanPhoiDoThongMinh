#include "logic.h"
#include "system_state.h"
#include "actuator.h"

/* ================================================================
 * logic.c
 * ================================================================ */

/* ---------- Thời gian quạt chạy thêm sau khi hết mưa ---------- */
#define FAN_POST_RAIN_MS  10000U   /* 10 giây */

/* Cờ nội bộ: đang ở giai đoạn "đếm 10s sau khi hết mưa" để tắt quạt */
static uint8_t  s_fan_cooldown_active = 0;

/* ----------------------------------------------------------------
 * Logic_UpdateRackState
 *
 * THAY ĐỔI: Mưa giờ là điều kiện ƯU TIÊN TUYỆT ĐỐI — kiểm tra và xử
 * lý TRƯỚC khi xét đến manual_mode. Nếu đang mưa, targetState luôn
 * bị ép về STATE_THU bất kể đang ở AUTO hay MANUAL, để bảo vệ quần
 * áo. Chỉ khi KHÔNG mưa thì manual_mode mới được quyền giữ nguyên
 * targetState do người dùng tự đặt (không bị AUTO ghi đè).
 * ---------------------------------------------------------------- */
void Logic_UpdateRackState(uint8_t isRaining, uint8_t isBright, float humidity)
{
    /* Mưa luôn thắng, không quan tâm manual_mode */
    if (isRaining)
    {
        targetState = STATE_THU;
        return;
    }

    /* Không mưa: nếu đang MANUAL, để người dùng tự quyết định */
    if (manual_mode)
    {
        return;
    }

    /* AUTO mode, không mưa: xét độ ẩm và ánh sáng */
    uint8_t humi_too_high = (humidity > 0.0f && humidity > 80.0f) ? 1U : 0U;

    if (humi_too_high || !isBright)
    {
        targetState = STATE_THU;
    }
    else
    {
        targetState = STATE_PHOI;
    }
}

/* ----------------------------------------------------------------
 * Logic_HandleFanAuto
 *
 * Kịch bản quạt mới (áp dụng cho MỌI chế độ, không chỉ AUTO):
 *   1. Khi giá phơi đã THU XONG (currentState == STATE_THU) và
 *      đang mưa (isRaining = 1) → bật quạt, giữ chạy liên tục.
 *   2. Khi vẫn còn mưa → quạt tiếp tục quay, không tắt.
 *   3. Khi mưa vừa dừng (cạnh xuống isRaining 1→0) → bắt đầu đếm
 *      FAN_POST_RAIN_MS (10 giây), quạt vẫn quay trong lúc đếm.
 *   4. Hết 10 giây sau khi tạnh mưa → tắt quạt.
 *
 * LƯU Ý: Toàn bộ khối này KHÔNG bị chặn bởi manual_mode, vì mưa
 * phải luôn được xử lý bất kể chế độ nào (theo yêu cầu).
 *
 * Phần xử lý quạt thủ công (fan_manual, do người dùng nhấn C/D)
 * vẫn được tôn trọng: nếu người dùng đã chủ động bật/tắt quạt bằng
 * tay, khối auto này sẽ không tự ý đổi lại, TRỪ khi có mưa — vì an
 * toàn quần áo được ưu tiên cao hơn lựa chọn quạt thủ công.
 *
 * MANUAL MODE LÀ QUYỀN CAO NHẤT: một khi người dùng đã bật MANUAL
 * (1#) thì hệ thống KHÔNG được tự ý chuyển về AUTO theo thời gian.
 * Ví dụ khi người dùng nhấn A (Thu vào), targetState phải được giữ
 * nguyên ở STATE_THU cho tới khi giá thu xong, hoặc cho tới khi
 * người dùng chủ động thoát bằng 0#. MANUAL CHỈ thoát khi người
 * dùng bấm "0#" — không có cơ chế tự động hết-giờ (timeout) quay
 * về AUTO.
 * ---------------------------------------------------------------- */
void Logic_HandleFanAuto(uint8_t isRaining)
{
    /* --- Logic quạt theo mưa: LUÔN chạy, không quan tâm manual_mode --- */

    /* Trường hợp 1 + 2: đang mưa */
    if (isRaining)
    {
        s_fan_cooldown_active = 0;   /* Hủy mọi đếm giờ tắt đang chạy */

        /* Chỉ bật quạt khi giá đã thu xong hẳn (an toàn, tránh quạt
         * thổi vào quần áo còn đang ở ngoài lúc motor chưa kéo về) */
        if (currentState == STATE_THU)
        {
            if (!fan_state)
            {
                fan_manual = 0;
                Actuator_SetFan(1);
            }
        }
    }
    /* Trường hợp 3 + 4: vừa hết mưa hoặc đã hết mưa từ trước */
    else
    {
        if (rain_prev && fan_state)
        {
            /* Cạnh xuống: mưa vừa dừng, quạt đang chạy → bắt đầu đếm */
            s_fan_cooldown_active = 1;
            fan_timer             = HAL_GetTick();
        }

        if (s_fan_cooldown_active)
        {
            if (HAL_GetTick() - fan_timer > FAN_POST_RAIN_MS)
            {
                Actuator_SetFan(0);
                s_fan_cooldown_active = 0;
            }
        }
    }

    rain_prev = isRaining;
}
