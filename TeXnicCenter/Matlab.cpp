#include "stdafx.h"
#include "Matlab.h"

#pragma comment(lib, "version.lib")

static CString s_strLibEng;

class Version
{
public:
	DWORD m_major, m_minor, m_build, m_revision;
	DWORD m_Null;

public:
	Version(DWORD major = 1, DWORD minor = 0, DWORD build = 0, DWORD revision = 1)
		: m_major(major)
		, m_minor(minor)
		, m_build(build)
		, m_revision(revision)
		, m_Null(0)
	{}

	Version& operator++() { m_revision++; return *this; }
	Version& operator+=(const Version& rhs)
	{
		m_major += rhs.m_major;
		m_minor += rhs.m_minor;
		m_build += rhs.m_build;
		m_revision += rhs.m_revision;
		return *this;
	}
	Version& operator-=(const Version& rhs)
	{
		m_major = rhs.m_major < m_major ? m_major - rhs.m_major : 0;
		m_minor = rhs.m_minor < m_minor ? m_minor - rhs.m_minor : 0;
		m_build = rhs.m_build < m_build ? m_build - rhs.m_build : 0;
		m_revision = rhs.m_revision < m_revision ? m_revision - rhs.m_revision : 0;
		return *this;
	}
	friend Version operator+(Version lhs, const Version& rhs)
	{
		lhs += rhs;
		return lhs;
	}
	friend Version operator-(Version lhs, const Version& rhs)
	{
		lhs -= rhs;
		return lhs;
	}
	friend bool operator<(const Version& l, const Version& r)
	{
		if (l.m_major < r.m_major) return true;
		if (l.m_major > r.m_major) return false;
		if (l.m_minor < r.m_minor) return true;
		if (l.m_minor > r.m_minor) return false;
		if (l.m_build < r.m_build) return true;
		if (l.m_build > r.m_build) return false;
		if (l.m_revision < r.m_revision) return true;
		return false;
	}
	friend bool operator> (const Version& lhs, const Version& rhs) { return rhs < lhs; }
	friend bool operator==(const Version& lhs, const Version& rhs)
	{
		return lhs.m_major == rhs.m_major && lhs.m_minor == rhs.m_minor && lhs.m_build == rhs.m_build && lhs.m_revision == rhs.m_revision;
	}
	friend bool operator!=(const Version& lhs, const Version& rhs) { return !(lhs == rhs); }
	friend bool operator<=(const Version& lhs, const Version& rhs) { return lhs < rhs || lhs == rhs; }
	friend bool operator>=(const Version& lhs, const Version& rhs) { return lhs > rhs || lhs == rhs; }
	DWORD& operator[](std::size_t idx)
	{
		if (idx == 0) return m_major;
		if (idx == 1) return m_minor;
		if (idx == 2) return m_build;
		if (idx == 3) return m_revision;
		return m_Null;
	}
	const DWORD& operator[](std::size_t idx) const
	{
		if (idx == 0) return m_major;
		if (idx == 1) return m_minor;
		if (idx == 2) return m_build;
		if (idx == 3) return m_revision;
		return m_Null;
	}
};

bool is_64bit(void)
{
#if defined(_WIN64)
	return true;  // 64-bit programs run only on Win64
#elif defined(_WIN32)
	BOOL f64 = FALSE;
	return IsWow64Process(GetCurrentProcess(), &f64) && !f64;
#endif
}

class MatlabVersion
{
public:
	CString szEnginePath;
	Version ver;

	MatlabVersion(CString path = _T(""))
		: szEnginePath(path)
	{}

	bool ReadDllVersion()
	{
		DWORD  verHandle = 0;
		UINT   size = 0;
		LPBYTE lpBuffer = NULL;
		DWORD  verSize = GetFileVersionInfoSize(szEnginePath, &verHandle);
		Version def;

		if (verSize != NULL)
		{
			LPSTR verData = new char[verSize];
			if (GetFileVersionInfo(szEnginePath, verHandle, verSize, verData))
			{
				if (VerQueryValue(verData, _T("\\"), (VOID FAR* FAR*)&lpBuffer, &size))
				{
					if (size)
					{
						VS_FIXEDFILEINFO *verInfo = (VS_FIXEDFILEINFO *)lpBuffer;
						ver = Version((DWORD)(HIWORD(verInfo->dwProductVersionMS)), (DWORD)(LOWORD(verInfo->dwProductVersionMS)), (DWORD)(HIWORD(verInfo->dwProductVersionLS)), (DWORD)(LOWORD(verInfo->dwProductVersionLS)));
						if (ver == def)
						{
							DWORD langD = 0;
							size = 0;
							TCHAR fileEntry[64];
							if (VerQueryValue(verData, _T("\\VarFileInfo\\Translation"), (VOID FAR* FAR*)&lpBuffer, &size) && size == 4)
							{
								memcpy(&langD, verData, 4);
								_stprintf_s(fileEntry, 63, _T("\\StringFileInfo\\%02X%02X%02X%02X\\ProductVersion"), (langD & 0xff00) >> 8, langD & 0xff, (langD & 0xff000000) >> 24, (langD & 0xff0000) >> 16);
							}
							else
								_stprintf_s(fileEntry, 63, _T("\\StringFileInfo\\%04X04B0\\ProductVersion"), GetUserDefaultLangID());
							size = 0;
							CString tmp;
							if (VerQueryValue(verData, fileEntry, (VOID FAR* FAR*)&lpBuffer, &size))
								tmp = CString((TCHAR*)lpBuffer);
							else
							{
								_stprintf_s(fileEntry, 63, _T("\\StringFileInfo\\%04X04B0\\ProductVersion"), GetUserDefaultLangID());
								if (VerQueryValue(verData, fileEntry, (VOID FAR* FAR*)&lpBuffer, &size))
									tmp = CString((TCHAR*)lpBuffer);
								else
								{
									_stprintf_s(fileEntry, 63, _T("\\StringFileInfo\\%04X04B0\\ProductVersion"), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
									if (VerQueryValue(verData, fileEntry, (VOID FAR* FAR*)&lpBuffer, &size))
										tmp = CString((TCHAR*)lpBuffer);
									else
									{
										_stprintf_s(fileEntry, 63, _T("\\StringFileInfo\\%04X04B0\\ProductVersion"), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
										if (VerQueryValue(verData, fileEntry, (VOID FAR* FAR*)&lpBuffer, &size))
											tmp = CString((TCHAR*)lpBuffer);
									}
								}
							}
							if (tmp.GetLength() > 0)
							{
								int nTokenPos = 0;
								CString strToken = tmp.Tokenize(_T("."), nTokenPos);
								std::vector<CString> tmpList;
								while (!strToken.IsEmpty())
								{
									tmpList.push_back(strToken);
									strToken = tmp.Tokenize(_T("."), nTokenPos);
								}
								DWORD tmajor = 0, tminor = 0, tbuild = 0, trevision = 0;
								if (tmpList.size() > 0) tmajor = _tstol((TCHAR*)tmpList[0].GetBuffer());
								if (tmpList.size() > 1) tminor = _tstol((TCHAR*)tmpList[1].GetBuffer());
								if (tmpList.size() > 2) tbuild = _tstol((TCHAR*)tmpList[2].GetBuffer());
								if (tmpList.size() > 3) trevision = _tstol((TCHAR*)tmpList[3].GetBuffer());
								ver = Version(tmajor, tminor, tbuild, trevision);
							}
						}
						delete[] verData;
						return true;
					}
				}
			}
			delete[] verData;
		}
		return false;
	}

	bool operator < (const MatlabVersion& rhs) const
	{
		return (ver < rhs.ver);
	}

	bool operator > (const MatlabVersion& rhs) const
	{
		return (ver > rhs.ver);
	}
};

class MatlabDll
{
private:
	typedef struct engine Engine;
	typedef Engine*(WINAPI *ptrEngOpen)(const char*);
	typedef int(WINAPI *ptrEngClose)(Engine*);
	typedef int(WINAPI *ptrEngEvalString)(Engine*, const char*);
	typedef int(WINAPI *ptrEngOutputBuffer)(Engine*, char*, int);
	typedef int(WINAPI *ptrEngSetVisible)(Engine*, bool);

	HMODULE m_hDLL;
	Engine* m_pEngine;
	ptrEngOpen lpfnEngOpen;
	ptrEngClose lpfnEngClose;
	ptrEngEvalString lpfnEngEvalString;
	ptrEngOutputBuffer lpfnEngOutputBuffer;
	ptrEngSetVisible lpfnEngSetVisible;

public:
	MatlabDll(CString path)
		: m_hDLL(NULL)
		, m_pEngine(NULL)
		, lpfnEngOpen(NULL)
		, lpfnEngClose(NULL)
		, lpfnEngEvalString(NULL)
		, lpfnEngOutputBuffer(NULL)
		, lpfnEngSetVisible(NULL)
	{
		SetDllDirectory(_T(""));
		SetDllDirectory(path.Left(path.GetLength() - 11));
		m_hDLL = LoadLibrary(path);
		if (m_hDLL == NULL) return;

		lpfnEngOpen = (ptrEngOpen)GetProcAddress(m_hDLL, "engOpen");
		lpfnEngClose = (ptrEngClose)GetProcAddress(m_hDLL, "engClose");
		lpfnEngEvalString = (ptrEngEvalString)GetProcAddress(m_hDLL, "engEvalString");
		lpfnEngOutputBuffer = (ptrEngOutputBuffer)GetProcAddress(m_hDLL, "engOutputBuffer");
		lpfnEngSetVisible = (ptrEngSetVisible)GetProcAddress(m_hDLL, "engSetVisible");
	}

	~MatlabDll()
	{
		engClose();
		if (m_hDLL != NULL)
		{
			FreeLibrary(m_hDLL);
			m_hDLL = NULL;
		}
	}

	inline bool IsValid() { return (m_hDLL != NULL); }

	bool engOpen()
	{
		if (lpfnEngOpen == NULL) return false;
		m_pEngine = lpfnEngOpen(NULL);
		return m_pEngine != NULL;
	}

	int engClose()
	{
		int retVal = 1;
		if (m_pEngine != NULL || lpfnEngClose == NULL)
		{
			retVal = lpfnEngClose(m_pEngine);
			m_pEngine = NULL;
		}
		return retVal;
	}

	int engEvalString(const char *string)
	{
		if (m_pEngine == NULL || lpfnEngEvalString == NULL) return 1;
		return lpfnEngEvalString(m_pEngine, string);
	}

	int engOutputBuffer(char *p, int n)
	{
		if (m_pEngine == NULL || lpfnEngOutputBuffer == NULL) return 1;
		return lpfnEngOutputBuffer(m_pEngine, p, n);
	}

	int engSetVisible(bool newVal)
	{
		if (m_pEngine == NULL || lpfnEngSetVisible == NULL) return 1;
		return lpfnEngSetVisible(m_pEngine, newVal);
	}
};

CMatlab::CMatlab(bool ff)
	: m_pDLL(NULL)
{
	if (s_strLibEng.GetLength() > 0 && !ff)
	{
		// Laden
		m_pDLL = new MatlabDll(s_strLibEng);
		if (!((MatlabDll*)m_pDLL)->IsValid())
		{
			delete m_pDLL;
			m_pDLL = NULL;
		}
		else
			return;
	}

	std::vector<MatlabVersion> version;
	DWORD access = KEY_READ | KEY_WOW64_64KEY;
	DWORD length;
	TCHAR buffer[512];
	CString format;
	bool x64 = is_64bit();
	memset(buffer, 0, 512 * sizeof(TCHAR));

	// Registry
	const TCHAR* mainKeys[] = { _T("SOFTWARE\\MathWorks"), _T("SOFTWARE\\Wow6432Node\\MathWorks") };
	for (int i = 0; i < 2; i++)
	{
		CRegKey rk;
		if (rk.Open(HKEY_LOCAL_MACHINE, mainKeys[i], access) == ERROR_SUCCESS)
		{
			int j = 0;
			length = 511;
			while (rk.EnumKey((DWORD)j, buffer, &length, NULL) == ERROR_SUCCESS)
			{
				if (buffer[0] == _T('R') && buffer[1] == _T('2') && buffer[2] == _T('0'))
				{
					CRegKey subkey;
					format.Format(_T("%s\\MATLAB"), buffer);
					if (subkey.Open(rk, format, access) == ERROR_SUCCESS)
					{
						ULONG size = 511;
						if (subkey.QueryStringValue(NULL, buffer, &size) == ERROR_SUCCESS)
						{
							format.SetString(buffer);
							format.Append(_T("\\libeng.dll"));
							if (format.Right(21) == _T("\\bin\\win64\\libeng.dll") && x64)
							{
								MatlabVersion mv(format);
								if (mv.ReadDllVersion()) version.push_back(mv);
							}
							if (format.Right(21) == _T("\\bin\\win32\\libeng.dll") && !x64)
							{
								MatlabVersion mv(format);
								if (mv.ReadDllVersion()) version.push_back(mv);
							}
						}
						subkey.Close();
					}
				}
				j++;
				length = 511;
			}
			rk.Close();
		}
	}

	// File system
	const TCHAR* mainPath[] = { _T("C:\\Program Files\\MATLAB\\*"), _T("C:\\Program Files (x86)\\MATLAB\\*") };
	for (int i = 0; i < 2; i++)
	{
		CFileFind finder;
		BOOL bWorking = finder.FindFile(static_cast<LPCTSTR>(mainPath[i]));
		while (bWorking)
		{
			bWorking = finder.FindNextFile();
			if (finder.GetFileName() == _T(".") || finder.GetFileName() == _T("..") || !finder.IsDirectory())
				continue;
			else
			{
				format = finder.GetFilePath() + _T("\\bin\\win64\\libeng.dll");
				if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(static_cast<LPCTSTR>(format)) && x64)
				{
					MatlabVersion mv(format);
					if (mv.ReadDllVersion()) version.push_back(mv);
				}
				format = finder.GetFilePath() + _T("\\bin\\win32\\libeng.dll");
				if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(static_cast<LPCTSTR>(format)) && !x64)
				{
					MatlabVersion mv(format);
					if (mv.ReadDllVersion()) version.push_back(mv);
				}
			}
		}
	}

	// Sort
	if (version.size() == 0) return;
	std::sort(version.begin(), version.end(), std::greater<MatlabVersion>());

	// Load
	m_pDLL = new MatlabDll(version[0].szEnginePath);
	if (!((MatlabDll*)m_pDLL)->IsValid())
	{
		delete m_pDLL;
		m_pDLL = NULL;
	}
	else
		s_strLibEng = version[0].szEnginePath;
}

CMatlab::~CMatlab()
{
	if (m_pDLL != NULL)
	{
		delete m_pDLL;
		m_pDLL = NULL;
	}
}

bool CMatlab::Open()
{
	if (m_pDLL == NULL) return false;
	return ((MatlabDll*)m_pDLL)->engOpen();
}

void CMatlab::Close()
{
	if (m_pDLL != NULL) ((MatlabDll*)m_pDLL)->engClose();
}

bool CMatlab::EvalString(const char *string)
{
	if (m_pDLL == NULL) return false;
	return ((MatlabDll*)m_pDLL)->engEvalString(string) == 0;
}

bool CMatlab::OutputBuffer(char *p, int n)
{
	if (m_pDLL == NULL) return false;
	return ((MatlabDll*)m_pDLL)->engOutputBuffer(p, n) == 0;
}

bool CMatlab::SetVisible(bool value)
{
	if (m_pDLL == NULL) return false;
	return ((MatlabDll*)m_pDLL)->engSetVisible(value) == 0;
}

bool CMatlab::HasMatlab()
{
	if (s_strLibEng.GetLength() > 0) return true;
	CMatlab mapp;
	return mapp;
}
