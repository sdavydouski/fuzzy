#pragma once

#include "ShaderType.h"
#include <string>

namespace fuzzy::graphics {
    
class Shader {
public:
    Shader(ShaderType type, const std::string& path);
    ~Shader();
    
    GLuint id() const { return id_; }
private:
    GLuint id_;

    void checkCompilationStatus() const;
};

}
