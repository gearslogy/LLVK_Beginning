#version 460 core

#include "common.glsl"
#include "noise.glsl"
layout(location=0) in vec2 uv;
layout(location=1) in vec3 N;
layout(location=2) in vec3 T;
layout(location=3) in vec3 B;
layout(location=4) in vec3 wP;
layout(location=0) out vec4 Cd;


/*
// LAYER1:
0 road
1 stone
2 dirt
4 grass

// LAYER2:
0 : brick0
1 : Stone Wal

*/
layout(set=1, binding=0) uniform sampler2DArray diff;
layout(set=1, binding=1) uniform sampler2DArray nor;
layout(set=1, binding=2) uniform sampler2D indexMap;
layout(set=1, binding=3) uniform sampler2D extUV;

float fn_zero_or_one(float data){
    return float(data>0.00001);
}
vec2 fn_index_rg_mask(vec4 index_data){
    return vec2(fn_zero_or_one(index_data.r), fn_zero_or_one(index_data.g));
}
void gen_noise_index(inout vec4[4] index_data, inout vec2 noise_data[4]){
    float frequency = 20;
    float amp = 0.01;
    vec2 noise1 = noise22((wP.xz-14) * frequency) * amp;
    vec2 noise2 = noise22((wP.xz+41) * frequency) * amp;
    vec2 noise3 = noise22((wP.xz+12.158) * frequency) * amp;
    vec2 noise4 = noise22((wP.xz+137) * frequency) * amp;
    // 3 samplers
    vec4 index1 = texture(indexMap, uv + noise1);  // point-sampler index, linear space
    vec4 index2 = texture(indexMap, uv + noise2);  // point-sampler index, linear space
    vec4 index3 = texture(indexMap, uv + noise3);  // point-sampler index, linear space
    vec4 index4 = texture(indexMap, uv + noise4);  // point-sampler index, linear space
    index_data[0] = index1;
    index_data[1] = index2;
    index_data[2] = index3;
    index_data[3] = index4;

    noise_data[0] = noise1;
    noise_data[1] = noise2;
    noise_data[2] = noise3;
    noise_data[3] = noise4;
}



vec3 reconstructNormal(vec2 NXY){

    // 将 [0,1] 范围的值重映射到 [-1,1]
    vec2 fenc = NXY * 2.0 - 1.0;
    // 重建 Z 分量
    float z = sqrt(1.0 - dot(fenc, fenc));
    // 构建完整的法线向量
    return normalize(vec3(fenc.x, fenc.y, z));

    /*
    // 计算Z分量
    float normalZ = sqrt(1.0 - dot(NXY, NXY));

    // 重建法线向量
    vec3 normal = vec3(NXY, normalZ);

    // 调整法线范围
    normal = normalize(normal * 2.0 - 1.0);

    return normal;*/
}



// index.R: first 4 layer
// index.G: Next 2 layer
vec3 fn_compute_layer1_uv(vec4 index){
    vec2 road_uv_mult = vec2(1,1);
    vec2 ext_uv = texture(extUV, uv).xy; // sampler ext uv, linear space
    int layer1_index =  int( round(index.r * 4 )) -1 ; // first layer 4 layer : 0-3 ID
    vec3 layer1_uv = vec3(uv * 10 , layer1_index);
    vec3 layer1_ext_uv = vec3(ext_uv * road_uv_mult, layer1_index);
    layer1_uv = mix(layer1_uv , layer1_ext_uv, float(layer1_index < 0.1) );  // if it's  road layer
    return layer1_uv;
}
vec3 fn_compute_layer2_uv(vec4 index){
    int layer2_index = int(round(index.g * 2)) + 3;  // 4-5 ID
    vec3 layer2_uv = vec3(uv * 5, layer2_index);
    return layer2_uv;
}

void fn_sampler_Cd_N_D_R(
                        in vec4 index_point_data,
                        in vec3 layer_uvs[2],
                        inout vec3 color[2], // LAYER1 AND LAYER2 color
                        inout vec3 normal[2],// LAYER1 AND LAYER2 N
                        inout float height[2],
                        inout float rough[2] ){

    // color
    vec2 index_rg_mask = fn_index_rg_mask(index_point_data);
    color[0] = texture(diff, layer_uvs[0]).rgb * index_rg_mask.r;
    color[1] = texture(diff, layer_uvs[1]).rgb * index_rg_mask.g;


    vec4 layer1_ndr = texture(nor, layer_uvs[0]) * index_rg_mask.r;
    vec4 layer2_ndr = texture(nor, layer_uvs[1]) * index_rg_mask.g;

    vec3 layer1_N = vec3(layer1_ndr.rg,0);
    vec3 layer2_N = vec3(layer2_ndr.rg,0);

    normal[0] = layer1_N;
    normal[1] = layer2_N;
    // displacement
    height[0] = layer1_ndr.b;
    height[1] = layer2_ndr.b;
    // roughness
    rough[0] = layer1_ndr.a;
    rough[1] = layer2_ndr.a;
}

// splatControl is mask area
vec3 heightBlend(vec3 layer1Cd , vec3 layer2Cd, vec2 splatControl, float height1, float height2, float heightTransation){
    vec2 splatHeight;
    splatHeight.x = height1 * splatControl.r;
    splatHeight.y = height2 * splatControl.g;

    float maxHeight = max(height1, height2);
    float transition = max(heightTransation, 1e-5); // 确保过度不为0
    vec2 weightedHeights = splatHeight + transition - maxHeight;
    weightedHeights = max( weightedHeights,  0);
    weightedHeights = (weightedHeights + 1e-6) * splatControl ;

    float sumHeight = max(dot(weightedHeights, vec2(1, 1)), 1e-6) ;
    vec2 finalControl = weightedHeights / sumHeight ;

    return layer1Cd* finalControl.r +layer2Cd * finalControl.g;
}

// cal hb cd

void computeHBData(vec4 index_point_data, vec3 layer_uvs[2],
            inout vec3 outColor,
            inout vec3 outN,
            inout float outMaxHeight){
    // Get the color/N/height/roughness from sampler
    vec3 colors[2];
    vec3 normals[2];
    float heights[2];
    float roughnesses[2];
    fn_sampler_Cd_N_D_R(index_point_data,layer_uvs, colors, normals, heights, roughnesses );
    //
    vec2 index_rg_mask = fn_index_rg_mask(index_point_data);
    // height blend two layers
    float newHeight1 = heights[0];
    float newHeight2 =heights[1] ;
    vec3 hbCd = heightBlend(colors[0],colors[1],  index_rg_mask, newHeight1, newHeight2, 0.1);
    vec3 hbN = heightBlend(normals[0],normals[1],  index_rg_mask, newHeight1, newHeight2, 0.1);
    // OUT RESULRT
    outMaxHeight = max(newHeight1,newHeight2 );
    outColor= hbCd;
    outN = hbN;
}


vec3 reduceHBArrayData( vec3 twoLayerHbColors[4], float twoLayerHbHeights[4] ){
    // blend 0-1
    vec3 mix01_Cd = heightBlend(twoLayerHbColors[0], twoLayerHbColors[1], vec2(1,1), twoLayerHbHeights[0],  twoLayerHbHeights[1], 0.02 );
    float height01 = max( twoLayerHbHeights[0].x,  twoLayerHbHeights[1]);
    // blend 01 ---> 2
    vec3 mix01_2_Cd = heightBlend(mix01_Cd, twoLayerHbColors[2], vec2(1,1),  height01,  twoLayerHbHeights[2], 0.02 );
    float height01_2 = max(height01,twoLayerHbHeights[2]);
    vec3 mix01_2_3_Cd = heightBlend(mix01_2_Cd, twoLayerHbColors[3], vec2(1,1), height01_2, twoLayerHbHeights[3], 0.02);
    // blend 01-2 ---> 3
    vec3 aaColor1 = (twoLayerHbColors[0] + twoLayerHbColors[1] + twoLayerHbColors[2] + twoLayerHbColors[3] )*0.25; // avage blend all
    vec3 aaColor2 = (mix01_Cd + mix01_2_Cd + mix01_2_3_Cd ) * 0.3333; // height blend avage
    vec3 aaColor = mix(aaColor1, aaColor2, 0.5);
    return aaColor;
}


void main(){

    vec4 point_indices[4];
    vec2 noise_data   [4];
    gen_noise_index(point_indices, noise_data);

    // two layer height blend
    vec3 twoLayerHbColors[4];
    vec3 twoLayerHbNormals[4];
    float twoLayerHbHeights[4];
    for(int i=0;i< 4 ;i++ ){
        vec4 index_point_data = point_indices[i];
        vec3 layer_uvs[2] ;
        layer_uvs[0] = fn_compute_layer1_uv(index_point_data);
        layer_uvs[1] = fn_compute_layer2_uv(index_point_data);
        // RESULT CALCULATED
        vec3 hbCd;
        vec3 hbN;
        float maxHeight;
        // RESULT CALCULATED
        computeHBData(index_point_data, layer_uvs, hbCd, hbN, maxHeight);
        twoLayerHbColors[i] = hbCd;
        twoLayerHbHeights[i] = maxHeight;
        twoLayerHbNormals[i] = hbN;
    }

    vec3 aaColor = reduceHBArrayData(twoLayerHbColors, twoLayerHbHeights);
    vec3 aaTangentNormal  = reduceHBArrayData(twoLayerHbNormals, twoLayerHbHeights);
    aaTangentNormal = reconstructNormal(aaTangentNormal.xy);

    mat3 TBN = mat3(T, B, N);
    vec3 wN = normalize(TBN * aaTangentNormal);
    float light = dot(wN , normalize(vec3(1,1,0)) );
    light = max(light, 0);
    Cd = vec4(aaColor * light,0 );

    //Cd = vec4(mix01_2,0) ;
    //Cd = vec4((mix01_2 + mix01)/2 , 0);
    //Cd =vec4(aaColor,0);
    Cd = vec4(gammaCorrect(Cd.rgb,2.2).rgb , 0);
    //Cd = vec4(twoLayerHbHeights[0],twoLayerHbHeights[1],twoLayerHbHeights[2],1);
    //Cd = vec4(ext_uv, 0,0);
    //Cd = vec4(layer2_N , 0);
    //Cd = index;
    //Cd = noise;
    //Cd = vec4(FBM_1_3(wP * 20, 4, 0.1, 0.2));
}