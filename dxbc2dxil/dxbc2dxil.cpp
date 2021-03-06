#include <windows.h>
#include <stdio.h>
#include "dxcapi.h"

#ifdef _WIN64
#error Platform not supported
#endif

extern unsigned char rawData[840704];				// Raw 32-bit DLL data
const char *DXBCFileInput = "C:\\myshader_ps.hlsl";	// Input DXBC
const char *DXILFileOutput = "C:\\outdxil.bin";		// Output DXIL ("dxc.exe -dumpbin $file" for LLVM IR text)

DxcCreateInstanceProc DxcCreateInstance;

void LoadDll()
{
	// Check if this is a default DLL on windows 10 or not
	HMODULE targetDll = LoadLibraryA("dxilconv.dll");

	if (!targetDll)
	{
		// dxilconv.dll wasn't found, dump it to the local directory
		FILE *dumpDll;
		fopen_s(&dumpDll, "dxilconv.dll", "wb");
		fwrite((void *)rawData, 1, sizeof(rawData), dumpDll);
		fclose(dumpDll);

		// Try loading it again
		targetDll = LoadLibraryA("dxilconv.dll");
	}

	DxcCreateInstance = (DxcCreateInstanceProc)GetProcAddress(targetDll, "DxcCreateInstance");
}

int main()
{
	LoadDll();

	if (!DxcCreateInstance)
	{
		__debugbreak();
		return 1;
	}

	IDxbcConverter *converter = nullptr;
	HRESULT hr = DxcCreateInstance(IID_IDxbcConverter, _GUID_ecc8691b_c1db_4dc0_855e_65f6c551af49, (LPVOID *)&converter);

	if (FAILED(hr))
	{
		__debugbreak();
		return 1;
	}

	FILE *fin = nullptr;
	FILE *fout = nullptr;

	if (fopen_s(&fin, DXBCFileInput, "rb") != 0 || fopen_s(&fout, DXILFileOutput, "wb") != 0)
	{
		__debugbreak();
		return 1;
	}

	fseek(fin, 0, SEEK_END);
	int size = ftell(fin);
	rewind(fin);

	BYTE *data = new BYTE[size];
	fread(data, 1, size, fin);
	fclose(fin);

	//
	// Extra options:
	// -disableHashCheck
	// -no-dxil-cleanup
	//
	LPVOID outBytecode = nullptr;
	UINT outLen = 0;

	hr = converter->Convert(data, size, nullptr, &outBytecode, &outLen, nullptr);

	fwrite(outBytecode, 1, outLen, fout);
	fclose(fout);

	getchar();
	return 0;
}