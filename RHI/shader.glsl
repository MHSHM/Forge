#version 460 core

layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
} ubo;

#ifdef VERTEX_SHADER

layout(location = 0) in vec3 position;

void main()
{
	gl_Position = ubo.model * vec4(position, 1.0);
}

#endif

#ifdef FRAGMENT_SHADER

layout (location = 0) out vec4 color;

void main()
{
	color = vec4(1.0, 1.0, 1.0, 1.0);
}

#endif