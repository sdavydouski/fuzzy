#include "../../include/glutils/ShaderProgram.h"
#include "../../include/glutils/Shader.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <stdexcept>
#include <iostream>

using namespace graphics;

ShaderProgram::ShaderProgram(const Shader& vertexShader,
                             const Shader& fragmentShader) :
        _vertexShader(vertexShader), _fragmentShader(fragmentShader) {
    this->_id = glCreateProgram();

    glAttachShader(this->_id, vertexShader.getId());
    glAttachShader(this->_id, fragmentShader.getId());

    this->link();
}

ShaderProgram::~ShaderProgram() {
    std::cout << "Deleting shaderProgram" << std::endl;
    this->end();
    glDeleteProgram(this->_id);
}

void ShaderProgram::link() {
    glLinkProgram(this->_id);
    this->checkLinkageStatus();
}

void ShaderProgram::use() {
    glUseProgram(this->_id);
}

void ShaderProgram::end() {
    glUseProgram(0);
}

//void ShaderProgram::setVertexAttribute(GLuint location,
//                                       GLuint size,
//                                       GLuint type,
//                                       GLboolean normalized,
//                                       GLuint stride,
//                                       GLuint offset) {
//    glVertexAttribPointer(location, size, type, normalized, stride, (GLvoid*) offset);
//    glEnableVertexAttribArray(location);
//}

void ShaderProgram::setUniform(std::string name, GLboolean value) {
    glUniform1i(this->getUniformLocation(name), value);
}

void ShaderProgram::setUniform(std::string name, GLint value) {
    glUniform1i(this->getUniformLocation(name), value);
}

void ShaderProgram::setUniform(std::string name, GLuint value) {
    glUniform1i(this->getUniformLocation(name), value);
}

void ShaderProgram::setUniform(std::string name, GLfloat value) {
    glUniform1f(this->getUniformLocation(name), value);
}

void ShaderProgram::setUniform(std::string name, glm::vec2 value) {
    glUniform2f(this->getUniformLocation(name), value.x, value.y);
}

void ShaderProgram::setUniform(std::string name, glm::vec3 value) {
    glUniform3f(this->getUniformLocation(name), value.x, value.y, value.z);
}

void ShaderProgram::setUniform(std::string name, glm::vec4 value) {
    glUniform4f(this->getUniformLocation(name), value.x, value.y, value.z, value.w);
}

void ShaderProgram::setUniform(std::string name, glm::mat2 value) {
    glUniformMatrix2fv(this->getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void ShaderProgram::setUniform(std::string name, glm::mat3 value) {
    glUniformMatrix3fv(this->getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void ShaderProgram::setUniform(std::string name, glm::mat4 value) {
    glUniformMatrix4fv(this->getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void ShaderProgram::checkLinkageStatus() {
    const int MAX_BUFFER_SIZE = 512;
    GLint success;
    GLchar infoLog[MAX_BUFFER_SIZE];

    glGetProgramiv(this->_id, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(this->_id, MAX_BUFFER_SIZE, nullptr, infoLog);
        throw std::runtime_error(infoLog);
    }
}

GLint ShaderProgram::getUniformLocation(std::string name) {
    GLint location = glGetUniformLocation(this->_id, name.c_str());
    if (location == -1) {
        // too strict
        //throw std::runtime_error("Unable to find uniform " + name);
    }
    return location;
}
