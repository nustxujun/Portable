#include "Material.h"

Material::Material()
{
}

Material::~Material()
{
}

void Material::setTexture(size_t index, Renderer::Texture2D::Ptr tex)
{
	if (textures.size() <= index)
		textures.resize(index + 1);

	textures[index] = tex;
}

size_t Material::getShaderID() 
{
	if (mShaderID == -1)
		generateShaderID();
	return mShaderID;
}

std::vector<D3D10_SHADER_MACRO> Material::generateShaderID()
{
	std::vector<D3D10_SHADER_MACRO> macros;
	auto& texs = textures;
	if (!texs.empty())
	{
		macros.push_back({ "ALBEDO","1" });
		if (texs.size() > Material::TU_NORMAL &&
			!texs[Material::TU_NORMAL].expired())
		{
			macros.push_back({ "NORMAL_MAP","1" });
		}
		if (texs.size() > Material::TU_ROUGH && !texs[Material::TU_ROUGH].expired())
		{
			macros.push_back({ "PBR_MAP","1" });
		}

		if (texs.size() > Material::TU_AO && !texs[Material::TU_AO].expired())
		{
			macros.push_back({ "AO_MAP","1" });
		}
	}

	macros.push_back({ NULL, NULL });

	mShaderID = Common::hash(macros.data(), macros.size() * sizeof(D3D10_SHADER_MACRO));

	return macros;
}

