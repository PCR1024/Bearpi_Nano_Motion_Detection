import gc
import matplotlib.pyplot as plt
from nnom import *
import numpy as np
import tensorflow as tf
from sklearn.model_selection import train_test_split
from keras.models import Sequential  # 用于构建序贯模型
from keras.layers import Conv1D,Dense,ZeroPadding1D,MaxPooling1D,AveragePooling1D,Dropout,Flatten, LSTM, Dropout  # 用于构建不同类型的网络层

save_model_name = "best_model.h5"

motion_name = ["Circle", "Letter_L", "Letter_M", "Letter_R", "Letter_W", "NoMotion", "Triangle", "Upanddown", "Wave"]
data_per_size = 50
class_num = len(motion_name)
# 全部数据
total_data = []
# 标签数组
total_y = np.array([j for j in range(class_num) for i in range(data_per_size)])

# 获取标签的唯一值并排序
total_labels = np.unique(total_y)

# 创建独热编码矩阵
total_labels = np.eye(len(total_labels))[total_y]

for mo_name in motion_name:
    for i in range(data_per_size):
        data = np.loadtxt(f"/home/whiz/Projects/Motion_classification/Dataset/{mo_name}/{mo_name}_{i}.txt", delimiter=',')
        total_data.append(data[ : , -3 :])
        
total_data = np.array(total_data)

gc.collect()

# 1. 分割为训练集和测试集
X_train, X_test, y_train, y_test = train_test_split(total_data, total_labels, test_size=0.2, shuffle=True)

# 2. 进一步分割训练集为训练集和验证集
X_train, X_val, y_train, y_val = train_test_split(X_train, y_train, test_size=0.2, shuffle=True)

# 创建模型  
model = Sequential()  
  
# 添加一维卷积层，卷积核大小为3，数量为64，输入形状为（128,3），激活函数为relu  
model.add(Conv1D(100, 5, 3, activation="relu", input_shape=(200, 3)))  
  
# 添加一维卷积层，卷积核大小为3，数量为64，激活函数为relu  
model.add(Conv1D(100, 3, 3, activation="relu"))  
  
# 添加一维最大池化层，池化窗口大小为2  
# model.add(MaxPooling1D(pool_size=2))  

# 添加 LSTM 层
# model.add(LSTM(units=200))
  
# 展平层，用于将输入展平，以便可以进入全连接层  
model.add(Flatten())  
  
# 添加全连接层，节点数为128，激活函数为relu  
model.add(Dense(128, activation='relu'))  
model.add(Dropout(0.35))
  
# 添加输出层，节点数为类别数，激活函数为softmax  
model.add(Dense(9, activation='softmax'))  # 如果有其他数量的类别，请修改这里的节点数  
  
# 模型结构概述  
model.summary()
# 编译模型，优化器使用adam，损失函数使用交叉熵损失函数，评估标准为准确率  
model.compile(optimizer="adam", loss="categorical_crossentropy", metrics=["accuracy"])

# 训练模型  
history = model.fit(X_train, y_train, epochs=250, batch_size=50, validation_data=(X_val, y_val))

model.save(save_model_name)

from keras.models import load_model
L_model = load_model(save_model_name)

loss, accuracy = L_model.evaluate(X_test, y_test)