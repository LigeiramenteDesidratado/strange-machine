#ifndef SM_CORE_RESOURCE_H
#define SM_CORE_RESOURCE_H

#include "animation/smPose.h"
#include "core/smArray.h"
#include "core/smRefCount.h"
#include "core/smString.h"
#include "math/smMath.h"
#include "math/smShape.h"

struct resource
{
#define RESOURCE_INVALID  0
#define RESOURCE_IMAGE	  BIT(0)
#define RESOURCE_MATERIAL BIT(1)
#define RESOURCE_MESH	  BIT(2)
#define RESOURCE_SCENE	  BIT(3)
#define RESOURCE_ARMATURE BIT(4)
#define RESOURCE_CLIP	  BIT(5)
#define RESOURCE_SHADER	  BIT(8)
	u32 type;

	str8 name;

	struct ref_counter refs;

	union
	{
		struct image_resource *image_data;
		struct material_resource *material_data;
		struct mesh_resource *mesh_data;
		struct scene_resource *scene_data;
		struct armature_resource *armature_data;
		struct clip_resource *clip_data;
		void *data;
	};
};

struct resource resource_make(str8 name, u32 type, void *ptr);
struct resource *resource_make_reference(struct resource *resource);
void resource_unmake_reference(struct resource *resource);
void resource_validate(struct resource *resource);
struct resource *resource_push(struct resource *resource);
struct resource *resource_get_by_name(str8 name);
void resource_print(struct resource *resource);
void resource_write(struct resource *resource);
void resource_for_each(b8 (*cb)(str8, struct resource *, void *), void *user_data);

struct resource resource_get_default_image(void);
struct resource resource_get_default_material(void);

typedef struct mesh_resource
{
	array(v3) positions;
	array(v2) uvs;
	array(v4) colors;
	array(v3) normals;
	array(u32) indices;

	struct
	{
		b8 is_skinned;
		array(v4) weights;
		array(iv4) influences;
		u32 vbos[2]; /* openGL vertex buffer objects */

		// add an additional copy of the pose and normal data, as well as a matrix
		// palette to use for CPU skinning.
		m4 *pose_palette;
	} skin_data;

	u32 vao;     /* openGL vertex array object */
	u32 vbos[4]; /* openGL vertex buffer objects */
	u32 ebo;     /* openGL vertex buffer object */

	aabb aabb;

#define MESH_FLAG_NONE	     0
#define MESH_FLAG_DIRTY	     BIT(0)
#define MESH_FLAG_RENDERABLE BIT(1)
#define MESH_FLAG_ON_CPU     BIT(2)
#define MESH_FLAG_ON_GPU     BIT(3)
#define MESH_FLAG_SKINNED    BIT(4)
#define MESH_FLAG_DRAW_AABB  BIT(5)
	u32 flags;

} mesh_resource;

typedef struct image_resource
{
	u32 width, height;
#define IMAGE_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE  0x00000001 // 8 bit per pixel (no alpha)
#define IMAGE_PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA 0x00000002 // 8*2 bpp (2 channels)
#define IMAGE_PIXELFORMAT_UNCOMPRESSED_ALPHA	  0x00000003 // 8 bpp
#define IMAGE_PIXELFORMAT_UNCOMPRESSED_R5G6B5	  0x00000004 // 16 bpp
#define IMAGE_PIXELFORMAT_UNCOMPRESSED_R8G8B8	  0x00000005 // 24 bpp
#define IMAGE_PIXELFORMAT_UNCOMPRESSED_R5G5B5A1	  0x00000006 // 16 bpp (1 bit alpha)
#define IMAGE_PIXELFORMAT_UNCOMPRESSED_R4G4B4A4	  0x00000007 // 16 bpp (4 bit alpha)
#define IMAGE_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8	  0x00000008 // 32 bpp
	u32 pixel_format;

	u8 *data;
	u32 texture_handle;

} image_resource;

u32 resource_image_size(u32 width, u32 height, u32 pixel_format);

typedef struct material_resource
{
	color color;
	b8 double_sided;

	str8 image;
	u32 shader_handle;

} material_resource;

typedef struct armature_resource
{
	struct pose rest;
	struct pose bind;

	array(m4) inverse_bind;
	array(str8) names;
} armature_resource;

struct armature_resource *resource_armature_get_by_name(str8 name);

typedef struct clip_resource
{
	// u32 id;
	array(struct transform_track) tracks;
	b8 looping;
	f32 start_time, end_time;
} clip_resource;

struct scene_node
{
	str8 name;

	i32 parent_index;
	array(i32) children;

	v3 position;
	v3 scale;
	v4 rotation;

	str8 mesh;
	str8 material;
	str8 armature;
};

typedef struct scene_resource
{
	array(struct scene_node) nodes;
} scene_resource;

typedef struct vert_shader_resource
{
	str8 name;
	str8 vertex;
	u32 id;

	struct ref_counter refs;
} vert_shader_resource;

sm__force_inline struct vert_shader_resource *
resource_vert_shader_make_reference(struct vert_shader_resource *vertex)
{
	rc_increment(&vertex->refs);

	return (vertex);
}

typedef struct frag_shader_resource
{
	str8 name;
	str8 fragment;
	u32 id;

	struct ref_counter refs;
} frag_shader_resource;

sm__force_inline struct frag_shader_resource *
resource_frag_shader_make_reference(struct frag_shader_resource *fragment)
{
	rc_increment(&fragment->refs);

	return (fragment);
}

struct shaders
{
	frag_shader_resource *fragment;
	vert_shader_resource *vertex;
};
struct shaders load_shader(str8 fragment, str8 vertex);

// struct scene_resource *load_scene_gltf(str8 filename);
// struct scene_resource *load_animated_model_gltf(str8 filename);

// file handling
extern const u32 FS_FILETYPE_REGULAR;
extern const u32 FS_FILETYPE_DIRECTORY;
extern const u32 FS_FILETYPE_SYMLINK;
extern const u32 FS_FILETYPE_OTHER;

struct fs_stat
{
	i64 filesize;	/**< size in bytes, -1 for non-files and unknown */
	i64 modtime;	/**< last modification time */
	i64 createtime; /**< like modtime, but for file creation time */
	i64 accesstime; /**< like modtime, but for file access time */
	u32 filetype;	/**< File? Directory? Symlink? */
	i32 readonly;	/**< non-zero if read only, zero if writable. */
};

struct fs_file
{
	b8 ok;
	void *fsfile;
	struct fs_stat status;
};
struct fs_file fs_file_open(str8 name, b8 read_only);
struct fs_file fs_file_open_read_cstr(const i8 *name);
struct fs_file fs_file_open_write_cstr(const i8 *name);
void fs_file_close(struct fs_file *resource_file);

// Conversion tool utils
b8 sm___resource_mock_init(struct buf base_memory, i8 *argv[], str8 assets_folder);
void sm___resource_mock_teardown(void);
void sm___resource_mock_read(void);

struct image_resource *sm___resource_mock_push_image(struct image_resource *image);
struct material_resource *sm___resource_mock_push_material(struct material_resource *material);
struct mesh_resource *sm___resource_mock_push_mesh(struct mesh_resource *mesh);
struct scene_resource *sm___resource_mock_push_scene(struct scene_resource *scene);
struct armature_resource *sm___resource_mock_push_armature(struct armature_resource *armature);
struct clip_resource *sm___resource_mock_push_clip(struct clip_resource *clip);

#endif // SM_CORE_RESOURCE_H
