#include "vgk.hpp"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include "common/common.hpp"

// ======================== CREATE ======================================

VkInstance vgk_create_instance()
{
    VkInstance instance;
    {
        VkApplicationInfo app_info = {};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.apiVersion = VK_API_VERSION_1_3;

        u32 glfw_ext_count = 0;
        const char **glfw_ext = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
        const char *other_exts[] =
        {
#ifdef OS_MAC
            VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
#endif
        };

        u32 ext_count = 0;
        const char **extensions = (const char **)xmalloc((glfw_ext_count + array_count(other_exts)) * sizeof(extensions[0]));
        for (u32 i = 0; i < glfw_ext_count; i++)
        {
            extensions[ext_count++] = glfw_ext[i];
        }
        for (u32 i = 0; i < array_count(other_exts); i++)
        {
            extensions[ext_count++] = other_exts[i];
        }

        const char *validation_layers[] = { "VK_LAYER_KHRONOS_validation" };

        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;
        create_info.enabledExtensionCount = ext_count;
        create_info.ppEnabledExtensionNames = extensions;
        create_info.enabledLayerCount = array_count(validation_layers);
        create_info.ppEnabledLayerNames = validation_layers;
#ifdef OS_MAC
        create_info.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

        VkResult result = vkCreateInstance(&create_info, NULL, &instance);
        if (result != VK_SUCCESS) fatal("Failed to create instance. Result: %d", result);

        free(extensions);
    }
    return instance;
}

VkSurfaceKHR vgk_create_surface(VkInstance instance, GLFWwindow *window)
{
    VkSurfaceKHR surface;
    {
        VkResult result = glfwCreateWindowSurface(instance, window, NULL, &surface);
        if (result != VK_SUCCESS) fatal("Failed to create surface");
    }
    return surface;
}

VkPhysicalDevice vgk_find_physical_device(VkInstance instance)
{
    VkPhysicalDevice physical_device;
    {
        u32 count;
        VkResult result = vkEnumeratePhysicalDevices(instance, &count, NULL);
        if (result != VK_SUCCESS) fatal("Failed to enumerate physical devices");
        VkPhysicalDevice *physical_devices = (VkPhysicalDevice *)xmalloc(count * sizeof(physical_devices[0]));
        result = vkEnumeratePhysicalDevices(instance, &count, physical_devices);
        if (result != VK_SUCCESS) fatal("Failed to enumerate physical devices 2");
        physical_device = physical_devices[0];
        free(physical_devices);
    }
    return physical_device;
}

VkDevice vgk_create_device(u32 queue_family_index, VkPhysicalDevice physical_device)
{
    VkDevice vk_device;
    {
        float priority = 1.0f;
        VkDeviceQueueCreateInfo queue_create_info = {};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family_index;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &priority;
        // VK_KHR_portability_subset must be enabled because physical device VkPhysicalDevice 0x600001667be0 supports it.
        const char *device_extensions[] = {
#ifdef OS_MAC
            "VK_KHR_portability_subset", 
#endif
            "VK_KHR_swapchain"
        };
        VkDeviceCreateInfo device_create_info = {};
        device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_create_info.queueCreateInfoCount = 1;
        device_create_info.pQueueCreateInfos = &queue_create_info;
        device_create_info.enabledExtensionCount = array_count(device_extensions);
        device_create_info.ppEnabledExtensionNames = device_extensions;

        VkResult result = vkCreateDevice(physical_device, &device_create_info, NULL, &vk_device);
        if (result != VK_SUCCESS) fatal("Failed to create logical device");
    }
    return vk_device;
}

VkQueue vgk_get_queue(VkDevice device, u32 queue_family_index)
{
    VkQueue vk_graphics_queue;
    vkGetDeviceQueue(device, queue_family_index, 0, &vk_graphics_queue);
    return vk_graphics_queue;
}

Vgk_SwapchainBundle vgk_create_swapchain_bundle(VkPhysicalDevice physical_device, VkSurfaceKHR surface, VkDevice device)
{
    VkSurfaceCapabilitiesKHR capabilities;
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities);
    if (result != VK_SUCCESS) fatal("Failed to get physical device-surface capabilities");

    u32 format_count;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, NULL);
    if (result != VK_SUCCESS) fatal("Failed to get physical device-surface formats");

    VkSurfaceFormatKHR *formats = (VkSurfaceFormatKHR *)xmalloc(format_count * sizeof(formats[0]));
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, formats);
    if (result != VK_SUCCESS) fatal("Failed to get physical device-surface formats 2");

    VkSurfaceFormatKHR surface_format = formats[0];
    assert(surface_format.format == VK_FORMAT_B8G8R8A8_UNORM && surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);

    free(formats);

    u32 image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount)
    {
        image_count = capabilities.maxImageCount;
    }

    VkSwapchainKHR swapchain;
    {
        VkSwapchainCreateInfoKHR create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.surface = surface;
        create_info.minImageCount = image_count;
        create_info.imageFormat = surface_format.format;
        create_info.imageColorSpace = surface_format.colorSpace;
        create_info.imageExtent = capabilities.currentExtent;
        create_info.imageArrayLayers = 1;
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.preTransform = capabilities.currentTransform;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR; // vsync
        create_info.clipped = VK_TRUE;

        result = vkCreateSwapchainKHR(device, &create_info, NULL, &swapchain);
        if (result != VK_SUCCESS) fatal("Failed to create swapchain");
    }

    VkImage *images;
    {
        result = vkGetSwapchainImagesKHR(device, swapchain, &image_count, NULL);
        if (result != VK_SUCCESS) fatal("Failed to get swapchain images");

        images = (VkImage *)xmalloc(image_count * sizeof(images[0]));

        result = vkGetSwapchainImagesKHR(device, swapchain, &image_count, images);
        if (result != VK_SUCCESS) fatal("Failed to get swapchain images 2");
    }

    VkImageView *image_views = (VkImageView *)xmalloc(image_count * sizeof(image_views[0]));
    for (u32 i = 0; i < image_count; i++)
    {
        VkImageViewCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = surface_format.format;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(device, &create_info, NULL, &image_views[i]);
        if (result != VK_SUCCESS) fatal("Failed to create image view");
    }

    VkSemaphore *submit_semaphores = (VkSemaphore *)xmalloc(image_count * sizeof(submit_semaphores[0]));
    for (u32 i = 0; i < image_count; i++)
    {
        VkSemaphoreCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        result = vkCreateSemaphore(device, &create_info, NULL, &submit_semaphores[i]);
        if (result != VK_SUCCESS) fatal("Failed to create submit semaphore");
    }

    Vgk_SwapchainBundle swapchain_bundle = {};
    swapchain_bundle.format = surface_format;
    swapchain_bundle.swapchain = swapchain;
    swapchain_bundle.image_count = image_count;
    swapchain_bundle.images = images;
    swapchain_bundle.image_views = image_views;
    swapchain_bundle.submit_semaphores = submit_semaphores;
    swapchain_bundle.extent = capabilities.currentExtent;
    return swapchain_bundle;
}

Vgk_DepthImageBundle vgk_create_depth_image_bundle(VkFormat depth_format, u32 image_count, VkExtent2D swapchain_extent, VkDevice device, VkPhysicalDevice physical_device)
{
    Vgk_DepthImageBundle depth_image_bundle = {};

    depth_image_bundle.image_count = image_count;
    depth_image_bundle.depth_format = depth_format;

    VkResult result;

    VkImage *images = (VkImage *)xmalloc(depth_image_bundle.image_count * sizeof(images[0]));
    VkDeviceMemory *memory_list = (VkDeviceMemory *)xmalloc(depth_image_bundle.image_count * sizeof(memory_list[0]));
    VkImageView *image_views = (VkImageView *)xmalloc(depth_image_bundle.image_count * sizeof(image_views[0]));
    for (u32 i = 0; i < depth_image_bundle.image_count; i++)
    {
        // Image
        {
            VkImageCreateInfo create_info = {};
            create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            create_info.imageType = VK_IMAGE_TYPE_2D;
            create_info.extent.width = swapchain_extent.width;
            create_info.extent.height = swapchain_extent.height;
            create_info.extent.depth = 1;
            create_info.mipLevels = 1;
            create_info.arrayLayers = 1;
            create_info.format = depth_format;
            create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
            create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            create_info.samples = VK_SAMPLE_COUNT_1_BIT;
            create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            result = vkCreateImage(device, &create_info, NULL, &images[i]);
            if (result != VK_SUCCESS) fatal("Failed to create depth buffer image");
        }

        // Device memory
        {
            VkMemoryRequirements mem_req;
            vkGetImageMemoryRequirements(device, images[i], &mem_req);

            VkMemoryAllocateInfo allocate_info = {};
            allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocate_info.allocationSize = mem_req.size;
            allocate_info.memoryTypeIndex = vgk_find_memory_type(
                physical_device,
                mem_req.memoryTypeBits,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            result = vkAllocateMemory(device, &allocate_info, NULL, &memory_list[i]);
            if (result != VK_SUCCESS) fatal("Failed to allocate memory for depth buffer image");

            result = vkBindImageMemory(device, images[i], memory_list[i], 0);
            if (result != VK_SUCCESS) fatal("Failed to bind memory for depth buffer image");
        }

        // Image view
        {
            VkImageViewCreateInfo create_info = {};
            create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            create_info.image = images[i];
            create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            create_info.format = depth_format;
            create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            create_info.subresourceRange.baseMipLevel = 0;
            create_info.subresourceRange.levelCount = 1;
            create_info.subresourceRange.baseArrayLayer = 0;
            create_info.subresourceRange.layerCount = 1;

            VkResult result = vkCreateImageView(device, &create_info, NULL, &image_views[i]);
            if (result != VK_SUCCESS) fatal("Failed to create image view");
        }
    }

    depth_image_bundle.images = images;
    depth_image_bundle.memory_list = memory_list;
    depth_image_bundle.image_views = image_views;

    return depth_image_bundle;
}

Vgk_RenderPassBundle vgk_create_render_pass_bundle(const Vgk_SwapchainBundle *swapchain_bundle, const Vgk_DepthImageBundle *depth_image_bundle, bool with_clear, bool is_final, VkDevice device)
{
    bool with_depth = (depth_image_bundle != NULL);
    Vgk_RenderPassBundle render_pass_bundle = {};
    render_pass_bundle.color_format = swapchain_bundle->format.format;
    render_pass_bundle.depth_format = with_depth ? depth_image_bundle->depth_format : VK_FORMAT_UNDEFINED;
    render_pass_bundle.framebuffer_count = swapchain_bundle->image_count;

    VkRenderPass render_pass;
    {
        VkAttachmentDescription color_attachment_description = {};
        color_attachment_description.format = render_pass_bundle.color_format;
        color_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
        color_attachment_description.loadOp = with_clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        color_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment_description.initialLayout = with_clear ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachment_description.finalLayout = is_final ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference color_attachment_reference = {};
        color_attachment_reference.attachment = 0;
        color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription depth_attachment_description = {};
        depth_attachment_description.format = render_pass_bundle.depth_format;
        depth_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
        depth_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth_attachment_description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depth_attachment_reference = {};
        depth_attachment_reference.attachment = 1;
        depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass_description = {};
        subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_description.colorAttachmentCount = 1;
        subpass_description.pColorAttachments = &color_attachment_reference;
        if (with_depth)
        {
            subpass_description.pDepthStencilAttachment = &depth_attachment_reference;
        }

        VkRenderPassCreateInfo render_pass_create_info = {};
        render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_create_info.subpassCount = 1;
        render_pass_create_info.pSubpasses = &subpass_description;
        if (with_depth)
        {
            VkAttachmentDescription render_pass_attachments[] = { color_attachment_description, depth_attachment_description };
            render_pass_create_info.attachmentCount = array_count(render_pass_attachments);
            render_pass_create_info.pAttachments = render_pass_attachments;
        }
        else
        {
            render_pass_create_info.attachmentCount = 1;
            render_pass_create_info.pAttachments = &color_attachment_description;
        }

        VkResult result = vkCreateRenderPass(device, &render_pass_create_info, NULL, &render_pass);
        if (result != VK_SUCCESS) fatal("Failed to create render pass");
    }
    render_pass_bundle.render_pass = render_pass;

    VkFramebuffer *framebuffers = (VkFramebuffer *)xmalloc(render_pass_bundle.framebuffer_count * sizeof(framebuffers[0]));
    for (u32 i = 0; i < render_pass_bundle.framebuffer_count; i++)
    {
        VkFramebufferCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass = render_pass;
        create_info.width = swapchain_bundle->extent.width;
        create_info.height = swapchain_bundle->extent.height;
        create_info.layers = 1;

        if (with_depth)
        {
            VkImageView attachments[] =
            {
                swapchain_bundle->image_views[i],
                depth_image_bundle->image_views[i]
            };
            create_info.attachmentCount = array_count(attachments);
            create_info.pAttachments = attachments;
        }
        else
        {
            create_info.attachmentCount = 1;
            create_info.pAttachments = &swapchain_bundle->image_views[i];
        }

        VkResult result = vkCreateFramebuffer(device, &create_info, NULL, &framebuffers[i]);
        if (result != VK_SUCCESS) fatal("Failed to create framebuffer");
    }
    render_pass_bundle.framebuffers = framebuffers;

    return render_pass_bundle;
}

VkCommandPool vgk_create_command_pool(u32 queue_family_index, VkDevice device)
{
    VkCommandPool command_pool;
    {
        VkCommandPoolCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        create_info.queueFamilyIndex = queue_family_index;
        create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // allows resetting individual buffers

        VkResult result = vkCreateCommandPool(device, &create_info, NULL, &command_pool);
        if (result != VK_SUCCESS) fatal("Failed to create command pool");
    }
    return command_pool;
}

Vgk_FrameList vgk_create_frame_list(u32 frames_in_flight, VkCommandPool command_pool, VkDevice device)
{
    Vgk_FrameList frame_list = {};
    frame_list.count = frames_in_flight;

    Vgk_Frame *frames = (Vgk_Frame *)xmalloc(frame_list.count * sizeof(frames[0]));
    for (u32 i = 0; i < frame_list.count; i++)
    {
        VkCommandBuffer command_buffer;
        {
            VkCommandBufferAllocateInfo allocate_info = {};
            allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocate_info.commandPool = command_pool;
            allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocate_info.commandBufferCount = 1;

            VkResult result = vkAllocateCommandBuffers(device, &allocate_info, &command_buffer);
            if (result != VK_SUCCESS) fatal("Failed to allocate command buffer");
        }

        VkFence in_flight_fence;
        {
            VkFenceCreateInfo create_info = {};
            create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            VkResult result = vkCreateFence(device, &create_info, NULL, &in_flight_fence);
            if (result != VK_SUCCESS) fatal("Failed to create in flight fence");
        }

        VkSemaphore acquire_semaphore;
        {
            VkSemaphoreCreateInfo semaphore_create_info = {};
            semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkResult result = vkCreateSemaphore(device, &semaphore_create_info, NULL, &acquire_semaphore);
            if (result != VK_SUCCESS) fatal("Failed to create acquire semaphore");
        }

        frames[i].command_buffer = command_buffer;
        frames[i].in_flight_fence = in_flight_fence;
        frames[i].acquire_semaphore = acquire_semaphore;
    }
    frame_list.frames = frames;

    return frame_list;
}

void vgk_frame_list_reset_sync_objects(Vgk_FrameList *frame_list, VkDevice device)
{
    for (u32 i = 0; i < frame_list->count; i++)
    {
        vkDestroyFence(device, frame_list->frames[i].in_flight_fence, NULL);
        {
            VkFenceCreateInfo create_info = {};
            create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            VkResult result = vkCreateFence(device, &create_info, NULL, &frame_list->frames[i].in_flight_fence);
            if (result != VK_SUCCESS) fatal("Failed to create in flight fence");
        }
        vkDestroySemaphore(device, frame_list->frames[i].acquire_semaphore, NULL);
        {
            VkSemaphoreCreateInfo create_info = {};
            create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkResult result = vkCreateSemaphore(device, &create_info, NULL, &frame_list->frames[i].acquire_semaphore);
            if (result != VK_SUCCESS) fatal("Failed to create acquire semaphore");
        }
    }
}

VkShaderModule vgk_create_shader_module(const char *path, VkDevice device)
{
    FILE *file = fopen(path, "rb");
    bassert(file);
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);
    char* buffer = (char *)xmalloc(size);
    fread(buffer, 1, size, file);
    fclose(file);

    VkShaderModule module;
    {
        VkShaderModuleCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = (size_t)size;
        create_info.pCode = (uint32_t *)buffer;

        VkResult result = vkCreateShaderModule(device, &create_info, NULL, &module);
        if (result != VK_SUCCESS) fatal("Failed to create shader module");
    }

    free(buffer);
    return module;
}

Vgk_BufferBundle vgk_create_buffer_bundle(VkDeviceSize size, VkBufferUsageFlags usage, VkDevice device, VkPhysicalDevice physical_device)
{
    VkBuffer buffer;
    {
        VkBufferCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        create_info.size = size;
        create_info.usage = usage;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkResult result = vkCreateBuffer(device, &create_info, NULL, &buffer);
        if (result != VK_SUCCESS) fatal("Failed to create uniform buffer");
    }

    VkDeviceMemory device_memory;
    {
        VkMemoryRequirements memory_requirements;
        vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

        VkMemoryAllocateInfo allocate_info = {};
        allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocate_info.allocationSize = memory_requirements.size;
        allocate_info.memoryTypeIndex = vgk_find_memory_type(
            physical_device,
            memory_requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        VkResult result = vkAllocateMemory(device, &allocate_info, NULL, &device_memory);
        if (result != VK_SUCCESS) fatal("Failed to allocate memory for uniform buffer");

        result = vkBindBufferMemory(device, buffer, device_memory, 0);
        if (result != VK_SUCCESS) fatal("Failed to bind memory to uniform buffer");
    }

    void *data;
    {
        VkResult result = vkMapMemory(device, device_memory, 0, size, 0, &data);
        if (result != VK_SUCCESS) fatal("Failed to map vertex buffer memory");
    }

    Vgk_BufferBundle buffer_bundle = {};
    buffer_bundle.buffer = buffer;
    buffer_bundle.memory = device_memory;
    buffer_bundle.data_ptr = data;
    buffer_bundle.size = size;
    return buffer_bundle;
}

Vgk_BufferBundleList vgk_create_buffer_bundle_list(VkDeviceSize max_size, VkBufferUsageFlags usage, u32 frames_in_flight, VkDevice device, VkPhysicalDevice physical_device)
{
    Vgk_BufferBundleList buffer_bundle_list = {};
    buffer_bundle_list.count = frames_in_flight;

    Vgk_BufferBundle *buffer_bundles = (Vgk_BufferBundle *)xmalloc(buffer_bundle_list.count * sizeof(buffer_bundles[0]));
    for (u32 i = 0; i < buffer_bundle_list.count; i++)
    {
        buffer_bundles[i] = vgk_create_buffer_bundle(max_size, usage, device, physical_device);
    }
    buffer_bundle_list.buffer_bundles = buffer_bundles;

    return buffer_bundle_list;
}

void vgk_destroy_buffer_bundle(Vgk_BufferBundle *bundle);

Vgk_TextureBundle vgk_load_texture_from_pixels(void *pixels, u32 w, u32 h, VkDeviceSize image_size, VkFormat format, VkDevice device, VkPhysicalDevice physical_device, VkCommandPool command_pool, VkQueue queue)
{
    Vgk_BufferBundle staging_buffer = vgk_create_buffer_bundle(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, device, physical_device);

    memcpy(staging_buffer.data_ptr, pixels, (size_t)image_size);

    VkImage image;
    {
        VkImageCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        create_info.imageType = VK_IMAGE_TYPE_2D;
        create_info.format = format;
        create_info.extent = (VkExtent3D){ w, h, 1 };
        create_info.mipLevels = 1;
        create_info.arrayLayers = 1;
        create_info.samples = VK_SAMPLE_COUNT_1_BIT;
        create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkResult result = vkCreateImage(device, &create_info, NULL, &image);
        if (result != VK_SUCCESS) fatal("Failed to create texture image");
    }

    VkDeviceMemory memory;
    {
        VkMemoryRequirements mem_req;
        vkGetImageMemoryRequirements(device, image, &mem_req);

        VkMemoryAllocateInfo allocate_info = {};
        allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocate_info.allocationSize = mem_req.size;
        allocate_info.memoryTypeIndex = vgk_find_memory_type(
            physical_device,
            mem_req.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        VkResult result = vkAllocateMemory(device, &allocate_info, NULL, &memory);
        if (result != VK_SUCCESS) fatal("Failed to allocate memory for texture image");

        result = vkBindImageMemory(device, image, memory, 0);
        if (result != VK_SUCCESS) fatal("Failed to bind memory to texture image");
    }

    VkCommandBuffer command_buffer;
    {
        VkCommandBufferAllocateInfo allocate_info = {};
        allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate_info.commandPool = command_pool;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandBufferCount = 1;

        VkResult result = vkAllocateCommandBuffers(device, &allocate_info, &command_buffer);
        if (result != VK_SUCCESS) fatal("Failed to allocate command buffer for texture");
    }

    // Copy texture from staging buffer to image (GPU side) -- record commands
    {
        {
            VkCommandBufferBeginInfo begin_info = {};
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            VkResult result = vkBeginCommandBuffer(command_buffer, &begin_info);
            if (result != VK_SUCCESS) fatal("Failed to begin texture command buffer");
        }

        // Image layout: UNDEFINED -> TRANSFER_DST_OPTIMAL
        {
            VkImageMemoryBarrier barrier = {};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.image = image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            vkCmdPipelineBarrier(
                command_buffer,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, NULL,
                0, NULL,
                1, &barrier
            );
        }

        // Copy buffer to image
        {
            VkBufferImageCopy copy = {};
            copy.bufferOffset = 0;
            copy.bufferRowLength = 0;
            copy.bufferImageHeight = 0;
            copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copy.imageSubresource.mipLevel = 0;
            copy.imageSubresource.baseArrayLayer = 0;
            copy.imageSubresource.layerCount = 1;
            copy.imageOffset = (VkOffset3D){0, 0, 0};
            copy.imageExtent = (VkExtent3D){w, h, 1};

            vkCmdCopyBufferToImage(
                command_buffer,
                staging_buffer.buffer,
                image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &copy
            );
        }

        // Image layout: TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
        {
            VkImageMemoryBarrier barrier = {};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.image = image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            vkCmdPipelineBarrier(
                command_buffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0,
                0, NULL,
                0, NULL,
                1, &barrier
            );
        }

        VkResult result = vkEndCommandBuffer(command_buffer);
        if (result != VK_SUCCESS) fatal("Failed to end texture command buffer");
    }

    // Copy texture from staging buffer to image (GPU side) -- submit command buffer
    {
        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;

        VkResult result = vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
        if (result != VK_SUCCESS) fatal("Failed to submit texture command buffer to queue");
    }

    // Wait until the queue is idle
    {
        VkResult result = vkQueueWaitIdle(queue);
        if (result != VK_SUCCESS) fatal("Failed to wait idle for queue");
    }

    vgk_destroy_buffer_bundle(&staging_buffer, device);

    VkImageView image_view;
    {
        VkImageViewCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = image;
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = format;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(device, &create_info, NULL, &image_view);
        if (result != VK_SUCCESS) fatal("Failed to create texture image view");
    }

    VkSampler sampler;
    {
        VkSamplerCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        create_info.magFilter = VK_FILTER_LINEAR;
        create_info.minFilter = VK_FILTER_LINEAR;
        create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        create_info.anisotropyEnable = VK_FALSE;
        create_info.maxAnisotropy = 1.0f;
        create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        create_info.unnormalizedCoordinates = VK_FALSE;
        create_info.compareEnable = VK_FALSE;
        create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        VkResult result = vkCreateSampler(device, &create_info, NULL, &sampler);
        if (result != VK_SUCCESS) fatal("Failed to create texture sampler");
    }

    Vgk_TextureBundle texture_bundle = {};
    texture_bundle.image = image;
    texture_bundle.memory = memory;
    texture_bundle.image_view = image_view;
    texture_bundle.sampler = sampler;
    texture_bundle.format = format;
    return texture_bundle;
}

Vgk_DescriptorPoolBundle vgk_create_descriptor_pool_bundle(VkDevice device)
{
    Vgk_DescriptorPoolBundle bundle = {};
    bundle.max_sets = MAX_DESCRIPTOR_SETS_IN_POOL;
    bundle.max_uniform_buffers = MAX_UNIFORM_BUFFERS_IN_POOL;
    bundle.max_image_samplers = MAX_IMAGE_SAMPLERS_IN_POOL;

    VkDescriptorPool descriptor_pool;
    {
        VkDescriptorPoolSize descriptor_pool_sizes[2] = {};
        descriptor_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_pool_sizes[0].descriptorCount = bundle.max_uniform_buffers;
        descriptor_pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_pool_sizes[1].descriptorCount = bundle.max_image_samplers;

        VkDescriptorPoolCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        create_info.poolSizeCount = array_count(descriptor_pool_sizes);
        create_info.pPoolSizes = descriptor_pool_sizes;
        create_info.maxSets = bundle.max_sets;

        VkResult result = vkCreateDescriptorPool(device, &create_info, NULL, &descriptor_pool);
        if (result != VK_SUCCESS) fatal("Failed to create descriptor pool");
    }
    bundle.descriptor_pool = descriptor_pool;

    return bundle;
}

void vgk_check_descriptor_pool_availability(Vgk_DescriptorPoolBundle *descriptor_pool_bundle, const Vgk_DescriptorSetDescription *description)
{
    for (u32 i = 0; i < description->binding_count; i++)
    {
        const Vgk_DescriptorBinding *binding_ref = &description->bindings[i];
        switch (binding_ref->descriptor_type)
        {
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            {
                u32 new_count = descriptor_pool_bundle->uniform_buffer_count + binding_ref->descriptor_count;
                bassert(new_count < descriptor_pool_bundle->max_uniform_buffers);
                descriptor_pool_bundle->uniform_buffer_count = new_count;
            }
            break;

            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            {
                u32 new_count = descriptor_pool_bundle->image_sampler_count + binding_ref->descriptor_count;
                bassert(new_count < descriptor_pool_bundle->max_image_samplers);
                descriptor_pool_bundle->image_sampler_count = new_count;
            } break;

            default: fatal("Descriptor type not implemented");
        }
    }
}

VkDescriptorSetLayout vgk_create_descriptor_set_layout_from_description(const Vgk_DescriptorSetDescription *description, VkDevice device)
{
    VkDescriptorSetLayout descriptor_set_layout;
    {
        VkDescriptorSetLayoutCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

        VkDescriptorSetLayoutBinding bindings[MAX_DESCRIPTOR_BINDINGS] = {};
        for (u32 i = 0; i < description->binding_count; i++)
        {
            const Vgk_DescriptorBinding *binding_ref = &description->bindings[i];
            bindings[i].binding = i;
            bindings[i].descriptorType = binding_ref->descriptor_type;
            bindings[i].descriptorCount = binding_ref->descriptor_count;
            bindings[i].stageFlags = binding_ref->stage_flags;
        }

        create_info.pBindings = bindings;
        create_info.bindingCount = description->binding_count;

        VkResult result = vkCreateDescriptorSetLayout(device, &create_info, NULL, &descriptor_set_layout);
        if (result != VK_SUCCESS) fatal("Failed to create descriptor set layout");
    }
    return descriptor_set_layout;
}

Vgk_DescriptorSetBundle vgk_create_descriptor_set_bundle(Vgk_DescriptorPoolBundle *descriptor_pool_bundle, const Vgk_DescriptorSetDescription *description, VkDevice device)
{
    bassert(description->binding_count < MAX_DESCRIPTOR_BINDINGS);

    Vgk_DescriptorSetBundle descriptor_set_bundle = {};
    descriptor_set_bundle.description = *description;
    
    vgk_check_descriptor_pool_availability(descriptor_pool_bundle, description);

    descriptor_set_bundle.layout = vgk_create_descriptor_set_layout_from_description(description, device);

    VkDescriptorSet descriptor_set;
    {
        VkDescriptorSetAllocateInfo allocate_info = {};
        allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocate_info.descriptorPool = descriptor_pool_bundle->descriptor_pool;
        allocate_info.descriptorSetCount = 1;
        allocate_info.pSetLayouts = &descriptor_set_bundle.layout;

        VkResult result = vkAllocateDescriptorSets(device, &allocate_info, &descriptor_set);
        if (result != VK_SUCCESS) fatal("Failed to allocate descriptor set");
    }
    descriptor_set_bundle.descriptor_set = descriptor_set;

    return descriptor_set_bundle;
}

VkPipelineLayout vgk_create_pipeline_layout_from_description(const Vgk_PipelineLayoutDescription *description, VkDevice device)
{
    VkPipelineLayout pipeline_layout;
    {
        VkDescriptorSetLayout descriptor_set_layouts[MAX_DESCRIPTOR_SETS] = {};
        for (u32 i = 0; i < description->descriptor_set_count; i++)
        {
            descriptor_set_layouts[i] = vgk_create_descriptor_set_layout_from_description(&description->descriptor_sets[i], device);
        }
        VkPipelineLayoutCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        create_info.setLayoutCount = description->descriptor_set_count;
        create_info.pSetLayouts = descriptor_set_layouts;

        VkResult result = vkCreatePipelineLayout(device, &create_info, NULL, &pipeline_layout);
        if (result != VK_SUCCESS) fatal("Failed to create pipeline layout");

        for (u32 i = 0; i < description->descriptor_set_count; i++)
        {
            vkDestroyDescriptorSetLayout(device, descriptor_set_layouts[i], NULL);
        }
    }

    return pipeline_layout;
}

Vgk_PipelineBundle vgk_create_pipeline_from_description(const Vgk_PipelineDescription *description, VkDevice device)
{
    Vgk_PipelineBundle pipeline_bundle = {};
    pipeline_bundle.description = *description;

    pipeline_bundle.layout = vgk_create_pipeline_layout_from_description(&description->pipeline_layout_description, device);

    VkPipeline pipeline;
    {
        VkShaderModule vert_shader_module = vgk_create_shader_module(description->vert_shader_path, device);
        VkShaderModule frag_shader_module = vgk_create_shader_module(description->frag_shader_path, device);

        VkPipelineShaderStageCreateInfo shader_stages[2] = {};
        shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shader_stages[0].module = vert_shader_module;
        shader_stages[0].pName = "main";
        shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shader_stages[1].module = frag_shader_module;
        shader_stages[1].pName = "main";

        VkVertexInputBindingDescription vertex_input_binding_description = {};
        vertex_input_binding_description.binding = 0;
        vertex_input_binding_description.stride = description->vert_input_description.stride;
        vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription vert_attr_desc[MAX_VERT_ATTRIBUTES];
        for (u32 i = 0; i < description->vert_input_description.attribute_count; i++)
        {
            const Vgk_VertAttributeDescription *attr_ref = &description->vert_input_description.attributes[i];
            vert_attr_desc[i].location = i;
            vert_attr_desc[i].binding = 0;
            vert_attr_desc[i].format = attr_ref->format;
            vert_attr_desc[i].offset = attr_ref->offset;
        }

        VkPipelineVertexInputStateCreateInfo vertex_input_state = {};
        vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_state.vertexBindingDescriptionCount = 1;
        vertex_input_state.pVertexBindingDescriptions = &vertex_input_binding_description;
        vertex_input_state.vertexAttributeDescriptionCount = description->vert_input_description.attribute_count;
        vertex_input_state.pVertexAttributeDescriptions = vert_attr_desc;

        VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {};
        input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo viewport_state = {};
        viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state.viewportCount = 1;
        viewport_state.pViewports = &description->viewport;
        viewport_state.scissorCount = 1;
        viewport_state.pScissors = &description->scissor;

        VkPipelineRasterizationStateCreateInfo rasterization_state = {};
        rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization_state.polygonMode = description->polygon_mode;
        rasterization_state.lineWidth = 1.0f;
        rasterization_state.cullMode = description->cull_mode;
        rasterization_state.frontFace = description->front_face;

        VkPipelineMultisampleStateCreateInfo multisample_state = {};
        multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample_state.rasterizationSamples = description->rasterization_samples;

        VkPipelineColorBlendAttachmentState color_blend_attachment = {};
        color_blend_attachment.colorWriteMask = (VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);
        color_blend_attachment.blendEnable = description->enable_blending ? VK_TRUE : VK_FALSE;
        if (description->enable_blending)
        {
            color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
            color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
        }

        VkPipelineColorBlendStateCreateInfo color_blend_state = {};
        color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_state.attachmentCount = 1;
        color_blend_state.pAttachments = &color_blend_attachment;

        VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {};
        depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        if (description->enable_depth_testing)
        {
            depth_stencil_state.depthTestEnable = VK_TRUE;
            depth_stencil_state.depthWriteEnable = VK_TRUE;
            depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS;
            depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
            depth_stencil_state.stencilTestEnable = VK_FALSE;
        }

        VkGraphicsPipelineCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        create_info.stageCount = array_count(shader_stages);
        create_info.pStages = shader_stages;
        create_info.pVertexInputState = &vertex_input_state;
        create_info.pInputAssemblyState = &input_assembly_state;
        create_info.pViewportState = &viewport_state;
        create_info.pRasterizationState = &rasterization_state;
        create_info.pMultisampleState = &multisample_state;
        create_info.pColorBlendState = &color_blend_state;
        create_info.pDepthStencilState = &depth_stencil_state;
        create_info.layout = pipeline_bundle.layout;
        create_info.renderPass = description->render_pass;
        create_info.subpass = 0;
        
        VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &create_info, NULL, &pipeline);
        if (result != VK_SUCCESS) fatal("Failed to create graphics pipeline");

        vkDestroyShaderModule(device, vert_shader_module, NULL);
        vkDestroyShaderModule(device, frag_shader_module, NULL);
    }
    pipeline_bundle.pipeline = pipeline;

    return pipeline_bundle;
}

// ==================== DESTROY =================================

void vgk_destroy_swapchain_bundle(Vgk_SwapchainBundle *bundle, VkDevice device)
{
    free(bundle->images);
    for (u32 i = 0; i < bundle->image_count; i++)
    {
        vkDestroySemaphore(device, bundle->submit_semaphores[i], NULL);
        vkDestroyImageView(device, bundle->image_views[i], NULL);
    }
    free(bundle->submit_semaphores);
    free(bundle->image_views);
    vkDestroySwapchainKHR(device, bundle->swapchain, NULL);
    *bundle = (Vgk_SwapchainBundle){};
}

void vgk_destroy_depth_image_bundle(Vgk_DepthImageBundle *bundle, VkDevice device)
{
    for (u32 i = 0; i < bundle->image_count; i++)
    {
        vkDestroyImage(device, bundle->images[i], NULL);
        vkFreeMemory(device, bundle->memory_list[i], NULL);
        vkDestroyImageView(device, bundle->image_views[i], NULL);
    }
    free(bundle->images);
    free(bundle->memory_list);
    free(bundle->image_views);
    *bundle = (Vgk_DepthImageBundle){};
}

void vgk_destroy_render_pass_bundle(Vgk_RenderPassBundle *bundle, VkDevice device)
{
    for (u32 i = 0; i < bundle->framebuffer_count; i++)
    {
        vkDestroyFramebuffer(device, bundle->framebuffers[i], NULL);
    }
    free(bundle->framebuffers);
    vkDestroyRenderPass(device, bundle->render_pass, NULL);
    *bundle = (Vgk_RenderPassBundle){};
}

void vgk_destroy_command_pool(VkCommandPool *command_pool, VkDevice device)
{
    vkDestroyCommandPool(device, *command_pool, NULL);
    *command_pool = VK_NULL_HANDLE;
}

void vgk_destroy_frame_list(Vgk_FrameList *list, VkDevice device)
{
    for (u32 i = 0; i < list->count; i++)
    {
        vkDestroySemaphore(device, list->frames[i].acquire_semaphore, NULL);
        vkDestroyFence(device, list->frames[i].in_flight_fence, NULL);
    }

    free(list->frames);

    *list = (Vgk_FrameList){};
}

void vgk_destroy_buffer_bundle(Vgk_BufferBundle *bundle, VkDevice device)
{
    vkUnmapMemory(device, bundle->memory);
    vkFreeMemory(device, bundle->memory, NULL);
    vkDestroyBuffer(device, bundle->buffer, NULL);
    *bundle = (Vgk_BufferBundle){};
}

void vgk_destroy_buffer_bundle_list(Vgk_BufferBundleList *list, VkDevice device)
{
    for (u32 i = 0; i < list->count; i++)
    {
        vgk_destroy_buffer_bundle(&list->buffer_bundles[i], device);
    }
    free(list->buffer_bundles);
    *list = (Vgk_BufferBundleList){};
}

void vgk_destroy_texture_bundle(Vgk_TextureBundle *bundle, VkDevice device)
{
    vkDestroyImage(device, bundle->image, NULL);
    vkFreeMemory(device, bundle->memory, NULL);
    vkDestroyImageView(device, bundle->image_view, NULL);
    vkDestroySampler(device, bundle->sampler, NULL);
}

// ============================ HELPERS ===============================

u32 vgk_get_queue_family_index(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
    u32 queue_family_index = UINT32_MAX;
    {
        u32 count;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, NULL);
        VkQueueFamilyProperties *queue_families = (VkQueueFamilyProperties *)xmalloc(count * sizeof(queue_families[0]));
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, queue_families);
        for (u32 i = 0; i < count; i++)
        {
            VkBool32 present_support;
            vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
            if ((queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present_support)
            {
                queue_family_index = i;
            }
        }
        free(queue_families);
        assert(queue_family_index < UINT32_MAX);
    }
    return queue_family_index;
}

u32 vgk_find_memory_type(VkPhysicalDevice physical_device, u32 type_filter, VkMemoryPropertyFlags props)
{
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_props);
    for (u32 i = 0; i < mem_props.memoryTypeCount; i++)
    {
        if ((type_filter & (1 << i)) &&
            (mem_props.memoryTypes[i].propertyFlags & props) == props)
        {
            return i;
        }
    }
    fatal("Failed to find suitable memory type");
    return 0;
}

VkViewport vgk_get_viewport_for_extent(VkExtent2D extent)
{
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = extent.width;
    viewport.height = extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    return viewport;
}

VkRect2D vgk_get_scissor_for_extent(VkExtent2D extent)
{
    VkRect2D scissor = {};
    scissor.offset.x = 0.0f;
    scissor.offset.y = 0.0f;
    scissor.extent = extent;
    return scissor;
}
