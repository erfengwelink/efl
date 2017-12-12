#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>

#include "elm_priv.h"
#include "efl_ui_naviframe_private.h"
#include "efl_ui_naviframe_part.eo.h"
#include "elm_part_helper.h"

#define MY_CLASS EFL_UI_NAVIFRAME_CLASS
#define MY_CLASS_NAME "Efl.Ui.Naviframe"

EOLIAN static Eo *
_efl_ui_naviframe_efl_object_constructor(Eo *obj, Efl_Ui_Naviframe_Data *pd EINA_UNUSED)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, NULL);

   if (!elm_widget_theme_klass_get(obj))
     elm_widget_theme_klass_set(obj, "naviframe");
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME);

   elm_widget_sub_object_parent_add(obj);

   elm_widget_can_focus_set(obj, EINA_TRUE);
   if (!elm_widget_theme_object_set(obj, wd->resize_obj,
                                       elm_widget_theme_klass_get(obj),
                                       elm_widget_theme_element_get(obj),
                                       elm_widget_theme_style_get(obj)))
     CRI("Failed to set layout!");

   return obj;
}


/* Standard widget overrides */

ELM_PART_CONTENT_DEFAULT_GET(efl_ui_naviframe, "elm.swallow.content")
ELM_PART_CONTENT_DEFAULT_IMPLEMENT(efl_ui_naviframe, Efl_Ui_Naviframe_Data)


static Eina_Bool
_part_is_efl_ui_naviframe_part(const Eo *obj EINA_UNUSED, const char *part)
{
   return (eina_streq(part, "title") || eina_streq(part, "elm.text.title"));
}

static Eina_Bool
_efl_ui_naviframe_text_set(Eo *obj, Efl_Ui_Naviframe_Data *pd, const char *part, const char *label)
{
   if (eina_streq(part, "title") || eina_streq(part, "elm.text.title"))
     {
        Eina_Bool changed = eina_stringshare_replace(&pd->title_text, label);
        if (changed)
          {
             efl_text_set(efl_part(efl_super(obj, MY_CLASS), "title"), label);
             if (label)
               elm_layout_signal_emit(obj, "elm,title,show", "elm");
             else
               elm_layout_signal_emit(obj, "elm,title,hide", "elm");

             ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, EINA_FALSE);
             edje_object_message_signal_process(wd->resize_obj);
             elm_layout_sizing_eval(obj);
          }
     }
   else
     efl_text_set(efl_part(efl_super(obj, MY_CLASS), part), label);

   return EINA_TRUE;
}

const char *
_efl_ui_naviframe_text_get(Eo *obj EINA_UNUSED, Efl_Ui_Naviframe_Data *pd, const char *part)
{
   if (eina_streq(part, "title") || eina_streq(part, "elm.text.title"))
     {
        if (pd->title_text)
          return pd->title_text;

        return NULL;
     }

   return efl_text_get(efl_part(efl_super(obj, MY_CLASS), part));
}


/* Efl.Part begin */
ELM_PART_OVERRIDE_PARTIAL(efl_ui_naviframe, EFL_UI_NAVIFRAME,
                          Efl_Ui_Naviframe_Data, _part_is_efl_ui_naviframe_part)
ELM_PART_OVERRIDE_TEXT_SET(efl_ui_naviframe, EFL_UI_NAVIFRAME, Efl_Ui_Naviframe_Data)
ELM_PART_OVERRIDE_TEXT_GET(efl_ui_naviframe, EFL_UI_NAVIFRAME, Efl_Ui_Naviframe_Data)
#include "efl_ui_naviframe_part.eo.c"

/* Efl.Part end */

#include "efl_ui_naviframe.eo.c"
