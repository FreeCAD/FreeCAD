#if !defined(__xmemfile_h)
#define __xmemfile_h

#include "xfile.h"

//////////////////////////////////////////////////////////
class DLL_EXP CxMemFile : public CxFile
{
public:
	CxMemFile(BYTE* pBuffer = NULL, DWORD size = 0);
	~CxMemFile();

	bool Open();
	BYTE* GetBuffer(bool bDetachBuffer = true);

	virtual bool	Close();
	virtual size_t	Read(void *buffer, size_t size, size_t count);
	virtual size_t	Write(const void *buffer, size_t size, size_t count);
	virtual bool	Seek(long offset, int origin);
	virtual long	Tell();
	virtual long	Size();
	virtual bool	Flush();
	virtual bool	Eof();
	virtual long	Error();
	virtual bool	PutC(unsigned char c);
	virtual long	GetC();
	virtual char *	GetS(char *string, int n);
	virtual long	Scanf(const char *format, void* output);

protected:
	bool	Alloc(DWORD nBytes);
	void	Free();

	BYTE*	m_pBuffer;
	DWORD	m_Size;
	bool	m_bFreeOnClose;
	long	m_Position;	//current position
	long	m_Edge;		//buffer size
};

#endif
