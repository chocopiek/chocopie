import serial
import csv
import time
from datetime import datetime

# ⚙️ Cấu hình cổng và tốc độ
PORT = "COM6"         # ⚠️ Đổi lại đúng cổng Arduino (ví dụ: COM5 hoặc /dev/ttyUSB0)
BAUD = 115200         # Phải trùng với Serial.begin() trong Arduino
FILENAME = "data.csv" # Tên file lưu dữ liệu

# ⏱️ Mở kết nối Serial
try:
    ser = serial.Serial(PORT, BAUD, timeout=1)
    time.sleep(2)  # Chờ Arduino khởi động lại
    print(f"🔌 Đang ghi dữ liệu từ {PORT} vào {FILENAME} ... (nhấn Ctrl + C để dừng)")
except Exception as e:
    print(f"❌ Lỗi mở cổng {PORT}: {e}")
    exit()

# 🚀 Mở file CSV để ghi
with open(FILENAME, mode="w", newline="") as file:
    writer = csv.writer(file)

    # 🧠 Header đúng với thứ tự Serial.print bên Arduino
    header = [
        "deltaT12",
        "SI",
        "RI",
        "AI",
        "AC_DC",
        "HR",
        "Glucose"
    ]
    writer.writerow(header)

    try:
        while True:
            try:
                line = ser.readline().decode("utf-8").strip()
                if not line:
                    continue

                # Bỏ qua các dòng không hợp lệ hoặc dòng tiêu đề từ Arduino
                if "AC_scaled" in line or "P1" in line:
                    continue

                # Cắt dữ liệu theo dấu phẩy
                values = line.split(",")
                if len(values) < len(header) - 1:
                    print("⚠️ Bỏ qua dòng không hợp lệ:", line)
                    continue

                # Thêm timestamp (thời gian thật của máy tính)
                writer.writerow(values)
                file.flush()  # Lưu ngay để tránh mất dữ liệu

                # In ra màn hình để theo dõi nhanh
                print("✅", values)

            except UnicodeDecodeError:
                continue  # Bỏ qua lỗi ký tự lạ

    except KeyboardInterrupt:
        print("\n🛑 Dừng ghi dữ liệu.")
        ser.close()
        