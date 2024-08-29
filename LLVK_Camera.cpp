//
// Created by liuya on 8/18/2024.
//

#include "LLVK_Camera.h"

#include <iostream>
LLVK_NAMESPACE_BEGIN
    glm::mat4 Camera::view() const {
    return glm::lookAt(mPosition, mPosition + mFront, mUp);
}
glm::mat4 Camera::projection() const {
    return glm::perspective(glm::radians(mZoom), mAspect, mNear, mFar);
}

void Camera::processKeyboard(Camera_Movement direction, float deltaTime) {
    float vel = mMoveSpeed * deltaTime;
    switch (direction) {
        case Camera_Movement::FORWARD:
            mPosition += mFront * vel;
            break;
        case Camera_Movement::BACKWARD:
            mPosition -= mFront * vel;
            break;
        case Camera_Movement::LEFT: {
            mPosition -= mRight  * vel;
            break;
        }
        case Camera_Movement::RIGHT: {
            mPosition += mRight  * vel;
            break;
        }
        default:
            break;
    }
}
// mouse rotation
void Camera::processMouseMovement(float xoffset, float yoffset) {
    xoffset *= mMouseSensitivity;
    yoffset *= mMouseSensitivity;

    mYaw += xoffset;
    mPitch += yoffset;

    if (mPitch > 89.0f)
        mPitch = 89.0f;
    if (mPitch < -89.0f)
        mPitch = -89.0f;
    updateCameraVectors();
}


void Camera::updateCameraVectors() {
    // calculate the new Front vector
    //std::cerr << "mYaw"<< mYaw << " pitch:" << mPitch << std::endl;
    glm::vec3 front;
    front.x = cos(glm::radians(mYaw)) * cos(glm::radians(mPitch));
    front.y = sin(glm::radians(mPitch));
    front.z = sin(glm::radians(mYaw)) * cos(glm::radians(mPitch));
    mFront = glm::normalize(front);
    // also re-calculate the Right and Up vector
    mRight = glm::normalize(glm::cross(mFront, glm::vec3{0,1,0}));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    mUp    = glm::normalize(glm::cross(mRight, mFront));
}


void Camera::processMouseScroll(float yoffset){
    mMoveSpeed += yoffset; // only change movement speed
}




LLVK_NAMESPACE_END