cmd: spirv_reflection.exe D:\plugin_dev\cpp\LLVK_Beginning\build\bin\shaders\spirv_reflect_vert.spv

```
shader stage:VK_SHADER_STAGE_VERTEX_BIT
entry points count:1
main
descriptor set count: 2
-set id:0
-binding count:3
        ----ubo, binding:0, descCount:1 VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, accessed:true----
        type:UBO
        members cout :4
                proj
                view
                model
                metaInfo
        ----ubo2, binding:1, descCount:1 VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, accessed:false----
        type:UBO2
        members cout :2
                time
                color
        ----ubo3, binding:2, descCount:1 VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, accessed:false----
        type:UBO3
        members cout :2
                data
                noise
-set id:1
-binding count:3
        ----ssbo, binding:1, descCount:1 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, accessed:true----
        type:SSBO
        members cout :1
                data
        ----testTex1, binding:2, descCount:1 VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, accessed:false----
        members cout :0
        ----testTex2, binding:3, descCount:1 VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, accessed:false----
        members cout :0
```

