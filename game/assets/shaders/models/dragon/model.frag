#version 330 core

struct DirectionalLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
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
vec3 calculateSpotLight(SpotLight light, vec3 normal, vec3 fragmentPosition, vec3 viewDirection);

in vec3 FragmentPosition;
in vec3 Normal;

out vec4 color;

uniform DirectionalLight directionalLight;
uniform SpotLight spotLight;
uniform vec3 viewPosition;

vec3 modelColor = vec3(1.0f, 0.0f, 0.0f);

void main() {
    // Properties
    vec3 normal = normalize(Normal);
    vec3 viewDirection = normalize(viewPosition - FragmentPosition);

    vec3 result = calculateDirectionalLight(directionalLight, normal, viewDirection);

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
    float spec = pow(max(dot(reflectDirection, viewDirection), 0.0f), 32.0f);

    // Combine results
    vec3 ambient = light.ambient * modelColor;
    vec3 diffuse = light.diffuse * diff * modelColor;
    vec3 specular = light.specular * spec * modelColor;

    return ambient + diffuse + specular;
}

vec3 calculateSpotLight(SpotLight light, vec3 normal, vec3 fragmentPosition, vec3 viewDirection) {
    vec3 lightDirection = normalize(light.position - fragmentPosition);
    // Diffuse shading
    float diff = max(dot(lightDirection, normal), 0.0f);
    // Specular shading
    vec3 reflectDirection = reflect(-lightDirection, normal);
    float spec = pow(max(dot(reflectDirection, viewDirection), 0.0f), 32.0f);
    // Attenuation
    float distance = length(light.position - fragmentPosition);
    float attenuation = 1.0f / ( light.constant + light.linear * distance + light.quadratic * distance * distance );
    // Spotlight intensity
    float theta = dot(lightDirection, normalize(-light.direction));
    float epsilon = (light.innerCutOff - light.outerCutOff);
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    //Combine results
    vec3 ambient = light.ambient * modelColor;
    vec3 diffuse = light.diffuse * diff * modelColor;
    vec3 specular = light.specular * spec * modelColor;

    ambient *= attenuation;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;

    return ambient + diffuse + specular;
}