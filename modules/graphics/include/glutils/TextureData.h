#pragma once

#include <GL/glew.h>
#include <string>

namespace graphics {

    struct TextureData {
        GLuint id;
        std::string type;
        std::string path;
    };

}
