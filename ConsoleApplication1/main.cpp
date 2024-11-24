#include <windows.h>

// 图像宽度和高度
const int WIDTH = 500;
const int HEIGHT = 500;

// 图像数据缓冲区（每像素用 4 字节表示：BGRA）
unsigned char image[WIDTH * HEIGHT * 4];

// 初始化图像数据
void InitializeImage() {
	for (int y = 0; y < HEIGHT; ++y) {
		for (int x = 0; x < WIDTH; ++x) {
			int index = (y * WIDTH + x) * 4;
			image[index + 0] = (unsigned char)(x / (double)WIDTH * 255); // Blue
			image[index + 1] = (unsigned char)(y / (double)HEIGHT * 255); // Green
			image[index + 2] = 0;                                  // Red
			image[index + 3] = 255;                                // Alpha
		}
	}
}

// 绘制图像到窗口
void DrawImage(HWND hwnd) {
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);

	// 创建兼容的设备上下文和位图
	HDC memDC = CreateCompatibleDC(hdc);
	BITMAPINFO bmi = {};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = WIDTH;
	bmi.bmiHeader.biHeight = -HEIGHT; // 负值表示位图从上到下绘制
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	// 创建 DIB 位图并设置像素数据
	HBITMAP hBitmap = CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_INIT, image, &bmi, DIB_RGB_COLORS);
	SelectObject(memDC, hBitmap);

	// 将位图绘制到窗口
	BitBlt(hdc, 0, 0, WIDTH, HEIGHT, memDC, 0, 0, SRCCOPY);

	// 释放资源
	DeleteObject(hBitmap);
	DeleteDC(memDC);

	EndPaint(hwnd, &ps);
}

// 窗口过程函数
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_PAINT:
		DrawImage(hwnd);
		return 0;

	case WM_KEYDOWN:
		if (wParam == VK_SPACE) {
			// 修改图像数据
			for (int i = 0; i < WIDTH * HEIGHT * 4; i += 4) {
				image[i + 0] = rand() % 256; // Blue
				image[i + 1] = rand() % 256; // Green
				image[i + 2] = rand() % 256; // Red
			}
			InvalidateRect(hwnd, nullptr, TRUE); // 请求重绘
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// 主函数
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	const wchar_t CLASS_NAME[] = L"ImageWindowClass";

	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);

	HWND hwnd = CreateWindowEx(
		0,                              // 扩展样式
		CLASS_NAME,                     // 窗口类名
		L"Image Viewer",                 // 窗口标题
		WS_OVERLAPPEDWINDOW,            // 窗口样式
		CW_USEDEFAULT, CW_USEDEFAULT,   // 初始位置
		WIDTH + 16, HEIGHT + 39,        // 初始大小（包含窗口边框）
		nullptr, nullptr, hInstance, nullptr);

	if (hwnd == nullptr) {
		return 0;
	}

	InitializeImage();
	ShowWindow(hwnd, nCmdShow);

	// 消息循环
	MSG msg = {};
	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}