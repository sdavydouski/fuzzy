package com.wiranoid.fuzzy.graphics;

import org.joml.Matrix4f;
import org.joml.Vector3f;

/**
 * Camera class that processes input and calculates the corresponding Eular Angles,
 * Vectors and Matrices for use in OpenGL.
 */
public class Camera {
    /**
     * Defines several possible options for camera movement.
     * Used as abstraction to stay away from window-system specific input methods.
     */
    public enum Movement {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT
    }

    /**
     * Camera attributes
     */
    private Vector3f position;
    private Vector3f direction;
    private Vector3f up;
    private Vector3f right;
    private Vector3f worldUp;

    /**
     * Eular angles
     */
    private float yaw;
    private float pitch;

    /**
     * Camera options
     */
    private float movementSpeed;
    private float mouseSensitivity;
    private float zoom;

    public Vector3f getPosition() {
        return position;
    }

    public Vector3f getDirection() {
        return direction;
    }

    public Vector3f getUp() {
        return up;
    }

    public Vector3f getRight() {
        return right;
    }

    public Vector3f getWorldUp() {
        return worldUp;
    }

    public float getYaw() {
        return yaw;
    }

    public float getPitch() {
        return pitch;
    }

    public float getMovementSpeed() {
        return movementSpeed;
    }

    public float getMouseSensitivity() {
        return mouseSensitivity;
    }

    public float getZoom() {
        return zoom;
    }

    public Camera(Vector3f position,
                  Vector3f direction,
                  Vector3f worldUp,
                  float yaw,
                  float pitch,
                  float movementSpeed,
                  float mouseSensitivity,
                  float zoom) {
        this.position = position;
        this.direction = direction;
        this.worldUp = worldUp;
        this.yaw = yaw;
        this.pitch = pitch;
        this.movementSpeed = movementSpeed;
        this.mouseSensitivity = mouseSensitivity;
        this.zoom = zoom;
        this.updateCameraVectors();
    }

    /**
     * Returns the view matrix calculated using Eular Angles and the LookAt Matrix.
     * @return
     */
    public Matrix4f getViewMatrix() {
        return new Matrix4f().lookAt(this.position, this.position.add(this.direction, new Vector3f()), this.up);
    }

    /**
     * Processes input received from any keyboard-like input system.
     * Accepts input parameter in the form of camera defined enum (to abstract it from windowing systems)
     * @param direction
     * @param deltaTime
     */
    public void processKeyboard(Camera.Movement direction, float deltaTime) {
        float velocity = this.movementSpeed * deltaTime;
        switch (direction) {
            case FORWARD:
                this.position.add(this.direction.mul(velocity, new Vector3f()));
                break;
            case BACKWARD:
                this.position.sub(this.direction.mul(velocity, new Vector3f()));
                break;
            case LEFT:
                this.position.sub(this.right.mul(velocity, new Vector3f()));
                break;
            case RIGHT:
                this.position.add(this.right.mul(velocity, new Vector3f()));
                break;
        }
    }

    /**
     * Processes input received from a mouse input system.
     * Expects the offset value in both the x and y direction.
     * @param xOffset
     * @param yOffset
     * @param constrainPitch
     */
    public void processMouseMovement(float xOffset, float yOffset, boolean constrainPitch) {
        xOffset *= this.mouseSensitivity;
        yOffset *= this.mouseSensitivity;

        this.yaw += xOffset;
        this.pitch += yOffset;

        // Make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch) {
            if (this.pitch > 89.0f) {
                this.pitch = 89.0f;
            } else if (this.pitch < -89.0f) {
                this.pitch = -89.0f;
            }
        }

        // Update Front, Right and Up Vectors using the updated Eular angles
        this.updateCameraVectors();
    }

    /**
     * Processes input received from a mouse scroll-wheel event.
     * Only requires input on the vertical wheel-axis.
     * @param yOffset
     */
    public void processMouseScroll(float yOffset) {
        if (1.0f <= this.zoom && this.zoom <= 80.0f) {
            this.zoom -= yOffset;
        }
        if (this.zoom <= 1.0f) {
            this.zoom = 1.0f;
        }
        if (this.zoom >= 80.0f) {
            this.zoom = 80.0f;
        }
    }

    /**
     * Calculates the direction vector from the Camera's (updated) Eular Angles.
     */
    private void updateCameraVectors() {
        // Calculate the new direction vector
        Vector3f direction = new Vector3f();
        direction.x = (float) ( Math.cos(Math.toRadians(this.yaw)) * Math.cos(Math.toRadians(this.pitch)) );
        direction.y = (float) Math.sin(Math.toRadians(this.pitch));
        direction.z = (float) ( Math.sin(Math.toRadians(this.yaw)) * Math.cos(Math.toRadians(this.pitch)) );
        this.direction = direction.normalize();
        // Also recalculate the Right and Up vector
        this.right = this.direction.cross(this.worldUp, new Vector3f()).normalize();
        this.up = this.right.cross(this.direction, new Vector3f()).normalize();
    }

}
