#include "download.h"
#include "file.h"

#define HOURLY_KEY_COMBO (KEY_R & KEY_L & KEY_UP & KEY_A) //same combo as booting to recovery

int main()
{
	gfxInitDefault();
	httpcInit(0);
	
	consoleInit(GFX_TOP, NULL);
	
	printf("Welcome to the automated Luma3DS updater!\n");
	
	Result ret = 0;
	hidScanInput();
	
	if (((hidKeysDown() | hidKeysHeld()) & HOURLY_KEY_COMBO) == HOURLY_KEY_COMBO) {
		printf("Hourly key combo detected!\n");
		printf("Downloading latest hourly...\n");
		ret = downloadToFile(HOURLY_URL, HOURLY_FILE_PATH);
		if (ret)
			printf("Error when downloading file: 0x%.8lx\n", ret);
		else {
			printf("Extracting boot.firm from zip...\n");
			ret = extractFileFromArchive(HOURLY_FILE_PATH, "out/boot.firm", "/boot.firm", true);
			if (ret)
				printf("Error when extracting file: 0x%.8lx\n", ret);
		}
	}
	else {
		printf("Downloading latest release...\n");
		ret = downloadLatestRelease(ARCHIVE_FILE_PATH);
		if (ret)
			printf("Error when downloading file: 0x%.8lx\n", ret);
		else {
			printf("Extracting boot.firm from 7z...\n");
			ret = extractFileFromArchive(ARCHIVE_FILE_PATH, "boot.firm", "/boot.firm", false);
			if (ret)
				printf("Error when extracting file: 0x%.8lx\n", ret);
		}
	}
	
	printf("Rebooting in 3 seconds...\nTake note if an error happened.\n");
	svcSleepThread(3e9);
	APT_HardwareResetAsync();
	for(;;);
	
	httpcExit();
	gfxExit();
	return 0;
}
