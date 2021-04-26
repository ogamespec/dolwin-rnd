#include "pch.h"
#include "CppUnitTest.h"

// Recommended online service for checking opcodes (supports modes 16, 32, 64):
// http://shell-storm.org/online/Online-Assembler-and-Disassembler

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace IntelCore;

namespace IntelAssemblerUnitTest
{
	TEST_CLASS(IntelAssemblerUnitTest)
	{
		void Check(AnalyzeInfo& info, char* bytes, size_t size)
		{
			uint8_t compiledInstr[32] = { 0 };
			size_t n = 0;

			for (size_t i = 0; i < info.prefixSize; i++)
			{
				compiledInstr[n++] = info.prefixBytes[i];
			}

			for (size_t i = 0; i < info.instrSize; i++)
			{
				compiledInstr[n++] = info.instrBytes[i];
			}

			Assert::IsTrue(memcmp(compiledInstr, bytes, size) == 0);
		}

		void ClearInfo(AnalyzeInfo& info)
		{
			memset(&info, 0, sizeof(info));
		}

	public:

		TEST_METHOD(SIB_mechanism)
		{
			Param groups[] = { 
				Param::MemSib32Scale1Start, Param::MemSib32Scale2Start, Param::MemSib32Scale4Start, Param::MemSib32Scale8Start, 
				Param::MemSib32Scale1Disp8Start, Param::MemSib32Scale2Disp8Start, Param::MemSib32Scale4Disp8Start, Param::MemSib32Scale8Disp8Start, 
				Param::MemSib32Scale1Disp32Start, Param::MemSib32Scale2Disp32Start, Param::MemSib32Scale4Disp32Start, Param::MemSib32Scale8Disp32Start, 
				Param::MemSib64Scale1Start, Param::MemSib64Scale2Start, Param::MemSib64Scale4Start, Param::MemSib64Scale8Start, 
				Param::MemSib64Scale1Disp8Start, Param::MemSib64Scale2Disp8Start, Param::MemSib64Scale4Disp8Start, Param::MemSib64Scale8Disp8Start, 
				Param::MemSib64Scale1Disp32Start, Param::MemSib64Scale2Disp32Start, Param::MemSib64Scale4Disp32Start, Param::MemSib64Scale8Disp32Start, };
			
			for (size_t g = 0; g < _countof(groups); g++)
			{
				size_t expected_scale = g % 4;

				for (int n = 0; n < 0x100; n++)
				{
					size_t scale, index, base;

					size_t expected_index = (n >> 4) & 0xf;
					size_t expected_base = n & 0xf;

					IntelAssembler::GetSS((Param)((size_t)groups[g] + n + 1), scale);
					IntelAssembler::GetIndex((Param)((size_t)groups[g] + n + 1), index);
					IntelAssembler::GetBase((Param)((size_t)groups[g] + n + 1), base);

					Assert::IsTrue(scale < 4);
					Assert::IsTrue(index < 16);
					Assert::IsTrue(base < 16);

					Assert::IsTrue(scale == expected_scale);
					Assert::IsTrue(index == expected_index);
					Assert::IsTrue(base == expected_base);
				}
			}
		}

		/// <summary>
		/// Check the correct processing of the ModRegRM boundary conditions for the 16-bit mode 
		/// Param 0: register
		/// Param 1: rm
		/// </summary>
		TEST_METHOD(ModRegRm_16)
		{
			AnalyzeInfo info = { 0 };

			// Register - Register

			// adc al, bl  "\x12\xc3"

			info.params[0] = Param::al;
			info.params[1] = Param::bl;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
			Check(info, "\x12\xc3", 2);
			ClearInfo(info);

			// adc ax, bx	"\x13\xc3"

			info.params[0] = Param::ax;
			info.params[1] = Param::bx;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
			Check(info, "\x13\xc3", 2);
			ClearInfo(info);

			// adc eax, ebx		"\x66\x13\xc3"

			info.params[0] = Param::eax;
			info.params[1] = Param::ebx;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
			Check(info, "\x66\x13\xc3", 3);
			ClearInfo(info);

			// adc al, r8  -- Fail

			Assert::ExpectException<char const*>([]() {
				AnalyzeInfo info = { 0 };
				info.params[0] = Param::al;
				info.params[1] = Param::r8;
				info.numParams = 2;
				IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
				});
			ClearInfo(info);

			// adc ax, r8w  -- Fail

			Assert::ExpectException<char const*>([]() {
				AnalyzeInfo info = { 0 };
				info.params[0] = Param::ax;
				info.params[1] = Param::r8w;
				info.numParams = 2;
				IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
				});
			ClearInfo(info);

			// adc rax, rbx  -- Fail

			Assert::ExpectException<char const*>([]() {
				AnalyzeInfo info = { 0 };
				info.params[0] = Param::rax;
				info.params[1] = Param::rbx;
				info.numParams = 2;
				IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
				});
			ClearInfo(info);

			// Register - Memory16

			// adc al, [bx + si]		"\x12\x00"

			info.params[0] = Param::al;
			info.params[1] = Param::m_bx_si;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
			Check(info, "\x12\x00", 2);
			ClearInfo(info);

			// adc al, [0x1234]	"\x12\x06\x34\x12"

			info.params[0] = Param::al;
			info.params[1] = Param::m_disp16;
			info.numParams = 2;
			info.Disp.disp16 = 0x1234;
			IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
			Check(info, "\x12\x06\x34\x12", 4);
			ClearInfo(info);

			// adc al, [si + 0xaa]		"\x12\x44\xaa"

			info.params[0] = Param::al;
			info.params[1] = Param::m_si_disp8;
			info.numParams = 2;
			info.Disp.disp8 = 0xaa;
			IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
			Check(info, "\x12\x44\xaa", 3);
			ClearInfo(info);

			// adc al, [si + 0x1234]	"\x12\x84\x34\x12"

			info.params[0] = Param::al;
			info.params[1] = Param::m_si_disp16;
			info.numParams = 2;
			info.Disp.disp16 = 0x1234;
			IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
			Check(info, "\x12\x84\x34\x12", 4);
			ClearInfo(info);

			// adc ax, [bx + si]		"\x13\x00"

			info.params[0] = Param::ax;
			info.params[1] = Param::m_bx_si;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
			Check(info, "\x13\x00", 2);
			ClearInfo(info);

			// adc ax, [0x1234]		"\x13\x06\x34\x12"

			info.params[0] = Param::ax;
			info.params[1] = Param::m_disp16;
			info.numParams = 2;
			info.Disp.disp16 = 0x1234;
			IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
			Check(info, "\x13\x06\x34\x12", 4);
			ClearInfo(info);

			// adc ax, [si + 0xaa]			"\x13\x44\xaa"

			info.params[0] = Param::ax;
			info.params[1] = Param::m_si_disp8;
			info.numParams = 2;
			info.Disp.disp8 = 0xaa;
			IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
			Check(info, "\x13\x44\xaa", 3);
			ClearInfo(info);

			// adc ax, [si + 0x1234]	"\x13\x84\x34\x12"

			info.params[0] = Param::ax;
			info.params[1] = Param::m_si_disp16;
			info.numParams = 2;
			info.Disp.disp16 = 0x1234;
			IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
			Check(info, "\x13\x84\x34\x12", 4);
			ClearInfo(info);

			// adc eax, [bx + si]		"\x66\x13\x00"

			info.params[0] = Param::eax;
			info.params[1] = Param::m_bx_si;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
			Check(info, "\x66\x13\x00", 3);
			ClearInfo(info);

			// adc eax, [0x1234]		"\x66\x13\x06\x34\x12"

			info.params[0] = Param::eax;
			info.params[1] = Param::m_disp16;
			info.numParams = 2;
			info.Disp.disp16 = 0x1234;
			IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
			Check(info, "\x66\x13\x06\x34\x12", 5);
			ClearInfo(info);

			// adc eax, [si + 0xaa]	"\x66\x13\x44\xaa"

			info.params[0] = Param::eax;
			info.params[1] = Param::m_si_disp8;
			info.numParams = 2;
			info.Disp.disp8 = 0xaa;
			IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
			Check(info, "\x66\x13\x44\xaa", 4);
			ClearInfo(info);

			// adc eax, [si + 0x1234]	"\x66\x13\x84\x34\x12"

			info.params[0] = Param::eax;
			info.params[1] = Param::m_si_disp16;
			info.numParams = 2;
			info.Disp.disp16 = 0x1234;
			IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
			Check(info, "\x66\x13\x84\x34\x12", 4);
			ClearInfo(info);

			// adc rax, [bx + si] -- Fail

			Assert::ExpectException<char const*>([]() {
				AnalyzeInfo info = { 0 };
				info.params[0] = Param::rax;
				info.params[1] = Param::m_bx_si;
				info.numParams = 2;
				IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
				});
			ClearInfo(info);

			// Register - Memory32

			// adc al, [eax]		"\x67\x12\x00"

			info.params[0] = Param::al;
			info.params[1] = Param::m_eax;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
			Check(info, "\x67\x12\x00", 3);
			ClearInfo(info);

			// adc al, [esi + 0x11223344]	"\x67\x12\x86\x44\x33\x22\x11"

			info.params[0] = Param::al;
			info.params[1] = Param::m_esi_disp32;
			info.numParams = 2;
			info.Disp.disp32 = 0x11223344;
			IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
			Check(info, "\x67\x12\x86\x44\x33\x22\x11", 7);
			ClearInfo(info);

			// adc ax, [eax]	"\x67\x13\x00"

			info.params[0] = Param::ax;
			info.params[1] = Param::m_eax;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
			Check(info, "\x67\x13\x00", 3);
			ClearInfo(info);

			// adc ax, [esi + 0x11223344]	"\x67\x13\x86\x44\x33\x22\x11"

			info.params[0] = Param::ax;
			info.params[1] = Param::m_esi_disp32;
			info.numParams = 2;
			info.Disp.disp32 = 0x11223344;
			IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
			Check(info, "\x67\x13\x86\x44\x33\x22\x11", 7);
			ClearInfo(info);

			// adc eax, [eax]		"\x66\x67\x13\x00"

			info.params[0] = Param::eax;
			info.params[1] = Param::m_eax;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
			Check(info, "\x66\x67\x13\x00", 4);
			ClearInfo(info);

			// adc eax, [esi + 0x11223344]			"\x66\x67\x13\x86\x44\x33\x22\x11"

			info.params[0] = Param::eax;
			info.params[1] = Param::m_esi_disp32;
			info.numParams = 2;
			info.Disp.disp32 = 0x11223344;
			IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
			Check(info, "\x66\x67\x13\x86\x44\x33\x22\x11", 8);
			ClearInfo(info);

			// Register - Memory32 + SIB

			// adc al, [eax * 2 + ecx]		"\x67\x12\x04\x41"

			info.params[0] = Param::al;
			info.params[1] = Param::sib_eax_2_ecx;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
			Check(info, "\x67\x12\x04\x41", 4);
			ClearInfo(info);

			// adc ax, [eax * 2 + ecx]		"\x67\x13\x04\x41"

			info.params[0] = Param::ax;
			info.params[1] = Param::sib_eax_2_ecx;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
			Check(info, "\x67\x13\x04\x41", 4);
			ClearInfo(info);

			// adc eax, [eax * 2 + ecx]		"\x66\x67\x13\x04\x41"

			info.params[0] = Param::eax;
			info.params[1] = Param::sib_eax_2_ecx;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
			Check(info, "\x66\x67\x13\x04\x41", 5);
			ClearInfo(info);

			// adc al, [eax * 2 + ecx + 0x11223344]	"\x67\x12\x84\x41\x44\x33\x22\x11"

			info.params[0] = Param::al;
			info.params[1] = Param::sib_eax_2_ecx_disp32;
			info.numParams = 2;
			info.Disp.disp32 = 0x11223344;
			IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
			Check(info, "\x67\x12\x84\x41\x44\x33\x22\x11", 8);
			ClearInfo(info);

			// adc ax, [eax * 2 + ecx + 0x11223344]	"\x67\x13\x84\x41\x44\x33\x22\x11"

			info.params[0] = Param::ax;
			info.params[1] = Param::sib_eax_2_ecx_disp32;
			info.numParams = 2;
			info.Disp.disp32 = 0x11223344;
			IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
			Check(info, "\x67\x13\x84\x41\x44\x33\x22\x11", 8);
			ClearInfo(info);

			// adc eax, [eax * 2 + ecx + 0x11223344]	"\x66\x67\x13\x84\x41\x44\x33\x22\x11"

			info.params[0] = Param::eax;
			info.params[1] = Param::sib_eax_2_ecx_disp32;
			info.numParams = 2;
			info.Disp.disp32 = 0x11223344;
			IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
			Check(info, "\x66\x67\x13\x84\x41\x44\x33\x22\x11", 9);
			ClearInfo(info);

			// Register - Memory64

			// adc al, [rax]  -- Fail

			Assert::ExpectException<char const*>([]() {
				AnalyzeInfo info = { 0 };
				info.params[0] = Param::al;
				info.params[1] = Param::m_rax;
				info.numParams = 2;
				IntelAssembler::HandleModRegRm(info, 16, 0, 1, 0x12, 0x13);
				});
			ClearInfo(info);
		}

		/// <summary>
		/// Testing ModRM/SIB addressing for 32-bit mode.
		/// Same as 16-bit version - check favorite addressing options.
		/// </summary>
		TEST_METHOD(ModRegRm_32)
		{
			AnalyzeInfo info = { 0 };

			// Register - Register

			// adc al, bl	"\x12\xc3"

			info.params[0] = Param::al;
			info.params[1] = Param::bl;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 32, 0, 1, 0x12, 0x13);
			Check(info, "\x12\xc3", 2);
			ClearInfo(info);

			// adc ax, bx	"\x66\x13\xc3"

			info.params[0] = Param::ax;
			info.params[1] = Param::bx;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 32, 0, 1, 0x12, 0x13);
			Check(info, "\x66\x13\xc3", 3);
			ClearInfo(info);

			// adc eax, ebx		"\x13\xc3"

			info.params[0] = Param::eax;
			info.params[1] = Param::ebx;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 32, 0, 1, 0x12, 0x13);
			Check(info, "\x13\xc3", 2);
			ClearInfo(info);

			// adc eax, al  -- Fail

			Assert::ExpectException<char const*>([]() {
				AnalyzeInfo info = { 0 };
				info.params[0] = Param::eax;
				info.params[1] = Param::al;
				info.numParams = 2;
				IntelAssembler::HandleModRegRm(info, 32, 0, 1, 0x12, 0x13);
				});
			ClearInfo(info);

			// Register - Memory16

			// adc al, [bp + si + 0xaa]		"\x67\x12\x42\xaa"

			info.params[0] = Param::al;
			info.params[1] = Param::m_bp_si_disp8;
			info.numParams = 2;
			info.Disp.disp8 = 0xaa;
			IntelAssembler::HandleModRegRm(info, 32, 0, 1, 0x12, 0x13);
			Check(info, "\x67\x12\x42\xaa", 4);
			ClearInfo(info);

			// adc ax, [bp + si + 0xaa]		"\x66\x67\x13\x42\xaa"

			info.params[0] = Param::ax;
			info.params[1] = Param::m_bp_si_disp8;
			info.numParams = 2;
			info.Disp.disp8 = 0xaa;
			IntelAssembler::HandleModRegRm(info, 32, 0, 1, 0x12, 0x13);
			Check(info, "\x66\x67\x13\x42\xaa", 5);
			ClearInfo(info);

			// adc eax, [bp + si + 0xaa]	"\x67\x13\x42\xaa"

			info.params[0] = Param::eax;
			info.params[1] = Param::m_bp_si_disp8;
			info.numParams = 2;
			info.Disp.disp8 = 0xaa;
			IntelAssembler::HandleModRegRm(info, 32, 0, 1, 0x12, 0x13);
			Check(info, "\x67\x13\x42\xaa", 4);
			ClearInfo(info);

			// Register - Memory32

			// adc al, [ebx]		"\x12\x03"

			info.params[0] = Param::al;
			info.params[1] = Param::m_ebx;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 32, 0, 1, 0x12, 0x13);
			Check(info, "\x12\x03", 2);
			ClearInfo(info);

			// adc ax, [ebx]		"\x66\x13\x03"

			info.params[0] = Param::ax;
			info.params[1] = Param::m_ebx;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 32, 0, 1, 0x12, 0x13);
			Check(info, "\x66\x13\x03", 3);
			ClearInfo(info);

			// adc eax, [ebx]		"\x13\x03"

			info.params[0] = Param::eax;
			info.params[1] = Param::m_ebx;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 32, 0, 1, 0x12, 0x13);
			Check(info, "\x13\x03", 2);
			ClearInfo(info);

			// Register - Memory32 + SIB

			// adc al, [ebp * 2 + 0x11223344]		"\x12\x04\x6d\x44\x33\x22\x11"

			info.params[0] = Param::al;
			info.params[1] = Param::sib_ebp_2_disp32;
			info.numParams = 2;
			info.Disp.disp32 = 0x11223344;
			IntelAssembler::HandleModRegRm(info, 32, 0, 1, 0x12, 0x13);
			Check(info, "\x12\x04\x6d\x44\x33\x22\x11", 7);
			ClearInfo(info);

			// adc ax, [esi * 2 + edx]		"\x66\x13\x04\x72"

			info.params[0] = Param::ax;
			info.params[1] = Param::sib_esi_2_edx;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 32, 0, 1, 0x12, 0x13);
			Check(info, "\x66\x13\x04\x72", 4);
			ClearInfo(info);

			// adc eax, [esi * 2 + esi]		"\x13\x04\x76"

			info.params[0] = Param::eax;
			info.params[1] = Param::sib_esi_2_esi;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 32, 0, 1, 0x12, 0x13);
			Check(info, "\x13\x04\x76", 3);
			ClearInfo(info);

			// Register - Memory64

			// adc al, [rax]  -- Fail

			Assert::ExpectException<char const*>([]() {
				AnalyzeInfo info = { 0 };
				info.params[0] = Param::al;
				info.params[1] = Param::m_rax;
				info.numParams = 2;
				IntelAssembler::HandleModRegRm(info, 32, 0, 1, 0x12, 0x13);
				});
			ClearInfo(info);

			// adc eax, [rax * 2 + rax] -- Fail

			Assert::ExpectException<char const*>([]() {
				AnalyzeInfo info = { 0 };
				info.params[0] = Param::eax;
				info.params[1] = Param::sib_rax_2_rax;
				info.numParams = 2;
				IntelAssembler::HandleModRegRm(info, 32, 0, 1, 0x12, 0x13);
				});
			ClearInfo(info);
		}

		/// <summary>
		/// Tests a group of opcodes for an `adc` instruction.
		/// Only basic testing is done to make sure the opcodes are correctly encoded.
		/// ModRM addressing is verified in other tests.
		/// </summary>
		TEST_METHOD(adc)
		{
			// I
			Check(IntelAssembler::adc<16>(Param::al, Param::imm8, 0, 0xaa), "\x14\xaa", 2);
			Check(IntelAssembler::adc<16>(Param::ax, Param::imm16, 0, 0x1234), "\x15\x34\x12", 3);
			Check(IntelAssembler::adc<16>(Param::eax, Param::imm32, 0, 0x12345678), "\x66\x15\x78\x56\x34\x12", 6);
			Check(IntelAssembler::adc<64>(Param::rax, Param::imm32, 0, 0x12345678), "\x48\x15\x78\x56\x34\x12", 6);

			// MI

			// MR
			Check(IntelAssembler::adc<16>(Param::m_bp_di, Param::al, 0, 0), "\x10\x03", 2);
			Check(IntelAssembler::adc<64>(Param::sib_r11_4_rdx, Param::r8b, 0, 0), "\x4E\x10\x04\x9a", 4);
			Check(IntelAssembler::adc<16>(Param::m_bx, Param::ax, 0, 0), "\x11\x07", 2);
			Check(IntelAssembler::adc<32>(Param::m_ebx, Param::eax, 0, 0), "\x11\x03", 2);
			Check(IntelAssembler::adc<64>(Param::m_rbx, Param::rax, 0, 0), "\x48\x11\x03", 3);
			Check(IntelAssembler::adc<64>(Param::m_eax, Param::rax, 0, 0), "\x67\x48\x11\x00", 4);

			// RM
			Check(IntelAssembler::adc<16>(Param::al, Param::m_bp_di, 0, 0), "\x12\x03", 2);
			Check(IntelAssembler::adc<64>(Param::r8b, Param::sib_r11_4_rdx, 0, 0), "\x4E\x12\x04\x9a", 4);
			Check(IntelAssembler::adc<16>(Param::ax, Param::m_bx, 0, 0), "\x13\x07", 2);
			Check(IntelAssembler::adc<32>(Param::eax, Param::m_ebx, 0, 0), "\x13\x03", 2);
			Check(IntelAssembler::adc<64>(Param::rax, Param::m_rbx, 0, 0), "\x48\x13\x03", 3);

		}

		TEST_METHOD(nop)
		{
			Check(IntelAssembler::nop<16>(), "\x90", 1);
		}
	};
}
