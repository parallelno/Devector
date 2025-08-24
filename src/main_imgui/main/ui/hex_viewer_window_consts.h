#pragma once

namespace dev
{
	namespace HexViewer
	{

		static constexpr int PAGES_MAX = 33;

		using PageNames = std::array<const char*, PAGES_MAX>;

		static constexpr PageNames page_names = {
				"Main Ram",
				"RAM Disk1 Bank0", "RAM Disk1 Bank1", "RAM Disk1 Bank2", "RAM Disk1 Bank3",
				"RAM Disk2 Bank0", "RAM Disk2 Bank1", "RAM Disk2 Bank2", "RAM Disk2 Bank3",
				"RAM Disk3 Bank0", "RAM Disk3 Bank1", "RAM Disk3 Bank2", "RAM Disk3 Bank3",
				"RAM Disk4 Bank0", "RAM Disk4 Bank1", "RAM Disk4 Bank2", "RAM Disk4 Bank3",
				"RAM Disk5 Bank0", "RAM Disk5 Bank1", "RAM Disk5 Bank2", "RAM Disk5 Bank3",
				"RAM Disk6 Bank0", "RAM Disk6 Bank1", "RAM Disk6 Bank2", "RAM Disk6 Bank3",
				"RAM Disk7 Bank0", "RAM Disk7 Bank1", "RAM Disk7 Bank2", "RAM Disk7 Bank3",
				"RAM Disk8 Bank0", "RAM Disk8 Bank1", "RAM Disk8 Bank2", "RAM Disk8 Bank3",
		};


		static constexpr std::array<const char*, 16> col_names1 = {
				"00", "01", "02", "03", "04", "05", "06", "07",
				"08", "09", "0A", "0B", "0C", "0D", "0E", "0F"
		};
		static constexpr std::array<const char*, 16> col_names2 = {
				"0", "1", "2", "3", "4", "5", "6", "7",
				"8", "9", "A", "B", "C", "D", "E", "F"
		};
	};
};