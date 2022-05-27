#ifndef SSDT_INDEX_H
#define SSDT_INDEX_H

#endif // SSDT_INDEX_H


#include <iostream>
#include <memory>
#include <Windows.h>
#include <strsafe.h>
#include <vector>


#define PE_ERROR ((UINT_PTR)-1)

namespace ssdt_index
{
	using namespace std;

	UINT_PTR RVAToOffset(PIMAGE_NT_HEADERS64 pinh, UINT_PTR RVA, DWORD file_size);
	UINT_PTR GetExportOffset(UINT_PTR file_data, DWORD file_size, LPCSTR func_name);
	INT GetExportSSDTIndex(LPCSTR func_name);
	vector<string> GetExportFunctions();
	BOOL LoadNtDLL();
}

