/*
 *  STFL - The Structured Terminal Forms Language/Library
 *  Copyright (C) 2006, 2007  Clifford Wolf <clifford@clifford.at>
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
 *  wt_box.c: Widget types 'hbox' and 'vbox'
 */

#include "stfl_internals.h"

#include <stdlib.h>
#include <string.h>

struct box_data {
	char type;
};

static void wt_vbox_init(struct stfl_widget *w)
{
	struct box_data *d = calloc(1, sizeof(struct box_data));
	d->type = 'V';
	w->internal_data = d;
}

static void wt_hbox_init(struct stfl_widget *w)
{
	struct box_data *d = calloc(1, sizeof(struct box_data));
	d->type = 'H';
	w->internal_data = d;
}

static void wt_box_done(struct stfl_widget *w)
{
	free(w->internal_data);
}

static void wt_box_prepare(struct stfl_widget *w, struct stfl_form *f)
{
	struct box_data *d = w->internal_data;

	w->min_w = 0;
	w->min_h = 0;

	struct stfl_widget *c = w->first_child;
	while (c) {
		if (stfl_widget_getkv_int(c, L".display", 1)) {
			c->type->f_prepare(c, f);
			if (d->type == 'H') {
				if (w->min_h < c->min_h)
					w->min_h = c->min_h;
				w->min_w += c->min_w;
			} else {
				if (w->min_w < c->min_w)
					w->min_w = c->min_w;
				w->min_h += c->min_h;
			}
		}
		c = c->next_sibling;
	}
}

static void wt_box_draw(struct stfl_widget *w, struct stfl_form *f, WINDOW *win)
{
	struct box_data *d = w->internal_data;

	int num_dyn_children = 0;
	int min_w = 0, min_h = 0;
	int i, j;

	struct stfl_widget *c = w->first_child;
	while (c)
	{
		if (stfl_widget_getkv_int(c, L".display", 1))
		{
			int size_w = stfl_widget_getkv_int(c, L".width", 0);
			if (size_w < c->min_w) size_w = c->min_w;

			int size_h = stfl_widget_getkv_int(c, L".height", 0);
			if (size_h < c->min_h) size_h = c->min_h;

			if (wcschr(stfl_widget_getkv_str(c, L".expand", L"vh"),
					d->type == 'H' ? 'h' : 'v'))
				num_dyn_children++;

			if (d->type == 'H') {
				min_w += size_w;
				if (min_h < size_h)
					min_h = size_h;
			} else {
				min_h += size_h;
				if (min_w < size_w)
					min_w = size_w;
			}
		}
		c = c->next_sibling;
	}

	int box_x = w->x, box_y = w->y;
	int box_w = w->w, box_h = w->h;

	stfl_widget_style(w, f, win);
	for (i=box_x; i<box_x+box_w; i++)
	for (j=box_y; j<box_y+box_h; j++)
		mvwaddch(win, j, i, ' ');

	const wchar_t *tie = stfl_widget_getkv_str(w, L"tie", L"lrtb");

	if (!wcschr(tie, L'l') && !wcschr(tie, L'r')) box_x += (box_w-min_w)/2;
	if (!wcschr(tie, L'l') &&  wcschr(tie, L'r')) box_x += box_w-min_w;
	if (!wcschr(tie, L'l') || !wcschr(tie, L'r')) box_w = min_w;

	if (!wcschr(tie, L't') && !wcschr(tie, L'b')) box_y += (box_h-min_h)/2;
	if (!wcschr(tie, L't') &&  wcschr(tie, L'b')) box_y += box_h-min_h;
	if (!wcschr(tie, L't') || !wcschr(tie, L'b')) box_h = min_h;

	int sizes_extra = (d->type == 'H' ? box_w - min_w : box_h - min_h);
	int cursor = (d->type == 'H' ? box_x : box_y);

	c = w->first_child;
	for (i=0; c; i++)
	{
		if (stfl_widget_getkv_int(c, L".display", 1))
		{
			int size = stfl_widget_getkv_int(c,
					d->type == 'H' ? L".width" : L".height", 0);

			if (size < (d->type == 'H' ? c->min_w : c->min_h))
				size = d->type == 'H' ? c->min_w : c->min_h;

			if (wcschr(stfl_widget_getkv_str(c, L".expand", L"vh"),
					d->type == 'H' ? 'h' : 'v')) {
				int extra = sizes_extra / num_dyn_children--;
				sizes_extra -= extra;
				size += extra;
			}
			
			if (d->type == 'H') {
				c->y = box_y;
				c->x = cursor;
				c->h = box_h;
				c->w = size;
				cursor += c->w;
			} else {
				c->x = box_x;
				c->y = cursor;
				c->w = box_w;
				c->h = size;
				cursor += c->h;
			}

			tie = stfl_widget_getkv_str(c, L".tie", L"lrtb");

			if (!wcschr(tie, L'l') && !wcschr(tie, L'r')) c->x += (c->w - c->min_w)/2;
			if (!wcschr(tie, L'l') &&  wcschr(tie, L'r')) c->x += c->w - c->min_w;
			if (!wcschr(tie, L'l') || !wcschr(tie, L'r')) c->w = c->min_w;

			if (!wcschr(tie, L't') && !wcschr(tie, L'b')) c->y += (c->h - c->min_h)/2;
			if (!wcschr(tie, L't') &&  wcschr(tie, L'b')) c->y += c->h - c->min_h;
			if (!wcschr(tie, L't') || !wcschr(tie, L'b')) c->h = c->min_h;

			c->type->f_draw(c, f, win);
		}
		c = c->next_sibling;
	}
}

static int wt_box_process(struct stfl_widget *w, struct stfl_widget *fw, struct stfl_form *f, wchar_t ch, int isfunckey)
{
	struct box_data *d = w->internal_data;

	if (d->type == 'H')
	{
		if (stfl_matchbind(w, ch, isfunckey, L"left", L"LEFT"))
			return stfl_focus_prev(w, fw, f);
		if (stfl_matchbind(w, ch, isfunckey, L"right", L"RIGHT"))
			return stfl_focus_next(w, fw, f);
	}

	if (d->type == 'V')
	{
		if (stfl_matchbind(w, ch, isfunckey, L"up", L"UP"))
			return stfl_focus_prev(w, fw, f);
		if (stfl_matchbind(w, ch, isfunckey, L"down", L"DOWN"))
			return stfl_focus_next(w, fw, f);
	}

	return 0;
}

struct stfl_widget_type stfl_widget_type_vbox = {
	L"vbox",
	wt_vbox_init,
	wt_box_done,
	0, // f_enter 
	0, // f_leave
	wt_box_prepare,
	wt_box_draw,
	wt_box_process
};

struct stfl_widget_type stfl_widget_type_hbox = {
	L"hbox",
	wt_hbox_init,
	wt_box_done,
	0, // f_enter 
	0, // f_leave
	wt_box_prepare,
	wt_box_draw,
	wt_box_process
};

