#version 330 core

#define NUMBER_OF_POINT_LIGHTS 4

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    sampler2D emission;
    float shininess;
};

struct DirectionalLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct SpotLight {
    vec3 position;
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;

    float innerCutOff;
    float outerCutOff;

    bool enabled;
};

// Function prototypes
vec3 calculateDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDirection);
vec3 calculatePointLight(PointLight light, vec3 normal, vec3 fragmentPosition, vec3 viewDirection);
vec3 calculateSpotLight(SpotLight light, vec3 normal, vec3 fragmentPosition, vec3 viewDirection);

in vec2 TextureCoordinates;
in vec3 FragmentPosition;
in vec3 Normal;

out vec4 color;

uniform Material material;
uniform DirectionalLight directionalLight;
uniform PointLight pointLights[NUMBER_OF_POINT_LIGHTS];
uniform SpotLight spotLight;
uniform vec3 viewPosition;

void main() {
    // Properties
    vec3 normal = normalize(Normal);
    vec3 viewDirection = normalize(viewPosition - FragmentPosition);

    // == ======================================
    // Our lighting is set up in 3 phases: directional, point lights and an optional flashlight
    // For each phase, a calculate function is defined that calculates the corresponding color
    // per lamp. In the main() function we take all the calculated colors and sum them up for
    // this fragment's final color.
    // == ======================================
    // Phase 1: Directional lighting
    vec3 result = calculateDirectionalLight(directionalLight, normal, viewDirection);
    // Phase 2: Point lights
    for (int i = 0; i < NUMBER_OF_POINT_LIGHTS; i++) {
        result += calculatePointLight(pointLights[i], normal, FragmentPosition, viewDirection);
    }
    // Phase 3: Spot light
    if (spotLight.enabled) {
        result += calculateSpotLight(spotLight, normal, FragmentPosition, viewDirection);
    }

    color = vec4(result, 1.0f);
}

vec3 calculateDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDirection) {
    vec3 lightDirection = normalize(-light.direction);
    // Diffuse shading
    float diff = max(dot(lightDirection, normal), 0.0f);
    // Specular shading
    vec3 reflectDirection = reflect(-lightDirection, normal);
    float spec = pow(max(dot(reflectDirection, viewDirection), 0.0f), material.shininess);

    // Combine results
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TextureCoordinates));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TextureCoordinates));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TextureCoordinates));

    return ambient + diffuse + specular;
}

vec3 calculatePointLight(PointLight light, vec3 normal, vec3 fragmentPosition, vec3 viewDirection) {
    vec3 lightDirection = normalize(light.position - fragmentPosition);
    // Diffuse shading
    float diff = max(dot(lightDirection, normal), 0.0f);
    // Specular shading
    vec3 reflectDirection = reflect(-lightDirection, normal);
    float spec = pow(max(dot(reflectDirection, viewDirection), 0.0f), material.shininess);
    // Attenuation
    float distance = length(light.position - fragmentPosition);
    float attenuation = 1.0f / ( light.constant + light.linear * distance + light.quadratic * distance * distance );

    //Combine results
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TextureCoordinates));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TextureCoordinates));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TextureCoordinates));
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    return ambient + diffuse + specular;
}

vec3 calculateSpotLight(SpotLight light, vec3 normal, vec3 fragmentPosition, vec3 viewDirection) {
    vec3 lightDirection = normalize(light.position - fragmentPosition);
    // Diffuse shading
    float diff = max(dot(lightDirection, normal), 0.0f);
    // Specular shading
    vec3 reflectDirection = reflect(-lightDirection, normal);
    float spec = pow(max(dot(reflectDirection, viewDirection), 0.0f), material.shininess);
    // Attenuation
    float distance = length(light.position - fragmentPosition);
    float attenuation = 1.0f / ( light.constant + light.linear * distance + light.quadratic * distance * distance );
    // Spotlight intensity
    float theta = dot(lightDirection, normalize(-light.direction));
    float epsilon = (light.innerCutOff - light.outerCutOff);
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    //Combine results
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TextureCoordinates));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TextureCoordinates));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TextureCoordinates));

    ambient *= attenuation;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;

    return ambient + diffuse + specular;
}