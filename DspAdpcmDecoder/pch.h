#pragma once

#include <cstdint>
#include <cassert>
#include <vector>
#include <list>
#include <map>
#include <string>
#include <unordered_map>

#include <Windows.h>

#include "../../dolwin/SRC/Common/Spinlock.h"
#include "../../dolwin/SRC/Common/Thread.h"
#include "../../dolwin/SRC/Common/File.h"
#include "../../dolwin/SRC/Common/Json.h"

#include "../../dolwin/SRC/DSP/DSP.h"

#include "../../dolwin/SRC/Debugger/Debugger.h"

#include "../../dolwin/SRC/GekkoCore/Gekko.h"
#include "../../dolwin/SRC/Hardware/HWConfig.h"
#include "../../dolwin/SRC/Hardware/MI.h"
#include "../../dolwin/SRC/Hardware/AR.h"
