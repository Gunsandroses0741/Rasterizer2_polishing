#pragma once // 防止头文件被重复包含

#include <vector>      // 包含std::vector容器
#include <string>      // 包含std::string字符串类

#include "geometry.h"  // 包含自定义的几何运算相关头文件 (例如 Vec2f, Vec3f)
#include "tgaimage.h"  // 包含自定义的TGA图像处理相关头文件

// Model类，用于加载和管理3D模型数据
class Model {
private:
	std::vector<Vec3f> verts_;     // 存储模型所有顶点的坐标 (x, y, z)
	std::vector<Vec2f> uv_;        // 存储模型所有纹理坐标 (u, v)
	std::vector<Vec3f> norms_;     // 存储模型所有法线向量 (nx, ny, nz)
	std::vector<int> facet_vrt_; // 存储每个面片(三角形)的顶点索引，三个一组构成一个三角形
	std::vector<int> facet_tex_;  // 存储每个面片(三角形)的纹理坐标索引，三个一组构成一个三角形
	std::vector<int> facet_nrm_;  // 存储每个面片(三角形)的法线向量索引，三个一组构成一个三角形
	TGAImage diffusemap_;         // 漫反射贴图对象，存储颜色信息
	TGAImage normalmap_;          // 法线贴图对象，存储表面法线扰动信息
	TGAImage specularmap_;        // 镜面高光贴图对象，存储表面高光强度信息

	// 私有辅助函数，用于加载纹理文件
	void load_texture(const std::string filename, const std::string suffix, TGAImage &img);

public:
	// 构造函数，从指定的.obj文件加载模型
	Model(const std::string filename);

	// 获取模型顶点数量
	int nverts() const;

	// 获取模型面片数量 (假设模型已三角化)
	int nfaces() const;

	// 获取指定面片、指定顶点的法线向量
	Vec3f normal(const int iface, const int nthvert) const;

	// 根据UV坐标从法线贴图中采样法线向量
	Vec3f normal(const Vec2f &uv) const;

	// 获取指定索引的顶点坐标
	Vec3f vert(const int i) const;

	// 获取指定面片、指定顶点的坐标
	Vec3f vert(const int iface, const int nthvert) const;

	// 获取指定面片、指定顶点的纹理坐标
	Vec2f uv(const int iface, const int nthvert) const;

	// 根据UV坐标从漫反射贴图中采样颜色
	TGAColor diffuse(const Vec2f &uv) const;

	// 根据UV坐标从镜面高光贴图中采样高光强度值 (通常是灰度值)
	double specular(const Vec2f &uv) const;
};
