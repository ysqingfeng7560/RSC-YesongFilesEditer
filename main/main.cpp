#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>
#include <fstream>
#include <vector>
#include <string>

// 链接库（Dev-C++ 需手动在编译选项中加 -lgdi32 -lcomdlg32 -lshell32）
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")

const unsigned char KEY = 0x5A;

// 全局控件句柄
HWND hEditEncrypt = NULL;
HWND hEditDecrypt = NULL;
HWND hBtnBrowseEnc = NULL;
HWND hBtnBrowseDec = NULL;
HWND hBtnProcessEnc = NULL;
HWND hBtnProcessDec = NULL;

// 版权区域位置（用于点击检测）
RECT rcCopyright1 = {0};
RECT rcCopyright2 = {0};

std::string getExtension(const std::string& filename) {
    if (filename.find_last_of('.') != std::string::npos) {
        std::string ext = filename.substr(filename.find_last_of('.') + 1);
        for (size_t i = 0; i < ext.length(); ++i)
            ext[i] = tolower(ext[i]);
        return ext;
    }
    return "";
}

std::vector<unsigned char> readFile(const char* filename) {
    std::ifstream in(filename, std::ios::binary);
    in.seekg(0, std::ios::end);
    long size = in.tellg();
    in.seekg(0, std::ios::beg);
    std::vector<unsigned char> data(size);
    in.read((char*)&data[0], size);
    return data;
}

void writeFile(const char* filename, const std::vector<unsigned char>& data) {
    std::ofstream out(filename, std::ios::binary);
    out.write((const char*)&data[0], data.size());
}

void xorCrypt(std::vector<unsigned char>& data) {
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] ^= KEY;
    }
}

void processFile(const std::string& filePath) {
    std::string ext = getExtension(filePath);
    std::vector<unsigned char> data;

    try {
        data = readFile(filePath.c_str());
    } catch (...) {
        MessageBoxA(NULL, "无法读取文件！", "错误", MB_ICONERROR);
        return;
    }

    xorCrypt(data);

    std::string outputPath;
    if (ext == "txt") {
        outputPath = filePath.substr(0, filePath.length() - 4) + ".yesong";
    } else if (ext == "yesong") {
        outputPath = filePath.substr(0, filePath.length() - 8) + ".txt";
    } else {
        MessageBoxA(NULL, "仅支持 .txt 或 .yesong 文件！", "提示", MB_ICONINFORMATION);
        return;
    }

    try {
        writeFile(outputPath.c_str(), data);
    } catch (...) {
        MessageBoxA(NULL, "无法写入输出文件！", "错误", MB_ICONERROR);
        return;
    }

    std::string msg = "已处理:\n" + outputPath;
    MessageBoxA(NULL, msg.c_str(), "成功", MB_ICONINFORMATION);
}

// 打开文件对话框
std::string openFileDialog(HWND hwnd, const char* filter) {
    OPENFILENAMEA ofn = {0};
    char szFile[MAX_PATH] = {0};

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn)) {
        return std::string(szFile);
    }
    return "";
}

// 处理按钮点击
void onBrowseEncrypt(HWND hwnd) {
    std::string path = openFileDialog(hwnd, "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0");
    if (!path.empty()) {
        SetWindowTextA(hEditEncrypt, path.c_str());
    }
}

void onBrowseDecrypt(HWND hwnd) {
    std::string path = openFileDialog(hwnd, "YeSong Files (*.yesong)\0*.yesong\0All Files (*.*)\0*.*\0");
    if (!path.empty()) {
        SetWindowTextA(hEditDecrypt, path.c_str());
    }
}

void onProcessEncrypt() {
    char buffer[MAX_PATH];
    GetWindowTextA(hEditEncrypt, buffer, MAX_PATH);
    if (strlen(buffer) > 0) {
        processFile(std::string(buffer));
    } else {
        MessageBoxA(NULL, "请输入或选择一个 .txt 文件！", "提示", MB_ICONINFORMATION);
    }
}

void onProcessDecrypt() {
    char buffer[MAX_PATH];
    GetWindowTextA(hEditDecrypt, buffer, MAX_PATH);
    if (strlen(buffer) > 0) {
        processFile(std::string(buffer));
    } else {
        MessageBoxA(NULL, "请输入或选择一个 .yesong 文件！", "提示", MB_ICONINFORMATION);
    }
}

// 绘制版权信息（在 WM_PAINT 中调用）
void drawCopyright(HDC hdc, int width) {
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0, 0, 255)); // 蓝色

    const char* text1 = "ysqingfeng7560@github.com";
    const char* text2 = "Rising Community (RSC)";

    SIZE sz1, sz2;
    GetTextExtentPoint32A(hdc, text1, strlen(text1), &sz1);
    GetTextExtentPoint32A(hdc, text2, strlen(text2), &sz2);

    int x1 = 10;
    int x2 = width - sz2.cx - 10;
    int y = 260; // 距离底部约 20px（窗口高 300）

    TextOutA(hdc, x1, y, text1, strlen(text1));
    TextOutA(hdc, x2, y, text2, strlen(text2));

    // 记录区域用于点击检测
    rcCopyright1.left = x1;
    rcCopyright1.top = y;
    rcCopyright1.right = x1 + sz1.cx;
    rcCopyright1.bottom = y + sz1.cy;

    rcCopyright2.left = x2;
    rcCopyright2.top = y;
    rcCopyright2.right = x2 + sz2.cx;
    rcCopyright2.bottom = y + sz2.cy;
}

const char CLASS_NAME[] = "YeSongToolWindow";

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            {
                // 创建控件（坐标基于 600x300 窗口）
                CreateWindowA("STATIC", ".yesong 信息交流文件密钥操作平台",
                              WS_CHILD | WS_VISIBLE | SS_CENTER,
                              0, 10, 600, 30, hwnd, NULL, NULL, NULL);

                CreateWindowA("STATIC", "拖动文件至此窗口以实现自动加密/解密转换",
                              WS_CHILD | WS_VISIBLE | SS_CENTER,
                              0, 40, 600, 20, hwnd, NULL, NULL, NULL);

                // 加密部分
                CreateWindowA("STATIC", "加密源文件 (.txt):", WS_CHILD | WS_VISIBLE,
                              30, 80, 150, 20, hwnd, NULL, NULL, NULL);
                hEditEncrypt = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                             180, 80, 300, 20, hwnd, NULL, NULL, NULL);
                hBtnBrowseEnc = CreateWindowA("BUTTON", "浏览...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                              490, 80, 60, 20, hwnd, (HMENU)1001, NULL, NULL);
                hBtnProcessEnc = CreateWindowA("BUTTON", "执行加密", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                               560, 80, 70, 20, hwnd, (HMENU)1003, NULL, NULL);

                // 解密部分
                CreateWindowA("STATIC", "解密源文件 (.yesong):", WS_CHILD | WS_VISIBLE,
                              30, 120, 150, 20, hwnd, NULL, NULL, NULL);
                hEditDecrypt = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                             180, 120, 300, 20, hwnd, NULL, NULL, NULL);
                hBtnBrowseDec = CreateWindowA("BUTTON", "浏览...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                              490, 120, 60, 20, hwnd, (HMENU)1002, NULL, NULL);
                hBtnProcessDec = CreateWindowA("BUTTON", "执行解密", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                               560, 120, 70, 20, hwnd, (HMENU)1004, NULL, NULL);

                DragAcceptFiles(hwnd, TRUE);
            }
            break;

        case WM_COMMAND:
            {
                int id = LOWORD(wParam);
                if (id == 1001) {
                    onBrowseEncrypt(hwnd);
                } else if (id == 1002) {
                    onBrowseDecrypt(hwnd);
                } else if (id == 1003) {
                    onProcessEncrypt();
                } else if (id == 1004) {
                    onProcessDecrypt();
                }
            }
            break;

        case WM_DROPFILES:
            {
                HDROP hDrop = (HDROP)wParam;
                UINT count = DragQueryFileA(hDrop, 0xFFFFFFFF, NULL, 0);
                for (UINT i = 0; i < count; ++i) {
                    char buffer[MAX_PATH];
                    if (DragQueryFileA(hDrop, i, buffer, MAX_PATH)) {
                        processFile(std::string(buffer));
                    }
                }
                DragFinish(hDrop);
            }
            break;

        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                RECT rc;
                GetClientRect(hwnd, &rc);
                SetBkColor(hdc, RGB(255, 255, 255));
                ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

                drawCopyright(hdc, rc.right);

                EndPaint(hwnd, &ps);
            }
            break;

        case WM_LBUTTONDOWN:
            {
                POINT pt = {LOWORD(lParam), HIWORD(lParam)};
                if (PtInRect(&rcCopyright1, pt)) {
                    ShellExecuteA(NULL, "open", "https://github.com/ysqingfeng7560", NULL, NULL, SW_SHOWNORMAL);
                } else if (PtInRect(&rcCopyright2, pt)) {
                    ShellExecuteA(NULL, "open", "https://github.com/ysqingfeng7560/rsc-RisingCommunity", NULL, NULL, SW_SHOWNORMAL);
                }
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        ".yesong 信息交流文件密钥操作平台",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 650, 300,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        MessageBoxA(NULL, "创建窗口失败！", "错误", MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
