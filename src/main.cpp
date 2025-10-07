#include <GLFW/glfw3.h>
#include <cstddef>

#include "vgk.hpp"

#define FRAMES_IN_FLIGHT 2

struct Vertex
{
    v3 pos;
    v2 uv;
    v4 color;
    u32 tex_index;
};

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    GLFWwindow *window = glfwCreateWindow(1000, 900, "arena", NULL, NULL);

    VkInstance instance = vgk_create_instance();
    VkSurfaceKHR surface = vgk_create_surface(instance, window);
    VkPhysicalDevice physical_device = vgk_find_physical_device(instance);
    u32 queue_family_index = vgk_get_queue_family_index(physical_device, surface);
    VkDevice device = vgk_create_device(queue_family_index, physical_device);
    VkQueue queue = vgk_get_queue(device, queue_family_index);
    Vgk_SwapchainBundle swapchain_bundle = vgk_create_swapchain_bundle(physical_device, surface, device);
    Vgk_DepthImageBundle depth_image_bundle = vgk_create_depth_image_bundle(VK_FORMAT_D32_SFLOAT, swapchain_bundle.image_count, swapchain_bundle.extent, device, physical_device);
    Vgk_RenderPassBundle render_pass_bundle = vgk_create_render_pass_bundle(&swapchain_bundle, &depth_image_bundle, true, false, device);
    VkCommandPool command_pool = vgk_create_command_pool(queue_family_index, device);
    Vgk_FrameList frame_list = vgk_create_frame_list(FRAMES_IN_FLIGHT, command_pool, device);

    Vgk_DescriptorPoolBundle descriptor_pool_bundle = vgk_create_descriptor_pool_bundle(device);

    Vgk_DescriptorSetSpec ui_descriptor_set_spec = vgk_make_descriptor_set_spec();
    vgk_add_descriptor_binding(&ui_descriptor_set_spec, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    vgk_add_descriptor_binding(&ui_descriptor_set_spec, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, VK_SHADER_STAGE_FRAGMENT_BIT);
    Vgk_DescriptorSetBundle ui_descriptor_set = vgk_create_descriptor_set_bundle_from_spec(&descriptor_pool_bundle, &ui_descriptor_set_spec, device);

    Vgk_VertInputSpec vert_input = vgk_make_vert_input_spec(sizeof(Vertex));
    vgk_add_vert_attribute(&vert_input, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos));
    vgk_add_vert_attribute(&vert_input, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv));
    vgk_add_vert_attribute(&vert_input, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color));
    vgk_add_vert_attribute(&vert_input, VK_FORMAT_R32_UINT, offsetof(Vertex, tex_index));

    Vgk_PipelineSpec pipeline_spec = vgk_make_pipeline_spec();
    vgk_set_vert_shader_path(&pipeline_spec, "bin/shaders/ui.vert.spv");
    vgk_set_frag_shader_path(&pipeline_spec, "bin/shaders/ui.frag.spv");
    vgk_set_frame_count(&pipeline_spec, frame_list.count);
    vgk_add_descriptor_set(&pipeline_spec, &ui_descriptor_set_spec);
    vgk_set_vert_input(&pipeline_spec, &vert_input);
    vgk_set_viewport(&pipeline_spec, vgk_get_viewport_for_extent(swapchain_bundle.extent));
    vgk_set_scissor(&pipeline_spec, vgk_get_scissor_for_extent(swapchain_bundle.extent));
    vgk_set_rasterization_state(&pipeline_spec, VK_POLYGON_MODE_FILL, 1.0f, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    vgk_set_sample_count(&pipeline_spec, VK_SAMPLE_COUNT_1_BIT);
    vgk_set_enable_blending(&pipeline_spec, true);
    vgk_set_render_pass(&pipeline_spec, render_pass_bundle.render_pass);

    Vgk_PipelineBundle pipeline_bundle = vgk_create_pipeline_from_spec(&pipeline_spec, device);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
