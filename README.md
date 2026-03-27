# 翻译快捷键

macOS 系统翻译快捷键工具，在 Preview（预览）等应用中快速调用系统翻译功能。

## 功能说明

按下 `CMD + SHIFT + A`，程序会自动：
1. 在当前鼠标位置右键点击，弹出上下文菜单
2. 截取菜单区域截图
3. 使用 Tesseract OCR + OpenCV 识别菜单文字
4. 找到"翻译/翻譯"菜单项并自动点击，触发系统翻译

## 可用代码文件

| 文件 | 语言 | 说明 |
|------|------|------|
| `click_021o.cpp` | C++ | 核心主程序：快捷键监听 + 截图 + OCR + 自动点击 |
| `capture.cpp` | C++ | 轻量截图工具，使用 macOS 原生框架保存全屏截图 |
| `021o.py` | Python | 图像放大与平滑处理，辅助提升 OCR 识别准确率 |
| `compile.txt` | 文本 | 编译 `click_021o.cpp` 的 g++ 命令 |

## 编译与运行

```sh
g++ click_021o.cpp -o ocr02 \
    -I/opt/homebrew/include \
    -I/opt/homebrew/include/opencv4 \
    -L/opt/homebrew/lib \
    -ltesseract -lleptonica \
    -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs \
    -framework ApplicationServices -framework Carbon \
    -std=c++17
./ocr02
```

## 依赖

- macOS
- [Tesseract OCR](https://github.com/tesseract-ocr/tesseract)（需安装 `chi_sim` 语言包）
- [Leptonica](http://www.leptonica.org/)
- [OpenCV](https://opencv.org/)
- macOS 框架：`ApplicationServices`、`Carbon`、`CoreGraphics`、`ImageIO`
