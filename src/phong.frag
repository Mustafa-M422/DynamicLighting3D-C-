#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;   // ✅ Added for M key color change
uniform sampler2D texture1;
uniform bool showTexture;  // ✅ Already there

void main() {
    vec3 objectColor = vec3(1.0); // Default white

    // ✅ Use texture only if showTexture is true
    if (showTexture) {
        objectColor = texture(texture1, TexCoord).rgb;
    }

    // Ambient
    vec3 ambient = 0.1 * lightColor * objectColor;

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor * objectColor;

    // Specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = spec * lightColor; // highlight matches light color

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
