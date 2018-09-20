#pragma once
#include <unzip.h>

#define CASESENSITIVITY (0)
#define WRITEBUFFERSIZE (8192)

#ifdef _WIN32
#define USEWIN32IOAPI
#include <iowin32.h>
#endif

class FileZip
{
public:
	typedef std::vector<CString> FileList;

private:
	unzFile m_uf;

public:
	FileZip(CString zip);
	~FileZip();

	FileList GetFiles();
	int ExtractFile(CString file_in_zip, CString output, bool bOutputIsFolder = true);
	CString ExtractFile2String(CString file_in_zip);
	int ExtractAll(CString output_folder);
	CString GetGlobalComment();
	CString GetFileComment(CString file_in_zip);

private:
	int do_extract_currentfile(CString output, bool bOutputIsFolder = true);
};
