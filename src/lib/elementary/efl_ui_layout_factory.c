#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>
#include "elm_priv.h"

//DEBUG-ONLY
#include "sh_log.h"
//END

#define MY_CLASS EFL_UI_LAYOUT_FACTORY_CLASS
#define MY_CLASS_NAME "Efl.Ui.Layout_Factory"

typedef struct _Efl_Ui_Layout_Factory_Data
{
    Eina_Array *layouts;
    Eina_Hash *connects;
    Eina_Stringshare *klass;
    Eina_Stringshare *group;
    Eina_Stringshare *style;
} Efl_Ui_Layout_Factory_Data;

Eina_Bool
_model_connect(const Eina_Hash *hash EINA_UNUSED, const void *key, void *data, void *fdata)
{
   Eo *layout = fdata;
   Eina_Stringshare *name = key;
   Eina_Stringshare *property = data;

   efl_ui_model_connect(layout, name, property);

   return EINA_TRUE;
}

EOLIAN static Eo *
_efl_ui_layout_factory_efl_object_constructor(Eo *obj, Efl_Ui_Layout_Factory_Data *pd)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));

   pd->klass = NULL;
   pd->group = NULL;
   pd->style = NULL;
   pd->layouts = eina_array_new(8);
   pd->connects = eina_hash_stringshared_new(EINA_FREE_CB(eina_stringshare_del));

   return obj;
}

EOLIAN static void
_efl_ui_layout_factory_efl_object_destructor(Eo *obj, Efl_Ui_Layout_Factory_Data *pd)
{
   Eina_Array_Iterator iterator;
   Eo *layout;
   unsigned int i;

   eina_stringshare_del(pd->klass);
   eina_stringshare_del(pd->group);
   eina_stringshare_del(pd->style);

   EINA_ARRAY_ITER_NEXT(pd->layouts, i, layout, iterator)
     efl_parent_set(layout, NULL);

   eina_array_free(pd->layouts);
   eina_hash_free(pd->connects);

   //SHPRT_ENDL
   //SHCRI("Layout Factory Destroyed");
   //SHPRT_ENDL

   efl_destructor(efl_super(obj, MY_CLASS));
}

int count;

EOLIAN static Efl_Gfx *
_efl_ui_layout_factory_efl_ui_factory_create(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Factory_Data *pd
                                                        , Efl_Model *model, Efl_Gfx *parent)
{
   Efl_Gfx *layout;
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);

   count++;

   if (eina_array_count(pd->layouts))
     {
        layout = eina_array_pop(pd->layouts);
        efl_parent_set(layout, parent);
        efl_ui_view_model_set(layout, model);

         //SHPRT_ENDL
         //SHCRI("Layout Create["_CR_"%p : %d"_CR_"] in Cache",
         //CRBLD(CRRED, BGBLK), layout, count, CRCRI);
         //SHPRT_ENDL
     }
   else
     {
        layout = efl_add(ELM_LAYOUT_CLASS, parent);
        efl_ui_view_model_set(layout, model);
        elm_layout_theme_set(layout, pd->klass, pd->group, pd->style);
        eina_hash_foreach(pd->connects, _model_connect, layout);

         //SHPRT_ENDL
         //SHCRI("Layout Create["_CR_"%p : %d"_CR_"] new",
         //CRBLD(CRRED, BGBLK), layout, count, CRCRI);
         //SHPRT_ENDL
     }

   return layout;
}

EOLIAN static void
_efl_ui_layout_factory_efl_ui_factory_release(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Factory_Data *pd, Efl_Gfx *layout)
{
   efl_ui_view_model_set(layout, NULL);
   eina_array_push(pd->layouts, layout);

   count--;

   //SHPRT_ENDL
   //SHCRI("Layout Released ["_CR_"%p : %d"_CR_"]",
   //CRBLD(CRRED, BGBLK), layout, count, CRCRI);
   //SHPRT_ENDL
}

EOLIAN static void
_efl_ui_layout_factory_efl_ui_model_connect_connect(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Factory_Data *pd
                                                                        , const char *name, const char *property)
{
   Eina_Stringshare *ss_name, *ss_prop;
   ss_name = eina_stringshare_add(name);

   if (property == NULL)
     {
        eina_hash_del(pd->connects, ss_name, NULL);
        return;
     }

   ss_prop = eina_stringshare_add(property);
   eina_stringshare_del(eina_hash_set(pd->connects, ss_name, ss_prop));
}

EOLIAN static void
_efl_ui_layout_factory_theme_config(Eo *obj EINA_UNUSED, Efl_Ui_Layout_Factory_Data *pd
                                        , const char *klass, const char *group, const char *style)
{
   eina_stringshare_replace(&pd->klass, klass);
   eina_stringshare_replace(&pd->group, group);
   eina_stringshare_replace(&pd->style, style);
}

#include "efl_ui_layout_factory.eo.c"
