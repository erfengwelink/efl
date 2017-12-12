#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>

#include "elm_priv.h"
#include "efl_ui_frame_stack_private.h"

#define MY_CLASS EFL_UI_FRAME_STACK_CLASS
#define MY_CLASS_NAME "Efl.Ui.Frame_Stack"

EOLIAN static void
_efl_ui_frame_stack_push(Eo *obj, Efl_Ui_Frame_Stack_Data *pd, Eo *frame)
{
   if (!frame) return;
/*
   Frame_Data *fd = NULL;
   EINA_INLIST_FOREACH(pd->stack, fd)
      if (fd->frame == frame)
        {
           ERR("Same frame exists in the stack!");
           return;
        }
*/
   Frame_Data *top_fd = NULL;
   if (pd->stack)
     top_fd = EINA_INLIST_CONTAINER_GET(pd->stack->last, Frame_Data);

   Frame_Data *fd = calloc(1, sizeof(Frame_Data));
   if (!fd)
     {
        ERR("Memory allocation error!");
        return;
     }

   fd->frame = frame;
   pd->stack = eina_inlist_append(pd->stack, EINA_INLIST_GET(fd));
   evas_object_smart_member_add(frame, obj);
   elm_widget_resize_object_set(obj, frame);

   //FIXME: Apply transition here
   evas_object_raise(frame);

   if (top_fd)
     efl_gfx_visible_set(top_fd->frame, EINA_FALSE);
}

EOLIAN static Eo *
_efl_ui_frame_stack_pop(Eo *obj EINA_UNUSED, Efl_Ui_Frame_Stack_Data *pd)
{
   if (!pd->stack) return NULL;

   Frame_Data *top_fd = EINA_INLIST_CONTAINER_GET(pd->stack->last, Frame_Data);
   if (!top_fd) return NULL;

   pd->stack = eina_inlist_remove(pd->stack, EINA_INLIST_GET(top_fd));

   //FIXME: Apply transition here
   if (pd->stack)
     {
        Frame_Data *prev_fd = EINA_INLIST_CONTAINER_GET(pd->stack->last, Frame_Data);
        if (prev_fd)
          {
             elm_widget_resize_object_set(obj, prev_fd->frame);
             efl_gfx_visible_set(prev_fd->frame, EINA_TRUE);
             evas_object_raise(prev_fd->frame);
          }
     }

   if (top_fd)
     {
        efl_del(top_fd->frame);
        free(top_fd);
     }

   return NULL;
}

EOLIAN static Eo *
_efl_ui_frame_stack_efl_object_constructor(Eo *obj, Efl_Ui_Frame_Stack_Data *pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME);

   return obj;
}

#include "efl_ui_frame_stack.eo.c"
