typedef struct Vec3
{
    f32 x;
    f32 y;
    f32 z;
} Vec3;

inline f32 vec3_max(Vec3 v)
{
    float x = fabsf(v.x);
    float y = fabsf(v.y);
    float z = fabsf(v.z);
    f32 m = MAX(x, y);
    return MAX(m, z);
}

inline f32 vec3_e(Vec3 v, i32 n)
{
    return ((f32 *)&v)[n];
}

inline Vec3 vec3_neg(Vec3 v)
{
    return (Vec3){ -v.x, -v.y, -v.z };
}

inline f32 vec3_dot(Vec3 a, Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 vec3_cross(Vec3 a, Vec3 b)
{
    return (Vec3){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

inline Vec3 vec3_add(Vec3 a, Vec3 b)
{
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    return a;
}

inline Vec3 vec3_mult(Vec3 a, Vec3 b)
{
    a.x *= b.x;
    a.y *= b.y;
    a.z *= b.z;
    return a;
}

inline Vec3 vec3_sub(Vec3 a, Vec3 b)
{
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    return a;
}

inline f32 vec3_len2(Vec3 v)
{
    return vec3_dot(v, v);
}

inline f32 vec3_len(Vec3 v)
{
    return sqrtf(vec3_len2(v));
}

inline Vec3 vec3_mult_k(Vec3 v, f32 a)
{
    v.x *= a;
    v.y *= a;
    v.z *= a;
    return v;
}

inline Vec3 vec3_normalize(Vec3 v)
{
    f32 inv_len = 1.0f / vec3_len(v);
    return vec3_mult_k(v, inv_len);
}

inline Vec3 vec3_pow(Vec3 v, float p)
{
    v.x = powf(v.x, p);
    v.y = powf(v.y, p);
    v.z = powf(v.z, p);
    return v;
}
