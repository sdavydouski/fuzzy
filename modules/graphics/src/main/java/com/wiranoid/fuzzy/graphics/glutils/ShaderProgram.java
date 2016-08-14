package com.wiranoid.fuzzy.graphics.glutils;

import com.wiranoid.fuzzy.core.utils.Disposable;
import org.joml.*;
import org.lwjgl.BufferUtils;

import java.nio.FloatBuffer;
import java.util.LinkedHashMap;

import static org.lwjgl.opengl.GL11.GL_TRUE;
import static org.lwjgl.opengl.GL20.*;


public class ShaderProgram implements Disposable {
    private final int id;

    private Shader vertexShader;
    private Shader geometryShader;
    private Shader fragmentShader;

    //todo: implement VertexAttribute class
    //private LinkedHashMap<String, Object> vertexAttributes;
    private LinkedHashMap<String, Object> uniforms;

    public Shader getVertexShader() {
        return vertexShader;
    }

    public Shader getGeometryShader() {
        return geometryShader;
    }

    public Shader getFragmentShader() {
        return fragmentShader;
    }

    public LinkedHashMap<String, Object> getUniforms() {
        return uniforms;
    }

    public Object getUniform(String name) {
        return uniforms.get(name);
    }

    public ShaderProgram(Shader vertexShader, Shader fragmentShader) {
        id = glCreateProgram();
        uniforms = new LinkedHashMap<>();

        attachShader(vertexShader);
        attachShader(fragmentShader);
    }

    public void attachShader(Shader shader) {
        switch (shader.getType()) {
            case VERTEX:
                this.vertexShader = shader;
                break;
            case GEOMETRY:
                this.geometryShader = shader;
                break;
            case FRAGMENT:
                this.fragmentShader = shader;
                break;
            default:
                throw new IllegalArgumentException("Unsupported shader type! " +
                        "Only vertex, geometry and fragment types are currently supported.");
        }

        glAttachShader(id, shader.getId());
    }

    public void link() {
        glLinkProgram(id);
        checkLinkageStatus();
    }

    public void use() {
        glUseProgram(id);
    }

    public void end() {
        glUseProgram(0);
    }

    public void setVertexAttribute(int location,
                                   int size,
                                   int type,
                                   boolean normalized,
                                   int stride,
                                   int offset) {
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(location, size, type, normalized, stride, offset);
    }

    public void setVertexAttribute(String name,
                                   int size,
                                   int type,
                                   boolean normalized,
                                   int stride,
                                   int offset) {
        int location = getAttributeLocation(name);
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(location, size, type, normalized, stride, offset);
    }

    public void setUniform(String name, boolean value) {
        uniforms.put(name, value);
        glUniform1i(getUniformLocation(name), value ? 1 : 0);
    }

    public void setUniform(String name, int value) {
        uniforms.put(name, value);
        glUniform1i(getUniformLocation(name), value);
    }

    public void setUniform(String name, float value) {
        uniforms.put(name, value);
        glUniform1f(getUniformLocation(name), value);
    }

    public void setUniform(String name, Vector2f value) {
        uniforms.put(name, value);
        FloatBuffer buffer = BufferUtils.createFloatBuffer(2);
        glUniform2fv(getUniformLocation(name), value.get(buffer));
    }

    public void setUniform(String name, Vector3f value) {
        uniforms.put(name, value);
        FloatBuffer buffer = BufferUtils.createFloatBuffer(3);
        glUniform3fv(getUniformLocation(name), value.get(buffer));
    }

    public void setUniform(String name, Vector4f value) {
        uniforms.put(name, value);
        FloatBuffer buffer = BufferUtils.createFloatBuffer(4);
        glUniform4fv(getUniformLocation(name), value.get(buffer));
    }

    public void setUniform(String name, Matrix3f value) {
        uniforms.put(name, value);
        FloatBuffer buffer = BufferUtils.createFloatBuffer(9);
        glUniformMatrix3fv(getUniformLocation(name), false, value.get(buffer));
    }

    public void setUniform(String name, Matrix4f value) {
        uniforms.put(name, value);
        FloatBuffer buffer = BufferUtils.createFloatBuffer(16);
        glUniformMatrix4fv(getUniformLocation(name), false, value.get(buffer));
    }

    public void dispose() {
        end();

        vertexShader.dispose();
        fragmentShader.dispose();
        if (geometryShader != null) {
            geometryShader.dispose();
        }

        glDeleteProgram(id);
    }

    private void checkLinkageStatus() {
        int status = glGetProgrami(id, GL_LINK_STATUS);
        if (status != GL_TRUE) {
            throw new RuntimeException(glGetProgramInfoLog(id));
        }
    }

    /*
    * You should stop using glGetAttribLocation. Assign each attribute a location,
    * either with glBindAttribLocation before linking the program or with explicit attribute locations
    * */
    private int getAttributeLocation(String name) {
        int location = glGetAttribLocation(id, name);
        if (location == -1) {
            throw new RuntimeException("Unable to find attribute '" + name + "' in the shaders!");
        }
        return location;
    }

    private int getUniformLocation(String name) {
        int location = glGetUniformLocation(id, name);
        if (location == -1) {
            throw new RuntimeException("Unable to find uniform '" + name + "' in the shaders!");
        }
        return location;
    }
}
