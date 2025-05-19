#include <iostream>  // 用于标准输入输出流 (例如 std::cerr)
#include <fstream>   // 用于文件流操作 (例如 std::ifstream, std::ofstream)
#include <cstring>   // 包含 C 风格字符串处理函数 (例如 memcpy)
#include <string>    // 包含 std::string 类

#include "tgaimage.h" // 包含 TGAImage 类的声明和相关结构体定义

// TGAImage 默认构造函数
// 初始化成员变量为默认值，图像数据为空
TGAImage::TGAImage() : data(), width(0), height(0), bytespp(0) {}

// TGAImage 带参构造函数
// w: 图像宽度, h: 图像高度, bpp: 每个像素的字节数 (bytes per pixel)
// 初始化图像数据为一个大小为 w*h*bpp 的向量，所有元素填充为0 (黑色/透明)
TGAImage::TGAImage(const int w, const int h, const int bpp) : data(w*h*bpp, 0), width(w), height(h), bytespp(bpp) {}

// 从 TGA 文件读取图像数据
// filename: TGA 文件的路径
// 返回值: 如果读取成功返回 true，否则返回 false
bool TGAImage::read_tga_file(const std::string filename) {
	std::ifstream in; // 创建输入文件流对象
	in.open(filename, std::ios::binary); // 以二进制模式打开文件
	if (!in.is_open()) { // 检查文件是否成功打开
		std::cerr << "can't open file " << filename << "\n"; // 输出错误信息到标准错误流
		in.close(); // 关闭文件流
		return false; // 返回读取失败
	}
	TGA_Header header; // 创建 TGA 文件头结构体对象
	// 从文件流中读取 TGA 文件头数据，大小为 sizeof(header)
	in.read(reinterpret_cast<char *>(&header), sizeof(header));
	if (!in.good()) { // 检查读取操作是否成功
		in.close();
		std::cerr << "an error occured while reading the header\n";
		return false;
	}
	width = header.width;   // 从文件头获取图像宽度
	height = header.height;  // 从文件头获取图像高度
	bytespp = header.bitsperpixel >> 3; // 从每个像素的位数计算每个像素的字节数 (bitsperpixel / 8)
	// 检查图像尺寸和bpp值是否有效
	if (width <= 0 || height <= 0 || (bytespp != GRAYSCALE && bytespp != RGB && bytespp != RGBA)) {
		in.close();
		std::cerr << "bad bpp (or width/height) value\n";
		return false;
	}
	size_t nbytes = bytespp * width*height; // 计算图像数据总字节数
	data = std::vector<std::uint8_t>(nbytes, 0); // 初始化图像数据向量，大小为 nbytes，填充为0
	// 根据文件头中的 datatypecode 判断图像数据格式 (未压缩或RLE压缩)
	if (3 == header.datatypecode || 2 == header.datatypecode) { // 2: 未压缩RGB, 3: 未压缩灰度
		// 直接读取 nbytes 的图像数据到 data 向量中
		in.read(reinterpret_cast<char *>(data.data()), nbytes);
		if (!in.good()) {
			in.close();
			std::cerr << "an error occured while reading the data\n";
			return false;
		}
	}
	else if (10 == header.datatypecode || 11 == header.datatypecode) { // 10: RLE压缩RGB, 11: RLE压缩灰度
		// 调用 load_rle_data 方法加载RLE压缩数据
		if (!load_rle_data(in)) {
			in.close();
			std::cerr << "an error occured while reading the data\n";
			return false;
		}
	}
	else { // 不支持的 datatypecode
		in.close();
		std::cerr << "unknown file format " << (int)header.datatypecode << "\n";
		return false;
	}
	// 根据 imagedescriptor 中的标志位决定是否需要翻转图像
	// bit 5 (0x20): 0 = bottom-left origin, 1 = top-left origin
	// 如果 bit 5 为0 (即从下到上存储)，则需要垂直翻转以匹配常见的从上到下显示方式
	if (!(header.imagedescriptor & 0x20))
		flip_vertically();
	// bit 4 (0x10): 0 = left-to-right, 1 = right-to-left
	// 如果 bit 4 为1 (即从右到左存储)，则需要水平翻转
	if (header.imagedescriptor & 0x10)
		flip_horizontally();
	// 输出加载图像的尺寸和颜色深度信息
	std::cerr << width << "x" << height << "/" << bytespp * 8 << "\n";
	in.close(); // 关闭文件流
	return true; // 返回读取成功
}

// 从输入文件流加载 RLE (Run-Length Encoding) 压缩的 TGA 图像数据
// in: 输入文件流对象的引用
// 返回值: 如果加载成功返回 true，否则返回 false
bool TGAImage::load_rle_data(std::ifstream &in) {
	size_t pixelcount = width * height; // 总像素数量
	size_t currentpixel = 0; // 当前已处理的像素数量
	size_t currentbyte = 0;  // 当前已写入 data 向量的字节位置
	TGAColor colorbuffer; // 用于临时存储读取的颜色数据
	do {
		std::uint8_t chunkheader = 0; // RLE 包的头部字节
		chunkheader = in.get(); // 从文件流读取一个字节作为包头
		if (!in.good()) { // 检查读取操作是否成功
			std::cerr << "an error occured while reading the data\n";
			return false;
		}
		if (chunkheader < 128) { // 如果包头 < 128，表示这是一个 RAW 包 (原始数据包)
			chunkheader++; // 包头值 + 1 表示 RAW 包中包含的像素数量
			for (int i = 0; i < chunkheader; i++) { // 循环读取 chunkheader 个像素的数据
				// 读取一个像素的颜色数据 (bytespp 个字节) 到 colorbuffer
				in.read(reinterpret_cast<char *>(colorbuffer.bgra), bytespp);
				if (!in.good()) {
					std::cerr << "an error occured while reading the header\n";
					return false;
				}
				for (int t = 0; t < bytespp; t++) // 将读取的像素数据写入 data 向量
					data[currentbyte++] = colorbuffer.bgra[t];
				currentpixel++; // 已处理像素数增加
				if (currentpixel > pixelcount) { // 检查是否读取了过多的像素
					std::cerr << "Too many pixels read\n";
					return false;
				}
			}
		}
		else { // 如果包头 >= 128，表示这是一个 RLE 包 (行程编码包)
			chunkheader -= 127; // 包头值 - 127 表示 RLE 包中重复的像素数量
			// 读取一个像素的颜色数据，这个颜色将重复 chunkheader 次
			in.read(reinterpret_cast<char *>(colorbuffer.bgra), bytespp);
			if (!in.good()) {
				std::cerr << "an error occured while reading the header\n";
				return false;
			}
			for (int i = 0; i < chunkheader; i++) { // 循环写入 chunkheader 个重复的像素
				for (int t = 0; t < bytespp; t++) // 将重复的颜色数据写入 data 向量
					data[currentbyte++] = colorbuffer.bgra[t];
				currentpixel++; // 已处理像素数增加
				if (currentpixel > pixelcount) { // 检查是否读取了过多的像素
					std::cerr << "Too many pixels read\n";
					return false;
				}
			}
		}
	} while (currentpixel < pixelcount); // 继续处理直到所有像素都被读取
	return true; // RLE 数据加载成功
}

// 将图像数据写入 TGA 文件
// filename: 输出文件名
// vflip: 是否在写入前垂直翻转图像数据 (默认为 true)
// rle: 是否使用 RLE 压缩 (默认为 true)
// 返回值: 如果写入成功返回 true，否则返回 false

bool TGAImage::write_tga_file(const std::string filename, const bool vflip, const bool rle) const {
	// TGA 文件规范中定义的一些固定字段值
	std::uint8_t developer_area_ref[4] = { 0, 0, 0, 0 }; // 开发者区域引用，通常为0
	std::uint8_t extension_area_ref[4] = { 0, 0, 0, 0 }; // 扩展区域引用，通常为0
	// TGA 文件尾部标识 (Signature)
	std::uint8_t footer[18] = { 'T','R','U','E','V','I','S','I','O','N','-','X','F','I','L','E','.','\0' };
	std::ofstream out; // 创建输出文件流对象
	out.open(filename, std::ios::binary); // 以二进制模式打开文件
	if (!out.is_open()) { // 检查文件是否成功打开
		std::cerr << "can't open file " << filename << "\n";
		out.close();
		return false;
	}
	TGA_Header header; // 创建 TGA 文件头结构体对象
	// 填充文件头信息
	header.bitsperpixel = bytespp << 3; // 每个像素的位数 (bytespp * 8)
	header.width = width;     // 图像宽度
	header.height = height;   // 图像高度
	// 根据 bytespp 和 rle 标志设置 datatypecode
	header.datatypecode = (bytespp == GRAYSCALE ? (rle ? 11 : 3) : (rle ? 10 : 2));
	// 设置 imagedescriptor 的 bit 5，控制图像原点是左下角还是左上角
	// vflip 为 true (不翻转写入，即原点在左上) -> imagedescriptor 的 bit 5 为 1 (0x20)
	// vflip 为 false (翻转写入，即原点在左下) -> imagedescriptor 的 bit 5 为 0 (0x00)
	// 注意：这里 vflip 的含义与读取时的 flip_vertically() 逻辑相反。写入时 vflip=true 表示数据已经是想要的顺序，无需额外翻转，对应TGA的左下角原点(bit5=0)。
	// 如果 vflip=false，表示数据需要翻转成TGA标准的左下角原点，因此设置 bit5=0x20 (左上角原点)，让加载器去翻转它。
	// 通常，如果内部数据是左上角原点，写入时 vflip=true, header.imagedescriptor = 0x20 (表示数据是左上角，加载器不需要翻转)
	// 如果内部数据是左下角原点，写入时 vflip=true (表示数据就是左下角)，header.imagedescriptor = 0x00
	// 此处逻辑：vflip ? 0x00 : 0x20。如果 vflip 为 true，则设为 0x00 (左下角原点，表示数据已经是这个顺序)。
	header.imagedescriptor = vflip ? 0x00 : 0x20; // top-left or bottom-left origin
	out.write(reinterpret_cast<const char *>(&header), sizeof(header)); // 写入文件头
	if (!out.good()) { // 检查写入操作是否成功
		out.close();
		std::cerr << "can't dump the tga file\n";
		return false;
	}
	if (!rle) { // 如果不使用 RLE 压缩
		// 直接写入原始图像数据
		out.write(reinterpret_cast<const char *>(data.data()), width*height*bytespp);
		if (!out.good()) {
			std::cerr << "can't unload raw data\n";
			out.close();
			return false;
		}
	}
	else { // 如果使用 RLE 压缩
		// 调用 unload_rle_data 方法写入 RLE 压缩数据
		if (!unload_rle_data(out)) {
			out.close();
			std::cerr << "can't unload rle data\n";
			return false;
		}
	}
	// 写入开发者区域引用 (通常全为0)
	out.write(reinterpret_cast<const char *>(developer_area_ref), sizeof(developer_area_ref));
	if (!out.good()) {
		std::cerr << "can't dump the tga file\n";
		out.close();
		return false;
	}
	// 写入扩展区域引用 (通常全为0)
	out.write(reinterpret_cast<const char *>(extension_area_ref), sizeof(extension_area_ref));
	if (!out.good()) {
		std::cerr << "can't dump the tga file\n";
		out.close();
		return false;
	}
	// 写入 TGA 文件尾部标识
	out.write(reinterpret_cast<const char *>(footer), sizeof(footer));
	if (!out.good()) {
		std::cerr << "can't dump the tga file\n";
		out.close();
		return false;
	}
	out.close(); // 关闭文件流
	return true; // 文件写入成功
}

// 将图像数据以 RLE 压缩格式写入输出文件流
// out: 输出文件流对象的引用
// 返回值: 如果写入成功返回 true，否则返回 false
// 注意：此处的 RLE 压缩算法可能不是最优的，可以有改进空间
// 例如：对于只有两个相同像素的情况，不一定需要创建 RLE 包，直接作为 RAW 包可能更节省空间。
bool TGAImage::unload_rle_data(std::ofstream &out) const {
	const std::uint8_t max_chunk_length = 128; // RLE 包或 RAW 包中像素数量的最大值
	size_t npixels = width * height; // 总像素数
	size_t curpix = 0; // 当前已处理的像素索引
	while (curpix < npixels) { // 当还有未处理的像素时循环
		size_t chunkstart = curpix * bytespp; // 当前包的起始字节在 data 向量中的索引
		size_t curbyte = curpix * bytespp;    // 用于比较的当前字节索引
		std::uint8_t run_length = 1; // 当前行程的长度 (初始为1个像素)
		bool raw = true; // 标志当前包是否为 RAW 包 (默认为是)
		// 尝试找到连续相同的像素 (行程) 或连续不同的像素 (原始数据)
		while (curpix + run_length < npixels && run_length < max_chunk_length) {
			bool succ_eq = true; // 标志当前像素是否与下一个像素相同
			for (int t = 0; succ_eq && t < bytespp; t++)
				succ_eq = (data[curbyte + t] == data[curbyte + t + bytespp]); // 比较每个字节
			curbyte += bytespp; // 移动到下一个像素的起始字节
			if (1 == run_length) // 如果这是行程的第一个像素
				raw = !succ_eq; // 如果它与下一个像素不同，则认为是 RAW 包的开始；否则是 RLE 包的开始
			if (raw && succ_eq) { // 如果当前是 RAW 包，但发现了连续相同的像素
				run_length--; // 则 RAW 包到此结束，不包含这个相同的像素
				break;
			}
			if (!raw && !succ_eq) { // 如果当前是 RLE 包，但发现了不同的像素
				// RLE 包到此结束，不包含这个不同的像素
				break;
			}
			run_length++; // 扩展当前行程的长度
		}
		curpix += run_length; // 更新已处理的像素索引
		// 写入包头：RAW 包头 = run_length - 1，RLE 包头 = run_length + 127 (因为run_length存储的是实际像素数，而RLE包头用run_length-1+128)
		out.put(raw ? run_length - 1 : run_length + 127);
		if (!out.good()) { // 检查写入操作
			std::cerr << "can't dump the tga file\n";
			return false;
		}
		// 写入包数据：RAW 包写入 run_length 个像素的数据，RLE 包写入 1 个像素的数据
		out.write(reinterpret_cast<const char *>(data.data() + chunkstart), (raw ? run_length * bytespp : bytespp));
		if (!out.good()) { // 检查写入操作
			std::cerr << "can't dump the tga file\n";
			return false;
		}
	}
	return true; // RLE 数据写入成功
}

// 获取指定坐标 (x, y) 处的像素颜色
// x, y: 像素坐标
// 返回值: TGAColor 对象；如果坐标无效或图像数据为空，则返回默认构造的 TGAColor (通常是黑色透明)
TGAColor TGAImage::get(const int x, const int y) const {
	// 检查坐标是否有效以及图像数据是否存在
	if (!data.size() || x < 0 || y < 0 || x >= width || y >= height)
		return {}; // 返回一个默认构造的 TGAColor 对象
	// 计算像素数据在 data 向量中的起始位置，并构造 TGAColor 对象
	return TGAColor(data.data() + (x + y * width)*bytespp, bytespp);
}

// 设置指定坐标 (x, y) 处的像素颜色
// x, y: 像素坐标
// c: 要设置的颜色 (TGAColor 对象)
void TGAImage::set(int x, int y, const TGAColor &c) {
	// 检查坐标是否有效以及图像数据是否存在
	if (!data.size() || x < 0 || y < 0 || x >= width || y >= height) return;
	// 使用 memcpy 将颜色 c 的字节数据复制到 data 向量中对应位置
	memcpy(data.data() + (x + y * width)*bytespp, c.bgra, bytespp);
}

// 获取每个像素的字节数 (bytes per pixel)
int TGAImage::get_bytespp() {
	return bytespp;
}

// 获取图像宽度 (像素)
int TGAImage::get_width() const {
	return width;
}

// 获取图像高度 (像素)
int TGAImage::get_height() const {
	return height;
}

// 水平翻转图像数据
void TGAImage::flip_horizontally() {
	if (!data.size()) return; // 如果图像数据为空，则不执行任何操作
	int half = width >> 1; // 宽度的一半，循环到中线即可
	for (int i = 0; i < half; i++) { // 遍历左半部分的每一列
		for (int j = 0; j < height; j++) { // 遍历每一行
			TGAColor c1 = get(i, j); // 获取左侧像素颜色
			TGAColor c2 = get(width - 1 - i, j); // 获取对称位置的右侧像素颜色
			set(i, j, c2); // 将右侧颜色设置到左侧
			set(width - 1 - i, j, c1); // 将左侧颜色设置到右侧，完成交换
		}
	}
}

// 垂直翻转图像数据
void TGAImage::flip_vertically() {
	if (!data.size()) return; // 如果图像数据为空，则不执行任何操作
	size_t bytes_per_line = width * bytespp; // 每行图像数据的字节数
	std::vector<std::uint8_t> line(bytes_per_line, 0); // 用于临时存储一行的像素数据
	int half = height >> 1; // 高度的一半，循环到中线即可
	for (int j = 0; j < half; j++) { // 遍历上半部分的每一行
		size_t l1 = j * bytes_per_line; // 上半部分当前行的起始字节索引
		size_t l2 = (height - 1 - j)*bytes_per_line; // 对称位置的下半部分行的起始字节索引
		// 将 l1 行的数据复制到临时行向量 line 中
		std::copy(data.begin() + l1, data.begin() + l1 + bytes_per_line, line.begin());
		// 将 l2 行的数据复制到 l1 行的位置
		std::copy(data.begin() + l2, data.begin() + l2 + bytes_per_line, data.begin() + l1);
		// 将临时行向量 line 中的数据 (即原始的l1行) 复制到 l2 行的位置，完成交换
		std::copy(line.begin(), line.end(), data.begin() + l2);
	}
}

// 返回指向图像原始像素数据缓冲区的指针
std::uint8_t *TGAImage::buffer() {
	return data.data(); // vector::data() 返回指向内部数组的指针
}

// 清空图像数据，即将所有像素数据重置为0
void TGAImage::clear() {
	// 创建一个新的用0填充的向量，并赋值给 data，旧数据会被释放
	data = std::vector<std::uint8_t>(width*height*bytespp, 0);
}

// 将图像缩放到新的宽度 w 和高度 h (使用简单的最近邻插值，效果可能较差)
// w: 目标宽度, h: 目标高度
void TGAImage::scale(int w, int h) {
	if (w <= 0 || h <= 0 || !data.size()) return; // 检查目标尺寸和原图像数据是否有效
	std::vector<std::uint8_t> tdata(w*h*bytespp, 0); // 创建用于存储缩放后图像数据的新向量
	int nscanline = 0; // 新图像当前扫描线的起始字节索引
	int oscanline = 0; // 原图像当前扫描线的起始字节索引
	int erry = 0;      // Y方向的Bresenham类似算法的误差项
	size_t nlinebytes = w * bytespp; // 新图像每行的字节数
	size_t olinebytes = width * bytespp; // 原图像每行的字节数
	for (int j = 0; j < height; j++) { // 遍历原图像的每一行 (作为源)
		int errx = width - w; // X方向的误差项初始化 (原宽度 - 目标宽度)
		int nx = -bytespp;    // 新图像当前行内像素的字节偏移量
		int ox = -bytespp;    // 原图像当前行内像素的字节偏移量
		for (int i = 0; i < width; i++) { // 遍历原图像当前行的每一个像素 (作为源)
			ox += bytespp; // 指向原图像下一个像素
			errx += w;     // 累加误差 (目标宽度)
			while (errx >= (int)width) { // 当误差累加到大于等于原宽度时，说明新图像应该取用当前原图像像素
				errx -= width; // 减去原宽度，更新误差
				nx += bytespp; // 指向新图像下一个像素位置
				// 将原图像 oscanline+ox 处的像素数据复制到新图像 nscanline+nx 处
				memcpy(tdata.data() + nscanline + nx, data.data() + oscanline + ox, bytespp);
			}
		}
		erry += h; // Y方向误差累加 (目标高度)
		oscanline += olinebytes; // 指向原图像下一行
		while (erry >= (int)height) { // 当Y误差累加到大于等于原高度时，说明新图像应该生成一行
			// 如果 erry 远大于原高度 (例如跳过了一行)，则复制上一行新图像的数据到当前行 (简单的行复制)
			if (erry >= (int)height << 1) 
				memcpy(tdata.data() + nscanline + nlinebytes, tdata.data() + nscanline, nlinebytes);
			erry -= height; // 减去原高度，更新Y误差
			nscanline += nlinebytes; // 指向新图像下一行
		}
	}
	data = tdata; // 用缩放后的数据替换原有数据
	width = w;    // 更新图像宽度
	height = h;   // 更新图像高度
}
