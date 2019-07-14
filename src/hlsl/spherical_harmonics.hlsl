
// sqrt(1/pi)/2
#define HARMONIC_COEFFICIENT0 0.28209479177387814347403972578039

// sqrt(3/pi)/2
#define HARMONIC_COEFFICIENT1 0.48860251190291992158638462283835

// sqrt(15/pi)/2
#define HARMONIC_COEFFICIENT2 1.0925484305920790705433857058027

// sqrt(5/pi)/4
#define HARMONIC_COEFFICIENT3 0.31539156525252000603089369029571

// harmonic band 0

float HarmonicBasis0() {
	return HARMONIC_COEFFICIENT0;
}


#ifndef DEGREE
#define DEGREE 1
#endif
#define NUM_COEFS (DEGREE + 1) * (DEGREE + 1)

#define PI 3.14159265358f

void HarmonicBasis(float3 vec, out float basis[NUM_COEFS]) {
	float x = vec.x;
	float y = vec.y;
	float z = vec.z;

	float x2 = x * x;
	float y2 = y * y;
	float z2 = z * z;

#if (DEGREE >= 0)
	basis[0] = 1.f / 2.f * sqrt(1.f / PI);
#endif

	
#if (DEGREE >= 1)
	basis[1] = sqrt(3.f / (4.f*PI))*y;
	basis[2] = sqrt(3.f / (4.f*PI))*z;
	basis[3] = sqrt(3.f / (4.f*PI))*x ;
#endif

#if (DEGREE >= 2)
	basis[4] = 1.f / 2.f * sqrt(15.f / PI) * x * y;
	basis[5] = 1.f / 2.f * sqrt(15.f / PI) * y * z;
	basis[6] = 1.f / 4.f * sqrt(5.f / PI) * (-x * x - y * y + 2 * z*z);
	basis[7] = 1.f / 2.f * sqrt(15.f / PI) * z * x ;
	basis[8] = 1.f / 4.f * sqrt(15.f / PI) * (x*x - y * y) ;
#endif

#if (DEGREE >= 3)
	basis[9] = 1.f / 4.f*sqrt(35.f / (2.f*PI))*(3 * x2 - y2)*y ;
	basis[10] = 1.f / 2.f*sqrt(105.f / PI)*x*y*z;
	basis[11] = 1.f / 4.f*sqrt(21.f / (2.f*PI))*y*(4 * z2 - x2 - y2) ;
	basis[12] = 1.f / 4.f*sqrt(7.f / PI)*z*(2 * z2 - 3 * x2 - 3 * y2) ;
	basis[13] = 1.f / 4.f*sqrt(21.f / (2.f*PI))*x*(4 * z2 - x2 - y2) ;
	basis[14] = 1.f / 4.f*sqrt(105.f / PI)*(x2 - y2)*z;
	basis[15] = 1.f / 4.f*sqrt(35.f / (2 * PI))*(x2 - 3 * y2)*x ;
#endif
}

void HarmonicEvaluate(float3 vec, float3 color, out float3 coefs[NUM_COEFS])
{
	float basis[NUM_COEFS];
	HarmonicBasis(vec, basis);
	for (int i = 0; i < NUM_COEFS; ++i)
	{
		coefs[i] +=  color * basis[i];
	}
}