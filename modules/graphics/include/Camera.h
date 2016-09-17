#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace graphics {

    class Camera {
    public:
        enum Movement {
            FORWARD,
            BACKWARD,
            LEFT,
            RIGHT
        };

        // Camera Attributes
        glm::vec3 Position;
        glm::vec3 Front;
        glm::vec3 Up;
        glm::vec3 Right;
        glm::vec3 WorldUp;

        // Eular Angles
        GLfloat Yaw;
        GLfloat Pitch;

        // Camera options
        GLfloat MovementSpeed;
        GLfloat MouseSensitivity;
        GLfloat Zoom;

        // Constructor with vectors
        Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
               glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
               GLfloat yaw = -90.0f,
               GLfloat pitch = 0.0f);

        // Constructor with scalar values
        Camera(GLfloat posX,
               GLfloat posY,
               GLfloat posZ,
               GLfloat upX,
               GLfloat upY,
               GLfloat upZ,
               GLfloat yaw,
               GLfloat pitch);

        // Returns the view matrix calculated using Eular Angles and the LookAt Matrix
        glm::mat4 GetViewMatrix();

        // Processes input received from any keyboard-like input system.
        // Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
        void ProcessKeyboard(Camera::Movement direction, GLfloat deltaTime);

        // Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
        void ProcessMouseMovement(GLfloat xoffset, GLfloat yoffset, GLboolean constrainPitch = true);

        // Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
        void ProcessMouseScroll(GLfloat yoffset);
    private:
        // Calculates the front vector from the Camera's (updated) Eular Angles
        void updateCameraVectors();
    };

}