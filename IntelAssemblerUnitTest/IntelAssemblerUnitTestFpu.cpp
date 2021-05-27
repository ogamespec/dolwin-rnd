#include "pch.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace IntelCore;

namespace IntelAssemblerUnitTest
{
	TEST_CLASS(IntelAssemblerUnitTestFpu)
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

	public:

		TEST_METHOD(fld)
		{
			Check(IntelAssembler::fld<16>(Param::m_bp_di, PtrHint::DwordPtr), "\xd9\x03", 2);
			Check(IntelAssembler::fld<16>(Param::m_bp_di, PtrHint::QwordPtr), "\xdd\x03", 2);
			Check(IntelAssembler::fld<16>(Param::m_bp_di, PtrHint::XwordPtr), "\xdb\x2b", 2);
			Check(IntelAssembler::fld<32>(Param::m_eax, PtrHint::DwordPtr), "\xd9\x00", 2);
			Check(IntelAssembler::fld<32>(Param::m_bp_di, PtrHint::DwordPtr), "\x67\xd9\x03", 3);
			Check(IntelAssembler::fld<64>(Param::m_rax, PtrHint::DwordPtr), "\xd9\x00", 2);
			Check(IntelAssembler::fld<16>(Param::st1), "\xd9\xc1", 2);
		}

	};
}
