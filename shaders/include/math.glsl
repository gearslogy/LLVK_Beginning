// Rotation around the X axis (3x3 matrix)
/*
mat3 rotate_x(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat3(
    1.0, 0.0, 0.0,
    0.0, c, -s,
    0.0, s, c
    );
}
*/
mat3 rotate_x(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat3(
    1.0, 0.0, 0.0,
    0.0, c, s,
    0.0, -s, c
    );
}

// Rotation around the Y axis (3x3 matrix)
/*
mat3 rotate_y(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat3(
    c, 0.0, s,
    0.0, 1.0, 0.0,
    -s, 0.0, c
    );
}*/

mat3 rotate_y(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat3(
    c, 0.0, -s,
    0.0, 1.0, 0.0,
    s, 0.0, c
    );
}

// Rotation around the Z axis (3x3 matrix)
/*
mat3 rotate_z(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat3(
    c, -s, 0.0,
    s, c, 0.0,
    0.0, 0.0, 1.0
    );
}
*/
mat3 rotate_z(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat3(
    c, s, 0.0,
    -s, c, 0.0,
    0.0, 0.0, 1.0
    );
}



mat3 scale(vec3 axis_scale){
    mat3 ret = mat3(
        axis_scale.x, 0.0, 0.0,
        0.0, axis_scale.y, 0.0,
        0.0, 0.0, axis_scale.z
    );
    return ret;
}


mat4 rotate(vec3 axis, float angle)
{
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    //Rodrigues' rotation formula
    return mat4(
    oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
    oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
    oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
    0.0,                                0.0,                                0.0,                                1.0
    );
}

#define PI 3.14159265358979323846

mat3 right_hand_eulerAnglesToRotationMatrix(vec3 angles)
{
    // 将角度转换为弧度
    vec3 radAngles = radians(angles);
    float cx = cos(radAngles.x);
    float sx = sin(radAngles.x);
    float cy = cos(radAngles.y);
    float sy = sin(radAngles.y);
    float cz = cos(radAngles.z);
    float sz = sin(radAngles.z);

    // 创建旋转矩阵
    mat3 rotX = mat3(1.0, 0.0, 0.0,
    0.0, cx, sx,
    0.0, -sx, cx);

    mat3 rotY = mat3(cy, 0.0, -sy,
    0.0, 1.0, 0.0,
    sy, 0.0, cy);

    mat3 rotZ = mat3(cz, sz, 0.0,
    -sz, cz, 0.0,
    0.0, 0.0, 1.0);

    // 按照 Z * Y * X 的顺序组合旋转矩阵
    return rotZ * rotY * rotX;
}

vec3 rotateVectorByQuat(vec3 v, vec4 quat) {
    // 提取四元数的分量
    float w = quat.w;
    float x = quat.x;
    float y = quat.y;
    float z = quat.z;

    // 计算四元数乘积
    vec3 t = 2.0 * cross(vec3(x, y, z), v);
    vec3 rotatedVector = v + w * t + cross(vec3(x, y, z), t);

    return rotatedVector;
}

mat3 normal_matrix(mat4 model_matrix){
    mat3 normalMatrix = mat3(model_matrix);
    normalMatrix = transpose(inverse(normalMatrix));
    return normalMatrix;
}
mat3 normal_matrix(mat3 model_matrix){
    return transpose(inverse(model_matrix));
}

