#pragma once

class CMatlab
{
private:
	void* m_pDLL;

public:
	CMatlab(bool ff = false);
	~CMatlab();

	bool Open();
	void Close();
	bool EvalString(const char *string);
	bool OutputBuffer(char *p, int n);
	bool SetVisible(bool value);

	operator bool() const { return m_pDLL != NULL; }

	static bool HasMatlab();
};
