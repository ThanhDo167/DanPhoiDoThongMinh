# Giàn Phơi Thông Minh — Smart Dry Rack

Hệ thống điều khiển giá phơi tự động dựa trên vi điều khiển STM32F103C8T6, có khả năng tự động thu/phơi đồ dựa trên điều kiện môi trường (mưa, ánh sáng, độ ẩm), hỗ trợ điều khiển thủ công qua bàn phím ma trận 4×4, hiển thị trạng thái lên LCD 16×2 và giám sát từ xa qua UART/PuTTY.

---

## Mục lục

1. [Tính năng](#1-tính-năng)
2. [Phần cứng và sơ đồ chân](#2-phần-cứng-và-sơ-đồ-chân)
3. [Cấu trúc phần mềm](#3-cấu-trúc-phần-mềm)
4. [Mô tả từng module](#4-mô-tả-từng-module)
5. [Logic hoạt động](#5-logic-hoạt-động)
6. [Bàn phím điều khiển](#6-bàn-phím-điều-khiển)
7. [Giám sát qua UART / PuTTY](#7-giám-sát-qua-uart--putty)
8. [Cài đặt và biên dịch](#8-cài-đặt-và-biên-dịch)
9. [Hạn chế và hướng phát triển](#9-hạn-chế-và-hướng-phát-triển)

---

## 1. Tính năng

**Chế độ tự động (AUTO)**
- Tự động phơi đồ khi trời sáng, không mưa, độ ẩm dưới 80%.
- Tự động thu đồ khi phát hiện mưa, trời tối, hoặc độ ẩm vượt 80%.
- Mưa là điều kiện ưu tiên tuyệt đối — luôn thu đồ bất kể chế độ nào.

**Chế độ thủ công (MANUAL)**
- Người dùng nhập mã `1#` trên bàn phím để vào chế độ thủ công.
- Nhấn phím `A/B/C/D` để thu đồ, phơi đồ, bật/tắt quạt.
- Không thể vào MANUAL khi đang mưa; không thể phơi đồ ra khi đang mưa dù đã ở MANUAL.
- Tự động quay về AUTO sau 15 giây không có tác động.
- Nhập mã `0#` để thoát MANUAL về AUTO ngay lập tức.

**Bảo vệ quần áo khi mưa**
- Thu đồ ngay khi phát hiện mưa bất kể chế độ hiện tại.
- Quạt tự động bật sau khi đồ đã thu xong để thổi khô và lưu thông không khí.
- Quạt duy trì chạy liên tục trong suốt thời gian mưa.
- Sau khi tạnh mưa, quạt chạy thêm 10 giây rồi tự tắt.

**Còi cảnh báo**
- Còi kêu khi phát hiện mưa.
- Nhập mã `1234#` để tắt còi (độc lập với chế độ AUTO/MANUAL).
- Còi tự reset cho lần mưa tiếp theo.

**Bảo mật**
- Hệ thống khóa hoàn toàn sau 5 lần nhập sai mã.

**Giám sát từ xa**
- Dashboard tự cập nhật mỗi giây qua UART/PuTTY hiển thị toàn bộ thông số và trạng thái hệ thống.

---

## 2. Phần cứng và sơ đồ chân

### Linh kiện sử dụng

| Linh kiện | Chức năng |
|---|---|
| STM32F103C8T6 (Blue Pill) | Vi điều khiển chính |
| Động cơ bước 28BYJ-48 + driver L298N | Kéo giá phơi vào/ra |
| Cảm biến mưa (module DO) | Phát hiện mưa |
| Cảm biến ánh sáng LDR | Phát hiện trời sáng/tối |
| DHT11 | Đo nhiệt độ và độ ẩm |
| LCD 16×2 + module I2C (PCF8574) | Hiển thị trạng thái |
| Bàn phím ma trận 4×4 | Nhập lệnh thủ công |
| Relay 5V | Điều khiển quạt |
| Buzzer | Còi cảnh báo mưa |
| Module USB-TTL (CP2102/CH340) | Kết nối UART với PC |

### Sơ đồ chân STM32

| Chân STM32 | Chức năng | Ghi chú |
|---|---|---|
| PA0 | LDR (ADC1_CH0) | Đọc ánh sáng qua ADC |
| PA1 | Cảm biến mưa DO | INPUT PULLUP, LOW = mưa |
| PA3 | Buzzer | OUTPUT, HIGH = kêu |
| PA4 | Relay (quạt) | OUTPUT, HIGH = bật quạt |
| PA5 | Stepper IN1 | L298N |
| PA6 | Stepper IN2 | L298N |
| PA7 | Stepper IN3 | L298N |
| PA9 | UART TX (USART1) | Kết nối RX của USB-TTL |
| PA10 | UART RX (USART1) | Không dùng (TX-only) |
| PB8 | Stepper IN4 | L298N |
| PB6 | I2C1 SCL | LCD qua PCF8574 |
| PB7 | I2C1 SDA | LCD qua PCF8574 |
| PB0 | Keypad ROW 1 | OUTPUT |
| PB1 | Keypad ROW 2 | OUTPUT |
| PB10 | Keypad ROW 3 | OUTPUT |
| PB11 | Keypad ROW 4 | OUTPUT |
| PB12 | Keypad COL 1 | INPUT PULLUP |
| PB13 | Keypad COL 2 | INPUT PULLUP |
| PB14 | Keypad COL 3 | INPUT PULLUP |
| PB15 | Keypad COL 4 | INPUT PULLUP |

### Cấu hình clock

- Nguồn: HSE (thạch anh ngoài)
- PLL × 9 → SYSCLK = 72 MHz
- APB1 = 36 MHz, APB2 = 72 MHz
- ADC clock = APB2 / 6 = 12 MHz

---

## 3. Cấu trúc phần mềm

```
Core/
├── Inc/
│   ├── system_state.h   # Biến toàn cục dùng chung (extern)
│   ├── sensor.h         # Cảm biến mưa, LDR, DHT11
│   ├── stepper.h        # Động cơ bước non-blocking
│   ├── actuator.h       # Quạt (relay) và còi (buzzer)
│   ├── logic.h          # Logic nghiệp vụ AUTO/MANUAL
│   ├── keypad.h         # Bàn phím ma trận 4×4
│   ├── display.h        # LCD 16×2 qua I2C
│   └── uart_cli.h       # Giám sát qua UART/PuTTY
└── Src/
    ├── system_state.c   # Định nghĩa biến toàn cục
    ├── sensor.c
    ├── stepper.c
    ├── actuator.c
    ├── logic.c
    ├── keypad.c
    ├── display.c
    ├── uart_cli.c
    └── main.c           # Khởi tạo HAL + vòng lặp chính
```

### Phân lớp kiến trúc

```
┌─────────────────────────────────────────┐
│              main.c                     │  Điều phối
├──────────┬──────────┬───────────────────┤
│ keypad.c │display.c │   uart_cli.c      │  Input / Output
├──────────┴──────────┴───────────────────┤
│        logic.c                          │  Nghiệp vụ
├──────────────────────────────────────────┤
│   sensor.c │ stepper.c │ actuator.c     │  Phần cứng
├──────────────────────────────────────────┤
│         HAL / CubeMX                    │  Driver STM32
└─────────────────────────────────────────┘
```

---

## 4. Mô tả từng module

### system_state.h / system_state.c
Nơi duy nhất khai báo và định nghĩa tất cả biến toàn cục. Mọi module khác chỉ dùng `extern` thông qua `#include "system_state.h"`, không tự khai báo thêm. Bao gồm các hằng số mã điều khiển và ngưỡng bảo mật.

Biến trạng thái chính:

| Biến | Kiểu | Ý nghĩa |
|---|---|---|
| `currentState` | SystemState | Vị trí thực tế của giá phơi |
| `targetState` | SystemState | Vị trí đích cần đến |
| `manual_mode` | uint8_t | 0 = AUTO, 1 = MANUAL |
| `system_locked` | uint8_t | 1 = bị khóa do sai mã 5 lần |
| `temp` / `humi` | float | Nhiệt độ (°C) và độ ẩm (%) |
| `last_isRaining` | uint8_t | Kết quả lọc cảm biến mưa |
| `last_isBright` | uint8_t | 1 = đủ sáng, 0 = tối |
| `fan_state` | uint8_t | Trạng thái quạt hiện tại |
| `fan_manual` | uint8_t | 0 = quạt tự động, 1 = thủ công |

### sensor.c
Đọc dữ liệu thô từ phần cứng, không chứa logic nghiệp vụ.

- `Sensor_RainDetect()` — Lọc nhiễu majority vote: đọc 5 lần cách nhau 5 ms, trả về 1 nếu ≥ 3 lần thấy LOW.
- `Sensor_ReadLDR()` — Khởi động ADC polling, đọc giá trị 12-bit, dừng ADC. Giá trị < 2000 = sáng.
- `Sensor_ReadDHT11()` — Wrapper thư viện DHT11. Nếu lỗi trả về 0.0 cho cả hai giá trị; logic có guard `humidity > 0.0f` để bỏ qua giá trị lỗi.

### stepper.c
Điều khiển động cơ bước 28BYJ-48 qua L298N bằng kỹ thuật half-step 8 pha, hoàn toàn non-blocking.

- `Stepper_Start(steps, dir)` — Đặt thông số, bật cờ `s_motor_running`. Không tự quay motor.
- `Stepper_Run()` — Gọi mỗi vòng `while(1)`. Kiểm tra `HAL_GetTick()`, tiến 1 bước nếu đã đủ 1 ms. Tự tắt điện cuộn dây và hạ cờ khi đủ 4096 bước.
- `Stepper_IsBusy()` — Trả về cờ `s_motor_running`, dùng cho main.c để bắt cạnh xuống (edge detection) xác định motor vừa hoàn thành.
- `Stepper_PowerOff()` — Tắt cả 4 chân IN để tránh cuộn dây bị nóng khi đứng yên.

### actuator.c
Lớp duy nhất được phép ghi GPIO relay và buzzer.

- `Actuator_SetFan(state)` — Ghi relay và đồng bộ `fan_state`.
- `Actuator_HandleBuzzer(isRaining)` — Ưu tiên: khóa hệ thống > tắt tiếng > đang mưa. Tự reset `buzzer_off` khi tạnh mưa.

### logic.c
Bộ não nghiệp vụ, không đọc GPIO trực tiếp.

- `Logic_UpdateRackState()` — Quyết định `targetState`. Thứ tự ưu tiên: mưa (thu vào, bỏ qua manual_mode) → MANUAL (giữ nguyên) → AUTO (xét độ ẩm và ánh sáng).
- `Logic_HandleFanAuto()` — Quản lý quạt theo chu kỳ mưa và timeout MANUAL 15 giây.
- `Logic_SetManualStartTick()` — Reset đồng hồ 15 giây mỗi khi người dùng có tác động trong MANUAL.

### keypad.c
Quét ma trận 4×4 với debounce 20 ms và chờ nhả phím.

Xử lý phím trong `Keypad_Handle()`:

| Phím | Điều kiện | Hành động |
|---|---|---|
| `1#` | Không mưa | Vào MANUAL mode |
| `1#` | Đang mưa | Từ chối, hiện "DANG MUA!" |
| `0#` | Bất kỳ | Thoát MANUAL về AUTO ngay |
| `A` | Đang MANUAL | Thu đồ vào |
| `B` | MANUAL + không mưa | Phơi đồ ra |
| `B` | MANUAL + đang mưa | Từ chối, hiện cảnh báo |
| `C` | Đang MANUAL | Bật quạt thủ công |
| `D` | Đang MANUAL | Tắt quạt thủ công |
| `A/B/C/D` | Không ở MANUAL | Nhắc nhập `1#` trước |
| `1234#` | Bất kỳ | Tắt còi, giữ nguyên chế độ |
| Sai mã | — | Tăng `wrong_count`, khóa sau 5 lần |
| `*` | — | Xóa mã đang nhập |

Mỗi phản hồi hiển thị trên LCD 1.5 giây (`LCD_FLASH_HOLD_MS`) trước khi trở lại màn hình bình thường.

### display.c
Hiển thị LCD 16×2 qua I2C với cơ chế cache hai dòng (`old_line1`, `old_line2`) — chỉ gửi I2C khi nội dung thực sự thay đổi, giảm tải bus I2C 100 kHz.

Dòng 1: `T:<°C> H:<%%> F:<fan> E:<sai>`
Dòng 2: `<Mua/Nang>|<Sang/Toi>|<Thu/Phoi>` hoặc `PASS:***` khi đang nhập mã.

### uart_cli.c
Dashboard chỉ đọc (TX-only) hiển thị toàn bộ thông số lên PuTTY, tự cập nhật mỗi giây mà không gây chớp màn hình.

Kỹ thuật chống chớp: khung tĩnh (viền, tiêu đề, nhãn) vẽ một lần khi khởi động bằng `ANSI_CLEAR`. Mỗi giây chỉ dùng lệnh nhảy tọa độ `\033[row;colH` để ghi đè đúng phần giá trị của từng dòng, kèm `ANSI_CLREOL` xóa ký tự thừa — không xóa toàn màn hình, không cuộn.

### main.c
Khởi tạo HAL và ngoại vi, sau đó vào vòng lặp `while(1)` với 4 phần:

1. `Keypad_Handle()` — mỗi vòng lặp
2. Điều khiển motor bằng edge detection: lưu `prev_busy` → `Stepper_Run()` → nếu `prev_busy && !IsBusy()` thì `currentState = targetState` → nếu `!IsBusy() && currentState != targetState` thì `Stepper_Start()`
3. Khối 1 giây: đọc cảm biến → logic → buzzer → LCD → UART

---

## 5. Logic hoạt động

### Sơ đồ quyết định trạng thái giá phơi

```
Mỗi 1 giây đọc cảm biến
          │
          ▼
    Đang mưa?
    ┌──YES──┐
    │       │
    ▼       ▼ NO
THU VÀO  manual_mode = 1?
          ┌──YES──┐
          │       │
          ▼       ▼ NO (AUTO)
        GIỮ    Độ ẩm > 80%
        NGUYÊN   hoặc tối?
                ┌──YES──┐
                │       │
                ▼       ▼ NO
            THU VÀO  PHƠI RA
```

### Logic quạt khi mưa

```
Mưa bắt đầu → Thu đồ (motor quay)
                    │
                    ▼
            Giá đã thu xong?
            (currentState == STATE_THU)
                    │ YES
                    ▼
              Bật quạt ──→ Chạy liên tục khi còn mưa
                                    │
                    ┌───────────────┘
                    ▼ Mưa dừng
              Đếm 10 giây
                    │
                    ▼
              Tắt quạt
```

### Cơ chế điều khiển motor (Edge Detection)

Vấn đề cần giải quyết: nếu cập nhật `currentState = targetState` mỗi khi `!IsBusy()`, khi `targetState` vừa đổi nhưng motor đang rảnh, `currentState` bị kéo theo ngay → điều kiện khởi động không bao giờ đúng → motor không quay.

Giải pháp: bắt cạnh xuống (falling edge) của `IsBusy()`.

```c
uint8_t prev_busy = Stepper_IsBusy();
Stepper_Run();

// Chỉ cập nhật khi motor VỪA dừng (1→0)
if (prev_busy && !Stepper_IsBusy())
    currentState = targetState;

// Khởi động khi motor rảnh và chưa đúng vị trí
if (!Stepper_IsBusy() && currentState != targetState)
    Stepper_Start(STEPPER_STEPS, dir);
```

---

## 6. Bàn phím điều khiển

### Sơ đồ bàn phím 4×4

```
┌───┬───┬───┬───┐
│ 1 │ 2 │ 3 │ A │  A = Thu đồ vào      (chỉ khi MANUAL)
├───┼───┼───┼───┤
│ 4 │ 5 │ 6 │ B │  B = Phơi đồ ra      (chỉ khi MANUAL, không mưa)
├───┼───┼───┼───┤
│ 7 │ 8 │ 9 │ C │  C = Bật quạt        (chỉ khi MANUAL)
├───┼───┼───┼───┤
│ * │ 0 │ # │ D │  D = Tắt quạt        (chỉ khi MANUAL)
└───┴───┴───┴───┘
* = Xóa mã đang nhập
# = Xác nhận mã
```

### Các mã điều khiển

| Mã | Chức năng |
|---|---|
| `1` + `#` | Vào MANUAL mode (bị từ chối nếu đang mưa) |
| `0` + `#` | Thoát MANUAL, về AUTO (cập nhật targetState ngay) |
| `1234` + `#` | Tắt còi báo mưa (luôn hoạt động, không phụ thuộc chế độ) |
| Mã khác + `#` | Sai mã, tăng đếm sai |

### Bảo mật

- Sau **5 lần nhập sai** bất kỳ mã nào, hệ thống khóa hoàn toàn (`system_locked = 1`).
- Khi bị khóa, mọi phím đều bị bỏ qua, còi tắt, LCD hiện "BI KHOA!".
- Chỉ có thể mở khóa bằng cách reset phần cứng (nút RESET trên board).

---

## 7. Giám sát qua UART / PuTTY

### Cấu hình PuTTY

| Thông số | Giá trị |
|---|---|
| Connection type | Serial |
| Serial line | COM* (xem Device Manager) |
| Speed (Baud rate) | 115200 |
| Data bits | 8 |
| Stop bits | 1 |
| Parity | None |
| Flow control | None |

Trong **Terminal**: bật `Implicit CR in every LF`.
Trong **Window → Colours**: bật `Allow terminal to use xterm 256-colour mode`.

### Kết nối vật lý

```
USB-TTL TX  →  PA10 (STM32 RX, không dùng trong TX-only mode)
USB-TTL RX  →  PA9  (STM32 TX)
USB-TTL GND →  GND
```

### Thông tin hiển thị trên dashboard

```
+=============================================+
|      SMART DRY RACK - GIAM SAT HE THONG    |
+---------------------------------------------+
|  [CAM BIEN]                                 |
| NHIET DO  : 28.5 C                          |
| DO AM     : 65.0 %                          |
| ANH SANG  : SANG   (ADC:  98)               |
| MUA       : KHONG MUA                       |
+---------------------------------------------+
|  [TRANG THAI]                               |
| CHE DO    : AUTO                            |
| GIA PHOI  : PHOI RA                         |
| QUAT      : OFF  (Auto  )                   |
| HE THONG  : HOAT DONG                       |
| SAI MA    : 0 / 5 lan                       |
+=============================================+
```

Màu sắc: xanh lá = bình thường, vàng = cảnh báo / đang di chuyển, đỏ = nguy hiểm / đang mưa.

Dashboard tự cập nhật mỗi 1 giây. Chỉ các dòng giá trị được ghi đè tại chỗ bằng escape code ANSI `\033[row;colH` — không xóa toàn màn hình nên không gây hiện tượng chớp.

---

## 8. Cài đặt và biên dịch

### Yêu cầu

- STM32CubeIDE 1.10 trở lên
- STM32CubeMX (tích hợp trong CubeIDE)
- Thư viện thêm vào `Core/Src` và `Core/Inc`:
  - `dht11.c` / `dht11.h` — thư viện đọc DHT11 (tùy viết hoặc dùng thư viện bên ngoài)
  - `lcd_i2c.c` / `lcd_i2c.h` — thư viện LCD 16×2 qua PCF8574

### Các bước

1. Clone hoặc giải nén project vào thư mục làm việc.
2. Mở STM32CubeIDE, chọn **File → Open Projects from File System**, trỏ đến thư mục project.
3. Thêm các file thư viện `dht11` và `lcd_i2c` vào `Core/Src` và `Core/Inc`.
4. Vào **Project → Properties → C/C++ Build → Settings → MCU GCC Compiler → Include paths**, đảm bảo `Core/Inc` đã có trong danh sách.
5. Nhấn **Build** (Ctrl+B). Kết quả mong đợi: 0 errors, tối đa 2 warnings (liên quan `const char*` của `lcd_send_string`).
6. Nạp firmware bằng ST-Link: **Run → Debug** hoặc dùng STM32CubeProgrammer với file `.hex` trong thư mục `Debug/`.

### Xử lý warning thường gặp khi build

```
warning: passing argument 1 of 'lcd_send_string' discards 'const' qualifier
```

Nguyên nhân: hàm `lcd_send_string()` trong thư viện khai báo `char *str` thay vì `const char *str`. Sửa bằng cách thêm cast `(char *)` tại nơi gọi, hoặc sửa prototype trong `lcd_i2c.h` thành `const char *str`.

---

## 9. Hạn chế và hướng phát triển

### Hạn chế hiện tại

**Điều khiển vòng hở (open-loop):** Hệ thống chỉ đếm số bước lệnh đã phát ra cho motor (`s_step_count`), không có cảm biến xác nhận vị trí thực tế. Nếu motor bị trượt bước do tải nặng hoặc mất điện giữa chừng, `currentState` vẫn được cập nhật như đã đến đích trong khi thực tế chưa tới.

**DHT11 không ổn định:** DHT11 có tỷ lệ lỗi đọc khá cao, đặc biệt khi vòng lặp bị block bởi `HAL_Delay()` trong keypad. Khi lỗi, hệ thống bỏ qua điều kiện độ ẩm và chỉ dựa vào mưa và ánh sáng.

**Keypad blocking:** Mỗi lần hiển thị thông báo LCD, `HAL_Delay(1500)` block toàn bộ vòng lặp, làm motor tạm dừng 1.5 giây nếu đang quay. Đây là đánh đổi có chủ đích vì thông báo chỉ xuất hiện khi người dùng chủ động nhấn phím.

**Không có bộ nhớ không khả dẫn:** Toàn bộ trạng thái mất sau khi mất điện. Board khởi động lại ở `STATE_THU` bất kể vị trí thực tế của giá phơi.

### Hướng phát triển

- Thêm **limit switch** ở hai đầu hành trình để calibrate vị trí sau mỗi lần khởi động, chuyển từ open-loop sang closed-loop.
- Thay DHT11 bằng **DHT22** hoặc **SHT31** để có độ chính xác và tính ổn định cao hơn.
- Sử dụng **DMA + interrupt** cho UART thay vì TX blocking, và **TIM interrupt** cho stepper thay vì polling `HAL_GetTick()` trong vòng lặp.
- Thêm **kết nối WiFi** (ESP8266/ESP32) để điều khiển qua điện thoại và gửi cảnh báo mưa qua ứng dụng.
- Lưu trạng thái vào **Flash nội bộ** hoặc EEPROM để giữ nguyên sau khi mất điện.
- Thêm **màn hình OLED** thay LCD 16×2 để hiển thị nhiều thông tin hơn với giao diện đẹp hơn.
