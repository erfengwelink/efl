#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>
#include "elm_priv.h"

#include "efl_page_transition.h"

#define MY_CLASS EFL_PAGE_TRANSITION_CLASS


EOLIAN static void
_efl_page_transition_bind(Eo *obj EINA_UNUSED,
                          Efl_Page_Transition_Data *_pd EINA_UNUSED,
                          Eo *pager EINA_UNUSED)
{
}

EOLIAN static void
_efl_page_transition_init(Eo *obj EINA_UNUSED,
                          Efl_Page_Transition_Data *_pd EINA_UNUSED)
{
}

EOLIAN static void
_efl_page_transition_update(Eo *obj EINA_UNUSED,
                            Efl_Page_Transition_Data *_pd EINA_UNUSED)
{
}

EOLIAN static void
_efl_page_transition_finish(Eo *obj EINA_UNUSED,
                            Efl_Page_Transition_Data *_pd EINA_UNUSED)
{
}

EOLIAN static Eo *
_efl_page_transition_efl_object_constructor(Eo *obj,
                                            Efl_Page_Transition_Data *sd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));

   return obj;
}

#include "efl_page_transition.eo.c"
