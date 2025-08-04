#if !defined(MOONCASTLE_COMMON_HLSLI) && !defined(__cplusplus)

#error Do not include this file directly in the shader files. Only do it via Common.hlsli.

#endif

static const float PI = 3.1415926535897932384626433832795f;

//Light types
//These have to be the same as mooncastle::graphics::light::type enumeration!
static const uint LIGHT_TYPE_DIRECTIONAL_LIGHT = 0;
static const uint LIGHT_TYPE_POINT_LIGHT = 1;
static const uint LIGHT_TYPE_SPOTLIGHT = 2;
