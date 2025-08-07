float Pow5(float x)
{
    float xx = x * x;
    return xx * xx * x;
}

float DGGX(float NoH, float a)
{
    float d = (NoH * a - NoH) * NoH + 1;
    return a / (PI * d * d);
}

float VSmithGGXCorrelated(float NoV, float NoL, float a)
{
    float GGXL = NoV * sqrt((-NoL * a + NoL) * NoL + a);
    float GGXV = NoL * sqrt((-NoV * a + NoV) * NoV + a);
    return 0.5f / (GGXV + GGXL);
}

float VSmithGGXCorrelatedApprox(float NoV, float NoL, float a)
{
    float GGXV = NoL * ((-NoV * a + NoV) + a);
    float GGXL = NoV * ((-NoL * a + NoL) + a);
    return 0.5f / (GGXV + GGXL);
}

float3 FSchlick(float3 F0, float VoH)
{
    float u = Pow5(1.f - VoH);
    float3 F90Approx = saturate(50.f * F0.g);
    return F90Approx * u + (1 - u) * F0;
}

float3 FSchlick(float u, float3 f0, float3 f90)
{
    return f0 + (f90 - f0) * Pow5(1.f - u);
}

float3 DiffuseLambert()
{
    return 1 / PI;
}

float3 DiffuseBurley(float NoV, float NoL, float VoH, float roughness)
{
    float u = Pow5(1.f - NoV);
    float v = Pow5(1.f - NoL);

    float FD90 = 0.5f + 2.f * VoH * VoH * roughness;
    float FdV = 1.f + (u * FD90 - u);
    float FdL = 1.f + (v * FD90 - v);
    return (1.f / PI) * FdV * FdL;
}