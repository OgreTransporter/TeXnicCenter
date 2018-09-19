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
	typedef std::vector<std::string> FileList;

private:
	unzFile m_uf;

public:
	FileZip(std::string zip);
	~FileZip();

	FileList GetFiles();
	int ExtractFile(std::string file_in_zip, std::string output_folder);
	int ExtractAll(std::string output_folder);

private:
	int do_extract_currentfile(std::string output_folder);
};
