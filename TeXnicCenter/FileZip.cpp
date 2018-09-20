#include "stdafx.h"
#include "FileZip.h"
#include <algorithm>
#include <sstream>

FileZip::FileZip(CString zip)
	: m_uf(NULL)
{
	zlib_filefunc64_def ffunc;
	fill_win32_filefunc64A(&ffunc);
	CT2CA zip_ansi(zip);
	m_uf = unzOpen2_64(zip_ansi.m_psz, &ffunc);
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
				if (err == UNZ_OK && file_info.compressed_size > 0) files.push_back(CString(filename_inzip));
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

int FileZip::ExtractFile(CString file_in_zip, CString output, bool bOutputIsFolder)
{
	if (m_uf == NULL) return 1;
	CT2CA file_in_zip_ansi(file_in_zip);
	if (unzLocateFile(m_uf, file_in_zip_ansi.m_psz, CASESENSITIVITY) != UNZ_OK) return 2;
	return do_extract_currentfile(output, bOutputIsFolder);
}

CString FileZip::ExtractFile2String(CString file_in_zip)
{
	if (m_uf == NULL) return CString();
	CT2CA file_in_zip_ansi(file_in_zip);
	if (unzLocateFile(m_uf, file_in_zip_ansi.m_psz, CASESENSITIVITY) != UNZ_OK) return CString();
	char filename_inzip[_MAX_PATH];
	unz_file_info64 file_info;
	int err = unzGetCurrentFileInfo64(m_uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
	if (err != UNZ_OK) return CString();
	err = unzOpenCurrentFile(m_uf);
	if (err != UNZ_OK) return CString();
	uInt size_buf = WRITEBUFFERSIZE;
	char* buf = new char[size_buf];
	memset(buf, 0, WRITEBUFFERSIZE);
	CString retVal;
	do
	{
		err = unzReadCurrentFile(m_uf, buf, size_buf - 1);
		if (err < 0) break;
		if (err > 0)
		{
			buf[err] = 0;
			retVal.Append(CString(buf));
		}
	} while (err > 0);
	unzCloseCurrentFile(m_uf);
	delete buf;
	return retVal;
}

int FileZip::ExtractAll(CString output_folder)
{
	if (m_uf == NULL) return 1;
	uLong i;
	unz_global_info64 gi;
	int err = unzGetGlobalInfo64(m_uf, &gi);
	if (err != UNZ_OK) return 2;
	unzGoToFirstFile(m_uf);
	for (i = 0; i < gi.number_entry; i++)
	{
		do_extract_currentfile(output_folder, true);
		if ((i + 1) < gi.number_entry)
		{
			err = unzGoToNextFile(m_uf);
			if (err != UNZ_OK) break;
		}
	}
	return 0;
}

CString FileZip::GetGlobalComment()
{
	CString retVal;
	if (m_uf == NULL) return retVal;
	char* buf = new char[WRITEBUFFERSIZE];
	memset(buf, 0, WRITEBUFFERSIZE);
	if (unzGetGlobalComment(m_uf, buf, WRITEBUFFERSIZE - 1) > 0) retVal = CString(buf);
	delete buf;
	return retVal;
}

CString FileZip::GetFileComment(CString file_in_zip)
{
	if (m_uf == NULL) return CString();
	CT2CA file_in_zip_ansi(file_in_zip);
	if (unzLocateFile(m_uf, file_in_zip_ansi.m_psz, CASESENSITIVITY) != UNZ_OK) return CString();
	char filename_inzip[_MAX_PATH];
	char comment[WRITEBUFFERSIZE];
	memset(comment, 0, WRITEBUFFERSIZE);
	unz_file_info64 file_info;
	int err = unzGetCurrentFileInfo64(m_uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, comment, sizeof(comment));
	if (err != UNZ_OK) return CString();
	return CString(comment);
}

int SetFileDateTime(CString szFilePath,
	FILETIME ftCreated,
	FILETIME ftModified,
	FILETIME ftAccessed)
{
	HANDLE hFile = CreateFile(szFilePath,
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

int FileZip::do_extract_currentfile(CString output, bool bOutputIsFolder)
{
	if (m_uf == NULL) return 1;
	char filename_inzip[_MAX_PATH];
	unz_file_info64 file_info;
	int err = unzGetCurrentFileInfo64(m_uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
	if (err != UNZ_OK) return 2;
	CString output_file = output;
	if (bOutputIsFolder)
	{
		if (output_file[output_file.GetLength() - 1] != '/' && output_file[output_file.GetLength() - 1] != '\\') output_file.AppendChar('\\');
		if (filename_inzip[0] != '/' && filename_inzip[0] != '\\')
			output_file.Append(CString(filename_inzip));
		else
			output_file.Append(CString(filename_inzip + 1));
		output_file.Replace(_T("/"), _T("\\"));
	}
	CPathTool::CreateDirectoryRecursive(CPathTool::GetDirectory(output_file));
	err = unzOpenCurrentFile(m_uf);
	if (err != UNZ_OK) return 3;
	FILE* fout = _tfopen(output_file, _T("wb"));
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
		SetFileDateTime(output_file, lft, lft, lft);
		return (err != UNZ_ERRNO) ? 0 : 4;
	}
	return 5;
}
