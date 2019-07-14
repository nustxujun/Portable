#pragma once

#include "Common.h"
#include "renderer.h"
#include "D3D11Helper.h"

#define PI 3.14159265358f

class SphericalHarmonics
{
public:

	template<size_t D>
	static void getBasis(const DirectX::SimpleMath::Vector3& vec, float* basis);

	template<>
	static void getBasis<0>(const DirectX::SimpleMath::Vector3& vec, float* basis)
	{
		basis[0] = { 1.f / 2.f * std::sqrt(1.f / PI) };
	}

	template<>
	static void getBasis<1>(const DirectX::SimpleMath::Vector3& vec, float* basis)
	{
		float x = vec.x;
		float y = vec.y;
		float z = vec.z;

		getBasis<0>(vec, basis);
		basis[1] = sqrt(3.f / (4.f*PI))*y;
		basis[2] = sqrt(3.f / (4.f*PI))*z;
		basis[3] = sqrt(3.f / (4.f*PI))*x;

	}

	template<>
	static void getBasis<2>(const DirectX::SimpleMath::Vector3& vec, float* basis)
	{
		float x = vec.x;
		float y = vec.y;
		float z = vec.z;

		getBasis<1>(vec, basis);
		basis[4] = 1.f / 2.f * sqrt(15.f / PI) * x * y;
		basis[5] = 1.f / 2.f * sqrt(15.f / PI) * y * z;
		basis[6] = 1.f / 4.f * sqrt(5.f / PI) * (-x * x - y * y + 2 * z*z);
		basis[7] = 1.f / 2.f * sqrt(15.f / PI) * z * x;
		basis[8] = 1.f / 4.f * sqrt(15.f / PI) * (x*x - y * y);
	}

	template<>
	static void getBasis<3>(const DirectX::SimpleMath::Vector3& vec, float* basis)
	{
		float x = vec.x;
		float y = vec.y;
		float z = vec.z;
		float x2 = x * x;
		float y2 = y * y;
		float z2 = z * z;

		getBasis<2>(vec, basis);
		basis[9] = 1.f / 4.f*sqrt(35.f / (2.f*PI))*(3 * x2 - y2)*y;
		basis[10] = 1.f / 2.f*sqrt(105.f / PI)*x*y*z;
		basis[11] = 1.f / 4.f*sqrt(21.f / (2.f*PI))*y*(4 * z2 - x2 - y2);
		basis[12] = 1.f / 4.f*sqrt(7.f / PI)*z*(2 * z2 - 3 * x2 - 3 * y2);
		basis[13] = 1.f / 4.f*sqrt(21.f / (2.f*PI))*x*(4 * z2 - x2 - y2);
		basis[14] = 1.f / 4.f*sqrt(105.f / PI)*(x2 - y2)*z;
		basis[15] = 1.f / 4.f*sqrt(35.f / (2 * PI))*(x2 - 3 * y2)*x;
	}

	template<size_t D>
	static void evaluate(const Vector3& normal, const Vector3& color,Vector3* coefs)
	{
		auto constexpr NUM_COEFS = (D + 1) * (D + 1);
		float basis[NUM_COEFS];
		getBasis<D>(normal, basis);
		for (size_t i = 0; i < NUM_COEFS; ++i)
		{
			coefs[i] += color * basis[i];
		}
	}

	template<size_t D>
	static std::vector<Vector3> precompute(const std::array<std::vector<Vector4>, 6>& tex, size_t length)
	{
		auto constexpr NUM_COEFS = (D + 1) * (D + 1);
		std::vector<Vector3> coefs(NUM_COEFS);

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
			const Vector4* data = tex[face].data();
			for (UINT y = 0; y < length; ++y)
			{
				for (UINT x = 0; x < length; ++x)
				{
					const Vector4* color = (data + length* y) + x ;
					Vector3 pos = convUVToPos(face, (float)x / (float)(length - 1), (float)y / (float)(length - 1));
					pos.Normalize();
					evaluate<D>(pos, *(const Vector3*)color, coefs.data());
					samplecount++;
				}
			}
		}

		for (auto& c : coefs)
		{
			c = c * 4 * PI / (float)samplecount;
		}
		return coefs;
	}


};

