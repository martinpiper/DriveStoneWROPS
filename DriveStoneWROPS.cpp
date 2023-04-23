// DriveStoneWROPS.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include "DriveStoneWROPS.h"

static void CalculateData(int *data, const size_t numElements)
{
	data[0] = 0;
	data[1] = 0;
	for (size_t i = 2; i < numElements; i++)
	{
		data[0] += data[i];
		data[1] ^= data[i];
	}
}

int main(int argc, char **argv)
{
	typedef std::chrono::high_resolution_clock Clock;
	int failWriteAfter = 0;
	int failReadAfter = 0;
	bool readTest = false;

	argc--;
	argv++;
	while (argc > 0)
	{
		if (argv[0][0] == '-' || argv[0][0] == '/')
		{
			if (argv[0][1] == 'h')
			{
				printf("/fw <number> : Fails the write test after <number> operations.");
				printf("/fr <number> : Fails the read test after <number> operations.");
			}
			else if (argv[0][1] == 'r')
			{
				readTest = true;
			}
			else if (argv[0][1] == 'f' && argv[0][2] == 'w')
			{
				argc--;
				argv++;
				failWriteAfter = atoi(argv[0]);
			}
			else if (argv[0][1] == 'f' && argv[0][2] == 'r')
			{
				argc--;
				argv++;
				failReadAfter = atoi(argv[0]);
			}
		}
		argc--;
		argv++;
	}

	const size_t kNumElements = 1024;
	int fileNumber = 0;
	char filenameBuffer[_MAX_PATH];
	int data[kNumElements];
	bool gotError = false;
	double numWriteOperations = 0;

	auto t1 = Clock::now();
	while (!gotError)
	{
		sprintf(filenameBuffer, "t%d.bin", fileNumber);

		FILE *fp = fopen(filenameBuffer , "wb");
		if (!fp)
		{
			gotError = true;
			break;
		}

		printf("Writing: %d\r", fileNumber);
		size_t numBlocks = 0;
		while (numBlocks < ((1024 * 1024)/4))
		{
			for (int i = 2; i < kNumElements; i++)
			{
				data[i] = rand() | (rand() << 16);
			}
			CalculateData(data, kNumElements);
			size_t numWritten = fwrite(data,sizeof(data[0]) , kNumElements , fp);
			if (numWritten != kNumElements)
			{
				gotError = true;
				break;
			}
			numWriteOperations++;
			if (failWriteAfter > 0)
			{
				failWriteAfter--;
				if (failWriteAfter <= 0)
				{
					gotError = true;
					printf("\nForced write error\n");
					break;
				}
			}
			numBlocks++;
		}

		fclose(fp);
		fileNumber++;
	}
	auto t2 = Clock::now();
	std::chrono::duration<double> delta(t2 - t1);
	double delta2 = delta.count();
	printf("\nTime=%f seconds Writes=%.0f WOPS=%f\n" , delta2 , numWriteOperations , numWriteOperations / delta2);

	if (!readTest)
	{
		return 0;
	}

	t1 = Clock::now();
	gotError = false;
	int readFileNumber = 0;
	double numReadOperations = 0;
	while (!gotError && (readFileNumber < fileNumber))
	{
		sprintf(filenameBuffer, "t%d.bin", readFileNumber);

		FILE* fp = fopen(filenameBuffer, "rb");
		printf("Reading: %d\r", readFileNumber);
		if (!fp)
		{
			gotError = true;
			break;
		}

		while (!gotError && !feof(fp))
		{
			size_t numRead = fread(data , sizeof(data[0]) , kNumElements , fp);
			if (numRead >= 2)
			{
				numReadOperations++;
				int t0 = data[0];
				int t1 = data[1];
				CalculateData(data, numRead);
				if (t0 != data[0])
				{
					printf("\nVerify t0 fail\n");
					break;
				}
				if (t1 != data[1])
				{
					printf("\nVerify t1 fail\n");
					break;
				}

				if (failReadAfter > 0)
				{
					failReadAfter--;
					if (failReadAfter <= 0)
					{
						gotError = true;
						printf("\nForced read error\n");
						break;
					}
				}

			}
			else
			{
				break;
			}
		}

		fclose(fp);

		remove(filenameBuffer);

		readFileNumber++;
	}

	t2 = Clock::now();
	delta = (t2 - t1);
	delta2 = delta.count();
	printf("\nTime=%f seconds Reads=%.0f ROPS=%f\n", delta2, numReadOperations, numReadOperations / delta2);

	return 0;
}
