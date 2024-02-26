#pragma once
#ifndef DEV_DEBUGGER_H
#define DEV_DEBUGGER_H

#include <memory>
#include <mutex>
#include <map>
#include <vector>

#include "I8080.h"
#include "Memory.h"

namespace dev
{
	class Debugger
	{
		/*
	public:
		enum MemAccess
		{
			RUN, READ, WRITE
		};

		static uint64_t mem_runs[Memory::GLOBAL_MEMORY_LEN];
		static uint64_t mem_reads[Memory::GLOBAL_MEMORY_LEN];
		static uint64_t mem_writes[Memory::GLOBAL_MEMORY_LEN];

		Hardware& m_hardware;
		std::map<int, std::vector<std::string>> labels;
		std::map<std::string, int> label_addrs;

		Debugger(Hardware& _hardware);
		void Init();

		uint16_t GetAddr(uint16_t _end_addr, int _before_addr_lines) const;
		
		auto GetDisasm(uint16_t _addr, int _lines, uint16_t _before_addr_lines) const
			-> const std::vector<std::string>;
			*/

	public:
		class Breakpoint
		{
		public:

			Breakpoint(const uint32_t _global_addr, const bool _active = true)
				: global_addr(_global_addr), active(_active)
			{}
			auto check() const -> const bool;
			auto is_active() const -> const bool;
			void print() const;

		private:
			uint32_t global_addr;
			bool active;
		};

		class Watchpoint
		{
		public:
			enum class Access : size_t { R = 0, W, RW, COUNT };
			static constexpr const char* access_s[] = { "R-", "-W", "RW" };
			static constexpr size_t VAL_BYTE_SIZE = sizeof(uint8_t);
			static constexpr size_t VAL_WORD_SIZE = sizeof(uint16_t);
			static constexpr size_t VAL_MAX_SIZE = VAL_WORD_SIZE;

			enum class Condition : size_t { ANY = 0, EQU, LESS, GREATER, LESS_EQU, GREATER_EQU, NOT_EQU, COUNT };
			static constexpr const char* conditions_s[] = { "ANY", "==", "<", ">", "<=", ">=", "!=" };

			Watchpoint(const Access _access, const uint32_t _global_addr, const Condition _cond, const uint16_t _value, const size_t _value_size = VAL_BYTE_SIZE, const bool _active = true)
				: access(static_cast<Watchpoint::Access>((size_t)_access % (size_t)Access::COUNT)), global_addr(_global_addr), cond(static_cast<Debugger::Watchpoint::Condition>((size_t)_cond& (size_t)Condition::COUNT)),
				value(_value & 0xffff), value_size(_value_size), active(_active), break_l(false), break_h(false)
			{}
			auto check(const Watchpoint::Access _access, const uint32_t _global_addr, const uint8_t _value) -> const bool;
			auto is_active() const -> const bool;
			auto get_global_addr() const -> const size_t;
			auto check_addr(const uint32_t _global_addr) const -> const bool;
			void reset();
			void print() const;

		private:
			Access access;
			uint32_t global_addr;
			Condition cond;
			uint16_t value;
			size_t value_size;
			bool active;
			bool break_l;
			bool break_h;
		};
		using Watchpoints = std::vector<Watchpoint>;

		//static const constexpr size_t MEM_BANK_SIZE = 0x10000;
		//static const constexpr size_t RAM_SIZE = MEM_BANK_SIZE;
		//static const constexpr size_t RAM_DISK_SIZE = MEM_BANK_SIZE * 4;
		//static const constexpr size_t GLOBAL_MEM_SIZE = RAM_SIZE + RAM_DISK_SIZE;
		static const constexpr size_t TRACE_LOG_SIZE = 100000;

		/*
		enum AddrSpace : size_t
		{
			CPU = 0,	// cpu adressed space. range: [0x0000, 0xffff]
			STACK,		// cpu adressed space via stack commands: xthl, push, pop. range: 0x0000 - 0xFFFF accessed 
			GLOBAL		// flat virtual addr space (ram + ram-disk). range: [0x0000, 0x4ffff]
		};
		*/
		Debugger(I8080& _cpu, Memory& _memory);
		void Init();

		void Read(const uint32_t _global_addr, Memory::AddrSpace _addrSpace, const uint8_t _val, const bool _is_opcode);
		void Write(const uint32_t _global_addr, Memory::AddrSpace _addrSpace, const uint8_t _val);

		auto get_disasm(const uint32_t _addr, const size_t _lines, const size_t _before_addr_lines) const->std::string;

		void add_breakpoint(const uint32_t _addr, const bool _active = true, const Memory::AddrSpace _addr_space = Memory::AddrSpace::RAM);
		void del_breakpoint(const uint32_t _addr, const Memory::AddrSpace _addr_space = Memory::AddrSpace::RAM);
		void add_watchpoint(const Watchpoint::Access _access, const uint32_t _addr, const Watchpoint::Condition _cond, const uint16_t _value, const size_t _value_size = 1, const bool _active = true, const Memory::AddrSpace _addr_space = Memory::AddrSpace::RAM);
		void del_watchpoint(const uint32_t _addr, const Memory::AddrSpace _addr_space = Memory::AddrSpace::RAM);
		void reset_watchpoints();
		void print_breakpoints();
		void print_watchpoints();
		//auto get_global_addr(size_t _addr, const AddrSpace _addr_space) const -> const size_t;
		bool check_breakpoints(const uint32_t _global_addr);
		bool check_watchpoint(const Watchpoint::Access _access, const uint32_t _global_addr, const uint8_t _value);
		bool check_break();
		auto get_trace_log(const int _offset, const size_t _lines, const size_t _filter) -> std::string;
		//void set_labels(const char* _labels_c);

	private:
		auto get_disasm_line(const uint32_t _addr, const uint8_t _opcode, const uint8_t _data_l, const uint8_t _data_h) const ->const std::string;
		auto get_disasm_db_line(const uint32_t _addr, const uint8_t _data) const ->const std::string;
		auto get_cmd_len(const uint8_t _addr) const -> const size_t;
		auto get_addr(const uint32_t _end_addr, const size_t _before_addr_lines) const->size_t;
		auto watchpoints_find(const uint32_t global_addr) -> Watchpoints::iterator;
		void watchpoints_erase(const uint32_t global_addr);

		void trace_log_update(const uint32_t _global_addr, const uint8_t _val);
		auto trace_log_next_line(const int _idx_offset, const bool _reverse, const size_t _filter) const ->int;
		auto trace_log_nearest_forward_line(const size_t _idx_offset, const size_t _filter) const ->int;

		uint64_t mem_runs[Memory::GLOBAL_MEMORY_LEN];
		uint64_t mem_reads[Memory::GLOBAL_MEMORY_LEN];
		uint64_t mem_writes[Memory::GLOBAL_MEMORY_LEN];

		struct TraceLog
		{
			int64_t global_addr;
			uint8_t opcode;
			uint8_t data_l;
			uint8_t data_h;

			auto to_str() const->std::string;
			void clear();
		};
		TraceLog trace_log[TRACE_LOG_SIZE];
		size_t trace_log_idx = 0;
		int trace_log_idx_view_offset = 0;

		std::map<size_t, std::string> labels;

		I8080& m_cpu;
		Memory m_memory;

		std::mutex breakpoints_mutex;
		std::mutex watchpoints_mutex;
		std::map<size_t, Breakpoint> breakpoints;
		Watchpoints watchpoints;
		bool wp_break;
	};
}
#endif // !DEV_DEBUGGER_H