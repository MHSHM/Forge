#version 460 core

layout(set = 0, binding = 0) uniform sampler2D opaques_color;

#ifdef VERTEX_SHADER

layout(location = 0) in vec3 position;

void main()
{
	gl_Position = vec4(position, 1.0f);
}

#endif

#ifdef FRAGMENT_SHADER

layout (location = 0) out vec4 color;

void main()
{
	ivec2 frag_coord = ivec2(gl_FragCoord.xy);
	color = texelFetch(opaques_color, frag_coord, 0);
}

#endif