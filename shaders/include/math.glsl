// Rotation around the X axis (3x3 matrix)

mat3 rotate_x(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat3(
    1.0, 0.0, 0.0,
    0.0, c, -s,
    0.0, s, c
    );
}

// Rotation around the Y axis (3x3 matrix)
mat3 rotate_y(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat3(
    c, 0.0, s,
    0.0, 1.0, 0.0,
    -s, 0.0, c
    );
}

// Rotation around the Z axis (3x3 matrix)
mat3 rotate_z(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat3(
    c, -s, 0.0,
    s, c, 0.0,
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

mat3 normal_matrix(mat4 model_matrix){
    return mat3(transpose(inverse(model_matrix)));
}
mat3 normal_matrix(mat3 model_matrix){
    return transpose(inverse(model_matrix));
}

