package com.wiranoid.fuzzy.graphics.glutils;

import com.wiranoid.fuzzy.graphics.Window;
import org.joml.*;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.lwjgl.glfw.GLFW.glfwInit;


public class ShaderProgramTest {

    private ShaderProgram shaderProgram;

    {
        glfwInit();
        new Window(800, 600, "Test", false, true);

        String vertexShaderSource =
                "#version 330 core\n" +
                "layout (location = 0) in vec3 position;\n" +
                "void main()\n" +
                "{\n" +
                "gl_Position = vec4(position, 1.0);\n" +
                "}\n";

        String fragmentShaderSource =
                "#version 330 core\n" +
                "out vec4 color;\n" +
                "uniform int intUniform;\n" +
                "uniform float floatUniform;\n" +
                "uniform vec2 vec2fUniform;\n" +
                "uniform vec3 vec3fUniform;\n" +
                "uniform vec4 vec4fUniform;\n" +
                "uniform mat3 mat3fUniform;\n" +
                "uniform mat4 mat4fUniform;\n" +
                "void main()\n" +
                "{\n" +
                    "color = intUniform * floatUniform * vec4(" +
                        "vec2fUniform.x, vec3fUniform.y, vec4fUniform.z * mat3fUniform[0][0], mat4fUniform[1][1]" +
                    ");\n" +
                "}\n";

        this.shaderProgram = new ShaderProgram(
                new Shader(Shader.Type.VERTEX, vertexShaderSource),
                new Shader(Shader.Type.FRAGMENT, fragmentShaderSource)
        );
    }

    @Test
    public void testUniforms() {
        shaderProgram.link();

        int intUniform = 42;
        float floatUniform = 3.14f;
        Vector2f vec2fUniform = new Vector2f(1.0f, 2.0f);
        Vector3f vec3fUniform = new Vector3f(1.0f, 2.0f, 3.0f);
        Vector4f vec4fUniform = new Vector4f(1.0f, 2.0f, 3.0f, 4.0f);
        Matrix3f mat3fUniform = new Matrix3f();
        Matrix4f mat4fUniform = new Matrix4f().add(new Matrix4f());

        shaderProgram.setUniform("intUniform", intUniform);
        shaderProgram.setUniform("floatUniform", floatUniform);
        shaderProgram.setUniform("vec2fUniform", vec2fUniform);
        shaderProgram.setUniform("vec3fUniform", vec3fUniform);
        shaderProgram.setUniform("vec4fUniform", vec4fUniform);
        shaderProgram.setUniform("mat3fUniform", mat3fUniform);
        shaderProgram.setUniform("mat4fUniform", mat4fUniform);

        assertEquals(intUniform, shaderProgram.getUniform("intUniform"));
        assertEquals(floatUniform, shaderProgram.getUniform("floatUniform"));
        assertEquals(vec2fUniform, shaderProgram.getUniform("vec2fUniform"));
        assertEquals(vec3fUniform, shaderProgram.getUniform("vec3fUniform"));
        assertEquals(vec4fUniform, shaderProgram.getUniform("vec4fUniform"));
        assertEquals(mat3fUniform, shaderProgram.getUniform("mat3fUniform"));
        assertEquals(mat4fUniform, shaderProgram.getUniform("mat4fUniform"));
        assertEquals(7, shaderProgram.getUniforms().size());

        shaderProgram.dispose();
    }
}
