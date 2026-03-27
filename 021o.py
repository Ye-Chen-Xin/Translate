import cv2
import sys

input_path = './menu.png'
output_path = './output_smooth.png'

# 读取图像
img = cv2.imread(input_path)
if img is None:
    print(f"无法读取图像: {input_path}")
    sys.exit(1)

# 设置放大倍数（例如 2 倍）
scale = 2.0
h, w = img.shape[:2]
resized = cv2.resize(img, (int(w * scale), int(h * scale)), interpolation=cv2.INTER_CUBIC)

# 平滑边缘（使用双边滤波保边缘）
smoothed = cv2.bilateralFilter(resized, d=9, sigmaColor=75, sigmaSpace=75)

# 保存结果
if cv2.imwrite(output_path, smoothed):
    print(f"平滑结果已保存到: {output_path}")
else:
    print("保存结果失败")