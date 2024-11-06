import socket
import json
import numpy as np

# 配置 TCP 客户端
HOST = '192.168.137.89'  # 服务器地址
PORT = 8888  # 服务器端口

# 初始化二维数组
accel_data = []
gyro_data = []

# 创建 TCP 客户端套接字
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    # 连接服务器
    s.connect((HOST, PORT))

    try:
        while True:
            # 接收数据
            data = s.recv(1024).decode('utf-8').strip()

            print(data)

            # # 解析 JSON 数据
            # try:
            #     json_data = json.loads(data)
            #
            #     # 提取 Accel 和 Gyro 数据
            #     accel = [json_data['Accel[0]'], json_data['Accel[1]'], json_data['Accel[2]']]
            #     gyro = [json_data['Gyro[0]'], json_data['Gyro[1]'], json_data['Gyro[2]']]
            #
            #     # 将数据添加到二维数组中
            #     accel_data.append(accel)
            #     gyro_data.append(gyro)
            #
            #     # 打印数据
            #     print(f"Accel: {accel}")
            #     print(f"Gyro: {gyro}")
            #
            # except json.JSONDecodeError:
            #     print("Invalid JSON data received")

    except KeyboardInterrupt:
        print("Exiting...")

# 将二维数组转换为 NumPy 数组
accel_data = np.array(accel_data)
gyro_data = np.array(gyro_data)

# 打印最终的二维数组
print("Accel Data:")
print(accel_data)
print("Gyro Data:")
print(gyro_data)