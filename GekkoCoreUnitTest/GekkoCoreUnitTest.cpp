// This module contains only basic tests.
#include "pch.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GekkoCoreUnitTest
{
	TEST_CLASS(GekkoCoreUnitTest)
	{
		void FixCWD()
		{
			char path[0x100] = { 0 };
			GetCurrentDirectoryA(sizeof(path), path);
			std::string newpath = (std::string(path) + "/../../../../dolwin");
			SetCurrentDirectoryA(newpath.c_str());
		}

	public:
		
		TEST_METHOD(GekkoCoreInstance)
		{
			FixCWD();

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

	};
}
