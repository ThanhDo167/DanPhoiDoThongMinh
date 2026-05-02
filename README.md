# Đề tài: Thiết kế và triển khai hệ thống phơi đồ thông minh sử dụng STM32

## 1.Hệ thống có khả năng:
 Tự động thu đồ khi trời mưa
 
 Phơi đồ khi trời nắng
 
 Hiển thị thông tin trên LCD
 
 Điều khiển bằng keypad + mật khẩu
 
Có chế độ Auto / Manual
## 2.Phần cứng sử dụng:
Thiết bị	                     | Chức năng 

STM32F103C8T6	                 |Vi điều khiển trung tâm

DHT11	                         | Đo nhiệt độ & độ ẩm

Cảm biến mưa	                 | Phát hiện mưa

LDR	                           | Đo ánh sáng

Động cơ bước 28BYJ-48 + ULN2003|Kéo giàn phơi

Relay 5V	                     | Điều khiển quạt

Buzzer	                       | Cảnh báo

LCD I2C	                       | Hiển thị trạng thái hệ thống

Keypad 4x4	                   | Nhập lệnh

Quạt mini 5V	                 |Sấy khô đồ 
## 3.Sơ đồ kết nối (tóm tắt)
Thiết bị	                     | Kết nối với STM32

DHT11	Data                     | PA2

Cảm biến mưa	                         | PA1

LDR	                           | PA2

Động cơ bước 28BYJ-48 + ULN2003|PA5 PA6 PA7 PB8

Relay 5V	                     |PA4

Buzzer	                       | PA3

LCD I2C	                       |PB6 (SCL), PB7 (SDA)

Keypad 4x4	                   | PB0 PB1 PB10 PB11 PB12 PB13 PB14 PB15
