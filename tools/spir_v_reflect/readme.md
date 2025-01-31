
dump this glsl: spirv_reflect_vert.vert
```glsl
#version 460 core
#include "math.glsl"


layout (constant_id = 0) const int myConstant0 = 0;
layout (constant_id = 1) const float myConstant1 = 1.0;

layout(push_constant) uniform PushConstantsVertex {
    float P_xOffset;
    float P_yOffset;
    float P_zOffset;
    float P_wOffset;
} myPushConstantData;


layout(location=0) in vec3 P;
layout(location=1) in vec3 Cd;
layout(location=2) in vec3 N;
layout(location=3) in vec3 T;
layout(location=4) in vec2 uv0;
layout(location=5) in int fracture_idx;


layout(location = 0) out vec3 fragN;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragCd;
layout(location = 3) out vec3 fragVAT_P;
layout(location = 4) out vec4 fragVAT_orient;

layout(set=0, binding=0) uniform UBO{
    mat4 proj;
    mat4 view;
    mat4 model;
    vec4 metaInfo; // x for frame,y is num fractures
}ubo;

layout(set=0, binding=1) uniform UBO2{
    vec4 time;
    vec4 color;
}ubo2;
layout(set=0, binding=2) uniform UBO3{
    vec4 data;
    vec4 noise;
}ubo3;

struct RBDData{
    vec4 rbdP;
    vec4 rbdOrient;
};

struct Light {
    vec4 position;
    vec3 color;
    float radius;
};
layout (set=0, binding = 3) uniform UBO4
{
    Light lights[10];
    vec4 viewPos;
} ubo4;

layout(set=1, binding=1) buffer SSBO{
    RBDData data[];  // numRBDS * numFrames
}ssbo;

layout(set=1, binding=2) uniform sampler2D testTex1;
layout(set=1, binding=3) uniform sampler2D testTex2;

```

cmd: spirv_reflection.exe D:\plugin_dev\cpp\LLVK_Beginning\build\bin\shaders\spirv_reflect_vert.spv

```
shader stage:VK_SHADER_STAGE_VERTEX_BIT
entry points count:1
main
descriptor set count: 2
>>>>  set id:0, binding count:4
        ----ubo, binding:0, descCount:1 type:VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, accessed:true----
        <<block begin ->name:ubo typename:UBO glsl_t:struct size:208 padded_size:208>>
        {
        name:proj, type:<not struct> abs_offset:0 ral_offset:0 size:64 padded_size:0 array_stride:64 var_flags:SPV_REFLECT_VARIABLE_FLAGS_NONE is_struct:false is_array:false is_array_struct:false sub_member_count:0
        name:view, type:<not struct> abs_offset:64 ral_offset:64 size:64 padded_size:0 array_stride:64 var_flags:SPV_REFLECT_VARIABLE_FLAGS_NONE is_struct:false is_array:false is_array_struct:false sub_member_count:0
        name:model, type:<not struct> abs_offset:128 ral_offset:128 size:64 padded_size:0 array_stride:64 var_flags:SPV_REFLECT_VARIABLE_FLAGS_NONE is_struct:false is_array:false is_array_struct:false sub_member_count:0
        name:metaInfo, type:<not struct> abs_offset:192 ral_offset:192 size:16 padded_size:0 array_stride:16 var_flags:SPV_REFLECT_VARIABLE_FLAGS_NONE is_struct:false is_array:false is_array_struct:false sub_member_count:0
        }
        ----ubo2, binding:1, descCount:1 type:VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, accessed:false----
        <<block begin ->name:ubo2 typename:UBO2 glsl_t:struct size:32 padded_size:32>>
        {
        name:time, type:<not struct> abs_offset:0 ral_offset:0 size:16 padded_size:0 array_stride:16 var_flags:SPV_REFLECT_VARIABLE_FLAGS_UNUSED is_struct:false is_array:false is_array_struct:false sub_member_count:0
        name:color, type:<not struct> abs_offset:16 ral_offset:16 size:16 padded_size:0 array_stride:16 var_flags:SPV_REFLECT_VARIABLE_FLAGS_UNUSED is_struct:false is_array:false is_array_struct:false sub_member_count:0
        }
        ----ubo3, binding:2, descCount:1 type:VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, accessed:false----
        <<block begin ->name:ubo3 typename:UBO3 glsl_t:struct size:32 padded_size:32>>
        {
        name:data, type:<not struct> abs_offset:0 ral_offset:0 size:16 padded_size:0 array_stride:16 var_flags:SPV_REFLECT_VARIABLE_FLAGS_UNUSED is_struct:false is_array:false is_array_struct:false sub_member_count:0
        name:noise, type:<not struct> abs_offset:16 ral_offset:16 size:16 padded_size:0 array_stride:16 var_flags:SPV_REFLECT_VARIABLE_FLAGS_UNUSED is_struct:false is_array:false is_array_struct:false sub_member_count:0
        }
        ----ubo4, binding:3, descCount:1 type:VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, accessed:false----
        <<block begin ->name:ubo4 typename:UBO4 glsl_t:struct size:336 padded_size:336>>
        {
        name:lights, type:Light abs_offset:0 ral_offset:0 size:320 padded_size:32 array_stride:320 var_flags:SPV_REFLECT_VARIABLE_FLAGS_UNUSED is_struct:true is_array:true is_array_struct:true sub_member_count:3 array-dim:1  [0]=10
                <<block begin ->name:position typename:<unnamed> glsl_t:vec4 size:16 padded_size:16>>
                <<block begin ->name:color typename:<unnamed> glsl_t:vec3 size:12 padded_size:12>>
                <<block begin ->name:radius typename:<unnamed> glsl_t:float size:4 padded_size:4>>
        name:viewPos, type:<not struct> abs_offset:320 ral_offset:320 size:16 padded_size:0 array_stride:16 var_flags:SPV_REFLECT_VARIABLE_FLAGS_UNUSED is_struct:false is_array:false is_array_struct:false sub_member_count:0
        }
>>>>  set id:1, binding count:3
        ----ssbo, binding:1, descCount:1 type:VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, accessed:true----
        <<block begin ->name:ssbo typename:SSBO glsl_t:struct size:0 padded_size:0>>
        {
        name:data, type:RBDData abs_offset:0 ral_offset:0 size:32 padded_size:0 array_stride:32 var_flags:SPV_REFLECT_VARIABLE_FLAGS_NONE is_struct:true is_array:true is_array_struct:true sub_member_count:2 array-dim:0
                <<block begin ->name:rbdP typename:<unnamed> glsl_t:vec4 size:16 padded_size:16>>
                <<block begin ->name:rbdOrient typename:<unnamed> glsl_t:vec4 size:16 padded_size:16>>
        }
        ----testTex1, binding:2, descCount:1 type:VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, accessed:false----
        ----testTex2, binding:3, descCount:1 type:VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, accessed:false----
```

