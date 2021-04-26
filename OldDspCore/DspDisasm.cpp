﻿// DSP disassembler

#include "pch.h"

namespace DSP
{
	std::string DspDisasm::ParameterToString(DspParameter index, AnalyzeInfo & info)
	{
		std::string text;

		switch (index)
		{
			// Registers

			case DspParameter::ar0: text = "ar0"; break;
			case DspParameter::ar1: text = "ar1"; break;
			case DspParameter::ar2: text = "ar2"; break;
			case DspParameter::ar3: text = "ar3"; break;
			case DspParameter::ix0: text = "ix0"; break;
			case DspParameter::ix1: text = "ix1"; break;
			case DspParameter::ix2: text = "ix2"; break;
			case DspParameter::ix3: text = "ix3"; break;
			case DspParameter::lm0: text = "lm0"; break;
			case DspParameter::lm1: text = "lm1"; break;
			case DspParameter::lm2: text = "lm2"; break;
			case DspParameter::lm3: text = "lm3"; break;
			case DspParameter::st0: text = "st0"; break;
			case DspParameter::st1: text = "st1"; break;
			case DspParameter::st2: text = "st2"; break;
			case DspParameter::st3: text = "st3"; break;
			case DspParameter::ac0h: text = "ac0.h"; break;
			case DspParameter::ac1h: text = "ac1.h"; break;
			case DspParameter::dpp: text = "dpp"; break;
			case DspParameter::psr: text = "psr"; break;
			case DspParameter::prodl: text = "prod.l"; break;
			case DspParameter::prodm1: text = "prod.m1"; break;
			case DspParameter::prodh: text = "prod.h"; break;
			case DspParameter::prodm2: text = "prod.m2"; break;
			case DspParameter::ax0l: text = "ax0.l"; break;
			case DspParameter::ax0h: text = "ax0.h"; break;
			case DspParameter::ax1l: text = "ax1.l"; break;
			case DspParameter::ax1h: text = "ax1.h"; break;
			case DspParameter::ac0l: text = "ac0.l"; break;
			case DspParameter::ac1l: text = "ac1.l"; break;
			case DspParameter::ac0m: text = "ac0.m"; break;
			case DspParameter::ac1m: text = "ac1.m"; break;

			case DspParameter::ac0: text = "ac0"; break;
			case DspParameter::ac1: text = "ac1"; break;

			case DspParameter::ax0: text = "ax0"; break;
			case DspParameter::ax1: text = "ax1"; break;

			case DspParameter::Indexed_ar0: text = "@ar0"; break;
			case DspParameter::Indexed_ar1: text = "@ar1"; break;
			case DspParameter::Indexed_ar2: text = "@ar2"; break;
			case DspParameter::Indexed_ar3: text = "@ar3"; break;
			case DspParameter::Indexed_ix0: text = "@ix0"; break;
			case DspParameter::Indexed_ix1: text = "@ix1"; break;
			case DspParameter::Indexed_ix2: text = "@ix2"; break;
			case DspParameter::Indexed_ix3: text = "@ix3"; break;

			// Immediates

			case DspParameter::Byte:
				text = "#0x" + ToHexString(info.ImmOperand.Byte);
				break;
			case DspParameter::Byte2:
				text = "#0x" + ToHexString(info.ImmOperand2.Byte);
				break;
			case DspParameter::SignedByte:
				text = std::to_string((int)(int16_t)info.ImmOperand.SignedByte);
				break;
			case DspParameter::SignedByte2:
				text = std::to_string((int)(int16_t)info.ImmOperand2.SignedByte);
				break;
			case DspParameter::UnsignedShort:
				text = "#0x" + ToHexString((uint16_t)info.ImmOperand.UnsignedShort);
				break;
			case DspParameter::UnsignedShort2:
				text = "#0x" + ToHexString((uint16_t)info.ImmOperand2.UnsignedShort);
				break;
			case DspParameter::Address:
				if (IsHardwareReg(info.ImmOperand.Address))
				{
					text = HardwareRegName(info.ImmOperand.Address);
				}
				else
				{
					text = "$0x" + ToHexString((uint16_t)info.ImmOperand.Address);
				}
				break;
			case DspParameter::Address2:
				text = "$0x" + ToHexString((uint16_t)info.ImmOperand2.Address);
				break;
		}

		return text;
	}

	std::string DspDisasm::InstrToString(DspInstruction instr, ConditionCode cc)
	{
		std::string text;

		switch (instr)
		{
			case DspInstruction::ABS: text = "abs"; break;

			case DspInstruction::ADD: text = "add"; break;
			case DspInstruction::ADDARN: text = "addarn"; break;
			case DspInstruction::ADDAX: text = "addax"; break;
			case DspInstruction::ADDAXL: text = "addaxl"; break;
			case DspInstruction::ADDI: text = "addi"; break;
			case DspInstruction::ADDIS: text = "addis"; break;
			case DspInstruction::ADDP: text = "addp"; break;
			case DspInstruction::ADDPAXZ: text = "addpaxz"; break;
			case DspInstruction::ADDR: text = "addr"; break;

			case DspInstruction::ANDC: text = "andc"; break;
			case DspInstruction::TCLR: text = "tclr"; break;
			case DspInstruction::TSET: text = "tset"; break;
			case DspInstruction::ANDI: text = "andi"; break;
			case DspInstruction::ANDR: text = "andr"; break;

			case DspInstruction::ASL: text = "asl"; break;
			case DspInstruction::ASR: text = "asr"; break;
			case DspInstruction::ASR16: text = "asr16"; break;

			case DspInstruction::BLOOP: text = "bloop"; break;
			case DspInstruction::BLOOPI: text = "bloopi"; break;
			case DspInstruction::CALLcc: text = "call" + CondCodeToString(cc); break;
			case DspInstruction::CALLR: text = "callr"; break;

			case DspInstruction::CLR: text = "clr"; break;
			case DspInstruction::CLRL: text = "clrl"; break;
			case DspInstruction::CLRP: text = "clrp"; break;

			case DspInstruction::CMP: text = "cmp"; break;
			case DspInstruction::CMPI: text = "cmpi"; break;
			case DspInstruction::CMPIS: text = "cmpis"; break;
			case DspInstruction::CMPAR: text = "cmpar"; break;

			case DspInstruction::DAR: text = "dar"; break;
			case DspInstruction::DEC: text = "dec"; break;
			case DspInstruction::DECM: text = "decm"; break;

			case DspInstruction::HALT: text = "halt"; break;

			case DspInstruction::IAR: text = "iar"; break;

			case DspInstruction::IFcc: text = "if" + CondCodeToString(cc); break;

			case DspInstruction::ILRR: text = "ilrr"; break;
			case DspInstruction::ILRRD: text = "ilrrd"; break;
			case DspInstruction::ILRRI: text = "ilrri"; break;
			case DspInstruction::ILRRN: text = "ilrrn"; break;

			case DspInstruction::INC: text = "inc"; break;
			case DspInstruction::INCM: text = "incm"; break;

			case DspInstruction::Jcc: text = "j" + CondCodeToString(cc); break;
			case DspInstruction::JMPR: text = "jmpr"; break;
			case DspInstruction::LOOP: text = "loop"; break;
			case DspInstruction::LOOPI: text = "loopi"; break;

			case DspInstruction::LR: text = "lr"; break;
			case DspInstruction::LRI: text = "lri"; break;
			case DspInstruction::LRIS: text = "lris"; break;
			case DspInstruction::LRR: text = "lrr"; break;
			case DspInstruction::LRRD: text = "lrrd"; break;
			case DspInstruction::LRRI: text = "lrri"; break;
			case DspInstruction::LRRN: text = "lrrn"; break;
			case DspInstruction::LRS: text = "lrs"; break;

			case DspInstruction::LSL: text = "lsl"; break;
			case DspInstruction::LSL16: text = "lsl16"; break;
			case DspInstruction::LSR: text = "lsr"; break;
			case DspInstruction::LSR16: text = "lsr16"; break;

			case DspInstruction::M2: text = "m2"; break;
			case DspInstruction::M0: text = "m0"; break;
			case DspInstruction::CLR15: text = "clr15"; break;
			case DspInstruction::SET15: text = "set15"; break;
			case DspInstruction::CLR40: text = "clr40"; break;
			case DspInstruction::SET40: text = "set40"; break;

			case DspInstruction::MADD: text = "madd"; break;
			case DspInstruction::MADDC: text = "maddc"; break;
			case DspInstruction::MADDX: text = "maddx"; break;

			case DspInstruction::MOV: text = "mov"; break;
			case DspInstruction::MOVAX: text = "movax"; break;
			case DspInstruction::MOVNP: text = "movnp"; break;
			case DspInstruction::MOVP: text = "movp"; break;
			case DspInstruction::MOVPZ: text = "movpz"; break;
			case DspInstruction::MOVR: text = "movr"; break;
			case DspInstruction::MRR: text = "mrr"; break;

			case DspInstruction::MSUB: text = "msub"; break;
			case DspInstruction::MSUBC: text = "msubc"; break;
			case DspInstruction::MSUBX: text = "msubx"; break;

			case DspInstruction::MUL: text = "mul"; break;
			case DspInstruction::MULAC: text = "mulac"; break;
			case DspInstruction::MULC: text = "mulc"; break;

			case DspInstruction::MULCAC: text = "mulcac"; break;
			case DspInstruction::MULCMV: text = "mulcmv"; break;
			case DspInstruction::MULCMVZ: text = "mulcmvz"; break;
			case DspInstruction::MULMV: text = "mulmv"; break;
			case DspInstruction::MULMVZ: text = "mulmvz"; break;

			case DspInstruction::MULX: text = "mulx"; break;

			case DspInstruction::MULXAC: text = "mulxac"; break;
			case DspInstruction::MULXMV: text = "mulxmv"; break;
			case DspInstruction::MULXMVZ: text = "mulxmvz"; break;

			case DspInstruction::NEG: text = "neg"; break;

			case DspInstruction::NOP: text = "nop"; break;
			case DspInstruction::NX: text = "nx"; break;

			case DspInstruction::ORC: text = "orc"; break;
			case DspInstruction::ORI: text = "ori"; break;
			case DspInstruction::ORR: text = "orr"; break;

			case DspInstruction::RETcc: text = "ret" + CondCodeToString(cc); break;
			case DspInstruction::RTI: text = "rti"; break;

			case DspInstruction::SBSET: text = "sbset"; break;
			case DspInstruction::SBCLR: text = "sbclr"; break;

			case DspInstruction::SI: text = "si"; break;
			case DspInstruction::SR: text = "sr"; break;
			case DspInstruction::SRR: text = "srr"; break;
			case DspInstruction::SRRD: text = "srrd"; break;
			case DspInstruction::SRRI: text = "srri"; break;
			case DspInstruction::SRRN: text = "srrn"; break;
			case DspInstruction::SRS: text = "srs"; break;

			case DspInstruction::SUB: text = "sub"; break;
			case DspInstruction::SUBAX: text = "subax"; break;
			case DspInstruction::SUBP: text = "subp"; break;
			case DspInstruction::SUBR: text = "subr"; break;

			case DspInstruction::TST: text = "tst"; break;
			case DspInstruction::TSTAXH: text = "tstaxh"; break;

			case DspInstruction::XORI: text = "xori"; break;
			case DspInstruction::XORR: text = "xorr"; break;

			case DspInstruction::LSN: text = "lsn"; break;
			case DspInstruction::ASN: text = "asn"; break;
		}

		while (text.size() < 5)
		{
			text += " ";
		}

		return text;
	}

	std::string DspDisasm::InstrExToString(DspInstructionEx instrEx)
	{
		std::string text;

		switch (instrEx)
		{
			// Dont show nop2's
			//case DspInstructionEx::NOP2: text = "nop2"; break;
			case DspInstructionEx::DR: text = "dr"; break;
			case DspInstructionEx::IR: text = "ir"; break;
			case DspInstructionEx::NR: text = "nr"; break;
			case DspInstructionEx::MV: text = "mv"; break;
			case DspInstructionEx::S: text = "s"; break;
			case DspInstructionEx::SN: text = "sn"; break;
			case DspInstructionEx::L: text = "l"; break;
			case DspInstructionEx::LN: text = "ln"; break;

			case DspInstructionEx::LS: text = "ls"; break;
			case DspInstructionEx::SL: text = "sl"; break;
			case DspInstructionEx::LSN: text = "lsn"; break;
			case DspInstructionEx::SLN: text = "sln"; break;
			case DspInstructionEx::LSM: text = "lsm"; break;
			case DspInstructionEx::SLM: text = "slm"; break;
			case DspInstructionEx::LSNM: text = "lsnm"; break;
			case DspInstructionEx::SLNM: text = "slnm"; break;

			case DspInstructionEx::LD: text = "ld"; break;
			case DspInstructionEx::LDN: text = "ldn"; break;
			case DspInstructionEx::LDM: text = "ldm"; break;
			case DspInstructionEx::LDNM: text = "ldnm"; break;

			case DspInstructionEx::LDAX: text = "ldax"; break;
			case DspInstructionEx::LDAXN: text = "ldaxn"; break;
			case DspInstructionEx::LDAXM: text = "ldaxm"; break;
			case DspInstructionEx::LDAXNM: text = "ldaxnm"; break;
		}

		while (text.size() < 5)
		{
			text += " ";
		}

		return text;
	}

	bool DspDisasm::IsHardwareReg(DspAddress address)
	{
		return address >= 0xFF00;
	}

	std::string DspDisasm::HardwareRegName(DspAddress address)
	{
		std::string text;

		switch ((DspHardwareRegs)address)
		{
			case DspHardwareRegs::CMBH: text = "CMBH"; break;
			case DspHardwareRegs::CMBL: text = "CMBL"; break;
			case DspHardwareRegs::DMBH: text = "DMBH"; break;
			case DspHardwareRegs::DMBL: text = "DMBL"; break;

			case DspHardwareRegs::DSMAH: text = "DSMAH"; break;
			case DspHardwareRegs::DSMAL: text = "DSMAL"; break;
			case DspHardwareRegs::DSPA: text = "DSPA"; break;
			case DspHardwareRegs::DSCR: text = "DSCR"; break;
			case DspHardwareRegs::DSBL: text = "DSBL"; break;

			case DspHardwareRegs::ACDAT2: text = "ACDAT2"; break;
			case DspHardwareRegs::ACSAH: text = "ACSAH"; break;
			case DspHardwareRegs::ACSAL: text = "ACSAL"; break;
			case DspHardwareRegs::ACEAH: text = "ACEAH"; break;
			case DspHardwareRegs::ACEAL: text = "ACEAL"; break;
			case DspHardwareRegs::ACCAH: text = "ACCAH"; break;
			case DspHardwareRegs::ACCAL: text = "ACCAL"; break;
			case DspHardwareRegs::ACDAT: text = "ACDAT"; break;

			case DspHardwareRegs::DIRQ: text = "DIRQ"; break;

			case DspHardwareRegs::ACFMT: text = "ACFMT"; break;
			case DspHardwareRegs::ACPDS: text = "ACPDS"; break;
			case DspHardwareRegs::ACYN1: text = "ACYN1"; break;
			case DspHardwareRegs::ACYN2: text = "ACYN2"; break;
			case DspHardwareRegs::ACGAN: text = "ACGAN"; break;

			case DspHardwareRegs::ADPCM_A00: text = "ADPCM_A00"; break;
			case DspHardwareRegs::ADPCM_A10: text = "ADPCM_A10"; break;
			case DspHardwareRegs::ADPCM_A20: text = "ADPCM_A20"; break;
			case DspHardwareRegs::ADPCM_A30: text = "ADPCM_A30"; break;
			case DspHardwareRegs::ADPCM_A40: text = "ADPCM_A40"; break;
			case DspHardwareRegs::ADPCM_A50: text = "ADPCM_A50"; break;
			case DspHardwareRegs::ADPCM_A60: text = "ADPCM_A60"; break;
			case DspHardwareRegs::ADPCM_A70: text = "ADPCM_A70"; break;
			case DspHardwareRegs::ADPCM_A01: text = "ADPCM_A01"; break;
			case DspHardwareRegs::ADPCM_A11: text = "ADPCM_A11"; break;
			case DspHardwareRegs::ADPCM_A21: text = "ADPCM_A21"; break;
			case DspHardwareRegs::ADPCM_A31: text = "ADPCM_A31"; break;
			case DspHardwareRegs::ADPCM_A41: text = "ADPCM_A41"; break;
			case DspHardwareRegs::ADPCM_A51: text = "ADPCM_A51"; break;
			case DspHardwareRegs::ADPCM_A61: text = "ADPCM_A61"; break;
			case DspHardwareRegs::ADPCM_A71: text = "ADPCM_A71"; break;

			default:
				text = "UnkHW_" + ToHexString((uint16_t)address);
		}

		return "$(" + text + ")";
	}

	std::string DspDisasm::CondCodeToString(ConditionCode cc)
	{
		std::string text = "";

		switch (cc)
		{
			case ConditionCode::GE: text = "ge"; break;
			case ConditionCode::LT: text = "lt"; break;
			case ConditionCode::GT: text = "gt"; break;
			case ConditionCode::LE: text = "le"; break;
			case ConditionCode::NZ: text = "nz"; break;
			case ConditionCode::Z: text = "z"; break;
			case ConditionCode::NC: text = "nc"; break;
			case ConditionCode::C: text = "c"; break;
			case ConditionCode::NE: text = "ne"; break;
			case ConditionCode::E: text = "e"; break;
			case ConditionCode::NM: text = "nm"; break;
			case ConditionCode::M: text = "m"; break;
			case ConditionCode::NT: text = "nt"; break;
			case ConditionCode::T: text = "t"; break;
			case ConditionCode::V: text = "v"; break;
			case ConditionCode::Always: text = ""; break;
		}

		return text;
	}

	std::string DspDisasm::Disasm(DspAddress startAddr, AnalyzeInfo& info)
	{
		std::string text = "";

		// Address and code bytes

		text += ToHexString((uint16_t)startAddr);
		text += " ";

		for (size_t i = 0; i < DspCore::MaxInstructionSizeInBytes; i++)
		{
			if (i < info.sizeInBytes)
			{
				text += ToHexString(info.bytes[i]) + " ";
			}
			else
			{
				text += "   ";
			}
		}

		// Regular instruction

		if (info.instr != DspInstruction::Unknown)
		{
			text += "\t" + DspDisasm::InstrToString(info.instr, info.cc) + "\t";
		}
		else
		{
			text += "\t??? " + ToHexString(info.instrBits);
		}

		bool firstParam = true;
		for (size_t i = 0; i < info.numParameters; i++)
		{
			if (!firstParam)
			{
				text += ", ";
			}
			text += ParameterToString(info.params[i], info);
			firstParam = false;
		}

		// Packed instruction pair (same line)

		if (info.extendedOpcodePresent)
		{
			while (text.size() < 40)
			{
				text += " ";
			}

			if (info.instrEx != DspInstructionEx::Unknown)
			{
				text += "\t" + InstrExToString(info.instrEx) + "\t";
			}
			else
			{
				text += "??? ext " + ToHexString(info.instrExBits);
			}

			bool firstExtendedParam = true;
			for (size_t i=0; i<info.numParametersEx; i++)
			{
				if (!firstExtendedParam)
				{
					text += ", ";
				}
				text += ParameterToString(info.paramsEx[i], info);
				firstExtendedParam = false;
			}
		}

		return text;
	}

}
