#pragma once
#ifndef DEV_DISASM_WINDOW_H
#define DEV_DISASM_WINDOW_H

#include <mutex>
#include <string>

#include "Utils/Globals.h"
#include "Utils/ImGuiUtils.h"
#include "Ui/BaseWindow.h"

#include "..\Devector\Hardware.h"

namespace dev
{
	static constexpr int DEFAULT_WINDOW_W = 600;
	static constexpr int DEFAULT_WINDOW_H = 800;

	// disasm background colors
	const ImU32 DISASM_TBL_BG_COLOR_BRK = dev::IM_U32(0x353636FF);
	const ImU32 DISASM_TBL_BG_COLOR_ADDR = dev::IM_U32(0x353636FF);
	//const ImU32 DISASM_TBL_BG_COLOR_LABELS = dev::IM_U32(0x282828FF);
	//const ImU32 DISASM_TBL_BG_COLOR_COMMENT = dev::IM_U32(0x383838FF);

	// disasm text colors
	const ImVec4 DISASM_TBL_COLOR_COMMENT = dev::IM_VEC4(0x909090FF);
	const ImVec4 DISASM_TBL_COLOR_LABEL_GLOBAL = dev::IM_VEC4(0xD0C443FF);
	const ImVec4 DISASM_TBL_COLOR_LABEL_LOCAL = dev::IM_VEC4(0xA8742FFF);
	const ImVec4 DISASM_TBL_COLOR_LABEL_MINOR = dev::IM_VEC4(0x909090FF);
	const ImVec4 DISASM_TBL_COLOR_ADDR = dev::IM_VEC4(0x909090FF);
	const ImVec4 DISASM_TBL_COLOR_MNEMONIC = dev::IM_VEC4(0x578DDFFF);
	const ImVec4 DISASM_TBL_COLOR_NUMBER = dev::IM_VEC4(0xD4D4D4FF);
	const ImVec4 DISASM_TBL_COLOR_REG = dev::IM_VEC4(0x1ECF44FF);
	const ImVec4 DISASM_TBL_COLOR_CONST = dev::IM_VEC4(0x8BE0E9FF);
	// disasm icons colors
	const ImU32 DISASM_TBL_COLOR_BREAKPOINT = dev::IM_U32(0xFF2828C0);
	const ImU32 DISASM_TBL_COLOR_PC = dev::IM_U32(0x88F038FF);

	class DisasmWindow : public BaseWindow
	{	
		// Set column widths
		static constexpr float BRK_W = 20;
		static constexpr float ADDR_W = 50.0f;
		static constexpr float CODE_W = 200.0f;
		static constexpr float COMMAND_W = 120.0f;
		static constexpr float STATS_W = 100.0f;

		//static constexpr int DISASM_LINES_VISIBLE_MAX = 30;
		//static constexpr int DISASM_LINES_MAX = 17 * 10;
		
		Hardware& m_hardware;
		ImFont* m_fontComment = nullptr;

		int m_scrollLineOffset = 0;

		char m_searchText[255] = "";

		std::vector<std::string> m_disasm;
		/*
		// TODO: make m_disasm empty
			= {
			"first_label:\t(minor_label1, minor_label2)",
			"; Some comment",
			"0xFFFF\tADD H\t1, 10, 99\t",
			"@local_label:",
			"0x0000\tMOV A C\t1, 10, 99\tSPR_H",
			"0x0002\tORA E\t1, 1, 9\t",
			"0x0003\tLXI SP STACK_ADDR=0xFFFF\t0, 0, 0\t",
			"0x0004\tCALL draw_sprite=0x0150\t0, 0, 0\t",
			"0x0005\tDB WEAPON_MIN=0x20\t0, 0, 9\t",
			"0x0100\tPUSH B\t11, 0, 9\t",
			"; bla-bla-bla",
			"0x010A\tRET\t1, 0, 0\t",
			"func_start:\t(some_label)",
			"; comment 1"
			"0x1456\tSHLD HERO_POS=0x3241\t1, 10, 99\t",
			"@loop:\t(func_draw_something2 func_draw_something3)",
			"0xFE0D\tMVI b SOME_CONST=0x99\t1, 10, 99\tOTHER_SPR_H",
			"first_label:\t(minor_label1, minor_label2)",
			"; Some comment",
			"0xFFFF\tADD H\t1, 10, 99\t",
			"@local_label:",
			"0x0000\tMOV A C\t1, 10, 99\tSPR_H",
			"0x0002\tORA E\t1, 1, 9\t",
			"0x0003\tLXI SP STACK_ADDR=0xFFFF\t0, 0, 0\t",
			"0x0004\tCALL draw_sprite=0x0150\t0, 0, 0\t",
			"0x0005\tDB WEAPON_MIN=0x20\t0, 0, 9\t",
			"0x0100\tPUSH B\t11, 0, 9\t",
			"; bla-bla-bla",
			"0x010A\tRET\t1, 0, 0\t",
			"func_start:\t(some_label)",
			"; comment 1"
			"0x1456\tSHLD HERO_POS=0x3241\t1, 10, 99\t",
			"@loop:\t(func_draw_something2 func_draw_something3)",
			"0xFE0D\tMVI b SOME_CONST=0x99\t1, 10, 99\tOTHER_SPR_H",
			"first_label:\t(minor_label1, minor_label2)",
			"; Some comment",
			"0xFFFF\tADD H\t1, 10, 99\t",
			"@local_label:",
			"0x0000\tMOV A C\t1, 10, 99\tSPR_H",
			"0x0002\tORA E\t1, 1, 9\t",
			"0x0003\tLXI SP STACK_ADDR=0xFFFF\t0, 0, 0\t",
			"0x0004\tCALL draw_sprite=0x0150\t0, 0, 0\t",
			"0x0005\tDB WEAPON_MIN=0x20\t0, 0, 9\t",
			"0x0100\tPUSH B\t11, 0, 9\t",
			"; bla-bla-bla",
			"0x010A\tRET\t1, 0, 0\t",
			"func_start:\t(some_label)",
			"; comment 1"
			"0x1456\tSHLD HERO_POS=0x3241\t1, 10, 99\t",
			"@loop:\t(func_draw_something2 func_draw_something3)",
			"0xFE0D\tMVI b SOME_CONST=0x99\t1, 10, 99\tOTHER_SPR_H",
			"first_label:\t(minor_label1, minor_label2)",
			"; Some comment",
			"0xFFFF\tADD H\t1, 10, 99\t",
			"@local_label:",
			"0x0000\tMOV A C\t1, 10, 99\tSPR_H",
			"0x0002\tORA E\t1, 1, 9\t",
			"0x0003\tLXI SP STACK_ADDR=0xFFFF\t0, 0, 0\t",
			"0x0004\tCALL draw_sprite=0x0150\t0, 0, 0\t",
			"0x0005\tDB WEAPON_MIN=0x20\t0, 0, 9\t",
			"0x0100\tPUSH B\t11, 0, 9\t",
			"; bla-bla-bla",
			"0x010A\tRET\t1, 0, 0\t",
			"func_start:\t(some_label)",
			"; comment 1"
			"0x1456\tSHLD HERO_POS=0x3241\t1, 10, 99\t",
			"@loop:\t(func_draw_something2 func_draw_something3)",
			"0xFE0D\tMVI b SOME_CONST=0x99\t1, 10, 99\tOTHER_SPR_H",
			"first_label:\t(minor_label1, minor_label2)",
			"; Some comment",
			"0xFFFF\tADD H\t1, 10, 99\t",
			"@local_label:",
			"0x0000\tMOV A C\t1, 10, 99\tSPR_H",
			"0x0002\tORA E\t1, 1, 9\t",
			"0x0003\tLXI SP STACK_ADDR=0xFFFF\t0, 0, 0\t",
			"0x0004\tCALL draw_sprite=0x0150\t0, 0, 0\t",
			"0x0005\tDB WEAPON_MIN=0x20\t0, 0, 9\t",
			"0x0100\tPUSH B\t11, 0, 9\t",
			"; bla-bla-bla",
			"0x010A\tRET\t1, 0, 0\t",
			"func_start:\t(some_label)",
			"; comment 1"
			"0x1456\tSHLD HERO_POS=0x3241\t1, 10, 99\t",
			"@loop:\t(func_draw_something2 func_draw_something3)",
			"0xFE0D\tMVI b SOME_CONST=0x99\t1, 10, 99\tOTHER_SPR_H",
			"first_label:\t(minor_label1, minor_label2)",
			"; Some comment",
			"0xFFFF\tADD H\t1, 10, 99\t",
			"@local_label:",
			"0x0000\tMOV A C\t1, 10, 99\tSPR_H",
			"0x0002\tORA E\t1, 1, 9\t",
			"0x0003\tLXI SP STACK_ADDR=0xFFFF\t0, 0, 0\t",
			"0x0004\tCALL draw_sprite=0x0150\t0, 0, 0\t",
			"0x0005\tDB WEAPON_MIN=0x20\t0, 0, 9\t",
			"0x0100\tPUSH B\t11, 0, 9\t",
			"; bla-bla-bla",
			"0x010A\tRET\t1, 0, 0\t",
			"func_start:\t(some_label)",
			"; comment 1"
			"0x1456\tSHLD HERO_POS=0x3241\t1, 10, 99\t",
			"@loop:\t(func_draw_something2 func_draw_something3)",
			"0xFE0D\tMVI b SOME_CONST=0x99\t1, 10, 99\tOTHER_SPR_H",
			"first_label:\t(minor_label1, minor_label2)",
			"; Some comment",
			"0xFFFF\tADD H\t1, 10, 99\t",
			"@local_label:",
			"0x0000\tMOV A C\t1, 10, 99\tSPR_H",
			"0x0002\tORA E\t1, 1, 9\t",
			"0x0003\tLXI SP STACK_ADDR=0xFFFF\t0, 0, 0\t",
			"0x0004\tCALL draw_sprite=0x0150\t0, 0, 0\t",
			"0x0005\tDB WEAPON_MIN=0x20\t0, 0, 9\t",
			"0x0100\tPUSH B\t11, 0, 9\t",
			"; bla-bla-bla",
			"0x010A\tRET\t1, 0, 0\t",
			"func_start:\t(some_label)",
			"; comment 1"
			"0x1456\tSHLD HERO_POS=0x3241\t1, 10, 99\t",
			"@loop:\t(func_draw_something2 func_draw_something3)",
			"0xFE0D\tMVI b SOME_CONST=0x99\t1, 10, 99\tOTHER_SPR_H",
			"first_label:\t(minor_label1, minor_label2)",
			"; Some comment",
			"0xFFFF\tADD H\t1, 10, 99\t",
			"@local_label:",
			"0x0000\tMOV A C\t1, 10, 99\tSPR_H",
			"0x0002\tORA E\t1, 1, 9\t",
			"0x0003\tLXI SP STACK_ADDR=0xFFFF\t0, 0, 0\t",
			"0x0004\tCALL draw_sprite=0x0150\t0, 0, 0\t",
			"0x0005\tDB WEAPON_MIN=0x20\t0, 0, 9\t",
			"0x0100\tPUSH B\t11, 0, 9\t",
			"; bla-bla-bla",
			"0x010A\tRET\t1, 0, 0\t",
			"func_start:\t(some_label)",
			"; comment 1"
			"0x1456\tSHLD HERO_POS=0x3241\t1, 10, 99\t",
			"@loop:\t(func_draw_something2 func_draw_something3)",
			"0xFE0D\tMVI b SOME_CONST=0x99\t1, 10, 99\tOTHER_SPR_H",
			"first_label:\t(minor_label1, minor_label2)",
			"; Some comment",
			"0xFFFF\tADD H\t1, 10, 99\t",
			"@local_label:",
			"0x0000\tMOV A C\t1, 10, 99\tSPR_H",
			"0x0002\tORA E\t1, 1, 9\t",
			"0x0003\tLXI SP STACK_ADDR=0xFFFF\t0, 0, 0\t",
			"0x0004\tCALL draw_sprite=0x0150\t0, 0, 0\t",
			"0x0005\tDB WEAPON_MIN=0x20\t0, 0, 9\t",
			"0x0100\tPUSH B\t11, 0, 9\t",
			"; bla-bla-bla",
			"0x010A\tRET\t1, 0, 0\t",
			"func_start:\t(some_label)",
			"; comment 1"
			"0x1456\tSHLD HERO_POS=0x3241\t1, 10, 99\t",
			"@loop:\t(func_draw_something2 func_draw_something3)",
			"0xFE0D\tMVI b SOME_CONST=0x99\t1, 10, 99\tOTHER_SPR_H",
			"first_label:\t(minor_label1, minor_label2)",
			"; Some comment",
			"0xFFFF\tADD H\t1, 10, 99\t",
			"@local_label:",
			"0x0000\tMOV A C\t1, 10, 99\tSPR_H",
			"0x0002\tORA E\t1, 1, 9\t",
			"0x0003\tLXI SP STACK_ADDR=0xFFFF\t0, 0, 0\t",
			"0x0004\tCALL draw_sprite=0x0150\t0, 0, 0\t",
			"0x0005\tDB WEAPON_MIN=0x20\t0, 0, 9\t",
			"0x0100\tPUSH B\t11, 0, 9\t",
			"; bla-bla-bla",
			"0x010A\tRET\t1, 0, 0\t",
			"func_start:\t(some_label)",
			"; comment 1"
			"0x1456\tSHLD HERO_POS=0x3241\t1, 10, 99\t",
			"@loop:\t(func_draw_something2 func_draw_something3)",
			"0xFE0D\tLAST b SOME_CONST=0x99\t1, 10, 99\tOTHER_SPR_H",
		};
		*/
		void DrawDebugControls();
		void DrawSearch();
		void DrawCode();
		void DrawDisassembly();

	public:

		DisasmWindow(Hardware& _hardware, ImFont* fontComment, const float const* _fontSizeP, const float const* _dpiScaleP);
		void Update();
		void UpdateDisasm();
	};

};

#endif // !DEV_DISASM_WINDOW_H