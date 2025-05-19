#include <cmath>      // 包含数学函数库，提供三角函数、指数函数等
#include <algorithm>  // 包含算法库，提供min、max等算法函数
#include <iostream>   // 包含输入输出流库，用于控制台输出
#include <cassert>    // 包含断言库，用于程序调试

#include "gl.h"       // 包含自定义的图形库头文件

// lookat函数：创建视图矩阵（View Matrix）
// 参数：eye - 相机位置，center - 观察目标点，up - 上方向向量
// 返回：视图矩阵，将世界坐标系变换到相机坐标系
Matrix lookat(Vec3f eye, Vec3f center, Vec3f up)
{
	Vec3f z = (eye - center).normalize();  // 计算z轴方向：相机指向方向的反方向，并归一化
	Vec3f x = cross(up, z).normalize();    // 计算x轴方向：up与z叉乘得到x轴，并归一化
	Vec3f y = cross(z, x).normalize();     // 计算y轴方向：z与x叉乘得到y轴，并归一化
                                           // 这三个向量构成了相机坐标系的正交基底
										   
	//初始化rotate作为旋转矩阵,translate作为平移矩阵
	Matrix rotate = Matrix::identity();    // 初始化旋转矩阵为单位矩阵
	Matrix translate = Matrix::identity(); // 初始化平移矩阵为单位矩阵

	for (int i = 0; i < 3; ++i)
	{
		rotate[0][i] = x[i];               // 将数组 x 的第 i 个元素赋值给二维数组 rotate 的第 0 行的第 i 列
		rotate[1][i] = y[i];               // 将y轴向量填入旋转矩阵第二行
		rotate[2][i] = z[i];               // 将z轴向量填入旋转矩阵第三行
		translate[i][3] = -eye[i];         // 设置平移矩阵，使世界坐标原点移动到相机位置
	}
	return rotate * translate;             // 返回旋转矩阵与平移矩阵的乘积，即视图矩阵
                                           // 这样的做法的效果是是对**物体**先平移后旋转, 或者相机要先旋转后平移，将世界坐标变换到相机坐标
}




// ortho函数：创建正交投影矩阵
// 参数：l,r,b,t,n,f分别为左、右、底、顶、近、远
// 返回：正交投影矩阵
Matrix ortho(float l, float r, float b, float t, float n, float f)
{
	Matrix ret = Matrix::identity();       // 初始化为单位矩阵
	ret[0][0] = 2.0f / (r - l);           // x轴缩放因子，将x从[l,r]映射到[-1,1]
	ret[1][1] = 2.0f / (t - b);           // y轴缩放因子，将y从[b,t]映射到[-1,1]
	ret[2][2] = 2.0f / (n - f);           // z轴缩放因子，将z从[n,f]映射到[-1,1]
	ret[0][3] = (l + r) / (l - r);        // x轴平移量
	ret[1][3] = (b + t) / (b - t);        // y轴平移量
	ret[2][3] = (n + f) / (f - n);        // z轴平移量
	return ret;                            // 返回正交投影矩阵
}

// perspective函数：创建透视变换矩阵（将透视视锥体变换为长方体）
// 参数：n - 近平面距离，f - 远平面距离
// 返回：透视变换矩阵
Matrix perspective(double n, double f)
{
	Matrix ret;                            // 创建空矩阵
	ret[0][0] = n;                         // x坐标缩放因子
	ret[1][1] = n;                         // y坐标缩放因子
	ret[2][2] = n + f;                     // z坐标变换系数
	ret[2][3] = -f * n;                    // z坐标变换常数
	ret[3][2] = 1.0f;                      // 设置w分量为z，实现深度透视
	return ret;                            // 返回透视变换矩阵
}

// projection函数：创建透视投影矩阵
// 参数：fov - 视场角，ratio - 宽高比，n - 近平面距离，f - 远平面距离
// 返回：透视投影矩阵
Matrix projection(double fov, double ratio, double n, double f)
{
	float t = (-n) * tanf(fov / 2);       // 计算近平面上半高，使用负n是因为OpenGL中z轴指向屏幕内
	float r = t * ratio;                   // 计算近平面右侧宽度，考虑宽高比
	return ortho(-r, r, -t, t, n, f) * perspective(n, f); // 返回正交投影与透视变换的组合
                                           // 先将透视视锥体变换为长方体，再进行正交投影
}

// viewport函数：创建视口变换矩阵
// 参数：width - 视口宽度，height - 视口高度
// 返回：视口变换矩阵，将规范化设备坐标(NDC)变换到屏幕坐标
Matrix viewport(unsigned width, unsigned height)
{
	Matrix ret = Matrix::identity();       // 初始化为单位矩阵
	ret[0][0] = width / 2.0f;             // x轴缩放因子，将x从[-1,1]映射到[0,width]
	ret[1][1] = height / 2.0f;            // y轴缩放因子，将y从[-1,1]映射到[0,height]
	ret[0][3] = (width - 1) / 2.0f;       // x轴平移量，考虑像素中心偏移
	ret[1][3] = (height - 1) / 2.0f;      // y轴平移量，考虑像素中心偏移
	return ret;                            // 返回视口变换矩阵
}





// barycentric函数：计算点P在三角形ABC中的重心坐标
// 参数：A,B,C为三角形三个顶点，P为待检测点
// 返回：点P关于三角形ABC的重心坐标
Vec3f barycentric(Vec2f A, Vec2f B, Vec2f C, Vec2f P)
{
	Vec3f t[2];                           // 创建两个向量数组
	for (int i = 0; i < 2; i++)           // 对x和y坐标分别处理
	{
		t[i][0] = B[i] - A[i];            // 计算AB边的分量
		t[i][1] = C[i] - A[i];            // 计算AC边的分量
		t[i][2] = A[i] - P[i];            // 计算PA的分量
	}
	Vec3f u = cross(t[0], t[1]);          // 计算叉乘，使用行列式求解线性方程组

	if (fabs(u.z) < 1e-5) return Vec3f(-1.0f, 0.0f, 0.0f); // 如果u.z接近0，表示三角形退化或点不在平面上
	return Vec3f(1.0f - (u.x + u.y) / u.z, u.x / u.z, u.y / u.z); // 返回规范化的重心坐标(1-α-β, α, β)
}

// triangle函数：三角形光栅化
// 参数：screenCoords - 三角形三个顶点的屏幕坐标，shader - 着色器对象，
//      colorBuffer - 颜色缓冲区，zBuffer - 深度缓冲区，
//      width - 屏幕宽度，height - 屏幕高度，
//      d - MSAA采样点偏移数组，cntSample - 每像素采样点数量

//screenCoords 是一个包含3个顶点的数组，每个顶点是 Vec4f 类型（齐次坐标，形式为 (x, y, z, w)）
//screenCoords[i] 表示第 i 个顶点的齐次坐标，其中 x, y, z 是屏幕坐标，w 是透视校正因子
//screenCoords[i][j] 表示第 i 个顶点的第 j 个分量，其中 j=0 表示 x 坐标，j=1 表示 y 坐标，j=2 表示 z 坐标，j=3 表示 w 坐标

void triangle(Vec4f *screenCoords, IShader &shader, Vec3f *colorBuffer, float *zBuffer, unsigned width, unsigned height, const float d[][2], unsigned cntSample)
{


	// 计算三角形在屏幕上的最小包围盒
	Vec2i bboxmin(width - 1, height - 1); // 初始化包围盒最右上点为屏幕最大坐标
	Vec2i bboxmax(0.0f, 0.0f);            // 初始化包围盒最左下点为原点

	for (int i = 0; i < 3; ++i)           // 遍历三角形的三个顶点
	{
		for (int j = 0; j < 2; ++j)       // 对x和y坐标分别处理
		{
			bboxmin[j] = std::min(bboxmin[j], int(screenCoords[i][j])); // 更新包围盒最小坐标
			bboxmax[j] = std::max(bboxmax[j], int(screenCoords[i][j])); // 更新包围盒最大坐标
		}
	}
	// 将包围盒裁剪到屏幕范围内
	bboxmin[0] = std::max(0, bboxmin[0]); // x坐标不小于0
	bboxmin[1] = std::max(0, bboxmin[1]); // y坐标不小于0
	bboxmax[0] = std::min(int(width - 1), bboxmax[0]);  // x坐标不大于屏幕最大宽度
	bboxmax[1] = std::min(int(height - 1), bboxmax[1]); // y坐标不大于屏幕最大高度


	// 三角形光栅化过程 
	//proj<2>：从齐次坐标中提取前2个分量（x, y），丢弃其他分量

	Vec2f A = proj<2>(screenCoords[0]), B = proj<2>(screenCoords[1]), C = proj<2>(screenCoords[2]); // 提取三角形顶点的2D坐标
	
	for (int x = bboxmin.x; x <= bboxmax.x; ++x) // 遍历包围盒中的每个像素的x坐标
	{
		for (int y = bboxmin.y; y <= bboxmax.y; ++y) // 遍历包围盒中的每个像素的y坐标
		{
			// 计算每个像素的多个采样点的颜色和深度
			Vec3f barMiddle, color;        // barMiddle存储像素中心点的重心坐标，color存储颜色
			bool covered = false;          // 标记该像素是否被三角形覆盖

			for (int i = 0; i < cntSample; ++i) // 遍历每个采样点
			{
				//d[i][0]和d[i][1]是MSAA采样点相对于像素中心的偏移量，用于在每个像素内进行多重采样
				
				Vec2f sample(x + d[i][0], y + d[i][1]); 
				Vec3f barSample = barycentric(A, B, C, sample); // 计算采样点的重心坐标

				float w = screenCoords[0].w * barSample.x + screenCoords[1].w * barSample.y + screenCoords[2].w * barSample.z; // 计算w分量插值
				w = 1.0f / w;              // 透视校正插值的权重因子
				float z = screenCoords[0].z * barSample.x + screenCoords[1].z * barSample.y + screenCoords[2].z * barSample.z; // 计算z分量插值
				z = z * w;                 // 应用透视校正
				
				unsigned idx = cntSample * (y*width + x) + i; // 计算采样点在缓冲区中的索引, 索引是像素的索引乘以采样点数加上采样点索引
				
				if (barSample.x < 0 || barSample.y < 0 || barSample.z < 0 || z < zBuffer[idx]) continue; // 如果采样点不在三角形内部或被遮挡，跳过

				if (!covered) // 如果这是该像素第一次被覆盖，计算颜色
				{
					barMiddle = barycentric(A, B, C, Vec2f(x + 0.5f, y + 0.5f)); // 计算像素中心点的重心坐标
					if (!shader.fragment(barMiddle, color)) break; // 调用片段着色器计算颜色，若返回false则跳过该像素
					covered = true;        // 标记该像素已被覆盖
				}

				colorBuffer[idx] = color;  // 将颜色写入颜色缓冲区
				zBuffer[idx] = z;          // 将深度写入深度缓冲区
			}
		}
	}
}








// singleFaceZClip函数：对多边形进行单一平面的裁剪
// 参数：original - 原始顶点列表，result - 裁剪后的顶点列表，axis - 裁剪的坐标轴索引
//顶点列表是模型的所有顶点坐标的集合

void singleFaceZClip(const std::vector<Vertex> &original, std::vector<Vertex> &result, unsigned axis)
{
	for (unsigned i = 0; i < original.size(); ++i) // 遍历原始多边形的所有边
	{
		Vertex now = original[i], next = original[(i + 1) % original.size()]; // 获取当前边的两个端点

		float checkNow = now.clipCoord[axis] / now.clipCoord.w, checkNext = next.clipCoord[axis] / next.clipCoord.w; // 计算端点在裁剪坐标系中的位置
		
		if (checkNow <= 1.0f)             // 如果当前顶点在裁剪平面内部或平面上
		{
			result.push_back(now);         // 将当前顶点添加到结果列表
		}
		
		if ((checkNow < 1.0f && checkNext > 1.0f) || (checkNow > 1.0f && checkNext < 1.0f)) // 如果当前边与裁剪平面相交
		{
			pushIntersection(result, now, next, axis); // 计算交点并添加到结果列表
		}
	}
}


// pushIntersection函数：计算边与裁剪平面的交点并添加到结果列表
// 参数：result - 结果顶点列表，now/next - 边的两个端点，axis - 裁剪的坐标轴索引
void pushIntersection(std::vector<Vertex> &result, Vertex now, Vertex next, unsigned axis)
{
	// 计算参数t，使得 w0 + t*(w1-w0) = z0 + t*(z1-z0)
	float t0 = now.clipCoord.w - now.clipCoord[axis];  // t0 = w0 - z0
	float t1 = next.clipCoord.w - next.clipCoord[axis]; // t1 = w1 - z1
	float t = t0 / (t0 - t1);             // 计算交点的插值参数t

	float w = 1.0f / now.clipCoord.w + t * (1.0f / next.clipCoord.w - 1.0f / now.clipCoord.w); // 计算交点的透视校正权重
	w = 1.0f / w;                         // 取倒数得到w

	// 计算交点
	Vertex inter;                          // 创建交点顶点
	inter.clipCoord = now.clipCoord + (next.clipCoord - now.clipCoord) * t; // 计算交点的裁剪坐标
	inter.worldCoord = now.worldCoord + (next.worldCoord - now.worldCoord) * t; // 计算交点的世界坐标
	inter.worldCoord = inter.worldCoord * w; // 应用透视校正
	inter.normal = now.normal + (next.normal - now.normal) * t; // 计算交点的法线
	inter.normal = inter.normal * w;      // 应用透视校正
	inter.uv = now.uv + (next.uv - now.uv) * t; // 计算交点的纹理坐标
	inter.uv = inter.uv * w;              // 应用透视校正
	result.push_back(inter);              // 将交点添加到结果列表
}


// homogeneousClip函数：对多边形进行齐次裁剪, 也就是对齐次坐标进行裁剪, 裁剪的坐标轴索引是axis, 裁剪的平面是w+axis和w-axis
// 参数：original - 原始顶点列表，result - 裁剪后的顶点列表，axis - 裁剪的坐标轴索引
void homogeneousClip(const std::vector<Vertex> &original, std::vector<Vertex> &result, unsigned axis)
{
	std::vector<Vertex> intermediate;     // 创建中间顶点列表

	// 第一次裁剪：处理正方向平面（如右裁剪平面 x ≤ w）// 对w+axis平面进行裁剪
	singleFaceZClip(original, intermediate, axis); 
	// 翻转坐标：将负方向平面（如左裁剪平面 x ≥ -w）转换为正方向判断
	for (auto &vertex : intermediate)     // 遍历中间顶点列表
	{
		vertex.clipCoord[axis] = -vertex.clipCoord[axis]; // 翻转坐标轴，准备对w-axis平面裁剪
	}

	// 第二次裁剪：处理翻转后的"正方向"（即原左平面）
	singleFaceZClip(intermediate, result, axis); 
	// 恢复坐标轴方向 // 对w-axis平面进行裁剪
	for (auto &vertex : result)           // 遍历结果顶点列表
	{
		vertex.clipCoord[axis] = -vertex.clipCoord[axis]; 
	}
}