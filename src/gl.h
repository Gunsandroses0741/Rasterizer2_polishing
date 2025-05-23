#pragma once

#include <vector>

#include "geometry.h"
#include "tgaimage.h"
#include "model.h"

// interface for shader struct
//这个是shader的接口，定义了顶点着色器和片段着色器
struct IShader
{
	virtual Vec4f vertex(unsigned nthvert, Vec4f worldCoord, Vec2f uv, Vec3f normal) = 0;
	//顶点着色器，输入参数是顶点索引，顶点在世界坐标系中的坐标，顶点的纹理坐标，顶点的法线，输出参数是顶点在裁剪坐标系中的坐标
	virtual bool fragment(Vec3f bar, Vec3f &color) = 0; //片段着色器，输入参数是重心坐标，输出参数是颜色
};

// struct for clipping parameter
//这个是顶点结构体，定义了顶点着色器和片段着色器的输入参数
//worldCoord是顶点在世界坐标系中的坐标，clipCoord是顶点在裁剪坐标系中的坐标，uv是顶点的纹理坐标，normal是顶点的法线
struct Vertex
{
	Vec3f normal;
	Vec4f worldCoord, clipCoord;
	Vec2f uv;

	Vertex(Vec4f worldCoord = Vec4f(), Vec4f clipCoord = Vec4f(), Vec2f uv = Vec2f(), Vec3f normal = Vec3f())
		: worldCoord(worldCoord), clipCoord(clipCoord), uv(uv), normal(normal) {}
};

// functions for viewing transformation
Matrix lookat(Vec3f eye, Vec3f center, Vec3f up);
Matrix projection(double fov, double ratio, double n, double f);
Matrix perspective(double n, double f);
Matrix ortho(float l, float r, float b, float t, float n, float f);
Matrix viewport(unsigned width, unsigned height);

// functions for rasterization
Vec3f barycentric(Vec2f A, Vec2f B, Vec2f C, Vec2f P);
void triangle(Vec4f *screenCoords, IShader &shader, Vec3f *colorBuffer, float *zBuffer, unsigned width, unsigned height, const float d[][2], unsigned cntSample);

// functions for clipping
void homogeneousClip(const std::vector<Vertex> &original, std::vector<Vertex> &result, unsigned axis);
void singleFaceZClip(const std::vector<Vertex> &original, std::vector<Vertex> &result, unsigned axis);
void pushIntersection(std::vector<Vertex> &result, Vertex now, Vertex next, unsigned axis);
