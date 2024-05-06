#pragma once

#include <vector>
#include <tuple>
#include <string>
#include <unordered_set>

//#include "util.h"

struct MDHeader
{
	struct MDHeaderData
	{
    uint8_t User;
    char Name[8];
    char Ext[3];
    uint8_t Extent;
    uint8_t Unknown1;
    uint8_t Unknown2;
    uint8_t Records;
    uint16_t FAT[8];
} __attribute__((packed));

	MDHeaderData* fields;
	uint8_t* bytes;

	static constexpr size_t SIZE = sizeof(MDHeaderData);

	uint8_t user() const { return fields->User; }
	auto name() const -> std::wstring;
	auto ext() const -> std::wstring;

	MDHeader(uint8_t* _bytes)
		: fields(reinterpret_cast< MDHeaderData* >(_bytes)), 
		bytes(_bytes)
	{}

	void init_with_filename(std::wstring stem, std::wstring ext);
	void overwrite(const MDHeader & other);
	bool operator==(const MDHeader & rhs) const;
	bool operator!=(const MDHeader & rhs) const;
};

struct Dirent
{
	MDHeader header;
	std::vector<uint16_t> chain;
	int size;

	Dirent() :header(nullptr), size(0) {}
	Dirent(MDHeader header) : header(header), size(0) { }
	Dirent(const Dirent& rhs)
		: header(rhs.header), chain(rhs.chain), size(rhs.size) {
	}

	auto name() const -> std::wstring { return header.name(); }
	auto ext() const -> std::wstring { return header.ext(); }
	auto user() const -> uint8_t { return header.user(); }
};

class FileSystemImage {
	//friend struct FilesystemTest;

	using bytes_t = std::vector<uint8_t>;
	using chain_t = std::vector<uint16_t>;

	std::unordered_set<std::wstring> taken_names;
	bytes_t bytes;

	chain_t build_available_chain(int length);

	auto allocate_file(const std::wstring & filename,
		const bytes_t& content, const chain_t& chain)
		-> std::wstring;

	std::tuple<int,int,int> cluster_to_ths(int cluster) const;
	bytes_t::iterator map_sector(int track, int head, int sector);
	// load full file information and sector map
	Dirent load_dirent(MDHeader header);
	bytes_t read_bytes(const Dirent & de);

	auto unique_cpm_filename(const std::wstring & filename)
		-> std::tuple<std::wstring, std::wstring>;

public:
	static constexpr int SECTOR_SIZE = 1024;
	static constexpr int SECTORS_CYL = 10;
	static constexpr int MAX_FS_BYTES = 860160;
	static constexpr int CLUSTER_BYTES = 2048;
	static constexpr int SYSTEM_TRACKS = 6;

	struct dir_iterator {
		FileSystemImage & fs;
		int position;

		dir_iterator(FileSystemImage & fs, int position)
			: fs(fs), position(position) {
		}

		MDHeader operator*() const {
			return MDHeader(&fs.bytes[position]);
		}

		const dir_iterator& operator++() { // prefix
			position += 32;
			return *this;
		}

		bool operator==(const dir_iterator& rhs) {
			return position == rhs.position;
		}

		bool operator!=(const dir_iterator& rhs) {
			return position != rhs.position;
		}
	};

	FileSystemImage() : FileSystemImage(0) {}

	FileSystemImage(int size) : bytes(size, 0xe5) {}

	FileSystemImage(const bytes_t & data) : bytes(data)
	{
	}

	bool mount_local_dir(std::wstring path);

	void set_data(const bytes_t & data);
	bytes_t& data() { return bytes; }

	dir_iterator begin();
	dir_iterator end();

	Dirent find_file(const std::wstring& filename);
	void listdir(std::function<void(const Dirent &)> cb);

	bytes_t read_file(const std::wstring& filename);

	// return ok, internal file name
	auto save_file(const std::wstring& filename, bytes_t content)
		-> std::tuple<bool,std::wstring>;
};

