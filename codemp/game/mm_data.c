
#include "q_shared.h"
#include "g_local.h" // for MAX_DATA_SIZE

void MM_ReadData(const char *fileName, int (*processData)(const char *fileData))
{
	fileHandle_t f;
	char *fileData = (char*)calloc(1, MAX_DATA_SIZE);
	char fullFileName[512] = { 0 }; // Seems reasonable for a path
	sprintf_s(fullFileName, sizeof(fullFileName), "data\\%s", fileName);
	int length = trap_FS_FOpenFile(fullFileName, &f, FS_READ);

	if (!f || length <= 0)
	{ // file not found, welp
		return;
	}

	trap_FS_Read(fileData, MAX_DATA_SIZE, f);
	
	processData(fileData);

	trap_FS_FCloseFile(f);
	free(fileData);
}

void MM_WriteData(const char *fileName, int(*processData)(char *fileData, int *fileSize))
{
	fileHandle_t f;
	int fileSize = MAX_DATA_SIZE;
	char *fileData = (char*)calloc(1, fileSize);
	char fullFileName[512] = { 0 }; // Seems reasonable for a path
	sprintf_s(fullFileName, sizeof(fullFileName), "data\\%s", fileName);
	trap_FS_FOpenFile(fullFileName, &f, FS_WRITE);

	processData(fileData, &fileSize);

	trap_FS_Write(fileData, fileSize > MAX_DATA_SIZE ? MAX_DATA_SIZE : fileSize, f);

	trap_FS_FCloseFile(f);
	free(fileData);
}