
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

// harmonic band 1

float HarmonicBasis1(float3 v) {
	return HARMONIC_COEFFICIENT1 * v.y;
}

float HarmonicBasis2(float3 v) {
	return HARMONIC_COEFFICIENT1 * v.z;
}

float HarmonicBasis3(float3 v) {
	return HARMONIC_COEFFICIENT1 * v.x;
}

// harmonic band 2

float HarmonicBasis4(float3 v) {
	return HARMONIC_COEFFICIENT2 * v.x * v.y;
}

float HarmonicBasis5(float3 v) {
	return HARMONIC_COEFFICIENT2 * v.y * v.z;
}

float HarmonicBasis6(float3 v) {
	return HARMONIC_COEFFICIENT3 * -(v.x * v.x + v.y * v.y - 2.f * v.z * v.z);
}

float HarmonicBasis7(float3 v) {
	return HARMONIC_COEFFICIENT2 * v.z * v.x;
}

float HarmonicBasis8(float3 v) {
	return HARMONIC_COEFFICIENT2 * 0.5f * (v.x * v.x - v.y * v.y);
}



#ifndef NUM_COEFS
#define NUM_COEFS 9
#endif



float HarmonicBasis(float3 vec) {
	


}

