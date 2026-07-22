#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>
#include <shlobj.h>
#include <fstream>
#include <vector>
#include <string>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")

const unsigned char KEY = 0x5A;

// 控件 ID
enum {
    IDC_EDIT_ENC_INPUT = 100,
    IDC_BTN_ENC_BROWSE,
    IDC_EDIT_ENC_OUTPUT_DIR,
    IDC_BTN_ENC_BROWSE_DIR,
    IDC_EDIT_ENC_FILENAME,
    IDC_BTN_ENC_PROCESS,

    IDC_EDIT_DEC_INPUT = 200,
    IDC_BTN_DEC_BROWSE,
    IDC_EDIT_DEC_OUTPUT_DIR,
    IDC_BTN_DEC_BROWSE_DIR,
    IDC_EDIT_DEC_FILENAME,
    IDC_BTN_DEC_PROCESS,

    IDC_STATIC_COPYRIGHT_LEFT,
    IDC_STATIC_COPYRIGHT_RIGHT
};

HWND hEditEncInput = NULL;
HWND hEditEncOutputDir = NULL;
HWND hEditEncFilename = NULL;
HWND hEditDecInput = NULL;
HWND hEditDecOutputDir = NULL;
HWND hEditDecFilename = NULL;

RECT rcCopyLeft = {0};
RECT rcCopyRight = {0};

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

std::string openFileDialog(HWND hwnd, const char* filter) {
    OPENFILENAMEA ofn = {0};
    char szFile[MAX_PATH] = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if (GetOpenFileNameA(&ofn)) return std::string(szFile);
    return "";
}

std::string selectFolderDialog(HWND hwnd) {
    BROWSEINFOA bi = {0};
    bi.hwndOwner = hwnd;
    bi.pszDisplayName = NULL;
    bi.lpszTitle = "选择输出文件夹";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (pidl) {
        char path[MAX_PATH];
        if (SHGetPathFromIDListA(pidl, path)) {
            CoTaskMemFree(pidl);
            return std::string(path);
        }
        CoTaskMemFree(pidl);
    }
    return "";
}

void processWithCustomOutput(const std::string& inputPath, const std::string& outputDir, const std::string& customName) {
    std::string ext = getExtension(inputPath);
    std::vector<unsigned char> data;
    try {
        data = readFile(inputPath.c_str());
    } catch (...) {
        MessageBoxA(NULL, "无法读取源文件！", "错误", MB_ICONERROR);
        return;
    }

    xorCrypt(data);

    std::string finalName;
    if (!customName.empty()) {
        // 用户指定了文件名
        finalName = customName;
        // 自动补全扩展名
        std::string nameExt = getExtension(customName);
        if (ext == "txt" && nameExt != "yesong") {
            finalName += ".yesong";
        } else if (ext == "yesong" && nameExt != "txt") {
            finalName += ".txt";
        }
    } else {
        // 自动生成
        std::string base = inputPath.substr(inputPath.find_last_of("\\/") + 1);
        if (ext == "txt") {
            finalName = base.substr(0, base.length() - 4) + ".yesong";
        } else {
            finalName = base.substr(0, base.length() - 8) + ".txt";
        }
    }

    std::string outputPath = outputDir;
    if (outputPath.back() != '\\' && outputPath.back() != '/') {
        outputPath += "\\";
    }
    outputPath += finalName;

    try {
        writeFile(outputPath.c_str(), data);
    } catch (...) {
        MessageBoxA(NULL, "无法写入输出文件！", "错误", MB_ICONERROR);
        return;
    }

    std::string msg = "处理成功！\n输出: " + outputPath;
    MessageBoxA(NULL, msg.c_str(), "成功", MB_ICONINFORMATION);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void createControls(HWND hwnd) {
    // 标题
    CreateWindowA("STATIC", ".yesong 信息交流文件密钥操作平台",
                  WS_CHILD | WS_VISIBLE | SS_CENTER | SS_SUNKEN,
                  0, 10, 700, 30, hwnd, NULL, NULL, NULL);

    // 提示
    CreateWindowA("STATIC", "提示：也可直接拖拽 .txt 或 .yesong 文件到窗口中自动处理",
                  WS_CHILD | WS_VISIBLE | SS_CENTER,
                  0, 45, 700, 20, hwnd, NULL, NULL, NULL);

    // === 加密区域 ===
    CreateWindowA("BUTTON", "加密 (.txt → .yesong)",
                  WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                  20, 70, 660, 100, hwnd, NULL, NULL, NULL);

    CreateWindowA("STATIC", "源文件:", WS_CHILD | WS_VISIBLE, 40, 95, 60, 20, hwnd, NULL, NULL, NULL);
    hEditEncInput = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                  100, 95, 380, 20, hwnd, (HMENU)IDC_EDIT_ENC_INPUT, NULL, NULL);
    CreateWindowA("BUTTON", "浏览...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                  490, 95, 60, 20, hwnd, (HMENU)IDC_BTN_ENC_BROWSE, NULL, NULL);
    CreateWindowA("BUTTON", "执行", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                  560, 95, 60, 20, hwnd, (HMENU)IDC_BTN_ENC_PROCESS, NULL, NULL);

    CreateWindowA("STATIC", "输出目录:", WS_CHILD | WS_VISIBLE, 40, 125, 60, 20, hwnd, NULL, NULL, NULL);
    hEditEncOutputDir = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                      100, 125, 380, 20, hwnd, (HMENU)IDC_EDIT_ENC_OUTPUT_DIR, NULL, NULL);
    CreateWindowA("BUTTON", "浏览...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                  490, 125, 60, 20, hwnd, (HMENU)IDC_BTN_ENC_BROWSE_DIR, NULL, NULL);

    CreateWindowA("STATIC", "文件名 (可选):", WS_CHILD | WS_VISIBLE, 40, 155, 90, 20, hwnd, NULL, NULL, NULL);
    hEditEncFilename = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                     130, 155, 250, 20, hwnd, (HMENU)IDC_EDIT_ENC_FILENAME, NULL, NULL);

    // === 解密区域 ===
    CreateWindowA("BUTTON", "解密 (.yesong → .txt)",
                  WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                  20, 180, 660, 100, hwnd, NULL, NULL, NULL);

    CreateWindowA("STATIC", "源文件:", WS_CHILD | WS_VISIBLE, 40, 205, 60, 20, hwnd, NULL, NULL, NULL);
    hEditDecInput = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                  100, 205, 380, 20, hwnd, (HMENU)IDC_EDIT_DEC_INPUT, NULL, NULL);
    CreateWindowA("BUTTON", "浏览...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                  490, 205, 60, 20, hwnd, (HMENU)IDC_BTN_DEC_BROWSE, NULL, NULL);
    CreateWindowA("BUTTON", "执行", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                  560, 205, 60, 20, hwnd, (HMENU)IDC_BTN_DEC_PROCESS, NULL, NULL);

    CreateWindowA("STATIC", "输出目录:", WS_CHILD | WS_VISIBLE, 40, 235, 60, 20, hwnd, NULL, NULL, NULL);
    hEditDecOutputDir = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                      100, 235, 380, 20, hwnd, (HMENU)IDC_EDIT_DEC_OUTPUT_DIR, NULL, NULL);
    CreateWindowA("BUTTON", "浏览...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                  490, 235, 60, 20, hwnd, (HMENU)IDC_BTN_DEC_BROWSE_DIR, NULL, NULL);

    CreateWindowA("STATIC", "文件名 (可选):", WS_CHILD | WS_VISIBLE, 40, 265, 90, 20, hwnd, NULL, NULL, NULL);
    hEditDecFilename = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                     130, 265, 250, 20, hwnd, (HMENU)IDC_EDIT_DEC_FILENAME, NULL, NULL);
}

void drawCopyright(HDC hdc, int width) {
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0, 0, 255));
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT hOld = (HFONT)SelectObject(hdc, hFont);

    const char* left = "ysqingfeng7560@github.com";
    const char* right = "Rising Community (RSC)";

    SIZE szL, szR;
    GetTextExtentPoint32A(hdc, left, strlen(left), &szL);
    GetTextExtentPoint32A(hdc, right, strlen(right), &szR);

    int y = 285; // 距离顶部
    TextOutA(hdc, 10, y, left, strlen(left));
    TextOutA(hdc, width - szR.cx - 10, y, right, strlen(right));

    // 更新点击区域
    rcCopyLeft = {10, y, 10 + szL.cx, y + szL.cy};
    rcCopyRight = {width - szR.cx - 10, y, width - 10, y + szR.cy};

    SelectObject(hdc, hOld);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            createControls(hwnd);
            DragAcceptFiles(hwnd, TRUE);
            break;

        case WM_COMMAND:
            {
                int id = LOWORD(wParam);
                switch (id) {
                    case IDC_BTN_ENC_BROWSE: {
                        std::string path = openFileDialog(hwnd, "Text Files (*.txt)\0*.txt\0");
                        if (!path.empty()) SetWindowTextA(hEditEncInput, path.c_str());
                        break;
                    }
                    case IDC_BTN_ENC_BROWSE_DIR: {
                        std::string dir = selectFolderDialog(hwnd);
                        if (!dir.empty()) SetWindowTextA(hEditEncOutputDir, dir.c_str());
                        break;
                    }
                    case IDC_BTN_ENC_PROCESS: {
                        char input[MAX_PATH], outputDir[MAX_PATH], filename[256];
                        GetWindowTextA(hEditEncInput, input, MAX_PATH);
                        GetWindowTextA(hEditEncOutputDir, outputDir, MAX_PATH);
                        GetWindowTextA(hEditEncFilename, filename, 256);
                        if (strlen(input) == 0) { MessageBoxA(hwnd, "请选择源文件！", "提示", MB_ICONWARNING); return 0; }
                        if (strlen(outputDir) == 0) { MessageBoxA(hwnd, "请选择输出目录！", "提示", MB_ICONWARNING); return 0; }
                        processWithCustomOutput(input, outputDir, filename);
                        break;
                    }

                    case IDC_BTN_DEC_BROWSE: {
                        std::string path = openFileDialog(hwnd, "YeSong Files (*.yesong)\0*.yesong\0");
                        if (!path.empty()) SetWindowTextA(hEditDecInput, path.c_str());
                        break;
                    }
                    case IDC_BTN_DEC_BROWSE_DIR: {
                        std::string dir = selectFolderDialog(hwnd);
                        if (!dir.empty()) SetWindowTextA(hEditDecOutputDir, dir.c_str());
                        break;
                    }
                    case IDC_BTN_DEC_PROCESS: {
                        char input[MAX_PATH], outputDir[MAX_PATH], filename[256];
                        GetWindowTextA(hEditDecInput, input, MAX_PATH);
                        GetWindowTextA(hEditDecOutputDir, outputDir, MAX_PATH);
                        GetWindowTextA(hEditDecFilename, filename, 256);
                        if (strlen(input) == 0) { MessageBoxA(hwnd, "请选择源文件！", "提示", MB_ICONWARNING); return 0; }
                        if (strlen(outputDir) == 0) { MessageBoxA(hwnd, "请选择输出目录！", "提示", MB_ICONWARNING); return 0; }
                        processWithCustomOutput(input, outputDir, filename);
                        break;
                    }
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
                        // 默认输出到原目录，无自定义名
                        std::string dir = buffer;
                        size_t pos = dir.find_last_of("\\/");
                        std::string outDir = (pos != std::string::npos) ? dir.substr(0, pos + 1) : ".";
                        processWithCustomOutput(buffer, outDir, "");
                    }
                }
                DragFinish(hDrop);
            }
            break;

        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                RECT rc; GetClientRect(hwnd, &rc);
                SetBkColor(hdc, RGB(255, 255, 255));
                ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
                drawCopyright(hdc, rc.right);
                EndPaint(hwnd, &ps);
            }
            break;

        case WM_LBUTTONDOWN:
            {
                POINT pt = {LOWORD(lParam), HIWORD(lParam)};
                if (PtInRect(&rcCopyLeft, pt)) {
                    ShellExecuteA(NULL, "open", "https://github.com/ysqingfeng7560", NULL, NULL, SW_SHOWNORMAL);
                } else if (PtInRect(&rcCopyRight, pt)) {
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
    wc.lpszClassName = "YeSongModern";
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        WS_EX_COMPOSITED, // 减少闪烁
        "YeSongModern",
        ".yesong 信息交流文件密钥操作平台",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 720, 340,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) return 1;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
