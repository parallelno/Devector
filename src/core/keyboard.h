#pragma once

#include <cstdint>
#include <unordered_map>
#include <functional>

#include <SDL3/SDL.h>

#include "utils/types.h"
#include "core/memory.h"

namespace dev
{
	class Keyboard
	{
	private:
		uint8_t m_encodingMatrix[8];
		using KeyCode = int;
		using RowColumnCode = int;
		std::unordered_map<KeyCode, RowColumnCode> m_keymap;

	public:
		enum class Operation {
			NONE = 0,
			RESET,
			RESTART
		};
		bool m_keySS = false;
		bool m_keyUS = false;
		bool m_keyRus = false;
		Operation m_rebootType = Operation::NONE;

		Keyboard();

		void KeyHandling(int _scancode, int _action);
		auto Read(int _rows) -> uint8_t;

	private:
		void InitMapping();
	};
}