#pragma once

#include <cstdint>
#include <map>
#include <functional>

#include "Utils/Types.h"
#include "Core/Memory.h"

namespace dev
{
	class Keyboard
	{
	private:
		uint8_t m_encodingMatrix[8];
		using KeyCode = int;
		using RowColumnCode = int;
		std::map<KeyCode, RowColumnCode> m_keymap;

	public:
		enum class Operation {
			NONE = 0,
			RESET,
			RESTART
		};
		bool m_keySS = false;
		bool m_keyUS = false;
		bool m_keyRus = false;
		bool m_terminate = false;
		Operation m_rebootType = Operation::NONE;

		Keyboard();
		auto KeyHandling(int _key, int _action) -> Operation;
		auto Read(int _rows) -> uint8_t;

	private:
		void InitMapping();
	};
}