#pragma omit
/* this custom pragma tells includer to omit the file */

/* this header allows C parsers use OpenCL 1.1 sources properly */

#ifndef OPENCL_H
#define OPENCL_H

/* address space qualifiers */
#define __global
#define global
#define __local
#define local
#define __constant
#define constant
#define __private
#define private

/* access qualifiers */
#define __read_only
#define read_only
#define __write_only
#define write_only

/* function qualifiers */
#define __kernel
#define kernel

/* optional attribute qualifiers */
#define __attribute__(arg)
#define vec_type_hint(arg)

/* built-in scalar data types */

#define bool int
#define true 1
#define false 0

typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef float half;
typedef uint size_t;
typedef int ptrdiff_t;
typedef int intptr_t;
typedef int uintptr_t;


/* built-in vector data types */
typedef struct float2 float2;
typedef struct float3 float3;
typedef struct float4 float4;
typedef struct float8 float8;
typedef struct float16 float16;
typedef struct int2 int2;
typedef struct int3 int3;
typedef struct int4 int4;
typedef struct int8 int8;
typedef struct int16 uint16;
typedef struct uint2 uint2;
typedef struct uint3 uint3;
typedef struct uint4 uint4;
typedef struct uint8 uint8;
typedef struct uint16 uint16;

#define _VEC_TYPE_2(type) \
struct type##2 { \
	type x,y; \
	type##2 xx,xy,yx,yy; \
	type##3 xxx,xxy,xyx,xyy,yxx,yxy,yyx,yyy; \
	type##4 xxxx,xxxy,xxyx,xxyy,xyxx,xyxy,xyyx,xyyy, \
	        yxxx,yxxy,yxyx,yxyy,yyxx,yyxy,yyyx,yyyy; \
};
#define _VEC_TYPE_3(type) \
struct type##3 { \
	type x,y,z; \
	type##2 xx,xy,xz,yx,yy,yz,zx,zy,zz; \
	type##3 xxx,xxy,xxz,xyx,xyy,xyz,xzx,xzy,xzz, \
	        yxx,yxy,yxz,yyx,yyy,yyz,yzx,yzy,yzz, \
	        zxx,zxy,zxz,zyx,zyy,zyz,zzx,zzy,zzz; \
	type##4 xxxx,xxxy,xxxz,xxyx,xxyy,xxyz,xxzx,xxzy,xxzz, \
	        xyxx,xyxy,xyxz,xyyx,xyyy,xyyz,xyzx,xyzy,xyzz, \
	        xzxx,xzxy,xzxz,xzyx,xzyy,xzyz,xzzx,xzzy,xzzz, \
	        yxxx,yxxy,yxxz,yxyx,yxyy,yxyz,yxzx,yxzy,yxzz, \
	        yyxx,yyxy,yyxz,yyyx,yyyy,yyyz,yyzx,yyzy,yyzz, \
	        yzxx,yzxy,yzxz,yzyx,yzyy,yzyz,yzzx,yzzy,yzzz, \
	        zxxx,zxxy,zxxz,zxyx,zxyy,zxyz,zxzx,zxzy,zxzz, \
	        zyxx,zyxy,zyxz,zyyx,zyyy,zyyz,zyzx,zyzy,zyzz, \
	        zzxx,zzxy,zzxz,zzyx,zzyy,zzyz,zzzx,zzzy,zzzz; \
};
#define _VEC_TYPE_4(type) \
struct type##4 {  \
	type x,y,z,w; \
	type##2 xx,xy,xz,xw,yx,yy,yz,yw,zx,zy,zz,zw,wx,wy,wz,ww; \
	type##3 xxx,xxy,xxz,xxw,xyx,xyy,xyz,xyw,xzx,xzy,xzz,xzw,xwx,xwy,xwz,xww, \
	        yxx,yxy,yxz,yxw,yyx,yyy,yyz,yyw,yzx,yzy,yzz,yzw,ywx,ywy,ywz,yww, \
	        zxx,zxy,zxz,zxw,zyx,zyy,zyz,zyw,zzx,zzy,zzz,zzw,zwx,zwy,zwz,zww, \
	        wxx,wxy,wxz,wxw,wyx,wyy,wyz,wyw,wzx,wzy,wzz,wzw,wwx,wwy,wwz,www; \
	type##4 xxxx,xxxy,xxxz,xxxw,xxyx,xxyy,xxyz,xxyw,xxzx,xxzy,xxzz,xxzw,xxwx,xxwy,xxwz,xxww, \
	        xyxx,xyxy,xyxz,xyxw,xyyx,xyyy,xyyz,xyyw,xyzx,xyzy,xyzz,xyzw,xywx,xywy,xywz,xyww, \
	        xzxx,xzxy,xzxz,xzxw,xzyx,xzyy,xzyz,xzyw,xzzx,xzzy,xzzz,xzzw,xzwx,xzwy,xzwz,xzww, \
	        xwxx,xwxy,xwxz,xwxw,xwyx,xwyy,xwyz,xwyw,xwzx,xwzy,xwzz,xwzw,xwwx,xwwy,xwwz,xwww, \
	        yxxx,yxxy,yxxz,yxxw,yxyx,yxyy,yxyz,yxyw,yxzx,yxzy,yxzz,yxzw,yxwx,yxwy,yxwz,yxww, \
	        yyxx,yyxy,yyxz,yyxw,yyyx,yyyy,yyyz,yyyw,yyzx,yyzy,yyzz,yyzw,yywx,yywy,yywz,yyww, \
	        yzxx,yzxy,yzxz,yzxw,yzyx,yzyy,yzyz,yzyw,yzzx,yzzy,yzzz,yzzw,yzwx,yzwy,yzwz,yzww, \
	        ywxx,ywxy,ywxz,ywxw,ywyx,ywyy,ywyz,ywyw,ywzx,ywzy,ywzz,ywzw,ywwx,ywwy,ywwz,ywww, \
	        zxxx,zxxy,zxxz,zxxw,zxyx,zxyy,zxyz,zxyw,zxzx,zxzy,zxzz,zxzw,zxwx,zxwy,zxwz,zxww, \
	        zyxx,zyxy,zyxz,zyxw,zyyx,zyyy,zyyz,zyyw,zyzx,zyzy,zyzz,zyzw,zywx,zywy,zywz,zyww, \
	        zzxx,zzxy,zzxz,zzxw,zzyx,zzyy,zzyz,zzyw,zzzx,zzzy,zzzz,zzzw,zzwx,zzwy,zzwz,zzww, \
	        zwxx,zwxy,zwxz,zwxw,zwyx,zwyy,zwyz,zwyw,zwzx,zwzy,zwzz,zwzw,zwwx,zwwy,zwwz,zwww, \
	        wxxx,wxxy,wxxz,wxxw,wxyx,wxyy,wxyz,wxyw,wxzx,wxzy,wxzz,wxzw,wxwx,wxwy,wxwz,wxww, \
	        wyxx,wyxy,wyxz,wyxw,wyyx,wyyy,wyyz,wyyw,wyzx,wyzy,wyzz,wyzw,wywx,wywy,wywz,wyww, \
	        wzxx,wzxy,wzxz,wzxw,wzyx,wzyy,wzyz,wzyw,wzzx,wzzy,wzzz,wzzw,wzwx,wzwy,wzwz,wzww, \
	        wwxx,wwxy,wwxz,wwxw,wwyx,wwyy,wwyz,wwyw,wwzx,wwzy,wwzz,wwzw,wwwx,wwwy,wwwz,wwww; \
};
#define _VEC_TYPE_8(type) \
struct type##8 {};
#define _VEC_TYPE_16(type) \
struct type##16 {};
/* TODO: .lo .hi .odd .even .sXXXX */

#define _VEC_TYPES(type) \
_VEC_TYPE_2(type) \
_VEC_TYPE_3(type) \
_VEC_TYPE_4(type) \
_VEC_TYPE_8(type) \
_VEC_TYPE_16(type)

_VEC_TYPES(long)
_VEC_TYPES(ulong)
_VEC_TYPES(int)
_VEC_TYPES(uint)
_VEC_TYPES(short)
_VEC_TYPES(ushort)
_VEC_TYPES(char)
_VEC_TYPES(uchar)
_VEC_TYPES(float)
_VEC_TYPES(double)

/* other built-in data types */
typedef struct {} image2d_t;
typedef struct {} image3d_t;
typedef struct {} sampler_t;
typedef struct {} event_t;

/* explicit type conversion */
typedef struct {} type_t;
#define _CNV(type) \
	type_t convert_##type(type_t arg); \
	type as_##type(type_t arg);
	
#define _CNV_VEC(type) \
	_CNV(type) \
	_CNV(type##2) \
	_CNV(type##3) \
	_CNV(type##4) \
	_CNV(type##8) \
	_CNV(type##16)

_CNV_VEC(long)
_CNV_VEC(ulong)
_CNV_VEC(int)
_CNV_VEC(uint)
_CNV_VEC(short)
_CNV_VEC(ushort)
_CNV_VEC(char)
_CNV_VEC(uchar)
_CNV_VEC(float)
_CNV_VEC(double)

/* built-in functions */

typedef struct {} gentype;
typedef struct {} ugentype;
typedef gentype gentypen;
typedef float floatn;
typedef int intn;
typedef uint uintn;
typedef long longn;
typedef ulong ulongn;

/* work-item functions */
uint get_work_dim();
size_t get_global_size(uint dimindx);
size_t get_global_id(uint dimindx);
size_t get_local_size(uint dimindx);
size_t get_local_id(uint dimindx);
size_t get_num_groups(uint dimindx);
size_t get_group_id(uint dimindx);
size_t get_global_offset(uint dimindx);

/* math functions */
gentype acos(gentype a);
gentype acosh(gentype a);
gentype acospi(gentype x);
gentype asin(gentype a);
gentype asinh(gentype a);
gentype asinpi(gentype x);
gentype atan(gentype y_over_x);
gentype atan2(gentype y, gentype x);
gentype atanh(gentype a);
gentype atanpi(gentype x);
gentype atan2pi(gentype y, gentype x);
gentype cbrt(gentype a);
gentype ceil(gentype a);
gentype copysign(gentype x, gentype y);
gentype cos(gentype a);
gentype cosh(gentype a);
gentype cospi(gentype x);
gentype erfc(gentype a);
gentype erf(gentype a);
gentype exp(gentype x);
gentype exp2(gentype a);
gentype exp10(gentype a);
gentype expm1(gentype x);
gentype fabs(gentype a);
gentype fdim(gentype x, gentype y);
gentype floor(gentype a);
gentype fma(gentype a, gentype b, gentype c);
gentype fmax(gentype x, gentype y);
gentype fmin(gentype x, gentype y);
gentype fmod(gentype x, gentype y);
gentype fract(gentype x, gentype *iptr);
floatn frexp(floatn x, intn *exp);
gentype hypot(gentype x, gentype y);
intn ilogb(floatn x);
floatn ldexp(floatn x, intn k);
gentype lgamma(gentype x);
gentype log(gentype a);
gentype log2(gentype a);
gentype log10(gentype a);
gentype log1p(gentype x);
gentype logb(gentype x);
gentype mad(gentype a, gentype b, gentype c);
gentype maxmag(gentype x, gentype y);
gentype minmag(gentype x, gentype y);
gentype modf(gentype x, gentype *iptr);
floatn nan(uintn nancode);
gentype nextafter(gentype x, gentype y);
gentype pow(gentype x, gentype y);
floatn pown(floatn x, intn y);
gentype powr(gentype x, gentype y);
gentype remainder(gentype x, gentype y);
floatn remquo(floatn x, floatn y, intn *quo);
gentype rint(gentype a);
floatn rootn(floatn x, intn y);
float rootn(float x, int y);
gentype round(gentype x);
gentype rsqrt(gentype a);
gentype sin(gentype a);
gentype sincos(gentype x, gentype *cosval);
gentype sinh(gentype a);
gentype sinpi(gentype x);
gentype sqrt(gentype a);
gentype tan(gentype a);
gentype tanh(gentype a);
gentype tanpi(gentype x);
gentype tgamma(gentype a);
gentype trunc(gentype a);

/* common functions */
gentype clamp(gentype x, gentype minval, gentype maxval);
gentype degrees(gentype radians);
gentype max(gentype x, gentype y);
gentype min(gentype x, gentype y);
gentype mix(gentype x, gentype y, gentype a);
gentype radians(gentype degrees);
gentype step(gentype edge, gentype x);
gentype smoothstep(gentype edge0, gentype edge1, gentype x);
gentype sign(gentype x);

/* geometric functions */
float4 cross(float4 p0, float4 p1);
float3 cross(float3 p0, float3 p1);
float dot(floatn p0, floatn p1);
float distance(floatn p0, floatn p1);
float length(floatn p);
floatn normalize(floatn p);
float fast_distance(floatn p0, floatn p1);
float fast_length(floatn p);
floatn fast_normalize(floatn p);

/* constants */
#define MAXFLOAT 1.0f
#define HUGE_VALF 1.0f
#define INFINITY 1.0f
#define NAN 1.0f

#define FLT_DIG        6
#define FLT_MANT_DIG   24
#define FLT_MAX_10_EXP +38
#define FLT_MAX_EXP    +128
#define FLT_MIN_10_EXP -37
#define FLT_MIN_EXP    -125
#define FLT_RADIX      2
#define FLT_MAX        0x1.fffffep127f
#define FLT_MIN        0x1.0p-126f
#define FLT_EPSILON    0x1.0p-23f

#define M_E_F        2.7f
#define M_LOG2E_F    1.5f
#define M_LOG10E_F   0.25f
#define M_LN2_F      0.69f
#define M_LN10_F     2.3f
#define M_PI_F       3.1415f
#define M_PI_2_F     1.57f
#define M_PI_4_F     0.78f
#define M_1_PI_F     0.32f
#define M_2_PI_F     0.63f
#define M_2_SQRTPI_F 1.12f
#define M_SQRT2_F    1.41f
#define M_SQRT1_2_F  0.7f

/* integer functions */
uintn abs(intn x);
uintn abs_diff(intn x, intn y);
intn add_sat(intn x, intn y);
intn hadd(intn x, intn y);
intn rhadd(intn x, intn y);
intn clamp(intn x, intn minval, intn maxval);
intn clz(intn x);
intn mad_hi(intn a, intn b, intn c);
intn mad_sat(intn a, intn b, intn c);
intn max(intn x, intn y);
intn max(intn x, sintn y);
intn min(intn x, intn y);
intn min(intn x, sintn y);
intn mul_hi(intn x, intn y);
intn rotate(intn v, intn i);
intn sub_sat(intn x, intn y);
longn upsample(intn hi, uintn lo);
intn mad24(intn x, intn y, intn z);
intn mul24(intn x, intn y);

#define CHAR_BIT  8
#define INT_MAX   2147483647
#define INT_MIN   (-2147483647 - 1)
#define LONG_MAX  0x7fffffffffffffffL
#define LONG_MIN  (-0x7fffffffffffffffL - 1)
#define SCHAR_MAX 127
#define SCHAR_MIN (-127 - 1)
#define CHAR_MAX  SCHAR_MAX
#define CHAR_MIN  SCHAR_MIN
#define SHRT_MAX  32767
#define SHRT_MIN  (-32767 - 1)
#define UCHAR_MAX 255
#define USHRT_MAX 65535
#define UINT_MAX  0xffffffff
#define ULONG_MAX 0xffffffffffffffffUL

/* relational functions */
intn isequal(floatn x, floatn y);
intn isnotequal(floatn x, floatn y);
intn isgreater(floatn x, floatn y);
intn isgreaterequal(floatn x, floatn y);
intn isless(floatn x, floatn y);
intn islessequal(floatn x, floatn y);
intn islessgreater(floatn x, floatn y);
intn isfinite(floatn);
intn isinf(floatn);
intn isnan(floatn);
intn isnormal(floatn);
intn isordered(floatn x, floatn y);
intn isunordered(floatn x, floatn y);
intn signbit(floatn);
int any(intn x);
int all(intn x);
gentype bitselect(gentype a, gentype b, gentype c);
gentype select(gentype a, gentype b, ugentype c);

/* vector data load and store functions */
#define _VLOAD(n) \
gentype##n vload##n(size_t offset, const gentype *p);
#define _STORE(n) \
void vstore##n(gentype##n data, size_t offset, gentype *p);

_VLOAD(2)
_VLOAD(3)
_VLOAD(4)
_VLOAD(8)
_VLOAD(16)
_STORE(2)
_STORE(3)
_STORE(4)
_STORE(8)
_STORE(16)


/* synchronization functions */
typedef int cl_mem_fence_flags;
#define CLK_LOCAL_MEM_FENCE  1
#define CLK_GLOBAL_MEM_FENCE 2

void barrier(cl_mem_fence_flags flags);
void mem_fence(cl_mem_fence_flags flags);
void read_mem_fence(cl_mem_fence_flags flags);
void write_mem_fence(cl_mem_fence_flags flags);

/* special qualifiers */
#define __OVERLOADABLE__

/* TODO: add next functions */

#endif // OPENCL_H
