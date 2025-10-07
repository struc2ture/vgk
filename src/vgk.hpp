#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "common/common.hpp"

#define MAX_DESCRIPTOR_SETS 4
#define MAX_DESCRIPTOR_BINDINGS 16
#define MAX_VERT_ATTRIBUTES 16

#define MAX_DESCRIPTOR_SETS_IN_POOL 256
#define MAX_UNIFORM_BUFFERS_IN_POOL 128
#define MAX_IMAGE_SAMPLERS_IN_POOL 128

struct Vgk_SwapchainBundle
{
    VkSwapchainKHR swapchain;
    VkSurfaceFormatKHR format;
    VkExtent2D extent;
    VkImage *images;
    VkImageView *image_views;
    VkSemaphore *submit_semaphores;
    u32 image_count;
};

struct Vgk_DepthImageBundle
{
    VkImage *images;
    VkDeviceMemory *memory_list;
    VkImageView *image_views;
    u32 image_count;
    VkFormat depth_format;
};

struct Vgk_RenderPassBundle
{
    VkRenderPass render_pass;
    VkFramebuffer *framebuffers;
    u32 framebuffer_count;
    VkFormat color_format;
    VkFormat depth_format;
};

struct Vgk_Frame
{
    VkCommandBuffer command_buffer;
    VkFence in_flight_fence;
    bool should_wait_on_fence;
    VkSemaphore acquire_semaphore;
};

struct Vgk_FrameList
{
    Vgk_Frame *frames;
    u32 count;
};

struct Vgk_BufferBundle
{
    VkBuffer buffer;
    VkDeviceMemory memory;
    void *data_ptr;
    VkDeviceSize size;
};

struct Vgk_BufferBundleList
{
    Vgk_BufferBundle *buffer_bundles;
    u32 count;
};

struct Vgk_TextureBundle
{
    VkImage image;
    VkDeviceMemory memory;
    VkImageView image_view;
    VkSampler sampler;
    VkFormat format;
};

struct Vgk_DescriptorPoolBundle
{
    VkDescriptorPool descriptor_pool;

    u32 set_count;
    u32 max_sets;

    u32 uniform_buffer_count;
    u32 max_uniform_buffers;

    u32 image_sampler_count;
    u32 max_image_samplers;
};

struct Vgk_DescriptorBinding
{
    VkDescriptorType descriptor_type;
    u32 descriptor_count;
    VkShaderStageFlags stage_flags;
};

struct Vgk_DescriptorSetSpec
{
    Vgk_DescriptorBinding bindings[MAX_DESCRIPTOR_BINDINGS];
    u32 binding_count;
};

struct Vgk_DescriptorSetBundle
{
    Vgk_DescriptorSetSpec spec;
    VkDescriptorSetLayout layout;
    VkDescriptorSet descriptor_set;
};

struct Vgk_VertAttributeSpec
{
    VkFormat format;
    u32 offset;
};

struct Vgk_VertInputSpec
{
    u32 stride;
    Vgk_VertAttributeSpec attributes[MAX_VERT_ATTRIBUTES];
    u32 attribute_count;
};

struct Vgk_PipelineLayoutSpec
{
    // TODO: Push constants
    Vgk_DescriptorSetSpec descriptor_sets[MAX_DESCRIPTOR_SETS];
    u32 descriptor_set_count;
};

// ====================================================================

struct Vgk_PipelineSpec
{
    const char *vert_shader_path;
    const char *frag_shader_path;

    u32 frame_count;

    Vgk_PipelineLayoutSpec pipeline_layout_spec;

    Vgk_VertInputSpec vert_input_spec;
    
    // .......

    VkViewport viewport;
    VkRect2D scissor;

    VkPolygonMode polygon_mode;
    f32 line_width;
    VkCullModeFlags cull_mode;
    VkFrontFace front_face;

    VkSampleCountFlagBits rasterization_samples;

    bool enable_blending;
    bool enable_depth_testing;

    VkRenderPass render_pass;
};

struct Vgk_PipelineBundle
{
    Vgk_PipelineSpec spec;
    // VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout layout;
    VkPipeline pipeline;
};

// ============================ CREATE ===============================

VkInstance vgk_create_instance();
VkSurfaceKHR vgk_create_surface(VkInstance instance, GLFWwindow *window);
VkPhysicalDevice vgk_find_physical_device(VkInstance instance);
VkDevice vgk_create_device(u32 queue_family_index, VkPhysicalDevice physical_device);
VkQueue vgk_get_queue(VkDevice device, u32 queue_family_index);
Vgk_SwapchainBundle vgk_create_swapchain_bundle(VkPhysicalDevice physical_device, VkSurfaceKHR surface, VkDevice device);
Vgk_DepthImageBundle vgk_create_depth_image_bundle(VkFormat depth_format, u32 image_count, VkExtent2D swapchain_extent, VkDevice device, VkPhysicalDevice physical_device);
Vgk_RenderPassBundle vgk_create_render_pass_bundle(const Vgk_SwapchainBundle *swapchain_bundle, const Vgk_DepthImageBundle *depth_image_bundle, bool with_clear, bool is_final, VkDevice device);
VkCommandPool vgk_create_command_pool(u32 queue_family_index, VkDevice device);
Vgk_FrameList vgk_create_frame_list(u32 frames_in_flight, VkCommandPool command_pool, VkDevice device);
void vgk_frame_list_reset_sync_objects(Vgk_FrameList *frame_list, VkDevice device);
VkShaderModule vgk_create_shader_module(const char *path, VkDevice device);
Vgk_BufferBundle vgk_create_buffer_bundle(VkDeviceSize size, VkBufferUsageFlags usage, VkDevice device, VkPhysicalDevice physical_device);
Vgk_BufferBundleList vgk_create_buffer_bundle_list(VkDeviceSize max_size, VkBufferUsageFlags usage, u32 frames_in_flight, VkDevice device, VkPhysicalDevice physical_device);

Vgk_DescriptorPoolBundle vgk_create_descriptor_pool_bundle(VkDevice device);
Vgk_DescriptorSetBundle vgk_create_descriptor_set_bundle_from_spec(Vgk_DescriptorPoolBundle *descriptor_pool_bundle, const Vgk_DescriptorSetSpec *description, VkDevice device);
void vgk_check_descriptor_pool_availability(Vgk_DescriptorPoolBundle *descriptor_pool_bundle, const Vgk_DescriptorSetSpec *description);
VkDescriptorSetLayout vgk_create_descriptor_set_layout_from_spec(const Vgk_DescriptorSetSpec *spec, VkDevice device);

Vgk_VertInputSpec vgk_make_vert_input_spec(size_t stride);
void vgk_add_vert_attribute(Vgk_VertInputSpec *description, VkFormat format, size_t offset);

Vgk_PipelineBundle vgk_create_pipeline_from_spec(const Vgk_PipelineSpec *description, VkDevice device);

// ============================ DESTROY ===============================

void vgk_destroy_swapchain_bundle(Vgk_SwapchainBundle *bundle, VkDevice device);
void vgk_destroy_depth_image_bundle(Vgk_DepthImageBundle *bundle, VkDevice device);
void vgk_destroy_render_pass_bundle(Vgk_RenderPassBundle *bundle, VkDevice device);
void vgk_destroy_command_pool(VkCommandPool *command_pool, VkDevice device);
void vgk_destroy_frame_list(Vgk_FrameList *list, VkDevice device);
void vgk_destroy_buffer_bundle(Vgk_BufferBundle *bundle, VkDevice device);
void vgk_destroy_buffer_bundle_list(Vgk_BufferBundleList *list, VkDevice device);
void vgk_destroy_texture_bundle(Vgk_TextureBundle *bundle, VkDevice device);
// TODO: destroy_descriptor_set_bundle
// TODO: destroy_descriptor_pool_bundle
// TODO: destroy_pipeline_bundle

// ============================ HELPERS ===============================

u32 vgk_get_queue_family_index(VkPhysicalDevice physical_device, VkSurfaceKHR surface);
u32 vgk_find_memory_type(VkPhysicalDevice physical_device, u32 type_filter, VkMemoryPropertyFlags props);
VkViewport vgk_get_viewport_for_extent(VkExtent2D extent);
VkRect2D vgk_get_scissor_for_extent(VkExtent2D extent);
