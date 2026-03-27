#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
#include <iostream>

// 获取主屏幕截图并保存为 PNG 文件到当前目录。
// 不依赖 OpenCV、无 system() 调用，只使用 macOS 原生框架。
int main() {
    // 1. 获取主显示器 ID
    CGDirectDisplayID displayID = CGMainDisplayID();

    // 2. 获取主显示器的像素区域
    CGRect screenRect = CGDisplayBounds(displayID);
    // 如果你只想截取某个小范围，可以改成：
    //   CGRect screenRect = CGRectMake(100, 100, 300, 200); // 例子

    // 3. 通过 CGWindowListCreateImage 截图
    //    这里选用 kCGWindowListOptionOnScreenOnly，可捕获当前实际屏幕图像
    CGImageRef screenImg = CGWindowListCreateImage(
        screenRect,
        kCGWindowListOptionOnScreenOnly,
        kCGNullWindowID,
        kCGWindowImageDefault
    );
    if (!screenImg) {
        std::cerr << "[Error] Failed to capture screen.\n";
        return -1;
    }

    // 4. 生成文件保存 URL
    //    这里保存到相对路径 ./test_capture.png
    const char *fileName = "test_capture.png";
    CFURLRef url = CFURLCreateFromFileSystemRepresentation(
        kCFAllocatorDefault,
        reinterpret_cast<const UInt8 *>(fileName),
        std::strlen(fileName),
        false // false 表示不是目录
    );
    if (!url) {
        std::cerr << "[Error] Failed to create CFURL.\n";
        CGImageRelease(screenImg);
        return -1;
    }

    // 5. 创建 Image Destination，用来写 PNG
    CGImageDestinationRef dest = CGImageDestinationCreateWithURL(
        url,
        kUTTypePNG, // 指定 PNG 格式
        1,          // 图片数量
        nullptr
    );
    if (!dest) {
        std::cerr << "[Error] Failed to create CGImageDestination.\n";
        CFRelease(url);
        CGImageRelease(screenImg);
        return -1;
    }

    // 6. 将截到的 CGImageRef 加入写入目标
    CGImageDestinationAddImage(dest, screenImg, nullptr);

    // 7. Finalize 写入文件
    if (!CGImageDestinationFinalize(dest)) {
        std::cerr << "[Error] Failed to finalize image writing.\n";
    } else {
        std::cout << "Screenshot saved to " << fileName << std::endl;
    }

    // 8. 释放资源
    CFRelease(dest);
    CFRelease(url);
    CGImageRelease(screenImg);

    return 0;
}