#pragma once
#ifndef DEV_GLOBALS_H
#define DEV_GLOBALS_H

#include <map>
#include <tuple>
#include <vector>
#include <string>
#include <memory>

namespace dev
{
	using ErrCode = int;

	constexpr static ErrCode NO_ERRORS = 0;
	constexpr static ErrCode ERROR_UNSPECIFIED = 1;
	constexpr static ErrCode ERROR_NO_FILES = 2;

	static constexpr int REQ_DISASM_NONE = 0;
	static constexpr int REQ_DISASM_DRAW = 1; // redraw disasm
	static constexpr int REQ_DISASM_BRK = 2; // redraw disasm at the prodived addr in m_reqDisasmUpdateData
}

#endif // !DEV_GLOBALS_H