import serial
import json
import numpy as np

# 配置串口
ser = serial.Serial('COM7', 115200, timeout=999)  # 根据实际情况修改串口号和波特率

motion_name = ["Circle", "Letter_L", "Letter_M", "Letter_R", "Letter_W", "NoMotion", "Triangle", "Upanddown"]
motion_id = 7
for j in range(50):
    file_name = f"./{motion_name[motion_id]}/{motion_name[motion_id]}_{j}.txt"
    file_data = []
    for i in range(200):
        # 读取一行数据
        line = ser.readline().decode('utf-8').strip()

        # 解析 JSON 数据
        try:
            data = json.loads(line)

            # 添加到文件数据
            file_data.append([data['Gyro[0]'], data['Gyro[1]'], data['Gyro[2]']])

        except json.JSONDecodeError:
            print("Invalid JSON data received")

    print(file_data)
    print(len(file_data))
    print(f"Save {file_name}")
    file_data = np.array(file_data)
    np.savetxt(file_name, file_data, delimiter=',', fmt='%.6f')

# 关闭串口
ser.close()
