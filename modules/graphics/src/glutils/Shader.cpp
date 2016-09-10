#include "../../include/glutils/Shader.h"
#include "GL/glew.h"
#include <string>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iostream>

using namespace graphics;

Shader::Shader(Shader::Type type, std::string source) :
        _type(type), _source(source) {
    this->_id = glCreateShader(type);

    const GLchar* shaderSource = source.c_str();
    glShaderSource(this->_id, 1, &shaderSource, nullptr);
    glCompileShader(this->_id);
    this->checkCompilationStatus();
}

Shader::~Shader() {
    std::cout << "Deleting shader" << std::endl;
    glDeleteShader(this->_id);
}

GLuint Shader::getId() {
    return this->_id;
}

Shader::Type Shader::getType() {
    return this->_type;
}

Shader Shader::load(Shader::Type type, std::string path) {
    std::string source;
    std::ifstream shaderFile;

    // ensures ifstream objects can throw exceptions
    shaderFile.exceptions(std::ifstream::badbit);
    try {
        // Open file
        shaderFile.open(path);
        std::stringstream shaderStream;
        // Read file's buffer contents into stream
        shaderStream << shaderFile.rdbuf();
        // close file handler
        shaderFile.close();
        // Convert stream into string
        source = shaderStream.str();
    }
    catch(std::ifstream::failure ex) {
        throw std::runtime_error("Failed to load shader file");
    }

    return Shader(type, source);
}

void Shader::checkCompilationStatus() {
    const int MAX_BUFFER_SIZE = 512;
    GLint success;
    GLchar infoLog[MAX_BUFFER_SIZE];

    glGetShaderiv(this->_id, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(this->_id, MAX_BUFFER_SIZE, nullptr, infoLog);
        throw std::runtime_error(infoLog);
    }
}
