#pragma once

#include "Common.h"
#include "renderer.h"
#include "D3D11Helper.h"

class SphericalHarmonics
{
	static const float PI;
public:

	template<size_t D>
	std::vector<float> getBasis(const DirectX::SimpleMath::Vector3& vec);

	template<>
	static std::vector<float> getBasis<0>(const DirectX::SimpleMath::Vector3& vec)
	{
		return { 1.f / 2.f * std::sqrt(1.f / PI) };
	}

	template<>
	static std::vector<float> getBasis<1>(const DirectX::SimpleMath::Vector3& vec)
	{
		float x = vec.x;
		float y = vec.y;
		float z = vec.z;

		auto basis = getBasis<0>(vec);
		basis.resize(4);
		basis[1] = sqrt(3.f / (4.f*PI))*y;
		basis[2] = sqrt(3.f / (4.f*PI))*z;
		basis[3] = sqrt(3.f / (4.f*PI))*x;

		return basis;
	}

	template<>
	static std::vector<float> getBasis<2>(const DirectX::SimpleMath::Vector3& vec)
	{
		float x = vec.x;
		float y = vec.y;
		float z = vec.z;

		auto basis = getBasis<1>(vec);
		basis.resize(9);
		basis[4] = 1.f / 2.f * sqrt(15.f / PI) * x * y;
		basis[5] = 1.f / 2.f * sqrt(15.f / PI) * y * z;
		basis[6] = 1.f / 4.f * sqrt(5.f / PI) * (-x * x - y * y + 2 * z*z);
		basis[7] = 1.f / 2.f * sqrt(15.f / PI) * z * x;
		basis[8] = 1.f / 4.f * sqrt(15.f / PI) * (x*x - y * y);
		return basis;
	}

	template<>
	static std::vector<float> getBasis<3>(const DirectX::SimpleMath::Vector3& vec)
	{
		float x = vec.x;
		float y = vec.y;
		float z = vec.z;
		float x2 = x * x;
		float y2 = y * y;
		float z2 = z * z;

		auto basis = getBasis<2>(vec);
		basis.resize(16);
		basis[9] = 1.f / 4.f*sqrt(35.f / (2.f*PI))*(3 * x2 - y2)*y;
		basis[10] = 1.f / 2.f*sqrt(105.f / PI)*x*y*z;
		basis[11] = 1.f / 4.f*sqrt(21.f / (2.f*PI))*y*(4 * z2 - x2 - y2);
		basis[12] = 1.f / 4.f*sqrt(7.f / PI)*z*(2 * z2 - 3 * x2 - 3 * y2);
		basis[13] = 1.f / 4.f*sqrt(21.f / (2.f*PI))*x*(4 * z2 - x2 - y2);
		basis[14] = 1.f / 4.f*sqrt(105.f / PI)*(x2 - y2)*z;
		basis[15] = 1.f / 4.f*sqrt(35.f / (2 * PI))*(x2 - 3 * y2)*x;
		return basis;
	}

	template<size_t D>
	static void evaluate(const Vector3& normal, const Vector3& color, std::vector<Vector3>& coefs)
	{
		auto basis = getBasis<D>(normal);
		auto constexpr NUM_COEFS = (D + 1) * (D + 1);
		for (size_t i = 0; i < NUM_COEFS; ++i)
		{
			coefs[i] += color * basis[i];
		}
	}

	template<size_t D>
	static std::vector<Vector3> precompute(Renderer::Texture2D::Ptr cube, Renderer::Ptr renderer)
	{
		auto constexpr NUM_COEFS = (D + 1) * (D + 1);
		std::vector<Vector3> coefs(NUM_COEFS);
		auto desc = cube->getDesc();
		auto tex = cube;
		Renderer::TemporaryRT::Ptr temp;

		{
			desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			desc.BindFlags = 0;
			desc.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
			desc.Usage = D3D11_USAGE_STAGING;
			temp = renderer->createTemporaryRT(desc);
			tex = temp->get();
			renderer->getContext()->CopyResource(tex->getTexture(), cube->getTexture());
		}

		auto pitch = D3D11Helper::sizeof_DXGI_FORMAT(desc.Format)  * desc.Width;
		D3D11_MAPPED_SUBRESOURCE subresource;
		auto context = renderer->getContext();
		auto convUVToPos = [](int index, float u, float v)->Vector3
		{
			u = u * 2.0f - 1.0f;
			v = 1 - v * 2.0f;

			switch (index)
			{
			case 0: return { 1,  v, -u }; 	// +x
			case 1: return { -1,  v,  u }; 	// -x
			case 2: return { u,  1, -v };  // +y
			case 3: return { u, -1,  v };	// -y
			case 4: return { u,  v,  1 };  // +z
			case 5: return { -u,  v, -1 };	// -z
			}

			return {};
		};
		size_t samplecount = 0;
		for (int face = 0; face < 6; ++face)
		{
			auto index = D3D10CalcSubresource(0, face, desc.MipLevels);
			HRESULT hr = context->Map(tex->getTexture(), index, D3D11_MAP_READ, 0, &subresource);

			for (UINT y = 0; y < desc.Height; ++y)
			{
				for (UINT x = 0; x < desc.Width; ++x)
				{
					const Vector4* color = (const Vector4*)((const char*)subresource.pData + subresource.RowPitch * y) + x ;
					Vector3 pos = convUVToPos(face, (float)x / (float)(desc.Width - 1), (float)y / (float)(desc.Height - 1));
					pos.Normalize();
					evaluate<D>(pos, *(const Vector3*)color, coefs);
					samplecount++;
				}
			}

			context->Unmap(tex->getTexture(), index);
		}

		for (auto& c : coefs)
		{
			c = c * 4 * PI / (float)samplecount;
		}
		return coefs;
	}


};

const float SphericalHarmonics::PI = 3.14159265358f;