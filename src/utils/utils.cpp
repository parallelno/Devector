#include <cstdarg>
#include <fstream>
#include <iostream>

#if defined(_WIN32)
	#include <Windows.h>
	#include <commdlg.h>
#elif defined(__linux__)
	#include <gtk/gtk.h>
#endif

#include "utils/utils.h"
#include "utils/consts.h"
#include "utils/str_utils.h"
#include "utils/result.h"

void dev::RunApp(const std::wstring& _dir, const std::wstring& _appName) 
{
	if ( !IsFileExist(_dir + _appName) )
	{
		return;
	}
	const std::wstring command = L"cd " + _dir + L" && " + _appName;
	system(dev::StrWToStr(command).c_str());
}

void dev::OsOpenInShell(const char* path)
{
#ifdef _WIN32
	// Note: executable path must use backslashes!
	::ShellExecuteA(NULL, "open", path, NULL, NULL, SW_SHOWDEFAULT);
#else
#if __APPLE__
	const char* open_executable = "open";
#else
	const char* open_executable = "xdg-open";
#endif
	char command[256];
	snprintf(command, 256, "%s \"%s\"", open_executable, path);
	system(command);
#endif
}

void dev::ThreadSleep(double _seconds) 
{
	std::this_thread::sleep_for(std::chrono::milliseconds((long long)(_seconds * 1000.0)));
}

void dev::CopyToClipboard(const std::string& _str) {
#if defined(_WIN32)
		if (OpenClipboard(nullptr)) {
			HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, _str.size() + 1);
			if (hMem) {
				memcpy(GlobalLock(hMem), _str.c_str(), _str.size() + 1);
				GlobalUnlock(hMem);
				EmptyClipboard();
				SetClipboardData(CF_TEXT, hMem);
				GlobalFree(hMem);
			}
			CloseClipboard();
		}
#elif defined(__linux__)
		std::ofstream("/dev/clipboard") << _str;
#endif
	}

auto dev::LoadTextFile(const std::wstring& _path)
-> std::string
{	
	if (!IsFileExist(_path)) {
		return {};
	}
	std::ifstream ifs(_path);
	std::string data = std::string(std::istreambuf_iterator<char>(ifs),
		std::istreambuf_iterator<char>());

	return data;
}

auto dev::LoadFile(const std::wstring& _path)
-> Result<std::vector<uint8_t>>
{
	if (!IsFileExist(_path)) {
		return {};
	}

	// Open the file in binary mode
	std::ifstream file(_path, std::ios::binary);

	// Check if the file is opened successfully
	if (!file.is_open()) {
		std::wcerr << L"Failed to open file: " << _path << std::endl;
		return false;
	}

	std::vector<uint8_t> data(std::istreambuf_iterator<char>(file), {});

	return { std::move(data) };
}

bool dev::SaveFile(const std::wstring& _path, const std::vector<uint8_t>& _data, const bool _override)
{
	std::ofstream file{_path, std::ios::binary | std::ios::ate};

	if (!file)
	{
		dev::Log(L"Failed to init a file object: {}", _path);
		return false;	
	}
	else if ((!_override && std::filesystem::exists(_path)))
	{
		dev::Log(L"Error. Failed to save file. File is already exist: {}", _path);
		return false;
	}

	file.write(reinterpret_cast<const char*>(_data.data()), _data.size());
	return true;
}

void dev::DeleteFiles(const std::wstring& _dir, const std::wstring& _mask)
{
	const std::wstring command = L"del /Q " + _dir + _mask;
	system(dev::StrWToStr(command).c_str());
}

size_t dev::GetFileSize(const std::wstring& _path)
{
	std::filesystem::path p{ _path };
	return std::filesystem::file_size(p);
}

auto dev::GetDir(const std::wstring& _path)
->std::wstring
{
	std::filesystem::path p{ _path };
	return p.parent_path().wstring();
}

auto dev::GetFilename(const std::wstring& _path)
->std::wstring
{
	std::filesystem::path p{ _path };
	return p.stem().wstring();
}

auto dev::GetExt(const std::wstring& _path)
->std::wstring
{
	std::filesystem::path p{ _path };
	return p.extension().wstring();
}

auto dev::GetDirStemExt(const std::wstring& _path)
-> std::tuple<std::wstring, std::wstring, std::wstring>
{
	std::filesystem::path p{ _path };
	return std::make_tuple(p.parent_path().wstring(), p.stem().wstring(), p.extension().wstring());
}

// return true if the user selected a file
// return false if the user canceled the dialog or an error occurred
bool dev::FileDialog(wchar_t* _filePath, int _size, const wchar_t* _filter, FILE_DIALOG _dialog)
{
	bool isSaveDialog = _dialog == FILE_DIALOG::SAVE;

#if defined(_WIN32)
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = _filePath;
    ofn.lpstrFile[0] = L'\0';
    ofn.nMaxFile = _size;
    ofn.lpstrFilter = _filter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    ofn.lpstrTitle = isSaveDialog ? L"Save File" : L"Open File";
    ofn.lpstrInitialDir = NULL;

    if (isSaveDialog) {
        ofn.Flags |= OFN_OVERWRITEPROMPT;
        return GetSaveFileNameW(&ofn) == TRUE;
    } else {
        return GetOpenFileNameW(&ofn) == TRUE;
    }

#elif defined(__linux__)
    static bool gtk_initialized = false;
    if (!gtk_initialized) {
        gtk_init(NULL, NULL);
        gtk_initialized = true;
    }

    GtkWidget *dialog;
    GtkFileChooserAction action = isSaveDialog ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN;
    gint res;

    dialog = gtk_file_chooser_dialog_new(isSaveDialog ? "Save File" : "Open File",
                                         NULL,
                                         action,
                                         "_Cancel",
                                         GTK_RESPONSE_CANCEL,
                                         isSaveDialog ? "_Save" : "_Open",
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);

    // Set up file filters
    if (_filter != NULL) {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
        std::string filter = conv.to_bytes(_filter);
        std::vector<std::string> filter_parts;
        size_t pos = 0;
        while ((pos = filter.find('|')) != std::string::npos) {
            filter_parts.push_back(filter.substr(0, pos));
            filter.erase(0, pos + 1);
        }
        filter_parts.push_back(filter);

        for (size_t i = 0; i < filter_parts.size(); i += 2) {
            GtkFileFilter *gtk_filter = gtk_file_filter_new();
            gtk_file_filter_set_name(gtk_filter, filter_parts[i].c_str());
            gtk_file_filter_add_pattern(gtk_filter, filter_parts[i + 1].c_str());
            gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), gtk_filter);
        }
    }

    if (isSaveDialog) {
        gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
    }

    res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT) {
        char *filename;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        filename = gtk_file_chooser_get_filename(chooser);
        
        std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
        std::wstring wfilename = conv.from_bytes(filename);
        wcsncpy(_filePath, wfilename.c_str(), _size - 1);
        _filePath[_size - 1] = L'\0';
        
        g_free(filename);
        gtk_widget_destroy(dialog);
        return true;
    }

    gtk_widget_destroy(dialog);
    return false;

#else
    // Other platforms implementation (if needed)
    return false;
#endif
}
