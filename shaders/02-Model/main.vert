//
#version 460 core

layout(push_constant) uniform PerFrameData {
	mat4 MVP;
	uint textureId;
} pc;

layout (constant_id = 0) const bool isWireframe = false;

layout (location=0) in vec3 inPos;
layout (location=1) in vec2 inUV;

layout (location=0) out vec2 vUV;
layout (location=1) out vec3 vColor;

void main() {
	gl_Position = pc.MVP * vec4(inPos, 1.0);
	vUV = inUV;
	vColor = isWireframe ? vec3(0.0) : inPos.xyz;
}