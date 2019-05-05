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
};
