#pragma once
#ifndef DEV_BREAKPOINTS_H
#define DEV_BREAKPOINTS_H

#include <memory>
#include <mutex>
#include <map>
#include <vector>
#include <format>

#include "Utils/Types.h"

namespace dev
{
	class Breakpoint
	{
	public:
		static constexpr uint8_t MAPPING_RAM		   = 1 << 0;
		static constexpr uint8_t MAPPING_RAMDISK_PAGE0 = 1 << 1;
		static constexpr uint8_t MAPPING_RAMDISK_PAGE1 = 1 << 2;
		static constexpr uint8_t MAPPING_RAMDISK_PAGE2 = 1 << 3;
		static constexpr uint8_t MAPPING_RAMDISK_PAGE3 = 1 << 4;
		static constexpr uint8_t MAPPING_PAGES_ALL =  // 43210R, R - ram, 0 - ram disk 0 page, 1 - ram disk 1 page, etc
											MAPPING_RAM | 
											MAPPING_RAMDISK_PAGE0 | 
											MAPPING_RAMDISK_PAGE1 |
											MAPPING_RAMDISK_PAGE2 |
											MAPPING_RAMDISK_PAGE3;
		enum class Status : int {
			DISABLED = 0,
			ACTIVE,
			DELETED,
		};
		struct Data {
			Data(const Addr _addr,
				const uint8_t _mappingPageRam = MAPPING_PAGES_ALL,
				const Status _status = Status::ACTIVE, const bool _autoDel = false)
				:
				addr(_addr), mappingPages(_mappingPageRam),
				status(_status), autoDel(_autoDel)
			{}
			Addr addr : 16;
			uint8_t mappingPages : 5; // 3210R, R - ram, 0 - ram disk 0 page, 1 - ram disk 1 page, etc
			Status status : 2;
			bool autoDel : 1;
			
			auto GetAddrS() const -> std::string { return std::format("0x{:04X}", addr); };
			auto GetAddrMappingS() const -> const char*;
			inline bool IsActive() const { return status == Breakpoint::Status::ACTIVE; };
		};

		Breakpoint(const Addr _addr,
			const uint8_t _mappingPages = MAPPING_PAGES_ALL,
			const Status _status = Status::ACTIVE, 
			const bool _autoDel = false, const std::string& _comment = "");
		void Update(const Addr _addr,
			const uint8_t _mappingPages = MAPPING_PAGES_ALL,
			const Status _status = Status::ACTIVE,
			const bool _autoDel = false, const std::string& _comment = "");
		Data GetData() const { return m_data; };
		bool CheckStatus(const uint8_t _mappingModeRam, const uint8_t _mappingPageRam) const;
		void SetStatus(const Status _status) { m_data.status = _status; };
		//auto GetConditionS() const -> std::string;
		auto GetComment() const -> const std::string& { return m_comment; };
		void Print() const;

	private:
		Data m_data;
		std::string m_comment;

		auto IsActiveS() const -> const char*;
	};
}
#endif // !DEV_BREAKPOINTS_H