// This module contains all GekkoCore tests.
#include "pch.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Gekko
{
	extern GekkoCore* Gekko;
}

namespace GekkoCoreUnitTest
{
	TEST_CLASS(GekkoCoreUnitTest)
	{
		std::string currentDir;

		void SetCWDToDolwin()
		{
			char path[0x100] = { 0 };
			if (currentDir == "")
			{
				GetCurrentDirectoryA(sizeof(path), path);
				currentDir = path;
			}
			std::string newpath = currentDir + "/../../../../dolwin";
			SetCurrentDirectoryA(newpath.c_str());
		}

		void SetCWDToTest()
		{
			char path[0x100] = { 0 };
			if (currentDir == "")
			{
				GetCurrentDirectoryA(sizeof(path), path);
				currentDir = path;
			}
			std::string newpath = currentDir + "/../../";
			SetCurrentDirectoryA(newpath.c_str());
		}

	public:
		
#pragma region "Basic Tests"

		TEST_METHOD(GekkoCoreInstance)
		{
			SetCWDToDolwin();

			Gekko::GekkoCore* core = new Gekko::GekkoCore();

			delete core;
		}

		void DumpGekkoAnalyzeInfo(Gekko::AnalyzeInfo* info)
		{
			char text[0x100] = { 0, };
			
			sprintf_s(text, sizeof(text) - 1, "instr: %i, numParam: %zi, p0: %i, p1: %i, p2: %i",
				(int)info->instr,
				info->numParam,
				info->paramBits[0],
				info->paramBits[1],
				info->paramBits[2]);

			Logger::WriteMessage(text);
		}

		TEST_METHOD(SimpleAnalyzeInfo)
		{
			Gekko::AnalyzeInfo info = { 0 };

			uint32_t instr = (31 << 26) | (266 << 1);

			Gekko::Analyzer::Analyze(0, instr, &info);

			DumpGekkoAnalyzeInfo(&info);
		}

#pragma endregion "Basic Tests"

#pragma region "Memory/Cache Tests"

		// TBD

#pragma endregion "Memory/Cache Tests"

#pragma region "MMU Tests"

		// TBD

		void SetupDolphinOSMemoryModel(Gekko::GekkoCore* core)
		{
			for (int sr = 0; sr < 16; sr++)
			{
				core->regs.sr[sr] = 0x80000000;
			}
			// DBATs
			core->regs.spr[Gekko::SPR::DBAT0U] = 0x80001fff; core->regs.spr[Gekko::SPR::DBAT0L] = 0x00000002;   // 0x80000000, 256mb, Write-back cached
			core->regs.spr[Gekko::SPR::DBAT1U] = 0xc0001fff; core->regs.spr[Gekko::SPR::DBAT1L] = 0x0000002a;   // 0xC0000000, 256mb, Cache inhibited, Guarded
			core->regs.spr[Gekko::SPR::DBAT2U] = 0x00000000; core->regs.spr[Gekko::SPR::DBAT2L] = 0x00000000;   // undefined
			core->regs.spr[Gekko::SPR::DBAT3U] = 0x00000000; core->regs.spr[Gekko::SPR::DBAT3L] = 0x00000000;   // undefined
			// IBATs
			core->regs.spr[Gekko::SPR::IBAT0U] = core->regs.spr[Gekko::SPR::DBAT0U];
			core->regs.spr[Gekko::SPR::IBAT0L] = core->regs.spr[Gekko::SPR::DBAT0L];
			core->regs.spr[Gekko::SPR::IBAT1U] = core->regs.spr[Gekko::SPR::DBAT1U];
			core->regs.spr[Gekko::SPR::IBAT1L] = core->regs.spr[Gekko::SPR::DBAT1L];
			core->regs.spr[Gekko::SPR::IBAT2U] = core->regs.spr[Gekko::SPR::DBAT2U];
			core->regs.spr[Gekko::SPR::IBAT2L] = core->regs.spr[Gekko::SPR::DBAT2L];
			core->regs.spr[Gekko::SPR::IBAT3U] = core->regs.spr[Gekko::SPR::DBAT3U];
			core->regs.spr[Gekko::SPR::IBAT3L] = core->regs.spr[Gekko::SPR::DBAT3L];
			// MSR MMU bits
			core->regs.msr |= (MSR_IR | MSR_DR);               // enable translation
			// page table
			core->regs.spr[Gekko::SPR::SDR1] = 0;
		}

#pragma endregion "MMU Tests"

#pragma region "GekkoISA Tests"

		void LoadJson(std::string& file, std::vector<uint8_t>& text)
		{
			FILE* f = nullptr;
			fopen_s(&f, file.c_str(), "rb");
			Assert::IsNotNull(f);

			if (f)
			{
				uint8_t* jsonText = nullptr;        // utf-8
				size_t jsonTextSize = 0;

				fseek(f, 0, SEEK_END);
				jsonTextSize = ftell(f);
				fseek(f, 0, SEEK_SET);

				jsonText = new uint8_t[jsonTextSize];

				fread(jsonText, 1, jsonTextSize, f);
				fclose(f);

				text.resize(jsonTextSize);
				memcpy(text.data(), jsonText, jsonTextSize);
				delete[] jsonText;
			}
		}

		uint32_t GetRegVal(Json::Value* n)
		{
			if (n->type == Json::ValueType::String)
			{
				std::string str = Util::WstringToString(n->value.AsString);
				return strtoul(str.c_str(), nullptr, 0);
			}
			else if (n->type == Json::ValueType::Int)
			{
				return n->value.AsUint32;
			}
			else if (n->type == Json::ValueType::Float)
			{
				return (uint32_t)(int32_t)n->value.AsFloat;
			}
			else
			{
				Assert::Fail();
			}
		}

		double GetFRegVal(Json::Value* n)
		{
			if (n->type == Json::ValueType::Float)
			{
				return (double)n->value.AsFloat;
			}
			else
			{
				Assert::Fail();
			}
		}

		void PrepareContext(Gekko::GekkoCore* core, Json::Value* before)
		{
			for (auto v = before->children.begin(); v != before->children.end(); ++v)
			{
				Json::Value* r = (*v)->children.front();
				std::string name = std::string(r->name);

				if (name == "pc")
				{
					core->regs.pc = GetRegVal(r);
				}

				else if (r->name && r->name[0] == 'r')
				{
					core->regs.gpr[atoi(r->name + 1)] = GetRegVal(r);
				}
				else if (r->name && strlen(r->name) > 2 && r->name[0] == 'f' && r->name[1] == 'r')
				{
					core->regs.fpr[atoi(r->name + 2)].dbl = GetFRegVal(r);
				}
				else if (r->name && strlen(r->name) > 2 && r->name[0] == 'p' && r->name[1] == 's')
				{
					core->regs.ps1[atoi(r->name + 2)].dbl = GetFRegVal(r);
				}

				else if (name == "CR0[LT]")
				{
					if (r->value.AsInt) core->regs.cr |= GEKKO_CR0_LT;
					else core->regs.cr &= ~GEKKO_CR0_LT;
				}
				else if (name == "CR0[GT]")
				{
					if (r->value.AsInt) core->regs.cr |= GEKKO_CR0_GT;
					else core->regs.cr &= ~GEKKO_CR0_GT;
				}
				else if (name == "CR0[EQ]")
				{
					if (r->value.AsInt) core->regs.cr |= GEKKO_CR0_EQ;
					else core->regs.cr &= ~GEKKO_CR0_EQ;
				}
				else if (name == "CR0[SO]")
				{
					if (r->value.AsInt) core->regs.cr |= GEKKO_CR0_SO;
					else core->regs.cr &= ~GEKKO_CR0_SO;
				}

				else if (name == "XER[SO]")
				{
					if (r->value.AsInt) core->regs.spr[Gekko::SPR::XER] |= GEKKO_XER_SO;
					else core->regs.spr[Gekko::SPR::XER] &= ~GEKKO_XER_SO;
				}
				else if (name == "XER[OV]")
				{
					if (r->value.AsInt) core->regs.spr[Gekko::SPR::XER] |= GEKKO_XER_OV;
					else core->regs.spr[Gekko::SPR::XER] &= ~GEKKO_XER_OV;
				}
				else if (name == "XER[CA]")
				{
					if (r->value.AsInt) core->regs.spr[Gekko::SPR::XER] |= GEKKO_XER_CA;
					else core->regs.spr[Gekko::SPR::XER] &= ~GEKKO_XER_CA;
				}
			}
		}

		Gekko::Instruction StrToInstr(std::string& str)
		{
			if (str == "add") return Gekko::Instruction::add;
			else if (str == "add.") return Gekko::Instruction::add_d;
			else if (str == "addo") return Gekko::Instruction::addo;
			else if (str == "addo.") return Gekko::Instruction::addo_d;
			else if (str == "addc") return Gekko::Instruction::addc;
			else if (str == "addc.") return Gekko::Instruction::addc_d;
			else if (str == "addco") return Gekko::Instruction::addco;
			else if (str == "addco.") return Gekko::Instruction::addco_d;
			else if (str == "adde") return Gekko::Instruction::adde;
			else if (str == "adde.") return Gekko::Instruction::adde_d;
			else if (str == "addeo") return Gekko::Instruction::addeo;
			else if (str == "addeo.") return Gekko::Instruction::addeo_d;
			else if (str == "addi") return Gekko::Instruction::addi;
			else if (str == "addic") return Gekko::Instruction::addic;
			else if (str == "addic.") return Gekko::Instruction::addic_d;
			else if (str == "addis") return Gekko::Instruction::addis;
			else if (str == "addme") return Gekko::Instruction::addme;
			else if (str == "addme.") return Gekko::Instruction::addme_d;
			else if (str == "addmeo") return Gekko::Instruction::addmeo;
			else if (str == "addmeo.") return Gekko::Instruction::addmeo_d;
			else if (str == "addze") return Gekko::Instruction::addze;
			else if (str == "addze.") return Gekko::Instruction::addze_d;
			else if (str == "addzeo") return Gekko::Instruction::addzeo;
			else if (str == "addzeo.") return Gekko::Instruction::addzeo_d;
			else if (str == "and") return Gekko::Instruction::_and;
			else if (str == "and.") return Gekko::Instruction::and_d;
			else if (str == "andc") return Gekko::Instruction::andc;
			else if (str == "andc.") return Gekko::Instruction::andc_d;
			else if (str == "andi.") return Gekko::Instruction::andi_d;
			else if (str == "andis.") return Gekko::Instruction::andis_d;
			
			if (str == "b") return Gekko::Instruction::b;
			else if (str == "ba") return Gekko::Instruction::ba;
			else if (str == "bl") return Gekko::Instruction::bl;
			else if (str == "bla") return Gekko::Instruction::bla;
			else if (str == "bc") return Gekko::Instruction::bc;
			else if (str == "bca") return Gekko::Instruction::bca;
			else if (str == "bcl") return Gekko::Instruction::bcl;
			else if (str == "bcla") return Gekko::Instruction::bcla;
			else if (str == "bcctr") return Gekko::Instruction::bcctr;
			else if (str == "bcctrl") return Gekko::Instruction::bcctrl;
			else if (str == "bclr") return Gekko::Instruction::bclr;
			else if (str == "bclrl") return Gekko::Instruction::bclrl;
			
			if (str == "cmp") return Gekko::Instruction::cmp;
			else if (str == "cmpi") return Gekko::Instruction::cmpi;
			else if (str == "cmpl") return Gekko::Instruction::cmpl;
			else if (str == "cmpli") return Gekko::Instruction::cmpli;
			else if (str == "cntlzw") return Gekko::Instruction::cntlzw;
			else if (str == "cntlzw.") return Gekko::Instruction::cntlzw_d;
			else if (str == "crand") return Gekko::Instruction::crand;
			else if (str == "crandc") return Gekko::Instruction::crandc;
			else if (str == "creqv") return Gekko::Instruction::creqv;
			else if (str == "crnand") return Gekko::Instruction::crnand;
			else if (str == "crnor") return Gekko::Instruction::crnor;
			else if (str == "cror") return Gekko::Instruction::cror;
			else if (str == "crorc") return Gekko::Instruction::crorc;
			else if (str == "crxor") return Gekko::Instruction::crxor;
			else if (str == "dcbf") return Gekko::Instruction::dcbf;
			else if (str == "dcbi") return Gekko::Instruction::dcbi;
			else if (str == "dcbst") return Gekko::Instruction::dcbst;
			else if (str == "dcbt") return Gekko::Instruction::dcbt;
			else if (str == "dcbtst") return Gekko::Instruction::dcbtst;
			else if (str == "dcbz") return Gekko::Instruction::dcbz;
			else if (str == "dcbz_l") return Gekko::Instruction::dcbz_l;
			else if (str == "divw") return Gekko::Instruction::divw;
			else if (str == "divw.") return Gekko::Instruction::divw_d;
			else if (str == "divwo") return Gekko::Instruction::divwo;
			else if (str == "divwo.") return Gekko::Instruction::divwo_d;
			else if (str == "divwu") return Gekko::Instruction::divwu;
			else if (str == "divwu.") return Gekko::Instruction::divwu_d;
			else if (str == "divwuo") return Gekko::Instruction::divwuo;
			else if (str == "divwuo.") return Gekko::Instruction::divwuo_d;
			else if (str == "eciwx") return Gekko::Instruction::eciwx;
			else if (str == "ecowx") return Gekko::Instruction::ecowx;
			else if (str == "eieio") return Gekko::Instruction::eieio;
			else if (str == "eqv") return Gekko::Instruction::eqv;
			else if (str == "eqv.") return Gekko::Instruction::eqv_d;
			else if (str == "extsb") return Gekko::Instruction::extsb;
			else if (str == "extsb.") return Gekko::Instruction::extsb_d;
			else if (str == "extsh") return Gekko::Instruction::extsh;
			else if (str == "extsh.") return Gekko::Instruction::extsh_d;
			
			if (str == "fabs") return Gekko::Instruction::fabs;
			else if (str == "fabs.") return Gekko::Instruction::fabs_d;
			else if (str == "fadd") return Gekko::Instruction::fadd;
			else if (str == "fadd.") return Gekko::Instruction::fadd_d;
			else if (str == "fadds") return Gekko::Instruction::fadds;
			else if (str == "fadds.") return Gekko::Instruction::fadds_d;
			else if (str == "fcmpo") return Gekko::Instruction::fcmpo;
			else if (str == "fcmpu") return Gekko::Instruction::fcmpu;
			else if (str == "fctiw") return Gekko::Instruction::fctiw;
			else if (str == "fctiw.") return Gekko::Instruction::fctiw_d;
			else if (str == "fctiwz") return Gekko::Instruction::fctiwz;
			else if (str == "fctiwz.") return Gekko::Instruction::fctiwz_d;
			else if (str == "fdiv") return Gekko::Instruction::fdiv;
			else if (str == "fdiv.") return Gekko::Instruction::fdiv_d;
			else if (str == "fdivs") return Gekko::Instruction::fdivs;
			else if (str == "fdivs.") return Gekko::Instruction::fdivs_d;
			else if (str == "fmadd") return Gekko::Instruction::fmadd;
			else if (str == "fmadd.") return Gekko::Instruction::fmadd_d;
			else if (str == "fmadds") return Gekko::Instruction::fmadds;
			else if (str == "fmadds.") return Gekko::Instruction::fmadds_d;
			else if (str == "fmr") return Gekko::Instruction::fmr;
			else if (str == "fmr.") return Gekko::Instruction::fmr_d;
			else if (str == "fmsub") return Gekko::Instruction::fmsub;
			else if (str == "fmsub.") return Gekko::Instruction::fmsub_d;
			else if (str == "fmsubs") return Gekko::Instruction::fmsubs;
			else if (str == "fmsubs.") return Gekko::Instruction::fmsubs_d;
			else if (str == "fmul") return Gekko::Instruction::fmul;
			else if (str == "fmul.") return Gekko::Instruction::fmul_d;
			else if (str == "fmuls") return Gekko::Instruction::fmuls;
			else if (str == "fmuls.") return Gekko::Instruction::fmuls_d;
			else if (str == "fnabs") return Gekko::Instruction::fnabs;
			else if (str == "fnabs.") return Gekko::Instruction::fnabs_d;
			else if (str == "fneg") return Gekko::Instruction::fneg;
			else if (str == "fneg.") return Gekko::Instruction::fneg_d;
			else if (str == "fnmadd") return Gekko::Instruction::fnmadd;
			else if (str == "fnmadd.") return Gekko::Instruction::fnmadd_d;
			else if (str == "fnmadds") return Gekko::Instruction::fnmadds;
			else if (str == "fnmadds.") return Gekko::Instruction::fnmadds_d;
			else if (str == "fnmsub") return Gekko::Instruction::fnmsub;
			else if (str == "fnmsub.") return Gekko::Instruction::fnmsub_d;
			else if (str == "fnmsubs") return Gekko::Instruction::fnmsubs;
			else if (str == "fnmsubs.") return Gekko::Instruction::fnmsubs_d;
			else if (str == "fres") return Gekko::Instruction::fres;
			else if (str == "fres.") return Gekko::Instruction::fres_d;
			else if (str == "frsp") return Gekko::Instruction::frsp;
			else if (str == "frsp.") return Gekko::Instruction::frsp_d;
			else if (str == "frsqrte") return Gekko::Instruction::frsqrte;
			else if (str == "frsqrte.") return Gekko::Instruction::frsqrte_d;
			else if (str == "fsel") return Gekko::Instruction::fsel;
			else if (str == "fsel.") return Gekko::Instruction::fsel_d;
			else if (str == "fsub") return Gekko::Instruction::fsub;
			else if (str == "fsub.") return Gekko::Instruction::fsub_d;
			else if (str == "fsubs") return Gekko::Instruction::fsubs;
			else if (str == "fsubs.") return Gekko::Instruction::fsubs_d;
			
			if (str == "icbi") return Gekko::Instruction::icbi;
			else if (str == "isync") return Gekko::Instruction::isync;
			
			if (str == "lbz") return Gekko::Instruction::lbz;
			else if (str == "lbzu") return Gekko::Instruction::lbzu;
			else if (str == "lbzux") return Gekko::Instruction::lbzux;
			else if (str == "lbzx") return Gekko::Instruction::lbzx;
			else if (str == "lfd") return Gekko::Instruction::lfd;
			else if (str == "lfdu") return Gekko::Instruction::lfdu;
			else if (str == "lfdux") return Gekko::Instruction::lfdux;
			else if (str == "lfdx") return Gekko::Instruction::lfdx;
			else if (str == "lfs") return Gekko::Instruction::lfs;
			else if (str == "lfsu") return Gekko::Instruction::lfsu;
			else if (str == "lfsux") return Gekko::Instruction::lfsux;
			else if (str == "lfsx") return Gekko::Instruction::lfsx;
			else if (str == "lha") return Gekko::Instruction::lha;
			else if (str == "lhau") return Gekko::Instruction::lhau;
			else if (str == "lhaux") return Gekko::Instruction::lhaux;
			else if (str == "lhax") return Gekko::Instruction::lhax;
			else if (str == "lhbrx") return Gekko::Instruction::lhbrx;
			else if (str == "lhz") return Gekko::Instruction::lhz;
			else if (str == "lhzu") return Gekko::Instruction::lhzu;
			else if (str == "lhzux") return Gekko::Instruction::lhzux;
			else if (str == "lhzx") return Gekko::Instruction::lhzx;
			else if (str == "lmw") return Gekko::Instruction::lmw;
			else if (str == "lswi") return Gekko::Instruction::lswi;
			else if (str == "lswx") return Gekko::Instruction::lswx;
			else if (str == "lwarx") return Gekko::Instruction::lwarx;
			else if (str == "lwbrx") return Gekko::Instruction::lwbrx;
			else if (str == "lwz") return Gekko::Instruction::lwz;
			else if (str == "lwzu") return Gekko::Instruction::lwzu;
			else if (str == "lwzux") return Gekko::Instruction::lwzux;
			else if (str == "lwzx") return Gekko::Instruction::lwzx;
			
			if (str == "mcrf") return Gekko::Instruction::mcrf;
			else if (str == "mcrfs") return Gekko::Instruction::mcrfs;
			else if (str == "mcrxr") return Gekko::Instruction::mcrxr;
			else if (str == "mfcr") return Gekko::Instruction::mfcr;
			else if (str == "mffs") return Gekko::Instruction::mffs;
			else if (str == "mffs.") return Gekko::Instruction::mffs_d;
			else if (str == "mfmsr") return Gekko::Instruction::mfmsr;
			else if (str == "mfspr") return Gekko::Instruction::mfspr;
			else if (str == "mfsr") return Gekko::Instruction::mfsr;
			else if (str == "mfsrin") return Gekko::Instruction::mfsrin;
			else if (str == "mftb") return Gekko::Instruction::mftb;
			else if (str == "mtcrf") return Gekko::Instruction::mtcrf;
			else if (str == "mtfsb0") return Gekko::Instruction::mtfsb0;
			else if (str == "mtfsb0.") return Gekko::Instruction::mtfsb0_d;
			else if (str == "mtfsb1") return Gekko::Instruction::mtfsb1;
			else if (str == "mtfsb1.") return Gekko::Instruction::mtfsb1_d;
			else if (str == "mtfsf") return Gekko::Instruction::mtfsf;
			else if (str == "mtfsf.") return Gekko::Instruction::mtfsf_d;
			else if (str == "mtfsfi") return Gekko::Instruction::mtfsfi;
			else if (str == "mtfsfi.") return Gekko::Instruction::mtfsfi_d;
			else if (str == "mtmsr") return Gekko::Instruction::mtmsr;
			else if (str == "mtspr") return Gekko::Instruction::mtspr;
			else if (str == "mtsr") return Gekko::Instruction::mtsr;
			else if (str == "mtsrin") return Gekko::Instruction::mtsrin;
			
			if (str == "mulhw") return Gekko::Instruction::mulhw;
			else if (str == "mulhw.") return Gekko::Instruction::mulhw_d;
			else if (str == "mulhwu") return Gekko::Instruction::mulhwu;
			else if (str == "mulhwu.") return Gekko::Instruction::mulhwu_d;
			else if (str == "mulli") return Gekko::Instruction::mulli;
			else if (str == "mullw") return Gekko::Instruction::mullw;
			else if (str == "mullw.") return Gekko::Instruction::mullw_d;
			else if (str == "mullwo") return Gekko::Instruction::mullwo;
			else if (str == "mullwo.") return Gekko::Instruction::mullwo_d;
			else if (str == "nand") return Gekko::Instruction::nand;
			else if (str == "nand.") return Gekko::Instruction::nand_d;
			else if (str == "neg") return Gekko::Instruction::neg;
			else if (str == "neg.") return Gekko::Instruction::neg_d;
			else if (str == "nego") return Gekko::Instruction::nego;
			else if (str == "nego.") return Gekko::Instruction::nego_d;
			else if (str == "nor") return Gekko::Instruction::nor;
			else if (str == "nor.") return Gekko::Instruction::nor_d;
			else if (str == "or") return Gekko::Instruction::_or;
			else if (str == "or.") return Gekko::Instruction::or_d;
			else if (str == "orc") return Gekko::Instruction::orc;
			else if (str == "orc.") return Gekko::Instruction::orc_d;
			else if (str == "ori") return Gekko::Instruction::ori;
			else if (str == "oris") return Gekko::Instruction::oris;
			
			if (str == "psq_l") return Gekko::Instruction::psq_l;
			else if (str == "psq_lu") return Gekko::Instruction::psq_lu;
			else if (str == "psq_lux") return Gekko::Instruction::psq_lux;
			else if (str == "psq_lx") return Gekko::Instruction::psq_lx;
			else if (str == "psq_st") return Gekko::Instruction::psq_st;
			else if (str == "psq_stu") return Gekko::Instruction::psq_stu;
			else if (str == "psq_stux") return Gekko::Instruction::psq_stux;
			else if (str == "psq_stx") return Gekko::Instruction::psq_stx;
			else if (str == "ps_abs") return Gekko::Instruction::ps_abs;
			else if (str == "ps_abs.") return Gekko::Instruction::ps_abs_d;
			else if (str == "ps_add") return Gekko::Instruction::ps_add;
			else if (str == "ps_add.") return Gekko::Instruction::ps_add_d;
			else if (str == "ps_cmpo0") return Gekko::Instruction::ps_cmpo0;
			else if (str == "ps_cmpo1") return Gekko::Instruction::ps_cmpo1;
			else if (str == "ps_cmpu0") return Gekko::Instruction::ps_cmpu0;
			else if (str == "ps_cmpu1") return Gekko::Instruction::ps_cmpu1;
			else if (str == "ps_div") return Gekko::Instruction::ps_div;
			else if (str == "ps_div.") return Gekko::Instruction::ps_div_d;
			else if (str == "ps_madd") return Gekko::Instruction::ps_madd;
			else if (str == "ps_madd.") return Gekko::Instruction::ps_madd_d;
			else if (str == "ps_madds0") return Gekko::Instruction::ps_madds0;
			else if (str == "ps_madds0.") return Gekko::Instruction::ps_madds0_d;
			else if (str == "ps_madds1") return Gekko::Instruction::ps_madds1;
			else if (str == "ps_madds1.") return Gekko::Instruction::ps_madds1_d;
			else if (str == "ps_merge00") return Gekko::Instruction::ps_merge00;
			else if (str == "ps_merge00.") return Gekko::Instruction::ps_merge00_d;
			else if (str == "ps_merge01") return Gekko::Instruction::ps_merge01;
			else if (str == "ps_merge01.") return Gekko::Instruction::ps_merge01_d;
			else if (str == "ps_merge10") return Gekko::Instruction::ps_merge10;
			else if (str == "ps_merge10.") return Gekko::Instruction::ps_merge10_d;
			else if (str == "ps_merge11") return Gekko::Instruction::ps_merge11;
			else if (str == "ps_merge11.") return Gekko::Instruction::ps_merge11_d;
			else if (str == "ps_mr") return Gekko::Instruction::ps_mr;
			else if (str == "ps_mr.") return Gekko::Instruction::ps_mr_d;
			else if (str == "ps_msub") return Gekko::Instruction::ps_msub;
			else if (str == "ps_msub.") return Gekko::Instruction::ps_msub_d;
			else if (str == "ps_mul") return Gekko::Instruction::ps_mul;
			else if (str == "ps_mul.") return Gekko::Instruction::ps_mul_d;
			else if (str == "ps_muls0") return Gekko::Instruction::ps_muls0;
			else if (str == "ps_muls0.") return Gekko::Instruction::ps_muls0_d;
			else if (str == "ps_muls1") return Gekko::Instruction::ps_muls1;
			else if (str == "ps_muls1.") return Gekko::Instruction::ps_muls1_d;
			else if (str == "ps_nabs") return Gekko::Instruction::ps_nabs;
			else if (str == "ps_nabs.") return Gekko::Instruction::ps_nabs_d;
			else if (str == "ps_neg") return Gekko::Instruction::ps_neg;
			else if (str == "ps_neg.") return Gekko::Instruction::ps_neg_d;
			else if (str == "ps_nmadd") return Gekko::Instruction::ps_nmadd;
			else if (str == "ps_nmadd.") return Gekko::Instruction::ps_nmadd_d;
			else if (str == "ps_nmsub") return Gekko::Instruction::ps_nmsub;
			else if (str == "ps_nmsub.") return Gekko::Instruction::ps_nmsub_d;
			else if (str == "ps_res") return Gekko::Instruction::ps_res;
			else if (str == "ps_res.") return Gekko::Instruction::ps_res_d;
			else if (str == "ps_rsqrte") return Gekko::Instruction::ps_rsqrte;
			else if (str == "ps_rsqrte.") return Gekko::Instruction::ps_rsqrte_d;
			else if (str == "ps_sel") return Gekko::Instruction::ps_sel;
			else if (str == "ps_sel.") return Gekko::Instruction::ps_sel_d;
			else if (str == "ps_sub") return Gekko::Instruction::ps_sub;
			else if (str == "ps_sub.") return Gekko::Instruction::ps_sub_d;
			else if (str == "ps_sum0") return Gekko::Instruction::ps_sum0;
			else if (str == "ps_sum0.") return Gekko::Instruction::ps_sum0_d;
			else if (str == "ps_sum1") return Gekko::Instruction::ps_sum1;
			else if (str == "ps_sum1.") return Gekko::Instruction::ps_sum1_d;
			
			if (str == "rfi") return Gekko::Instruction::rfi;
			else if (str == "rlwimi") return Gekko::Instruction::rlwimi;
			else if (str == "rlwimi.") return Gekko::Instruction::rlwimi_d;
			else if (str == "rlwinm") return Gekko::Instruction::rlwinm;
			else if (str == "rlwinm.") return Gekko::Instruction::rlwinm_d;
			else if (str == "rlwnm") return Gekko::Instruction::rlwnm;
			else if (str == "rlwnm.") return Gekko::Instruction::rlwnm_d;
			else if (str == "sc") return Gekko::Instruction::sc;
			else if (str == "slw") return Gekko::Instruction::slw;
			else if (str == "slw.") return Gekko::Instruction::slw_d;
			else if (str == "sraw") return Gekko::Instruction::sraw;
			else if (str == "sraw.") return Gekko::Instruction::sraw_d;
			else if (str == "srawi") return Gekko::Instruction::srawi;
			else if (str == "srawi.") return Gekko::Instruction::srawi_d;
			else if (str == "srw") return Gekko::Instruction::srw;
			else if (str == "srw.") return Gekko::Instruction::srw_d;
			
			if (str == "stb") return Gekko::Instruction::stb;
			else if (str == "stbu") return Gekko::Instruction::stbu;
			else if (str == "stbux") return Gekko::Instruction::stbux;
			else if (str == "stbx") return Gekko::Instruction::stbx;
			else if (str == "stfd") return Gekko::Instruction::stfd;
			else if (str == "stfdu") return Gekko::Instruction::stfdu;
			else if (str == "stfdux") return Gekko::Instruction::stfdux;
			else if (str == "stfdx") return Gekko::Instruction::stfdx;
			else if (str == "stfiwx") return Gekko::Instruction::stfiwx;
			else if (str == "stfs") return Gekko::Instruction::stfs;
			else if (str == "stfsu") return Gekko::Instruction::stfsu;
			else if (str == "stfsux") return Gekko::Instruction::stfsux;
			else if (str == "stfsx") return Gekko::Instruction::stfsx;
			else if (str == "sth") return Gekko::Instruction::sth;
			else if (str == "sthbrx") return Gekko::Instruction::sthbrx;
			else if (str == "sthu") return Gekko::Instruction::sthu;
			else if (str == "sthux") return Gekko::Instruction::sthux;
			else if (str == "sthx") return Gekko::Instruction::sthx;
			else if (str == "stmw") return Gekko::Instruction::stmw;
			else if (str == "stswi") return Gekko::Instruction::stswi;
			else if (str == "stswx") return Gekko::Instruction::stswx;
			else if (str == "stw") return Gekko::Instruction::stw;
			else if (str == "stwbrx") return Gekko::Instruction::stwbrx;
			else if (str == "stwcx.") return Gekko::Instruction::stwcx_d;
			else if (str == "stwu") return Gekko::Instruction::stwu;
			else if (str == "stwux") return Gekko::Instruction::stwux;
			else if (str == "stwx") return Gekko::Instruction::stwx;
			
			if (str == "subf") return Gekko::Instruction::subf;
			else if (str == "subf.") return Gekko::Instruction::subf_d;
			else if (str == "subfo") return Gekko::Instruction::subfo;
			else if (str == "subfo.") return Gekko::Instruction::subfo_d;
			else if (str == "subfc") return Gekko::Instruction::subfc;
			else if (str == "subfc.") return Gekko::Instruction::subfc_d;
			else if (str == "subfco") return Gekko::Instruction::subfco;
			else if (str == "subfco.") return Gekko::Instruction::subfco_d;
			else if (str == "subfe") return Gekko::Instruction::subfe;
			else if (str == "subfe.") return Gekko::Instruction::subfe_d;
			else if (str == "subfeo") return Gekko::Instruction::subfeo;
			else if (str == "subfeo.") return Gekko::Instruction::subfeo_d;
			else if (str == "subfic") return Gekko::Instruction::subfic;
			else if (str == "subfme") return Gekko::Instruction::subfme;
			else if (str == "subfme.") return Gekko::Instruction::subfme_d;
			else if (str == "subfmeo") return Gekko::Instruction::subfmeo;
			else if (str == "subfmeo.") return Gekko::Instruction::subfmeo_d;
			else if (str == "subfze") return Gekko::Instruction::subfze;
			else if (str == "subfze.") return Gekko::Instruction::subfze_d;
			else if (str == "subfzeo") return Gekko::Instruction::subfzeo;
			else if (str == "subfzeo.") return Gekko::Instruction::subfzeo_d;
			else if (str == "sync") return Gekko::Instruction::sync;
			else if (str == "tlbie") return Gekko::Instruction::tlbie;
			else if (str == "tlbsync") return Gekko::Instruction::tlbsync;
			else if (str == "tw") return Gekko::Instruction::tw;
			else if (str == "twi") return Gekko::Instruction::twi;
			else if (str == "xor") return Gekko::Instruction::_xor;
			else if (str == "xor.") return Gekko::Instruction::xor_d;
			else if (str == "xori") return Gekko::Instruction::xori;
			else if (str == "xoris") return Gekko::Instruction::xoris;

			else return Gekko::Instruction::Unknown;
		}

		void StrToParam(Json::Value *val, Gekko::Param &p, int& bits)
		{
			if (std::string(val->name) == "r")
			{
				p = Gekko::Param::Reg;
				bits = (int)val->value.AsInt;
			}
			else if (std::string(val->name) == "fr")
			{
				p = Gekko::Param::FReg;
				bits = (int)val->value.AsInt;
			}
		}

		void DispatchAsInterpreter(Gekko::GekkoCore* core, Json::Value* instr, Json::Value* param, Json::Value* notes)
		{
			std::string instrName = Util::WstringToString(instr->value.AsString);

			// Assemble the instruction

			Gekko::AnalyzeInfo info = { 0 };

			info.instr = StrToInstr(instrName);

			for (auto p = param->children.begin(); p != param->children.end(); ++p)
			{
				StrToParam((*p)->children.front(), info.param[info.numParam], info.paramBits[info.numParam]);
				info.numParam++;
			}

			Gekko::GekkoAssembler::Assemble(info);

			// Output information about the current instruction.

			std::string disassembledInstr = Gekko::GekkoDisasm::Disasm(core->regs.pc, &info, true, true);

			if (notes)
			{
				std::wstring notesWstr(notes->value.AsString);
				Logger::WriteMessage((disassembledInstr + " (" + Util::WstringToString(notesWstr) + ")\n").c_str());
			}
			else
			{
				Logger::WriteMessage((disassembledInstr + "\n").c_str());
			}

			// Emit instruction at `pc`

			int wimg;
			uint32_t pa = core->EffectiveToPhysical(core->regs.pc, Gekko::MmuAccess::Write, wimg);
			Assert::IsTrue(pa != Gekko::BadAddress);

			PIWriteWord(pa, info.instrBits);

			// Execute a single instruction with an interpreter

			core->interp->ExecuteOpcode();
		}

		void DispatchAsRecompiler(Gekko::GekkoCore* core, Json::Value* instr, Json::Value* param, Json::Value* notes)
		{
			std::string instrName = Util::WstringToString(instr->value.AsString);

			// Assemble the instruction

			Gekko::AnalyzeInfo info = { 0 };

			info.instr = StrToInstr(instrName);

			for (auto p = param->children.begin(); p != param->children.end(); ++p)
			{
				StrToParam((*p)->children.front(), info.param[info.numParam], info.paramBits[info.numParam]);
				info.numParam++;
			}

			Gekko::GekkoAssembler::Assemble(info);

			// Output information about the current instruction.

			std::string disassembledInstr = Gekko::GekkoDisasm::Disasm(core->regs.pc, &info, true, true);

			if (notes)
			{
				std::wstring notesWstr(notes->value.AsString);
				Logger::WriteMessage((disassembledInstr + " (" + Util::WstringToString(notesWstr) + ")\n").c_str());
			}
			else
			{
				Logger::WriteMessage((disassembledInstr + "\n").c_str());
			}

			// Emit instruction at `pc`

			int wimg;
			uint32_t pa = core->EffectiveToPhysical(core->regs.pc, Gekko::MmuAccess::Write, wimg);
			Assert::IsTrue(pa != Gekko::BadAddress);

			PIWriteWord(pa, info.instrBits);

			// Execute a single instruction with recompiler

			core->jitc->maxInstructions = 1;
			core->jitc->Execute();
		}

		void CheckContext(Gekko::GekkoCore* core, Json::Value* expected)
		{
			for (auto v = expected->children.begin(); v != expected->children.end(); ++v)
			{
				Json::Value* r = (*v)->children.front();
				std::string name = std::string(r->name);

				if (name == "pc")
				{
					Assert::IsTrue(core->regs.pc == GetRegVal(r));
				}

				else if (r->name && r->name[0] == 'r')
				{
					Assert::IsTrue(core->regs.gpr[atoi(r->name + 1)] == GetRegVal(r));
				}
				else if (r->name && strlen(r->name) > 2 && r->name[0] == 'f' && r->name[1] == 'r')
				{
					Assert::IsTrue(core->regs.fpr[atoi(r->name + 2)].dbl == GetFRegVal(r));
				}
				else if (r->name && strlen(r->name) > 2 && r->name[0] == 'p' && r->name[1] == 's')
				{
					Assert::IsTrue(core->regs.ps1[atoi(r->name + 2)].dbl == GetFRegVal(r));
				}

				else if (name == "CR0[LT]")
				{
					if (r->value.AsInt) Assert::IsTrue ( (core->regs.cr & GEKKO_CR0_LT) != 0);
					else Assert::IsTrue ( (core->regs.cr & GEKKO_CR0_LT) == 0);
				}
				else if (name == "CR0[GT]")
				{
					if (r->value.AsInt) Assert::IsTrue((core->regs.cr & GEKKO_CR0_GT) != 0);
					else Assert::IsTrue((core->regs.cr & GEKKO_CR0_GT) == 0);
				}
				else if (name == "CR0[EQ]")
				{
					if (r->value.AsInt) Assert::IsTrue((core->regs.cr & GEKKO_CR0_EQ) != 0);
					else Assert::IsTrue((core->regs.cr & GEKKO_CR0_EQ) == 0);
				}
				else if (name == "CR0[SO]")
				{
					if (r->value.AsInt) Assert::IsTrue((core->regs.cr & GEKKO_CR0_SO) != 0);
					else Assert::IsTrue((core->regs.cr & GEKKO_CR0_SO) == 0);
				}

				else if (name == "XER[SO]")
				{
					if (r->value.AsInt) Assert::IsTrue((core->regs.spr[Gekko::SPR::XER] & GEKKO_XER_SO) != 0);
					else Assert::IsTrue((core->regs.spr[Gekko::SPR::XER] & GEKKO_XER_SO) == 0);
				}
				else if (name == "XER[OV]")
				{
					if (r->value.AsInt) Assert::IsTrue((core->regs.spr[Gekko::SPR::XER] & GEKKO_XER_OV) != 0);
					else Assert::IsTrue((core->regs.spr[Gekko::SPR::XER] & GEKKO_XER_OV) == 0);
				}
				else if (name == "XER[CA]")
				{
					if (r->value.AsInt) Assert::IsTrue((core->regs.spr[Gekko::SPR::XER] & GEKKO_XER_CA) != 0);
					else Assert::IsTrue((core->regs.spr[Gekko::SPR::XER] & GEKKO_XER_CA) == 0);
				}
			}
		}

		TEST_METHOD(GekkoISATest)
		{
			// Load JSON with the rules.

			SetCWDToTest();

			std::vector<uint8_t> text;
			Json json;

			LoadJson(std::string("GekkoIsaTests.json"), text);
			json.Deserialize(text.data(), text.size());

			SetCWDToDolwin();

			// Enumerate the rules

			Json::Value* rules = json.root.children.front();

			for (auto s = rules->children.begin(); s != rules->children.end(); ++s)
			{
				if (std::string((*s)->name) == "rules")
				{
					Logger::WriteMessage("Dispatching instructions (interpreter mode):\n");

					for (auto i = (*s)->children.begin(); i != (*s)->children.end(); ++i)
					{
						// Get key sections to check the next instruction

						Json::Value* instr = nullptr;
						Json::Value* param = nullptr;
						Json::Value* notes = nullptr;
						Json::Value* before = nullptr;
						Json::Value* expected = nullptr;

						for (auto p = (*i)->children.begin(); p != (*i)->children.end(); ++p)
						{
							if (std::string((*p)->name) == "instr")
							{
								instr = *p;
							}
							if (std::string((*p)->name) == "param")
							{
								param = *p;
							}
							if (std::string((*p)->name) == "notes")
							{
								notes = *p;
							}
							if (std::string((*p)->name) == "before")
							{
								before = *p;
							}
							if (std::string((*p)->name) == "expected")
							{
								expected = *p;
							}
						}

						Assert::IsNotNull(instr);
						Assert::IsNotNull(param);
						Assert::IsNotNull(before);
						Assert::IsNotNull(expected);

						// Instantiate GekkoCore

						Gekko::GekkoCore* core = new Gekko::GekkoCore();
						core->Suspend();

						// Initialize the memory model (flat Dolphin OS) and set initial register values.

						SetupDolphinOSMemoryModel(core);
						PrepareContext(core, before);

						// Execute the instruction in interpreter mode.

						DispatchAsInterpreter(core, instr, param, notes);

						// Check the registers for the expected values.

						CheckContext(core, expected);

						delete core;
					}

					Logger::WriteMessage("\nDispatching instructions (recompiler mode):\n");

					for (auto i = (*s)->children.begin(); i != (*s)->children.end(); ++i)
					{
						// Get key sections to check the next instruction

						Json::Value* instr = nullptr;
						Json::Value* param = nullptr;
						Json::Value* notes = nullptr;
						Json::Value* before = nullptr;
						Json::Value* expected = nullptr;

						for (auto p = (*i)->children.begin(); p != (*i)->children.end(); ++p)
						{
							if (std::string((*p)->name) == "instr")
							{
								instr = *p;
							}
							if (std::string((*p)->name) == "param")
							{
								param = *p;
							}
							if (std::string((*p)->name) == "notes")
							{
								notes = *p;
							}
							if (std::string((*p)->name) == "before")
							{
								before = *p;
							}
							if (std::string((*p)->name) == "expected")
							{
								expected = *p;
							}
						}

						Assert::IsNotNull(instr);
						Assert::IsNotNull(param);
						Assert::IsNotNull(before);
						Assert::IsNotNull(expected);

						// Instantiate GekkoCore

						Gekko::GekkoCore* core = new Gekko::GekkoCore();
						Gekko::Gekko = core;		// Required by recompiler static members.
						core->Suspend();

						// Initialize the memory model (flat Dolphin OS) and set initial register values.

						SetupDolphinOSMemoryModel(core);
						PrepareContext(core, before);

						// Execute the instruction in recompiler mode.

						DispatchAsRecompiler(core, instr, param, notes);

						// Check the registers for the expected values.

						CheckContext(core, expected);

						delete core;
					}
				}
			}
		}

#pragma endregion "GekkoISA Tests"

	};
}
