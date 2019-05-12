#pragma once

#include "SimpleMath.h"

using namespace DirectX::SimpleMath;

class MathUtilities
{
public :
	static Matrix makeViewMatrix(const Vector3& pos, const Quaternion& orin)
	{
		Matrix rot;
		rot = Matrix::CreateFromQuaternion(orin);
		auto rotT = rot.Transpose();
		auto trans = Vector3::Transform(-pos, rotT);
		Matrix view = rotT;
		view.Translation(trans);

		//using namespace DirectX;
		//Matrix mat = XMMatrixLookToLH(pos, getDirection(), Vector3(0, 1, 0));
		return view;
	}

	static Matrix makeViewMatrix(const Vector3& pos, const Vector3& dir)
	{
		Vector3 up(0, 1, 0);
		if (fabs(up.Dot(dir)) == 1.0f)
			up = { 0 ,0, 1 };

		return DirectX::XMMatrixLookToLH(pos, dir, up);
	}

	static Matrix makeMatrixFromAxis(const Vector3& x, const Vector3& y, const Vector3& z)
	{
		Matrix mat = Matrix::Identity;
		mat._11 = x.x;
		mat._12 = x.y;
		mat._13 = x.z;

		mat._21 = y.x;
		mat._22 = y.y;
		mat._23 = y.z;

		mat._31 = z.x;
		mat._32 = z.y;
		mat._33 = z.z;

		return mat;
	}

	static std::array<Vector3, 8> calFrustumCorners( float width, float height, float neardist, float fardist,float fovy, bool perspective = true )
	{
		float halfW, halfH;
		if (perspective)
		{
			float thetaY(fovy * 0.5f);
			float tanThetaY = std::tan(thetaY);
			float tanThetaX = tanThetaY * (width / height);

			halfW = tanThetaX * neardist;
			halfH = tanThetaY * neardist;
		}
		else
		{
			halfW = width * 0.5;
			halfH = height * 0.5;
		}

		std::array<Vector3,8> ret;

		float nearLeft = -halfW, nearRight = halfW, nearBottom = -halfH, nearTop = halfH;

		// Calc far palne corners
		float radio = perspective ? fardist / neardist : 1;
		float farLeft = nearLeft * radio;
		float farRight = nearRight * radio;
		float farBottom = nearBottom * radio;
		float farTop = nearTop * radio;

		// near
		ret[0] = Vector3(nearRight, nearTop, neardist);
		ret[1] = Vector3(nearLeft, nearTop, neardist);
		ret[2] = Vector3(nearLeft, nearBottom, neardist);
		ret[3] = Vector3(nearRight, nearBottom, neardist);
		// far
		ret[4] = Vector3(farRight, farTop, fardist);
		ret[5] = Vector3(farLeft, farTop, fardist);
		ret[6] = Vector3(farLeft, farBottom, fardist);
		ret[7] = Vector3(farRight, farBottom, fardist);

		return ret;
	}
};
