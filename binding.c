/*
 *  STFL - The Structured Terminal Forms Language/Library
 *  Copyright (C) 2007  Clifford Wolf <clifford@clifford.at>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *  
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *  
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA
 *
 *  binding.c: Helper functions for key bindings and stuff
 */

#include "stfl_internals.h"
#include "stfl_compat.h"

#include <string.h>
#include <stdlib.h>
#include <wchar.h>

wchar_t *stfl_keyname(wchar_t ch, int isfunckey)
{
	if (!isfunckey)
	{
		if (ch == L'\r' || ch == L'\n')
			return compat_wcsdup(L"ENTER");

		if (ch == L' ')
			return compat_wcsdup(L"SPACE");

		if (ch == L'\t')
			return compat_wcsdup(L"TAB");

		if (ch == 27)
			return compat_wcsdup(L"ESC");

		if (ch == 127)
			return compat_wcsdup(L"BACKSPACE");

		wchar_t *ret;
		if (ch < 32) {
			const char * key = keyname(ch);
			unsigned int keylen = strlen(key) + 1, i;
			ret = malloc(keylen * sizeof(wchar_t));
			for (i=0; i<keylen; i++)
				ret[i] = key[i];
		} else {
			ret = compat_wcsdup(L" ");
			ret[0] = ch;
		}
		return ret;
	}

	if (KEY_F(0) <= ch && ch <= KEY_F(63)) {
		wchar_t *name = malloc(4 * sizeof(wchar_t));
		swprintf(name, 4, L"F%d", ch - KEY_F0);
		return name;
	}

	const char *event_c = keyname(ch);

	if (!event_c)
		return compat_wcsdup(L"UNKNOWN");

	if (!strncmp(event_c, "KEY_", 4))
		event_c += 4;
	
	int event_len = strlen(event_c) + 1, i;
	wchar_t *event = malloc(event_len * sizeof(wchar_t));

	for (i=0; i<event_len; i++)
		event[i] = event_c[i];

	return event;
}

int stfl_matchbind(struct stfl_widget *w, wchar_t ch, int isfunckey, wchar_t *name, wchar_t *auto_desc)
{
	wchar_t *event = stfl_keyname(ch, isfunckey);
	int event_len = wcslen(event);

	int kvname_len = wcslen(name) + 6;
	wchar_t kvname[kvname_len];
	swprintf(kvname, kvname_len, L"bind_%ls", name);

	if (stfl_widget_getkv_int(w, L"autobind", 1) == 0)
		auto_desc = L"";

	const wchar_t* desc = stfl_widget_getkv_str(w, kvname, auto_desc);
	int retry_auto_desc = 0;

retry_auto_desc:
	while (*desc) {
		desc += wcsspn(desc, L" \t\n\r");
		int len = wcscspn(desc, L" \t\n\r");
		if (!retry_auto_desc && len == 2 && !wcsncmp(desc, L"**", 2))
			retry_auto_desc = 1;
		if (len > 0 && len == event_len && !wcsncmp(desc, event, len)) {
			free(event);
			return 1;
		}
		desc += len;
	}

	if (retry_auto_desc > 0) {
		retry_auto_desc = -1;
		desc = auto_desc;
		goto retry_auto_desc;
	}

	free(event);
	return 0;
}

