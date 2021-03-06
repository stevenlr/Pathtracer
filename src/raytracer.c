#include <stdlib.h>
#include <string.h>
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

f32 randf(f32 a, f32 b)
{
    return ((f32)rand() / RAND_MAX) * (b - a) + a;
}

#include "vec3.c" 

typedef struct Frame
{
    Vec3 u;
    Vec3 v;
    Vec3 n;
} Frame;

Frame make_frame_from_normal(Vec3 n)
{
    Frame frame = { .n = vec3_normalize(n) };

    if (fabsf(frame.n.x) > fabsf(frame.n.y)) {
        frame.u = vec3_normalize((Vec3){ -frame.n.z, 0.0f, frame.n.x });
    }
    else {
        frame.u = vec3_normalize((Vec3){ 0.0f, frame.n.z, -frame.n.y });
    }

    frame.v = vec3_cross(frame.n, frame.u);
    return frame;
}

Vec3 make_ray_hemisphere(Vec3 n, f32 * probability)
{
    Frame frame = make_frame_from_normal(n);

    f32 h_uniform = randf(0.0f, 1.0f);
    f32 v_uniform = randf(0.0f, 1.0f);

    f32 hangle = 2.0f * (f32)M_PI * h_uniform;

    f32 sin_vangle = sqrtf(v_uniform);
    f32 cos_vangle = sqrtf(1.0f - v_uniform);
    f32 vangle = asinf(sin_vangle);

    *probability = cos_vangle / ((f32)M_PI);

    Vec3 u = vec3_mult_k(frame.u, cosf(hangle) * sin_vangle);
    Vec3 v = vec3_mult_k(frame.v, sinf(hangle) * sin_vangle);
    n = vec3_mult_k(frame.n, cos_vangle);
    
    return vec3_add(vec3_add(u, v), n);
}

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
    i32 width;
    i32 height;
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
    f32 jitter_x = randf(-0.5f, 0.5f);
    f32 jitter_y = randf(-0.5f, 0.5f);

    f32 xx = ((f32)x + jitter_x) * 2.0f / cam->width - 1.0f;
    f32 yy = (1.0f - ((f32)y + jitter_y) * 2.0f / cam->height) / cam->ratio;
    Vec3 dir = { xx, 1.5f, yy };
    dir = vec3_normalize(dir);
    return (Ray){ cam->center, dir };
}

typedef enum ObjectType {
    OBJECT_SPHERE,
    OBJECT_CUBE,
    OBJECT_PLANE
} ObjectType;

typedef struct Object {
    ObjectType type;
    Vec3 center;
    Vec3 color;
    f32 emissive;
    union
    {
        struct
        {
            f32 radius;
        } sphere, cube;
        struct
        {
            Vec3 normal;
        } plane;
    };
} Object;

Object make_sphere(f32 radius, Vec3 center, Vec3 color, f32 emissive)
{
    return (Object){
        .type = OBJECT_SPHERE,
        .center = center,
        .color = color,
        .emissive = emissive,
        .sphere.radius = radius
    };
}

Object make_cube(f32 radius, Vec3 center, Vec3 color, f32 emissive)
{
    return (Object){
        .type = OBJECT_CUBE,
        .center = center,
        .color = color,
        .emissive = emissive,
        .cube.radius = radius
    };
}

Object make_plane(Vec3 normal, Vec3 center, Vec3 color, f32 emissive)
{
    return (Object){
        .type = OBJECT_PLANE,
        .center = center,
        .color = color,
        .emissive = emissive,
        .plane.normal = vec3_normalize(normal)
    };
}

#define HIT_EPSILON 0.0001

bool plane_intersect(const Object * obj, const Ray * ray, Intersection * intersection)
{
    f32 n_dot_dir = vec3_dot(obj->plane.normal, ray->dir);
    if (fabs(n_dot_dir) > 15) return false;

    f32 d = -vec3_dot(obj->plane.normal, obj->center);
    f32 n_dot_o = vec3_dot(obj->plane.normal, ray->o);
    f32 t = -(d + n_dot_o) / n_dot_dir;

    if (t < HIT_EPSILON) return false;

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
    if (t < HIT_EPSILON) return false;

    intersection->dist = t;
    intersection->normal = vec3_normalize(vec3_sub(ray_make_point(ray, t), obj->center));
    return true;
}

bool cube_intersect(const Object * obj, const Ray * ray, Intersection * intersection)
{
    const Vec3 normals[6] = {
        {  1.0f,  0.0f,  0.0f },
        { -1.0f,  0.0f,  0.0f },
        {  0.0f,  1.0f,  0.0f },
        {  0.0f, -1.0f,  0.0f },
        {  0.0f,  0.0f,  1.0f },
        {  0.0f,  0.0f, -1.0f },
    };
    const i32 axis[6] = { 0, 0, 1, 1, 2, 2 };

    f32 r = obj->cube.radius;
    const Vec3 c = obj->center;

    f32 t_min = -1.0f;
    Vec3 int_normal;
    Vec3 o_to_c = vec3_sub(c, ray->o);

    for (i32 i = 0; i < 6; ++i) {
        const Vec3 n = normals[i];
        f32 n_dot_d = vec3_dot(n, ray->dir);
        if (n_dot_d >= 0.0f) continue;

        f32 t = (vec3_dot(n, o_to_c) + r) / n_dot_d;
        if (t > t_min && t_min >= 0.0f) continue;

        Vec3 diff = vec3_sub(ray_make_point(ray, t), c);
        if (vec3_max(diff) <= r + HIT_EPSILON) {
            t_min = t;
            int_normal = n;
        }
    }

    if (t_min < 0.0f) {
        return false;
    }

    intersection->dist = t_min;
    intersection->normal = int_normal;
    return true;
}

bool object_intersect(const Object * obj, const Ray * ray, Intersection * intersection)
{
    switch (obj->type) {
    case OBJECT_SPHERE:
        return sphere_intersect(obj, ray, intersection);
    case OBJECT_CUBE:
        return cube_intersect(obj, ray, intersection);
    case OBJECT_PLANE:
        return plane_intersect(obj, ray, intersection);
    }
    return false;
}

typedef struct World
{
    Vec3 sun_dir;
    Vec3 sun_color;
    Vec3 sky_color;
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

static const i32 nb_threads = 10;

typedef struct TracerContext
{
    const Camera        *   camera;
    Vec3                *   image_data;
    const World         *   world;
    SDL_atomic_t            current_line;
    i32                     num_iter;

    SDL_mutex           *   status_mtx;
    SDL_cond            *   status_cv;

    SDL_mutex           *   finished_mtx;
    SDL_cond            *   finished_cv;
    i32                     finished;
} TracerContext;

void trace_line(const TracerContext * ctx, i32 y)
{
    Vec3 * ptr_v = ctx->image_data + y * ctx->camera->width;
    for (i32 x = 0; x < ctx->camera->width; ++x) {
        Ray ray = camera_gen_ray(ctx->camera, x, y);
        Vec3 mask = { 1.0f, 1.0f, 1.0f };
        Vec3 accum = { 0.0f, 0.0f, 0.0f };

        i32 nb_bounces = 0;
        while (true) {
            Intersection intersection = { 0 };
            i32 int_object = world_intersect(ctx->world, &ray, &intersection);

            if (int_object == -1) {
                accum = vec3_add(accum, vec3_mult(mask, ctx->world->sky_color));
                break;
            }
            else {
                Vec3 int_pos = ray_make_point(&ray, intersection.dist);
                mask = vec3_mult(mask, ctx->world->objects[int_object].color);

                // Sun ray
                {
                    Intersection int_sun;
                    Ray sun_ray = { int_pos, vec3_mult_k(ctx->world->sun_dir, -1.0f) };
                    i32 int_sun_obj = world_intersect(ctx->world, &sun_ray, &int_sun);

                    if (int_sun_obj == -1) {
                        f32 intensity = -vec3_dot(intersection.normal, ctx->world->sun_dir);
                        intensity = MAX(intensity, 0.0f);
                        accum = vec3_add(accum, vec3_mult_k(vec3_mult(ctx->world->sun_color, mask), intensity));
                    }
                }
                
                f32 emissive = ctx->world->objects[int_object].emissive;
                if (emissive > 0.0f) {
                    accum = vec3_add(accum, vec3_mult((Vec3){ emissive, emissive, emissive }, mask));
                    break;
                }
                else {
                    f32 probability = 1.0f;
                    ray.o = int_pos;
                    ray.dir = make_ray_hemisphere(intersection.normal, &probability);
                    probability = MAX(probability, 0.0001f);

                    f32 intensity = vec3_dot(intersection.normal, ray.dir) / ((f32)M_PI);
                    intensity = MAX(intensity, 0.0f);

                    intensity /= probability;
                    mask = vec3_mult_k(mask, intensity);
                }
            }

            f32 rr = 0.95f;
            if (randf(0.0f, 1.0f) > rr) {
                break;
            }

            if (nb_bounces > 0) {
                mask = vec3_mult_k(mask, 1.0f / rr);
            }

            nb_bounces++;
        }

        f32 contrib = 1.0f / (f32)(ctx->num_iter + 1);
        *ptr_v = vec3_add(vec3_mult_k(*ptr_v, 1 - contrib), vec3_mult_k(accum, contrib));
        ptr_v++;
    }
}

i32 fetch_current_line(TracerContext * ctx)
{
    while (true) {
        i32 value = SDL_AtomicGet(&ctx->current_line);
        if (value >= ctx->camera->height) return -1;

        if (SDL_AtomicCAS(&ctx->current_line, value, value + 1)) {
            return value;
        }
    }
}

i32 trace_thread(void * ctx_raw)
{
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);
    TracerContext * ctx = (TracerContext *)ctx_raw;

    while (true) {
        i32 line = fetch_current_line(ctx);

        if (line < 0) {
            SDL_LockMutex(ctx->status_mtx);
            while (true) {
                line = fetch_current_line(ctx);
                if (line >= 0) {
                    break;
                }
                SDL_CondWait(ctx->status_cv, ctx->status_mtx);
            }
            SDL_UnlockMutex(ctx->status_mtx);
        }

        trace_line(ctx, line);
        SDL_CompilerBarrier();

        SDL_LockMutex(ctx->finished_mtx);
        ctx->finished++;
        SDL_CondSignal(ctx->finished_cv);
        SDL_UnlockMutex(ctx->finished_mtx);
    }

    return 0;
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
    Vec3 * image_data_linear = malloc(width * height * sizeof(Vec3));

    memset(image_data_linear, width * height * sizeof(Vec3), 0);

    SDL_Surface * image_surface = SDL_CreateRGBSurfaceFrom(
            image_data, width, height, 24, width * 3,
            0xff0000, 0x00ff00, 0x0000ff, 0x000000);

    i32 objects_count = 3;
    Object objects[] = { 
        make_plane((Vec3){ 0.0f, 0.0f, 1.0f }, (Vec3){ 0.0f, 0.0f, 0.0f }, (Vec3){ 0.6f, 0.6f, 0.6f }, 0.0f),
        make_cube(0.5f, (Vec3){ -1.5f, 5.0f, 0.5f }, (Vec3){ 0.9f, 0.02f, 0.02f }, 0.0f),
        make_sphere(1.0f, (Vec3){ 1.5f, 5.0f, 1.0f }, (Vec3){ 0.6f, 0.6f, 0.6f }, 0.0f),
        make_sphere(0.3f, (Vec3){ 0.0f, 5.0f, 2.5f }, (Vec3){ 1.0f, 1.0f, 1.0f }, 20.0f)
    };

    const f32 sun_mult = 1.0f;
    const f32 sky_mult = 1.0f;
    World world = {
        .sun_dir = vec3_normalize((Vec3){ -0.5f, 0.1f, -0.6f }),
        .sun_color = vec3_mult_k((Vec3){ 1.0f, 0.95f, 0.9f }, sun_mult),
        .sky_color = vec3_mult_k((Vec3){ 0.1f, 0.15f, 0.2f }, sky_mult),
        .objects_count = objects_count,
        .objects = objects
    };

    SDL_Rect to_blit = { .x = 0, .y = 0, .w = width, .h = height };

    TracerContext ctx = {
        .camera = &camera,
        .image_data = image_data_linear,
        .world = &world,
        .num_iter = 0,
        .status_mtx = SDL_CreateMutex(),
        .status_cv = SDL_CreateCond(),
        .finished_mtx = SDL_CreateMutex(),
        .finished_cv = SDL_CreateCond(),
        .finished = 0
    };
    SDL_AtomicSet(&ctx.current_line, 0);

    for (i32 i = 0; i < nb_threads; ++i) {
        SDL_Thread * thread = SDL_CreateThread(trace_thread, "tracer", &ctx);
    }

    bool run = true;
    i32 num_iter = 0;
    while (run) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) run = false;
        }

        SDL_LockMutex(ctx.finished_mtx);
        ctx.finished = 0;
        SDL_UnlockMutex(ctx.finished_mtx);

        SDL_AtomicSet(&ctx.current_line, 0);
        SDL_CondBroadcast(ctx.status_cv);

        SDL_LockMutex(ctx.finished_mtx);
        while (ctx.finished < camera.height) {
            SDL_CondWait(ctx.finished_cv, ctx.finished_mtx);
        }
        SDL_UnlockMutex(ctx.finished_mtx);

        ctx.num_iter++;

        if (ctx.num_iter % 50 == 0) {
            printf("%d\n", ctx.num_iter);

            u8 * ptr = image_data;
            Vec3 * ptr_v = image_data_linear;
            for (u32 y = 0; y < height; ++y) {
                for (u32 x = 0; x < width; ++x) {
                    Vec3 color = vec3_mult_k(vec3_pow(*ptr_v++, 0.45f), 255.99f);
                    *ptr++ = (u8)(MIN(MAX(color.z, 0.0f), 255.99f));
                    *ptr++ = (u8)(MIN(MAX(color.y, 0.0f), 255.99f));
                    *ptr++ = (u8)(MIN(MAX(color.x, 0.0f), 255.99f));
                }
            }

            SDL_BlitSurface(image_surface, &to_blit, screen, &to_blit);
            SDL_UpdateWindowSurface(window);
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

