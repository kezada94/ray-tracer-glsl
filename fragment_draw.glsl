#version 410

uniform int framecount;
uniform sampler2D renderedTexture;
in vec2 texture_coordinates;

out vec4 fragcolor;

void main() {
    vec3 col = texture(renderedTexture, texture_coordinates).xyz;
    fragcolor = vec4(col, 1.0);
}
