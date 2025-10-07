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

    Vgk_DescriptorSetDescription global_descriptor_set_description = {};
    global_descriptor_set_description.bindings[0].descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    global_descriptor_set_description.bindings[0].descriptor_count = 1;
    global_descriptor_set_description.bindings[0].stage_flags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    global_descriptor_set_description.binding_count = 1;
    Vgk_DescriptorSetBundle global_descriptor_set = vgk_create_descriptor_set_bundle(&descriptor_pool_bundle, &global_descriptor_set_description, device);

    Vgk_DescriptorSetDescription ui_descriptor_set_description = {};
    Vgk_DescriptorBinding ui_descriptor_bindings[MAX_DESCRIPTOR_BINDINGS] = {};
    ui_descriptor_set_description.bindings[0].descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ui_descriptor_set_description.bindings[0].descriptor_count = 2;
    ui_descriptor_set_description.bindings[0].stage_flags = VK_SHADER_STAGE_FRAGMENT_BIT;
    ui_descriptor_set_description.binding_count = 1;
    Vgk_DescriptorSetBundle ui_descriptor_set = vgk_create_descriptor_set_bundle(&descriptor_pool_bundle, &ui_descriptor_set_description, device);

    Vgk_PipelineDescription pipeline_description = {};
    pipeline_description.vert_shader_path = "bin/shaders/ui.vert.spv";
    pipeline_description.frag_shader_path = "bin/shaders/ui.frag.spv";

    pipeline_description.frame_count = frame_list.count;

    pipeline_description.descriptor_sets[0] = global_descriptor_set;
    pipeline_description.descriptor_sets[1] = ui_descriptor_set;
    pipeline_description.descriptor_set_count = 2;

    Vgk_VertInputDescription vert_input_description = {};
    vert_input_description.stride = sizeof(Vertex);
    vert_input_description.attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vert_input_description.attributes[0].offset = offsetof(Vertex, pos);
    vert_input_description.attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
    vert_input_description.attributes[1].offset = offsetof(Vertex, uv);
    vert_input_description.attributes[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    vert_input_description.attributes[2].offset = offsetof(Vertex, color);
    vert_input_description.attributes[3].format = VK_FORMAT_R32_UINT;
    vert_input_description.attributes[3].offset = offsetof(Vertex, tex_index);
    vert_input_description.attribute_count = 4;

    pipeline_description.viewport = vgk_get_viewport_for_extent(swapchain_bundle.extent);
    pipeline_description.scissor = vgk_get_scissor_for_extent(swapchain_bundle.extent);

    pipeline_description.polygon_mode = VK_POLYGON_MODE_FILL;
    pipeline_description.line_width = 1.0f;
    pipeline_description.cull_mode = VK_CULL_MODE_NONE;
    pipeline_description.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    pipeline_description.rasterization_samples = VK_SAMPLE_COUNT_1_BIT;

    pipeline_description.enable_blending = true;

    pipeline_description.render_pass = render_pass_bundle.render_pass;

    Vgk_PipelineBundle pipeline_bundle = vgk_create_pipeline_from_description(&pipeline_description, device);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
