#include "stdafx.h"
#include "FileZip.h"
#include <algorithm>
#include <string>

FileZip::FileZip(std::string zip)
	: m_uf(NULL)
{
	zlib_filefunc64_def ffunc;
	fill_win32_filefunc64A(&ffunc);
	m_uf = unzOpen2_64(zip.c_str(), &ffunc);
}

FileZip::~FileZip()
{
	if (m_uf != NULL) unzClose(m_uf);
}

FileZip::FileList FileZip::GetFiles()
{
	FileList files;
	if (m_uf != NULL)
	{
		uLong i;
		unz_global_info64 gi;
		int err = unzGetGlobalInfo64(m_uf, &gi);
		if (err == UNZ_OK)
		{
			unzGoToFirstFile(m_uf);
			for (i = 0; i < gi.number_entry; i++)
			{
				char filename_inzip[_MAX_PATH];
				unz_file_info64 file_info;
				err = unzGetCurrentFileInfo64(m_uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
				if (err == UNZ_OK && file_info.compressed_size > 0) files.push_back(std::string(filename_inzip));
				if ((i + 1) < gi.number_entry)
				{
					err = unzGoToNextFile(m_uf);
					if (err != UNZ_OK) break;
				}
			}
		}
	}
	return files;
}

int FileZip::ExtractFile(std::string file_in_zip, std::string output_folder)
{
	if (m_uf == NULL) return 1;
	if (unzLocateFile(m_uf, file_in_zip.c_str(), CASESENSITIVITY) != UNZ_OK) return 2;
	return do_extract_currentfile(output_folder);
}

int FileZip::ExtractAll(std::string output_folder)
{
	if (m_uf == NULL) return 1;
	uLong i;
	unz_global_info64 gi;
	int err = unzGetGlobalInfo64(m_uf, &gi);
	if (err != UNZ_OK) return 2;
	unzGoToFirstFile(m_uf);
	for (i = 0; i < gi.number_entry; i++)
	{
		do_extract_currentfile(output_folder);
		if ((i + 1) < gi.number_entry)
		{
			err = unzGoToNextFile(m_uf);
			if (err != UNZ_OK) break;
		}
	}
	return 0;
}

int SetFileDateTime(char *szFilePath,
	FILETIME ftCreated,
	FILETIME ftModified,
	FILETIME ftAccessed)
{
	HANDLE hFile = CreateFileA(szFilePath,
		GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_ARCHIVE,
		NULL);

	if (hFile == INVALID_HANDLE_VALUE) return 1;

	BOOL bVal = SetFileTime(hFile, &ftCreated, &ftAccessed, &ftModified);

	CloseHandle(hFile);

	if (!bVal)
		return 2;

	return 0;
}

int FileZip::do_extract_currentfile(std::string output_folder)
{
	if (m_uf == NULL) return 1;
	char filename_inzip[_MAX_PATH];
	unz_file_info64 file_info;
	int err = unzGetCurrentFileInfo64(m_uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
	std::string output_file = output_folder;
	if (output_file[output_file.size() - 1] != '/' && output_file[output_file.size() - 1] != '\\') output_file.append(1, '\\');
	if (filename_inzip[0] != '/' && filename_inzip[0] != '\\')
		output_file.append(filename_inzip);
	else
		output_file.append(filename_inzip + 1);
	std::replace(output_file.begin(), output_file.end(), '/', '\\');
	std::string::size_type prev_pos = 0, pos = 0;
	std::string directory;
	while ((pos = output_file.find('\\', pos)) != std::string::npos)
	{
		std::string substring(output_file.substr(prev_pos, pos - prev_pos));
		if (directory.size() > 0) directory.append(1, '\\');
		directory.append(substring);
		if(directory.size() > 3) CreateDirectoryA(directory.c_str(), NULL);
		prev_pos = ++pos;
	}
	err = unzOpenCurrentFile(m_uf);
	if (err != UNZ_OK) return 2;
	FILE* fout = fopen64(output_file.c_str(), "wb");
	if (fout != NULL)
	{
		uInt size_buf = WRITEBUFFERSIZE;
		void* buf = (void*)malloc(size_buf);
		do
		{
			err = unzReadCurrentFile(m_uf, buf, size_buf);
			if (err < 0) break;
			if (err > 0)
			{
				if (fwrite(buf, err, 1, fout) != 1)
				{
					err = UNZ_ERRNO;
					break;
				}
			}
		} while (err > 0);
		unzCloseCurrentFile(m_uf);
		fclose(fout);
		free(buf);
		FILETIME ft, lft;
		DosDateTimeToFileTime(HIWORD(file_info.dosDate), LOWORD(file_info.dosDate), &ft);
		LocalFileTimeToFileTime(&ft, &lft);
		SetFileDateTime((char*)output_file.c_str(), lft, lft, lft);
		return (err != UNZ_ERRNO) ? 0 : 4;
	}
	return 3;
}
