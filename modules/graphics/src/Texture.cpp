#include "../include/Texture.h"
#include <GL/glew.h>
// STB_IMAGE_IMPLEMENTATION must be defined in *.c or *.cpp file (not in header)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <string>
#include <stdexcept>
#include <iostream>

using namespace graphics;

Texture::Texture(GLuint width, GLuint height, unsigned char *image) :
        _width(width), _height(height) {
    glGenTextures(1, &this->_id);

    this->bind();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(image);
    this->unbind();
}

Texture::~Texture() {
    std::cout << "Deleting texture" << std::endl;
    glDeleteTextures(1, &this->_id);
}

void Texture::bind() {
    glBindTexture(GL_TEXTURE_2D, this->_id);
}

void Texture::bind(GLuint unit) {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, this->_id);
}

void Texture::unbind() {
    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture Texture::load(std::string path) {
    int width;
    int height;
    int components;

    stbi_set_flip_vertically_on_load(true);
    unsigned char* image = stbi_load(path.c_str(), &width, &height, &components, 4);
    if (image == nullptr) {
        throw std::runtime_error("Failed to load texture file: \n" + path + "\n Reason: " + stbi_failure_reason());
    }

    return Texture((GLuint) width, (GLuint) height, image);
}


