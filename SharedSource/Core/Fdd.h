#pragma once

static constexpr int FDD_SIDES = 2;
static constexpr int FDD_TRACKS_PER_SIDE = 82;
static constexpr int FDD_SECTORS_PER_TRACK = 5;
static constexpr int FDD_SECTOR_LEN = 1024;
static constexpr int FDD_SIZE = FDD_SIDES * FDD_TRACKS_PER_SIDE * FDD_SECTORS_PER_TRACK * FDD_SECTOR_LEN;