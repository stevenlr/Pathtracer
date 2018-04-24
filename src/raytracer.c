#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <SDL.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

typedef char i8;
typedef short i16;
typedef long i32;
typedef long long i64;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef unsigned long long u64;
typedef float f32;
typedef double f64;

#include "vec3.c" 

typedef struct Ray
{
    Vec3 o;
    Vec3 dir;
} Ray;

inline Vec3 ray_make_point(const Ray * ray, f32 t)
{
    return vec3_add(ray->o, vec3_mult_k(ray->dir, t));
}

typedef struct Intersection
{
    f32 dist;
    Vec3 normal;
} Intersection;

typedef struct Camera
{
    Vec3 center;
    u32 width;
    u32 height;
    f32 ratio;
} Camera;

Camera make_camera(u32 width, u32 height)
{
    return (Camera) {
        { 0, 0, 1.0f },
        width, height,
        (f32)width / height
    };
}

Ray camera_gen_ray(const Camera * cam, u32 x, u32 y)
{
    f32 xx = (f32)x * 2.0f / cam->width - 1.0f;
    f32 yy = (1.0f - (f32)y * 2.0f / cam->height) / cam->ratio;
    Vec3 dir = { xx, 1.5f, yy };
    dir = vec3_normalize(dir);
    return (Ray){ cam->center, dir };
}

typedef enum ObjectType {
    OBJECT_SPHERE,
    OBJECT_PLANE
} ObjectType;

typedef struct Object {
    ObjectType type;
    Vec3 center;
    Vec3 color;
    union
    {
        struct
        {
            f32 radius;
        } sphere;
        struct
        {
            Vec3 normal;
        } plane;
    };
} Object;

Object make_sphere(f32 radius, Vec3 center, Vec3 color)
{
    return (Object){
        .type = OBJECT_SPHERE,
        .center = center,
        .color = color,
        .sphere.radius = radius
    };
}

Object make_plane(Vec3 normal, Vec3 center, Vec3 color)
{
    return (Object){
        .type = OBJECT_PLANE,
        .center = center,
        .color = color,
        .plane.normal = vec3_normalize(normal)
    };
}

bool plane_intersect(const Object * obj, const Ray * ray, Intersection * intersection)
{
    f32 n_dot_dir = vec3_dot(obj->plane.normal, ray->dir);
    if (fabs(n_dot_dir) > 15) return false;

    f32 d = -vec3_dot(obj->plane.normal, obj->center);
    f32 n_dot_o = vec3_dot(obj->plane.normal, ray->o);
    f32 t = -(d + n_dot_o) / n_dot_dir;

    if (t < 0) return false;

    intersection->dist = t;
    intersection->normal = obj->plane.normal;
    return true;
}

bool sphere_intersect(const Object * obj, const Ray * ray, Intersection * intersection)
{
    Vec3 diff = vec3_sub(ray->o, obj->center);
    f32 a = vec3_dot(ray->dir, ray->dir);
    f32 b = 2 * vec3_dot(ray->dir, diff);
    f32 c = vec3_dot(diff, diff) - obj->sphere.radius * obj->sphere.radius;
    f32 d = b * b - 4 * a * c;

    if (d < 0) return false;

    d = sqrtf(d);
    f32 t0 = (-b - d) / (2 * a);
    f32 t1 = (-b + d) / (2 * a);

    f32 t = MIN(t0, t1);
    if (t <= 0) t = MAX(t0, t1);
    if (t <= 0) return false;

    intersection->dist = t;
    intersection->normal = vec3_normalize(vec3_sub(ray_make_point(ray, t), obj->center));
    return true;
}

bool object_intersect(const Object * obj, const Ray * ray, Intersection * intersection)
{
    switch (obj->type) {
    case OBJECT_SPHERE:
        return sphere_intersect(obj, ray, intersection);
    case OBJECT_PLANE:
        return plane_intersect(obj, ray, intersection);
    }
    return false;
}

Vec3 object_shade(const Object * obj, const Intersection * intersection, Vec3 direction)
{
    Vec3 color = obj->color;
    f32 k = vec3_dot(intersection->normal, direction);
    k = MAX(k, 0.0f);
    return vec3_mult_k(color, k);
}

typedef struct World
{
    i32 objects_count;
    Object * objects;
} World;

i32 world_intersect(const World * world, const Ray * ray, Intersection * intersection)
{
    *intersection = (Intersection){ 0 };
    i32 min_object = -1;
    for (i32 i = 0; i < world->objects_count; ++i) {
        Intersection this_int = { 0, 0 };
        if (object_intersect(&world->objects[i], ray, &this_int)) {
            if (this_int.dist < intersection->dist || min_object < 0) {
                min_object = i;
                *intersection = this_int;
            }
        }
    }
    return min_object;
}

int main(int argc, char * argv[])
{
    const u32 width = 600;
    const u32 height = 450;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window * window = SDL_CreateWindow("Raytracer",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            width, height, 0);

    SDL_ShowWindow(window);
    SDL_Surface * screen = SDL_GetWindowSurface(window);

    Camera camera = make_camera(width, height);
    u8 * image_data = malloc(width * height * 3);

    SDL_Surface * image_surface = SDL_CreateRGBSurfaceFrom(
            image_data, width, height, 24, width * 3,
            0xff0000, 0x00ff00, 0x0000ff, 0x000000);

    i32 objects_count = 3;
    Object objects[] = { 
        make_sphere(1.0f, (Vec3){ -1.5f, 5.0f, 1.0f }, (Vec3){ 1.0f, 0.0f, 0.0f }),
        make_sphere(1.0f, (Vec3){ 1.5f, 5.0f, 1.0f }, (Vec3){ 1.0f, 0.5f, 0.0f }),
        make_plane((Vec3){ 0.0f, 0.0f, 1.0f }, (Vec3){ 0.0f, 0.0f, 0.0f }, (Vec3){ 0.7f, 0.7f, 0.7f })
    };
    
    Vec3 sun_direction = vec3_normalize((Vec3){ -0.7f, -0.3f, 1.0f });

    World world = {
        .objects_count = objects_count,
        .objects = objects
    };

    u8 * ptr = image_data;
    for (u32 y = 0; y < height; ++y) {
        for (u32 x = 0; x < width; ++x) {
            Ray ray = camera_gen_ray(&camera, x, y);
            Intersection intersection = { 0 };

            i32 int_object = world_intersect(&world, &ray, &intersection);

            Vec3 color = { 0 };
            if (int_object >= 0) {
                color = object_shade(&objects[int_object], &intersection, sun_direction);
            }

            color = vec3_mult_k(vec3_pow(color, 0.45f), 255.99f);
            *ptr++ = (u8)color.z;
            *ptr++ = (u8)color.y;
            *ptr++ = (u8)color.x;
        }
    }

    SDL_Rect to_blit = { .x = 0, .y = 0, .w = width, .h = height };

    bool run = true;
    while (run) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) run = false;
        }

        SDL_BlitSurface(image_surface, &to_blit, screen, &to_blit);
        SDL_UpdateWindowSurface(window);
        SDL_Delay(33);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

