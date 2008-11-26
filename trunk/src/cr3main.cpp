/*
    First version of CR3 for EWL, based on etimetool example by Lunohod
*/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <Ewl.h>
#include <crengine.h>



typedef enum {
	H1,
	H2,
	S,
	M1,
	M2,
} t_state;

int state;
int h, m;

Ewl_Widget *clock_entry;

void keypress_cb(Ewl_Widget *w, void *event, void *data)
{
	char *text;
	char *s;
	int lh, lm;

	Ewl_Event_Key_Up *e = (Ewl_Event_Key_Up*)event;

	const char *k = e->base.keyname;
/*
	if(isdigit(k[0]) && !k[1]) {
		text = ewl_text_text_get(&EWL_ENTRY(clock_entry)->text);
		text[state] = k[0];
		if(state == H1 && text[H2] > '3')
			text[H2] = '0';

		sscanf(text, "%u:%u", &lh, &lm);

//		printf("h: %d, m: %d\n", h, m);

		if(lh > 23 || lm > 59)
			return;

		h = lh;
		m = lm;

		ewl_text_text_set(&EWL_ENTRY(clock_entry)->text, text);

		state++;
		if(state == S)
			state++;
		state %= 5;
	} else if(!strcmp(k, "Return")) {
	 	asprintf(&s, "date 0101%02d%02d2008", h, m);
		system(s);
		if(s)
			free(s);
		exit(0);
	} else 
	*/
	if(!strcmp(k, "Escape")) {
		exit(0);
	}
}

static void reveal_cb(Ewl_Widget *w, void *ev, void *data) {
	ewl_window_move(EWL_WINDOW(w), (600 - CURRENT_W(w)) / 2, (800 - CURRENT_H(w)) / 2);
	ewl_window_keyboard_grab_set(EWL_WINDOW(w), 1);
}

int main(int argc, char **argv)
{
	InitFontManager( lString8() );
	Ewl_Widget *win, *vbox, *tmpw, *tmpw2, *label;

	if(!ewl_init(&argc, argv))
		return 1;

	//ewl_theme_theme_set("/usr/share/FBReader/themes/oitheme.edj");

	win = ewl_window_new();
	ewl_window_title_set(EWL_WINDOW(win), "EWL_WINDOW");
	ewl_window_name_set(EWL_WINDOW(win), "EWL_WINDOW");
	ewl_window_class_set(EWL_WINDOW(win), "etimetool");
	ewl_callback_append(win, EWL_CALLBACK_KEY_UP, keypress_cb, NULL);
	ewl_callback_append(win, EWL_CALLBACK_REVEAL, reveal_cb, NULL);
	ewl_widget_show(win);
/*
	vbox = ewl_vbox_new();
	ewl_container_child_append(EWL_CONTAINER(win), vbox);
	ewl_object_fill_policy_set(EWL_OBJECT(vbox), EWL_FLAG_FILL_FILL);
	ewl_widget_show(vbox);

	tmpw = ewl_hbox_new();
	ewl_container_child_append(EWL_CONTAINER(vbox), tmpw);
	ewl_object_fill_policy_set(EWL_OBJECT(tmpw), EWL_FLAG_FILL_FILL);
	ewl_widget_show(tmpw);

	label = ewl_label_new();
	ewl_container_child_append(EWL_CONTAINER(tmpw), label);
	ewl_theme_data_str_set(EWL_WIDGET(label), "/label/group", "ewl/label/dlg_label");
	ewl_theme_data_str_set(EWL_WIDGET(label), "/label/textpart", "ewl/label/dlg_label/text");
	ewl_label_text_set(EWL_LABEL(label), "Set clock:");
	ewl_widget_show(label);

	tmpw = ewl_hbox_new();
	ewl_container_child_append(EWL_CONTAINER(vbox), tmpw);
	ewl_object_fill_policy_set(EWL_OBJECT(tmpw), EWL_FLAG_FILL_FILL);
	ewl_widget_show(tmpw);
	clock_entry = ewl_entry_new();
	ewl_container_child_append(EWL_CONTAINER(tmpw), clock_entry);
	ewl_widget_name_set(clock_entry, "clock_entry");
	ewl_theme_data_str_set(EWL_WIDGET(clock_entry),"/entry/group","ewl/dlg_entry");
	ewl_theme_data_str_set(EWL_WIDGET(clock_entry),"/entry/cursor/group","ewl/dlg_entry/cursor");
	ewl_theme_data_str_set(EWL_WIDGET(clock_entry),"/entry/selection_area/group","ewl/dlg_entry/selection");
	ewl_text_text_set(&EWL_ENTRY(clock_entry)->text, "00:00");
	ewl_entry_editable_set(EWL_ENTRY(clock_entry), 0);
	ewl_widget_show(clock_entry);
	ewl_widget_focus_send(clock_entry);
*/

	state = H1;
	h = 0;
	m = 0;

	ewl_main();

	return 1;
}

