package com.wiranoid.fuzzy.graphics.glutils;

import com.wiranoid.fuzzy.utils.Disposable;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;

import static org.lwjgl.opengl.GL11.GL_TRUE;
import static org.lwjgl.opengl.GL20.*;
import static org.lwjgl.opengl.GL32.*;


public class Shader implements Disposable {
    public enum Type {
        VERTEX(GL_VERTEX_SHADER),
        GEOMETRY(GL_GEOMETRY_SHADER),
        FRAGMENT(GL_FRAGMENT_SHADER);

        private final int type;

        public int get() {
            return type;
        }

        Type(int type) {
            this.type = type;
        }
    }

    private final int id;

    private Type type;
    private String source;

    public int getId() {
        return id;
    }

    public Type getType() {
        return type;
    }

    public String getSource() {
        return source;
    }

    public Shader(Shader.Type type, String source) {
        this.type = type;
        this.source = source;

        id = glCreateShader(type.get());
        glShaderSource(id, source);
        glCompileShader(id);

        checkCompilationStatus();
    }

    public void dispose() {
        glDeleteShader(id);
    }

    public static Shader load(Shader.Type type, String path) {
        String source;

        try {
            //todo: move to file utilities
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
