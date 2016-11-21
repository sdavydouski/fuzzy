#pragma once

#include "Shader.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>

namespace graphics {
    
    class ShaderProgram {
    public:
        ShaderProgram(const Shader& vertexShader, const Shader& fragmentShader);
        ~ShaderProgram();
        void link();
        void use();
        void end();
//        void setVertexAttribute(GLuint location,
//                                GLuint size,
//                                GLuint type,
//                                GLboolean normalized,
//                                GLuint stride,
//                                GLuint offset);
        void setUniform(std::string name, GLboolean value);
        void setUniform(std::string name, GLint value);
        void setUniform(std::string name, GLuint value);
        void setUniform(std::string name, GLfloat value);
        void setUniform(std::string name, glm::vec2 value);
        void setUniform(std::string name, glm::vec3 value);
        void setUniform(std::string name, glm::vec4 value);
        void setUniform(std::string name, glm::mat2 value);
        void setUniform(std::string name, glm::mat3 value);
        void setUniform(std::string name, glm::mat4 value);

    private:
        GLuint _id;
        const Shader& _vertexShader;
        const Shader& _fragmentShader;

        void checkLinkageStatus();
        GLint getUniformLocation(std::string name);
    };
    
}