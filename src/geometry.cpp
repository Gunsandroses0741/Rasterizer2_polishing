#include "geometry.h" // 包含 geometry.h 头文件，其中定义了 vec 类模板和相关函数声明

// 下面是一系列 vec 类模板的类型转换构造函数的显式模板特化定义
// 这些特化允许在不同数值类型 (如 float 和 int) 的同维度向量之间进行转换

// 特化：从 vec<4, float> 转换为 vec<4, int>
// v: 输入的四维浮点向量
// 实现方式：对每个浮点分量加上 0.5f (用于四舍五入) 然后转换为 int 类型
template <> template <> vec<4, int>  ::vec(const vec<4, float> &v) : x(int(v.x + .5f)), y(int(v.y + .5f)), z(int(v.z + .5f)), w(int(v.w + .5f)) {}

// 特化：从 vec<4, int> 转换为 vec<4, float>
// v: 输入的四维整型向量
// 实现方式：直接将整型分量赋值给浮点分量
template <> template <> vec<4, float>::vec(const vec<4, int> &v) : x(v.x), y(v.y), z(v.z), w(v.w) {}

// 特化：从 vec<3, float> 转换为 vec<3, int>
// v: 输入的三维浮点向量
// 实现方式：对每个浮点分量加上 0.5f (四舍五入) 然后转换为 int 类型
template <> template <> vec<3, int>  ::vec(const vec<3, float> &v) : x(int(v.x + .5f)), y(int(v.y + .5f)), z(int(v.z + .5f)) {}

// 特化：从 vec<3, int> 转换为 vec<3, float>
// v: 输入的三维整型向量
// 实现方式：直接将整型分量赋值给浮点分量
template <> template <> vec<3, float>::vec(const vec<3, int> &v) : x(v.x), y(v.y), z(v.z) {}

// 特化：从 vec<2, float> 转换为 vec<2, int>
// v: 输入的二维浮点向量
// 实现方式：对每个浮点分量加上 0.5f (四舍五入) 然后转换为 int 类型
template <> template <> vec<2, int>  ::vec(const vec<2, float> &v) : x(int(v.x + .5f)), y(int(v.y + .5f)) {}

// 特化：从 vec<2, int> 转换为 vec<2, float>
// v: 输入的二维整型向量
// 实现方式：直接将整型分量赋值给浮点分量
template <> template <> vec<2, float>::vec(const vec<2, int> &v) : x(v.x), y(v.y) {}
