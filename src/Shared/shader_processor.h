#pragma once

#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/resource_limits_c.h>

#include <lvk/LVK.h>
#include <lvk/vulkan/VulkanUtils.h>
#include <minilog/minilog.h>

#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_set>
#include <span>

namespace fs = std::filesystem;

inline std::string readTextFile(const fs::path& file, std::unordered_set<fs::path>& includeGuard)
{
	if (!includeGuard.insert(fs::absolute(file)).second)
	{
		LLOGW("Circular include detected: %s\n", file.string().c_str());
		return {};
	}

	std::ifstream in(file, std::ios::binary);
	if (!in)
	{
		LLOGW("Failed to open shader file %s\n", file.string().c_str());
		return {};
	}

	std::string code;
	in.seekg(0, std::ios::end);
	code.reserve(in.tellg());
	in.seekg(0, std::ios::beg);
	code.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

	// Remove UTF-8 BOM
	if (code.size() >= 3 &&
		static_cast<unsigned char>(code[0]) == 0xEF &&
		static_cast<unsigned char>(code[1]) == 0xBB &&
		static_cast<unsigned char>(code[2]) == 0xBF)
	{
		code.erase(0, 3);
	}

	// Handle #include <file>
	size_t pos = 0;
	while ((pos = code.find("#include", pos)) != std::string::npos)
	{
		const auto start = code.find('<', pos);
		const auto end = code.find('>', start);

		if (start == std::string::npos || end == std::string::npos)
			break;

		const fs::path includePath = file.parent_path() / code.substr(start + 1, end - start - 1);

		const std::string includeCode = readTextFile(includePath, includeGuard);

		code.replace(pos, end - pos + 1, includeCode);
	}

	return code;
}

inline std::string readShaderFile(const fs::path& file)
{
	std::unordered_set<fs::path> guard;
	return readTextFile(file, guard);
}

inline lvk::ShaderStage shaderStageFromPath(const fs::path& file)
{
	const auto extension = file.extension().string();

	if (extension == ".vert") return lvk::Stage_Vert;
	if (extension == ".frag") return lvk::Stage_Frag;
	if (extension == ".geom") return lvk::Stage_Geom;
	if (extension == ".comp") return lvk::Stage_Comp;
	if (extension == ".tesc") return lvk::Stage_Tesc;
	if (extension == ".tese") return lvk::Stage_Tese;

	LLOGW("Unknown shader extension: %s\n", extension.c_str());
	return lvk::Stage_Vert;
}

inline void saveSPIRV(const fs::path& file, std::span<const uint8_t> data)
{
	std::ofstream out(file, std::ios::binary);
	out.write(reinterpret_cast<const char*>(data.data()), data.size());
}

inline bool compileShaderToSPIRV(const fs::path& source, const fs::path& dest)
{
	const std::string code = readShaderFile(source);
	if (code.empty())
		return false;

	std::vector<uint8_t> spirv;
	const lvk::Result result = lvk::compileShaderGlslang(shaderStageFromPath(source), code.c_str(), &spirv, glslang_default_resource());

	if (!result.isOk() || spirv.empty())
	{
		LLOGW("Shader compilation failed: %s\n", source.string().c_str());
		return false;
	}

	saveSPIRV(dest, spirv);
	return true;
}

inline lvk::Holder<lvk::ShaderModuleHandle> loadShaderModule(const std::unique_ptr<lvk::IContext>& ctx, const std::filesystem::path& file)
{
	const std::string code = readShaderFile(file);
	const lvk::ShaderStage stage = shaderStageFromPath(file);

	if (code.empty())
		return {};

	lvk::Result result;

	/**
	* glCreateShader
	* glCompileShader
	* glLinkProgram
	*/
	lvk::Holder<lvk::ShaderModuleHandle> handle = ctx->createShaderModule({ code.c_str(), stage, (std::string("Shader module : ") + file.string()).c_str() }, &result);

	if (!result.isOk())
		return {};

	std::cout << "Loaded Shader Module" << std::endl;
	return handle;
}

