namespace Devector
{
	public static class Consts
	{
		public const string DEFAULT_SETTING_PATH = "settings.json";

        public const int INVALID_ID = -1;

		public enum ErrCode
		{
            UNSPECIFIED = -1,
            NO_ERRORS = 0,
			NO_FILES,
			FAILED_SDL_INIT,
			FAILED_CREATION_WINDOW,
			FAILED_SDL_GET_DISPLAY_BOUNDS,
			FAILED_OPENGL_INIT,
			UNRECOGNIZED_CPU_INSTR,
            WARNING_FDD_IMAGE_TOO_BIG,
        }


        public const int FDD_SIDES = 2;
        public const int FDD_TRACKS_PER_SIDE = 82;
        public const int FDD_SECTORS_PER_TRACK = 5;
        public const int FDD_SECTOR_LEN = 1024;
        public const int FDD_SIZE = FDD_SIDES * FDD_TRACKS_PER_SIDE * FDD_SECTORS_PER_TRACK * FDD_SECTOR_LEN;
    }
}
