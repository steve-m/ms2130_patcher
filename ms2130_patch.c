/*
 * MS2130 firmware patcher for optimized video quality
 * Disables sharpening, scaling, etc.
 *
 * Copyright (C) 2024 by Steve Markgraf <steve@steve-m.de>
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>

#define CODE_OFFSET	0x30
#define CODE_CHECKSUM	0x09db

/* Patch the firmware to disable scaler, sharpening etc., also see
 * https://github.com/steve-m/hsdaoh/blob/21a4b470b4c079792034258304f6044bddc8abad/src/libhsdaoh.c#L205 */
void patch_firmware(uint8_t *code)
{
	/* patch function call from clear_extmem_mask() to set_extmem_mask()
	 * so that the result is:
	 * set_extmem_mask(0xf6be, 0x11) */
	code[0x9604] = 0xbf;
	code[0x9605] = 0x44;

	/* patch function call from clear_extmem_mask() to set_extmem_mask()
	 * so that the result is:
	 * set_extmem_mask(0xf6bf, 0x11) */
	code[0x960d] = 0xbf;
	code[0x960e] = 0x44;

	/* patch function call from FUN_CODE_b8ad() to clear_extmem_mask()
	 * so that the result is:
	 * clear_extmem_mask(0xf6b0, 0x01) (clear bit 0) */
	code[0xbe90] = 0x00;
	code[0xbe91] = 0x06;

	/* patch value of call to set_extmem_mask()
	 * so that the result is:
	 * set_extmem_mask(0xf600, 0x80) (set bit 7) */
	code[0xbee8] = 0x80;

	/* horizontal scaler config, patch out function call to calculation subroutine
	 * and force disable of scaling */
	code[0x937e] = 0x00; // NOP
	code[0x937f] = 0x7e; // MOV R6
	code[0x9380] = 0x10; // #0x10

	/* vertical scaler config, patch out function call to calculation subroutine
	 * and force disable of scaling */
	code[0x9399] = 0x00; // NOP
	code[0x939a] = 0x7e; // MOV R6
	code[0x939b] = 0x10; // #0x10
}

uint16_t calculate_header_checksum(uint8_t *data, uint16_t len)
{
	uint32_t csum = 0;

	for (int i = 0x02; i < len; i++) {
		if ((i < 0x0c) || (i > 0x0f))
			csum += data[i];
	}

	return csum & 0xffff;
}

uint16_t calculate_code_checksum(uint8_t *data, uint16_t len)
{
	uint32_t csum = 0;

	for (int i = 0; i < len; i++)
		csum += data[i];

	return csum & 0xffff;
}

int main(int argc, char *argv[])
{
	FILE *fp = fopen("./4k2.bin", "rb");
	if (!fp) {
		printf("Error opening file\n");
		goto err;
	}

	struct stat finfo;
	fstat(fileno(fp), &finfo);
	size_t file_len = finfo.st_size;

	printf("Length of file: %ld\n", file_len);

	uint8_t *fw = malloc(file_len);
	if (fw) {
		if (fread(fw, 1, file_len, fp) != file_len) {
			printf("Error reading firmware file!\n");
			goto err;
		}
		fclose(fp);
		fp = NULL;
	}

	uint16_t code_len = (fw[0x02] << 8) | fw[0x03];
	printf("Code length: %d\n", code_len);

	uint16_t calc_header_csum = calculate_header_checksum(fw, CODE_OFFSET);
	uint16_t orig_header_csum  = (fw[CODE_OFFSET + code_len + 0] << 8) | fw[CODE_OFFSET + code_len + 1];

	if (calc_header_csum != orig_header_csum)
		printf("Original header checksum mismatch: %04x != %04x\n", orig_header_csum, calc_header_csum);
	else
		printf("Original header checksum matches: %04x\n", orig_header_csum);

	uint16_t calc_code_csum = calculate_code_checksum(fw + CODE_OFFSET, code_len);
	uint16_t orig_code_csum  = (fw[CODE_OFFSET + code_len + 2] << 8) | fw[CODE_OFFSET + code_len + 3];

	if (calc_code_csum != orig_code_csum)
		printf("Original code checksum mismatch: %04x != %04x\n", orig_code_csum, calc_code_csum);
	else
		printf("Original code checksum matches: %04x\n", orig_code_csum);

	if (CODE_CHECKSUM != calc_code_csum) {
		printf("The code checksum does not match the firmware file this tool was written for, patch not applied!\n");
		goto err;
	}

	patch_firmware(fw + CODE_OFFSET);

	/* replace the code checksum */
	calc_code_csum = calculate_code_checksum(fw + CODE_OFFSET, code_len);
	fw[CODE_OFFSET + code_len + 2] = calc_code_csum >> 8;
	fw[CODE_OFFSET + code_len + 3] = calc_code_csum & 0xff;

	/* write the resulting file */
	fp = fopen("./patched.bin", "wb");
	fwrite(fw, 1, file_len, fp);

err:
	if (fp)
		fclose(fp);

	if (fw)
		free(fw);

	return 0;
}
