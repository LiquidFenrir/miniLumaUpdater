#include "file.h"

#include "minizip/unzip.h"

#include "7z/7z.h"
#include "7z/7zAlloc.h"
#include "7z/7zCrc.h"
#include "7z/7zMemInStream.h"

Result extractFileFromZip(const char * archive_file, const char * wanted, const char * outpath)
{	
	FILE * fh = fopen(outpath, "wb");
	if (fh == NULL) {
		printf("Error: couldn't open file to write.\n");
		return EXTRACTION_ERROR_WRITEFILE;
	}
	
	unzFile uf = unzOpen64(archive_file);
	if (uf == NULL) {
		printf("Couldn't open zip file.\n");
		fclose(fh);
		return EXTRACTION_ERROR_ARCHIVE_OPEN;
	}
	
	unz_file_info payloadInfo = {};
	
	u8 * buf = NULL;
	Result ret = unzLocateFile(uf, wanted, NULL);
	
	if (ret == UNZ_END_OF_LIST_OF_FILE) {
		printf("Couldn't find the wanted file in the zip.\n");
		ret = EXTRACTION_ERROR_FIND;
	}
	else if (ret == UNZ_OK) {
		ret = unzGetCurrentFileInfo(uf, &payloadInfo, NULL, 0, NULL, 0, NULL, 0);
		if (ret != UNZ_OK) {
			printf("Couldn't get the file information.\n");
			ret = EXTRACTION_ERROR_INFO;
			goto finish;
		}
		
		u32 toRead = 0x1000;
		buf = malloc(toRead);
		if (buf == NULL) {
			ret = EXTRACTION_ERROR_ALLOC;
			goto finish;
		}
		
		ret = unzOpenCurrentFile(uf);
		if (ret != UNZ_OK) {
			printf("Couldn't open file in the archive to read\n");
			ret = EXTRACTION_ERROR_OPEN_IN_ARCHIVE;
			goto finish;
		}
		
		u32 size = payloadInfo.uncompressed_size;
		
		do {
			if (size < toRead) toRead = size;
			ret = unzReadCurrentFile(uf, buf, toRead);
			if (ret < 0) {
				printf("Couldn't read data from the file in the archive\n");
				ret = EXTRACTION_ERROR_READ_IN_ARCHIVE;
				goto finish;
			}
			else {
				fwrite(buf, 1, toRead, fh);
			}
			size -= toRead;
		} while(size);
	}
	
	finish:
	free(buf);
	unzClose(uf);
	fclose(fh);
	
	return ret;
}

Result extractFileFrom7z(const char * archive_file, const char * wanted, const char * outpath)
{
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
	if (archiveData == NULL) {
		ret = EXTRACTION_ERROR_ALLOC;
		goto finish;
	}
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
		
		if (!strcmp(name8, wanted)) {
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
			
			FILE * fh = fopen(outpath, "wb");
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

Result extractFileFromArchive(const char * archive_path, const char * wanted, const char * outpath, bool zip)
{
	Result ret = 0;
	if (zip)
		ret = extractFileFromZip(archive_path, wanted, outpath);
	else
		ret = extractFileFrom7z(archive_path, wanted, outpath);
	
	remove(archive_path);
	return ret;
}
