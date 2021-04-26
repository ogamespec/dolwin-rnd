// DSP multiply instructions

// The Duddie documentation describing multiplication instructions everywhere states that operands are treated as signed 16-bit numbers.
// But there are also mentioned about 2 control bits of the status register:
// - Bit 15 (SU): Operands are signed (1 = unsigned)
// - Bit 13 (AM): Product multiply result by 2 (when AM = 0) 

// Did not notice that microcodes check flags after multiplication operations, so leave flags for now..

#include "pch.h"

namespace DSP
{
	void DspInterpreter::Mul(int16_t a, int16_t b)
	{
		bool scale = core->regs.psr.im == 0;
		core->regs.prod = DspCore::Muls(a, b, scale);
	}

	void DspInterpreter::Mulx(int16_t a, int16_t b, int an, int bn)
	{
		bool scale = core->regs.psr.im == 0;

		if (core->regs.psr.dp)
		{
			if (an == 0 && bn == 0)
			{
				core->regs.prod = DspCore::Mulu(a, b, scale);
			}
			else if (an == 0 && bn == 1)
			{
				core->regs.prod = DspCore::Mulus(a, b, scale);
			}
			else if (an == 1 && bn == 0)
			{
				core->regs.prod = DspCore::Mulus(b, a, scale);
			}
			else
			{
				core->regs.prod = DspCore::Muls(a, b, scale);
			}
		}
		else
		{
			core->regs.prod = DspCore::Muls(a, b, scale);
		}
	}

	void DspInterpreter::Madd(int16_t a, int16_t b)
	{
		bool scale = core->regs.psr.im == 0;
		DspProduct temp = DspCore::Muls(a, b, scale);
		
		DspCore::PackProd(core->regs.prod);
		DspCore::PackProd(temp);
		core->regs.prod.bitsPacked = DspCore::SignExtend40(core->regs.prod.bitsPacked);
		core->regs.prod.bitsPacked += DspCore::SignExtend40(temp.bitsPacked);
		DspCore::UnpackProd(core->regs.prod);
	}

	void DspInterpreter::Msub(int16_t a, int16_t b)
	{
		bool scale = core->regs.psr.im == 0;
		DspProduct temp = DspCore::Muls(a, b, scale);

		DspCore::PackProd(core->regs.prod);
		DspCore::PackProd(temp);
		core->regs.prod.bitsPacked = DspCore::SignExtend40(core->regs.prod.bitsPacked);
		core->regs.prod.bitsPacked -= DspCore::SignExtend40(temp.bitsPacked);
		DspCore::UnpackProd(core->regs.prod);
	}

	void DspInterpreter::Mulac(int16_t a, int16_t b, int r)
	{
		bool scale = core->regs.psr.im == 0;

		DspCore::PackProd(core->regs.prod);
		int64_t ac = DspCore::SignExtend40(core->regs.ac[r].sbits);
		ac += DspCore::SignExtend40(core->regs.prod.bitsPacked);
		core->regs.ac[r].sbits = ac & 0xff'ffff'ffff;

		core->regs.prod = DspCore::Muls(a, b, scale);
	}

	void DspInterpreter::Mulxac(int16_t a, int16_t b, int r, int an, int bn)
	{
		DspCore::PackProd(core->regs.prod);
		int64_t ac = DspCore::SignExtend40(core->regs.ac[r].sbits);
		ac += DspCore::SignExtend40(core->regs.prod.bitsPacked);
		core->regs.ac[r].sbits = ac & 0xff'ffff'ffff;

		Mulx(a, b, an, bn);
	}

	void DspInterpreter::Mulmv(int16_t a, int16_t b, int r)
	{
		bool scale = core->regs.psr.im == 0;

		DspCore::PackProd(core->regs.prod);
		core->regs.ac[r].sbits = core->regs.prod.bitsPacked;
		core->regs.ac[r].sbits &= 0xff'ffff'ffff;

		core->regs.prod = DspCore::Muls(a, b, scale);
	}

	void DspInterpreter::Mulxmv(int16_t a, int16_t b, int r, int an, int bn)
	{
		DspCore::PackProd(core->regs.prod);
		core->regs.ac[r].sbits = core->regs.prod.bitsPacked;
		core->regs.ac[r].sbits &= 0xff'ffff'ffff;

		Mulx(a, b, an, bn);
	}

	void DspInterpreter::Mulmvz(int16_t a, int16_t b, int r)
	{
		bool scale = core->regs.psr.im == 0;

		DspCore::PackProd(core->regs.prod);
		core->regs.ac[r].sbits = core->regs.prod.bitsPacked;
		core->regs.ac[r].sbits &= 0xff'ffff'0000;

		core->regs.prod = DspCore::Muls(a, b, scale);
	}

	void DspInterpreter::Mulxmvz(int16_t a, int16_t b, int r, int an, int bn)
	{
		DspCore::PackProd(core->regs.prod);
		core->regs.ac[r].sbits = core->regs.prod.bitsPacked;
		core->regs.ac[r].sbits &= 0xff'ffff'0000;

		Mulx(a, b, an, bn);
	}

	// MUL

	void DspInterpreter::MUL(AnalyzeInfo& info)
	{
		// Multiply $axS.l by high part $axS.h (treat them both as signed).

		Mul(core->regs.ax[info.paramBits[0]].l, core->regs.ax[info.paramBits[0]].h);
	}

	void DspInterpreter::MULC(AnalyzeInfo& info)
	{
		// Multiply mid part of accumulator register $acS.m by $axT.h (treat them both as signed).

		Mul(core->regs.ac[info.paramBits[0]].m, core->regs.ax[info.paramBits[1]].h);
	}

	void DspInterpreter::MULX(AnalyzeInfo& info)
	{
		// Multiply one part $ax0 by one part $ax1 (treat them both as signed). Part is selected by S and T bits. Zero selects low part, one selects high part.
		
		int16_t a = info.paramBits[0] ? core->regs.ax[0].h : core->regs.ax[0].l;
		int16_t b = info.paramBits[1] ? core->regs.ax[1].h : core->regs.ax[1].l;
		Mulx(a, b, info.paramBits[0], info.paramBits[1]);
	}

	// MADD

	void DspInterpreter::MADD(AnalyzeInfo& info)
	{
		// Multiply $axS.l by $axS.h (treat them both as signed)
		// and add result to product register. 

		Madd(core->regs.ax[info.paramBits[0]].l, core->regs.ax[info.paramBits[0]].h);
	}

	void DspInterpreter::MADDC(AnalyzeInfo& info)
	{
		// Multiply middle part of accumulator $acS.m by $axT.h (treat them both as signed)
		// and add result to product register.

		Madd(core->regs.ac[info.paramBits[0]].m, core->regs.ax[info.paramBits[1]].h);
	}

	void DspInterpreter::MADDX(AnalyzeInfo& info)
	{
		// Multiply one part of $ax0 (selected by S) by one part of $ax1 (selected by T) (treat them both as signed)
		// and add result to product register. 

		int16_t a = info.paramBits[0] ? core->regs.ax[0].h : core->regs.ax[0].l;
		int16_t b = info.paramBits[1] ? core->regs.ax[1].h : core->regs.ax[1].l;
		Madd(a, b);
	}

	// MSUB

	void DspInterpreter::MSUB(AnalyzeInfo& info)
	{
		// Multiply $axS.l by $axS.h (treat them both as signed)
		// and subtract result from product register. 

		Msub(core->regs.ax[info.paramBits[0]].l, core->regs.ax[info.paramBits[0]].h);
	}

	void DspInterpreter::MSUBC(AnalyzeInfo& info)
	{
		// Multiply middle part of accumulator $acS.m by $axT.h (treat them both as signed) 
		// and subtract result from product register.

		Msub(core->regs.ac[info.paramBits[0]].m, core->regs.ax[info.paramBits[1]].h);
	}

	void DspInterpreter::MSUBX(AnalyzeInfo& info)
	{
		// Multiply one part of $ax0 (selected by S) by one part of $ax1 (selected by T) (treat them both as signed) 
		// and subtract result from product register. 

		int16_t a = info.paramBits[0] ? core->regs.ax[0].h : core->regs.ax[0].l;
		int16_t b = info.paramBits[1] ? core->regs.ax[1].h : core->regs.ax[1].l;
		Msub(a, b);
	}

	// AC

	void DspInterpreter::MULAC(AnalyzeInfo& info)
	{
		// Add product register to accumulator register $acR.
		// Multiply $axS.l by $axS.h (treat them both as signed).

		Mulac(core->regs.ax[info.paramBits[0]].l, core->regs.ax[info.paramBits[0]].h, info.paramBits[2]);
	}

	void DspInterpreter::MULCAC(AnalyzeInfo& info)
	{
		// Add product register before multiplication to accumulator $acR.
		// Multiply mid part of accumulator register $acS.m by $axT.h (treat them both as signed).

		Mulac(core->regs.ac[info.paramBits[0]].m, core->regs.ax[info.paramBits[1]].h, info.paramBits[2]);
	}

	void DspInterpreter::MULXAC(AnalyzeInfo& info)
	{
		// Add product register to accumulator register $acR. 
		// Multiply one part $ax0 by one part $ax1 (treat them both as signed). Part is selected by S and T bits. Zero selects low part, one selects high part. 

		int16_t a = info.paramBits[0] ? core->regs.ax[0].h : core->regs.ax[0].l;
		int16_t b = info.paramBits[1] ? core->regs.ax[1].h : core->regs.ax[1].l;
		Mulxac(a, b, info.paramBits[2], info.paramBits[0], info.paramBits[1]);
	}

	// MV

	void DspInterpreter::MULMV(AnalyzeInfo& info)
	{
		// Move product register to accumulator register $acR.
		// Multiply $axS.l by $axS.h (treat them both as signed). 

		Mulmv(core->regs.ax[info.paramBits[0]].l, core->regs.ax[info.paramBits[0]].h, info.paramBits[2]);
	}

	void DspInterpreter::MULCMV(AnalyzeInfo& info)
	{
		// Move product register before multiplication to accumulator $acR.
		// Multiply mid part of accumulator register $acS.m by $axT.h (treat them both as signed).

		Mulmv(core->regs.ac[info.paramBits[0]].m, core->regs.ax[info.paramBits[1]].h, info.paramBits[2]);
	}

	void DspInterpreter::MULXMV(AnalyzeInfo& info)
	{
		// Move product register to accumulator register $acR.
		// Multiply one part $ax0 by one part $ax1 (treat them both as signed). Part is selected by Sand T bits. Zero selects low part, one selects high part.

		int16_t a = info.paramBits[0] ? core->regs.ax[0].h : core->regs.ax[0].l;
		int16_t b = info.paramBits[1] ? core->regs.ax[1].h : core->regs.ax[1].l;
		Mulxmv(a, b, info.paramBits[2], info.paramBits[0], info.paramBits[1]);
	}

	// MVZ

	void DspInterpreter::MULMVZ(AnalyzeInfo& info)
	{
		// Move product register to accumulator register $acR and clear low part of accumulator register $acR.l.
		// Multiply $axS.l by $axS.h (treat them both as signed).

		Mulmvz(core->regs.ax[info.paramBits[0]].l, core->regs.ax[info.paramBits[0]].h, info.paramBits[2]);
	}

	void DspInterpreter::MULCMVZ(AnalyzeInfo& info)
	{
		// Move product register before multiplication to accumulator $acR. Set low part of accumulator $acR.l to zero. 
		// Multiply mid part of accumulator register $acS.m by $axT.h (treat them both as signed). 

		Mulmvz(core->regs.ac[info.paramBits[0]].m, core->regs.ax[info.paramBits[1]].h, info.paramBits[2]);
	}

	void DspInterpreter::MULXMVZ(AnalyzeInfo& info)
	{
		// Move product register to accumulator register $acR and clear low part of accumulator register $acR.l. 
		// Multiply one part $ax0 by one part $ax1 (treat them both as signed). Part is selected by S and T bits. Zero selects low part, one selects high part.

		int16_t a = info.paramBits[0] ? core->regs.ax[0].h : core->regs.ax[0].l;
		int16_t b = info.paramBits[1] ? core->regs.ax[1].h : core->regs.ax[1].l;
		Mulxmvz(a, b, info.paramBits[2], info.paramBits[0], info.paramBits[1]);
	}
}
