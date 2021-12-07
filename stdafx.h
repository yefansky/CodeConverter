#pragma once

#include <assert.h>
#include <windows.h>

#ifndef KGLOG_PROCESS_ERROR
#define KGLOG_PROCESS_ERROR(x) if (!(x)) {	\
	printf("KGLOG_PROCESS_ERROR(%s) at %s %s:%d\n", #x, __FUNCTION__, __FILE__, __LINE__);	\
	goto Exit0;	\
}
#endif

#ifndef KG_PROCESS_ERROR
#define KG_PROCESS_ERROR(x) if (!(x)) goto Exit0;
#endif

#ifndef KG_PROCESS_SUCCESS
#define KG_PROCESS_SUCCESS(x) if (x) goto Exit1;
#endif

#ifndef KG_DELETE
#define KG_DELETE(p) if (p) { delete p; p = nullptr; }
#endif

#ifndef KG_DELETE_ARRAY
#define KG_DELETE_ARRAY(p) if (p) { delete [] p; p = nullptr; }
#endif

struct SetConsoleColor
{
	SetConsoleColor(int nColor);
	~SetConsoleColor();
};
