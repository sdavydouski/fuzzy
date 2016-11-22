#pragma once

#include <GL/glew.h>
#include <string>

namespace graphics {

    struct TextureData {
        GLuint id;
        std::string type;
        // We store the path of the texture to compare with other textures
        std::string path;
    };

}
