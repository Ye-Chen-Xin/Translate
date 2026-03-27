#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <thread>
#include <chrono>
#include <opencv2/opencv.hpp>
// 先处理 macOS 头文件，以免污染后面的 Leptonica 头文件
#ifdef fract1
#undef fract1
#endif

#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>  // 必须在 #undef fract1 之后

using namespace std;
int BILLY = 2;//系统缩放比例
float Yoffset = 750;//截图的y轴偏移量 //真实像素//750
float Xoffset = 16;//截图的x轴偏移量 //真实像素//翻32//譯75
float Ylength = 1820;//截图高度 //真实像素//1820
float Xlength = 44;//截图宽度 //真实像素//翻44//翻译71//譯27
//接受截图的常数是实际像素，但函数接受的是系统像素
//截图参数是缩放比例，但是截图的像素是实际像素
//截图跟ocr识别取得的都是实际像素比例
//鼠标的移动时系统缩放比例=实际像素比例/系统缩放值
// ========== 鼠标与键盘事件相关 ==========
CGPoint getMousePosition() {
    CGEventRef event = CGEventCreate(NULL);
    CGPoint cursor = CGEventGetLocation(event);
    CFRelease(event);
    return cursor;
}

void moveMouse(CGPoint point) {
    CGEventRef moveEvent = CGEventCreateMouseEvent(NULL, kCGEventMouseMoved, point, kCGMouseButtonLeft);
    CGEventPost(kCGHIDEventTap, moveEvent);
    CFRelease(moveEvent);
}

void rightClick(CGPoint point) {
    point.x-=10;
    CGEventRef mouseDown = CGEventCreateMouseEvent(NULL, kCGEventRightMouseDown, point, kCGMouseButtonRight);
    CGEventRef mouseUp   = CGEventCreateMouseEvent(NULL, kCGEventRightMouseUp,   point, kCGMouseButtonRight);
    point.x+=10;
    CGEventPost(kCGHIDEventTap, mouseDown);
    this_thread::sleep_for(chrono::milliseconds(5));
    CGEventPost(kCGHIDEventTap, mouseUp);
    CFRelease(mouseDown);
    CFRelease(mouseUp);
    
}

// 左键点击
void leftClick(CGPoint point) {
    CGEventRef mouseDown = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDown, point, kCGMouseButtonLeft);
    CGEventRef mouseUp   = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseUp,   point, kCGMouseButtonLeft);
    CGEventPost(kCGHIDEventTap, mouseDown);
    this_thread::sleep_for(chrono::milliseconds(5));
    CGEventPost(kCGHIDEventTap, mouseUp);

    CFRelease(mouseDown);
    CFRelease(mouseUp);
}

// ========== 截图函数 ==========
void captureScreen(CGPoint point, int width, int height, const char* filename) {
    string command = "screencapture -x -R" +
                     to_string(static_cast<int>(point.x)) + "," +
                     to_string(static_cast<int>(point.y)) + "," +
                     to_string(width) + "," +
                     to_string(height) + " " + filename;
    system(command.c_str());
}

//此处取得的像素为实际像素，截图的信息也是实际像素，要记得将结果除以缩放比例
// ========== OCR 函数 (返回文本与对应 bounding box) ==========

vector<pair<string, CGRect>> runTesseractOCRWithOpenCV(const char* imagePath) {
    vector<pair<string, CGRect>> results;
    tesseract::TessBaseAPI api;
    if (api.Init("/opt/homebrew/share/tessdata", "chi_sim") != 0) {
        cerr << "无法初始化 Tesseract." << endl;
        return results;
    }
    
    cv::Mat src = cv::imread(imagePath);
    if (src.empty()) {
        cerr << "无法打开输入图像：" << imagePath << endl;
        return results;
    }
    
    cv::Mat gray, binary;
    cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    // Otsu 自动二值化
    //cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    
    // 保存临时灰度
    std::string tempPath = "./tmp_binary.png";
    cv::imwrite(tempPath, gray);
    
    Pix *pix = pixRead(tempPath.c_str());
    if (!pix) {
        cerr << "无法加载处理后的图像：" << tempPath << endl;
        return results;
    }
    
    // 使用处理后的 image 进行 OCR
    api.SetImage(pix);
    char* outText = api.GetUTF8Text();
    istringstream iss(outText);
    vector<string> lines;
    string line;
    while (getline(iss, line))
        lines.push_back(line);
    delete[] outText;
    
    Boxa* boxes = api.GetComponentImages(tesseract::RIL_TEXTLINE, true, nullptr, nullptr);
    if (!boxes) {
        cerr << "无法获取文本区域." << endl;
        api.End();
        pixDestroy(&pix);
        return results;
    }
    int boxCount = boxaGetCount(boxes);
    for (size_t i = 0; i < lines.size() && i < static_cast<size_t>(boxCount); ++i) {
        Box* box = boxaGetBox(boxes, i, L_CLONE);
        if (!box) continue;
        l_int32 x, y, w, h;
        boxGetGeometry(box, &x, &y, &w, &h);
        CGRect rect = CGRectMake(x, y, w, h);
        results.emplace_back(lines[i], rect);
        boxDestroy(&box);
    }
    api.End();
    boxaDestroy(&boxes);
    pixDestroy(&pix);
    
    // 删除临时文件
    remove(tempPath.c_str());
    return results;
}
/*
vector<pair<string, CGRect>> runTesseractOCR(const char* imagePath) {
    vector<pair<string, CGRect>> results;
    tesseract::TessBaseAPI api;
    if (api.Init("/opt/homebrew/share/tessdata", "chi_sim") != 0) {
        cerr << "无法初始化 Tesseract." << endl;
        return results;
    }

    Pix *image = pixRead(imagePath);
    if (!image) {
        cerr << "无法打开输入图像：" << imagePath << endl;
        return results;
    }
    api.SetImage(image);

    // 获取 OCR 文本（仅用于配合 bounding box 个数）
    char* outText = api.GetUTF8Text();
    istringstream iss(outText);
    vector<string> lines;
    string line;
    while (getline(iss, line))
        lines.push_back(line);
    delete[] outText;

    // 获取每行文本的 bounding box
    Boxa* boxes = api.GetComponentImages(tesseract::RIL_TEXTLINE, true, nullptr, nullptr);
    if (!boxes) {
        cerr << "无法获取文本区域." << endl;
        api.End();
        pixDestroy(&image);
        return results;
    }
    int boxCount = boxaGetCount(boxes);
    for (size_t i = 0; i < lines.size() && i < static_cast<size_t>(boxCount); ++i) {
        Box* box = boxaGetBox(boxes, i, L_CLONE);
        if (!box) continue;
        l_int32 x, y, w, h;//y是文字格子左上角
        boxGetGeometry(box, &x, &y, &w, &h);
        // 注意：这里根据实际需要调整 rect 计算，下面取文本框中心坐标
        CGRect rect = CGRectMake(x, y, w, h);
        results.emplace_back(lines[i], rect);
        //cout<<"x:"<<x<<"\ty:"<<y<<"\ty+h:"<<y+h<<"\n";
        boxDestroy(&box);
    }
    api.End();
    boxaDestroy(&boxes);
    pixDestroy(&image);
    return results;
}
*/
// ========== 查找“翻译/翻譯”菜单，直接使用 OCR 返回的文字 bounding box 中心坐标 ==========
CGPoint findTranslateOption(const vector<pair<string, CGRect>>& ocrResults) {
    for (const auto& [text, rect] : ocrResults) {
        // 只要包含 “翻” 就认为匹配
        if (text.find("翻") != string::npos) {
            CGPoint result;
            result.x = rect.origin.x + rect.size.width / 2;
            result.y = rect.origin.y + rect.size.height / 2;
            cout<<result.x<<" "<<result.y<<"\n";
            return result;
        }
    }
    return CGPoint{-1, -1};
}

// ========== 主自动化流程 ==========
void processAutomation() {
    CGPoint startPos = getMousePosition();
    rightClick(startPos);
    this_thread::sleep_for(chrono::milliseconds(10));

    // 截图区域，根据实际情况设置菜单的起始位置及尺寸
    CGPoint menuTopLeft = { startPos.x + Xoffset / BILLY, startPos.y - Yoffset / BILLY };
    captureScreen(menuTopLeft, Xlength / BILLY, Ylength / BILLY, "./menu.png");

    vector<pair<string, CGRect>> ocrResults = runTesseractOCRWithOpenCV("./menu.png");
    if (!ocrResults.empty()) {
        CGPoint translatePos = findTranslateOption(ocrResults);
/*
        if (translatePos.x != -1) {
            cout << "[自动化] 找到‘翻译/翻譯’菜单，点击..." << endl;
            //translatePos.x = translatePos.x/BILLY + startPos.x+38;
            translatePos.x = startPos.x+40.0 / BILLY;
            int offset=startPos.y - Yoffset/BILLY;
            offset=offset<0?0:offset;
            translatePos.y = translatePos.y/BILLY + offset;
            //cout<<"start x: "<<startPos.x<<"\ttranslatePos x: "<<translatePos.x<<"\n";
            //cout<<"start y: "<<startPos.y<<"\ttranslatePos y: "<<translatePos.y<<"\n";
            moveMouse(translatePos);
            //leftClick(translatePos);
            moveMouse(startPos);
        } else {
            cout << "[自动化] 未找到‘翻译/翻譯’，结束。" << endl;
        }
    } else {
        cout << "[自动化] OCR 识别失败。" << endl;
    }
*/
        if (translatePos.x != -1) {
            cout << "[自动化] 找到‘翻译/翻譯’菜单，点击..." << endl;
            // 输出 OCR识别到的原始坐标
            cout << "[调试] OCR识别的原始坐标: (" << translatePos.x << ", " << translatePos.y << ")" << endl;
            // 输出起始鼠标坐标
            cout << "[调试] 鼠标起始位置: (" << startPos.x << ", " << startPos.y << ")" << endl;

            // 修改 x 坐标
            translatePos.x = startPos.x + (40.0 / BILLY);
            cout << "[调试] 修改后的 translatePos.x: " << translatePos.x << endl;

            // 计算偏移量，注意使用括号明确运算顺序
            int offset = startPos.y - (Yoffset / BILLY);
            offset = (offset < 0 ? 0 : offset);
            cout << "[调试] 计算的偏移量: " << offset << endl;

            // 修改 y 坐标
            translatePos.y = (translatePos.y / BILLY) + offset;
            cout << "[调试] 修改后的 translatePos.y: " << translatePos.y << endl;

            //moveMouse(translatePos);
            leftClick(translatePos);
            moveMouse(startPos);
        } else {
            cout << "[自动化] 未找到‘翻译/翻譯’，结束。" << endl;
        }    
        } else {
            cout << "[自动化] OCR 识别失败。" << endl;
        }
}

// ========== 全局快捷键绑定 ==========
int main() {
    EventHotKeyRef hotKeyRef;
    EventHotKeyID hotKeyID = { 'htk1', 1 };
    RegisterEventHotKey(kVK_ANSI_A, cmdKey | shiftKey, hotKeyID,
                          GetApplicationEventTarget(), 0, &hotKeyRef);

    EventTypeSpec eventType = { kEventClassKeyboard, kEventHotKeyPressed };
    InstallApplicationEventHandler([](EventHandlerCallRef, EventRef, void*) -> OSStatus {
        cout << "[快捷键] CMD + SHIFT + A 触发，执行自动化..." << endl;
        processAutomation();
        return noErr;
    }, 1, &eventType, nullptr, nullptr);

    cout << "程序已启动，按下 CMD + SHIFT + A 触发自动化..." << endl;
    // 简单事件循环替代 RunApplicationEventLoop()
    while (true) {
        EventRef event;
        if (ReceiveNextEvent(0, nullptr, kEventDurationForever, true, &event) == noErr) {
            SendEventToEventTarget(event, GetApplicationEventTarget());
            ReleaseEvent(event);
        }
    }
    return 0;
}