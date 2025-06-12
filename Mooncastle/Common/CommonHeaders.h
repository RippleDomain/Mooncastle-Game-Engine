#pragma once

#pragma warning(disable: 4530) //Disable exception warnings

//C/C++
#include <stdint.h>
#include <assert.h>
#include <typeinfo>
#include <memory>

#if defined(_WIN64)
#include <DirectXMath.h>
#endif

//Common
#include "PrimitiveTypes.h"
#include "..\Utilities\Utilities.h"
#include "..\Utilities\MathTypes.h"