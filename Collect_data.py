import serial
import csv
import time

# ⚙️ Cấu hình
PORT = "COM6"        # ⚠️ Đổi lại đúng cổng Arduino của bạn (ví dụ: COM5 hoặc /dev/ttyUSB0)
BAUD = 115200        # Tốc độ truyền phải trùng với Serial.begin() trong Arduino
FILENAME = "data.csv"

# ⏱️ Mở kết nối serial
ser = serial.Serial(PORT, BAUD)
time.sleep(2)  # Chờ Arduino khởi động

print(f"🔌 Đang ghi dữ liệu từ {PORT} vào {FILENAME} ... (nhấn Ctrl + C để dừng)")

# 🚀 Tạo file CSV và bắt đầu ghi
with open(FILENAME, mode="w", newline="") as file:
    writer = csv.writer(file)
    header_written = False

    try:
        while True:
            line = ser.readline().decode("utf-8").strip()
            if not line:
                continue

            # In ra màn hình để theo dõi
            print(line)

            # Nếu là dòng tiêu đề thì ghi header
            if not header_written and "AC_scaled" in line:
                writer.writerow(line.split(","))
                header_written = True
                continue

            # Ghi dữ liệu
            if header_written:
                writer.writerow(line.split(","))

    except KeyboardInterrupt:
        print("\n🛑 Dừng ghi dữ liệu.")
        ser.close()
