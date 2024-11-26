#include <windows.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <fstream>
#include <cmath>

// 图像宽度和高度
const int WIDTH = 500;
const int HEIGHT = 500;
uint32_t 超边界计数 = 0;

struct Color {
	uint8_t blue;  // 蓝色分量
	uint8_t green; // 绿色分量
	uint8_t red;   // 红色分量
	uint8_t alpha; // Alpha（透明度）分量
};

// 图像数据缓冲区（每像素用 4 字节表示：BGRA）
struct Image {
	Color* image;
	uint32_t width;
	uint32_t height;

	// 初始化图像数据
	Image(uint32_t width, uint32_t height) {
		this->width = width;
		this->height = height;
		image = new Color[width * height];
	}
	void set(int x, int y, Color color) {
		// 如果超出边界，则不绘制
		if (x < 0 || x >= width || y < 0 || y >= height) {
			超边界计数++;
			return;
		}
		this->image[y * width + x] = color;
	}
};

// 全局窗口图像数据
Image image(WIDTH, HEIGHT);

// 三角形数据结构
struct Triangle {
	float vertices[3][3]; // 顶点坐标 (x, y, z)
	float normals[3][3];  // 法向量 (nx, ny, nz)
};
struct Triangle_int {
	int vertices[3][3]; // 顶点坐标 (x, y, z)
};



static void line(int x0, int y0, int x1, int y1, Image image, Color color) {
	bool steep = false;
	if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
		std::swap(x0, y0);
		std::swap(x1, y1);
		steep = true;
	}
	if (x0 > x1) {
		std::swap(x0, x1);
		std::swap(y0, y1);
	}
	int dx = x1 - x0;
	int dy = y1 - y0;
	int derror2 = std::abs(dy) * 2;
	int error2 = 0;
	int y = y0;
	for (int x = x0; x <= x1; x++) {
		if (steep) {
			image.set(y, x, color);
		}
		else {
			image.set(x, y, color);
		}
		error2 += derror2;
		if (error2 > dx) {
			y += (y1 > y0 ? 1 : -1);
			error2 -= dx * 2;
		}
	}
}




// 绘制图像到窗口
void DrawImage(HWND hwnd, Image image) {
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);

	// 创建兼容的设备上下文和位图
	HDC memDC = CreateCompatibleDC(hdc);
	BITMAPINFO bmi = {};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = image.width;
	bmi.bmiHeader.biHeight = image.height; // 负值表示位图从上到下绘制
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	// 创建 DIB 位图并设置像素数据
	HBITMAP hBitmap = CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_INIT, image.image, &bmi, DIB_RGB_COLORS);
	SelectObject(memDC, hBitmap);

	// 将位图绘制到窗口
	BitBlt(hdc, 0, 0, image.width, image.height, memDC, 0, 0, SRCCOPY);

	// 释放资源
	DeleteObject(hBitmap);
	DeleteDC(memDC);

	EndPaint(hwnd, &ps);
}

// 读取 3D 文件的函数
std::vector<Triangle> read3DFile(const std::string& filename) {
	std::ifstream file(filename);
	if (!file.is_open()) {
		throw std::runtime_error("无法打开文件: " + filename);
	}

	int expectedTriangleCount = 0;
	file >> expectedTriangleCount;
	if (file.fail()) {
		throw std::runtime_error("文件格式错误: 无法读取三角形总数");
	}

	std::vector<Triangle> triangles;
	std::string line;
	std::getline(file, line); // 跳过当前行

	// 开始读取三角形数据
	while (std::getline(file, line)) {
		if (line.empty()) continue; // 跳过空行

		Triangle triangle;
		for (int i = 0; i < 3; ++i) {
			std::istringstream vertexStream(line);
			if (!(vertexStream >> triangle.vertices[i][0] >> triangle.vertices[i][1] >> triangle.vertices[i][2])) {
				throw std::runtime_error("文件格式错误: 顶点数据不完整");
			}
			std::getline(file, line);
			if (line.empty()) {
				throw std::runtime_error("文件格式错误: 缺少法向量数据");
			}
			std::istringstream normalStream(line);
			if (!(normalStream >> triangle.normals[i][0] >> triangle.normals[i][1] >> triangle.normals[i][2])) {
				throw std::runtime_error("文件格式错误: 法向量数据不完整");
			}
			std::getline(file, line); // 读取下一行，可能是顶点或空行
		}
		triangles.push_back(triangle);
	}

	// 检查三角形总数是否一致
	if (triangles.size() != expectedTriangleCount) {
		throw std::runtime_error("三角形数量不匹配: 声明的数量为 " + std::to_string(expectedTriangleCount) +
			", 实际读取到 " + std::to_string(triangles.size()));
	}

	return triangles;
}

// 窗口过程函数
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_PAINT:
		DrawImage(hwnd, image);
		return 0;

	case WM_KEYDOWN:
		if (wParam == VK_SPACE) {
			// 修改图像数据
			for (int i = 0; i < image.width * image.height; i++)
			{
				image.set(i % image.width, i / image.width, { 255, 0, 0, 255 });
			}

			InvalidateRect(hwnd, nullptr, TRUE); // 请求重绘
		}
		else if (wParam == 0x43) {// C键
			std::cout << "超边界计数:" << 超边界计数 << std::endl;
			超边界计数 = 0;
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
	AllocConsole();
	FILE* stream;
	freopen_s(&stream, "CON", "r", stdin);//重定向输入流
	freopen_s(&stream, "CON", "w", stdout);//重定向输入流


	const wchar_t CLASS_NAME[] = L"test";

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
	std::vector<Triangle> cc = read3DFile("C:\\Users\\PTA00\\Desktop\\teapot_surface0.norm.txt");
	std::cout << "三角形数量:" << cc.size() << std::endl;

	int n = 20;
	// 将模型向量放大n倍
	for (auto& triangle : cc) {
		for (int i = 0; i < 3; i++) {
			triangle.vertices[i][0] *= n;
			triangle.vertices[i][1] *= n;
			triangle.vertices[i][2] *= n;
		}
	}
	// 遍历获取xyz分别的最大值和最小值
	float maxX = INT_MIN, minX = INT_MAX;
	float maxY = INT_MIN, minY = INT_MAX;
	float maxZ = INT_MIN, minZ = INT_MAX;
	for (auto& triangle : cc) {
		for (int i = 0; i < 3; i++) {
			maxX = (std::max)(maxX, triangle.vertices[i][0]);
			minX = (std::min)(minX, triangle.vertices[i][0]);
			maxY = (std::max)(maxY, triangle.vertices[i][1]);
			minY = (std::min)(minY, triangle.vertices[i][1]);
			maxZ = (std::max)(maxZ, triangle.vertices[i][2]);
			minZ = (std::min)(minZ, triangle.vertices[i][2]);
		}
	}
	// 偏移=图片中心-最小值-（最大值-最小值）/2
	float offsetX = image.width / 2 - minX - (maxX - minX) / 2;
	float offsetY = image.height / 2 - minY - (maxY - minY) / 2;

	for (auto& triangle : cc) {
		// 设置xy临时变量，int类型
		int tempX1 = (int)round(triangle.vertices[0][0] + offsetX);
		int tempY1 = (int)round(triangle.vertices[0][1] + offsetY);
		int tempX2 = (int)round(triangle.vertices[1][0] + offsetX);
		int tempY2 = (int)round(triangle.vertices[1][1] + offsetY);
		line(tempX1, tempY1, tempX2, tempY2, image, { 0, 0, 0, 255 });
	}

	ShowWindow(hwnd, nCmdShow);

	// 消息循环
	MSG msg = {};
	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	FreeConsole();
	return 0;
}