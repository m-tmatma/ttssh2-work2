/*
 * Copyright (C) 2020 TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#if !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>


#include "i18n.h"
#include "layer_for_unicode.h"
#include "asprintf.h"

#include "ttlib.h"

/**
 *	GetI18nStrW() の動的バッファ版
 */
wchar_t *TTGetLangStrW(const char *section, const char *key, const wchar_t *def, const char *UILanguageFile)
{
	wchar_t *buf = (wchar_t *)malloc(MAX_UIMSG * sizeof(wchar_t));
	size_t size = GetI18nStrW(section, key, buf, MAX_UIMSG, def, UILanguageFile);
	buf = (wchar_t *)realloc(buf, size * sizeof(wchar_t));
	return buf;
}

/**
 *	MessageBoxを表示する
 *
 *	@param[in]	hWnd			親 window
 *	@param[in]	info			タイトル、メッセージ
 *	@param[in]	uType			MessageBoxの uType
 *	@param[in]	UILanguageFile	lngファイル
 *	@param[in]	...				フォーマット引数
 *
 *	info.message を書式化文字列として、
 *	UILanguageFileより後ろの引数を出力する
 *
 *	info.message_key, info.message_default 両方ともNULLの場合
 *		可変引数の1つ目を書式化文字列として使用する
 */
int TTMessageBoxW(HWND hWnd, const TTMessageBoxInfoW *info, UINT uType, const char *UILanguageFile, ...)
{
	const char *section = info->section;
	wchar_t *title;
	if (info->title_key == NULL) {
		title = _wcsdup(info->title_default);
	}
	else {
		title = TTGetLangStrW(section, info->title_key, info->title_default, UILanguageFile);
	}

	wchar_t *message = NULL;
	if (info->message_key == NULL && info->message_default == NULL) {
		wchar_t *format;
		va_list ap;
		va_start(ap, UILanguageFile);
		format = va_arg(ap, wchar_t *);
		vaswprintf(&message, format, ap);
	}
	else {
		wchar_t *format = TTGetLangStrW(section, info->message_key, info->message_default, UILanguageFile);
		va_list ap;
		va_start(ap, UILanguageFile);
		vaswprintf(&message, format, ap);
		free(format);
	}

	int r = _MessageBoxW(hWnd, message, title, uType);

	free(title);
	free(message);

	return r;
}


// from ttxssh
static void format_line_hexdump(char *buf, int buflen, int addr, int *bytes, int byte_cnt)
{
	int i, c;
	char tmp[128];

	buf[0] = 0;

	/* 先頭のアドレス表示 */
	_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%08X : ", addr);
	strncat_s(buf, buflen, tmp, _TRUNCATE);

	/* バイナリ表示（4バイトごとに空白を挿入）*/
	for (i = 0; i < byte_cnt; i++) {
		if (i > 0 && i % 4 == 0) {
			strncat_s(buf, buflen, " ", _TRUNCATE);
		}

		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%02X", bytes[i]);
		strncat_s(buf, buflen, tmp, _TRUNCATE);
	}

	/* ASCII表示部分までの空白を補う */
	_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "   %*s%*s", (16 - byte_cnt) * 2 + 1, " ", (16 - byte_cnt + 3) / 4, " ");
	strncat_s(buf, buflen, tmp, _TRUNCATE);

	/* ASCII表示 */
	for (i = 0; i < byte_cnt; i++) {
		c = bytes[i];
		if (isprint(c)) {
			_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%c", c);
			strncat_s(buf, buflen, tmp, _TRUNCATE);
		}
		else {
			strncat_s(buf, buflen, ".", _TRUNCATE);
		}
	}
}

void DebugHexDump(void (*f)(const char *s), const void *data_, size_t len)
{
	const char *data = (char *)data_;
	char buff[4096];
	int c, addr;
	int bytes[16], *ptr;
	int byte_cnt;
	int i;

	addr = 0;
	byte_cnt = 0;
	ptr = bytes;
	for (i = 0; i < len; i++) {
		c = data[i];
		*ptr++ = c & 0xff;
		byte_cnt++;

		if (byte_cnt == 16) {
			format_line_hexdump(buff, sizeof(buff), addr, bytes, byte_cnt);
			f(buff);

			addr += 16;
			byte_cnt = 0;
			ptr = bytes;
		}
	}

	if (byte_cnt > 0) {
		format_line_hexdump(buff, sizeof(buff), addr, bytes, byte_cnt);
		f(buff);
	}
}

static void OutputDebugHexDumpSub(const char *s)
{
	OutputDebugPrintf("%s\n", s);
}

void OutputDebugHexDump(const void *data, size_t len)
{
	DebugHexDump(OutputDebugHexDumpSub, data, len);
}
