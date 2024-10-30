//
// Created by liuya on 8/18/2024.
//

#ifndef LLVK_CAMERA_H
#define LLVK_CAMERA_H

#include "LLVK_SYS.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

LLVK_NAMESPACE_BEGIN


struct Camera {
    enum class Camera_Movement {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT
    };
    Camera() {
        updateCameraVectors();
    }

    // According to the algorithm verified by Houdini, the camera is assumed to be pointing in the negative z direction by default.
    void setRotation(glm::vec3 rot) ;

    glm::vec3 mPosition{0,0,0};
    glm::vec3 mFront{0,0,-1};
    glm::vec3 mUp{0,1,0};
    glm::vec3 mWorldsUp{0,1,0};
    glm::vec3 mRight{0,0,1};

    float mAspect{16.0f/9.0f};
    float mNear{0.1};
    float mFar{1500.0};

    // euler Angles
    float mYaw{-90}; // Rotate Y
    float mPitch{0}; // Rotate X

    float mMoveSpeed{2.5f};
    float mMouseSensitivity{0.1f};
    float mZoom{45};

    [[nodiscard]] glm::mat4 view() const ;
    [[nodiscard]] glm::mat4 projection() const;
    void processMouseMovement(float xoffset, float yoffset) ;
    void processMouseScroll(float yoffset) ;
    void updateCameraVectors();
    void processKeyboard(Camera_Movement direction, float deltaTime);
};

LLVK_NAMESPACE_END

#endif //LLVK_CAMERA_H
