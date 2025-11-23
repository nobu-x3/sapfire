#include "engpch.h"

#include "core/file_system.h"
#include "core/logger.h"
#include "render/dx12/dx12_util.h"
#include "render/dx12/dx12_shader_compiler.h"

namespace sf::render::dx12 {

	using namespace Microsoft::WRL;

	// Responsible for the actual compilation of shaders.
	ComPtr<IDxcCompiler3> compiler{};

	// Used to create include handle and provides interfaces for loading shader
	// to blob, etc.
	ComPtr<IDxcUtils> utils{};
	ComPtr<IDxcIncludeHandler> includeHandler{};

	std::wstring shader_directory{};

	Shader compile(const ShaderType& type, const stl::string_view path, const stl::string_view entry_point,
				   const bool extract_root_signature /*= false*/) {
		Shader shader{};
		if (!utils) {
			dx12_check(::DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils)));
			dx12_check(::DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));
			dx12_check(utils->CreateDefaultIncludeHandler(&includeHandler));
			shader_directory = fs::full_path(L"assets/shaders");
			CORE_INFO("Shader base directory: {}", wstring_to_ansi(shader_directory));
		}
		CORE_INFO("Compiling shader at path: {}", path);
		std::wstring shader_path = ansi_to_wstring(stl::tstring{path});
		// Setup compilation arguments.
		const std::wstring target_profile = [=]() {
			switch (type) {
			case ShaderType::Vertex:
				{
					return L"vs_6_6";
				}
				break;
			case ShaderType::Pixel:
				{
					return L"ps_6_6";
				}
				break;
			case ShaderType::Compute:
				{
					return L"cs_6_6";
				}
				break;
			default:
				{
					return L"";
				}
				break;
			}
		}();
		std::wstring entry = ansi_to_wstring(stl::tstring{entry_point});
		stl::tvector<LPCWSTR> compilation_arguments = {
			L"-HV",
			L"2021",
			L"-E",
			entry.data(),
			L"-T",
			target_profile.c_str(),
			DXC_ARG_PACK_MATRIX_ROW_MAJOR,
			DXC_ARG_WARNINGS_ARE_ERRORS,
			DXC_ARG_ALL_RESOURCES_BOUND,
			L"-I",
			shader_directory.c_str(),
#if defined(DEBUG) | defined(_DEBUG)
			DXC_ARG_DEBUG
#else
			DXC_ARG_OPTIMIZATION_LEVEL3
#endif
		};
		// Load the shader source file to a blob.
		ComPtr<IDxcBlobEncoding> source_blob{nullptr};
		dx12_check(utils->LoadFile(shader_path.data(), nullptr, &source_blob));
		const DxcBuffer sourceBuffer = {
			.Ptr = source_blob->GetBufferPointer(),
			.Size = source_blob->GetBufferSize(),
			.Encoding = 0u,
		};
		// Compile the shader.
		ComPtr<IDxcResult> compiled_shader_buffer{};
		const HRESULT hr =
			compiler->Compile(&sourceBuffer, compilation_arguments.data(), static_cast<uint32_t>(compilation_arguments.size()),
							  includeHandler.Get(), IID_PPV_ARGS(&compiled_shader_buffer));
		if (FAILED(hr)) {
			CORE_CRITICAL("Failed to compile shader with path : {}", path);
		}
		// Get compilation errors (if any).
		ComPtr<IDxcBlobUtf8> errors{};
		dx12_check(compiled_shader_buffer->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr));
		if (errors && errors->GetStringLength() > 0) {
			const LPCSTR errorMessage = errors->GetStringPointer();
			CORE_CRITICAL("Shader path : {}, Error : {}", path, errorMessage);
		}
		ComPtr<IDxcBlob> compiled_shader_blob{nullptr};
		compiled_shader_buffer->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&compiled_shader_blob), nullptr);
		shader.shader_blob = compiled_shader_blob;
		ComPtr<IDxcBlob> root_signature_blob{nullptr};
		if (extract_root_signature) {
			compiled_shader_buffer->GetOutput(DXC_OUT_ROOT_SIGNATURE, IID_PPV_ARGS(&root_signature_blob), nullptr);
			shader.root_signature_blob = root_signature_blob;
		}
		return shader;
	}

	Shader compile(const ShaderType& type, const stl::wstring_view path, const stl::wstring_view entry,
				   const bool extract_root_signature) {
		Shader shader{};
		if (!utils) {
			dx12_check(::DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils)));
			dx12_check(::DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));
			dx12_check(utils->CreateDefaultIncludeHandler(&includeHandler));
			shader_directory = fs::full_path(L"assets/shaders");
			if (shader_directory.empty()) {
				CORE_WARN("Filesystem failed to find shader directory in assets/shaders. Taking a wild guess.");
				shader_directory = ansi_to_wstring(fs::FileSystem::root_directory()) + L"assets/shaders";
			}
			CORE_INFO("Shader base directory: {}", wstring_to_ansi(shader_directory));
		}
        std::wstring full_path = shader_directory + L"/" + std::wstring{path};
        std::string path_str = wstring_to_ansi(full_path);
		CORE_INFO("Compiling shader at path: {}", path_str);
		// Setup compilation arguments.
		const std::wstring target_profile = [=]() {
			switch (type) {
			case ShaderType::Vertex:
				{
					return L"vs_6_6";
				}
				break;
			case ShaderType::Pixel:
				{
					return L"ps_6_6";
				}
				break;
			case ShaderType::Compute:
				{
					return L"cs_6_6";
				}
				break;
			default:
				{
					return L"";
				}
				break;
			}
		}();
		stl::tvector<LPCWSTR> compilation_arguments = {
			L"-HV",
			L"2021",
			L"-E",
			entry.data(),
			L"-T",
			target_profile.c_str(),
			DXC_ARG_PACK_MATRIX_ROW_MAJOR,
			DXC_ARG_WARNINGS_ARE_ERRORS,
			DXC_ARG_ALL_RESOURCES_BOUND,
			L"-I",
			shader_directory.c_str(),
#if defined(DEBUG) | defined(_DEBUG)
			DXC_ARG_DEBUG
#else
			DXC_ARG_OPTIMIZATION_LEVEL3
#endif
		};
		// Load the shader source file to a blob.
		ComPtr<IDxcBlobEncoding> source_blob{nullptr};
		dx12_check(utils->LoadFile(full_path.data(), nullptr, &source_blob));
		const DxcBuffer sourceBuffer = {
			.Ptr = source_blob->GetBufferPointer(),
			.Size = source_blob->GetBufferSize(),
			.Encoding = 0u,
		};
		// Compile the shader.
		ComPtr<IDxcResult> compiled_shader_buffer{};
		const HRESULT hr =
			compiler->Compile(&sourceBuffer, compilation_arguments.data(), static_cast<uint32_t>(compilation_arguments.size()),
							  includeHandler.Get(), IID_PPV_ARGS(&compiled_shader_buffer));
		if (FAILED(hr)) {
			CORE_CRITICAL("Failed to compile shader with path : {}", path_str);
		}
		// Get compilation errors (if any).
		ComPtr<IDxcBlobUtf8> errors{};
		dx12_check(compiled_shader_buffer->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr));
		if (errors && errors->GetStringLength() > 0) {
			const LPCSTR errorMessage = errors->GetStringPointer();
			CORE_CRITICAL("Shader path : {}, Error : {}", path_str, errorMessage);
		}
		ComPtr<IDxcBlob> compiled_shader_blob{nullptr};
		compiled_shader_buffer->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&compiled_shader_blob), nullptr);
		shader.shader_blob = compiled_shader_blob;
		ComPtr<IDxcBlob> root_signature_blob{nullptr};
		if (extract_root_signature) {
			compiled_shader_buffer->GetOutput(DXC_OUT_ROOT_SIGNATURE, IID_PPV_ARGS(&root_signature_blob), nullptr);
			shader.root_signature_blob = root_signature_blob;
		}
		return shader;
	}

} // namespace sf::tools::shader_compiler
