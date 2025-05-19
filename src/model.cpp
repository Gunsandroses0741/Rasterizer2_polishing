#include <iostream>  // 用于标准输入输出流操作，例如 std::cerr
#include <fstream>   // 用于文件流操作，例如 std::ifstream
#include <sstream>   // 用于字符串流操作，例如 std::istringstream

#include "model.h"    // 包含Model类的声明

// Model类的构造函数，负责从.obj文件加载模型数据和纹理
// 参数 filename: .obj文件的路径
Model::Model(const std::string filename) : verts_(), uv_(), norms_(), facet_vrt_(), facet_tex_(), facet_nrm_(), diffusemap_(), normalmap_(), specularmap_() {
	std::ifstream in; // 创建一个输入文件流对象
	in.open(filename, std::ifstream::in); // 以只读方式打开指定的.obj文件
	if (in.fail()) return; // 如果文件打开失败，则直接返回，不进行后续操作
	std::string line; // 用于存储从文件中读取的每一行内容
	while (!in.eof()) { // 当未到达文件末尾时，循环读取
		std::getline(in, line); // 从文件流中读取一行到line字符串
		std::istringstream iss(line.c_str()); // 将读取的行内容转换为字符串流，方便解析
		char trash; // 用于丢弃不需要的字符，例如行开头的标识符或分隔符
		if (!line.compare(0, 2, "v ")) { // 如果行的前两个字符是 "v " (表示顶点坐标)
			iss >> trash; // 丢弃 'v'
			Vec3f v; // 创建一个三维向量用于存储顶点坐标
			for (int i = 0; i < 3; i++) iss >> v[i]; // 从字符串流中读取三个浮点数作为x, y, z坐标
			verts_.push_back(v); // 将读取到的顶点坐标添加到顶点列表 verts_ 中
		}
		else if (!line.compare(0, 3, "vn ")) { // 如果行的前三个字符是 "vn " (表示法线向量)
			iss >> trash >> trash; // 丢弃 'v' 和 'n'
			Vec3f n; // 创建一个三维向量用于存储法线向量
			for (int i = 0; i < 3; i++) iss >> n[i]; // 从字符串流中读取三个浮点数作为nx, ny, nz分量
			norms_.push_back(n.normalize()); // 将读取到的法线向量归一化后添加到法线列表 norms_ 中
		}
		else if (!line.compare(0, 3, "vt ")) { // 如果行的前三个字符是 "vt " (表示纹理坐标)
			iss >> trash >> trash; // 丢弃 'v' 和 't'
			Vec2f uv; // 创建一个二维向量用于存储纹理坐标
			for (int i = 0; i < 2; i++) iss >> uv[i]; // 从字符串流中读取两个浮点数作为u, v坐标
			uv_.push_back(uv); // 将读取到的纹理坐标添加到纹理坐标列表 uv_ 中
		}
		else if (!line.compare(0, 2, "f ")) { // 如果行的前两个字符是 "f " (表示面片索引)
			int f, t, n; // 用于存储顶点索引、纹理索引、法线索引
			iss >> trash; // 丢弃 'f'
			int cnt = 0; // 计数器，用于检查面片是否为三角形
			// .obj文件中面片索引格式通常为 v/vt/vn
			while (iss >> f >> trash >> t >> trash >> n) { // 读取一组顶点/纹理/法线索引
				// .obj文件中的索引是从1开始的，需要减1转换为从0开始的数组索引
				facet_vrt_.push_back(--f); // 存储顶点索引
				facet_tex_.push_back(--t); // 存储纹理索引
				facet_nrm_.push_back(--n); // 存储法线索引
				cnt++; // 索引组计数增加
			}
			if (3 != cnt) { // 如果一个面片解析出的顶点数不是3，说明文件未三角化或格式错误
				std::cerr << "Error: the obj file is supposed to be triangulated" << std::endl; // 输出错误信息
				in.close(); // 关闭文件流
				return; // 退出构造函数
			}
		}
	}
	in.close(); // 完成文件读取后关闭文件流
	// 输出加载的模型信息：顶点数，面片数，纹理坐标数，法线向量数
	std::cerr << "# v# " << nverts() << " f# " << nfaces() << " vt# " << uv_.size() << " vn# " << norms_.size() << std::endl;
	// 加载纹理贴图，通常与.obj文件同名，但后缀不同
	load_texture(filename, "_diffuse.tga", diffusemap_);    // 加载漫反射贴图
	load_texture(filename, "_nm_tangent.tga", normalmap_); // 加载切线空间法线贴图
	load_texture(filename, "_spec.tga", specularmap_);   // 加载镜面高光贴图
}

// 返回模型中顶点的数量
int Model::nverts() const {
	return verts_.size(); // verts_ 存储了所有顶点信息，其大小即为顶点数量
}

// 返回模型中面片的数量
// 假设每个面片都是三角形，每个三角形有3个顶点索引
int Model::nfaces() const {
	return facet_vrt_.size() / 3; // facet_vrt_ 存储了所有面片的顶点索引，总索引数除以3即为面片数量
}

// 根据索引 i 返回顶点坐标
Vec3f Model::vert(const int i) const {
	return verts_[i]; // 直接从顶点列表 verts_ 中获取指定索引的顶点坐标
}

// 根据面片索引 iface 和该面片内的顶点序号 nthvert (0, 1, 或 2) 返回顶点坐标
Vec3f Model::vert(const int iface, const int nthvert) const {
	// facet_vrt_ 存储了面片顶点索引，iface * 3 + nthvert 定位到具体某个顶点的索引
	return verts_[facet_vrt_[iface * 3 + nthvert]]; // 再根据此索引从 verts_ 中获取顶点坐标
}

// 加载纹理文件的私有辅助函数
// 参数 filename: .obj文件的原始路径名
// 参数 suffix: 纹理文件的后缀 (例如 "_diffuse.tga")
// 参数 img: TGAImage 对象，用于存储加载的纹理数据
void Model::load_texture(std::string filename, const std::string suffix, TGAImage &img) {
	size_t dot = filename.find_last_of("."); // 查找原始文件名中最后一个'.'的位置，用于替换扩展名
	if (dot == std::string::npos) return; // 如果没有找到'.'，则无法确定基本文件名，直接返回
	// 构建纹理文件的完整路径：原始文件名的基本部分 + 后缀
	std::string texfile = filename.substr(0, dot) + suffix;
	// 尝试读取TGA文件，并输出加载状态（成功或失败）
	std::cerr << "texture file " << texfile << " loading " << (img.read_tga_file(texfile.c_str()) ? "ok" : "failed") << std::endl;
	img.flip_vertically(); // TGA图像通常需要垂直翻转以匹配OpenGL等图形API的纹理坐标系约定
}

// 根据给定的纹理坐标 uvf 从漫反射贴图中采样颜色
TGAColor Model::diffuse(const Vec2f &uvf) const {
	// 将归一化的UV坐标(0到1范围)映射到纹理图像的实际像素坐标
	// uvf[0] (u) * 纹理宽度, uvf[1] (v) * 纹理高度
	return diffusemap_.get(uvf[0] * diffusemap_.get_width(), uvf[1] * diffusemap_.get_height());
}

// 根据给定的纹理坐标 uvf 从法线贴图中采样法线向量
Vec3f Model::normal(const Vec2f &uvf) const {
	// 从法线贴图获取对应UV坐标的颜色值
	TGAColor c = normalmap_.get(uvf[0] * normalmap_.get_width(), uvf[1] * normalmap_.get_height());
	Vec3f res; // 用于存储转换后的法线向量
	for (int i = 0; i < 3; i++)
		// 法线贴图中的颜色值 (0-255) 需要映射到法线向量分量 (-1 到 1)
		// 映射公式: (color_channel / 255.0) * 2.0 - 1.0
		// 注意这里 res[2-i] = ... 是因为TGA颜色通道顺序(BGR)和期望法线向量(XYZ)的对应关系
		res[2 - i] = c[i] / 255. * 2 - 1;
	return res; // 返回计算得到的法线向量
}

// 根据给定的纹理坐标 uvf 从镜面高光贴图中采样高光强度值
// 通常镜面高光贴图是灰度图，只使用一个颜色通道（例如红色通道）的值
double Model::specular(const Vec2f &uvf) const {
	// 从镜面高光贴图获取对应UV坐标的像素颜色，并取其第一个颜色通道(通常是R通道)作为高光强度
	return specularmap_.get(uvf[0] * specularmap_.get_width(), uvf[1] * specularmap_.get_height())[0];
}

// 根据面片索引 iface 和该面片内的顶点序号 nthvert 返回纹理坐标
Vec2f Model::uv(const int iface, const int nthvert) const {
	// facet_tex_ 存储了面片纹理坐标索引，iface * 3 + nthvert 定位到具体某个顶点的纹理索引
	return uv_[facet_tex_[iface * 3 + nthvert]]; // 再根据此索引从 uv_ 列表中获取纹理坐标
}

// 根据面片索引 iface 和该面片内的顶点序号 nthvert 返回法线向量
Vec3f Model::normal(const int iface, const int nthvert) const {
	// facet_nrm_ 存储了面片法线向量索引，iface * 3 + nthvert 定位到具体某个顶点的法线索引
	return norms_[facet_nrm_[iface * 3 + nthvert]]; // 再根据此索引从 norms_ 列表中获取法线向量
}
