package com.wiranoid.fuzzy.graphics.glutils;

import com.wiranoid.fuzzy.graphics.Window;
import org.joml.*;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.lwjgl.glfw.GLFW.glfwInit;


public class ShaderProgramTest {

    //setup opengl
    {
        glfwInit();
        Window window = new Window(800, 600, "Test", false, true);
    }

    private String vertexShaderSource = "#version 330 core\n" +
            "layout (location = 0) in vec3 position;\n" +
            "void main()\n" +
            "{\n" +
                "gl_Position = vec4(position, 1.0);\n" +
            "}\n";

    private String fragmentShaderSource = "#version 330 core\n" +
            "out vec4 color;\n" +
            "uniform int intVal;\n" +
            "uniform float floatVal;\n" +
            "uniform vec2 vec2fVal;\n" +
            "uniform vec3 vec3fVal;\n" +
            "uniform vec4 vec4fVal;\n" +
            "uniform mat3 mat3fVal;\n" +
            "uniform mat4 mat4fVal;\n" +
            "void main()\n" +
            "{\n" +
                "color = intVal * floatVal * vec4(vec2fVal.x, vec3fVal.y, vec4fVal.z * mat3fVal[0][0], mat4fVal[1][1]);\n" +
            "}\n";

    @Test
    public void checkUniforms() {
        ShaderProgram shaderProgram = new ShaderProgram(
                new Shader(Shader.Type.VERTEX, vertexShaderSource),
                new Shader(Shader.Type.FRAGMENT, fragmentShaderSource)
        );
        shaderProgram.link();
        shaderProgram.use();

        int intVal = 42;
        float floatVal = 3.14f;
        Vector2f vec2fVal = new Vector2f(1.0f, 2.0f);
        Vector3f vec3fVal = new Vector3f(1.0f, 2.0f, 3.0f);
        Vector4f vec4fVal = new Vector4f(1.0f, 2.0f, 3.0f, 4.0f);
        Matrix3f mat3fVal = new Matrix3f();
        Matrix4f mat4fVal = new Matrix4f().add(new Matrix4f());

        shaderProgram.setUniform("intVal", intVal);
        shaderProgram.setUniform("floatVal", floatVal);
        shaderProgram.setUniform("vec2fVal", vec2fVal);
        shaderProgram.setUniform("vec3fVal", vec3fVal);
        shaderProgram.setUniform("vec4fVal", vec4fVal);
        shaderProgram.setUniform("mat3fVal", mat3fVal);
        shaderProgram.setUniform("mat4fVal", mat4fVal);

        assertEquals(intVal, shaderProgram.getUniform("intVal"));
        assertEquals(floatVal, shaderProgram.getUniform("floatVal"));
        assertEquals(vec2fVal, shaderProgram.getUniform("vec2fVal"));
        assertEquals(vec3fVal, shaderProgram.getUniform("vec3fVal"));
        assertEquals(vec4fVal, shaderProgram.getUniform("vec4fVal"));
        assertEquals(mat3fVal, shaderProgram.getUniform("mat3fVal"));
        assertEquals(mat4fVal, shaderProgram.getUniform("mat4fVal"));

        assertEquals(7, shaderProgram.getUniforms().size());

        shaderProgram.dispose();
    }
}
