struct Vec3
{
    f32 x;
    f32 y;
    f32 z;
};
typedef struct Vec3 Vec3;

inline Vec3 vec3_neg(Vec3 v)
{
    Vec3 n = { -v.x, -v.y, -v.z };
    return n;
}

inline f32 vec3_dot(Vec3 a, Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 vec3_add(Vec3 a, Vec3 b)
{
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
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

