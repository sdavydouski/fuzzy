#pragma once

#include <GL/glew.h>
#include <string>

namespace graphics {

    class Shader {
    public:
        static enum Type {
            VERTEX = GL_VERTEX_SHADER,
            FRAGMENT = GL_FRAGMENT_SHADER
        };

        Shader(Shader::Type type, std::string source);
        ~Shader();
        GLuint getId();
        Shader::Type getType();
        static Shader load(Shader::Type type, std::string path);
    private:
        GLuint _id;
        Shader::Type _type;
        std::string _source;

        void checkCompilationStatus();
    };

}