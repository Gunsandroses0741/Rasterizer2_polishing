#include <limits>
#include <vector>

#include <filesystem>

#include "tgaimage.h"
#include "geometry.h"
#include "model.h"
#include "gl.h"

/**
 * DepthShader类：专门用于生成阴影贴图的着色器
 * 实现了IShader接口，用于深度值计算而非颜色渲染
 */
struct DepthShader : public IShader
{
	// 统一变量（uniform变量）：在整个着色过程中保持不变的数据
	Matrix uVpPV;  // 视口变换 * 投影矩阵 * 视图矩阵的组合变换矩阵
	// 顶点间插值变量（varying变量）：在顶点间插值传递的数据
	mat<4, 3, float> vScreenCoords;  // 存储3个顶点的屏幕坐标


	DepthShader() {}  // 默认构造函数

	/**
	 * 顶点着色器函数：将顶点从世界坐标转换为屏幕坐标
	 * @param nthvert 当前顶点的索引(0,1,2)
	 * @param worldCoord 世界坐标
	 * @param uv 纹理坐标（此处未使用）
	 * @param normal 法线向量（此处未使用）
	 * @return 变换后的屏幕坐标
	 */
	Vec4f vertex(unsigned nthvert, Vec4f worldCoord, Vec2f uv, Vec3f normal)
	{
		// 将世界坐标转换为屏幕坐标
		Vec4f screenCoord = uVpPV * worldCoord;
		// 进行透视除法，将齐次坐标转换为欧氏坐标
		screenCoord = screenCoord / screenCoord[3];
		// 存储屏幕坐标，用于后续的片段着色
		vScreenCoords.set_col(nthvert, screenCoord);

		return screenCoord;
	}

	/**
	 * 片段着色器函数：计算片段的颜色
	 * @param bar 重心坐标，用于插值计算
	 * @param color 输出的颜色
	 * @return 是否渲染该片段
	 */
	bool fragment(Vec3f bar, Vec3f &color)
	{
		// 使用重心坐标计算插值后的片段位置
		Vec4f fragPos = vScreenCoords * bar;
		// 根据深度值计算颜色，深度值越大，颜色越暗，形成可视化的深度图
		// 使用指数函数增强对比度，方便可视化
		color = Vec3f(255.0f, 255.0f, 255.0f) * powf(expf(fragPos[2]-1.0f), 4.0f);

		return true;  // 渲染该片段
	}
};

/**
 * LightColor结构：表示光源的颜色属性
 * 包含环境光、漫反射和镜面反射三种颜色分量
 */
struct LightColor
{
	Vec3f ambient, diffuse, specular;  // 环境光、漫反射和镜面反射颜色

	/**
	 * 构造函数：初始化光源颜色
	 * @param ambi 环境光颜色
	 * @param diff 漫反射颜色
	 * @param spec 镜面反射颜色
	 */
	LightColor(Vec3f ambi = Vec3f(), Vec3f diff = Vec3f(), Vec3f spec = Vec3f())
	{
		ambient = ambi;
		diffuse = diff;
		specular = spec;
	}
};

/**
 * Shader类：实现Phong着色模型的着色器
 * 实现了IShader接口，用于执行完整的光照计算
 */
struct Shader : public IShader
{
	// 统一变量（在所有顶点和片段处理中保持一致的数据） uniform所以加u
	Model *uTexture;  // 模型纹理
	Matrix uModel, uVpPV, uLightVpPV;  // 模型、视图投影和光源视图投影变换矩阵
	Vec3f uEyePos, uLightPos, uTangent, uBitangent;  // 视点位置、光源位置、切线和副切线向量
	LightColor uLightColor;  // 光源颜色
	float *uShadowBuffer;  // 阴影缓冲区（深度图）
	unsigned uShadowBufferWidth, uShadowBufferHeight;  // 阴影缓冲区尺寸
	
	// 顶点间插值变量（varying变量）varying所以加v
	mat<4, 3, float> vScreenCoords;  // 屏幕坐标
	mat<2, 3, float> vUv;  // 纹理坐标
	mat<3, 3, float> vN;  // 法线向量
	mat<3, 3, float> vLightSpacePos;  // 光源空间位置，用于阴影计算
	mat<3, 3, float> vWorldCoords;  // 世界坐标


	Shader() {}  // 默认构造函数

	/**
	 * 顶点着色器函数：处理单个顶点
	 * @param nthvert 当前顶点索引
	 * @param worldCoord 世界坐标
	 * @param uv 纹理坐标
	 * @param normal 法线向量
	 * @return 变换后的屏幕坐标
	 */
	Vec4f vertex(unsigned nthvert, Vec4f worldCoord, Vec2f uv, Vec3f normal)
	{
		// 计算屏幕坐标
		Vec4f screenCoord = uVpPV * worldCoord;
		float w = screenCoord[3];  // 保存透视除法的分母

		// 存储世界坐标，透视校正插值需要除以w
		vWorldCoords.set_col(nthvert, proj<3>(worldCoord) / w);

		// 进行透视除法，将齐次坐标转换为标准设备坐标
		screenCoord = screenCoord / w;
		// 保存z/w值，用于透视校正插值
		screenCoord[2] = screenCoord[2] / w;
		// 保存1/w值，用于后续透视校正插值
		screenCoord[3] = 1.0f / w;
		vScreenCoords.set_col(nthvert, screenCoord);

		// 存储纹理坐标，应用透视校正
		Vec2f vertUv = uv / w;
		vUv.set_col(nthvert, vertUv);

		// 存储法线向量，应用透视校正
		Vec3f vertN = normal / w;
		vN.set_col(nthvert, vertN);

		// 计算光源空间位置，用于阴影映射
		Vec4f temp = uLightVpPV * worldCoord;  // 将顶点变换到光源空间
		temp = temp / temp.w;  // 透视除法
		Vec3f vertLightSpacePos = proj<3>(temp) / w;  // 投影到3D并应用透视校正
		vLightSpacePos.set_col(nthvert, vertLightSpacePos);

		return screenCoord;
	}

	/**
	 * 片段着色器函数：计算片段颜色
	 * @param bar 重心坐标，用于插值计算
	 * @param color 输出的颜色
	 * @return 是否渲染该片段
	 */
	bool fragment(Vec3f bar, Vec3f &color)
	{
		// 计算透视校正插值的w值
		float w = (vScreenCoords * bar)[3];
		if (fabs(w) < 1e-7) return false;  // 避免除以接近零的数
		w = 1.0f / w;  // 将1/w转换回w

		// 计算透视校正的纹理坐标
		Vec2f uv = vUv * bar * w;

		// 从切线空间计算法线向量（法线贴图）
		mat<3, 3, float> TBN;  // 切线空间到世界空间的变换矩阵
		TBN.set_col(0, uTangent);  // 设置切线向量
		TBN.set_col(1, uBitangent);  // 设置副切线向量
		TBN.set_col(2, vN * bar * w);  // 设置插值后的法线向量
		// 从法线贴图获取切线空间法线并转换到世界空间
		Vec3f n = (TBN * uTexture->normal(uv)).normalize();
		
		// 计算用于光照的方向向量
		Vec3f worldCoord = vWorldCoords * bar * w;  // 插值后的世界坐标
		Vec3f lightDir = uLightPos.normalize();  // 光照方向（此处假设为方向光）
		Vec3f eyeDir = (uEyePos - worldCoord).normalize();  // 视线方向
		Vec3f half = (lightDir + eyeDir) / 2.0f;  // 半程向量，用于Blinn-Phong高光计算

		// 环境光反射计算
		Vec3f materialAmbient = uTexture->diffuse(uv).rgb();  // 材质环境光反射系数（从漫反射纹理获取）
		Vec3f ambient = uLightColor.ambient * materialAmbient;  // 环境光分量
		
		// 漫反射计算 (Lambert模型)
		Vec3f materialDiffuse = uTexture->diffuse(uv).rgb();  // 材质漫反射系数
		// 漫反射强度 = 光照强度 * 材质漫反射系数 * max(0, 法线·光照方向)
		Vec3f diffuse = uLightColor.diffuse * (materialDiffuse * std::max(0.0f, dot(n, lightDir)));

		// 镜面反射计算 (Blinn-Phong模型)
		float materialSpecular = uTexture->specular(uv);  // 材质镜面反射系数
		// 镜面反射强度 = 光照强度 * 材质镜面反射系数 * (法线·半程向量)^32
		Vec3f specular = uLightColor.specular * (materialSpecular * powf(std::max(0.0f, dot(n, half)), 32.0f));

		// 计算阴影
		float shadow = 0.0f;
		Vec3f lightSpacePos = vLightSpacePos * bar * w;  // 插值后的光源空间坐标
		int cntSample = 0;  // 采样计数
		// 进行阴影采样（PCF滤波，减少阴影锯齿）
		for (int dx = -2; dx < 2; dx++)  // 在x方向采样4个点
		{
			int sampleX = lightSpacePos.x + dx;
			if (sampleX < 0 || sampleX >= uShadowBufferWidth) continue;  // 边界检查
			for (int dy = -2; dy < 2; dy++)  // 在y方向采样4个点
			{
				int sampleY = lightSpacePos.y + dy;
				if (sampleY < 0 || sampleY >= uShadowBufferHeight) continue;  // 边界检查
				cntSample++;  // 有效采样点计数
				// 比较当前深度与阴影贴图中的深度
				// 添加偏移量(0.005f)避免自阴影问题
				if (lightSpacePos.z + 0.005f < uShadowBuffer[sampleY * uShadowBufferWidth + sampleX])
					shadow += 1.0f;  // 在阴影中
			}
		}
		shadow /= cntSample;  // 计算阴影因子（0到1之间）

		// 使用Blinn-Phong光照模型计算最终颜色
		// 环境光 + (漫反射 + 镜面反射) * (1 - 阴影因子)
		color = ambient + (diffuse + specular) * (1.0f - shadow);

		return true;  // 渲染该片段
	}
};




// 常量定义
const float PI = acosf(-1.0f);  // π值，用于角度计算

const unsigned SCREEN_WIDTH = 800;   // 屏幕宽度
const unsigned SCREEN_HEIGHT = 800;  // 屏幕高度

const unsigned SHADOW_WIDTH = 800;   // 阴影贴图宽度
const unsigned SHADOW_HEIGHT = 800;  // 阴影贴图高度

const unsigned CNT_SAMPLE = 4;          // 每个像素的采样数（用于MSAA）
const float D_MSAA[CNT_SAMPLE][2] = {   // MSAA采样点的偏移量
	{0.25f, 0.25f}, {0.25f, 0.75f},
	{0.75f, 0.25f}, {0.75f, 0.75f}
};
const float D_NonMSAA[1][2] = {         // 非MSAA采样的偏移量（居中采样）
	{0.0f, 0.0f}
};



// 全局变量定义
Vec3f lightPos(1.0f, 1.0f, 1.0f);  // 光源位置
// 光源颜色：环境光、漫反射和镜面反射分量
LightColor lightColor(Vec3f(0.3f, 0.3f, 0.3f), Vec3f(1.0f, 1.0f, 1.0f), Vec3f(0.5f, 0.5f, 0.5f));

Vec3f eye(1.0f, 1.0f, 3.0f);      // 相机位置
Vec3f center(0.0f, 0.0f, 0.0f);   // 相机看向的点
Vec3f up(0.0f, 1.0f, 0.0f);       // 相机上方向



/**
 * 阴影映射函数：从光源视角渲染场景，生成深度图
 * @param modelData 模型数据数组
 * @param modelTrans 模型变换矩阵数组
 * @param cntModel 模型数量
 * @param zBuffer 深度缓冲区
 * @param colorBuffer 颜色缓冲区
 * @param depth 深度图像
 * @return 光源视图-投影-视口变换的组合矩阵
 */
Matrix shadowMapping(Model **modelData, Matrix *modelTrans, unsigned cntModel, float *zBuffer, Vec3f *colorBuffer, TGAImage &depth)
{
	// 设置光照视角的视图矩阵
	Matrix view = lookat(lightPos, center, up);
	// 设置正交投影矩阵（正交投影避免透视失真，适合阴影映射）
	Matrix project = ortho(-2.0f, 2.0f, -2.0f, 2.0f, -0.01f, -10.0f);
	// 设置视口变换矩阵（将NDC坐标转换为屏幕坐标）
	Matrix vp = viewport(SHADOW_WIDTH, SHADOW_HEIGHT);

	// 遍历所有模型
	for (unsigned m = 0; m < cntModel; ++m)
	{
		// 创建深度着色器并设置统一变量
		DepthShader depthShader;
		depthShader.uVpPV = vp * project*view;  // 组合变换矩阵

		// 渲染管线：从光源视角计算深度
		Matrix modelInverTranspose = modelTrans[m].invert_transpose();  // 计算模型变换矩阵的逆转置（用于法线变换）
		for (int i = 0; i < modelData[m]->nfaces(); ++i)  // 遍历模型的每个面
		{
			// 顶点处理阶段
			Vec4f screenCoords[3];  // 存储变换后的顶点坐标
			for (int j = 0; j < 3; ++j)  // 处理三角形的三个顶点
			{
				// 将顶点从模型空间变换到世界空间
				Vec4f worldCoord = modelTrans[m] * embed<4>(modelData[m]->vert(i, j));
				Vec2f uv = modelData[m]->uv(i, j);  // 获取纹理坐标
				// 变换法线向量（使用逆转置矩阵）
				Vec3f normal = proj<3>(modelInverTranspose * Vec4f(modelData[m]->normal(i, j), 0.0f));
				// 调用顶点着色器处理顶点
				screenCoords[j] = depthShader.vertex(j, worldCoord, uv, normal);
			}

			// 光栅化 + 片段处理阶段
			// 使用非MSAA模式渲染三角形到深度缓冲区
			triangle(screenCoords, depthShader, colorBuffer, zBuffer, SHADOW_WIDTH, SHADOW_HEIGHT, D_NonMSAA, 1);
		}
	}
	
	// 返回光源的视图-投影-视口变换组合矩阵（用于后续阴影计算）
	return vp * project * view;
}

/**
 * Phong着色函数：使用Phong着色模型渲染场景
 * @param modelData 模型数据数组
 * @param modelTrans 模型变换矩阵数组
 * @param cntModel 模型数量
 * @param zBuffer 深度缓冲区
 * @param colorBuffer 颜色缓冲区
 * @param lightVpPV 光源视图-投影-视口变换组合矩阵
 * @param shadowBuffer 阴影缓冲区
 * @param frame 输出图像
 */
void PhongShading(Model **modelData, Matrix *modelTrans, unsigned cntModel, float *zBuffer, Vec3f *colorBuffer, Matrix lightVpPV, float *shadowBuffer, TGAImage &frame)
{
	// 设置相机视角的视图矩阵
	Matrix view = lookat(eye, center, up);
	// 设置透视投影矩阵（FOV=45度，宽高比=1.0）
	Matrix project = projection(PI / 4.0f, 1.0f, -0.01f, -10.0f);
	// 设置视口变换矩阵
	Matrix vp = viewport(SCREEN_WIDTH, SCREEN_HEIGHT);
	Matrix PV = project * view;  // 组合投影和视图矩阵，用于裁剪

	// 遍历所有模型
	for (unsigned m = 0; m < cntModel; ++m)
	{
		// 创建Phong着色器并设置统一变量
		Shader PhongShader;
		PhongShader.uTexture = modelData[m];  // 设置模型纹理
		PhongShader.uModel = modelTrans[m];  // 设置模型变换矩阵
		PhongShader.uVpPV = vp * project * view;  // 设置视图-投影-视口变换组合矩阵
		PhongShader.uLightVpPV = lightVpPV;  // 设置光源视图-投影-视口变换组合矩阵
		PhongShader.uEyePos = eye;  // 设置相机位置
		PhongShader.uLightPos = lightPos;  // 设置光源位置
		PhongShader.uLightColor = lightColor;  // 设置光源颜色
		PhongShader.uShadowBuffer = shadowBuffer;  // 设置阴影缓冲区
		PhongShader.uShadowBufferWidth = SHADOW_WIDTH;  // 设置阴影缓冲区宽度
		PhongShader.uShadowBufferHeight = SHADOW_HEIGHT;  // 设置阴影缓冲区高度

		// 渲染管线：计算每个采样的信息
		for (int i = 0; i < modelData[m]->nfaces(); ++i)  // 遍历模型的每个面
		{
			// 背面剔除：计算面法线并判断是否背向相机
			Vec3f n = cross(modelData[m]->vert(i, 1) - modelData[m]->vert(i, 0), modelData[m]->vert(i, 2) - modelData[m]->vert(i, 0)).normalize();
			// 将法线从模型空间变换到相机空间
			n = proj<3>((view * modelTrans[m]).invert_transpose() * Vec4f(n, 0.0f));
			if (n.z <= 0.0f) continue;  // 如果面背向相机则跳过

			// z轴裁剪：去除远近平面外的面片
			Matrix modelInverTranspose = modelTrans[m].invert_transpose();  // 计算模型变换矩阵的逆转置
			std::vector<Vertex> original, clipped;  // 原始顶点和裁剪后顶点
			for (int j = 0; j < 3; j++)  // 处理三角形的三个顶点
			{
				// 将顶点从模型空间变换到世界空间
				Vec4f worldCoord = modelTrans[m] * embed<4>(modelData[m]->vert(i, j));
				// 将顶点变换到裁剪空间
				Vec4f clipCoord = PV * worldCoord;
				// 变换法线向量
				Vec3f normal = proj<3>(modelInverTranspose * Vec4f(modelData[m]->normal(i, j), 0.0f));
				Vec2f uv = modelData[m]->uv(i, j);  // 获取纹理坐标
				// 创建顶点对象并存入原始顶点列表
				Vertex vertex(worldCoord, clipCoord, uv, normal);
				original.push_back(vertex);
			}
			// 执行齐次裁剪（z平面）
			homogeneousClip(original, clipped, 2);

			// 视锥体剔除：如果三角形完全在视锥体外则跳过
			if (clipped.size() < 3) continue;  // 裁剪后少于3个顶点，无法形成三角形

			// 计算三角形的切线和副切线向量（用于法线映射）
			mat<2, 3, float> A;  // 存储边向量
			// 计算三角形的两条边向量
			A[0] = proj<3>(modelTrans[m] * Vec4f(modelData[m]->vert(i, 1) - modelData[m]->vert(i, 0), 0.0f));
			A[1] = proj<3>(modelTrans[m] * Vec4f(modelData[m]->vert(i, 2) - modelData[m]->vert(i, 0), 0.0f));
			mat<2, 2, float> U;  // 存储纹理坐标差值
			// 计算纹理坐标的差值
			U[0] = modelData[m]->uv(i, 1) - modelData[m]->uv(i, 0);
			U[1] = modelData[m]->uv(i, 2) - modelData[m]->uv(i, 0);
			// 通过求解线性方程组计算切线和副切线
			mat<2, 3, float> tTB = U.invert() * A;
			PhongShader.uTangent = tTB[0].normalize();  // 归一化切线向量
			PhongShader.uBitangent = tTB[1].normalize();  // 归一化副切线向量

			// 对每个子三角形进行着色（裁剪可能产生多个三角形）
			for (size_t j = 1; j < clipped.size() - 1; ++j)
			{
				// 顶点处理：调用顶点着色器处理每个顶点
				Vec4f screenCoords[3];
				screenCoords[0] = PhongShader.vertex(0, clipped[0].worldCoord, clipped[0].uv, clipped[0].normal);
				screenCoords[1] = PhongShader.vertex(1, clipped[j].worldCoord, clipped[j].uv, clipped[j].normal);
				screenCoords[2] = PhongShader.vertex(2, clipped[j+1].worldCoord, clipped[j+1].uv, clipped[j+1].normal);

				// 光栅化 + 片段处理：使用MSAA渲染三角形
				triangle(screenCoords, PhongShader, colorBuffer, zBuffer, SCREEN_WIDTH, SCREEN_HEIGHT, D_MSAA, CNT_SAMPLE);
			}
		}
	}
}

/**
 * 将深度颜色写入TGA图像
 * @param depth 输出的深度图像
 * @param colorBuffer 颜色缓冲区
 */
void writeDepth(TGAImage &depth, Vec3f *colorBuffer)
{
	// 将深度颜色写入TGAImage（用于调试和可视化）
	for (unsigned x = 0; x < depth.get_width(); ++x)
	{
		for (unsigned y = 0; y < depth.get_height(); ++y)
		{
			Vec3f color = colorBuffer[y * depth.get_width() + x];
			depth.set(x, y, TGAColor(color.x, color.y, color.z, 255));
		}
	}
}

/**
 * 将渲染结果写入TGA图像
 * @param frame 输出图像
 * @param zBuffer 深度缓冲区
 * @param colorBuffer 颜色缓冲区
 * @param cntSample 每个像素的采样数
 */

void writeFrame(TGAImage &frame, float *zBuffer, Vec3f *colorBuffer, unsigned cntSample)
{
	// 将着色结果写入TGA图像，对每个像素的MSAA采样进行平均
	for (unsigned x = 0; x < frame.get_width(); ++x)
	{
		for (unsigned y = 0; y < frame.get_height(); ++y)
		{
			if (x == 362 && y == 417)
				x = x;  // 调试断点位置
			Vec3f color(0.0f, 0.0f, 0.0f);  // 初始化颜色为黑色
			for (unsigned i = 0; i < cntSample; ++i)  // 遍历像素的所有采样点
			{
				// 如果采样点有深度值（被渲染）则累加颜色
				if (zBuffer[cntSample * (y*frame.get_width() + x) + i] > -std::numeric_limits<float>::max())
				{
					color = color + colorBuffer[cntSample * (y*frame.get_width() + x) + i];
				}
			}
			color = color / cntSample;  // 计算平均颜色（MSAA抗锯齿原理）
			frame.set(x, y, TGAColor(color.x, color.y, color.z, 255));  // 设置像素颜色
		}
	}
}

/**
 * 主函数：程序入口点,main函数是程序的入口点，程序从这里开始执行
 * 程序的执行顺序是main函数->PhongShading函数->triangle函数->homogeneousClip函数->singleFaceZClip函数->pushIntersection函数
 */
int main()
{
	if (!std::filesystem::exists("./thisoutput")) {
		std::filesystem::create_directory("./thisoutput");
	}

	// 分配缓冲区内存
	float *zBuffer = new float[SCREEN_WIDTH * SCREEN_HEIGHT * CNT_SAMPLE];  // 深度缓冲区
	Vec3f *colorBuffer = new Vec3f[SCREEN_WIDTH * SCREEN_HEIGHT * CNT_SAMPLE];  // 颜色缓冲区
	float *shadowZBuffer = new float[SCREEN_WIDTH * SCREEN_HEIGHT];  // 阴影深度缓冲区
	Vec3f *shadowColorBuffer = new Vec3f[SCREEN_WIDTH * SCREEN_HEIGHT];  // 阴影颜色缓冲区
	
	// 初始化缓冲区
	for (int i = 0; i < SCREEN_WIDTH*SCREEN_HEIGHT; ++i)
	{
		for (int j = 0; j < CNT_SAMPLE; ++j)
		{
			zBuffer[CNT_SAMPLE * i + j] = -std::numeric_limits<float>::max();  // 初始化深度为负无穷
			colorBuffer[CNT_SAMPLE * i + j] = Vec3f(0.0f, 0.0f, 0.0f);  // 初始化颜色为黑色
		}
		shadowZBuffer[i] = -std::numeric_limits<float>::max();  // 初始化阴影深度为负无穷
		shadowColorBuffer[i] = Vec3f(0.0f, 0.0f, 0.0f);  // 初始化阴影颜色为黑色
	}

	// 加载模型
	unsigned cntModel = 2;  // 模型数量
	Model **modelData = new Model*[cntModel];  // 创建模型数组

	modelData[0] = new Model("C:/Users/25498/Desktop/Rasterizer2/dragon_obj/diablo3pose.obj");  // 这里一定一定要写成绝对路径!!!
	//而且纹理tga的文件命名要按_diffuse和_nm_tangent以及_spec来命名, 这都是因为model函数里面的规范一定要继承

	modelData[1] = new Model("C:/Users/25498/Desktop/Rasterizer2/obj/floor.obj");  // 加载地板模型
	//两个模型的文件和文件夹都放在了和.sln解决方案同一个目录下:  C:\Users\25498\Desktop\Rasterizer2
	//最终输出储存在这个路径:  C:\Users\25498\Desktop\Rasterizer2\Rasterizer2\thisoutput

	std::cerr << std::endl;  // 输出空行
	
	// 设置每个模型的变换矩阵
	Matrix *modelTrans = new Matrix[cntModel];
	modelTrans[0] = Matrix::identity();  // 人头模型使用单位矩阵（不变换）
	modelTrans[1] = Matrix::identity();  // 地板模型基于单位矩阵
	modelTrans[1][1][3] = -0.3f;  // 在y方向（高度）上偏移地板

	// 阴影通道：从光源角度渲染深度图
	TGAImage depth(SCREEN_WIDTH, SCREEN_HEIGHT, TGAImage::RGB);  // 创建深度图像
	// 生成阴影贴图并获取光源变换矩阵
	Matrix lightVpPV = shadowMapping(modelData, modelTrans, cntModel, shadowZBuffer, shadowColorBuffer, depth);
	std::cerr << "finish shadow depth buffer calculation" << std::endl;  // 输出进度信息
	writeDepth(depth, shadowColorBuffer);  // 将深度缓冲区写入图像
	depth.write_tga_file("thisoutput/new_depth.tga");  // 保存深度图像
	std::cerr << "finish writing depth.tga" << std::endl;  // 输出进度信息
	std::cerr << "Shadow Pass Over" << std::endl << std::endl;  // 输出阶段完成信息

	// 着色通道：从相机角度渲染场景
	TGAImage frame(SCREEN_WIDTH, SCREEN_HEIGHT, TGAImage::RGB);  // 创建输出图像
	// 使用Phong着色模型渲染场景
	PhongShading(modelData, modelTrans, cntModel, zBuffer, colorBuffer, lightVpPV, shadowZBuffer, frame);
	std::cerr << "finish shading" << std::endl;  // 输出进度信息
	writeFrame(frame, zBuffer, colorBuffer, CNT_SAMPLE);  // 将渲染结果写入图像
	frame.write_tga_file("thisoutput/new_frame.tga");  // 保存渲染图像
	std::cerr << "finish writing frame.tga" << std::endl;  // 输出进度信息
	std::cerr << "Shading Pass Over" << std::endl << std::endl;  // 输出阶段完成信息

	// 释放资源
	for (unsigned i = 0; i < cntModel; ++i)
	{
		delete modelData[i];  // 释放每个模型占用的内存
	}
	delete[] modelData;    // 释放模型数组
	delete[] modelTrans;   // 释放变换矩阵数组
	delete[] zBuffer;      // 释放深度缓冲区
	delete[] colorBuffer;  // 释放颜色缓冲区
	delete[] shadowZBuffer;     // 释放阴影深度缓冲区
	delete[] shadowColorBuffer; // 释放阴影颜色缓冲区

	return 0;  // 程序正常结束
}
