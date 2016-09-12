#pragma once

#include <GL/glew.h>
#include <stb_image.h>
#include <string>

namespace graphics {

    class Texture {
    public:
        Texture(GLuint width, GLuint height, unsigned char* image);
        ~Texture();
        void bind();
        void bind(GLuint unit);
        void unbind();
        static Texture load(std::string path);
    private:
        GLuint _id;
        GLuint _width;
        GLuint _height;
    };

}