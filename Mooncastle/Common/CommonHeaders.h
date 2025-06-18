#pragma once

#pragma warning(disable: 4530) //Disable exception warnings

//C/C++
#include <stdint.h>
#include <assert.h>
#include <typeinfo>
#include <memory>
#include <unordered_map>

#if defined(_WIN64)
#include <DirectXMath.h>
#endif

//Common
#include "PrimitiveTypes.h"
#include "..\Utilities\Utilities.h"
#include "..\Utilities\MathTypes.h"
#include "Id.h"

//Macro
#ifdef _DEBUG
#define DEBUG_OP(x) x
#else 
#define DEBUG_OP(x) (void(0))
#endif