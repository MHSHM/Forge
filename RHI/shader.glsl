#version 460 core

layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D base_color_texture;
layout (set = 0, binding = 2, rgba8) uniform readonly image2D inputImage;
layout (set = 0, binding = 3, rgba8) uniform image2D resultImage;
layout (set = 0, binding = 4, rgba8) uniform writeonly image2D resultImage2;

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
	color = texture(base_color_texture, vec2(0.0));
	color *= imageLoad(inputImage, ivec2(1));
	imageStore(resultImage, ivec2(1), color);
	imageStore(resultImage2, ivec2(1), color);
}

#endif