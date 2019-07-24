#include "SeparableSSS.h"

std::vector<Vector4> SeparableSSS::CalculateKernel(int nSamples, const Vector3 & strength, const Vector3 & falloff)
{
	std::vector<Vector4> kernel;
	float RANGE = nSamples > 20 ? 3.0f : 2.0f;
	float EXPONENT = 2.0f;

	// Calculate the SSS_Offset_UV:
	float step = 2.0f * RANGE / (nSamples - 1);
	for (int i = 0; i < nSamples; i++) {
		float o = -RANGE + i * step;
		float sign = o < 0.0f ? -1.0f : 1.0f;
		float w = RANGE * sign * std::abs(std::pow(o, EXPONENT)) / std::pow(RANGE, EXPONENT);
		kernel.push_back(Vector4(0, 0, 0, w));
	}
	// Calculate the SSS_Scale:
	for (int i = 0; i < nSamples; i++) {
		float w0 = i > 0 ? std::abs(kernel[i].w - kernel[i - 1].w) : 0.0f;
		float w1 = i < nSamples - 1 ? std::abs(kernel[i].w - kernel[i + 1].w) : 0.0f;
		float area = (w0 + w1) / 2.0f;
		Vector3 temp = profile(kernel[i].w, falloff);
		Vector4 tt = Vector4(area * temp.x, area * temp.y, area * temp.z, kernel[i].w);
		kernel[i] = tt;
	}
	Vector4 t = kernel[nSamples / 2];
	for (int i = nSamples / 2; i > 0; i--)
		kernel[i] = kernel[i - 1];
	kernel[0] = t;
	Vector4 sum = Vector4::Zero;

	for (int i = 0; i < nSamples; i++) {
		sum.x += kernel[i].x;
		sum.y += kernel[i].y;
		sum.z += kernel[i].z;
	}

	for (int i = 0; i < nSamples; i++) {
		Vector4 vecx = kernel[i];
		vecx.x /= sum.x;
		vecx.y /= sum.y;
		vecx.z /= sum.z;
		kernel[i] = vecx;
	}

	Vector4 vec = kernel[0];
	vec.x = (1.0f - strength.x) * 1.0f + strength.x * vec.x;
	vec.y = (1.0f - strength.y) * 1.0f + strength.y * vec.y;
	vec.z = (1.0f - strength.z) * 1.0f + strength.z * vec.z;
	kernel[0] = vec;

	for (int i = 1; i < nSamples; i++) {
		auto vect = kernel[i];
		vect.x *= strength.x;
		vect.y *= strength.y;
		vect.z *= strength.z;
		kernel[i] = vect;
	}
}

Vector3 SeparableSSS::gaussian(float variance, float r, const Vector3 & falloff)
{
	Vector3 g;
	float rr1 = r / (0.001f + falloff.x);
	g.x = std::exp((-(rr1 * rr1)) / (2.0f * variance)) / (2.0f * 3.14f * variance);

	float rr2 = r / (0.001f + falloff.y);
	g.y = std::exp((-(rr2 * rr2)) / (2.0f * variance)) / (2.0f * 3.14f * variance);

	float rr3 = r / (0.001f + falloff.z);
	g.z = std::exp((-(rr3 * rr3)) / (2.0f * variance)) / (2.0f * 3.14f * variance);

	return g;
}

Vector3 SeparableSSS::profile(float r, const Vector3 & falloff)
{
	return  0.100f * gaussian(0.0484f, r, falloff) +
		0.118f * gaussian(0.187f, r, falloff) +
		0.113f * gaussian(0.567f, r, falloff) +
		0.358f * gaussian(1.99f, r, falloff) +
		0.078f * gaussian(7.41f, r, falloff);
}
