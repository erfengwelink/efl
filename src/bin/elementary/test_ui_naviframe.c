#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>

static void
_clicked_cb(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Eo *fs = data;
   efl_ui_frame_stack_pop(fs);
}

static Eo *
_naviframe_create(Eo *parent, const char *title)
{
   Eo *nf = efl_add(EFL_UI_NAVIFRAME_CLASS, parent);
   efl_text_set(efl_part(nf, "title"), title);

   Eo *btn = efl_add(EFL_UI_BUTTON_CLASS, nf);
   efl_text_set(btn, "pop frame stack");
   efl_event_callback_add(btn, EFL_UI_EVENT_CLICKED, _clicked_cb, parent);

   efl_content_set(nf, btn);

   return nf;
}

void
test_ui_naviframe(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *win = efl_add(EFL_UI_WIN_CLASS, NULL,
                     efl_text_set(efl_added, "Efl.Ui.Naviframe"),
                     efl_ui_win_autodel_set(efl_added, EINA_TRUE));

   efl_gfx_size_set(win, EINA_SIZE2D(500, 500));

   Eo *fs = efl_add(EFL_UI_FRAME_STACK_CLASS, win);
   efl_content_set(win, fs);

   Eo *nf1 = _naviframe_create(fs, "1st Frame");
   efl_ui_frame_stack_push(fs, nf1);

   Eo *nf2 = _naviframe_create(fs, "2nd Frame");
   efl_ui_frame_stack_push(fs, nf2);

   Eo *nf3 = _naviframe_create(fs, "3rd Frame");
   efl_ui_frame_stack_push(fs, nf3);
}
