#include "../../include/g3d/Model.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <stb_image.h>
#include <string>
#include <stdexcept>
#include <vector>
#include "../../include/glutils/VertexData.h"
#include "../../include/glutils/TextureData.h"

using namespace graphics;

// Constructor, expects a filepath to a 3D model
Model::Model(std::string path) {
    this->loadModel(path);
}

// Draws the model, and thus all its meshes
void Model::Draw(ShaderProgram& shader) {
    for (Mesh& mesh : this->_meshes) {
        mesh.Draw(shader);
    }
}

// Loads a model with supported ASSIMP extensions from file
// and stores the resulting meshes in the meshes vector
void Model::loadModel(std::string path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        throw std::runtime_error(importer.GetErrorString());
    }

    this->_directory = path.substr(0, path.find_last_of('/'));
    this->processNode(scene->mRootNode, scene);
}

// Processes a node in a recursive fashion. Processes each
// individual mesh located at the node and repeats this process
// on its children nodes (if any).
void Model::processNode(aiNode* node, const aiScene* scene) {
    // Process each mesh located at the current node
    for (GLuint i = 0; i < node->mNumMeshes; i++) {
        // The node object only contains indices to index the actual objects in the scene.
        // The scene contains all the data, node is just to keep stuff organized (like relations between nodes).
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        this->_meshes.push_back(this->processMesh(mesh, scene));
    }
    // After we've processed all of the meshes (if any) we then recursively process each of the children nodes
    for (GLuint i = 0; i < node->mNumChildren; i++) {
        this->processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene) {
    std::vector<VertexData> vertices;
    std::vector<GLuint> indices;
    std::vector<TextureData> textures;

    for (GLuint i = 0; i < mesh->mNumVertices; i++) {
        VertexData vertex;
        // Process vertex positions, normals and texture coordinates
        glm::vec3 vector;

        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.Position = vector;

        vector.x = mesh->mNormals[i].x;
        vector.y = mesh->mNormals[i].y;
        vector.z = mesh->mNormals[i].z;
        vertex.Normal = vector;

        // Does the mesh contain texture coordinates?
        if (mesh->mTextureCoords[0]) {
            glm::vec2 uv;
            uv.x = mesh->mTextureCoords[0][i].x;
            uv.y = mesh->mTextureCoords[0][i].y;
            vertex.UV = uv;
        } else {
            vertex.UV = glm::vec2(0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }

    // Process indices
    for (GLuint i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (GLuint j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // Process materials
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        // We assume a convention for sampler names in the shaders. Each diffuse texture should be named
        // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER.
        // Same applies to other texture as the following list summarizes:
        // Diffuse: texture_diffuseN
        // Specular: texture_specularN
        // Normal: texture_normalN
        std::vector<TextureData> diffuseMaps =
                this->loadMaterialTextures(material,
                                           aiTextureType_DIFFUSE,
                                           "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        std::vector<TextureData> specularMaps =
                this->loadMaterialTextures(material,
                                           aiTextureType_SPECULAR,
                                           "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    }

    return Mesh(vertices, indices, textures);
}

// Checks all material textures of a given type and loads the textures if they're not loaded yet.
// The required info is returned as a Texture struct.
std::vector<TextureData> Model::loadMaterialTextures(aiMaterial* mat,
                                                     aiTextureType type,
                                                     std::string typeName) {
    std::vector<TextureData> textures;

    for (GLuint i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        // Check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
        GLboolean skip = false;
        for (GLuint j = 0; j < this->_loadedTextures.size(); j++) {
            if (this->_loadedTextures[j].path == str.C_Str()) {
                textures.push_back(this->_loadedTextures[j]);
                // A texture with the same filepath has already been loaded, continue to next one. (optimization)
                skip = true;
                break;
            }
        }
        if (!skip) {
            // If texture hasn't been loaded already, load it
            TextureData texture;
            texture.id = this->TextureFromFile(str.C_Str(), this->_directory);
            texture.type = typeName;
            texture.path = str.C_Str();

            textures.push_back(texture);
            // Store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
            this->_loadedTextures.push_back(texture);
        }
    }

    return textures;
}

GLuint Model::TextureFromFile(const char *path, std::string directory) {
    //Generate texture ID and load texture data
    std::string filename = std::string(path);
    filename = directory + '/' + filename;
    GLuint textureID;
    glGenTextures(1, &textureID);
    int width,height;
    unsigned char* image = stbi_load(filename.c_str(), &width, &height, 0, 3);
    // Assign texture to ID
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);
    // Parameters
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(image);
    return textureID;
}
