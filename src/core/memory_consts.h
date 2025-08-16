#include "utils/types.h"

enum class AddrSpace : uint8_t { RAM = 0, STACK = 1 };
enum class MemType : uint8_t { ROM = 0, RAM };

static constexpr dev::GlobalAddr ROM_LOAD_ADDR = 0x100;

static constexpr size_t MEM_64K = 64 * 1024;
static constexpr size_t RAM_DISK_PAGE_LEN = MEM_64K;
static constexpr size_t RAMDISK_PAGES_MAX = 4;
static constexpr size_t MEMORY_RAMDISK_LEN = RAMDISK_PAGES_MAX * MEM_64K;
static constexpr size_t RAM_DISK_MAX = 8;

static constexpr size_t MEMORY_MAIN_LEN = MEM_64K;

static constexpr size_t MEMORY_GLOBAL_LEN =
								MEMORY_MAIN_LEN +
								MEMORY_RAMDISK_LEN * RAM_DISK_MAX;

static constexpr uint8_t MAPPING_RAM_MODE_MASK = 0b11100000;
static constexpr uint8_t MAPPING_MODE_MASK = 0b11110000;