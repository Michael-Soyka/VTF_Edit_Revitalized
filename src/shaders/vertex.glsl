#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

uniform mat4x4 SCALING;
uniform vec2 OFFSET;

out vec2 TexCoord;

void main()
{
    gl_Position = SCALING * vec4(aPos, 1.0) + vec4(OFFSET,0,0);
    TexCoord = vec2(aTexCoord.x,-aTexCoord.y);
}