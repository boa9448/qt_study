#include "ssdt_index.h"


namespace ssdt_index
{
    static shared_ptr<BYTE[]> file_data;
    static DWORD file_size = 0;

    UINT_PTR RVAToOffset(PIMAGE_NT_HEADERS64 pinh, UINT_PTR RVA, DWORD file_size)
    {
        PIMAGE_SECTION_HEADER ish = IMAGE_FIRST_SECTION(pinh);
        INT number_of_section = pinh->FileHeader.NumberOfSections;
        for (INT idx = 0; idx < number_of_section; idx++)
        {
            if (ish->VirtualAddress <= RVA && RVA < ish->VirtualAddress + ish->Misc.VirtualSize)
            {
                RVA -= ish->VirtualAddress;
                RVA += ish->PointerToRawData;

                return RVA < file_size ? RVA : PE_ERROR;
            }

            ish++;
        }

        return PE_ERROR;
    }

    UINT_PTR GetExportOffset(UINT_PTR file_data, DWORD file_size, LPCSTR func_name)
    {
        PIMAGE_DOS_HEADER idh = (PIMAGE_DOS_HEADER)file_data;
        if (idh->e_magic != IMAGE_DOS_SIGNATURE)
        {
            //cout << "IMAGE_DOS_SIGNATURE error" << endl;
            return PE_ERROR;
        }

        PIMAGE_NT_HEADERS64 inh = (PIMAGE_NT_HEADERS64)(file_data + idh->e_lfanew);
        if (inh->Signature != IMAGE_NT_SIGNATURE)
        {
            //cout << "IMAGE_NT_SIGNATURE error" << endl;
            return PE_ERROR;
        }

        PIMAGE_FILE_HEADER ifh = (PIMAGE_FILE_HEADER)&inh->FileHeader;
        PIMAGE_OPTIONAL_HEADER64 ioh = (PIMAGE_OPTIONAL_HEADER64)&inh->OptionalHeader;
        PIMAGE_DATA_DIRECTORY idd = (PIMAGE_DATA_DIRECTORY)&ioh->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];

        UINT_PTR ied_offset = RVAToOffset(inh, idd->VirtualAddress, file_size);
        PIMAGE_EXPORT_DIRECTORY ied = (PIMAGE_EXPORT_DIRECTORY)(file_data + ied_offset);

        UINT_PTR export_name_table_offset = RVAToOffset(inh, ied->AddressOfNames, file_size);
        UINT_PTR export_ordinal_table_offset = RVAToOffset(inh, ied->AddressOfNameOrdinals, file_size);
        UINT_PTR export_function_table_offset = RVAToOffset(inh, ied->AddressOfFunctions, file_size);
        if (export_name_table_offset == PE_ERROR
            || export_ordinal_table_offset == PE_ERROR
            || export_function_table_offset == PE_ERROR)
        {
            //cout << "export director content error" << endl;
            return PE_ERROR;
        }

        PDWORD export_name_table = (PDWORD)(file_data + export_name_table_offset);
        PWORD export_ordinal_table = (PWORD)(file_data + export_ordinal_table_offset);
        PDWORD export_function_table = (PDWORD)(file_data + export_function_table_offset);

        UINT_PTR export_function_offset = PE_ERROR;
        for (INT idx = 0; idx < ied->NumberOfNames; idx++)
        {
            UINT_PTR name_offset = RVAToOffset(inh, export_name_table[idx], file_size);
            char* name = (char*)(file_data + name_offset);
            if (lstrcmpA(func_name, name) == 0)
            {
                export_function_offset = RVAToOffset(inh, export_function_table[export_ordinal_table[idx]], file_size);
                break;
            }
        }

        return export_function_offset;
    }

    INT GetExportSSDTIndex(LPCSTR func_name)
    {
        UINT_PTR data = (UINT_PTR)file_data.get();
        UINT_PTR export_function_offset = GetExportOffset(data, file_size, func_name);
        if (export_function_offset == PE_ERROR)
        {
            //cout << "export function offset error" << endl;
            return -1;
        }

        INT SSDT_index = -1;
        PBYTE function_data = (PBYTE)(data + export_function_offset);
        for (INT idx = 0; idx < 32 && export_function_offset + 1 < file_size; idx++)
        {
            if (function_data[idx] == 0xC2 || function_data[idx] == 0xC3) // RET
                break;

            if (function_data[idx] == 0xB8)
            {
                SSDT_index = *(INT*)(function_data + idx + 1);
                break;
            }
        }

        //if (SSDT_index == PE_ERROR)
        //    cout << "SSDT index not found" << endl;

        return SSDT_index;
    }

    vector<string> GetExportFunctions()
    {
        vector<string> function_list;

        UINT_PTR data = (UINT_PTR)file_data.get();
        PIMAGE_DOS_HEADER idh = (PIMAGE_DOS_HEADER)data;
        if (idh->e_magic != IMAGE_DOS_SIGNATURE)
        {
            cout << "IMAGE_DOS_SIGNATURE error" << endl;
            return function_list;
        }

        PIMAGE_NT_HEADERS64 inh = (PIMAGE_NT_HEADERS64)(data + idh->e_lfanew);
        if (inh->Signature != IMAGE_NT_SIGNATURE)
        {
            cout << "IMAGE_NT_SIGNATURE error" << endl;
            return function_list;
        }

        PIMAGE_FILE_HEADER ifh = (PIMAGE_FILE_HEADER)&inh->FileHeader;
        PIMAGE_OPTIONAL_HEADER64 ioh = (PIMAGE_OPTIONAL_HEADER64)&inh->OptionalHeader;
        PIMAGE_DATA_DIRECTORY idd = (PIMAGE_DATA_DIRECTORY)&ioh->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];

        UINT_PTR ied_offset = RVAToOffset(inh, idd->VirtualAddress, file_size);
        PIMAGE_EXPORT_DIRECTORY ied = (PIMAGE_EXPORT_DIRECTORY)(data + ied_offset);

        UINT_PTR export_name_table_offset = RVAToOffset(inh, ied->AddressOfNames, file_size);
        UINT_PTR export_ordinal_table_offset = RVAToOffset(inh, ied->AddressOfNameOrdinals, file_size);
        UINT_PTR export_function_table_offset = RVAToOffset(inh, ied->AddressOfFunctions, file_size);
        if (export_name_table_offset == PE_ERROR
            || export_ordinal_table_offset == PE_ERROR
            || export_function_table_offset == PE_ERROR)
        {
            cout << "export director content error" << endl;
            return function_list;
        }

        PDWORD export_name_table = (PDWORD)(data + export_name_table_offset);
        PWORD export_ordinal_table = (PWORD)(data + export_ordinal_table_offset);
        PDWORD export_function_table = (PDWORD)(data + export_function_table_offset);

        UINT_PTR export_function_offset = PE_ERROR;
        for (INT idx = 0; idx < ied->NumberOfNames; idx++)
        {
            UINT_PTR name_offset = RVAToOffset(inh, export_name_table[idx], file_size);
            char* name = (char*)(data + name_offset);
            function_list.push_back(string(name));
        }

        return function_list;
    }

    BOOL LoadNtDLL()
    {
        LPCWSTR file_name = L"\\ntdll.dll";
        WCHAR file_path[MAX_PATH] = { 0, };
        //GetCurrentDirectoryW(MAX_PATH, file_path);
        GetSystemDirectoryW(file_path, MAX_PATH);
        StringCchCatW(file_path, MAX_PATH, file_name);

        DWORD size = 0;
        HANDLE file_handle = CreateFile(file_path, GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);


        if (file_handle == INVALID_HANDLE_VALUE)
            return FALSE;

        size = GetFileSize(file_handle, NULL);
        if (size == INVALID_FILE_SIZE)
        {
            CloseHandle(file_handle);
            return FALSE;
        }

        shared_ptr<BYTE[]> file_buffer = shared_ptr<BYTE[]>(new BYTE[size], [](BYTE* p) {delete[] p; });
        if (!ReadFile(file_handle, file_buffer.get(), size, NULL, NULL))
        {
            CloseHandle(file_handle);
            return FALSE;
        }

        file_data = file_buffer;
        file_size = size;
        return TRUE;
    }
}
