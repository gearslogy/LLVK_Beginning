const float PI = 3.14159265359;
vec3 getNormalFromMap(vec3 fragNormal,
                      vec3 tangentNormal,
                      vec3 fragPosition,
                      vec2 fragTexCoord) {
    // 假设已知相邻像素坐标和位置
    float dx = 0.0001;
    vec3 deltaPos1 = fragPosition - vec3(dx, 0.0, 0.0); // 你应该从邻近的顶点或片段中获得这些差异
    vec3 deltaPos2 = fragPosition - vec3(0.0, dx, 0.0);

    vec2 deltaUV1 = fragTexCoord - vec2(dx, 0.0); // 同样，这些差异应该来自邻近的顶点或片段
    vec2 deltaUV2 = fragTexCoord - vec2(0.0,dx);

    // 计算切线和副切线
    float r = 1.0 / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
    vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
    vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;

    // 规范化切线和副切线
    tangent = normalize(tangent);
    bitangent = normalize(bitangent);

    // 构建TBN矩阵
    mat3 TBN = mat3(tangent, bitangent, fragNormal);
    vec3 worldNormal = normalize(TBN * tangentNormal);
    return worldNormal;
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float calLight(vec3 N){
    vec3 lightDir= {1,1,0};
    lightDir = normalize(lightDir);
    return max(dot(lightDir, N),0.4);
}


float linearizeDepth(float depth, float near, float far)
{
    float z = depth * 2.0 - 1.0; // back to NDC
    return (2.0 * near * far) / (far + near - z * (far - near));
}



vec3 gammaCorrect(vec3 color, float gamma) {
    return pow(color, vec3(1.0 / gamma));
}
vec4 gammaCorrect(vec4 color, float gamma) {
    return pow(color, vec4(1.0 / gamma));
}

vec3 normalCorrect(vec3 N){
    return N * 2.0 -1.0;
}

// sample UBO scene
const vec3 lightColor = vec3(1,1,1)*4;
const vec3 cameraPosition= {0,80,200} ;
const vec3 lightPosition = {0,150,0};
const float lightRadius = 300;