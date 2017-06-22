#include "download.h"
#include "file.h"

int main()
{
	gfxInitDefault();
	httpcInit(0);
	
	consoleInit(GFX_TOP, NULL);
	
	printf("Welcome to the automated Luma3DS updater!\n");
	
	printf("Downloading latest release...\n");
	downloadLatestRelease(ARCHIVE_FILE_PATH);
	
	printf("Extracting boot.firm from 7z...\n");
	extractFileFromArchive(ARCHIVE_FILE_PATH, "boot.firm", "/boot.firm");
	
	printf("Rebooting in 3 seconds...\n");
	svcSleepThread(3e9);
	APT_HardwareResetAsync();
	
	httpcExit();
	gfxExit();
	return 0;
}
