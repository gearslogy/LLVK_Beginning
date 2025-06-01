
const mat4 biasMat = mat4(
0.5, 0.0, 0.0, 0.0,
0.0, 0.5, 0.0, 0.0,
0.0, 0.0, 1.0, 0.0,
0.5, 0.5, 0.0, 1.0
);

float textureProj(sampler2D shadowMap, vec4 shadow_coord, vec2 off)
{
    float bias =  0.000004;
    vec4 shadowCoord = shadow_coord / shadow_coord.w;
    float shadow = 1.0;
    if ( shadowCoord.z > 0 && shadowCoord.z < 1.0 ) //没必要判断.z>-1
    {
        float closeDepth = texture( shadowMap, shadowCoord.st + off ).r;
        float currentDepth = shadowCoord.z;
        if ( shadowCoord.w > 0.0 && closeDepth < (currentDepth - bias ) ){
            shadow = 0;
        }
    }
    return shadow;
}
float filterPCF(sampler2D shadowMap, vec4 sc)
{
    ivec2 texDim = textureSize(shadowMap, 0); // MIP = 0
    float scale = 2;
    float dx = scale * 1.0 / float(texDim.x);
    float dy = scale * 1.0 / float(texDim.y);

    float shadowFactor = 0.0;
    int count = 0;
    int range = 5;

    for (int x = -range; x <= range; x++)
    {
        for (int y = -range; y <= range; y++)
        {
            shadowFactor += textureProj(shadowMap, sc, vec2(dx*x, dy*y));
            count++;
        }
    }
    return shadowFactor / count;
}
