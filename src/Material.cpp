#include "Material.h"

Material::Ptr Material::Default = Material::create();

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
		macros.push_back({ "UV", "1" });
		for (size_t i = TU_ALBEDO; i < TU_MAX; ++i)
		{
			if (texs.size() <= i ||
				texs[i].expired())
				continue;

			switch (i)
			{
			case TU_ALBEDO: macros.push_back({ "ALBEDO","1" }); break;
			case TU_NORMAL: macros.push_back({ "NORMAL_MAP","1" }); break;
			case TU_ROUGH: macros.push_back({ "PBR_MAP","1" }); break;
			//case TU_METAL: macros.push_back({ "ALBEDO","1" }); break;
			case TU_AO: macros.push_back({ "AO_MAP","1" }); break;
			case TU_HEIGHT: macros.push_back({ "HEIGHT_MAP","1" }); break;
			case TU_BUMP: macros.push_back({ "BUMP_MAP","1" }); break;
			}
		}
	}

	if (usingVertexColor)
	{
		macros.push_back({ "DIFFUSE", "1" });
	}

	macros.push_back({ NULL, NULL });

	mShaderID = Common::hash(macros.data(), macros.size() * sizeof(D3D10_SHADER_MACRO));

	return macros;
}

