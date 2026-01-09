//
#version 460 core
#extension GL_EXT_nonuniform_qualifier : enable

layout (location = 0) in vec2 vUV;
layout (location = 1) in vec3 vColor;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform texture2D kTextures2D[];
layout (set = 0, binding = 1) uniform sampler kSamplers[];

layout(push_constant) uniform PerFrameData{
	mat4 MVP;
	uint textureId;
} pc;

layout (constant_id = 0) const bool isWireframe = false;

void main()
{
	if(isWireframe)
	{
		outColor = vec4(vColor, 1.0);
	} else
	{
		outColor = texture(sampler2D(kTextures2D[pc.textureId], kSamplers[0]), vUV);
	}
}