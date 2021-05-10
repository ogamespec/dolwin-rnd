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

			Assert::IsTrue(memcmp(compiledInstr, bytes, size) == 0 && (info.prefixSize + info.instrSize) == size);
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
			Check(info, "\x66\x13\x84\x34\x12", 5);
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
		/// Testing ModRM/SIB addressing for 64-bit mode.
		/// Same as 16-bit version - check favorite addressing options.
		/// </summary>
		TEST_METHOD(ModRegRm_64)
		{
			AnalyzeInfo info = { 0 };

			// Register - Register

			// adc ah, r12b   -- Fail

			Assert::ExpectException<char const*>([]() {
				AnalyzeInfo info = { 0 };
				info.params[0] = Param::ah;
				info.params[1] = Param::r12b;
				info.numParams = 2;
				IntelAssembler::HandleModRegRm(info, 64, 0, 1, 0x12, 0x13);
				});
			ClearInfo(info);

			// adc al, bl	"\x12\xc3"

			info.params[0] = Param::al;
			info.params[1] = Param::bl;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 64, 0, 1, 0x12, 0x13);
			Check(info, "\x12\xc3", 2);
			ClearInfo(info);

			// adc bpl, sil		"\x40\x12\xee"

			info.params[0] = Param::bpl;
			info.params[1] = Param::sil;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 64, 0, 1, 0x12, 0x13);
			Check(info, "\x40\x12\xee", 3);
			ClearInfo(info);

			// adc ax, bx	"\x66\x13\xc3"

			info.params[0] = Param::ax;
			info.params[1] = Param::bx;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 64, 0, 1, 0x12, 0x13);
			Check(info, "\x66\x13\xc3", 3);
			ClearInfo(info);

			// adc r8w, r9w		"\x66\x45\x13\xc1"

			info.params[0] = Param::r8w;
			info.params[1] = Param::r9w;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 64, 0, 1, 0x12, 0x13);
			Check(info, "\x66\x45\x13\xc1", 4);
			ClearInfo(info);

			// adc eax, ebx		"\x13\xc3"

			info.params[0] = Param::eax;
			info.params[1] = Param::ebx;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 64, 0, 1, 0x12, 0x13);
			Check(info, "\x13\xc3", 2);
			ClearInfo(info);

			// adc r8d, r9d		"\x45\x13\xc1"

			info.params[0] = Param::r8d;
			info.params[1] = Param::r9d;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 64, 0, 1, 0x12, 0x13);
			Check(info, "\x45\x13\xc1", 3);
			ClearInfo(info);

			// adc rax, rbx		"\x48\x13\xc3"

			info.params[0] = Param::rax;
			info.params[1] = Param::rbx;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 64, 0, 1, 0x12, 0x13);
			Check(info, "\x48\x13\xc3", 3);
			ClearInfo(info);

			// Register - Memory16

			// adc al, [bp + si + 0xaa]		-- Fail

			Assert::ExpectException<char const*>([]() {
				AnalyzeInfo info = { 0 };
				info.params[0] = Param::al;
				info.params[1] = Param::m_bp_si_disp8;
				info.numParams = 2;
				info.Disp.disp8 = 0xaa;
				IntelAssembler::HandleModRegRm(info, 64, 0, 1, 0x12, 0x13);
				});
			ClearInfo(info);

			// Register - Memory32

			// adc al, [ebx]		"\x67\x12\x03"

			info.params[0] = Param::al;
			info.params[1] = Param::m_ebx;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 64, 0, 1, 0x12, 0x13);
			Check(info, "\x67\x12\x03", 3);
			ClearInfo(info);

			// adc ax, [ebx]		"\x66\x67\x13\x03"

			info.params[0] = Param::ax;
			info.params[1] = Param::m_ebx;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 64, 0, 1, 0x12, 0x13);
			Check(info, "\x66\x67\x13\x03", 4);
			ClearInfo(info);

			// adc eax, [ebx]		"\x67\x13\x03"

			info.params[0] = Param::eax;
			info.params[1] = Param::m_ebx;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 64, 0, 1, 0x12, 0x13);
			Check(info, "\x67\x13\x03", 3);
			ClearInfo(info);

			// adc r15, [ebx]		"\x67\x4c\x13\x3b"

			info.params[0] = Param::r15;
			info.params[1] = Param::m_ebx;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 64, 0, 1, 0x12, 0x13);
			Check(info, "\x67\x4c\x13\x3b", 4);
			ClearInfo(info);

			// adc r15d, [ebx]		"\x67\x44\x13\x3b"

			info.params[0] = Param::r15d;
			info.params[1] = Param::m_ebx;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 64, 0, 1, 0x12, 0x13);
			Check(info, "\x67\x44\x13\x3b", 4);
			ClearInfo(info);

			// adc sil, [ebx]		"\x67\x40\x12\x33"

			info.params[0] = Param::sil;
			info.params[1] = Param::m_ebx;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 64, 0, 1, 0x12, 0x13);
			Check(info, "\x67\x40\x12\x33", 4);
			ClearInfo(info);

			// adc r10w, [ebx]		"\x66\x67\x44\x13\x13"

			info.params[0] = Param::r10w;
			info.params[1] = Param::m_ebx;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 64, 0, 1, 0x12, 0x13);
			Check(info, "\x66\x67\x44\x13\x13", 5);
			ClearInfo(info);

			// Register - Memory32 + SIB

			// adc al, [ebp * 2 + 0x11223344]	"\x67\x12\x04\x6d\x44\x33\x22\x11"

			info.params[0] = Param::al;
			info.params[1] = Param::sib_ebp_2_disp32;
			info.numParams = 2;
			info.Disp.disp32 = 0x11223344;
			IntelAssembler::HandleModRegRm(info, 64, 0, 1, 0x12, 0x13);
			Check(info, "\x67\x12\x04\x6d\x44\x33\x22\x11", 8);
			ClearInfo(info);

			// adc ax, [esi * 2 + edx]		"\x66\x67\x13\x04\x72"

			info.params[0] = Param::ax;
			info.params[1] = Param::sib_esi_2_edx;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 64, 0, 1, 0x12, 0x13);
			Check(info, "\x66\x67\x13\x04\x72", 5);
			ClearInfo(info);

			// adc eax, [esi * 2 + esi]			"\x67\x13\x04\x76"

			info.params[0] = Param::eax;
			info.params[1] = Param::sib_esi_2_esi;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 64, 0, 1, 0x12, 0x13);
			Check(info, "\x67\x13\x04\x76", 4);
			ClearInfo(info);

			// adc eax, [r15d * 2 + r10d]		"\x67\x43\x13\x04\x7a"

			info.params[0] = Param::eax;
			info.params[1] = Param::sib_r15d_2_r10d;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 64, 0, 1, 0x12, 0x13);
			Check(info, "\x67\x43\x13\x04\x7a", 5);
			ClearInfo(info);

			// Register - Memory64

			// adc al, [rax]			"\x12\x00"

			info.params[0] = Param::al;
			info.params[1] = Param::m_rax;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 64, 0, 1, 0x12, 0x13);
			Check(info, "\x12\x00", 2);
			ClearInfo(info);

			// adc rax, [rip + 0x11223344]		"\x48\x13\x05\x44\x33\x22\x11"

			info.params[0] = Param::rax;
			info.params[1] = Param::m_rip_disp32;
			info.numParams = 2;
			info.Disp.disp32 = 0x11223344;
			IntelAssembler::HandleModRegRm(info, 64, 0, 1, 0x12, 0x13);
			Check(info, "\x48\x13\x05\x44\x33\x22\x11", 7);
			ClearInfo(info);

			// adc rax, [eip + 0x11223344]		"\x67\x48\x13\x05\x44\x33\x22\x11"

			info.params[0] = Param::rax;
			info.params[1] = Param::m_eip_disp32;
			info.numParams = 2;
			info.Disp.disp32 = 0x11223344;
			IntelAssembler::HandleModRegRm(info, 64, 0, 1, 0x12, 0x13);
			Check(info, "\x67\x48\x13\x05\x44\x33\x22\x11", 8);
			ClearInfo(info);

			// Register - Memory64 + SIB

			// adc eax, [rax * 2 + rax]			"\x13\x04\x40"

			info.params[0] = Param::eax;
			info.params[1] = Param::sib_rax_2_rax;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 64, 0, 1, 0x12, 0x13);
			Check(info, "\x13\x04\x40", 3);
			ClearInfo(info);

			// adc spl, [r12 * 2 + rax]			"\x42\x12\x24\x60"

			info.params[0] = Param::spl;
			info.params[1] = Param::sib_r12_2_rax;
			info.numParams = 2;
			IntelAssembler::HandleModRegRm(info, 64, 0, 1, 0x12, 0x13);
			Check(info, "\x42\x12\x24\x60", 4);
			ClearInfo(info);
		}

		/// <summary>
		/// Instructions Form_MI testing that use ModRM and imm8/imm16/imm32/simm8 (16-bit)
		/// </summary>
		TEST_METHOD(ModRmImm_16)
		{
			AnalyzeInfo info = { 0 };

			// Register - Imm

			// adc cl, 0xaa			"\x80\xd1\xaa"

			info.params[0] = Param::cl;
			info.params[1] = Param::imm8;
			info.numParams = 2;
			info.Imm.uimm8 = 0xaa;
			IntelAssembler::HandleModRmImm(info, 16, 0x80, 0x81, 0x83, 2);
			Check(info, "\x80\xd1\xaa", 3);
			ClearInfo(info);

			// adc cx, 0x1234	"\x81\xd1\x34\x12"

			info.params[0] = Param::cx;
			info.params[1] = Param::imm16;
			info.numParams = 2;
			info.Imm.uimm16 = 0x1234;
			IntelAssembler::HandleModRmImm(info, 16, 0x80, 0x81, 0x83, 2);
			Check(info, "\x81\xd1\x34\x12", 4);
			ClearInfo(info);

			// adc ecx, 0x12345678	"\x66\x81\xd1\x78\x56\x34\x12"

			info.params[0] = Param::ecx;
			info.params[1] = Param::imm32;
			info.numParams = 2;
			info.Imm.uimm32 = 0x12345678;
			IntelAssembler::HandleModRmImm(info, 16, 0x80, 0x81, 0x83, 2);
			Check(info, "\x66\x81\xd1\x78\x56\x34\x12", 7);
			ClearInfo(info);

			// adc rcx, 0x12345678	 -- Failed

			Assert::ExpectException<char const*>([]() {
				AnalyzeInfo info = { 0 };
				info.params[0] = Param::rcx;
				info.params[1] = Param::imm32;
				info.numParams = 2;
				info.Imm.uimm32 = 0x12345678;
				IntelAssembler::HandleModRmImm(info, 16, 0x80, 0x81, 0x83, 2);
				});
			ClearInfo(info);

			// adc cx, (signed)0xaa		"\x83\xd1\xaa"

			info.params[0] = Param::cx;
			info.params[1] = Param::simm8_as16;
			info.numParams = 2;
			info.Imm.simm8 = -0x56;
			IntelAssembler::HandleModRmImm(info, 16, 0x80, 0x81, 0x83, 2);
			Check(info, "\x83\xd1\xaa", 3);
			ClearInfo(info);

			// adc ecx, (signed)0xaa	"\x66\x83\xd1\xaa"

			info.params[0] = Param::ecx;
			info.params[1] = Param::simm8_as32;
			info.numParams = 2;
			info.Imm.simm8 = -0x56;
			IntelAssembler::HandleModRmImm(info, 16, 0x80, 0x81, 0x83, 2);
			Check(info, "\x66\x83\xd1\xaa", 4);
			ClearInfo(info);

			// adc rcx, (signed)0xaa  -- Failed

			Assert::ExpectException<char const*>([]() {
				AnalyzeInfo info = { 0 };
				info.params[0] = Param::rcx;
				info.params[1] = Param::simm8_as64;
				info.numParams = 2;
				info.Imm.simm8 = -0x56;
				IntelAssembler::HandleModRmImm(info, 16, 0x80, 0x81, 0x83, 2);
				});
			ClearInfo(info);

			// Memory16 - Imm

			// adc byte ptr [bx + si], 0xaa		"\x80\x10\xaa"

			info.params[0] = Param::m_bx_si;
			info.params[1] = Param::imm8;
			info.numParams = 2;
			info.Imm.uimm8 = 0xaa;
			IntelAssembler::HandleModRmImm(info, 16, 0x80, 0x81, 0x83, 2);
			Check(info, "\x80\x10\xaa", 3);
			ClearInfo(info);

			// adc word ptr [bx + si], 0x1234	"\x81\x10\x34\x12"

			info.params[0] = Param::m_bx_si;
			info.params[1] = Param::imm16;
			info.numParams = 2;
			info.Imm.uimm16 = 0x1234;
			IntelAssembler::HandleModRmImm(info, 16, 0x80, 0x81, 0x83, 2);
			Check(info, "\x81\x10\x34\x12", 4);
			ClearInfo(info);

			// adc dword ptr [bx + si], 0x12345678	"\x66\x81\x10\x78\x56\x34\x12"

			info.params[0] = Param::m_bx_si;
			info.params[1] = Param::imm32;
			info.numParams = 2;
			info.Imm.uimm32 = 0x12345678;
			IntelAssembler::HandleModRmImm(info, 16, 0x80, 0x81, 0x83, 2);
			Check(info, "\x66\x81\x10\x78\x56\x34\x12", 7);
			ClearInfo(info);

			// adc word ptr [bx + si], (signed)0xaa			"\x83\x10\xaa"

			info.params[0] = Param::m_bx_si;
			info.params[1] = Param::simm8_as16;
			info.numParams = 2;
			info.Imm.simm8 = -0x56;
			IntelAssembler::HandleModRmImm(info, 16, 0x80, 0x81, 0x83, 2);
			Check(info, "\x83\x10\xaa", 3);
			ClearInfo(info);

			// adc dword ptr [bx + si], (signed)0xaa		"\x66\x83\x10\xaa"

			info.params[0] = Param::m_bx_si;
			info.params[1] = Param::simm8_as32;
			info.numParams = 2;
			info.Imm.simm8 = -0x56;
			IntelAssembler::HandleModRmImm(info, 16, 0x80, 0x81, 0x83, 2);
			Check(info, "\x66\x83\x10\xaa", 4);
			ClearInfo(info);

			// Memory32 - Imm

			// adc byte ptr [eax], 0xaa			"\x67\x80\x10\xaa"

			info.params[0] = Param::m_eax;
			info.params[1] = Param::imm8;
			info.numParams = 2;
			info.Imm.uimm8 = 0xaa;
			IntelAssembler::HandleModRmImm(info, 16, 0x80, 0x81, 0x83, 2);
			Check(info, "\x67\x80\x10\xaa", 4);
			ClearInfo(info);

			// adc word ptr [eax], 0x1234		"\x67\x81\x10\x34\x12"

			info.params[0] = Param::m_eax;
			info.params[1] = Param::imm16;
			info.numParams = 2;
			info.Imm.uimm16 = 0x1234;
			IntelAssembler::HandleModRmImm(info, 16, 0x80, 0x81, 0x83, 2);
			Check(info, "\x67\x81\x10\x34\x12", 5);
			ClearInfo(info);

			// adc dword ptr [eax], 0x12345678	"\x66\x67\x81\x10\x78\x56\x34\x12"

			info.params[0] = Param::m_eax;
			info.params[1] = Param::imm32;
			info.numParams = 2;
			info.Imm.uimm32 = 0x12345678;
			IntelAssembler::HandleModRmImm(info, 16, 0x80, 0x81, 0x83, 2);
			Check(info, "\x66\x67\x81\x10\x78\x56\x34\x12", 8);
			ClearInfo(info);

			// adc word ptr [eax], (signed)0xaa		"\x67\x83\x10\xaa"

			info.params[0] = Param::m_eax;
			info.params[1] = Param::simm8_as16;
			info.numParams = 2;
			info.Imm.simm8 = -0x56;
			IntelAssembler::HandleModRmImm(info, 16, 0x80, 0x81, 0x83, 2);
			Check(info, "\x67\x83\x10\xaa", 4);
			ClearInfo(info);

			// adc dword ptr [eax], (signed)0xaa	"\x66\x67\x83\x10\xaa"

			info.params[0] = Param::m_eax;
			info.params[1] = Param::simm8_as32;
			info.numParams = 2;
			info.Imm.simm8 = -0x56;
			IntelAssembler::HandleModRmImm(info, 16, 0x80, 0x81, 0x83, 2);
			Check(info, "\x66\x67\x83\x10\xaa", 5);
			ClearInfo(info);

			// Memory32 + SIM - Imm

			// adc byte ptr [eax * 2 + ecx], 0xaa		"\x67\x80\x14\x41\xaa"

			info.params[0] = Param::sib_eax_2_ecx;
			info.params[1] = Param::imm8;
			info.numParams = 2;
			info.Imm.uimm8 = 0xaa;
			IntelAssembler::HandleModRmImm(info, 16, 0x80, 0x81, 0x83, 2);
			Check(info, "\x67\x80\x14\x41\xaa", 5);
			ClearInfo(info);

			// adc word ptr [eax * 2 + ecx], 0x1234			"\x67\x81\x14\x41\x34\x12"

			info.params[0] = Param::sib_eax_2_ecx;
			info.params[1] = Param::imm16;
			info.numParams = 2;
			info.Imm.uimm16 = 0x1234;
			IntelAssembler::HandleModRmImm(info, 16, 0x80, 0x81, 0x83, 2);
			Check(info, "\x67\x81\x14\x41\x34\x12", 6);
			ClearInfo(info);

			// adc dword ptr [eax * 2 + ecx], 0x12345678		"\x66\x67\x81\x14\x41\x78\x56\x34\x12"

			info.params[0] = Param::sib_eax_2_ecx;
			info.params[1] = Param::imm32;
			info.numParams = 2;
			info.Imm.uimm32 = 0x12345678;
			IntelAssembler::HandleModRmImm(info, 16, 0x80, 0x81, 0x83, 2);
			Check(info, "\x66\x67\x81\x14\x41\x78\x56\x34\x12", 9);
			ClearInfo(info);

			// adc word ptr [eax * 2 + ecx], (signed)0xaa	"\x67\x83\x14\x41\xaa"

			info.params[0] = Param::sib_eax_2_ecx;
			info.params[1] = Param::simm8_as16;
			info.numParams = 2;
			info.Imm.simm8 = -0x56;
			IntelAssembler::HandleModRmImm(info, 16, 0x80, 0x81, 0x83, 2);
			Check(info, "\x67\x83\x14\x41\xaa", 5);
			ClearInfo(info);

			// adc dword ptr [eax * 2 + ecx], (signed)0xaa	"\x66\x67\x83\x14\x41\xaa"

			info.params[0] = Param::sib_eax_2_ecx;
			info.params[1] = Param::simm8_as32;
			info.numParams = 2;
			info.Imm.simm8 = -0x56;
			IntelAssembler::HandleModRmImm(info, 16, 0x80, 0x81, 0x83, 2);
			Check(info, "\x66\x67\x83\x14\x41\xaa", 6);
			ClearInfo(info);

			// Memory64 - Imm

			// adc byte ptr [rax], 0xaa -- Failed

			Assert::ExpectException<char const*>([]() {
				AnalyzeInfo info = { 0 };
				info.params[0] = Param::m_rax;
				info.params[1] = Param::simm8_as64;
				info.numParams = 2;
				info.Imm.simm8 = -0x56;
				IntelAssembler::HandleModRmImm(info, 16, 0x80, 0x81, 0x83, 2);
				});
			ClearInfo(info);

		}

		/// <summary>
		/// Instructions Form_MI testing that use ModRM and imm8/imm16/imm32/simm8 (32-bit)
		/// </summary>
		TEST_METHOD(ModRmImm_32)
		{
			AnalyzeInfo info = { 0 };

			// Register - Imm

			// adc cl, 0xaa		"\x80\xd1\xaa"

			info.params[0] = Param::cl;
			info.params[1] = Param::imm8;
			info.numParams = 2;
			info.Imm.uimm8 = 0xaa;
			IntelAssembler::HandleModRmImm(info, 32, 0x80, 0x81, 0x83, 2);
			Check(info, "\x80\xd1\xaa", 3);
			ClearInfo(info);

			// adc cx, 0x1234		"\x66\x81\xd1\x34\x12"

			info.params[0] = Param::cx;
			info.params[1] = Param::imm16;
			info.numParams = 2;
			info.Imm.uimm16 = 0x1234;
			IntelAssembler::HandleModRmImm(info, 32, 0x80, 0x81, 0x83, 2);
			Check(info, "\x66\x81\xd1\x34\x12", 5);
			ClearInfo(info);

			// adc ecx, 0x12345678	"\x81\xd1\x78\x56\x34\x12"

			info.params[0] = Param::ecx;
			info.params[1] = Param::imm32;
			info.numParams = 2;
			info.Imm.uimm32 = 0x12345678;
			IntelAssembler::HandleModRmImm(info, 32, 0x80, 0x81, 0x83, 2);
			Check(info, "\x81\xd1\x78\x56\x34\x12", 6);
			ClearInfo(info);

			// adc rcx, 0x12345678	 -- Failed

			Assert::ExpectException<char const*>([]() {
				AnalyzeInfo info = { 0 };
				info.params[0] = Param::rcx;
				info.params[1] = Param::imm32;
				info.numParams = 2;
				info.Imm.uimm32 = 0x12345678;
				IntelAssembler::HandleModRmImm(info, 32, 0x80, 0x81, 0x83, 2);
				});
			ClearInfo(info);

			// adc cx, (signed)0xaa		"\x66\x83\xd1\xaa"

			info.params[0] = Param::cx;
			info.params[1] = Param::simm8_as16;
			info.numParams = 2;
			info.Imm.simm8 = -0x56;
			IntelAssembler::HandleModRmImm(info, 32, 0x80, 0x81, 0x83, 2);
			Check(info, "\x66\x83\xd1\xaa", 4);
			ClearInfo(info);

			// adc ecx, (signed)0xaa	"\x83\xd1\xaa"

			info.params[0] = Param::ecx;
			info.params[1] = Param::simm8_as32;
			info.numParams = 2;
			info.Imm.simm8 = -0x56;
			IntelAssembler::HandleModRmImm(info, 32, 0x80, 0x81, 0x83, 2);
			Check(info, "\x83\xd1\xaa", 3);
			ClearInfo(info);

			// adc rcx, (signed)0xaa  -- Failed

			Assert::ExpectException<char const*>([]() {
				AnalyzeInfo info = { 0 };
				info.params[0] = Param::rcx;
				info.params[1] = Param::simm8_as64;
				info.numParams = 2;
				info.Imm.simm8 = -0x56;
				IntelAssembler::HandleModRmImm(info, 32, 0x80, 0x81, 0x83, 2);
				});
			ClearInfo(info);

			// Memory16 - Imm

			// adc byte ptr [bx + si], 0xaa		"\x67\x80\x10\xaa"

			info.params[0] = Param::m_bx_si;
			info.params[1] = Param::imm8;
			info.numParams = 2;
			info.Imm.uimm8 = 0xaa;
			IntelAssembler::HandleModRmImm(info, 32, 0x80, 0x81, 0x83, 2);
			Check(info, "\x67\x80\x10\xaa", 4);
			ClearInfo(info);

			// adc word ptr [bx + si], 0x1234	 "\x66\x67\x81\x10\x34\x12"

			info.params[0] = Param::m_bx_si;
			info.params[1] = Param::imm16;
			info.numParams = 2;
			info.Imm.uimm16 = 0x1234;
			IntelAssembler::HandleModRmImm(info, 32, 0x80, 0x81, 0x83, 2);
			Check(info, "\x66\x67\x81\x10\x34\x12", 6);
			ClearInfo(info);

			// adc dword ptr [bx + si], 0x12345678	"\x67\x81\x10\x78\x56\x34\x12"

			info.params[0] = Param::m_bx_si;
			info.params[1] = Param::imm32;
			info.numParams = 2;
			info.Imm.uimm32 = 0x12345678;
			IntelAssembler::HandleModRmImm(info, 32, 0x80, 0x81, 0x83, 2);
			Check(info, "\x67\x81\x10\x78\x56\x34\x12", 7);
			ClearInfo(info);

			// adc word ptr [bx + si], (signed)0xaa		"\x66\x67\x83\x10\xaa"

			info.params[0] = Param::m_bx_si;
			info.params[1] = Param::simm8_as16;
			info.numParams = 2;
			info.Imm.simm8 = -0x56;
			IntelAssembler::HandleModRmImm(info, 32, 0x80, 0x81, 0x83, 2);
			Check(info, "\x66\x67\x83\x10\xaa", 5);
			ClearInfo(info);

			// adc dword ptr [bx + si], (signed)0xaa	"\x67\x83\x10\xaa"

			info.params[0] = Param::m_bx_si;
			info.params[1] = Param::simm8_as32;
			info.numParams = 2;
			info.Imm.simm8 = -0x56;
			IntelAssembler::HandleModRmImm(info, 32, 0x80, 0x81, 0x83, 2);
			Check(info, "\x67\x83\x10\xaa", 4);
			ClearInfo(info);

			// Memory32 - Imm

			// adc byte ptr [eax], 0xaa		"\x80\x10\xaa"

			info.params[0] = Param::m_eax;
			info.params[1] = Param::imm8;
			info.numParams = 2;
			info.Imm.uimm8 = 0xaa;
			IntelAssembler::HandleModRmImm(info, 32, 0x80, 0x81, 0x83, 2);
			Check(info, "\x80\x10\xaa", 3);
			ClearInfo(info);

			// adc word ptr [eax], 0x1234	"\x66\x81\x10\x34\x12"

			info.params[0] = Param::m_eax;
			info.params[1] = Param::imm16;
			info.numParams = 2;
			info.Imm.uimm16 = 0x1234;
			IntelAssembler::HandleModRmImm(info, 32, 0x80, 0x81, 0x83, 2);
			Check(info, "\x66\x81\x10\x34\x12", 5);
			ClearInfo(info);

			// adc dword ptr [eax], 0x12345678	"\x81\x10\x78\x56\x34\x12"

			info.params[0] = Param::m_eax;
			info.params[1] = Param::imm32;
			info.numParams = 2;
			info.Imm.uimm32 = 0x12345678;
			IntelAssembler::HandleModRmImm(info, 32, 0x80, 0x81, 0x83, 2);
			Check(info, "\x81\x10\x78\x56\x34\x12", 6);
			ClearInfo(info);

			// adc word ptr [eax], (signed)0xaa		"\x66\x83\x10\xaa"

			info.params[0] = Param::m_eax;
			info.params[1] = Param::simm8_as16;
			info.numParams = 2;
			info.Imm.simm8 = -0x56;
			IntelAssembler::HandleModRmImm(info, 32, 0x80, 0x81, 0x83, 2);
			Check(info, "\x66\x83\x10\xaa", 4);
			ClearInfo(info);

			// adc dword ptr [eax], (signed)0xaa	"\x83\x10\xaa"

			info.params[0] = Param::m_eax;
			info.params[1] = Param::simm8_as32;
			info.numParams = 2;
			info.Imm.simm8 = -0x56;
			IntelAssembler::HandleModRmImm(info, 32, 0x80, 0x81, 0x83, 2);
			Check(info, "\x83\x10\xaa", 3);
			ClearInfo(info);

			// Memory32 + SIM - Imm

			// adc byte ptr [eax * 2 + ecx], 0xaa	"\x80\x14\x41\xaa"

			info.params[0] = Param::sib_eax_2_ecx;
			info.params[1] = Param::imm8;
			info.numParams = 2;
			info.Imm.uimm8 = 0xaa;
			IntelAssembler::HandleModRmImm(info, 32, 0x80, 0x81, 0x83, 2);
			Check(info, "\x80\x14\x41\xaa", 4);
			ClearInfo(info);

			// adc word ptr [eax * 2 + ecx], 0x1234		"\x66\x81\x14\x41\x34\x12"

			info.params[0] = Param::sib_eax_2_ecx;
			info.params[1] = Param::imm16;
			info.numParams = 2;
			info.Imm.uimm16 = 0x1234;
			IntelAssembler::HandleModRmImm(info, 32, 0x80, 0x81, 0x83, 2);
			Check(info, "\x66\x81\x14\x41\x34\x12", 6);
			ClearInfo(info);

			// adc dword ptr [eax * 2 + ecx], 0x12345678	"\x81\x14\x41\x78\x56\x34\x12"

			info.params[0] = Param::sib_eax_2_ecx;
			info.params[1] = Param::imm32;
			info.numParams = 2;
			info.Imm.uimm32 = 0x12345678;
			IntelAssembler::HandleModRmImm(info, 32, 0x80, 0x81, 0x83, 2);
			Check(info, "\x81\x14\x41\x78\x56\x34\x12", 7);
			ClearInfo(info);

			// adc word ptr [eax * 2 + ecx], (signed)0xaa	"\x66\x83\x14\x41\xaa"

			info.params[0] = Param::sib_eax_2_ecx;
			info.params[1] = Param::simm8_as16;
			info.numParams = 2;
			info.Imm.simm8 = -0x56;
			IntelAssembler::HandleModRmImm(info, 32, 0x80, 0x81, 0x83, 2);
			Check(info, "\x66\x83\x14\x41\xaa", 5);
			ClearInfo(info);

			// adc dword ptr [eax * 2 + ecx], (signed)0xaa	"\x83\x14\x41\xaa"

			info.params[0] = Param::sib_eax_2_ecx;
			info.params[1] = Param::simm8_as32;
			info.numParams = 2;
			info.Imm.simm8 = -0x56;
			IntelAssembler::HandleModRmImm(info, 32, 0x80, 0x81, 0x83, 2);
			Check(info, "\x83\x14\x41\xaa", 4);
			ClearInfo(info);

			// Memory64 - Imm

			// adc byte ptr [rax], 0xaa -- Failed

			Assert::ExpectException<char const*>([]() {
				AnalyzeInfo info = { 0 };
				info.params[0] = Param::m_rax;
				info.params[1] = Param::simm8_as64;
				info.numParams = 2;
				info.Imm.simm8 = -0x56;
				IntelAssembler::HandleModRmImm(info, 32, 0x80, 0x81, 0x83, 2);
				});
			ClearInfo(info);
		}

		/// <summary>
		/// Instructions Form_MI testing that use ModRM and imm8/imm16/imm32/simm8 (64-bit)
		/// </summary>
		TEST_METHOD(ModRmImm_64)
		{
			AnalyzeInfo info = { 0 };

			// Register - Imm

			// adc cl, 0xaa			"\x80\xd1\xaa"

			info.params[0] = Param::cl;
			info.params[1] = Param::imm8;
			info.numParams = 2;
			info.Imm.uimm8 = 0xaa;
			IntelAssembler::HandleModRmImm(info, 64, 0x80, 0x81, 0x83, 2);
			Check(info, "\x80\xd1\xaa", 3);
			ClearInfo(info);

			// adc cx, 0x1234		"\x66\x81\xd1\x34\x12"

			info.params[0] = Param::cx;
			info.params[1] = Param::imm16;
			info.numParams = 2;
			info.Imm.uimm16 = 0x1234;
			IntelAssembler::HandleModRmImm(info, 64, 0x80, 0x81, 0x83, 2);
			Check(info, "\x66\x81\xd1\x34\x12", 5);
			ClearInfo(info);

			// adc ecx, 0x12345678	"\x81\xd1\x78\x56\x34\x12"

			info.params[0] = Param::ecx;
			info.params[1] = Param::imm32;
			info.numParams = 2;
			info.Imm.uimm32 = 0x12345678;
			IntelAssembler::HandleModRmImm(info, 64, 0x80, 0x81, 0x83, 2);
			Check(info, "\x81\xd1\x78\x56\x34\x12", 6);
			ClearInfo(info);

			// adc rcx, 0x12345678		"\x48\x81\xd1\x78\x56\x34\x12"

			info.params[0] = Param::rcx;
			info.params[1] = Param::imm32;
			info.numParams = 2;
			info.Imm.uimm32 = 0x12345678;
			IntelAssembler::HandleModRmImm(info, 64, 0x80, 0x81, 0x83, 2);
			Check(info, "\x48\x81\xd1\x78\x56\x34\x12", 7);
			ClearInfo(info);

			// adc cx, (signed)0xaa		"\x66\x83\xd1\xaa"

			info.params[0] = Param::cx;
			info.params[1] = Param::simm8_as16;
			info.numParams = 2;
			info.Imm.simm8 = -0x56;
			IntelAssembler::HandleModRmImm(info, 64, 0x80, 0x81, 0x83, 2);
			Check(info, "\x66\x83\xd1\xaa", 4);
			ClearInfo(info);

			// adc ecx, (signed)0xaa	"\x83\xd1\xaa"

			info.params[0] = Param::ecx;
			info.params[1] = Param::simm8_as32;
			info.numParams = 2;
			info.Imm.simm8 = -0x56;
			IntelAssembler::HandleModRmImm(info, 64, 0x80, 0x81, 0x83, 2);
			Check(info, "\x83\xd1\xaa", 3);
			ClearInfo(info);

			// adc rcx, (signed)0xaa	"\x48\x83\xd1\xaa"

			info.params[0] = Param::rcx;
			info.params[1] = Param::simm8_as64;
			info.numParams = 2;
			info.Imm.simm8 = -0x56;
			IntelAssembler::HandleModRmImm(info, 64, 0x80, 0x81, 0x83, 2);
			Check(info, "\x48\x83\xd1\xaa", 4);
			ClearInfo(info);

			// adc sil, 0xaa		"\x40\x80\xd6\xaa"

			info.params[0] = Param::sil;
			info.params[1] = Param::imm8;
			info.numParams = 2;
			info.Imm.uimm8 = 0xaa;
			IntelAssembler::HandleModRmImm(info, 64, 0x80, 0x81, 0x83, 2);
			Check(info, "\x40\x80\xd6\xaa", 4);
			ClearInfo(info);

			// adc r11w, 0x1234		"\x66\x41\x81\xd3\x34\x12"

			info.params[0] = Param::r11w;
			info.params[1] = Param::imm16;
			info.numParams = 2;
			info.Imm.uimm16 = 0x1234;
			IntelAssembler::HandleModRmImm(info, 64, 0x80, 0x81, 0x83, 2);
			Check(info, "\x66\x41\x81\xd3\x34\x12", 6);
			ClearInfo(info);

			// adc r14d, 0x12345678		"\x41\x81\xd6\x78\x56\x34\x12"

			info.params[0] = Param::r14d;
			info.params[1] = Param::imm32;
			info.numParams = 2;
			info.Imm.uimm32 = 0x12345678;
			IntelAssembler::HandleModRmImm(info, 64, 0x80, 0x81, 0x83, 2);
			Check(info, "\x41\x81\xd6\x78\x56\x34\x12", 7);
			ClearInfo(info);

			// Memory16 - Imm

			// adc byte ptr [bx + si], 0xaa	  -- Failed

			Assert::ExpectException<char const*>([]() {
				AnalyzeInfo info = { 0 };
				info.params[0] = Param::m_bx_si;
				info.params[1] = Param::imm8;
				info.numParams = 2;
				info.Imm.uimm8 = 0xaa;
				IntelAssembler::HandleModRmImm(info, 64, 0x80, 0x81, 0x83, 2);
				});
			ClearInfo(info);

			// Memory32 - Imm

			// adc byte ptr [eax], 0xaa		"\x67\x80\x10\xaa"

			info.params[0] = Param::m_eax;
			info.params[1] = Param::imm8;
			info.numParams = 2;
			info.Imm.uimm8 = 0xaa;
			IntelAssembler::HandleModRmImm(info, 64, 0x80, 0x81, 0x83, 2);
			Check(info, "\x67\x80\x10\xaa", 4);
			ClearInfo(info);

			// adc word ptr [eax], 0x1234		"\x66\x67\x81\x10\x34\x12"

			info.params[0] = Param::m_eax;
			info.params[1] = Param::imm16;
			info.numParams = 2;
			info.Imm.uimm16 = 0x1234;
			IntelAssembler::HandleModRmImm(info, 64, 0x80, 0x81, 0x83, 2);
			Check(info, "\x66\x67\x81\x10\x34\x12", 6);
			ClearInfo(info);

			// adc dword ptr [eax], 0x12345678	"\x67\x81\x10\x78\x56\x34\x12"

			info.params[0] = Param::m_eax;
			info.params[1] = Param::imm32;
			info.numParams = 2;
			info.Imm.uimm32 = 0x12345678;
			IntelAssembler::HandleModRmImm(info, 64, 0x80, 0x81, 0x83, 2);
			Check(info, "\x67\x81\x10\x78\x56\x34\x12", 7);
			ClearInfo(info);

			// adc word ptr [eax], (signed)0xaa		"\x66\x67\x83\x10\xaa"

			info.params[0] = Param::m_eax;
			info.params[1] = Param::simm8_as16;
			info.numParams = 2;
			info.Imm.simm8 = -0x56;
			IntelAssembler::HandleModRmImm(info, 64, 0x80, 0x81, 0x83, 2);
			Check(info, "\x66\x67\x83\x10\xaa", 5);
			ClearInfo(info);

			// adc dword ptr [eax], (signed)0xaa	"\x67\x83\x10\xaa"

			info.params[0] = Param::m_eax;
			info.params[1] = Param::simm8_as32;
			info.numParams = 2;
			info.Imm.simm8 = -0x56;
			IntelAssembler::HandleModRmImm(info, 64, 0x80, 0x81, 0x83, 2);
			Check(info, "\x67\x83\x10\xaa", 4);
			ClearInfo(info);

			// Memory64 - Imm

			// adc byte ptr [rax], 0xaa			"\x80\x10\xaa"

			info.params[0] = Param::m_rax;
			info.params[1] = Param::imm8;
			info.numParams = 2;
			info.Imm.uimm8 = 0xaa;
			IntelAssembler::HandleModRmImm(info, 64, 0x80, 0x81, 0x83, 2);
			Check(info, "\x80\x10\xaa", 3);
			ClearInfo(info);

			// Memory64 + SIB - Imm
			
			// adc byte ptr [r15 * 2 + r14], 0xaa	"\x43\x80\x14\x7e\xaa"

			info.params[0] = Param::sib_r15_2_r14;
			info.params[1] = Param::imm8;
			info.numParams = 2;
			info.Imm.uimm8 = 0xaa;
			IntelAssembler::HandleModRmImm(info, 64, 0x80, 0x81, 0x83, 2);
			Check(info, "\x43\x80\x14\x7e\xaa", 5);
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
			Check(IntelAssembler::adc<16>(Param::cl, Param::imm8, 0, 0xaa), "\x80\xd1\xaa", 3);
			Check(IntelAssembler::adc<16>(Param::cx, Param::imm16, 0, 0x1234), "\x81\xd1\x34\x12", 4);
			Check(IntelAssembler::adc<32>(Param::ecx, Param::imm32, 0, 0x12345678), "\x81\xd1\x78\x56\x34\x12", 6);
			Check(IntelAssembler::adc<64>(Param::rcx, Param::imm32, 0, 0x12345678), "\x48\x81\xd1\x78\x56\x34\x12", 7);
			Check(IntelAssembler::adc<16>(Param::cx, Param::simm8_as16, 0, -0x56), "\x83\xd1\xaa", 3);
			Check(IntelAssembler::adc<32>(Param::ecx, Param::simm8_as32, 0, -0x56), "\x83\xd1\xaa", 3);
			Check(IntelAssembler::adc<64>(Param::rcx, Param::simm8_as64, 0, -0x56), "\x48\x83\xd1\xaa", 4);

			// MR
			Check(IntelAssembler::adc<16>(Param::m_bp_di, Param::al, 0, 0), "\x10\x03", 2);
			Check(IntelAssembler::adc<64>(Param::sib_r11_4_rdx, Param::r8b, 0, 0), "\x46\x10\x04\x9a", 4);
			Check(IntelAssembler::adc<16>(Param::m_bx, Param::ax, 0, 0), "\x11\x07", 2);
			Check(IntelAssembler::adc<32>(Param::m_ebx, Param::eax, 0, 0), "\x11\x03", 2);
			Check(IntelAssembler::adc<64>(Param::m_rbx, Param::rax, 0, 0), "\x48\x11\x03", 3);
			Check(IntelAssembler::adc<64>(Param::m_eax, Param::rax, 0, 0), "\x67\x48\x11\x00", 4);

			// RM
			Check(IntelAssembler::adc<16>(Param::al, Param::m_bp_di, 0, 0), "\x12\x03", 2);
			Check(IntelAssembler::adc<64>(Param::r8b, Param::sib_r11_4_rdx, 0, 0), "\x46\x12\x04\x9a", 4);
			Check(IntelAssembler::adc<16>(Param::ax, Param::m_bx, 0, 0), "\x13\x07", 2);
			Check(IntelAssembler::adc<32>(Param::eax, Param::m_ebx, 0, 0), "\x13\x03", 2);
			Check(IntelAssembler::adc<64>(Param::rax, Param::m_rbx, 0, 0), "\x48\x13\x03", 3);
		}

		/// <summary>
		/// Tests a group of opcodes for an `add` instruction.
		/// </summary>
		TEST_METHOD(add)
		{
			// I
			Check(IntelAssembler::add<16>(Param::al, Param::imm8, 0, 0xaa), "\x04\xaa", 2);
			Check(IntelAssembler::add<16>(Param::ax, Param::imm16, 0, 0x1234), "\x05\x34\x12", 3);
			Check(IntelAssembler::add<16>(Param::eax, Param::imm32, 0, 0x12345678), "\x66\x05\x78\x56\x34\x12", 6);
			Check(IntelAssembler::add<64>(Param::rax, Param::imm32, 0, 0x12345678), "\x48\x05\x78\x56\x34\x12", 6);

			// MI
			Check(IntelAssembler::add<16>(Param::cl, Param::imm8, 0, 0xaa), "\x80\xc1\xaa", 3);
			Check(IntelAssembler::add<16>(Param::cx, Param::imm16, 0, 0x1234), "\x81\xc1\x34\x12", 4);
			Check(IntelAssembler::add<32>(Param::ecx, Param::imm32, 0, 0x12345678), "\x81\xc1\x78\x56\x34\x12", 6);
			Check(IntelAssembler::add<64>(Param::rcx, Param::imm32, 0, 0x12345678), "\x48\x81\xc1\x78\x56\x34\x12", 7);
			Check(IntelAssembler::add<16>(Param::cx, Param::simm8_as16, 0, -0x56), "\x83\xc1\xaa", 3);
			Check(IntelAssembler::add<32>(Param::ecx, Param::simm8_as32, 0, -0x56), "\x83\xc1\xaa", 3);
			Check(IntelAssembler::add<64>(Param::rcx, Param::simm8_as64, 0, -0x56), "\x48\x83\xc1\xaa", 4);

			// MR
			Check(IntelAssembler::add<16>(Param::m_bp_di, Param::al, 0, 0), "\x00\x03", 2);
			Check(IntelAssembler::add<64>(Param::sib_r11_4_rdx, Param::r8b, 0, 0), "\x46\x00\x04\x9a", 4);
			Check(IntelAssembler::add<16>(Param::m_bx, Param::ax, 0, 0), "\x01\x07", 2);
			Check(IntelAssembler::add<32>(Param::m_ebx, Param::eax, 0, 0), "\x01\x03", 2);
			Check(IntelAssembler::add<64>(Param::m_rbx, Param::rax, 0, 0), "\x48\x01\x03", 3);
			Check(IntelAssembler::add<64>(Param::m_eax, Param::rax, 0, 0), "\x67\x48\x01\x00", 4);

			// RM
			Check(IntelAssembler::add<16>(Param::al, Param::m_bp_di, 0, 0), "\x02\x03", 2);
			Check(IntelAssembler::add<64>(Param::r8b, Param::sib_r11_4_rdx, 0, 0), "\x46\x02\x04\x9a", 4);
			Check(IntelAssembler::add<16>(Param::ax, Param::m_bx, 0, 0), "\x03\x07", 2);
			Check(IntelAssembler::add<32>(Param::eax, Param::m_ebx, 0, 0), "\x03\x03", 2);
			Check(IntelAssembler::add<64>(Param::rax, Param::m_rbx, 0, 0), "\x48\x03\x03", 3);
		}

		/// <summary>
		/// Tests a group of opcodes for an `and` instruction.
		/// </summary>
		TEST_METHOD(_and)
		{
			// I
			Check(IntelAssembler::_and<16>(Param::al, Param::imm8, 0, 0xaa), "\x24\xaa", 2);
			Check(IntelAssembler::_and<16>(Param::ax, Param::imm16, 0, 0x1234), "\x25\x34\x12", 3);
			Check(IntelAssembler::_and<16>(Param::eax, Param::imm32, 0, 0x12345678), "\x66\x25\x78\x56\x34\x12", 6);
			Check(IntelAssembler::_and<64>(Param::rax, Param::imm32, 0, 0x12345678), "\x48\x25\x78\x56\x34\x12", 6);

			// MI
			Check(IntelAssembler::_and<16>(Param::cl, Param::imm8, 0, 0xaa), "\x80\xe1\xaa", 3);
			Check(IntelAssembler::_and<16>(Param::cx, Param::imm16, 0, 0x1234), "\x81\xe1\x34\x12", 4);
			Check(IntelAssembler::_and<32>(Param::ecx, Param::imm32, 0, 0x12345678), "\x81\xe1\x78\x56\x34\x12", 6);
			Check(IntelAssembler::_and<64>(Param::rcx, Param::imm32, 0, 0x12345678), "\x48\x81\xe1\x78\x56\x34\x12", 7);
			Check(IntelAssembler::_and<16>(Param::cx, Param::simm8_as16, 0, -0x56), "\x83\xe1\xaa", 3);
			Check(IntelAssembler::_and<32>(Param::ecx, Param::simm8_as32, 0, -0x56), "\x83\xe1\xaa", 3);
			Check(IntelAssembler::_and<64>(Param::rcx, Param::simm8_as64, 0, -0x56), "\x48\x83\xe1\xaa", 4);

			// MR
			Check(IntelAssembler::_and<16>(Param::m_bp_di, Param::al, 0, 0), "\x20\x03", 2);
			Check(IntelAssembler::_and<64>(Param::sib_r11_4_rdx, Param::r8b, 0, 0), "\x46\x20\x04\x9a", 4);
			Check(IntelAssembler::_and<16>(Param::m_bx, Param::ax, 0, 0), "\x21\x07", 2);
			Check(IntelAssembler::_and<32>(Param::m_ebx, Param::eax, 0, 0), "\x21\x03", 2);
			Check(IntelAssembler::_and<64>(Param::m_rbx, Param::rax, 0, 0), "\x48\x21\x03", 3);
			Check(IntelAssembler::_and<64>(Param::m_eax, Param::rax, 0, 0), "\x67\x48\x21\x00", 4);

			// RM
			Check(IntelAssembler::_and<16>(Param::al, Param::m_bp_di, 0, 0), "\x22\x03", 2);
			Check(IntelAssembler::_and<64>(Param::r8b, Param::sib_r11_4_rdx, 0, 0), "\x46\x22\x04\x9a", 4);
			Check(IntelAssembler::_and<16>(Param::ax, Param::m_bx, 0, 0), "\x23\x07", 2);
			Check(IntelAssembler::_and<32>(Param::eax, Param::m_ebx, 0, 0), "\x23\x03", 2);
			Check(IntelAssembler::_and<64>(Param::rax, Param::m_rbx, 0, 0), "\x48\x23\x03", 3);
		}

		TEST_METHOD(arpl)
		{
			// 16-bit

			Check(IntelAssembler::arpl<16>(Param::m_bx_si, Param::ax, 0), "\x63\x00", 2);

			// 32-bit

			Check(IntelAssembler::arpl<32>(Param::m_bx_si, Param::ax, 0), "\x66\x67\x63\x00", 4);

			Assert::ExpectException<char const*>([]() {
				IntelAssembler::arpl<64>(Param::m_eax, Param::eax, 0);
				});

			// 64-bit -- Failed

			Assert::ExpectException<char const*>([]() {
				IntelAssembler::arpl<64>(Param::m_bx_si, Param::ax, 0);
				});
		}

		TEST_METHOD(bound)
		{
			// 16-bit

			Check(IntelAssembler::bound<16>(Param::ax, Param::m_bx_si, 0), "\x62\x00", 2);
			Check(IntelAssembler::bound<16>(Param::ecx, Param::m_eax, 0), "\x66\x67\x62\x08", 4);

			// 32-bit

			Check(IntelAssembler::bound<32>(Param::ax, Param::m_bx_si, 0), "\x66\x67\x62\x00", 4);
			Check(IntelAssembler::bound<32>(Param::ecx, Param::m_eax, 0), "\x62\x08", 2);

			// 64-bit -- Failed

			Assert::ExpectException<char const*>([]() {
				IntelAssembler::bound<64>(Param::ax, Param::m_bx_si, 0);
				});
		}

		TEST_METHOD(bsf)
		{
			Check(IntelAssembler::bsf<16>(Param::ax, Param::m_bx_si, 0), "\x0f\xbc\x00", 3);
			Check(IntelAssembler::bsf<32>(Param::eax, Param::m_eax, 0), "\x0f\xbc\x00", 3);
			Check(IntelAssembler::bsf<64>(Param::rax, Param::m_rax, 0), "\x48\x0f\xbc\x00", 4);
		}

		TEST_METHOD(bsr)
		{
			Check(IntelAssembler::bsr<16>(Param::ax, Param::m_bx_si, 0), "\x0f\xbd\x00", 3);
			Check(IntelAssembler::bsr<32>(Param::eax, Param::m_eax, 0), "\x0f\xbd\x00", 3);
			Check(IntelAssembler::bsr<64>(Param::rax, Param::m_rax, 0), "\x48\x0f\xbd\x00", 4);
		}

		TEST_METHOD(bt)
		{
			Check(IntelAssembler::bt<16>(Param::m_bx_si, Param::ax, 0), "\x0f\xa3\x00", 3);
			Check(IntelAssembler::bt<32>(Param::m_eax, Param::eax, 0), "\x0f\xa3\x00", 3);
			Check(IntelAssembler::bt<64>(Param::m_rax, Param::rax, 0), "\x48\x0f\xa3\x00", 4);

			Check(IntelAssembler::bt<16>(Param::m_bx_si, Param::imm8, 0, 0xaa), "\x0f\xba\x20\xaa", 4);
			Check(IntelAssembler::bt<32>(Param::m_eax, Param::imm8, 0, 0xaa), "\x0f\xba\x20\xaa", 4);
			Check(IntelAssembler::bt<64>(Param::m_rax, Param::imm8, 0, 0xaa), "\x0f\xba\x20\xaa", 4);
		}

		TEST_METHOD(btc)
		{
			Check(IntelAssembler::btc<16>(Param::m_bx_si, Param::ax, 0), "\x0f\xbb\x00", 3);
			Check(IntelAssembler::btc<32>(Param::m_eax, Param::eax, 0), "\x0f\xbb\x00", 3);
			Check(IntelAssembler::btc<64>(Param::m_rax, Param::rax, 0), "\x48\x0f\xbb\x00", 4);

			Check(IntelAssembler::btc<16>(Param::m_bx_si, Param::imm8, 0, 0xaa), "\x0f\xba\x38\xaa", 4);
			Check(IntelAssembler::btc<32>(Param::m_eax, Param::imm8, 0, 0xaa), "\x0f\xba\x38\xaa", 4);
			Check(IntelAssembler::btc<64>(Param::m_rax, Param::imm8, 0, 0xaa), "\x0f\xba\x38\xaa", 4);
		}

		TEST_METHOD(btr)
		{
			Check(IntelAssembler::btr<16>(Param::m_bx_si, Param::ax, 0), "\x0f\xb3\x00", 3);
			Check(IntelAssembler::btr<32>(Param::m_eax, Param::eax, 0), "\x0f\xb3\x00", 3);
			Check(IntelAssembler::btr<64>(Param::m_rax, Param::rax, 0), "\x48\x0f\xb3\x00", 4);

			Check(IntelAssembler::btr<16>(Param::m_bx_si, Param::imm8, 0, 0xaa), "\x0f\xba\x30\xaa", 4);
			Check(IntelAssembler::btr<32>(Param::m_eax, Param::imm8, 0, 0xaa), "\x0f\xba\x30\xaa", 4);
			Check(IntelAssembler::btr<64>(Param::m_rax, Param::imm8, 0, 0xaa), "\x0f\xba\x30\xaa", 4);
		}

		TEST_METHOD(bts)
		{
			Check(IntelAssembler::bts<16>(Param::m_bx_si, Param::ax, 0), "\x0f\xab\x00", 3);
			Check(IntelAssembler::bts<32>(Param::m_eax, Param::eax, 0), "\x0f\xab\x00", 3);
			Check(IntelAssembler::bts<64>(Param::m_rax, Param::rax, 0), "\x48\x0f\xab\x00", 4);

			Check(IntelAssembler::bts<16>(Param::m_bx_si, Param::imm8, 0, 0xaa), "\x0f\xba\x28\xaa", 4);
			Check(IntelAssembler::bts<32>(Param::m_eax, Param::imm8, 0, 0xaa), "\x0f\xba\x28\xaa", 4);
			Check(IntelAssembler::bts<64>(Param::m_rax, Param::imm8, 0, 0xaa), "\x0f\xba\x28\xaa", 4);
		}

		TEST_METHOD(call)
		{
			// Rel16, Rel32
			Check(IntelAssembler::call<16>(Param::rel16, 0x1234), "\xe8\x34\x12", 3);
			Check(IntelAssembler::call<32>(Param::rel16, 0x1234), "\x66\xe8\x34\x12", 4);
			Check(IntelAssembler::call<16>(Param::rel32, 0x12345678), "\x66\xe8\x78\x56\x34\x12", 6);
			Check(IntelAssembler::call<32>(Param::rel32, 0x12345678), "\xe8\x78\x56\x34\x12", 5);
			Assert::ExpectException<char const*>([]() {
				IntelAssembler::call<64>(Param::rel16, 0x1234);
				});
			Check(IntelAssembler::call<64>(Param::rel32, 0x12345678), "\xe8\x78\x56\x34\x12", 5);

			// M
			Check(IntelAssembler::call<16>(Param::m_bx_si), "\xff\x10", 2);
			Check(IntelAssembler::call<32>(Param::m_bx_si), "\x67\xff\x10", 3);
			Check(IntelAssembler::call<16>(Param::m_eax), "\x67\xff\x10", 3);
			Check(IntelAssembler::call<32>(Param::m_eax), "\xff\x10", 2);
			Assert::ExpectException<char const*>([]() {
				IntelAssembler::call<64>(Param::m_bx_si);
				});
			Check(IntelAssembler::call<64>(Param::m_eax), "\x67\xff\x10", 3);
			Check(IntelAssembler::call<64>(Param::m_rax), "\xff\x10", 2);
		}

		TEST_METHOD(callf)
		{
			// D
			Check(IntelAssembler::callf<16>(Param::farptr16, 0x1234, 0x1234), "\x9a\x34\x12\x34\x12", 5);
			Check(IntelAssembler::callf<32>(Param::farptr16, 0x1234, 0x1234), "\x66\x9a\x34\x12\x34\x12", 6);
			Check(IntelAssembler::callf<16>(Param::farptr32, 0x1234, 0x12345678), "\x66\x9a\x78\x56\x34\x12\x34\x12", 8);
			Check(IntelAssembler::callf<32>(Param::farptr32, 0x1234, 0x12345678), "\x9a\x78\x56\x34\x12\x34\x12", 7);

			// M
			Check(IntelAssembler::callf<16>(Param::m_bx_si), "\xff\x18", 2);
			Check(IntelAssembler::callf<32>(Param::m_bx_si), "\x67\xff\x18", 3);
			Check(IntelAssembler::callf<16>(Param::m_eax), "\x67\xff\x18", 3);
			Check(IntelAssembler::callf<32>(Param::m_eax), "\xff\x18", 2);
			Assert::ExpectException<char const*>([]() {
				IntelAssembler::callf<64>(Param::m_bx_si);
				});
			Check(IntelAssembler::callf<64>(Param::m_eax), "\x67\xff\x18", 3);
			Check(IntelAssembler::callf<64>(Param::m_rax), "\xff\x18", 2);
		}

		TEST_METHOD(cmp)
		{
			// I
			Check(IntelAssembler::cmp<16>(Param::al, Param::imm8, 0, 0xaa), "\x3C\xaa", 2);
			Check(IntelAssembler::cmp<16>(Param::ax, Param::imm16, 0, 0x1234), "\x3D\x34\x12", 3);
			Check(IntelAssembler::cmp<16>(Param::eax, Param::imm32, 0, 0x12345678), "\x66\x3D\x78\x56\x34\x12", 6);
			Check(IntelAssembler::cmp<64>(Param::rax, Param::imm32, 0, 0x12345678), "\x48\x3D\x78\x56\x34\x12", 6);

			// MI
			Check(IntelAssembler::cmp<16>(Param::cl, Param::imm8, 0, 0xaa), "\x80\xf9\xaa", 3);
			Check(IntelAssembler::cmp<16>(Param::cx, Param::imm16, 0, 0x1234), "\x81\xf9\x34\x12", 4);
			Check(IntelAssembler::cmp<32>(Param::ecx, Param::imm32, 0, 0x12345678), "\x81\xf9\x78\x56\x34\x12", 6);
			Check(IntelAssembler::cmp<64>(Param::rcx, Param::imm32, 0, 0x12345678), "\x48\x81\xf9\x78\x56\x34\x12", 7);
			Check(IntelAssembler::cmp<16>(Param::cx, Param::simm8_as16, 0, -0x56), "\x83\xf9\xaa", 3);
			Check(IntelAssembler::cmp<32>(Param::ecx, Param::simm8_as32, 0, -0x56), "\x83\xf9\xaa", 3);
			Check(IntelAssembler::cmp<64>(Param::rcx, Param::simm8_as64, 0, -0x56), "\x48\x83\xf9\xaa", 4);

			// MR
			Check(IntelAssembler::cmp<16>(Param::m_bp_di, Param::al, 0, 0), "\x38\x03", 2);
			Check(IntelAssembler::cmp<64>(Param::sib_r11_4_rdx, Param::r8b, 0, 0), "\x46\x38\x04\x9a", 4);
			Check(IntelAssembler::cmp<16>(Param::m_bx, Param::ax, 0, 0), "\x39\x07", 2);
			Check(IntelAssembler::cmp<32>(Param::m_ebx, Param::eax, 0, 0), "\x39\x03", 2);
			Check(IntelAssembler::cmp<64>(Param::m_rbx, Param::rax, 0, 0), "\x48\x39\x03", 3);
			Check(IntelAssembler::cmp<64>(Param::m_eax, Param::rax, 0, 0), "\x67\x48\x39\x00", 4);

			// RM
			Check(IntelAssembler::cmp<16>(Param::al, Param::m_bp_di, 0, 0), "\x3a\x03", 2);
			Check(IntelAssembler::cmp<64>(Param::r8b, Param::sib_r11_4_rdx, 0, 0), "\x46\x3a\x04\x9a", 4);
			Check(IntelAssembler::cmp<16>(Param::ax, Param::m_bx, 0, 0), "\x3b\x07", 2);
			Check(IntelAssembler::cmp<32>(Param::eax, Param::m_ebx, 0, 0), "\x3b\x03", 2);
			Check(IntelAssembler::cmp<64>(Param::rax, Param::m_rbx, 0, 0), "\x48\x3b\x03", 3);
		}

		TEST_METHOD(cmpxchg)
		{
			Check(IntelAssembler::cmpxchg<16>(Param::m_bp_di, Param::al, 0), "\x0f\xb0\x03", 3);
			Check(IntelAssembler::cmpxchg<64>(Param::sib_r11_4_rdx, Param::r8b, 0), "\x46\x0f\xb0\x04\x9a", 5);
			Check(IntelAssembler::cmpxchg<16>(Param::m_bx, Param::ax, 0), "\x0f\xb1\x07", 3);
			Check(IntelAssembler::cmpxchg<32>(Param::m_ebx, Param::eax, 0), "\x0f\xb1\x03", 3);
			Check(IntelAssembler::cmpxchg<64>(Param::m_rbx, Param::rax, 0), "\x48\x0f\xb1\x03", 4);
			Check(IntelAssembler::cmpxchg<64>(Param::m_eax, Param::rax, 0), "\x67\x48\x0f\xb1\x00", 5);
		}

		TEST_METHOD(dec)
		{
			// M
			Check(IntelAssembler::dec<16>(Param::m_bp_di, PtrHint::BytePtr), "\xfe\x0b", 2);
			Check(IntelAssembler::dec<16>(Param::m_bp_di, PtrHint::WordPtr), "\xff\x0b", 2);
			Check(IntelAssembler::dec<16>(Param::m_eax, PtrHint::WordPtr), "\x67\xff\x08", 3);
			Check(IntelAssembler::dec<16>(Param::m_eax, PtrHint::DwordPtr), "\x66\x67\xff\x08", 4);

			Check(IntelAssembler::dec<32>(Param::m_bp_di, PtrHint::BytePtr), "\x67\xfe\x0b", 3);
			Check(IntelAssembler::dec<32>(Param::m_bp_di, PtrHint::WordPtr), "\x66\x67\xff\x0b", 4);
			Check(IntelAssembler::dec<32>(Param::m_eax, PtrHint::WordPtr), "\x66\xff\x08", 3);
			Check(IntelAssembler::dec<32>(Param::m_eax, PtrHint::DwordPtr), "\xff\x08", 2);

			Check(IntelAssembler::dec<64>(Param::m_eax, PtrHint::BytePtr), "\x67\xfe\x08", 3);
			Check(IntelAssembler::dec<64>(Param::m_eax, PtrHint::WordPtr), "\x66\x67\xff\x08", 4);
			Check(IntelAssembler::dec<64>(Param::m_eax, PtrHint::DwordPtr), "\x67\xff\x08", 3);
			Check(IntelAssembler::dec<64>(Param::m_eax, PtrHint::QwordPtr), "\x67\x48\xff\x08", 4);
			Check(IntelAssembler::dec<64>(Param::m_rax, PtrHint::BytePtr), "\xfe\x08", 2);
			Check(IntelAssembler::dec<64>(Param::m_rax, PtrHint::WordPtr), "\x66\xff\x08", 3);
			Check(IntelAssembler::dec<64>(Param::m_rax, PtrHint::DwordPtr), "\xff\x08", 2);
			Check(IntelAssembler::dec<64>(Param::m_rax, PtrHint::QwordPtr), "\x48\xff\x08", 3);

			// O
			Check(IntelAssembler::dec<16>(Param::ax), "\x48", 1);
			Check(IntelAssembler::dec<32>(Param::ax), "\x66\x48", 2);
			Check(IntelAssembler::dec<16>(Param::eax), "\x66\x48", 2);
			Check(IntelAssembler::dec<32>(Param::eax), "\x48", 1);
			Check(IntelAssembler::dec<64>(Param::ax), "\x66\xff\xc8", 3);
			Check(IntelAssembler::dec<64>(Param::eax), "\xff\xc8", 2);
		}

		TEST_METHOD(div)
		{
			Check(IntelAssembler::div<16>(Param::m_bp_di, PtrHint::BytePtr), "\xf6\x33", 2);
			Check(IntelAssembler::div<16>(Param::m_bp_di, PtrHint::WordPtr), "\xf7\x33", 2);
			Check(IntelAssembler::div<16>(Param::m_eax, PtrHint::WordPtr), "\x67\xf7\x30", 3);
			Check(IntelAssembler::div<16>(Param::m_eax, PtrHint::DwordPtr), "\x66\x67\xf7\x30", 4);

			Check(IntelAssembler::div<32>(Param::m_bp_di, PtrHint::BytePtr), "\x67\xf6\x33", 3);
			Check(IntelAssembler::div<32>(Param::m_bp_di, PtrHint::WordPtr), "\x66\x67\xf7\x33", 4);
			Check(IntelAssembler::div<32>(Param::m_eax, PtrHint::WordPtr), "\x66\xf7\x30", 3);
			Check(IntelAssembler::div<32>(Param::m_eax, PtrHint::DwordPtr), "\xf7\x30", 2);

			Check(IntelAssembler::div<64>(Param::m_eax, PtrHint::BytePtr), "\x67\xf6\x30", 3);
			Check(IntelAssembler::div<64>(Param::m_eax, PtrHint::WordPtr), "\x66\x67\xf7\x30", 4);
			Check(IntelAssembler::div<64>(Param::m_eax, PtrHint::DwordPtr), "\x67\xf7\x30", 3);
			Check(IntelAssembler::div<64>(Param::m_eax, PtrHint::QwordPtr), "\x67\x48\xf7\x30", 4);
			Check(IntelAssembler::div<64>(Param::m_rax, PtrHint::BytePtr), "\xf6\x30", 2);
			Check(IntelAssembler::div<64>(Param::m_rax, PtrHint::WordPtr), "\x66\xf7\x30", 3);
			Check(IntelAssembler::div<64>(Param::m_rax, PtrHint::DwordPtr), "\xf7\x30", 2);
			Check(IntelAssembler::div<64>(Param::m_rax, PtrHint::QwordPtr), "\x48\xf7\x30", 3);
		}

		TEST_METHOD(idiv)
		{
			Check(IntelAssembler::idiv<16>(Param::m_bp_di, PtrHint::BytePtr), "\xf6\x3b", 2);
			Check(IntelAssembler::idiv<16>(Param::m_bp_di, PtrHint::WordPtr), "\xf7\x3b", 2);
			Check(IntelAssembler::idiv<16>(Param::m_eax, PtrHint::WordPtr), "\x67\xf7\x38", 3);
			Check(IntelAssembler::idiv<16>(Param::m_eax, PtrHint::DwordPtr), "\x66\x67\xf7\x38", 4);

			Check(IntelAssembler::idiv<32>(Param::m_bp_di, PtrHint::BytePtr), "\x67\xf6\x3b", 3);
			Check(IntelAssembler::idiv<32>(Param::m_bp_di, PtrHint::WordPtr), "\x66\x67\xf7\x3b", 4);
			Check(IntelAssembler::idiv<32>(Param::m_eax, PtrHint::WordPtr), "\x66\xf7\x38", 3);
			Check(IntelAssembler::idiv<32>(Param::m_eax, PtrHint::DwordPtr), "\xf7\x38", 2);

			Check(IntelAssembler::idiv<64>(Param::m_eax, PtrHint::BytePtr), "\x67\xf6\x38", 3);
			Check(IntelAssembler::idiv<64>(Param::m_eax, PtrHint::WordPtr), "\x66\x67\xf7\x38", 4);
			Check(IntelAssembler::idiv<64>(Param::m_eax, PtrHint::DwordPtr), "\x67\xf7\x38", 3);
			Check(IntelAssembler::idiv<64>(Param::m_eax, PtrHint::QwordPtr), "\x67\x48\xf7\x38", 4);
			Check(IntelAssembler::idiv<64>(Param::m_rax, PtrHint::BytePtr), "\xf6\x38", 2);
			Check(IntelAssembler::idiv<64>(Param::m_rax, PtrHint::WordPtr), "\x66\xf7\x38", 3);
			Check(IntelAssembler::idiv<64>(Param::m_rax, PtrHint::DwordPtr), "\xf7\x38", 2);
			Check(IntelAssembler::idiv<64>(Param::m_rax, PtrHint::QwordPtr), "\x48\xf7\x38", 3);
		}

		TEST_METHOD(imul)
		{
			// M
			Check(IntelAssembler::imul<16>(Param::m_bp_di, PtrHint::BytePtr), "\xf6\x2b", 2);
			Check(IntelAssembler::imul<16>(Param::m_bp_di, PtrHint::WordPtr), "\xf7\x2b", 2);
			Check(IntelAssembler::imul<16>(Param::m_eax, PtrHint::WordPtr), "\x67\xf7\x28", 3);
			Check(IntelAssembler::imul<16>(Param::m_eax, PtrHint::DwordPtr), "\x66\x67\xf7\x28", 4);

			Check(IntelAssembler::imul<32>(Param::m_bp_di, PtrHint::BytePtr), "\x67\xf6\x2b", 3);
			Check(IntelAssembler::imul<32>(Param::m_bp_di, PtrHint::WordPtr), "\x66\x67\xf7\x2b", 4);
			Check(IntelAssembler::imul<32>(Param::m_eax, PtrHint::WordPtr), "\x66\xf7\x28", 3);
			Check(IntelAssembler::imul<32>(Param::m_eax, PtrHint::DwordPtr), "\xf7\x28", 2);

			Check(IntelAssembler::imul<64>(Param::m_eax, PtrHint::BytePtr), "\x67\xf6\x28", 3);
			Check(IntelAssembler::imul<64>(Param::m_eax, PtrHint::WordPtr), "\x66\x67\xf7\x28", 4);
			Check(IntelAssembler::imul<64>(Param::m_eax, PtrHint::DwordPtr), "\x67\xf7\x28", 3);
			Check(IntelAssembler::imul<64>(Param::m_eax, PtrHint::QwordPtr), "\x67\x48\xf7\x28", 4);
			Check(IntelAssembler::imul<64>(Param::m_rax, PtrHint::BytePtr), "\xf6\x28", 2);
			Check(IntelAssembler::imul<64>(Param::m_rax, PtrHint::WordPtr), "\x66\xf7\x28", 3);
			Check(IntelAssembler::imul<64>(Param::m_rax, PtrHint::DwordPtr), "\xf7\x28", 2);
			Check(IntelAssembler::imul<64>(Param::m_rax, PtrHint::QwordPtr), "\x48\xf7\x28", 3);

			// RM
			Check(IntelAssembler::imul<16>(Param::cx, Param::m_bp_di), "\x0f\xaf\x0b", 3);
			Check(IntelAssembler::imul<32>(Param::ecx, Param::m_eax), "\x0f\xaf\x08", 3);
			Check(IntelAssembler::imul<64>(Param::rcx, Param::m_rax), "\x48\x0f\xaf\x08", 4);

			// RMI
			Check(IntelAssembler::imul<16>(Param::cx, Param::m_bp_di, Param::imm8, 0, 0xaa), "\x6b\x0b\xaa", 3);
			Check(IntelAssembler::imul<32>(Param::ecx, Param::m_eax, Param::imm8, 0, 0xaa), "\x6b\x08\xaa", 3);
			Check(IntelAssembler::imul<64>(Param::rcx, Param::m_rax, Param::imm8, 0, 0xaa), "\x48\x6b\x08\xaa", 4);
			Check(IntelAssembler::imul<16>(Param::cx, Param::m_bp_di, Param::imm16, 0, 0x1234), "\x69\x0b\x34\x12", 4);
			Check(IntelAssembler::imul<32>(Param::ecx, Param::m_eax, Param::imm32, 0, 0x12345678), "\x69\x08\x78\x56\x34\x12", 6);
			Check(IntelAssembler::imul<64>(Param::rcx, Param::m_rax, Param::imm32, 0, 0x12345678), "\x48\x69\x08\x78\x56\x34\x12", 7);
		}

		TEST_METHOD(inc)
		{
			// M
			Check(IntelAssembler::inc<16>(Param::m_bp_di, PtrHint::BytePtr), "\xfe\x03", 2);
			Check(IntelAssembler::inc<16>(Param::m_bp_di, PtrHint::WordPtr), "\xff\x03", 2);
			Check(IntelAssembler::inc<16>(Param::m_eax, PtrHint::WordPtr), "\x67\xff\x00", 3);
			Check(IntelAssembler::inc<16>(Param::m_eax, PtrHint::DwordPtr), "\x66\x67\xff\x00", 4);

			Check(IntelAssembler::inc<32>(Param::m_bp_di, PtrHint::BytePtr), "\x67\xfe\x03", 3);
			Check(IntelAssembler::inc<32>(Param::m_bp_di, PtrHint::WordPtr), "\x66\x67\xff\x03", 4);
			Check(IntelAssembler::inc<32>(Param::m_eax, PtrHint::WordPtr), "\x66\xff\x00", 3);
			Check(IntelAssembler::inc<32>(Param::m_eax, PtrHint::DwordPtr), "\xff\x00", 2);

			Check(IntelAssembler::inc<64>(Param::m_eax, PtrHint::BytePtr), "\x67\xfe\x00", 3);
			Check(IntelAssembler::inc<64>(Param::m_eax, PtrHint::WordPtr), "\x66\x67\xff\x00", 4);
			Check(IntelAssembler::inc<64>(Param::m_eax, PtrHint::DwordPtr), "\x67\xff\x00", 3);
			Check(IntelAssembler::inc<64>(Param::m_eax, PtrHint::QwordPtr), "\x67\x48\xff\x00", 4);
			Check(IntelAssembler::inc<64>(Param::m_rax, PtrHint::BytePtr), "\xfe\x00", 2);
			Check(IntelAssembler::inc<64>(Param::m_rax, PtrHint::WordPtr), "\x66\xff\x00", 3);
			Check(IntelAssembler::inc<64>(Param::m_rax, PtrHint::DwordPtr), "\xff\x00", 2);
			Check(IntelAssembler::inc<64>(Param::m_rax, PtrHint::QwordPtr), "\x48\xff\x00", 3);

			// O
			Check(IntelAssembler::inc<16>(Param::ax), "\x40", 1);
			Check(IntelAssembler::inc<32>(Param::ax), "\x66\x40", 2);
			Check(IntelAssembler::inc<16>(Param::eax), "\x66\x40", 2);
			Check(IntelAssembler::inc<32>(Param::eax), "\x40", 1);
			Check(IntelAssembler::inc<64>(Param::ax), "\x66\xff\xc0", 3);
			Check(IntelAssembler::inc<64>(Param::eax), "\xff\xc0", 2);
		}

		TEST_METHOD(invlpg)
		{
			Check(IntelAssembler::invlpg<16>(Param::m_bp_di, PtrHint::BytePtr), "\x0f\x01\x3b", 3);
			Check(IntelAssembler::invlpg<32>(Param::m_bp_di, PtrHint::BytePtr), "\x67\x0f\x01\x3b", 4);
			Check(IntelAssembler::invlpg<32>(Param::m_eax, PtrHint::BytePtr), "\x0f\x01\x38", 3);
			Check(IntelAssembler::invlpg<64>(Param::m_eax, PtrHint::BytePtr), "\x67\x0f\x01\x38", 4);
			Check(IntelAssembler::invlpg<64>(Param::m_rax, PtrHint::BytePtr), "\x0f\x01\x38", 3);
		}

		TEST_METHOD(invpcid)
		{
			Check(IntelAssembler::invpcid<16>(Param::eax, Param::m_ecx), "\x66\x67\x0f\x38\x82\x01", 6);
			Check(IntelAssembler::invpcid<32>(Param::eax, Param::m_ecx), "\x66\x0f\x38\x82\x01", 5);
			Check(IntelAssembler::invpcid<64>(Param::rax, Param::m_rcx), "\x66\x48\x0f\x38\x82\x01", 6);

			Assert::ExpectException<char const*>([]() {
				IntelAssembler::invpcid<32>(Param::al, Param::m_eax);
				});
			Assert::ExpectException<char const*>([]() {
				IntelAssembler::invpcid<32>(Param::ax, Param::m_eax);
				});
			Assert::ExpectException<char const*>([]() {
				IntelAssembler::invpcid<64>(Param::al, Param::m_rax);
				});
			Assert::ExpectException<char const*>([]() {
				IntelAssembler::invpcid<64>(Param::ax, Param::m_rax);
				});
		}

		TEST_METHOD(jmp)
		{
			// Rel8, Rel16, Rel32
			Check(IntelAssembler::jmp<16>(Param::rel8, 0xaa), "\xeb\xaa", 2);
			Check(IntelAssembler::jmp<16>(Param::rel16, 0x1234), "\xe9\x34\x12", 3);
			Check(IntelAssembler::jmp<32>(Param::rel16, 0x1234), "\x66\xe9\x34\x12", 4);
			Check(IntelAssembler::jmp<16>(Param::rel32, 0x12345678), "\x66\xe9\x78\x56\x34\x12", 6);
			Check(IntelAssembler::jmp<32>(Param::rel32, 0x12345678), "\xe9\x78\x56\x34\x12", 5);
			Assert::ExpectException<char const*>([]() {
				IntelAssembler::jmp<64>(Param::rel16, 0x1234);
				});
			Check(IntelAssembler::jmp<64>(Param::rel32, 0x12345678), "\xe9\x78\x56\x34\x12", 5);

			// M
			Check(IntelAssembler::jmp<16>(Param::m_bx_si), "\xff\x20", 2);
			Check(IntelAssembler::jmp<32>(Param::m_bx_si), "\x67\xff\x20", 3);
			Check(IntelAssembler::jmp<16>(Param::m_eax), "\x67\xff\x20", 3);
			Check(IntelAssembler::jmp<32>(Param::m_eax), "\xff\x20", 2);
			Assert::ExpectException<char const*>([]() {
				IntelAssembler::jmp<64>(Param::m_bx_si);
				});
			Check(IntelAssembler::jmp<64>(Param::m_eax), "\x67\xff\x20", 3);
			Check(IntelAssembler::jmp<64>(Param::m_rax), "\xff\x20", 2);
		}

		TEST_METHOD(jmpf)
		{
			// D
			Check(IntelAssembler::jmpf<16>(Param::farptr16, 0x1234, 0x1234), "\xea\x34\x12\x34\x12", 5);
			Check(IntelAssembler::jmpf<32>(Param::farptr16, 0x1234, 0x1234), "\x66\xea\x34\x12\x34\x12", 6);
			Check(IntelAssembler::jmpf<16>(Param::farptr32, 0x1234, 0x12345678), "\x66\xea\x78\x56\x34\x12\x34\x12", 8);
			Check(IntelAssembler::jmpf<32>(Param::farptr32, 0x1234, 0x12345678), "\xea\x78\x56\x34\x12\x34\x12", 7);

			// M
			Check(IntelAssembler::jmpf<16>(Param::m_bx_si), "\xff\x28", 2);
			Check(IntelAssembler::jmpf<32>(Param::m_bx_si), "\x67\xff\x28", 3);
			Check(IntelAssembler::jmpf<16>(Param::m_eax), "\x67\xff\x28", 3);
			Check(IntelAssembler::jmpf<32>(Param::m_eax), "\xff\x28", 2);
			Assert::ExpectException<char const*>([]() {
				IntelAssembler::jmpf<64>(Param::m_bx_si);
				});
			Check(IntelAssembler::jmpf<64>(Param::m_eax), "\x67\xff\x28", 3);
			Check(IntelAssembler::jmpf<64>(Param::m_rax), "\xff\x28", 2);
		}

		TEST_METHOD(lar)
		{
			Check(IntelAssembler::lar<16>(Param::ax, Param::m_bx_si, 0), "\x0f\x02\x00", 3);
			Check(IntelAssembler::lar<32>(Param::eax, Param::m_eax, 0), "\x0f\x02\x00", 3);
			Check(IntelAssembler::lar<64>(Param::rax, Param::m_rax, 0), "\x48\x0f\x02\x00", 4);
		}

		TEST_METHOD(lds)
		{
			// 16-bit

			Check(IntelAssembler::lds<16>(Param::ax, Param::m_bx_si, 0), "\xc5\x00", 2);
			Check(IntelAssembler::lds<16>(Param::ecx, Param::m_eax, 0), "\x66\x67\xc5\x08", 4);

			// 32-bit

			Check(IntelAssembler::lds<32>(Param::ax, Param::m_bx_si, 0), "\x66\x67\xc5\x00", 4);
			Check(IntelAssembler::lds<32>(Param::ecx, Param::m_eax, 0), "\xc5\x08", 2);

			// 64-bit -- Failed

			Assert::ExpectException<char const*>([]() {
				IntelAssembler::lds<64>(Param::ax, Param::m_bx_si, 0);
				});
		}

		TEST_METHOD(lea)
		{
			// 16-bit

			Check(IntelAssembler::lea<16>(Param::ax, Param::m_bx_si, 0), "\x8d\x00", 2);
			Check(IntelAssembler::lea<16>(Param::ecx, Param::m_eax, 0), "\x66\x67\x8d\x08", 4);

			// 32-bit

			Check(IntelAssembler::lea<32>(Param::ax, Param::m_bx_si, 0), "\x66\x67\x8d\x00", 4);
			Check(IntelAssembler::lea<32>(Param::ecx, Param::m_eax, 0), "\x8d\x08", 2);

			// 64-bit

			Check(IntelAssembler::lea<64>(Param::rcx, Param::m_rax, 0), "\x48\x8d\x08", 3);
		}

		TEST_METHOD(les)
		{
			// 16-bit

			Check(IntelAssembler::les<16>(Param::ax, Param::m_bx_si, 0), "\xc4\x00", 2);
			Check(IntelAssembler::les<16>(Param::ecx, Param::m_eax, 0), "\x66\x67\xc4\x08", 4);

			// 32-bit

			Check(IntelAssembler::les<32>(Param::ax, Param::m_bx_si, 0), "\x66\x67\xc4\x00", 4);
			Check(IntelAssembler::les<32>(Param::ecx, Param::m_eax, 0), "\xc4\x08", 2);

			// 64-bit -- Failed

			Assert::ExpectException<char const*>([]() {
				IntelAssembler::les<64>(Param::ax, Param::m_bx_si, 0);
				});
		}

		TEST_METHOD(lfs)
		{
			Check(IntelAssembler::lfs<16>(Param::ax, Param::m_bx_si, 0), "\x0f\xb4\x00", 3);
			Check(IntelAssembler::lfs<16>(Param::ecx, Param::m_eax, 0), "\x66\x67\x0f\xb4\x08", 5);
			Check(IntelAssembler::lfs<32>(Param::ax, Param::m_bx_si, 0), "\x66\x67\x0f\xb4\x00", 5);
			Check(IntelAssembler::lfs<32>(Param::ecx, Param::m_eax, 0), "\x0f\xb4\x08", 3);
			Check(IntelAssembler::lfs<64>(Param::rcx, Param::m_rax, 0), "\x48\x0f\xb4\x08", 4);
		}

		TEST_METHOD(lgdt)
		{
			Check(IntelAssembler::lgdt<16>(Param::m_bx_si), "\x0f\x01\x10", 3);
			Check(IntelAssembler::lgdt<32>(Param::m_eax), "\x0f\x01\x10", 3);
			Check(IntelAssembler::lgdt<64>(Param::m_rax), "\x0f\x01\x10", 3);
		}

		TEST_METHOD(lgs)
		{
			Check(IntelAssembler::lgs<16>(Param::ax, Param::m_bx_si, 0), "\x0f\xb5\x00", 3);
			Check(IntelAssembler::lgs<16>(Param::ecx, Param::m_eax, 0), "\x66\x67\x0f\xb5\x08", 5);
			Check(IntelAssembler::lgs<32>(Param::ax, Param::m_bx_si, 0), "\x66\x67\x0f\xb5\x00", 5);
			Check(IntelAssembler::lgs<32>(Param::ecx, Param::m_eax, 0), "\x0f\xb5\x08", 3);
			Check(IntelAssembler::lgs<64>(Param::rcx, Param::m_rax, 0), "\x48\x0f\xb5\x08", 4);
		}

		TEST_METHOD(lidt)
		{
			Check(IntelAssembler::lidt<16>(Param::m_bx_si), "\x0f\x01\x18", 3);
			Check(IntelAssembler::lidt<32>(Param::m_eax), "\x0f\x01\x18", 3);
			Check(IntelAssembler::lidt<64>(Param::m_rax), "\x0f\x01\x18", 3);
		}

		TEST_METHOD(lldt)
		{
			Check(IntelAssembler::lldt<16>(Param::m_bx_si), "\x0f\x00\x10", 3);
			Check(IntelAssembler::lldt<32>(Param::m_eax), "\x0f\x00\x10", 3);
			Check(IntelAssembler::lldt<64>(Param::m_rax), "\x0f\x00\x10", 3);
		}

		TEST_METHOD(lmsw)
		{
			Check(IntelAssembler::lmsw<16>(Param::m_bx_si), "\x0f\x01\x30", 3);
			Check(IntelAssembler::lmsw<32>(Param::m_eax), "\x0f\x01\x30", 3);
			Check(IntelAssembler::lmsw<64>(Param::m_rax), "\x0f\x01\x30", 3);
		}

		TEST_METHOD(lsl)
		{
			Check(IntelAssembler::lsl<16>(Param::ax, Param::m_bx_si, 0), "\x0f\x03\x00", 3);
			Check(IntelAssembler::lsl<16>(Param::ecx, Param::m_eax, 0), "\x66\x67\x0f\x03\x08", 5);
			Check(IntelAssembler::lsl<32>(Param::ax, Param::m_bx_si, 0), "\x66\x67\x0f\x03\x00", 5);
			Check(IntelAssembler::lsl<32>(Param::ecx, Param::m_eax, 0), "\x0f\x03\x08", 3);
			Check(IntelAssembler::lsl<64>(Param::rcx, Param::m_rax, 0), "\x48\x0f\x03\x08", 4);
		}

		TEST_METHOD(lss)
		{
			Check(IntelAssembler::lss<16>(Param::ax, Param::m_bx_si, 0), "\x0f\xb2\x00", 3);
			Check(IntelAssembler::lss<16>(Param::ecx, Param::m_eax, 0), "\x66\x67\x0f\xb2\x08", 5);
			Check(IntelAssembler::lss<32>(Param::ax, Param::m_bx_si, 0), "\x66\x67\x0f\xb2\x00", 5);
			Check(IntelAssembler::lss<32>(Param::ecx, Param::m_eax, 0), "\x0f\xb2\x08", 3);
			Check(IntelAssembler::lss<64>(Param::rcx, Param::m_rax, 0), "\x48\x0f\xb2\x08", 4);
		}

		TEST_METHOD(ltr)
		{
			Check(IntelAssembler::ltr<16>(Param::m_bx_si), "\x0f\x00\x18", 3);
			Check(IntelAssembler::ltr<32>(Param::m_eax), "\x0f\x00\x18", 3);
			Check(IntelAssembler::ltr<64>(Param::m_rax), "\x0f\x00\x18", 3);
		}

		TEST_METHOD(verr)
		{
			Check(IntelAssembler::verr<16>(Param::m_bx_si), "\x0f\x00\x20", 3);
			Check(IntelAssembler::verr<32>(Param::m_eax), "\x0f\x00\x20", 3);
			Check(IntelAssembler::verr<64>(Param::m_rax), "\x0f\x00\x20", 3);
		}

		TEST_METHOD(verw)
		{
			Check(IntelAssembler::verw<16>(Param::m_bx_si), "\x0f\x00\x28", 3);
			Check(IntelAssembler::verw<32>(Param::m_eax), "\x0f\x00\x28", 3);
			Check(IntelAssembler::verw<64>(Param::m_rax), "\x0f\x00\x28", 3);
		}

		TEST_METHOD(mov)
		{
			// MR
			Check(IntelAssembler::mov<16>(Param::m_bp_di, Param::al), "\x88\x03", 2);
			Check(IntelAssembler::mov<64>(Param::sib_r11_4_rdx, Param::r8b), "\x46\x88\x04\x9a", 4);
			Check(IntelAssembler::mov<16>(Param::m_bx, Param::ax), "\x89\x07", 2);
			Check(IntelAssembler::mov<32>(Param::m_ebx, Param::eax), "\x89\x03", 2);
			Check(IntelAssembler::mov<64>(Param::m_rbx, Param::rax), "\x48\x89\x03", 3);
			Check(IntelAssembler::mov<64>(Param::m_eax, Param::rax), "\x67\x48\x89\x00", 4);

			// RM
			Check(IntelAssembler::mov<16>(Param::al, Param::m_bp_di), "\x8a\x03", 2);
			Check(IntelAssembler::mov<64>(Param::r8b, Param::sib_r11_4_rdx), "\x46\x8a\x04\x9a", 4);
			Check(IntelAssembler::mov<16>(Param::ax, Param::m_bx), "\x8b\x07", 2);
			Check(IntelAssembler::mov<32>(Param::eax, Param::m_ebx), "\x8b\x03", 2);
			Check(IntelAssembler::mov<64>(Param::rax, Param::m_rbx), "\x48\x8b\x03", 3);

			// MSr (Sreg)
			Check(IntelAssembler::mov<16>(Param::ax, Param::ds), "\x8c\xd8", 2);
			Check(IntelAssembler::mov<16>(Param::m_bp_di, Param::ds), "\x8c\x1b", 2);
			Check(IntelAssembler::mov<32>(Param::ax, Param::ds), "\x66\x8c\xd8", 3);
			Check(IntelAssembler::mov<32>(Param::m_eax, Param::ds, 0, 0, PtrHint::WordPtr), "\x66\x8c\x18", 3);
			Check(IntelAssembler::mov<64>(Param::rax, Param::ds), "\x48\x8c\xd8", 3);
			Check(IntelAssembler::mov<64>(Param::m_rax, Param::ds), "\x8c\x18", 2);

			// SrM (Sreg)
			Check(IntelAssembler::mov<16>(Param::ds, Param::ax), "\x8e\xd8", 2);
			Check(IntelAssembler::mov<16>(Param::ds, Param::m_bp_di), "\x8e\x1b", 2);
			Check(IntelAssembler::mov<32>(Param::ds, Param::ax), "\x66\x8e\xd8", 3);
			Check(IntelAssembler::mov<32>(Param::ds, Param::m_eax, 0, 0, PtrHint::WordPtr), "\x66\x8e\x18", 3);
			Check(IntelAssembler::mov<64>(Param::ds, Param::rax), "\x48\x8e\xd8", 3);
			Check(IntelAssembler::mov<64>(Param::ds, Param::m_rax), "\x8e\x18", 2);

			// FD
			Check(IntelAssembler::mov<16>(Param::al, Param::moffs16, 0x1234), "\xa0\x34\x12", 3);
			Check(IntelAssembler::mov<64>(Param::al, Param::moffs64, 0x12345678aabbccdd), "\x48\xa0\xdd\xcc\xbb\xaa\x78\x56\x34\x12", 10);
			Check(IntelAssembler::mov<16>(Param::ax, Param::moffs16, 0x1234), "\xa1\x34\x12", 3);
			Check(IntelAssembler::mov<32>(Param::eax, Param::moffs32, 0x12345678), "\xa1\x78\x56\x34\x12", 5);
			Check(IntelAssembler::mov<64>(Param::rax, Param::moffs64, 0x12345678aabbccdd), "\x48\xa1\xdd\xcc\xbb\xaa\x78\x56\x34\x12", 10);

			// TD
			Check(IntelAssembler::mov<16>(Param::moffs16, Param::al, 0x1234), "\xa2\x34\x12", 3);
			Check(IntelAssembler::mov<64>(Param::moffs64, Param::al, 0x12345678aabbccdd), "\x48\xa2\xdd\xcc\xbb\xaa\x78\x56\x34\x12", 10);
			Check(IntelAssembler::mov<16>(Param::moffs16, Param::ax, 0x1234), "\xa3\x34\x12", 3);
			Check(IntelAssembler::mov<32>(Param::moffs32, Param::eax, 0x12345678), "\xa3\x78\x56\x34\x12", 5);
			Check(IntelAssembler::mov<64>(Param::moffs64, Param::rax, 0x12345678aabbccdd), "\x48\xa3\xdd\xcc\xbb\xaa\x78\x56\x34\x12", 10);

			// OI
			Check(IntelAssembler::mov<16>(Param::cl, Param::imm8, 0, 0x12), "\xb1\x12", 2);
			Check(IntelAssembler::mov<64>(Param::r9b, Param::imm8, 0, 0x12), "\x41\xb1\x12", 3);
			Check(IntelAssembler::mov<16>(Param::cx, Param::imm16, 0, 0x1234), "\xb9\x34\x12", 3);
			Check(IntelAssembler::mov<32>(Param::ecx, Param::imm32, 0, 0x12345678), "\xb9\x78\x56\x34\x12", 5);
			Check(IntelAssembler::mov<64>(Param::rcx, Param::imm64, 0, 0x1122334455667788), "\x48\xb9\x88\x77\x66\x55\x44\x33\x22\x11", 10);

			// MI
			Check(IntelAssembler::mov<16>(Param::m_bx_si, Param::imm8, 0, 0xaa), "\xc6\x00\xaa", 3);
			Check(IntelAssembler::mov<16>(Param::m_bx_si, Param::imm16, 0, 0x1234), "\xc7\x00\x34\x12", 4);
			Check(IntelAssembler::mov<32>(Param::m_ecx, Param::imm32, 0, 0x12345678), "\xc7\x01\x78\x56\x34\x12", 6);
			Check(IntelAssembler::mov<64>(Param::m_rcx, Param::imm32, 0, 0x12345678), "\xc7\x01\x78\x56\x34\x12", 6);
			Check(IntelAssembler::mov<64>(Param::m_rcx, Param::imm32, 0, 0x12345678, PtrHint::QwordPtr), "\x48\xc7\x01\x78\x56\x34\x12", 7);
		}

		TEST_METHOD(movbe)
		{
			Check(IntelAssembler::movbe<16>(Param::ax, Param::m_bp_di), "\x0f\x38\xf0\x03", 4);
			Check(IntelAssembler::movbe<32>(Param::eax, Param::m_eax), "\x0f\x38\xf0\x00", 4);
			Check(IntelAssembler::movbe<64>(Param::rax, Param::m_rax), "\x48\x0f\x38\xf0\x00", 5);

			Check(IntelAssembler::movbe<16>(Param::m_bp_di, Param::ax), "\x0f\x38\xf1\x03", 4);
			Check(IntelAssembler::movbe<32>(Param::m_eax, Param::eax), "\x0f\x38\xf1\x00", 4);
			Check(IntelAssembler::movbe<64>(Param::m_rax, Param::rax), "\x48\x0f\x38\xf1\x00", 5);
		}

		TEST_METHOD(movsx)
		{
			Check(IntelAssembler::movsx<16>(Param::ax, Param::al), "\x0f\xbe\xc0", 3);
			Check(IntelAssembler::movsx<32>(Param::eax, Param::al), "\x0f\xbe\xc0", 3);
			Check(IntelAssembler::movsx<64>(Param::rax, Param::al), "\x48\x0f\xbe\xc0", 4);
			Check(IntelAssembler::movsx<32>(Param::eax, Param::ax), "\x0f\xbf\xc0", 3);
			Check(IntelAssembler::movsx<64>(Param::rax, Param::ax), "\x48\x0f\xbf\xc0", 4);

			// Try MOVSX with memory (we will not try MOVSXD and MOVZX).

			Check(IntelAssembler::movsx<16>(Param::ax, Param::m_bp_di, 0, PtrHint::BytePtr), "\x0f\xbe\x03", 3);
			Check(IntelAssembler::movsx<32>(Param::eax, Param::m_bp_di, 0, PtrHint::BytePtr), "\x67\x0f\xbe\x03", 4);
			Check(IntelAssembler::movsx<32>(Param::eax, Param::m_eax, 0, PtrHint::BytePtr), "\x0f\xbe\x00", 3);
			Check(IntelAssembler::movsx<32>(Param::eax, Param::m_eax, 0, PtrHint::WordPtr), "\x0f\xbf\x00", 3);
			Check(IntelAssembler::movsx<16>(Param::eax, Param::m_eax, 0, PtrHint::WordPtr), "\x66\x67\x0f\xbf\x00", 5);
			Check(IntelAssembler::movsx<64>(Param::rax, Param::m_rax, 0, PtrHint::WordPtr), "\x48\x0f\xbf\x00", 4);
		}

		TEST_METHOD(movsxd)
		{
			Check(IntelAssembler::movsxd<64>(Param::rax, Param::eax), "\x48\x63\xc0", 3);
		}

		TEST_METHOD(movzx)
		{
			Check(IntelAssembler::movzx<16>(Param::ax, Param::al), "\x0f\xb6\xc0", 3);
			Check(IntelAssembler::movzx<32>(Param::eax, Param::al), "\x0f\xb6\xc0", 3);
			Check(IntelAssembler::movzx<64>(Param::rax, Param::al), "\x48\x0f\xb6\xc0", 4);
			Check(IntelAssembler::movzx<32>(Param::eax, Param::ax), "\x0f\xb7\xc0", 3);
			Check(IntelAssembler::movzx<64>(Param::rax, Param::ax), "\x48\x0f\xb7\xc0", 4);
		}

		TEST_METHOD(mul)
		{
			Check(IntelAssembler::mul<16>(Param::m_bp_di, PtrHint::BytePtr), "\xf6\x23", 2);
			Check(IntelAssembler::mul<32>(Param::m_bp_di, PtrHint::BytePtr), "\x67\xf6\x23", 3);
			Check(IntelAssembler::mul<64>(Param::m_eax, PtrHint::BytePtr), "\x67\xf6\x20", 3);
		}

		TEST_METHOD(nop)
		{
			Check(IntelAssembler::nop<16>(), "\x90", 1);

			// Table 4-12. Recommended Multi-Byte Sequence of NOP Instruction

			Check(IntelAssembler::nop<32>(Prefix::OperandSize), "\x66\x90", 2);
			Check(IntelAssembler::nop<32>(Param::m_eax, PtrHint::DwordPtr), "\x0f\x1f\x00", 3);
			Check(IntelAssembler::nop<32>(Param::m_eax_disp8, PtrHint::DwordPtr, 0), "\x0f\x1f\x40\x00", 4);
			Check(IntelAssembler::nop<32>(Param::sib_eax_eax_disp8, PtrHint::DwordPtr, 0), "\x0f\x1f\x44\x00\x00", 5);
			Check(IntelAssembler::nop<32>(Param::sib_eax_eax_disp8, PtrHint::WordPtr, 0), "\x66\x0f\x1f\x44\x00\x00", 6);
			Check(IntelAssembler::nop<32>(Param::m_eax_disp32, PtrHint::DwordPtr, 0), "\x0f\x1f\x80\x00\x00\x00\x00", 7);
			Check(IntelAssembler::nop<32>(Param::sib_eax_eax_disp32, PtrHint::DwordPtr, 0), "\x0f\x1f\x84\x00\x00\x00\x00", 8);
			Check(IntelAssembler::nop<32>(Param::sib_eax_eax_disp32, PtrHint::WordPtr, 0), "\x66\x0f\x1f\x84\x00\x00\x00\x00", 9);
		}

		TEST_METHOD(_not)
		{
			Check(IntelAssembler::_not<16>(Param::m_bp_di, PtrHint::BytePtr), "\xf6\x13", 2);
			Check(IntelAssembler::_not<32>(Param::m_bp_di, PtrHint::BytePtr), "\x67\xf6\x13", 3);
			Check(IntelAssembler::_not<64>(Param::m_eax, PtrHint::WordPtr), "\x66\x67\xf7\x10", 4);
		}

		TEST_METHOD(_or)
		{
			Check(IntelAssembler::_or<16>(Param::al, Param::imm8, 0, 0xaa), "\x0c\xaa", 2);
			Check(IntelAssembler::_or<16>(Param::ax, Param::imm16, 0, 0x1234), "\x0d\x34\x12", 3);
			Check(IntelAssembler::_or<16>(Param::cl, Param::imm8, 0, 0xaa), "\x80\xc9\xaa", 3);
			Check(IntelAssembler::_or<16>(Param::cx, Param::imm16, 0, 0x1234), "\x81\xc9\x34\x12", 4);
			Check(IntelAssembler::_or<16>(Param::cx, Param::simm8_as16, 0, -0x56), "\x83\xc9\xaa", 3);
			Check(IntelAssembler::_or<16>(Param::m_bp_di, Param::al, 0, 0), "\x08\x03", 2);
			Check(IntelAssembler::_or<16>(Param::m_bx, Param::ax, 0, 0), "\x09\x07", 2);
			Check(IntelAssembler::_or<16>(Param::al, Param::m_bp_di, 0, 0), "\x0a\x03", 2);
			Check(IntelAssembler::_or<16>(Param::ax, Param::m_bx, 0, 0), "\x0b\x07", 2);
		}

		TEST_METHOD(pop)
		{
			Check(IntelAssembler::pop<16>(Param::m_bp_di, PtrHint::WordPtr), "\x8f\x03", 2);
			Check(IntelAssembler::pop<32>(Param::m_eax, PtrHint::DwordPtr), "\x8f\x00", 2);
			Check(IntelAssembler::pop<64>(Param::m_rax, PtrHint::QwordPtr), "\x48\x8f\x00", 3);

			Check(IntelAssembler::pop<16>(Param::ax), "\x58", 1);
			Check(IntelAssembler::pop<32>(Param::eax), "\x58", 1);
			Check(IntelAssembler::pop<64>(Param::rax), "\x58", 1);
		}

		TEST_METHOD(push)
		{
			Check(IntelAssembler::push<16>(Param::m_bp_di, PtrHint::WordPtr), "\xff\x33", 2);
			Check(IntelAssembler::push<32>(Param::m_eax, PtrHint::DwordPtr), "\xff\x30", 2);
			Check(IntelAssembler::push<64>(Param::m_rax, PtrHint::QwordPtr), "\x48\xff\x30", 3);

			Check(IntelAssembler::push<16>(Param::ax), "\x50", 1);
			Check(IntelAssembler::push<32>(Param::eax), "\x50", 1);
			Check(IntelAssembler::push<64>(Param::rax), "\x50", 1);

			Check(IntelAssembler::push<16>(Param::imm8, PtrHint::BytePtr, 0, 0xaa), "\x6a\xaa", 2);
			Check(IntelAssembler::push<16>(Param::imm16, PtrHint::WordPtr, 0, 0x1234), "\x68\x34\x12", 3);
			Check(IntelAssembler::push<32>(Param::imm32, PtrHint::DwordPtr, 0, 0x12345678), "\x68\x78\x56\x34\x12", 5);
		}

		TEST_METHOD(rcl)
		{
			// M1
			Check(IntelAssembler::rcl<16>(Param::al, Param::imm8, 0, 1), "\xd0\xd0", 2);
			Check(IntelAssembler::rcl<64>(Param::sil, Param::imm8, 0, 1), "\x40\xd0\xd6", 3);
			Check(IntelAssembler::rcl<16>(Param::m_bp_di, Param::imm8, 0, 1, PtrHint::BytePtr), "\xd0\x13", 2);
			Check(IntelAssembler::rcl<16>(Param::ax, Param::imm8, 0, 1), "\xd1\xd0", 2);
			Check(IntelAssembler::rcl<16>(Param::m_bp_di, Param::imm8, 0, 1, PtrHint::WordPtr), "\xd1\x13", 2);
			Check(IntelAssembler::rcl<32>(Param::m_eax, Param::imm8, 0, 1, PtrHint::DwordPtr), "\xd1\x10", 2);
			Check(IntelAssembler::rcl<64>(Param::rax, Param::imm8, 0, 1), "\x48\xd1\xd0", 3);
			Check(IntelAssembler::rcl<64>(Param::m_rax, Param::imm8, 0, 1, PtrHint::QwordPtr), "\x48\xd1\x10", 3);

			// MC
			Check(IntelAssembler::rcl<16>(Param::al, Param::cl), "\xd2\xd0", 2);
			Check(IntelAssembler::rcl<64>(Param::sil, Param::cl), "\x40\xd2\xd6", 3);
			Check(IntelAssembler::rcl<16>(Param::m_bp_di, Param::cl, 0, 0, PtrHint::BytePtr), "\xd2\x13", 2);
			Check(IntelAssembler::rcl<32>(Param::eax, Param::cl), "\xd3\xd0", 2);
			Check(IntelAssembler::rcl<32>(Param::m_eax, Param::cl, 0, 0, PtrHint::DwordPtr), "\xd3\x10", 2);
			Check(IntelAssembler::rcl<64>(Param::m_rax, Param::cl, 0, 0, PtrHint::QwordPtr), "\x48\xd3\x10", 3);
			Check(IntelAssembler::rcl<64>(Param::rax, Param::cl), "\x48\xd3\xd0", 3);

			// MI
			Check(IntelAssembler::rcl<16>(Param::m_bp_di, Param::imm8, 0, 3, PtrHint::BytePtr), "\xc0\x13\x03", 3);
			Check(IntelAssembler::rcl<32>(Param::m_eax, Param::imm8, 0, 3, PtrHint::BytePtr), "\xc0\x10\x03", 3);
			Check(IntelAssembler::rcl<16>(Param::m_bp_di, Param::imm8, 0, 3, PtrHint::WordPtr), "\xc1\x13\x03", 3);
			Check(IntelAssembler::rcl<32>(Param::m_eax, Param::imm8, 0, 3, PtrHint::DwordPtr), "\xc1\x10\x03", 3);
			Check(IntelAssembler::rcl<64>(Param::m_rax, Param::imm8, 0, 3, PtrHint::QwordPtr), "\x48\xc1\x10\x03", 4);
		}

		TEST_METHOD(rcr)
		{
			Check(IntelAssembler::rcr<16>(Param::al, Param::imm8, 0, 1), "\xd0\xd8", 2);
		}

		TEST_METHOD(rol)
		{
			Check(IntelAssembler::rol<16>(Param::al, Param::imm8, 0, 1), "\xd0\xc0", 2);
		}

		TEST_METHOD(ror)
		{
			Check(IntelAssembler::ror<16>(Param::al, Param::imm8, 0, 1), "\xd0\xc8", 2);
		}

		TEST_METHOD(sal)
		{
			Check(IntelAssembler::sal<16>(Param::al, Param::imm8, 0, 1), "\xd0\xe0", 2);
		}

		TEST_METHOD(sar)
		{
			Check(IntelAssembler::sar<16>(Param::al, Param::imm8, 0, 1), "\xd0\xf8", 2);
		}

		TEST_METHOD(sbb)
		{
			Check(IntelAssembler::sbb<16>(Param::al, Param::imm8, 0, 0xaa), "\x1c\xaa", 2);
			Check(IntelAssembler::sbb<16>(Param::ax, Param::imm16, 0, 0x1234), "\x1d\x34\x12", 3);
			Check(IntelAssembler::sbb<16>(Param::cl, Param::imm8, 0, 0xaa), "\x80\xd9\xaa", 3);
			Check(IntelAssembler::sbb<16>(Param::cx, Param::imm16, 0, 0x1234), "\x81\xd9\x34\x12", 4);
			Check(IntelAssembler::sbb<16>(Param::cx, Param::simm8_as16, 0, -0x56), "\x83\xd9\xaa", 3);
			Check(IntelAssembler::sbb<16>(Param::m_bp_di, Param::al, 0, 0), "\x18\x03", 2);
			Check(IntelAssembler::sbb<16>(Param::m_bx, Param::ax, 0, 0), "\x19\x07", 2);
			Check(IntelAssembler::sbb<16>(Param::al, Param::m_bp_di, 0, 0), "\x1a\x03", 2);
			Check(IntelAssembler::sbb<16>(Param::ax, Param::m_bx, 0, 0), "\x1b\x07", 2);
		}

		TEST_METHOD(seta)
		{
			Check(IntelAssembler::seta<16>(Param::al), "\x0f\x97\xc0", 3);
			Check(IntelAssembler::seta<16>(Param::m_bp_di), "\x0f\x97\x03", 3);
			Check(IntelAssembler::seta<32>(Param::m_eax), "\x0f\x97\x00", 3);
			Check(IntelAssembler::seta<64>(Param::sil), "\x40\x0f\x97\xc6", 4);
			Check(IntelAssembler::seta<64>(Param::m_rax), "\x0f\x97\x00", 3);
		}

		TEST_METHOD(setae)
		{
			Check(IntelAssembler::setae<16>(Param::al), "\x0f\x93\xc0", 3);
		}

		TEST_METHOD(setb)
		{
			Check(IntelAssembler::setb<16>(Param::al), "\x0f\x92\xc0", 3);
		}

		TEST_METHOD(setbe)
		{
			Check(IntelAssembler::setbe<16>(Param::al), "\x0f\x96\xc0", 3);
		}

		TEST_METHOD(setc)
		{
			Check(IntelAssembler::setc<16>(Param::al), "\x0f\x92\xc0", 3);
		}

		TEST_METHOD(sete)
		{
			Check(IntelAssembler::sete<16>(Param::al), "\x0f\x94\xc0", 3);
		}

		TEST_METHOD(setg)
		{
			Check(IntelAssembler::setg<16>(Param::al), "\x0f\x9f\xc0", 3);
		}

		TEST_METHOD(setge)
		{
			Check(IntelAssembler::setge<16>(Param::al), "\x0f\x9d\xc0", 3);
		}

		TEST_METHOD(setl)
		{
			Check(IntelAssembler::setl<16>(Param::al), "\x0f\x9c\xc0", 3);
		}

		TEST_METHOD(setle)
		{
			Check(IntelAssembler::setle<16>(Param::al), "\x0f\x9e\xc0", 3);
		}

		TEST_METHOD(setna)
		{
			Check(IntelAssembler::setna<16>(Param::al), "\x0f\x96\xc0", 3);
		}

		TEST_METHOD(setnae)
		{
			Check(IntelAssembler::setnae<16>(Param::al), "\x0f\x92\xc0", 3);
		}

		TEST_METHOD(setnb)
		{
			Check(IntelAssembler::setnb<16>(Param::al), "\x0f\x93\xc0", 3);
		}

		TEST_METHOD(setnbe)
		{
			Check(IntelAssembler::setnbe<16>(Param::al), "\x0f\x97\xc0", 3);
		}

		TEST_METHOD(setnc)
		{
			Check(IntelAssembler::setnc<16>(Param::al), "\x0f\x93\xc0", 3);
		}

		TEST_METHOD(setne)
		{
			Check(IntelAssembler::setne<16>(Param::al), "\x0f\x95\xc0", 3);
		}

		TEST_METHOD(setng)
		{
			Check(IntelAssembler::setng<16>(Param::al), "\x0f\x9e\xc0", 3);
		}

		TEST_METHOD(setnge)
		{
			Check(IntelAssembler::setnge<16>(Param::al), "\x0f\x9c\xc0", 3);
		}

		TEST_METHOD(setnl)
		{
			Check(IntelAssembler::setnl<16>(Param::al), "\x0f\x9d\xc0", 3);
		}

		TEST_METHOD(setnle)
		{
			Check(IntelAssembler::setnle<16>(Param::al), "\x0f\x9f\xc0", 3);
		}

		TEST_METHOD(setno)
		{
			Check(IntelAssembler::setno<16>(Param::al), "\x0f\x91\xc0", 3);
		}

		TEST_METHOD(setnp)
		{
			Check(IntelAssembler::setnp<16>(Param::al), "\x0f\x9b\xc0", 3);
		}

		TEST_METHOD(setns)
		{
			Check(IntelAssembler::setns<16>(Param::al), "\x0f\x99\xc0", 3);
		}

		TEST_METHOD(setnz)
		{
			Check(IntelAssembler::setnz<16>(Param::al), "\x0f\x95\xc0", 3);
		}

		TEST_METHOD(seto)
		{
			Check(IntelAssembler::seto<16>(Param::al), "\x0f\x90\xc0", 3);
		}

		TEST_METHOD(setp)
		{
			Check(IntelAssembler::setp<16>(Param::al), "\x0f\x9a\xc0", 3);
		}

		TEST_METHOD(setpe)
		{
			Check(IntelAssembler::setpe<16>(Param::al), "\x0f\x9a\xc0", 3);
		}

		TEST_METHOD(setpo)
		{
			Check(IntelAssembler::setpo<16>(Param::al), "\x0f\x9b\xc0", 3);
		}

		TEST_METHOD(sets)
		{
			Check(IntelAssembler::sets<16>(Param::al), "\x0f\x98\xc0", 3);
		}

		TEST_METHOD(setz)
		{
			Check(IntelAssembler::setz<16>(Param::al), "\x0f\x94\xc0", 3);
		}

		TEST_METHOD(sgdt)
		{
			Check(IntelAssembler::sgdt<32>(Param::m_eax), "\x0f\x01\x00", 3);
		}

		TEST_METHOD(shl)
		{
			Check(IntelAssembler::shl<16>(Param::al, Param::imm8, 0, 1), "\xd0\xe0", 2);
		}

		TEST_METHOD(shld)
		{
			Check(IntelAssembler::shld<16>(Param::ax, Param::cx, Param::imm8, 0, 3), "\x0f\xa4\xc8\x03", 4);
			Check(IntelAssembler::shld<16>(Param::m_bp_di, Param::cx, Param::imm8, 0, 3), "\x0f\xa4\x0b\x03", 4);
			Check(IntelAssembler::shld<16>(Param::ax, Param::cx, Param::cl), "\x0f\xa5\xc8", 3);

			Check(IntelAssembler::shld<32>(Param::eax, Param::ecx, Param::imm8, 0, 3), "\x0f\xa4\xc8\x03", 4);
			Check(IntelAssembler::shld<32>(Param::m_eax, Param::ecx, Param::imm8, 0, 3), "\x0f\xa4\x08\x03", 4);
			Check(IntelAssembler::shld<32>(Param::eax, Param::ecx, Param::cl), "\x0f\xa5\xc8", 3);

			Check(IntelAssembler::shld<64>(Param::rax, Param::rcx, Param::imm8, 0, 3), "\x48\x0f\xa4\xc8\x03", 5);
			Check(IntelAssembler::shld<64>(Param::m_rax, Param::rcx, Param::imm8, 0, 3), "\x48\x0f\xa4\x08\x03", 5);
			Check(IntelAssembler::shld<64>(Param::rax, Param::rcx, Param::cl), "\x48\x0f\xa5\xc8", 4);
		}

		TEST_METHOD(shr)
		{
			Check(IntelAssembler::shr<16>(Param::al, Param::imm8, 0, 1), "\xd0\xe8", 2);
		}

		TEST_METHOD(shrd)
		{
			Check(IntelAssembler::shrd<16>(Param::ax, Param::cx, Param::imm8, 0, 3), "\x0f\xac\xc8\x03", 4);
			Check(IntelAssembler::shrd<16>(Param::ax, Param::cx, Param::cl), "\x0f\xad\xc8", 3);
		}

		TEST_METHOD(sidt)
		{
			Check(IntelAssembler::sidt<32>(Param::m_eax), "\x0f\x01\x08", 3);
		}

		TEST_METHOD(sldt)
		{
			Check(IntelAssembler::sldt<32>(Param::m_eax), "\x0f\x00\x00", 3);
		}

		TEST_METHOD(smsw)
		{
			Check(IntelAssembler::smsw<32>(Param::m_eax), "\x0f\x01\x20", 3);
		}

		TEST_METHOD(str)
		{
			Check(IntelAssembler::str<32>(Param::m_eax), "\x0f\x00\x08", 3);
		}

	};
}
