#include "sensor1.h"
#include "dht11.h"

/* ================================================================
 * sensor.c
 * Các hàm đọc phần cứng cảm biến. Không chứa logic nghiệp vụ —
 * chỉ đọc và trả về dữ liệu thô / đã lọc.
 * ================================================================ */

/* Tham chiếu ADC handle từ main (do CubeMX khai báo) */
extern ADC_HandleTypeDef hadc1;

/* ----------------------------------------------------------------
 * Sensor_RainDetect
 * Đọc chân DO của module mưa 5 lần, cách nhau 5 ms.
 * Trả về 1 nếu ít nhất 3/5 lần đọc thấy mức LOW (= đang mưa).
 * Lọc nhiễu tránh trigger giả do rung động hoặc nước bắn.
 * ---------------------------------------------------------------- */
uint8_t Sensor_RainDetect(void)
{
    int count = 0;

    for (int i = 0; i < 5; i++)
    {
        if (HAL_GPIO_ReadPin(RAIN_PORT, RAIN_PIN) == GPIO_PIN_RESET)
        {
            count++;
        }
        HAL_Delay(5);
    }

    return (count >= 3) ? 1U : 0U;
}

/* ----------------------------------------------------------------
 * Sensor_ReadLDR
 * Khởi động chuyển đổi ADC theo polling, lấy kết quả, rồi dừng.
 * Sử dụng kênh ADC1_CH0 (PA0) đã cấu hình sẵn bởi CubeMX.
 * Giá trị trả về: 0 (tối đa ánh sáng) → 4095 (tối hoàn toàn).
 * ---------------------------------------------------------------- */
uint16_t Sensor_ReadLDR(void)
{
    uint16_t value = 0;

    HAL_ADC_Start(&hadc1);

    if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
    {
        value = (uint16_t)HAL_ADC_GetValue(&hadc1);
    }

    HAL_ADC_Stop(&hadc1);
    return value;
}

/* ----------------------------------------------------------------
 * Sensor_ReadDHT11
 * Wrapper gọi thư viện DHT11. Nếu đọc lỗi (timeout / checksum sai),
 * ghi 0.0 để module logic biết dữ liệu không hợp lệ.
 * ---------------------------------------------------------------- */
void Sensor_ReadDHT11(float *out_temp, float *out_humi)
{
    if (DHT11_Read(out_temp, out_humi) != 0)
    {
        *out_temp = 0.0f;
        *out_humi = 0.0f;
    }
}
