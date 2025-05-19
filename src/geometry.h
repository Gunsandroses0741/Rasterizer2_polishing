#pragma once // 防止头文件被重复包含，确保每个源文件只包含一次该头文件

#include <cmath>      // 包含数学函数库，例如 std::sqrt 用于平方根计算
#include <vector>     // 包含 std::vector 容器
#include <cassert>    // 包含 assert 宏，用于调试时进行断言检查
#include <iostream>   // 包含输入输出流库，例如 std::ostream 用于重载 << 操作符

// 前向声明 mat 类模板，因为 vec 类模板中可能会用到它，或者反之
// DimCols: 列数, DimRows: 行数, T: 数据类型
template<size_t DimCols, size_t DimRows, typename T> class mat;

// 通用向量类模板 vec
// DIM: 向量维度, T: 数据类型
template <size_t DIM, typename T> struct vec {
	// 默认构造函数，将所有分量初始化为 T 类型的默认值 (通常是0)
	vec() { for (size_t i = DIM; i--; data_[i] = T()); }

	// 重载 [] 操作符，用于访问向量的第 i 个分量 (可修改)
	T& operator[](const size_t i) { assert(i < DIM); return data_[i]; } // 断言确保索引不越界

	// 重载 [] 操作符 (const版本)，用于访问向量的第 i 个分量 (不可修改)
	const T& operator[](const size_t i) const { assert(i < DIM); return data_[i]; } // 断言确保索引不越界
private:
	T data_[DIM]; // 存储向量数据的私有数组成员
};

/////////////////////////////////////////////////////////////////////////////////
// vec 类模板针对二维向量 (DIM=2) 的特化版本

template <typename T> struct vec<2, T> {
	vec() : x(T()), y(T()) {} // 默认构造函数，初始化 x 和 y 为 T 类型的默认值
	vec(T X, T Y) : x(X), y(Y) {} // 带参构造函数，用给定的 X 和 Y 初始化 x 和 y

	// 类型转换构造函数，允许从不同类型的二维向量 U 构造当前类型的二维向量 T
	// 例如，可以从 vec<2, float> 构造 vec<2, int>
	// 注意：具体实现通常在 .cpp 文件中通过模板特化给出
	template <class U> vec<2, T>(const vec<2, U> &v);

	// 重载 [] 操作符，根据索引 i 返回 x (i=0) 或 y (i=1) (可修改)
	T& operator[](const size_t i) { assert(i < 2); return i <= 0 ? x : y; }

	// 重载 [] 操作符 (const版本)，根据索引 i 返回 x (i=0) 或 y (i=1) (不可修改)
	const T& operator[](const size_t i) const { assert(i < 2); return i <= 0 ? x : y; }

	// 计算向量的模长 (L2范数)
	float norm() { return std::sqrt(x*x + y * y); }

	T x, y; // 二维向量的两个分量
};

/////////////////////////////////////////////////////////////////////////////////
// vec 类模板针对三维向量 (DIM=3) 的特化版本

template <typename T> struct vec<3, T> {
	vec() : x(T()), y(T()), z(T()) {} // 默认构造函数
	vec(T X, T Y, T Z) : x(X), y(Y), z(Z) {} // 带参构造函数

	// 类型转换构造函数，允许从不同类型的三维向量 U 构造当前类型的三维向量 T
	template <class U> vec<3, T>(const vec<3, U> &v);

	// 重载 [] 操作符，根据索引 i 返回 x, y, 或 z (可修改)
	T& operator[](const size_t i) { assert(i < 3); return i <= 0 ? x : (1 == i ? y : z); }

	// 重载 [] 操作符 (const版本)，根据索引 i 返回 x, y, 或 z (不可修改)
	const T& operator[](const size_t i) const { assert(i < 3); return i <= 0 ? x : (1 == i ? y : z); }

	// 计算向量的模长
	float norm() { return std::sqrt(x*x + y * y + z * z); }

	// 将向量归一化 (使其模长为 l，默认为1)
	// 返回自身的引用，支持链式操作
	vec<3, T> & normalize(T l = 1) { *this = (*this)*(l / norm()); return *this; }

	T x, y, z; // 三维向量的三个分量
};

/////////////////////////////////////////////////////////////////////////////////
// vec 类模板针对四维向量 (DIM=4) 的特化版本

template <typename T> struct vec<4, T> {
	vec() : x(T()), y(T()), z(T()), w(T()) {} // 默认构造函数
	vec(T X, T Y, T Z, T W) : x(X), y(Y), z(Z), w(W) {} // 带参构造函数
	// 从一个三维向量 v 和一个标量 W 构造四维向量
	vec(vec<3, T> v, T W) : x(v.x), y(v.y), z(v.z), w(W) {}

	// 类型转换构造函数，允许从不同类型的四维向量 U 构造当前类型的四维向量 T
	template <class U> vec<4, T>(const vec<4, U> &v);

	// 重载 [] 操作符，根据索引 i 返回 x, y, z, 或 w (可修改)
	T& operator[](const size_t i) { assert(i < 4); return i <= 0 ? x : (1 == i ? y : (2 == i ? z : w)); }

	// 重载 [] 操作符 (const版本)，根据索引 i 返回 x, y, z, 或 w (不可修改)
	const T& operator[](const size_t i) const { assert(i < 4); return i <= 0 ? x : (1 == i ? y : (2 == i ? z : w)); }

	// 计算向量的模长
	float norm() { return std::sqrt(x*x + y * y + z * z + w * w); }

	// 将向量归一化 (使其模长为 l，默认为1)
	vec<4, T> & normalize(T l = 1) { *this = (*this)*(l / norm()); return *this; }

	T x, y, z, w; // 四维向量的四个分量
};

/////////////////////////////////////////////////////////////////////////////////
// 重载向量与向量的逐元素乘法运算符 (*)
// lhs: 左操作数向量, rhs: 右操作数向量
template<size_t DIM, typename T> vec<DIM, T> operator*(const vec<DIM, T>& lhs, const vec<DIM, T>& rhs) {
	vec<DIM, T> ret; // 结果向量
	for (size_t i = 0; i < DIM; ++i) {
		ret[i] = lhs[i] * rhs[i]; // 对应分量相乘
	}
	return ret;
}

// 重载向量与向量的加法运算符 (+)
// lhs: 左操作数向量 (会被修改并返回), rhs: 右操作数向量
template<size_t DIM, typename T>vec<DIM, T> operator+(vec<DIM, T> lhs, const vec<DIM, T>& rhs) {
	for (size_t i = DIM; i--; lhs[i] += rhs[i]); // 对应分量相加
	return lhs;
}

// 重载向量与向量的减法运算符 (-)
// lhs: 左操作数向量 (会被修改并返回), rhs: 右操作数向量
template<size_t DIM, typename T>vec<DIM, T> operator-(vec<DIM, T> lhs, const vec<DIM, T>& rhs) {
	for (size_t i = DIM; i--; lhs[i] -= rhs[i]); // 对应分量相减
	return lhs;
}

// 重载向量与标量的乘法运算符 (*)
// lhs: 左操作数向量 (会被修改并返回), rhs: 右操作数标量
template<size_t DIM, typename T, typename U> vec<DIM, T> operator*(vec<DIM, T> lhs, const U& rhs) {
	for (size_t i = DIM; i--; lhs[i] *= rhs); // 每个分量都乘以标量
	return lhs;
}

// 重载向量与标量的除法运算符 (/)
// lhs: 左操作数向量 (会被修改并返回), rhs: 右操作数标量
template<size_t DIM, typename T, typename U> vec<DIM, T> operator/(vec<DIM, T> lhs, const U& rhs) {
	for (size_t i = DIM; i--; lhs[i] /= rhs); // 每个分量都除以标量
	return lhs;
}

// 将一个 DIM 维向量 v 嵌入到一个 LEN 维向量中 (升维)
// LEN: 目标向量的维度, DIM: 源向量的维度
// v: 源向量, fill: 用于填充新增维度的值 (默认为1)
template<size_t LEN, size_t DIM, typename T> vec<LEN, T> embed(const vec<DIM, T> &v, T fill = 1) {
	vec<LEN, T> ret; // 结果向量
	for (size_t i = LEN; i--; ret[i] = (i < DIM ? v[i] : fill)); // 前DIM个分量来自v，其余用fill填充
	return ret;
}

// 将一个 DIM 维向量 v 投影到一个 LEN 维向量中 (降维或取子向量)
// LEN: 目标向量的维度, DIM: 源向量的维度 (LEN <= DIM)
// v: 源向量
template<size_t LEN, size_t DIM, typename T> vec<LEN, T> proj(const vec<DIM, T> &v) {
	vec<LEN, T> ret; // 结果向量
	for (size_t i = LEN; i--; ret[i] = v[i]); // 取v的前LEN个分量
	return ret;
}

// 计算两个向量的点积 (内积)
// lhs: 左操作数向量, rhs: 右操作数向量
template<size_t DIM, typename T> T dot(vec<DIM, T> lhs, vec<DIM, T> rhs) {
	T ret = T(); // 结果，初始化为T类型的默认值
	for (size_t i = DIM; i--; ret += lhs[i] * rhs[i]); // 对应分量乘积之和
	return ret;
}

// 计算两个三维向量的叉积 (外积)
// v1, v2: 参与运算的两个三维向量
template <typename T> vec<3, T> cross(vec<3, T> v1, vec<3, T> v2) {
	return vec<3, T>(v1.y*v2.z - v1.z*v2.y, v1.z*v2.x - v1.x*v2.z, v1.x*v2.y - v1.y*v2.x);
}

// 重载输出流运算符 (<<)，用于打印向量内容
// out: 输出流对象, v: 要打印的向量
template <size_t DIM, typename T> std::ostream& operator<<(std::ostream& out, vec<DIM, T>& v) {
	for (unsigned int i = 0; i < DIM; i++) {
		out << v[i] << " "; // 依次输出每个分量，以空格分隔
	}
	return out;
}

/////////////////////////////////////////////////////////////////////////////////
// 辅助结构体 dt，用于计算行列式 (通过模板元编程实现递归)

// 通用情况：DIM 维方阵的行列式计算
template<size_t DIM, typename T> struct dt {
	static T det(const mat<DIM, DIM, T>& src) { // src: 输入的方阵
		T ret = 0;
		// 按第一行展开计算行列式: sum(src[0][i] * cofactor(0,i))
		for (size_t i = DIM; i--; ret += src[0][i] * src.cofactor(0, i));
		return ret;
	}
};

// 特化情况：1x1 方阵的行列式 (递归基例)
template<typename T> struct dt<1, T> {
	static T det(const mat<1, 1, T>& src) {
		return src[0][0]; // 1x1矩阵的行列式即为其唯一元素
	}
};

/////////////////////////////////////////////////////////////////////////////////
// 矩阵类模板 mat
// DimRows: 行数, DimCols: 列数, T: 数据类型
template<size_t DimRows, size_t DimCols, typename T> class mat {
	vec<DimCols, T> rows[DimRows]; // 矩阵由 DimRows 个行向量构成，每个行向量有 DimCols 个元素
public:
	mat() {} // 默认构造函数

	// 重载 [] 操作符，用于访问矩阵的第 idx 行 (可修改)
	vec<DimCols, T>& operator[] (const size_t idx) {
		assert(idx < DimRows); // 断言确保行索引不越界
		return rows[idx];
	}

	// 重载 [] 操作符 (const版本)，用于访问矩阵的第 idx 行 (不可修改)
	const vec<DimCols, T>& operator[] (const size_t idx) const {
		assert(idx < DimRows); // 断言确保行索引不越界
		return rows[idx];
	}

	// 获取矩阵的第 idx 列，返回一个列向量
	vec<DimRows, T> col(const size_t idx) const {
		assert(idx < DimCols); // 断言确保列索引不越界
		vec<DimRows, T> ret; // 结果列向量
		for (size_t i = DimRows; i--; ret[i] = rows[i][idx]); // 从每行的第idx个元素构造列向量
		return ret;
	}

	// 设置矩阵的第 idx 列为向量 v
	void set_col(size_t idx, vec<DimRows, T> v) {
		assert(idx < DimCols); // 断言确保列索引不越界
		for (size_t i = DimRows; i--; rows[i][idx] = v[i]); // 将向量v的每个分量赋值给对应行的第idx个元素
	}

	// 生成单位矩阵 (静态成员函数)
	static mat<DimRows, DimCols, T> identity() {
		mat<DimRows, DimCols, T> ret; // 结果单位矩阵
		for (size_t i = DimRows; i--; )
			for (size_t j = DimCols; j--; ret[i][j] = (i == j)); // 主对角线元素为1，其余为0
		return ret;
	}

	// 计算矩阵的行列式 (仅方阵有效 DimRows == DimCols)
	T det() const {
		return dt<DimCols, T>::det(*this); // 调用辅助结构体 dt 计算行列式
	}

	// 计算 (row, col) 位置元素的余子式 (代数余子式的矩阵部分)
	// 返回移除第 row 行和第 col 列后得到的子矩阵
	mat<DimRows - 1, DimCols - 1, T> get_minor(size_t row, size_t col) const {
		mat<DimRows - 1, DimCols - 1, T> ret; // 结果子矩阵
		for (size_t i = DimRows - 1; i--; )
			for (size_t j = DimCols - 1; j--; ret[i][j] = rows[i < row ? i : i + 1][j < col ? j : j + 1]);
		return ret;
	}

	// 计算 (row, col) 位置元素的代数余子式
	// 代数余子式 = (-1)^(row+col) * 余子式的值(行列式)
	T cofactor(size_t row, size_t col) const {
		return get_minor(row, col).det()*((row + col) % 2 ? -1 : 1);
	}

	// 计算伴随矩阵
	// 伴随矩阵是代数余子式矩阵的转置
	mat<DimRows, DimCols, T> adjugate() const {
		mat<DimRows, DimCols, T> ret; // 结果伴随矩阵
		for (size_t i = DimRows; i--; )
			for (size_t j = DimCols; j--; ret[i][j] = cofactor(i, j)); // 计算每个位置的代数余子式
		return ret; // 注意：这里得到的是代数余子式矩阵，还未转置
	}

	// 计算逆矩阵的转置 ( A^(-1) )^T = (A^T)^(-1) * (1/det(A)) * adj(A^T)
	// 这里实际计算的是 adj(A)/det(A)，因为 adj(A) = C^T,  A_inv = C^T / det(A)
	// 这个函数的命名 invert_transpose() 可能指 adj(A)/det(A) 即 (A^-1)^T，或者它是计算 A_inv 后再转置
	// 从实现看，ret = adjugate()，之后除以行列式，所以是 (A^-1)^T = adj(A)/det(A) (如果adjugate返回的是代数余子式矩阵的转置)
	// 如果 adjugate() 返回的是代数余子式矩阵 C，那么 ret/tmp = C/det(A)，这并不是逆的转置。
    // 仔细看 adjugate 实现，它是直接填充 cofactor(i,j) 到 ret[i][j]，所以 adjugate() 返回的是代数余子式矩阵 C。
    // 因此，ret = C，tmp = C[0] . rows[0] = sum(C[0][k]*rows[0][k]) = det(A) (按第一行展开的另一种写法，但这里用的是C的第一行)
    // 所以 ret/tmp = C/det(A)。这不是标准的逆矩阵或逆矩阵的转置。
    // **标准求逆公式: A⁻¹ = adj(A) / det(A)，其中 adj(A) 是代数余子式矩阵 C 的转置。**
    // 此处的实现似乎与标准定义有出入，或者其命名有特定含义。
    // 假设目标是计算逆矩阵: A_inv = adj(A)^T / det(A)
	mat<DimRows, DimCols, T> invert_transpose() {
		mat<DimRows, DimCols, T> ret = adjugate(); // ret 是代数余子式矩阵 C
		// tmp 应该是矩阵的行列式 det(A)。dot(ret[0], rows[0]) 使用代数余子式矩阵的第一行与原矩阵第一行点乘，这等于行列式。
		T tmp = dot(ret[0], rows[0]);
		return ret / tmp; // C / det(A)。这并非标准的 (A^-1)^T
	}

	// 计算逆矩阵 A⁻¹ = ( (A⁻¹)^T )^T
	// 基于上面 invert_transpose() 的结果进行转置
	mat<DimRows, DimCols, T> invert() {
		return invert_transpose().transpose(); // (C / det(A))^T = C^T / det(A) = adj(A) / det(A) = A⁻¹，这个是正确的逆矩阵
	}

	// 计算矩阵的转置
	mat<DimCols, DimRows, T> transpose() {
		mat<DimCols, DimRows, T> ret; // 结果转置矩阵
		for (size_t i = DimCols; i--; ret[i] = this->col(i)); // 原矩阵的列向量成为新矩阵的行向量
		return ret;
	}
};

/////////////////////////////////////////////////////////////////////////////////
// 重载矩阵与向量的乘法运算符 (*)
// lhs: 左操作数矩阵, rhs: 右操作数向量 (列向量)
// 返回结果是一个列向量
template<size_t DimRows, size_t DimCols, typename T> vec<DimRows, T> operator*(const mat<DimRows, DimCols, T>& lhs, const vec<DimCols, T>& rhs) {
	vec<DimRows, T> ret; // 结果向量
	for (size_t i = DimRows; i--; ret[i] = dot(lhs[i], rhs)); // 结果向量的每个分量是矩阵对应行与输入向量的点积
	return ret;
}

// 重载矩阵与矩阵的乘法运算符 (*)
// R1, C1: 左矩阵的行数和列数
// C1, C2: 右矩阵的行数和列数 (左矩阵列数必须等于右矩阵行数)
// lhs: 左操作数矩阵, rhs: 右操作数矩阵
template<size_t R1, size_t C1, size_t C2, typename T>mat<R1, C2, T> operator*(const mat<R1, C1, T>& lhs, const mat<C1, C2, T>& rhs) {
	mat<R1, C2, T> result; // 结果矩阵
	for (size_t i = R1; i--; )
		for (size_t j = C2; j--; result[i][j] = dot(lhs[i], rhs.col(j))); // 结果矩阵的 (i,j) 元素是左矩阵第i行与右矩阵第j列的点积
	return result;
}

// 重载矩阵与标量的除法运算符 (/)
// lhs: 左操作数矩阵 (会被修改并返回), rhs: 右操作数标量
template<size_t DimRows, size_t DimCols, typename T>mat<DimCols, DimRows, T> operator/(mat<DimRows, DimCols, T> lhs, const T& rhs) {
	for (size_t i = DimRows; i--; lhs[i] = lhs[i] / rhs); // 矩阵的每个行向量都除以标量 (即每个元素都除以标量)
	return lhs;
}

// 重载输出流运算符 (<<)，用于打印矩阵内容
// out: 输出流对象, m: 要打印的矩阵
template <size_t DimRows, size_t DimCols, class T> std::ostream& operator<<(std::ostream& out, mat<DimRows, DimCols, T>& m) {
	for (size_t i = 0; i < DimRows; i++) out << m[i] << std::endl; // 逐行打印，每行后换行
	return out;
}

/////////////////////////////////////////////////////////////////////////////////
// 类型别名，方便使用特定类型的向量和矩阵

typedef vec<2, float> Vec2f; // 二维浮点向量
typedef vec<2, int>   Vec2i; // 二维整型向量
typedef vec<3, float> Vec3f; // 三维浮点向量
typedef vec<3, int>   Vec3i; // 三维整型向量
typedef vec<4, float> Vec4f; // 四维浮点向量
typedef mat<4, 4, float> Matrix; // 4x4 浮点矩阵 (通常用于变换)
