namespace Devector
{
	public static class Consts
	{
		public const int INVALID_ID = -1;

		public enum ErrCode
		{
			NO_ERRORS = 0,
			UNSPECIFIED = -1,
			NO_FILES,
			FAILED_SDL_INIT,
			FAILED_CREATION_WINDOW,
			FAILED_SDL_GET_DISPLAY_BOUNDS,
			FAILED_OPENGL_INIT,
			UNRECOGNIZED_CPU_INSTR,
			INVALID_ID,
		}
	}
}
