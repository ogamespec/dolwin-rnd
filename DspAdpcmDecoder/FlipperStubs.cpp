#include "pch.h"

MIControl mi;

ARControl aram;

namespace Flipper
{
    DSP::Dsp16* DSP;      // Instance of dsp core

    void DSPAssertInt()
    {
    }

    bool DSPGetInterruptStatus()
    {
        return false;
    }

    bool DSPGetResetModifier()
    {
        return false;
    }
}
