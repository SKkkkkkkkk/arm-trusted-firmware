
#include <stdio.h>
#include <stdlib.h>
#include "time_stamp.h"

int main(int argc, char *argv[])
{
	FILE *input_fp;
	FILE *output_fp = stdout;

	if (argc != 2)
	{
		printf("Usage: %s <input_file>\n", argv[0]);
		return 1;
	}

	input_fp = fopen(argv[1], "rb");
	if (input_fp == NULL)
	{
		printf("Failed to open input file %s\n", argv[1]);
		return 1;
	}

	uint32_t entry_cout;
	time_stamp_entry_t *timestamp_entry;
	fread(&entry_cout, sizeof(uint32_t), 1, input_fp);
	timestamp_entry = (time_stamp_entry_t *)malloc(entry_cout * sizeof(time_stamp_entry_t));
	fread(timestamp_entry, sizeof(time_stamp_entry_t), entry_cout, input_fp);

	fprintf(output_fp,
		"时间戳位置, 较初始位置(ms), 较上个时间戳(ms)\n");
	for (uint32_t i = 0; i < entry_cout; i++)
	{
		if(strlen(timestamp_entry[i].func) > sizeof(timestamp_entry[i].func))
			return 1;
		fprintf(output_fp, "%s:%d, %.3f, %.3f\n",
			timestamp_entry[i].func,
			timestamp_entry[i].line,
			(double)timestamp_entry[i].tick / TIME_STAMP_FREQ * 1000,
			(i == 0 ? ((double)timestamp_entry[i].tick / TIME_STAMP_FREQ * 1000) \
			        : (((double)timestamp_entry[i].tick - timestamp_entry[i - 1].tick) / TIME_STAMP_FREQ * 1000)) );
	}

	free(timestamp_entry);
	fclose(input_fp);
	fclose(output_fp);
	return 0;
}