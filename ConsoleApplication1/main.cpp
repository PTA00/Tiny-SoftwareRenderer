#include <windows.h>
#include <limits>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <fstream>
#include <cmath>
#include <array>

// 图像宽度和高度
const int WIDTH = 500;
const int HEIGHT = 500;
int 超边界计数 = 0;

struct Color {
	uint8_t blue;  // 蓝色分量
	uint8_t green; // 绿色分量
	uint8_t red;   // 红色分量
	uint8_t alpha; // Alpha（透明度）分量
};

// 图像数据缓冲区（每像素用 4 字节表示：BGRA）
struct Image {
	Color* image;
	int width;
	int height;

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

struct vec2i {
	int x, y;
	/*vec2i() : x(0), y(0) {}
	vec2i(int _x, int _y) : x(_x), y(_y) {}*/
};
//struct Triangle2i {
//	vec2i vertices[3]; // 顶点坐标 (x, y, z)
//};
struct vec3 {
	double x, y, z;
};
// 三角形数据结构
struct Triangle {
	vec3 vertices[3]; // 顶点坐标 (x, y, z)
	vec3 normals[3];  // 法向量 (nx, ny, nz)
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

// 读取二进制 STL 文件的函数
std::vector<Triangle> readBinarySTL(const std::string& filename) {
	std::ifstream file(filename, std::ios::binary);
	if (!file) {
		throw std::runtime_error("无法打开文件: " + filename);
	}

	// 跳过 80 字节文件头
	char header[80] = { 0 };
	file.read(header, 80);

	// 读取三角形数量
	uint32_t triangleCount = 0;
	file.read(reinterpret_cast<char*>(&triangleCount), sizeof(uint32_t));
	std::cout << "文件中记录的三角形数量: " << triangleCount << std::endl;

	std::vector<Triangle> triangles;
	triangles.reserve(triangleCount); // 预先分配内存

	// 逐个读取三角形数据
	for (uint32_t i = 0; i < triangleCount; ++i) {
		Triangle triangle;

		// 每个 STL 三角形的法向量是统一的，将其复制到 normals
		float normal[3];
		if (!file.read(reinterpret_cast<char*>(normal), sizeof(normal))) {
			throw std::runtime_error("读取三角形法向量失败，文件可能已损坏或格式错误。");
		}

		for (int j = 0; j < 3; ++j) {
			triangle.normals[j].x = normal[0];
			triangle.normals[j].y = normal[1];
			triangle.normals[j].z = normal[2];
		}

		// 读取三个顶点
		float vertex[3];
		for (int j = 0; j < 3; ++j) {
			if (!file.read(reinterpret_cast<char*>(vertex), sizeof(vertex))) {
				throw std::runtime_error("读取三角形顶点失败，文件可能已损坏或格式错误。");
			}
			triangle.vertices[j].x = vertex[0];
			triangle.vertices[j].y = vertex[1];
			triangle.vertices[j].z = vertex[2];
		}

		// 跳过属性字节
		uint16_t attribute = 0;
		if (!file.read(reinterpret_cast<char*>(&attribute), sizeof(attribute))) {
			throw std::runtime_error("读取属性字节失败，文件可能已损坏或格式错误。");
		}

		triangles.push_back(triangle);
	}

	// 检查是否有多余数据
	if (file.peek() != EOF) {
		throw std::runtime_error("读取结束后文件仍有多余数据，文件格式可能不正确。");
	}

	// 检查实际读取的三角形数量是否与文件头中记录的数量一致
	if (triangles.size() != triangleCount) {
		throw std::runtime_error(
			"实际读取的三角形数量 (" + std::to_string(triangles.size()) +
			") 与文件头记录的数量 (" + std::to_string(triangleCount) + ") 不一致。"
		);
	}

	return triangles;
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
			if (!(vertexStream >> triangle.vertices[i].x >> triangle.vertices[i].y >> triangle.vertices[i].z)) {
				throw std::runtime_error("文件格式错误: 顶点数据不完整");
			}
			std::getline(file, line);
			if (line.empty()) {
				throw std::runtime_error("文件格式错误: 缺少法向量数据");
			}
			std::istringstream normalStream(line);
			if (!(normalStream >> triangle.normals[i].x >> triangle.normals[i].y >> triangle.normals[i].z)) {
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

// 获取包围盒函数(顺序: min, max)
static std::array<vec3, 2> getBBox2(const std::vector<Triangle> list) {
	// 初始化包围盒的最小值和最大值
	vec3 bboxMin = { (std::numeric_limits<int>::max)(), (std::numeric_limits<int>::max)(), (std::numeric_limits<int>::max)() };
	vec3 bboxMax = { (std::numeric_limits<int>::min)(), (std::numeric_limits<int>::min)(), (std::numeric_limits<int>::min)() };

	for (auto& t : list) {
		for (int i = 0; i < 3; i++) {
			bboxMin.x = (std::min)((float)bboxMin.x, (float)t.vertices[i].x);
			bboxMin.y = (std::min)((float)bboxMin.y, (float)t.vertices[i].y);
			bboxMin.z = (std::min)((float)bboxMin.z, (float)t.vertices[i].z);

			bboxMax.x = (std::max)((float)bboxMax.x, (float)t.vertices[i].x);
			bboxMax.y = (std::max)((float)bboxMax.y, (float)t.vertices[i].y);
			bboxMax.z = (std::max)((float)bboxMax.z, (float)t.vertices[i].z);
		}
	}

	return { bboxMin, bboxMax };
}

// 绘制三角形
void drawTriangle(Image image, const Triangle triangle, Color color) {
	// 四舍五入转换
	line(round(triangle.vertices[0].x), round(triangle.vertices[0].y), round(triangle.vertices[1].x), round(triangle.vertices[1].y), image, color);
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
	//std::vector<Triangle> cc = read3DFile("C:\\Users\\PTA00\\Desktop\\teapot_surface0.norm.txt");
	std::vector<Triangle> cc = readBinarySTL("C:\\Users\\PTA00\\Desktop\\75-55-2-8.STL");

	std::array<vec3, 2> ret = getBBox2(cc);
	std::cout << "min: " << ret[0].x << " " << ret[0].y << " " << ret[0].z << std::endl;
	std::cout << "max: " << ret[1].x << " " << ret[1].y << " " << ret[1].z << std::endl;
	for (auto& t : cc) {
		drawTriangle(image, t, { 0, 0, 0, 255 });
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