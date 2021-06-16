
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <cassert>
#include <atomic>
#include <unordered_map>

#include "../../dolwin/SRC/Common/ByteSwap.h"
#include "../../dolwin/SRC/Common/Spinlock.h"
#include "../../dolwin/SRC/Common/Thread.h"
#include "../../dolwin/SRC/Common/Json.h"
#include "../../dolwin/SRC/Common/Jdi.h"
#include "../../dolwin/SRC/Common/String.h"

#include "../../dolwin/SRC/IntelCore/IntelCore.h"

#include "../../dolwin/SRC/GekkoCore/Gekko.h"
#include "../../dolwin/SRC/GekkoCore/GekkoDisasm.h"
#include "../../dolwin/SRC/GekkoCore/GekkoAssembler.h"
#include "../../dolwin/SRC/GekkoCore/Interpreter.h"
#include "../../dolwin/SRC/GekkoCore/Jitc.h"

#include "../../dolwin/SRC/Hardware/HWConfig.h"
#include "../../dolwin/SRC/Hardware/PI.h"
#include "../../dolwin/SRC/Hardware/MI.h"

#include "../../dolwin/SRC/Debugger/Debugger.h"
