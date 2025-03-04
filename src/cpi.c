#include "cpi.h"
#include "vec.h"
#include "vec_path.h"
#include "debug.h"
#include <stdlib.h>
#include <SDL3/SDL.h>
#include <SDL3_shadercross/SDL_shadercross.h>
#include "spirv_reflect.h"

// ===============================================================================================================
// CPU states
// ===============================================================================================================
typedef struct CPI_Pointer {
#ifdef DEBUG
	unsigned long long  			id;
#endif
	void* 							ptr;
	bool   							read;
	bool   							write;
} CPI_Pointer;

// ===============================================================================================================
// CPU operators
// ===============================================================================================================
typedef struct CPI_Process {
#ifdef DEBUG
	unsigned long long 				id;
#endif
} CPI_Process;

typedef struct CPI_Thread {
#ifdef DEBUG
	unsigned long long 				id;
#endif
	SDL_Thread*  					p_thread;
	SDL_ThreadID        			thread_id;
} CPI_Thread;

typedef struct CPI_Function{
#ifdef DEBUG
	unsigned long long 				id;
#endif

} CPI_Function;

typedef struct CPI_ShadercCompiler{
#ifdef DEBUG
	unsigned long long 				id;
#endif
	SDL_ThreadID    				thread_id;
	shaderc_compiler_t  			shaderc_compiler;
	shaderc_compile_options_t 		shaderc_options;
} CPI_ShadercCompiler;


// ===============================================================================================================
// GPU states
// ===============================================================================================================
typedef struct CPI_GPUDevice {
#if defined(DEBUG)
	long long   					id;
#endif
	SDL_GPUDevice*					p_gpu_device;
} CPI_GPUDevice;

typedef struct CPI_Window {
#ifdef DEBUG
	unsigned long long 				id;
#endif
	SDL_Window* 					p_sdl_window;
	SDL_ThreadID        			thread_id;
} CPI_Window;

typedef struct CPI_Fence {
#if defined(DEBUG)
	long long   					id;
#endif
} CPI_Fence;

typedef struct CPI_Sampler {
#if defined(DEBUG)
	long long   					id;
#endif
} CPI_Sampler;

typedef struct CPI_Image {
#if defined(DEBUG)
	long long   					id;
#endif
} CPI_Image;

typedef struct CPI_Transfer {
#if defined(DEBUG)
	long long   					id;
#endif
} CPI_Transfer;

typedef struct CPI_Buffer{
#if defined(DEBUG)
	long long   					id;
#endif
} CPI_Buffer;


// ===============================================================================================================
// GPU operators
// ===============================================================================================================
typedef struct CPI_Shader {
#if defined(DEBUG)
	long long   					id;
#endif
    void*                           p_glsl_code;
    void*                           p_spv_code;
    unsigned int                    spv_code_size;
    unsigned int                    glsl_code_size;
    const char*  					entrypoint;

	char*  							shaderc_compiler_path;
    shaderc_shader_kind             shader_kind;

    SpvReflectShaderModule          reflect_shader_module;

	char*   						gpu_device_path;
    SDL_GPUShader*    				p_sdl_shader;
} CPI_Shader;

typedef struct CPI_GraphicsPipeline {
#if defined(DEBUG)
	long long   					id;
#endif
	char*  							vertex_shader_path;
	char* 							fragment_shader_path;
	SDL_GPUGraphicsPipeline* 		p_sdl_pipeline;
} CPI_GraphicsPipeline;

typedef struct CPI_ComputePipeline {
#if defined(DEBUG)
	long long   					id;
#endif
	CPI_Shader 						vertex_shader;
	CPI_Shader 						fragment_shader;
} CPI_ComputePipeline;

typedef struct CPI_RenderPass {
#if defined(DEBUG)
	long long   					id;
#endif
} CPI_RenderPass;

typedef struct CPI_ComputePass {
#if defined(DEBUG)
	long long   					id;
#endif
} CPI_ComputePass;

typedef struct CPI_CopyPass {
#if defined(DEBUG)
	long long   					id;
#endif
} CPI_CopyPass;

typedef struct CPI_Command {
#if defined(DEBUG)
	long long   					id;
#endif
	SDL_ThreadID        			thread_id;
} CPI_Command;

// CPU States
static Type cpi_box_type;
// CPU Operators
static Type cpi_process_type;
static Type cpi_thread_type;
static Type cpi_function_type;
static Type cpi_shaderc_compiler_type;
// GPU States
static Type cpi_window_type;
static Type cpi_fence_type;
static Type cpi_sampler_type;
static Type cpi_image_type;
static Type cpi_transfer_type;
static Type cpi_buffer_type;
// GPU Operators
static Type cpi_gpu_device_type;
static Type cpi_command_type;
static Type cpi_renderpass_type;
static Type cpi_computepass_type;
static Type cpi_copyPass_type;
static Type cpi_graphics_pipeline_type;
static Type cpi_computerpipeline_type;
static Type cpi_shader_type;

static Vec g_vec = {0};
#ifdef DEBUG
	SDL_Mutex*  			g_unique_id_mutex = NULL;
	unsigned long long  	g_unique_id = 0;
#endif

// ===============================================================================================================
// Internal Functions
// ===============================================================================================================

// ===============================================================================================================
// main
// ===============================================================================================================
void cpi_Initialize() 
{
	#ifdef DEBUG
		DEBUG_ASSERT(!g_unique_id_mutex, "not NULL pointer");
		DEBUG_SCOPE(g_unique_id_mutex = SDL_CreateMutex());
	#endif
	DEBUG_SCOPE(g_vec = vec_Create(NULL, vec_type));

	// You have to set the types that needs to be destroyed first, first
	DEBUG_SCOPE(cpi_window_type = type_Create("CPI_Window", sizeof(CPI_Window), cpi_Window_Destructor));
	DEBUG_SCOPE(cpi_shaderc_compiler_type = type_Create("CPI_ShadercCompiler", sizeof(CPI_ShadercCompiler), cpi_ShadercCompiler_Destructor));
	DEBUG_SCOPE(cpi_shader_type = type_Create("CPI_Shader", sizeof(CPI_Shader), cpi_Shader_Destructor));
	DEBUG_SCOPE(cpi_graphics_pipeline_type = type_Create("CPI_GraphicsPipeline", sizeof(CPI_GraphicsPipeline), cpi_GraphicsPipeline_Destructor));
	DEBUG_SCOPE(cpi_gpu_device_type = type_Create("CPI_GPUDevice", sizeof(CPI_GPUDevice), cpi_GPUDevice_Destructor));

	DEBUG_SCOPE(bool result = SDL_Init(SDL_INIT_VIDEO));
	DEBUG_ASSERT(result, "ERROR: failed to initialize SDL3: %s", SDL_GetError());
    DEBUG_SCOPE(result = SDL_ShaderCross_Init());
    DEBUG_ASSERT(result, "Failed to initialize SDL_ShaderCross. %s", SDL_GetError());
}

// ===============================================================================================================
// Window
// ===============================================================================================================
char* cpi_Window_Create(
	unsigned int width,
	unsigned int height,
	const char* title)
{
	DEBUG_ASSERT(title, "title is NULL");
	DEBUG_SCOPE(int window_vec_index = vec_UpsertIndexOfFirstVecWithType_SafeWrite(&g_vec, cpi_window_type))
	DEBUG_SCOPE(Vec* p_window_vec = vec_GetAtVaArgs_LockWrite(&g_vec, 1, window_vec_index));
	DEBUG_ASSERT(vec_IsValid_UnsafeRead(p_window_vec), "window vec is not valid");
	DEBUG_SCOPE(int window_index = vec_UpsertIndexOfNullElement_SafeWrite(p_window_vec));
	DEBUG_SCOPE(CPI_Window* p_window = (CPI_Window*)vec_GetAtVaArgs_LockWrite(p_window_vec, 1, window_index));
	DEBUG_ASSERT(p_window, "NULL pointer");
	DEBUG_ASSERT(!p_window->p_sdl_window, "INTERNAL ERROR: sdl window should be NULL");
	DEBUG_SCOPE(p_window->p_sdl_window = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE));
	DEBUG_ASSERT(p_window->p_sdl_window, "ERROR: failed to create window: %s", SDL_GetError());

	#ifdef DEBUG
		SDL_LockMutex(g_unique_id_mutex);
		p_window->id = g_unique_id++;
		SDL_UnlockMutex(g_unique_id_mutex);
	#endif

	DEBUG_SCOPE(ASSERT(p_window == vec_GetAtVaArgs_UnlockWrite(p_window_vec, 1, window_index), "unlocked write for wrong element"));
	DEBUG_SCOPE(ASSERT(p_window_vec == vec_GetAtVaArgs_UnlockWrite(&g_vec, 1, window_vec_index), "unlocked write for wrong element"));

    DEBUG_SCOPE(char* return_path = vec_Path_FromVaArgs(2, window_vec_index, window_index));
    DEBUG_SCOPE(ASSERT(vec_IsValidAtPath_SafeRead(&g_vec, cpi_window_type, return_path), "newly created path is not valid")); 
    printf("SUCCESSFULLY created window\n");
	return return_path;
}
void cpi_Window_Show(
	char* window_path) 
{
	vec_LockRead(&g_vec);
	vec_Print_UnsafeRead(&g_vec, 0, 1);
	vec_UnlockRead(&g_vec);
	SDL_Event event;
    bool quit = false;
    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                quit = true;
            }
        }
        SDL_Delay(16);
    }
}
void cpi_Window_Destructor(void* p_void) {
	CPI_Window* p_window = (CPI_Window*)p_void;
	DEBUG_ASSERT(p_window, "NULL pointer");
	DEBUG_ASSERT(p_window->p_sdl_window, "NULL pointer");
	bool is_null = true;
	for (int i = 0; i < sizeof(CPI_ShadercCompiler); ++i) {
		if (((unsigned char*)p_window)[i] != 0) {
			is_null = false;
		}
	}
	ASSERT(!is_null, "shaderc compiler is null");
	DEBUG_SCOPE(SDL_DestroyWindow(p_window->p_sdl_window));
	memset(p_window, 0, sizeof(CPI_Window));
}
void cpi_Window_Destroy(
	char** p_window_path)
{
	DEBUG_ASSERT(p_window_path, "NULL pointer");
	DEBUG_ASSERT(vec_IsValidAtPath_SafeRead(&g_vec, cpi_window_type, *p_window_path), "UNVALID WINDOW HANDLE");
	DEBUG_SCOPE(CPI_Window* p_window = vec_GetAtPath_LockWrite(&g_vec, *p_window_path));
	DEBUG_SCOPE(cpi_Window_Destructor(p_window));
	DEBUG_SCOPE(ASSERT(p_window == vec_GetAtPath_UnlockWrite(&g_vec, *p_window_path), "unlocked write for wrong element"));
	DEBUG_SCOPE(free(*p_window_path));
	*p_window_path = NULL;
	printf("SUCCESSFULLY destroyed window\n");
}

// ===============================================================================================================
// Shaderc Compiler
// ===============================================================================================================
char* cpi_ShadercCompiler_GetPath() 
{
	DEBUG_SCOPE(SDL_ThreadID this_thread_id = SDL_GetCurrentThreadID());

	// Check if a shaderc compiler already exists for this thread
	DEBUG_SCOPE(unsigned short shaderc_compiler_vec_index = vec_UpsertIndexOfFirstVecWithType_SafeWrite(&g_vec, cpi_shaderc_compiler_type));
	DEBUG_SCOPE(Vec* p_sub_vec = vec_GetAtVaArgs_LockRead(&g_vec, 1, shaderc_compiler_vec_index));
	DEBUG_ASSERT(p_sub_vec, "NULL pointer");
	DEBUG_SCOPE(unsigned int count = vec_GetCount_UnsafeRead(p_sub_vec));
	DEBUG_SCOPE(ASSERT(p_sub_vec == vec_GetAtVaArgs_UnlockRead(&g_vec, 1, shaderc_compiler_vec_index), "unlocked read for wrong element"));
	if (count >= 1) {
		DEBUG_SCOPE(CPI_ShadercCompiler* p_compiler = vec_GetAtVaArgs_LockWrite(&g_vec, 2, shaderc_compiler_vec_index, 0));
		DEBUG_ASSERT(p_compiler, "NULL pointer");
		for (unsigned short i = 0; i < count; ++i) {
			if (p_compiler[i].thread_id == this_thread_id) {
				DEBUG_SCOPE(ASSERT(p_compiler == vec_GetAtVaArgs_UnlockWrite(&g_vec, 2, shaderc_compiler_vec_index, 0), "unlocked write for wrong element\n"));
				DEBUG_SCOPE(char* path = vec_Path_FromVaArgs(2, shaderc_compiler_vec_index, i));
				DEBUG_ASSERT(vec_IsValidAtPath_SafeRead(&g_vec, cpi_shaderc_compiler_type, path), "path is invalid");
				return path;
			}
		}
		DEBUG_SCOPE(ASSERT(p_compiler == vec_GetAtVaArgs_UnlockWrite(&g_vec, 2, shaderc_compiler_vec_index, 0), "unlocked write for wrong element\n"));
	}
		
	// at this point a shaderc compiler doesn't exist for this thread so the following will create it
	DEBUG_SCOPE(Vec* p_shaderc_compiler_vec = vec_GetAtVaArgs_LockWrite(&g_vec, 1, shaderc_compiler_vec_index));
	DEBUG_ASSERT(vec_IsValid_UnsafeRead(p_shaderc_compiler_vec), "window vec is not valid");
	DEBUG_SCOPE(int shaderc_compiler_index = vec_UpsertIndexOfNullElement_SafeWrite(p_shaderc_compiler_vec));
	DEBUG_SCOPE(CPI_ShadercCompiler* p_compiler = (CPI_ShadercCompiler*)vec_GetAtVaArgs_LockWrite(p_shaderc_compiler_vec, 1, shaderc_compiler_index));
	DEBUG_ASSERT(p_compiler, "NULL pointer");
	p_compiler->thread_id = this_thread_id;
    DEBUG_SCOPE(p_compiler->shaderc_compiler = shaderc_compiler_initialize());
    DEBUG_ASSERT(p_compiler->shaderc_compiler, "failed to initialize\n ");
    DEBUG_SCOPE(p_compiler->shaderc_options = shaderc_compile_options_initialize());
    DEBUG_ASSERT(p_compiler->shaderc_options, "failed to initialize\n ");
    DEBUG_SCOPE(shaderc_compile_options_set_optimization_level(p_compiler->shaderc_options, shaderc_optimization_level_zero));
    #ifdef __linux__
    	DEBUG_SCOPE(shaderc_compile_options_set_target_env(p_compiler->shaderc_options, shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_0));
    #else 
    	DEBUG_ASSERT(false, "OS not supported yet\n");
    #endif
    DEBUG_SCOPE(ASSERT(p_compiler == vec_GetAtVaArgs_UnlockWrite(p_shaderc_compiler_vec, 1, shaderc_compiler_index), "unlocked write for wrong element\n"));
    DEBUG_SCOPE(ASSERT(p_compiler == vec_GetAtVaArgs_UnlockWrite(&g_vec,				 1, shaderc_compiler_vec_index), "unlocked write for wrong element\n"));
    DEBUG_SCOPE(char* return_path = vec_Path_FromVaArgs(2, shaderc_compiler_vec_index, shaderc_compiler_index));
    DEBUG_SCOPE(ASSERT(vec_IsValidAtPath_SafeRead(&g_vec, cpi_shaderc_compiler_type, return_path), "newly created shaderc compiler path is not valid"));
    printf("SUCCESSFULLY created shaderc compiler\n");
    return return_path;
}
void cpi_ShadercCompiler_Destructor(void* p_void) {
	CPI_ShadercCompiler* p_shaderc_compiler = (CPI_ShadercCompiler*)p_void;
	DEBUG_ASSERT(p_shaderc_compiler, "NULL pointer");
	bool is_null = true;
	for (int i = 0; i < sizeof(CPI_ShadercCompiler); ++i) {
		if (((unsigned char*)p_shaderc_compiler)[i] != 0) {
			is_null = false;
		}
	}
	ASSERT(!is_null, "shaderc compiler is null");
    shaderc_compiler_release(p_shaderc_compiler->shaderc_compiler);
    shaderc_compile_options_release(p_shaderc_compiler->shaderc_options);
    memset(p_shaderc_compiler, 0, sizeof(CPI_ShadercCompiler));
}
void cpi_ShadercCompiler_Destroy(char** p_shaderc_compiler_path) {
    ASSERT(p_shaderc_compiler_path, "NULL pointer");
    ASSERT(vec_IsValidAtPath_SafeRead(&g_vec, cpi_shaderc_compiler_type, *p_shaderc_compiler_path), "*p_shaderc_compiler_path is not valid");

    DEBUG_SCOPE(CPI_ShadercCompiler* p_compiler = (CPI_ShadercCompiler*)vec_GetAtPath_LockWrite(&g_vec, *p_shaderc_compiler_path));

    DEBUG_SCOPE(cpi_ShadercCompiler_Destructor(p_compiler));

    DEBUG_ASSERT(p_compiler == vec_GetAtPath_UnlockWrite(&g_vec, *p_shaderc_compiler_path), "unlocked write for wrong element");

    free(*p_shaderc_compiler_path);
    *p_shaderc_compiler_path = NULL;
}

// ===============================================================================================================
// GPUDevice
// ===============================================================================================================
char* cpi_GPUDevice_Create() 
{
	DEBUG_SCOPE(int gpu_device_vec_index = vec_UpsertIndexOfFirstVecWithType_SafeWrite(&g_vec, cpi_gpu_device_type));
	DEBUG_SCOPE(Vec* p_gpu_device_vec = vec_GetAtVaArgs_LockWrite(&g_vec, 1, gpu_device_vec_index));
	DEBUG_ASSERT(vec_IsValid_UnsafeRead(p_gpu_device_vec), "window vec is not valid");
	DEBUG_SCOPE(int gpu_device_index = vec_UpsertIndexOfNullElement_SafeWrite(p_gpu_device_vec));
	DEBUG_SCOPE(CPI_GPUDevice* p_gpu_device = vec_GetAtVaArgs_LockWrite(p_gpu_device_vec, 1, gpu_device_index));

	DEBUG_ASSERT(!p_gpu_device->p_gpu_device, "pointer should be NULL");

	#ifdef __linux__
		#ifdef DEBUG
			DEBUG_SCOPE(p_gpu_device->p_gpu_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL));
		#else 
			DEBUG_SCOPE(p_gpu_device->p_gpu_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, NULL));
		#endif // LCPI_DEBUG
	#elif defined(_WIN64)
		DEBUG_ASSERT(false, "windows 64-bit is not supported yet\n");
	#elif defined(_WIN32)
		DEBUG_ASSERT(false, "windows 32-bit is not supported yet\n");
	#elif defined(__CYGWIN__)
		DEBUG_ASSERT(false, "cygwin is not supported yet\n");
	#elif defined(__APPLE__)
		DEBUG_ASSERT(false, "macos is not supported yet\n");
	#elif defined(__FreeBSD__)
		DEBUG_ASSERT(false, "free bsd is not supported yet\n");
	#elif defined(__ANDROID__)
		DEBUG_ASSERT(false, "android is not supported yet\n");
	#else 
		DEBUG_ASSERT(false, "unrecignized os is not supported\n");
	#endif

	DEBUG_ASSERT(p_gpu_device->p_gpu_device, "ERROR: failed to create SDL3 device: %s\n", SDL_GetError());
	#ifdef DEBUG 
		SDL_LockMutex(g_unique_id_mutex);
		p_gpu_device->id = g_unique_id++;
		SDL_UnlockMutex(g_unique_id_mutex);
	#endif 
	DEBUG_SCOPE(ASSERT(p_gpu_device == vec_GetAtVaArgs_UnlockWrite(p_gpu_device_vec, 1, gpu_device_index), "unlocked write for wrong element\n"));
	DEBUG_SCOPE(ASSERT(p_gpu_device_vec == vec_GetAtVaArgs_UnlockWrite(&g_vec, 1, gpu_device_vec_index), "unlocked write for wrong element\n"));

	DEBUG_SCOPE(char* return_path = vec_Path_FromVaArgs(2, gpu_device_vec_index, gpu_device_index));
	DEBUG_SCOPE(ASSERT(vec_IsValidAtPath_SafeRead(&g_vec, cpi_gpu_device_type, return_path), "newly created path is not valid"));
	printf("SUCCESSFULLY created gpu device\n");
	return return_path;
}
void cpi_GPUDevice_Destructor(void* p_void) {
	CPI_GPUDevice* p_gpu_device = (CPI_GPUDevice*)p_void;
	DEBUG_ASSERT(p_gpu_device, "NULL pointer");
	DEBUG_ASSERT((CPI_GPUDevice*)p_gpu_device->p_gpu_device, "NULL pointer");

	bool is_null = true;
	for (int i = 0; i < sizeof(CPI_ShadercCompiler); ++i) {
		if (((unsigned char*)p_gpu_device)[i] != 0) {
			is_null = false;
		}
	}
	ASSERT(!is_null, "shaderc compiler is null");
	DEBUG_SCOPE(SDL_DestroyGPUDevice(p_gpu_device->p_gpu_device));
	memset(p_gpu_device, 0, sizeof(CPI_GPUDevice));
}
void cpi_GPUDevice_Destroy(char** p_gpu_device_path) {
	DEBUG_ASSERT(p_gpu_device_path, "NULL pointer");
	DEBUG_ASSERT(vec_IsValidAtPath_SafeRead(&g_vec, cpi_gpu_device_type, *p_gpu_device_path), "gpu device path is invalid");

    DEBUG_SCOPE(CPI_GPUDevice* p_gpu_device = (CPI_GPUDevice*)vec_GetAtPath_LockWrite(&g_vec, *p_gpu_device_path));
    DEBUG_SCOPE(cpi_GPUDevice_Destructor(p_gpu_device->p_gpu_device));
    DEBUG_ASSERT(p_gpu_device == vec_GetAtPath_UnlockWrite(&g_vec, *p_gpu_device_path), "unlocked write for wrong element");

    free(*p_gpu_device_path);
    *p_gpu_device_path = NULL;
}

// ===============================================================================================================
// Shader
// ===============================================================================================================
unsigned long long _cpi_Shader_ReadFile(
	const char* filename,
	char** const dst_buffer)
{
    FILE* file = fopen(filename, "rb");
    if (!file) {
        DEBUG_ASSERT(false, "Failed to open shader source file '%s'\n", filename);
    }

    fseek(file, 0, SEEK_END);
    unsigned long long file_size = ftell(file);
    rewind(file);

    DEBUG_SCOPE(*dst_buffer = (char*)alloc(NULL, file_size + 1));
    if (!*dst_buffer) {
        fclose(file);
        exit(-1);
        DEBUG_ASSERT(false, "Failed to allocate memory for shader source '%s'\n", filename);
    }

    unsigned long long readSize = fread(*dst_buffer, 1, file_size, file);
    (*dst_buffer)[file_size] = '\0'; 

    if (readSize != file_size) {
        free(*dst_buffer);
        fclose(file);
        DEBUG_ASSERT(false, "Failed to read shader source '%s'\n", filename);
    }

    fclose(file);
    return (unsigned int)file_size;
}
unsigned int _cpi_Shader_FormatSize(
	SDL_GPUVertexElementFormat format) 
{
	DEBUG_ASSERT(format != SDL_GPU_VERTEXELEMENTFORMAT_INVALID, "INTERNAL ERROR: format is invlaid\n");
  	unsigned int result = 0;
  	switch (format) {
	    case SDL_GPU_VERTEXELEMENTFORMAT_INVALID: result = 0; break;

	    /* 32-bit Signed Integers */
	    case SDL_GPU_VERTEXELEMENTFORMAT_INT: result = 4; break;
	    case SDL_GPU_VERTEXELEMENTFORMAT_INT2: result = 8; break;
	    case SDL_GPU_VERTEXELEMENTFORMAT_INT3: result = 12; break;
	    case SDL_GPU_VERTEXELEMENTFORMAT_INT4: result = 16; break;

	    /* 32-bit Unsigned Integers */
	    case SDL_GPU_VERTEXELEMENTFORMAT_UINT: result = 4; break;
	    case SDL_GPU_VERTEXELEMENTFORMAT_UINT2: result = 8; break;
	    case SDL_GPU_VERTEXELEMENTFORMAT_UINT3: result = 12; break;
	    case SDL_GPU_VERTEXELEMENTFORMAT_UINT4: result = 16; break;

	    /* 32-bit Floats */
	    case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT: result = 4; break;
	    case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2: result = 8; break;
	    case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3: result = 12; break;
	    case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4: result = 16; break;

	    /* 8-bit Signed Integers */
	    case SDL_GPU_VERTEXELEMENTFORMAT_BYTE2: result = 2; break;
	    case SDL_GPU_VERTEXELEMENTFORMAT_BYTE4: result = 4; break;

	    /* 8-bit Unsigned Integers */
	    case SDL_GPU_VERTEXELEMENTFORMAT_UBYTE2: result = 2; break;
	    case SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4: result = 4; break;

	    /* 8-bit Signed Normalized */
	    case SDL_GPU_VERTEXELEMENTFORMAT_BYTE2_NORM: result = 2; break;
	    case SDL_GPU_VERTEXELEMENTFORMAT_BYTE4_NORM: result = 4; break;

	    /* 8-bit Unsigned Normalized */
	    case SDL_GPU_VERTEXELEMENTFORMAT_UBYTE2_NORM: result = 2; break;
	    case SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM: result = 4; break;

	    /* 16-bit Signed Integers */
	    case SDL_GPU_VERTEXELEMENTFORMAT_SHORT2: result = 4; break;
	    case SDL_GPU_VERTEXELEMENTFORMAT_SHORT4: result = 8; break;

	    /* 16-bit Unsigned Integers */
	    case SDL_GPU_VERTEXELEMENTFORMAT_USHORT2: result = 4; break;
	    case SDL_GPU_VERTEXELEMENTFORMAT_USHORT4: result = 8; break;

	    /* 16-bit Signed Normalized */
	    case SDL_GPU_VERTEXELEMENTFORMAT_SHORT2_NORM: result = 4; break;
	    case SDL_GPU_VERTEXELEMENTFORMAT_SHORT4_NORM: result = 8; break;

	    /* 16-bit Unsigned Normalized */
	    case SDL_GPU_VERTEXELEMENTFORMAT_USHORT2_NORM: result = 4; break;
	    case SDL_GPU_VERTEXELEMENTFORMAT_USHORT4_NORM: result = 8; break;

	    /* 16-bit Floats */
	    case SDL_GPU_VERTEXELEMENTFORMAT_HALF2: result = 4; break;
	    case SDL_GPU_VERTEXELEMENTFORMAT_HALF4: result = 8; break;
  	}
  	DEBUG_ASSERT(result != 0, "INTERNAL ERROR: could not find any format\n");
  	return result;
}
SDL_GPUVertexElementFormat _cpi_spvReflectFormatToSDLGPUformat(
	SpvReflectFormat format) 
{
    switch(format) {
        case SPV_REFLECT_FORMAT_R16G16_UINT: return SDL_GPU_VERTEXELEMENTFORMAT_USHORT2;
        case SPV_REFLECT_FORMAT_R16G16_SINT: return SDL_GPU_VERTEXELEMENTFORMAT_SHORT2;
        case SPV_REFLECT_FORMAT_R16G16_SFLOAT: return SDL_GPU_VERTEXELEMENTFORMAT_HALF2;
        case SPV_REFLECT_FORMAT_R16G16B16A16_UINT: return SDL_GPU_VERTEXELEMENTFORMAT_USHORT4;
        case SPV_REFLECT_FORMAT_R16G16B16A16_SINT: return SDL_GPU_VERTEXELEMENTFORMAT_SHORT4;
        case SPV_REFLECT_FORMAT_R16G16B16A16_SFLOAT: return SDL_GPU_VERTEXELEMENTFORMAT_HALF4;
        case SPV_REFLECT_FORMAT_R32_UINT: return SDL_GPU_VERTEXELEMENTFORMAT_UINT;
        case SPV_REFLECT_FORMAT_R32_SINT: return SDL_GPU_VERTEXELEMENTFORMAT_INT;
        case SPV_REFLECT_FORMAT_R32_SFLOAT: return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
        case SPV_REFLECT_FORMAT_R32G32_UINT: return SDL_GPU_VERTEXELEMENTFORMAT_UINT2;
        case SPV_REFLECT_FORMAT_R32G32_SINT: return SDL_GPU_VERTEXELEMENTFORMAT_INT2;
        case SPV_REFLECT_FORMAT_R32G32_SFLOAT: return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        case SPV_REFLECT_FORMAT_R32G32B32A32_UINT: return SDL_GPU_VERTEXELEMENTFORMAT_UINT4; 
        case SPV_REFLECT_FORMAT_R32G32B32A32_SINT: return SDL_GPU_VERTEXELEMENTFORMAT_INT4;
        case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT: return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;

        // 64-bit Formats (Unsupported)
        default: {
        	DEBUG_ASSERT(false, "spv reflect format is not supported\n");
            return SDL_GPU_VERTEXELEMENTFORMAT_INVALID;
        }
    }
}
void _cpi_Shader_PrintAttributeDescriptions(
	SDL_GPUVertexAttribute* p_attributes, 
	unsigned long long attribs_count) 
{
	DEBUG_ASSERT(p_attributes, "NULL pointer");
    for (unsigned int i = 0; i < attribs_count; ++i) {
        printf("attrib %d\n", i);
        printf("\t%d\n", p_attributes[i].location);
        printf("\t%d\n", p_attributes[i].buffer_slot);
        printf("\t%d\n", p_attributes[i].format);
        printf("\t%d\n", p_attributes[i].offset);
    }
}
SDL_GPUVertexAttribute* _cpi_Shader_Create_VertexInputAttribDesc(
	const char* vertex_shader_path,
	unsigned int* p_attribute_count, 
	unsigned int* p_binding_stride) 
{
	DEBUG_ASSERT(p_attribute_count, "NULL pointer");
	DEBUG_ASSERT(p_binding_stride, "NULL pointer");
	DEBUG_SCOPE(CPI_Shader* p_shader = vec_GetAtPath_LockRead(&g_vec, vertex_shader_path));
	DEBUG_ASSERT(p_shader, "NULL pointer");
    DEBUG_ASSERT(p_shader->reflect_shader_module.shader_stage == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT, "Provided shader is not a vertex shader\n");

    // Enumerate input variables
    unsigned int input_var_count = 0;
    DEBUG_SCOPE(SpvReflectResult result = spvReflectEnumerateInputVariables(&p_shader->reflect_shader_module, &input_var_count, NULL));
    DEBUG_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, "Failed to enumerate input variables\n");

    DEBUG_SCOPE(SpvReflectInterfaceVariable** input_vars = alloc(NULL, input_var_count * sizeof(SpvReflectInterfaceVariable*)));
    DEBUG_ASSERT(input_vars, "Failed to allocate memory for input variables\n");

    DEBUG_SCOPE(result = spvReflectEnumerateInputVariables(&p_shader->reflect_shader_module, &input_var_count, input_vars));
    DEBUG_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, "Failed to get input variables\n");
    DEBUG_SCOPE(ASSERT(p_shader == vec_GetAtPath_UnlockRead(&g_vec, vertex_shader_path), "unlocked read for wrong element\n"));

    // Create an array to hold SDL_GPUVertexAttribute
    DEBUG_SCOPE(SDL_GPUVertexAttribute* attribute_descriptions = alloc(NULL, input_var_count * sizeof(SDL_GPUVertexAttribute)));
    DEBUG_ASSERT(attribute_descriptions, "Failed to allocate memory for vertex input attribute descriptions\n");

    unsigned int attribute_index = 0;
    for (unsigned int i = 0; i < input_var_count; ++i) {
        SpvReflectInterfaceVariable* refl_var = input_vars[i];

        // Ignore built-in variables
        if (refl_var->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) {
            continue;
        }

        attribute_descriptions[attribute_index].location = refl_var->location;
        attribute_descriptions[attribute_index].buffer_slot = 0;
        DEBUG_SCOPE(attribute_descriptions[attribute_index].format = _cpi_spvReflectFormatToSDLGPUformat(refl_var->format));
        attribute_descriptions[attribute_index].offset = 0; // WILL CALCULATE OFFSET LATER
        attribute_index++;
    }

    // Update the attribute count
    *p_attribute_count = attribute_index;

    // Sort attributes by location
    for (unsigned int i = 0; i < attribute_index - 1; ++i) {
        for (unsigned int j = 0; j < attribute_index - i - 1; ++j) {
            if (attribute_descriptions[j].location > attribute_descriptions[j + 1].location) {
                SDL_GPUVertexAttribute temp = attribute_descriptions[j];
                attribute_descriptions[j] = attribute_descriptions[j + 1];
                attribute_descriptions[j+1] = temp;
            }
        }
    }

    // Compute offsets and buffer_slot stride
    unsigned int offset = 0;
    for (unsigned int i = 0; i < attribute_index; ++i) {
        DEBUG_SCOPE(unsigned int format_size = _cpi_Shader_FormatSize(attribute_descriptions[i].format));

        if (format_size == 0) {
            printf("Unsupported format for input variable at location %u\n", attribute_descriptions[i].location);
            continue;
        }

        // Align the offset if necessary (e.g., 4-byte alignment)
        unsigned int alignment = 4;
        offset = (offset + (alignment - 1)) & ~(alignment - 1);
        attribute_descriptions[i].offset = offset;
        offset += format_size;
    }

    *p_binding_stride = offset;
    free(input_vars);
    _cpi_Shader_PrintAttributeDescriptions(attribute_descriptions, *p_attribute_count);

    return attribute_descriptions;  
}
char* cpi_Shader_CreateFromGlslFile(
	char* device_path,
	const char* glsl_file_path, 
	const char* entrypoint,
	shaderc_shader_kind shader_kind, 
	bool enable_debug) 
{
	DEBUG_ASSERT(vec_IsValidAtPath_SafeRead(&g_vec, cpi_gpu_device_type, device_path), "path is invalid\n");
	DEBUG_ASSERT(glsl_file_path, "NULL pointer");
	DEBUG_ASSERT(entrypoint, "NULL pointer");
	DEBUG_ASSERT(shader_kind == shaderc_vertex_shader ||
  				 shader_kind == shaderc_fragment_shader || 
  				 shader_kind == shaderc_compute_shader,
  				"shader kind is not supported");

	DEBUG_SCOPE(int shader_vec_index = vec_UpsertIndexOfNullElement_SafeWrite(&g_vec));
	DEBUG_SCOPE(Vec* p_shader_vec = (Vec*)vec_GetAtVaArgs_LockRead(&g_vec, 1, shader_vec_index));
	DEBUG_SCOPE(int shader_index = vec_UpsertIndexOfFirstVecWithType_SafeWrite(p_shader_vec, cpi_shader_type));
	DEBUG_SCOPE(ASSERT(p_shader_vec == vec_GetAtVaArgs_UnlockRead(&g_vec, 1, shader_vec_index), "unlocked read for wrong element"));

	CPI_Shader shader = {0};
	shader.entrypoint = entrypoint;
	shader.shader_kind = shader_kind;

	// get shaderc compiler path
	{
		DEBUG_SCOPE(char* shaderc_compiler_path = cpi_ShadercCompiler_GetPath());
		DEBUG_SCOPE(DEBUG_ASSERT(vec_IsValidAtPath_SafeRead(&g_vec, cpi_shaderc_compiler_type, shaderc_compiler_path), "shaderc compiler path is invalid\n"));
		shader.shaderc_compiler_path = shaderc_compiler_path;
	}

	// spv code compilation
	{
		char* p_glsl_code = NULL;
	    DEBUG_SCOPE(unsigned long long glsl_code_size = _cpi_Shader_ReadFile(glsl_file_path, &p_glsl_code));
	    DEBUG_ASSERT(p_glsl_code, "NULL pointer");
	    DEBUG_SCOPE(CPI_ShadercCompiler* p_shaderc_compiler = vec_GetAtPath_LockRead(&g_vec, shader.shaderc_compiler_path))
	    DEBUG_ASSERT(p_shaderc_compiler, "NULL pointer");
	   	DEBUG_SCOPE(shaderc_compilation_result_t result = shaderc_compile_into_spv(p_shaderc_compiler->shaderc_compiler, p_glsl_code, glsl_code_size, shader_kind, glsl_file_path, "main", p_shaderc_compiler->shaderc_options));
	   	DEBUG_SCOPE(ASSERT(p_shaderc_compiler == vec_GetAtPath_UnlockRead(&g_vec, shader.shaderc_compiler_path), "unlocked read for wrong element\n"));
	    DEBUG_ASSERT(shaderc_result_get_compilation_status(result) == shaderc_compilation_status_success, "Shader compilation error in '%s':\n%s\n", glsl_file_path, shaderc_result_get_error_message(result));
		DEBUG_SCOPE(shader.spv_code_size = shaderc_result_get_length(result));
	    DEBUG_SCOPE(shader.p_spv_code = alloc(NULL, shader.spv_code_size));
	    DEBUG_ASSERT(shader.p_spv_code, "Failed to allocate memory for SPIR-V code");
	    DEBUG_SCOPE(memcpy(shader.p_spv_code, shaderc_result_get_bytes(result), shader.spv_code_size));
	    DEBUG_SCOPE(shaderc_result_release(result));
	}

    // SpvReflectShaderModule
    {
		DEBUG_SCOPE(SpvReflectResult reflectResult = spvReflectCreateShaderModule(shader.spv_code_size, shader.p_spv_code, &shader.reflect_shader_module));
	    DEBUG_ASSERT(reflectResult == SPV_REFLECT_RESULT_SUCCESS, "Failed to create SPIRV-Reflect shader module\n");
	    DEBUG_ASSERT(shader_kind != shaderc_vertex_shader || shader.reflect_shader_module.shader_stage == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT, "generated reflect shader and shaderc kind is not the same");
	    DEBUG_ASSERT(shader_kind != shaderc_fragment_shader || shader.reflect_shader_module.shader_stage == SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT, "generated reflect shader and shaderc kind is not the same");
	    DEBUG_ASSERT(shader_kind != shaderc_compute_shader || shader.reflect_shader_module.shader_stage == SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT, "generated reflect shader and shaderc kind is not the same");
		DEBUG_ASSERT(shader_kind == shaderc_vertex_shader || shader_kind == shaderc_fragment_shader || shader_kind == shaderc_compute_shader, "shader kind is not supported. AND THIS SHOULD HAVE BEEN CHECKED EARLIER");
	}

	// get gpu device path
	{
		shader.gpu_device_path = device_path;
	}

	// SDL shader 
	{
		// vertex or fragment shader
		if (shader_kind == shaderc_vertex_shader || shader_kind == shaderc_fragment_shader) {
			bool is_vert = shader_kind == shaderc_vertex_shader;
			// Prepare SPIRV_Info for Vertex Shader
		    SDL_ShaderCross_SPIRV_Info shader_info = {
		        .bytecode = shader.p_spv_code,
		        .bytecode_size = shader.spv_code_size,
		        .entrypoint = entrypoint,
		        .shader_stage = is_vert ? SDL_SHADERCROSS_SHADERSTAGE_VERTEX : SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT,
		        .enable_debug = enable_debug,
		        .name = NULL,
		        .props = 0  // Assuming no special properties
		    };
		    
			DEBUG_SCOPE(CPI_GPUDevice* p_gpu_device = vec_GetAtPath_LockRead(&g_vec, shader.gpu_device_path));
			DEBUG_ASSERT(p_gpu_device, "NULL pointer");
			DEBUG_ASSERT(p_gpu_device->p_gpu_device, "NULL pointer");
			SDL_ShaderCross_GraphicsShaderMetadata metadata;
		    DEBUG_SCOPE(shader.p_sdl_shader = SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(p_gpu_device->p_gpu_device, &shader_info, &metadata));
		    DEBUG_ASSERT(shader.p_sdl_shader, "Failed to compile Shader from SPIR-V. %s\n", SDL_GetError());
			DEBUG_SCOPE(ASSERT(p_gpu_device == vec_GetAtPath_UnlockRead(&g_vec, shader.gpu_device_path), "unlocked read for wrong element"));
		} 
		// compute shader. there is no sdl shader for compute. its integrated directly into the pipeline
		else {
			shader.p_sdl_shader = NULL;
		}
	}

	#ifdef DEBUG 
		SDL_LockMutex(g_unique_id_mutex);
		shader.id = g_unique_id++;
		SDL_UnlockMutex(g_unique_id_mutex);
	#endif 

	DEBUG_SCOPE(CPI_Shader* p_shader = vec_GetAtVaArgs_LockWrite(&g_vec, 2, shader_vec_index, shader_index));
	DEBUG_ASSERT(p_shader, "NULL pointer");
	memcpy(p_shader, &shader, sizeof(CPI_Shader));
	DEBUG_SCOPE(ASSERT(p_shader == vec_GetAtVaArgs_UnlockWrite(&g_vec, 2, shader_vec_index, shader_index), ""));

	DEBUG_SCOPE(char* return_path = vec_Path_FromVaArgs(2, shader_vec_index, shader_index));
	DEBUG_SCOPE(ASSERT(vec_IsValidAtPath_SafeRead(&g_vec, cpi_shader_type, return_path), "newly created path is invalid"));
	printf("SUCCESSFULLY created shader\n");
    return return_path;
}
void cpi_Shader_Destructor(void* p_void) {
    CPI_Shader* p_shader = (CPI_Shader*)p_void;
    DEBUG_ASSERT(p_shader, "NULL pointer");
    DEBUG_ASSERT(p_shader->gpu_device_path, "NULL pointer");

    bool is_null = true;
	for (int i = 0; i < sizeof(CPI_ShadercCompiler); ++i) {
		if (((unsigned char*)p_shader)[i] != 0) {
			is_null = false;
		}
	}
	ASSERT(!is_null, "shaderc compiler is null");

    if (p_shader->p_glsl_code) {free(p_shader->p_glsl_code);}
    if (p_shader->p_spv_code) {free(p_shader->p_spv_code);}

    DEBUG_SCOPE(spvReflectDestroyShaderModule(&p_shader->reflect_shader_module));
    DEBUG_ASSERT(vec_IsValidAtPath_SafeRead(&g_vec, cpi_gpu_device_type, p_shader->gpu_device_path), "path is not valid");
    DEBUG_SCOPE(CPI_GPUDevice* p_gpu_device = vec_GetAtPath_LockRead(&g_vec, p_shader->gpu_device_path));
	DEBUG_ASSERT(p_gpu_device, "NULL pointer");
	DEBUG_ASSERT(p_gpu_device->p_gpu_device, "NULL pointer");
    DEBUG_SCOPE(SDL_ReleaseGPUShader(p_gpu_device->p_gpu_device, p_shader->p_sdl_shader));
    DEBUG_SCOPE(ASSERT(p_gpu_device == vec_GetAtPath_UnlockRead(&g_vec, p_shader->gpu_device_path), "unlock read for wrong element\n"));
    memset(p_shader, 0, sizeof(CPI_Shader));
}
void cpi_Shader_Destroy(char** p_shader_path) {
    DEBUG_ASSERT(p_shader_path, "NULL pointer");
    DEBUG_ASSERT(vec_IsValidAtPath_SafeRead(&g_vec, cpi_shader_type, *p_shader_path), "shader path is invalid");
	DEBUG_SCOPE(CPI_Shader* p_shader = vec_GetAtPath_LockWrite(&g_vec, *p_shader_path));

	DEBUG_SCOPE(cpi_Shader_Destructor(p_shader));

    DEBUG_SCOPE(ASSERT(p_shader == vec_GetAtPath_UnlockWrite(&g_vec, *p_shader_path), "unlock write for wrong element\n"));
    DEBUG_SCOPE(free(*p_shader_path));
    *p_shader_path = NULL;
}

// ===============================================================================================================
// Graphics Pipeline
// ===============================================================================================================
char* cpi_GraphicsPipeline_Create(
    char* vertex_shader_path,
    char* fragment_shader_path,
    bool enable_debug)
{
	DEBUG_ASSERT(vec_IsValidAtPath_SafeRead(&g_vec, cpi_shader_type, vertex_shader_path), "vertex_shader path is not valid");
	DEBUG_ASSERT(vec_IsValidAtPath_SafeRead(&g_vec, cpi_shader_type, fragment_shader_path), "fragmetn_shader path is not valid");

	DEBUG_SCOPE(CPI_Shader* p_vertex_shader = vec_GetAtPath_LockRead(&g_vec, vertex_shader_path));
	DEBUG_SCOPE(CPI_Shader* p_fragment_shader = vec_GetAtPath_LockRead(&g_vec, fragment_shader_path));

	DEBUG_ASSERT(strcmp(p_vertex_shader->gpu_device_path, p_fragment_shader->gpu_device_path) == 0,"shaders does not contain the same gpu device\n");
	char* gpu_device_path = p_vertex_shader->gpu_device_path;

	// 1. Vertex Input State
	unsigned int vertex_attributes_count;
	unsigned int vertex_binding_stride;
	DEBUG_SCOPE(SDL_GPUVertexAttribute* vertex_attributes = _cpi_Shader_Create_VertexInputAttribDesc(vertex_shader_path, &vertex_attributes_count, &vertex_binding_stride));

	SDL_GPUVertexInputState vertex_input_state = {
	    .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]) {
	        {
	            .slot = 0,
	            .pitch = vertex_binding_stride,
	            .input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE,
	            .instance_step_rate = 1
	        }
	    },
	    .num_vertex_buffers = 1,
	    .vertex_attributes = vertex_attributes,
	    .num_vertex_attributes = vertex_attributes_count
	};

	// 3. Rasterizer State
	SDL_GPURasterizerState rasterizer_state = {
	    .fill_mode = SDL_GPU_FILLMODE_FILL,
	    .cull_mode = SDL_GPU_CULLMODE_BACK,
	    .front_face = SDL_GPU_FRONTFACE_CLOCKWISE,
	    .depth_bias_constant_factor = 0.0f,
	    .depth_bias_clamp = 0.0f,
	    .depth_bias_slope_factor = 0.0f,
	    .enable_depth_bias = false,
	    .enable_depth_clip = true,
	    .padding1 = 0,
	    .padding2 = 0
	};

	// 4. Multisample State
	SDL_GPUMultisampleState multisample_state = {
	    .sample_count = SDL_GPU_SAMPLECOUNT_1,
	    .sample_mask = 0xFFFFFFFF,
	    .enable_mask = false,
	    .padding1 = 0,
	    .padding2 = 0,
	    .padding3 = 0
	};

	// 5. Depth Stencil State
	SDL_GPUDepthStencilState depth_stencil_state = {
	    .compare_op = SDL_GPU_COMPAREOP_LESS,
	    .back_stencil_state = { /* Initialize as needed */ },
	    .front_stencil_state = { /* Initialize as needed */ },
	    .compare_mask = 0xFF,
	    .write_mask = 0xFF,
	    .enable_depth_test = false,
	    .enable_depth_write = false,
	    .enable_stencil_test = false,
	    .padding1 = 0,
	    .padding2 = 0,
	    .padding3 = 0
	};

	// 7. Render Targets
	SDL_GPUTextureFormat color_format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;

	SDL_GPUColorTargetDescription color_targets[] = {
	    {
	        .format = color_format,
	        .blend_state = (SDL_GPUColorTargetBlendState){
	            .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
	            .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
	            .color_blend_op = SDL_GPU_BLENDOP_ADD,
	            .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
	            .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
	            .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
	            .color_write_mask = SDL_GPU_COLORCOMPONENT_R
	                              | SDL_GPU_COLORCOMPONENT_G
	                              | SDL_GPU_COLORCOMPONENT_B
	                              | SDL_GPU_COLORCOMPONENT_A,
	            .enable_blend = true,
	            .enable_color_write_mask = true
	        }
	    }
	};

	SDL_GPUGraphicsPipelineTargetInfo target_info = {
	    .color_target_descriptions = color_targets,
	    .num_color_targets = 1,
	    .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_INVALID,
	    .has_depth_stencil_target = false,
	    .padding1 = 0,
	    .padding2 = 0,
	    .padding3 = 0
	};


	// 8. Pipeline Creation
	SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info = {
	    .vertex_shader = p_vertex_shader->p_sdl_shader,
    	.fragment_shader = p_fragment_shader->p_sdl_shader,
	    .vertex_input_state = vertex_input_state,
	    .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
	    .rasterizer_state = rasterizer_state,
	    .multisample_state = multisample_state,
	    .depth_stencil_state = depth_stencil_state,
	    .target_info = target_info,
	    .props = 0
	};

	DEBUG_SCOPE(CPI_GPUDevice* p_gpu_device = vec_GetAtPath_LockRead(&g_vec, gpu_device_path));
	DEBUG_ASSERT(p_gpu_device, "NULL pointer\n");
	DEBUG_ASSERT(p_gpu_device->p_gpu_device, "NULL pointer\n");

	CPI_GraphicsPipeline pipeline = {0};
    pipeline.vertex_shader_path = vertex_shader_path;
    pipeline.fragment_shader_path = fragment_shader_path;

	DEBUG_SCOPE(pipeline.p_sdl_pipeline = SDL_CreateGPUGraphicsPipeline(p_gpu_device->p_gpu_device, &pipeline_create_info));
	DEBUG_ASSERT(pipeline.p_sdl_pipeline, "Failed to create SDL3 graphics pipeline: %s\n", SDL_GetError());
	DEBUG_SCOPE(ASSERT(p_gpu_device == 		vec_GetAtPath_UnlockRead(&g_vec, gpu_device_path), "unlock read for wrong element\n"));
	DEBUG_SCOPE(ASSERT(p_fragment_shader == vec_GetAtPath_UnlockRead(&g_vec, fragment_shader_path), "unlock read for wrong element\n"));
	DEBUG_SCOPE(ASSERT(p_vertex_shader == 	vec_GetAtPath_UnlockRead(&g_vec, vertex_shader_path), "unlock read for wrong element\n"));


	DEBUG_SCOPE(int pipeline_vec_index = vec_UpsertIndexOfFirstVecWithType_SafeWrite(&g_vec, cpi_graphics_pipeline_type));
	DEBUG_SCOPE(Vec* p_pipeline_vec = vec_GetAtVaArgs_LockRead(&g_vec, 1, pipeline_vec_index));
	DEBUG_SCOPE(int pipeline_index = vec_UpsertIndexOfNullElement_SafeWrite(p_pipeline_vec));
	DEBUG_SCOPE(ASSERT(p_pipeline_vec == vec_GetAtVaArgs_UnlockRead(&g_vec, 1, pipeline_vec_index), "unlocked read for wrong element"));
	DEBUG_SCOPE(CPI_GraphicsPipeline* p_pipeline = vec_GetAtVaArgs_LockWrite(&g_vec, 2, pipeline_vec_index, pipeline_index));

	DEBUG_SCOPE(memcpy(p_pipeline, &pipeline, sizeof(CPI_GraphicsPipeline)));
	DEBUG_SCOPE(ASSERT(p_pipeline == vec_GetAtVaArgs_UnlockWrite(&g_vec, 2, pipeline_vec_index, pipeline_index), "unlocked write for wrong element\n"));

    DEBUG_SCOPE(char* return_path = vec_Path_FromVaArgs(2, pipeline_vec_index, pipeline_index));
    DEBUG_SCOPE(ASSERT(vec_IsValidAtPath_SafeRead(&g_vec, cpi_graphics_pipeline_type, return_path), "newly created path is not valid")); 

    printf("Graphics Pipeline created successfully.");
    return return_path;
}
void cpi_GraphicsPipeline_Destructor(
	void* p_void)
{
	CPI_GraphicsPipeline* p_graphics_pipeline = (CPI_GraphicsPipeline*)p_void;
	DEBUG_ASSERT(p_graphics_pipeline, "NULL pointer");

	DEBUG_SCOPE(CPI_Shader* p_vertex_shader = vec_GetAtPath_LockRead(&g_vec, p_graphics_pipeline->vertex_shader_path));
	DEBUG_ASSERT(vec_IsValidAtPath_SafeRead(&g_vec, cpi_gpu_device_type, p_vertex_shader->gpu_device_path), "gpu device path is invlaid");
	DEBUG_SCOPE(CPI_GPUDevice* p_gpu_device = vec_GetAtPath_LockRead(&g_vec, p_vertex_shader->gpu_device_path));
	DEBUG_ASSERT(p_gpu_device->p_gpu_device, "NULL pointer\n");

	DEBUG_SCOPE(SDL_ReleaseGPUGraphicsPipeline(p_gpu_device->p_gpu_device, p_graphics_pipeline->p_sdl_pipeline));

	DEBUG_SCOPE(ASSERT(p_gpu_device == vec_GetAtPath_UnlockRead(&g_vec, p_vertex_shader->gpu_device_path), "unlocked read for wrong element\n"));
	DEBUG_SCOPE(ASSERT(p_vertex_shader == vec_GetAtPath_UnlockRead(&g_vec, p_graphics_pipeline->vertex_shader_path), "unlocked read for wrong element\n"));
}
void cpi_GraphicsPipeline_Destroy(
	char** p_graphics_pipeline_path)
{
	DEBUG_ASSERT(p_graphics_pipeline_path, "NULL pointer");
	DEBUG_ASSERT(vec_IsValidAtPath_SafeRead(&g_vec, cpi_graphics_pipeline_type, *p_graphics_pipeline_path), "graphics pipeline path is invlaid");
	DEBUG_SCOPE(CPI_GraphicsPipeline* p_pipeline = vec_GetAtPath_LockWrite(&g_vec, *p_graphics_pipeline_path));

	DEBUG_SCOPE(cpi_GraphicsPipeline_Destructor(p_pipeline));
	DEBUG_SCOPE(memset(p_pipeline, 0, sizeof(CPI_GraphicsPipeline)));

	DEBUG_SCOPE(ASSERT(p_pipeline == vec_GetAtPath_UnlockWrite(&g_vec, *p_graphics_pipeline_path), "unlocked write for wrong element"));
	*p_graphics_pipeline_path = NULL;
}