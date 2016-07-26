package com.wiranoid.fuzzy.graphics.shaders;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;

import static org.lwjgl.opengl.GL11.GL_TRUE;
import static org.lwjgl.opengl.GL20.*;

public class Shader {
    private final int id;

    public int getId() {
        return id;
    }

    private Shader(int type, String source) {
        id = glCreateShader(type);
        glShaderSource(id, source);
        glCompileShader(id);

        checkCompilationStatus();
    }

    public void delete() {
        glDeleteShader(id);
    }

    public static Shader load(int type, String path) {
        String source;

        try {
            source = new String(Files.readAllBytes(Paths.get(path)));
        }
        catch (IOException ex) {
            throw new RuntimeException("Failed to load a shader file!"
                    + System.lineSeparator() + ex.getMessage());
        }

        return new Shader(type, source);
    }

    private void checkCompilationStatus() {
        int status = glGetShaderi(id, GL_COMPILE_STATUS);
        if (status != GL_TRUE) {
            throw new RuntimeException(glGetShaderInfoLog(id));
        }
    }
}
