package com.wiranoid.fuzzy.graphics.glutils;

import com.wiranoid.fuzzy.core.utils.Disposable;
import org.joml.*;
import org.lwjgl.BufferUtils;

import java.nio.FloatBuffer;

import static org.lwjgl.opengl.GL11.GL_TRUE;
import static org.lwjgl.opengl.GL20.*;

public class ShaderProgram implements Disposable {
    private final int id;

    public ShaderProgram() {
        id = glCreateProgram();
    }

    public void attachShader(Shader shader) {
        glAttachShader(id, shader.getId());
    }

    public void link() {
        glLinkProgram(id);

        checkLinkageStatus();
    }

    public void use() {
        glUseProgram(id);
    }

    public void dispose() {
        glDeleteProgram(id);
    }

    public void setVertexAttributePointer(int location,
                                          int size,
                                          int type,
                                          boolean normalized,
                                          int stride,
                                          int offset) {
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(location, size, type, normalized, stride, offset);
    }

    public void setUniform(String name, int value) {
        glUniform1i(getUniformLocation(name), value);
    }

    public void setUniform(String name, Vector2f value) {
        FloatBuffer buffer = BufferUtils.createFloatBuffer(2);
        glUniform2fv(getUniformLocation(name), value.get(buffer));
    }

    public void setUniform(String name, Vector3f value) {
        FloatBuffer buffer = BufferUtils.createFloatBuffer(3);
        glUniform3fv(getUniformLocation(name), value.get(buffer));
    }

    public void setUniform(String name, Vector4f value) {
        FloatBuffer buffer = BufferUtils.createFloatBuffer(4);
        glUniform4fv(getUniformLocation(name), value.get(buffer));
    }

    public void setUniform(String name, Matrix3f value) {
        FloatBuffer buffer = BufferUtils.createFloatBuffer(9);
        glUniformMatrix3fv(getUniformLocation(name), false, value.get(buffer));
    }

    public void setUniform(String name, Matrix4f value) {
        FloatBuffer buffer = BufferUtils.createFloatBuffer(16);
        glUniformMatrix4fv(getUniformLocation(name), false, value.get(buffer));
    }

    private void checkLinkageStatus() {
        int status = glGetProgrami(id, GL_LINK_STATUS);
        if (status != GL_TRUE) {
            throw new RuntimeException(glGetProgramInfoLog(id));
        }
    }

    private int getUniformLocation(String name) {
        return glGetUniformLocation(id, name);
    }
}
