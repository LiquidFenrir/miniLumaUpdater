#include "file.h"

#include "7z/7z.h"
#include "7z/7zAlloc.h"
#include "7z/7zCrc.h"
#include "7z/7zMemInStream.h"

Result extractFileFrom7z(const char * archive_file, const char * filename, const char * filepath)
{	
	if (filename == NULL) {
		printf("Cannot start, the inarchive in config.json is blank.\n");
		return EXTRACTION_ERROR_CONFIG;
	}
	
	if (filepath == NULL) {
		printf("Cannot start, the path in config.json is blank.\n");
		return EXTRACTION_ERROR_CONFIG;
	}
	
	Result ret = 0;
	
	FILE * archive = fopen(archive_file, "rb");
	if (archive == NULL) {
		printf("Error: couldn't open archive to read.\n");
		return EXTRACTION_ERROR_ARCHIVE_OPEN;
	}
	
	fseek(archive, 0, SEEK_END);
	u32 archiveSize = ftell(archive);
	fseek(archive, 0, SEEK_SET);
	
	u8 * archiveData = malloc(archiveSize);
	fread(archiveData, archiveSize, 1, archive);
	
	CMemInStream memStream;
	CSzArEx db;
	ISzAlloc allocImp;
	ISzAlloc allocTempImp;
	
	MemInStream_Init(&memStream, archiveData, archiveSize);
	
	allocImp.Alloc = SzAlloc;
	allocImp.Free = SzFree;
	allocTempImp.Alloc = SzAllocTemp;
	allocTempImp.Free = SzFreeTemp;
	
	CrcGenerateTable();
	SzArEx_Init(&db);
	
	SRes res = SzArEx_Open(&db, &memStream.s, &allocImp, &allocTempImp);
	if (res != SZ_OK) {
		ret = EXTRACTION_ERROR_ARCHIVE_OPEN;
		goto finish;
	}
	
	for (u32 i = 0; i < db.NumFiles; ++i) {
		// Skip directories
		unsigned isDir = SzArEx_IsDir(&db, i);
		if (isDir) continue;
		
		// Get name
		size_t len;
		len = SzArEx_GetFileNameUtf16(&db, i, NULL);
		// Super long filename? Just skip it..
		if (len >= 256) continue;
		
		u16 name[256] = {0};
		SzArEx_GetFileNameUtf16(&db, i, name);
		
		// Convert name to ASCII (just cut the other bytes)
		char name8[256] = {0};
		for (size_t j = 0; j < len; ++j) {
			name8[j] = name[j] % 0xff;
		}
		
		u8 * buf = NULL;
		UInt32 blockIndex = UINT32_MAX;
		size_t fileBufSize = 0;
		size_t offset = 0;
		size_t fileSize = 0;

		if (!strcmp(name8, filename)) {
			res = SzArEx_Extract(
						&db,
						&memStream.s,
						i,
						&blockIndex,
						&buf,
						&fileBufSize,
						&offset,
						&fileSize,
						&allocImp,
						&allocTempImp
			);
			if (res != SZ_OK) {
				printf("Error: Couldn't extract file data from archive.\n");
				ret = EXTRACTION_ERROR_READ_IN_ARCHIVE;
				goto finish;
			}
			
			FILE * fh = fopen(filepath, "wb");
			if (fh == NULL) {
				printf("Error: couldn't open file to write.\n");
				return EXTRACTION_ERROR_WRITEFILE;
			}
			
			fwrite(buf+offset, fileSize, 1, fh);
			fclose(fh);
			
			free(buf);
		}
	}
	
	ret = EXTRACTION_ERROR_FIND;
	
	finish:
	free(archiveData);
	fclose(archive);
	
	return ret;
}

Result extractFileFromArchive(const char * archive_path, const char * filename, const char * filepath)
{
	Result ret = 0;
	ret = extractFileFrom7z(archive_path, filename, filepath);
	remove(archive_path);
	return ret;
}
