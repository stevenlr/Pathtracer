#include <stdlib.h>
#include <stdio.h>
#include <math.h>

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

enum
{
    TGA_COLOR_MAP_TYPE_NONE = 0,
    TGA_IMAGE_TYPE_UNCOMPRESSED_TRUE_COLOR = 2,
    TGA_IMAGE_ORIGIN_TOP_LEFT = 0x20,
};

#pragma pack(push, 1)
struct TgaHeader
{
    u8 id_length;
    u8 color_map_type;
    u8 image_type;
    struct
    {
        u16 first_entry_index;
        u16 length;
        u8 entry_size;
    } color_map;
    struct
    {
        u16 x_origin;
        u16 y_origin;
        u16 width;
        u16 height;
        u8 pixel_depth;
        u8 image_descriptor;
    } image_specs;
};
#pragma pack(pop)
typedef struct TgaHeader TgaHeader;

void write_tga_header(TgaHeader * header, u16 width, u16 height)
{
    header->id_length = 0;
    header->color_map_type = TGA_COLOR_MAP_TYPE_NONE;
    header->image_type = TGA_IMAGE_TYPE_UNCOMPRESSED_TRUE_COLOR;
    header->color_map.first_entry_index = 0;
    header->color_map.length = 0;
    header->color_map.entry_size = 0;
    header->image_specs.x_origin = 0;
    header->image_specs.y_origin = 0;
    header->image_specs.width = width;
    header->image_specs.height = height;
    header->image_specs.pixel_depth = 24;
    header->image_specs.image_descriptor = TGA_IMAGE_ORIGIN_TOP_LEFT;
}

struct Ray
{
    Vec3 o;
    Vec3 dir;
};
typedef struct Ray Ray;

struct Camera
{
    u32 width;
    u32 height;
    float ratio;
};
typedef struct Camera Camera;

Camera camera_new(u32 width, u32 height)
{
    Camera cam = {
        width, height,
        (float)width / height
    };
    return cam;
}

Ray camera_gen_ray(Camera * cam, u32 x, u32 y)
{
    float xx = (float)x * 2.0f / cam->width - 1.0f;
    float yy = (1.0f - (float)y * 2.0f / cam->height) / cam->ratio;
    Vec3 dir = { xx, 1.0f, yy };
    dir = vec3_normalize(dir);
    Ray ray = { { 0, 0, 0 }, dir };
    return ray;
}

int main(int argc, char * argv[])
{
    const u32 width = 640;
    const u32 height = 360;

    Camera camera = camera_new(width, height);

    TgaHeader tga_header;
    write_tga_header(&tga_header, width, height);
    u8 * image_data = malloc(width * height * 3);

    Vec3 bottom_color = { 1.0f, 1.0f, 1.0f };
    Vec3 top_color = { 0.5f, 0.75f, 1.0f };

    u8 * ptr = image_data;
    for (u32 y = 0; y < height; ++y) {
        for (u32 x = 0; x < width; ++x) {
            Ray ray = camera_gen_ray(&camera, x, y);
            Vec3 color = vec3_add(
                    vec3_mult_k(top_color, (ray.dir.z + 1.0f) / 2.0f),
                    vec3_mult_k(bottom_color, 1.0 - (ray.dir.z + 1.0f) / 2.0f));

            *ptr++ = color.z * 255.99f;
            *ptr++ = color.y * 255.99f;
            *ptr++ = color.x * 255.99f;
        }
    }

    FILE * fp = fopen("image.tga", "wb+");
    fwrite((char *)&tga_header, sizeof(TgaHeader), 1, fp);
    fwrite(image_data, 1, width * height * 3, fp);
    fclose(fp);
    return 0;
}

