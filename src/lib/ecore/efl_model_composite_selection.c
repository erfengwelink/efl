#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Eina.h"
#include "Efl.h"
#include <Ecore.h>
#include "Eo.h"

#include "efl_model_composite_selection.eo.h"
#include "efl_model_accessor_view_private.h"

typedef struct _Efl_Model_Composite_Selection_Data Efl_Model_Composite_Selection_Data;
typedef struct _Efl_Model_Composite_Selection_Children_Data Efl_Model_Composite_Selection_Children_Data;

struct _Efl_Model_Composite_Selection_Data
{
   unsigned long last;

   Eina_Bool exclusive : 1;
   Eina_Bool none : 1;
};

struct _Efl_Model_Composite_Selection_Children_Data
{
};

static Eo*
_efl_model_composite_selection_efl_object_constructor(Eo *obj,
                                                      Efl_Model_Composite_Selection_Data *pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, EFL_MODEL_COMPOSITE_SELECTION_CLASS));

   efl_model_composite_boolean_add(obj, "selected", EINA_FALSE);

   pd->last = -1;

   return obj;
}

static Eina_Value
_commit_change(void *data, const Eina_Value v, const Eina_Future *dead_future EINA_UNUSED)
{
   Efl_Model_Composite_Selection_Data *pd;
   Efl_Model *child = data;
   Eina_Value *vc = NULL;
   Eina_Value *selected = NULL;
   Eina_Bool selflag = EINA_FALSE;

   if (v.type == EINA_VALUE_TYPE_ERROR)
     goto on_error;

   vc = efl_model_property_get(child, "Child.index");
   selected = efl_model_property_get(child, "selected");

   pd = efl_data_scope_get(efl_parent_get(child), EFL_MODEL_COMPOSITE_SELECTION_CLASS);
   if (!pd) goto on_error;

   eina_value_bool_get(selected, &selflag);
   if (selflag)
     {
        // select case
        pd->none = EINA_FALSE;
        eina_value_ulong_get(vc, &pd->last);
        efl_event_callback_call(child, EFL_MODEL_COMPOSITE_SELECTION_EVENT_SELECTED, child);
     }
   else
     {
        // unselect case
        unsigned long last;

        eina_value_ulong_get(vc, &last);
        if (pd->last == last)
          {
             pd->last = 0;
             pd->none = EINA_TRUE;
          }
        efl_event_callback_call(child, EFL_MODEL_COMPOSITE_SELECTION_EVENT_UNSELECTED, child);
     }

 on_error:
   eina_value_free(vc);
   eina_value_free(selected);
   efl_unref(child);
   return v;
}

static Efl_Model *
_select_child_get(const Eina_Value *array, unsigned int idx)
{
   Eina_Value vo = EINA_VALUE_EMPTY;

   if (eina_value_type_get(array) != EINA_VALUE_TYPE_ARRAY)
     return NULL;

   if (idx < eina_value_array_count(array))
     return NULL;

   eina_value_array_get(array, idx, &vo);

   return eina_value_object_get(&vo);
}

static Eina_Future *
_check_child_change(Efl_Model *child, Eina_Bool value)
{
   Eina_Future *r = NULL;
   Eina_Value *prev;
   Eina_Bool prevflag = EINA_FALSE;

   prev = efl_model_property_get(child, "selected");
   eina_value_bool_get(prev, &prevflag);
   eina_value_free(prev);

   if (prevflag & value)
     {
        r = eina_future_resolved(efl_loop_future_scheduler_get(child),
                                 eina_value_bool_init(value));
     }
   else
     {
        r = efl_model_property_set(child, "selected", eina_value_bool_new(!!value));
        r = eina_future_then(r, _commit_change, child);
     }

   return r;
}

static Eina_Future *
_select_child(Efl_Model *child)
{
   return _check_child_change(child, EINA_TRUE);
}

static Eina_Future *
_unselect_child(Efl_Model *child)
{
   return _check_child_change(child, EINA_FALSE);
}

static Eina_Value
_select_slice_then(void *data EINA_UNUSED,
                   const Eina_Value v,
                   const Eina_Future *dead_future EINA_UNUSED)
{
   Efl_Model *child = NULL;
   Eina_Future *r;

   child = _select_child_get(&v, 0);
   if (!child) goto on_error;

   r = _select_child(child);
   return eina_future_as_value(r);

 on_error:
   return v;
}

static Eina_Value
_unselect_slice_then(void *data EINA_UNUSED,
                     const Eina_Value v,
                     const Eina_Future *dead_future EINA_UNUSED)
{
   Efl_Model *child = NULL;
   Eina_Future *r;

   child = _select_child_get(&v, 0);
   if (!child) goto on_error;

   r = _unselect_child(child);
   return eina_future_as_value(r);

 on_error:
   return v;
}

static Eina_Future *
_efl_model_composite_selection_efl_model_property_set(Eo *obj,
                                                      Efl_Model_Composite_Selection_Data *pd,
                                                      const char *property, Eina_Value *value)
{
   Eina_Value vf = EINA_VALUE_EMPTY;

   if (!strcmp("exclusive", property))
     {
        Eina_Bool exclusive = pd->exclusive;

        vf = eina_value_bool_init(exclusive);
        eina_value_convert(value, &vf);
        eina_value_bool_get(&vf, &exclusive);

        pd->exclusive = !!exclusive;

        return eina_future_resolved(efl_loop_future_scheduler_get(obj), vf);
     }

   if (!strcmp("selected", property))
     {
        Eina_Value vl = EINA_VALUE_EMPTY;
        unsigned long l = 0;
        Eina_Bool success = EINA_TRUE;

        vl = eina_value_ulong_init(0);
        success &= eina_value_convert(value, &vl);
        success &= eina_value_ulong_get(&vl, &l);
        if (!success)
          return eina_future_rejected(efl_loop_future_scheduler_get(obj), EFL_MODEL_ERROR_INCORRECT_VALUE);

        return efl_future_Eina_FutureXXX_then(obj,
                                              eina_future_then(efl_model_children_slice_get(obj, l, 1),
                                                               _select_slice_then, obj));
     }

   return efl_model_property_set(efl_super(obj, EFL_MODEL_COMPOSITE_SELECTION_CLASS), property, value);
}

static Eina_Value *
_efl_model_composite_selection_efl_model_property_get(Eo *obj, Efl_Model_Composite_Selection_Data *pd, const char *property)
{
   if (!strcmp("exclusive", property))
     return eina_value_bool_new(pd->exclusive);
   if (!strcmp("selected", property))
     {
        if (pd->none)
          return eina_value_error_new(EFL_MODEL_ERROR_INCORRECT_VALUE);
        else
          return eina_value_ulong_new(pd->last);
     }

   return efl_model_property_get(efl_super(obj, EFL_MODEL_COMPOSITE_SELECTION_CLASS), property);
}

static Eina_Value
_regenerate_error(void *data,
                  const Eina_Value v,
                  const Eina_Future *dead_future EINA_UNUSED)
{
   Eina_Error *error = data;
   Eina_Value r = v;

   if (v.type == EINA_VALUE_TYPE_ERROR)
     goto cleanup;

   r = eina_value_error_init(*error);

 cleanup:
   free(error);

   return r;
}

static Eina_Value
_untangle_array(void *data,
                const Eina_Value v,
                const Eina_Future *dead_future EINA_UNUSED)
{
   Efl_Model *child = data;
   Eina_Value va = EINA_VALUE_EMPTY;
   Eina_Future *f;

   if (v.type == EINA_VALUE_TYPE_ERROR)
     {
        // We need to roll back the change, which means in this
        // case to unselect this child as this is the only case
        // where we could end up here.
        Eina_Error *error = calloc(1, sizeof (Eina_Error));

        f = efl_model_property_set(efl_super(child, EFL_MODEL_COMPOSITE_SELECTION_CHILDREN_CLASS),
                                   "selected", eina_value_bool_new(EINA_FALSE));
        // Once this is done, we need to repropagate the error
        eina_value_error_get(&v, error);
        f = eina_future_then(f, _regenerate_error, error);

        return eina_future_as_value(f);
     }

   // Only return the commit change, not the result of the unselect
   eina_value_array_get(&v, 0, &va);
   return va;
}

static Eina_Future *
_efl_model_composite_selection_children_efl_model_property_set(Eo *obj,
                                                               Efl_Model_Composite_Selection_Children_Data *pd EINA_UNUSED,
                                                               const char *property, Eina_Value *value)
{
   Eina_Value *ve = NULL;
   Eina_Value *vb = NULL;
   Eina_Value lvb = EINA_VALUE_EMPTY;
   Eina_Bool success = EINA_TRUE;
   Eina_Bool exclusive = EINA_FALSE;
   Eina_Bool prevflag = EINA_FALSE, newflag = EINA_FALSE;
   Eina_Future *chain;

   if (strcmp("selected", property))
     return efl_model_property_set(efl_super(obj, EFL_MODEL_COMPOSITE_SELECTION_CHILDREN_CLASS),
                                   property, value);

   vb = efl_model_property_get(obj, "selected");
   success &= eina_value_bool_get(vb, &prevflag);
   eina_value_free(vb);

   lvb = eina_value_bool_init(prevflag);
   success &= eina_value_convert(value, &lvb);
   success &= eina_value_bool_get(&lvb, &newflag);
   eina_value_flush(&lvb);

   if (!success)
     return eina_future_rejected(efl_loop_future_scheduler_get(obj), EFL_MODEL_ERROR_INCORRECT_VALUE);

   // Nothing changed
   if (newflag == prevflag)
     return eina_future_resolved(efl_loop_future_scheduler_get(obj),
                                 eina_value_bool_init(newflag));

   ve = efl_model_property_get(efl_parent_get(obj), "exclusive");
   eina_value_bool_get(ve, &exclusive);
   eina_value_free(ve);

   // First store the new value in the boolean model we inherit from
   chain = efl_model_property_set(efl_super(obj, EFL_MODEL_COMPOSITE_SELECTION_CHILDREN_CLASS),
                                  "selected", value);

   // Now act !
   if (exclusive)
     {
        Eina_Value *vs;
        unsigned long selected = 0;
        Eina_Bool success;

        // We are here either, because we weren't and are after this call
        // or because we were selected and are not anymore. In the later case,
        // there is nothing special to do, just normal commit change will do.

        if (newflag)
          {
             // In this case we need to first unselect the previously selected one
             // and then commit the change to this one.

             // Fetch the one to unselect
             vs = efl_model_property_get(efl_parent_get(obj), "selected");
             // Check if there was any selected
             if (eina_value_type_get(vs) == EINA_VALUE_TYPE_ERROR)
               {
                  eina_value_free(vs);
                  goto commit_change;
               }
             success = eina_value_ulong_get(vs, &selected);
             eina_value_free(vs);

             if (!success)
               return eina_future_rejected(efl_loop_future_scheduler_get(obj),
                                           EFL_MODEL_ERROR_INCORRECT_VALUE);

             // There was, need to unselect the previous one along setting the new value
             chain = eina_future_all(chain,
                                     eina_future_then(efl_model_children_slice_get(efl_parent_get(obj), selected, 1),
                                                      _unselect_slice_then, NULL));

             chain = eina_future_then(chain, _untangle_array, obj);
          }
     }

 commit_change:
   chain = eina_future_then(chain, _commit_change, obj);

   return efl_future_Eina_FutureXXX_then(obj, chain);
}

static Eina_Value
_slice_get(void *data,
           const Eina_Value v,
           const Eina_Future *dead_future EINA_UNUSED)
{
   Efl_Model *parent = data;
   unsigned int length, it;
   Eina_Value child = EINA_VALUE_EMPTY;
   Eina_Value r = EINA_VALUE_EMPTY;

   if (v.type == EINA_VALUE_TYPE_ERROR)
     return v;

   eina_value_array_setup(&r, EINA_VALUE_TYPE_OBJECT, 4);

   EINA_VALUE_ARRAY_FOREACH(&v, length, it, &child)
     {
        Eo *composited = NULL;
        Eo *compositing;

        composited = eina_value_object_get(&child);
        if (!composited) continue ;

        compositing = efl_add(EFL_MODEL_COMPOSITE_SELECTION_CHILDREN_CLASS, parent,
                              efl_composite_attach(efl_added, efl_ref(composited)));
        eina_value_array_append(&r, compositing);
     }

   return r;
}

static Eina_Future *
_efl_model_composite_selection_efl_model_children_slice_get(Eo *obj,
                                                            Efl_Model_Composite_Selection_Data *pd EINA_UNUSED,
                                                            unsigned int start, unsigned int count)
{
   Eina_Future *f;

   f = efl_model_children_slice_get(efl_super(obj, EFL_MODEL_COMPOSITE_SELECTION_CLASS),
                                    start, count);
   f = eina_future_then(f, _slice_get, obj);

   return efl_future_Eina_FutureXXX_then(obj, f);
}

#include "efl_model_composite_selection.eo.c"
#include "efl_model_composite_selection_children.eo.c"
