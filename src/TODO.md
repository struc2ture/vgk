# TODO

* Vgk_ImageBundle -- for textures
    - image + memory
    - parallel to buffer + memory
* Function for copying image from staging buffer to device-local image
* Separate descriptor set layout from pipeline bundle
* Pipeline creation: unhardcode shader stage composition
* Pipeline creation: unhardcode blending configuration

# DOING

* Pipeline creation from description -- only interface

# DONE

* Destroy functions

# MISC

* Rename "Description" to something else?

* Idea for descriptor set <-> pipeline API.
    * create descriptor sets separately, will keepa a reference to its layout 
    * pass descriptor sets when creating pipelines, will get its descriptor set layout for creation of pipeline layout
    
    
    * LATER: each pipeline "subscribes" to a descriptor set, e.g. global -- all the transforms, etc.

    * LATER: Is there an issue with lifetime of descriptor sets? Specifically them being copied into pipeline description. For now it should be ok, as I won't be recreating description sets. Later, maybe I need some kind of central storage for all these items. OR: maybe just keep the descriptor set description in the pipeline description, not the actual descriptor set/layout VK handles.

* Pipeline bundle
    * Shaders -- code -> spv binaries -> spv bytes
    * Pipeline layout
        * Descriptor set layout
            * Define bindings for each descriptor
    * Descriptor pool
        * Max size to be able to allocate every descriptor.
    * Descriptor sets
        * One descriptor set per frame in flight
        * For every descriptor set -- update every descriptor binding to point to the right resource
    * Pipeline
        * Shader stages
        * Define vertex input state:
            * Stride
            * Offsets and formats for all attributes
        * Viewport, scissor
        * Culling
        * Multisampling
        * Blending

* How to define a pipeline
    * Shader paths
    * Descriptors
    * Vertex input
    * Viewport, scissor
    * Culling
    * Multisampling
    * Blending

* Separation:
    * What's bound separately in command buffer:
        * Render pass
        * Pipeline
        * Vertex buffer
        * Index buffer
        * Descriptor sets

* Can bind multiple descriptor sets. So create global descriptor sets that are bound for each pipeline. E.g. descriptor set 0 -- global UBO

```
// during init
Pipeline p1 = begin_pipeline_description();
add_shader_stage(&p1, VERT, "path/to/shader.vert");
add_shader_stage(&p1, FRAG, "path/to/shader.frag");
set_vert_stride(&p1, sizeof(Vert));
add_vert_attribute(&p1, offsetof(Vert, pos), FORMAT);
add_vert_attribute(&p1, offsetof(Vert, uv), FORMAT);
add_descriptor(&p1, mvp_ubo, mvp_ubo_buffer);
add_descriptor(&p1, tex1, tex_image1);
add_descriptor(&p1, tex2, tex_image2);
set_viewport(&p1, viewport, scissor);
set_culling(&p1, CULLING);
set_sampling(&p1, SAMPLING);
set_blending(&p1, BLENDING);
set_render_pass(&p1, 2d_render_pass);
end_pipeline_description(&p1);

// during frame's rendering
begin_render_pass(2d_render_pass);
bind_pipeline(&p1);
update_buffer(&p1->vert_buffer, vert_data);
update_buffer(&p1->index_buffer, index_data);
update_buffer(&mvp_ubo_buffer, mvp_ubo_data);
draw_elements(&p1, index_count);
end_render_pass(2d_render_pass);

```