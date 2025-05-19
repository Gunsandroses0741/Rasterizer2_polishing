#pragma once // 防止头文件被重复包含

#include <cstdint>   // 包含标准整型类型定义，如 std::uint8_t, std::uint16_t
#include <fstream>   // 包含文件流操作类，如 std::ifstream, std::ofstream
#include <vector>    // 包含 std::vector 动态数组容器
#include <iostream>  // 包含标准输入输出流，如 std::cerr
#include <algorithm> // 包含算法函数，如 std::max, std::min

#include "geometry.h" // 包含自定义的几何类 (如 Vec3f，TGAColor中可能用到)

// 结构体内存对齐指令，确保TGA_Header结构体按照1字节对齐
// 这对于直接读写文件头至关重要，以避免因编译器默认对齐方式导致的兼容性问题
#pragma pack(push,1)
// TGA文件头结构体定义，共18字节
struct TGA_Header {
	std::uint8_t  idlength{};        // 图像ID字段的长度，通常为0
	std::uint8_t  colormaptype{};    // 颜色表类型，0表示无颜色表，1表示有颜色表
	std::uint8_t  datatypecode{};    // 图像数据类型码，例如 2=未压缩RGB, 3=未压缩灰度, 10=RLE压缩RGB, 11=RLE压缩灰度
	std::uint16_t colormaporigin{};  // 颜色表首个条目的索引，如果colormaptype为0则忽略
	std::uint16_t colormaplength{};  // 颜色表中的条目数量
	std::uint8_t  colormapdepth{};   // 每个颜色表条目的位数 (例如 15, 16, 24, 32)
	std::uint16_t x_origin{};        // 图像左下角的X坐标
	std::uint16_t y_origin{};        // 图像左下角的Y坐标
	std::uint16_t width{};           // 图像宽度 (像素)
	std::uint16_t height{};          // 图像高度 (像素)
	std::uint8_t  bitsperpixel{};    // 每个像素的位数 (bpp)，例如 8, 16, 24, 32
	std::uint8_t  imagedescriptor{}; // 图像描述符字节，包含alpha通道深度和图像原点位置信息
	                                 // bit 0-3: alpha通道位数 (或属性位数)
	                                 // bit 4: 0=从左到右排序, 1=从右到左排序
	                                 // bit 5: 0=从下到上排序(左下角原点), 1=从上到下排序(左上角原点)
	                                 // bit 6-7: 必须为0
};
#pragma pack(pop) // 恢复之前的内存对齐设置

// TGA颜色结构体
struct TGAColor {
	// 存储颜色数据，顺序为 B, G, R, A (低字节在前)
	// TGA格式通常以BGR(A)顺序存储像素数据
	std::uint8_t bgra[4] = { 0,0,0,0 };
	std::uint8_t bytespp = { 0 }; // 每个像素的字节数 (Bytes Per Pixel)

	TGAColor() = default; // 默认构造函数

	// 构造函数：通过R, G, B, A分量创建颜色 (A默认为255，不透明)
	TGAColor(const std::uint8_t R, const std::uint8_t G, const std::uint8_t B, const std::uint8_t A = 255) : bgra{ B,G,R,A }, bytespp(4) { }

	// 构造函数：创建灰度颜色 (单通道，通常用于GRAYSCALE格式)
	TGAColor(const std::uint8_t v) : bgra{ v,0,0,0 }, bytespp(1) { }

	// 构造函数：从指向原始像素数据的指针p和每个像素的字节数bpp创建颜色
	TGAColor(const std::uint8_t *p, const std::uint8_t bpp) : bgra{ 0,0,0,0 }, bytespp(bpp) {
		for (int i = 0; i < bpp; i++)
			bgra[i] = p[i]; // 复制bpp个字节到bgra数组
	}

	// 重载 [] 操作符，方便访问bgra数组中的颜色分量
	std::uint8_t& operator[](const int i) { return bgra[i]; }

	// 重载 * 操作符，用于颜色与强度值(0.0-1.0)相乘，调整颜色亮度
	TGAColor operator *(const double intensity) const {
		TGAColor res = *this; // 复制当前颜色对象
		double clamped = std::max(0., std::min(intensity, 1.)); // 将强度值限制在 [0, 1] 区间
		for (int i = 0; i < 4; i++) res.bgra[i] = bgra[i] * clamped; // 各分量乘以强度值
		return res;
	}

	// 将BGRA颜色转换为Vec3f类型的RGB颜色 (忽略Alpha)
	Vec3f rgb() const {
		return Vec3f(bgra[2], bgra[1], bgra[0]); // R=bgra[2], G=bgra[1], B=bgra[0]
	}
};

// TGA图像处理类
class TGAImage {
protected:
	std::vector<std::uint8_t> data; // 存储图像所有像素数据的动态数组 (原始字节流)
	int width;    // 图像宽度 (像素)
	int height;   // 图像高度 (像素)
	int bytespp;  // 每个像素的字节数 (Bytes Per Pixel)

	// 私有辅助函数：从文件流中加载RLE (Run-Length Encoding) 压缩的数据
	bool   load_rle_data(std::ifstream &in);
	// 私有辅助函数：将图像数据以RLE压缩格式写入文件流
	bool unload_rle_data(std::ofstream &out) const;
public:
	// TGA图像格式枚举
	enum Format { GRAYSCALE = 1, RGB = 3, RGBA = 4 }; // 分别对应每像素1、3、4字节

	TGAImage(); // 默认构造函数
	TGAImage(const int w, const int h, const int bpp); // 带参构造函数，创建指定尺寸和bpp的空图像

	// 从指定的TGA文件读取图像数据
	bool  read_tga_file(const std::string filename);
	// 将图像数据写入指定的TGA文件
	// vflip: 是否垂直翻转图像 (默认为true，以匹配常见坐标系)
	// rle: 是否使用RLE压缩 (默认为true)
	bool write_tga_file(const std::string filename, const bool vflip = true, const bool rle = true) const;

	void flip_horizontally(); // 水平翻转图像
	void flip_vertically();   // 垂直翻转图像

	// 缩放图像到指定的宽度w和高度h (使用最近邻插值)
	void scale(const int w, const int h);

	// 获取指定坐标(x, y)处的像素颜色
	TGAColor get(const int x, const int y) const;
	// 设置指定坐标(x, y)处的像素颜色为c
	void set(const int x, const int y, const TGAColor &c);

	int get_width() const;  // 获取图像宽度
	int get_height() const; // 获取图像高度
	int get_bytespp();    // 获取每个像素的字节数

	// 返回指向图像原始像素数据缓冲区的指针
	std::uint8_t *buffer();

	void clear(); // 清空图像数据 (所有像素置为0)
};
