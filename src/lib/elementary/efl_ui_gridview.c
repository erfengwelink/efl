#include "efl_ui_gridview_private.h"
//#include "efl_ui_gridview.eo.h"

#include <assert.h>

#define EFL_UI_GRIDVIEW_PROTECTED

#define MY_CLASS EFL_UI_GRIDVIEW_CLASS
#define MY_CLASS_NAME "Efl.Ui.Gridview"
#define MY_PAN_CLASS EFL_UI_GRIDVIEW_PAN_CLASS

#define SIG_CHILD_ADDED "child,added"
#define SIG_CHILD_REMOVED "child,removed"
#define SELECTED_PROP "selected"
#define AVERAGE_SIZE_INIT 30


static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_CHILD_ADDED, ""},
   {SIG_CHILD_REMOVED, ""},
   {NULL, NULL}
};

static void _efl_ui_gridview_item_select_set(Efl_Ui_Gridview_Item *, Eina_Bool);
static Eina_Bool _efl_ui_gridview_item_select_clear(Eo *);
static Efl_Ui_Gridview_Item *_child_new(Efl_Ui_Gridview_Data *, Efl_Model *);
static void _child_remove(Efl_Ui_Gridview_Data *, Efl_Ui_Gridview_Item *);
static void _item_calc(Efl_Ui_Gridview_Data *, Efl_Ui_Gridview_Item *);
static void _layout_realize(Efl_Ui_Gridview_Data *, Efl_Ui_Gridview_Item *);
static void _layout_unrealize(Efl_Ui_Gridview_Data *, Efl_Ui_Gridview_Item *);
static Eina_Bool _load_items(Eo *, Efl_Ui_Gridview_Data *, Eina_Bool);
static Eina_Bool _view_update_internal(Eo *);

static Eina_Bool _key_action_move(Evas_Object *obj, const char *params);
static Eina_Bool _key_action_select(Evas_Object *obj, const char *params);
static Eina_Bool _key_action_escape(Evas_Object *obj, const char *params);

static const Elm_Action key_actions[] = {
   {"move", _key_action_move},
   {"select", _key_action_select},
   {"escape", _key_action_escape},
   {NULL, NULL}
};

// Debug Macros : Should be removed before master merge
#include "sh_log.h"
// End

//FIXME : Need to change new direction enum interface. orient should means degree.
static inline Eina_Bool
_horiz(Efl_Orient dir)
{
   return dir % 180 == EFL_ORIENT_RIGHT;
}

EOLIAN static void
_efl_ui_gridview_pan_elm_pan_pos_set(Eo *obj,
                                     Efl_Ui_Gridview_Pan_Data * ppd,
                                     Evas_Coord x,
                                     Evas_Coord y)
{
   Efl_Ui_Gridview_Data *pd = ppd->wpd;
  /*
   Evas_Coord ox, oy, ow, oh, cw;
   Efl_Ui_Gridview_Item *litem;
   Eina_Array_Iterator iterator;
   unsigned int i;
*/
   EINA_SAFETY_ON_NULL_RETURN(pd);

   if (((x == pd->pan.x) && (y == pd->pan.y))) return;

   if (_horiz(pd->orient))
     {
        pd->pan.diff += x - pd->pan.x;
     }
   else
     {
        pd->pan.diff += y - pd->pan.y;
     }

   pd->pan.x = x;
   pd->pan.y = y;

   efl_canvas_group_change(obj);
/*
   if (abs(pd->pan.diff) > cw)
     {
        pd->pan.diff = 0;
        pd->need_reload = EINA_TRUE;
        _load_items(obj, pd, EINA_FALSE);
        pd->need_reload = EINA_FALSE;
        //efl_ui_gridview_view_update(pd->obj, EINA_FALSE);
        return;
     }


   EINA_ARRAY_ITER_NEXT(pd->items, i, litem, iterator)
      efl_gfx_position_set(litem->layout, (litem->x + 0 - pd->pan.x), (litem->y + 0 - pd->pan.y));8
*/
}

EOLIAN static void
_efl_ui_gridview_pan_elm_pan_pos_get(Eo *obj EINA_UNUSED,
                                     Efl_Ui_Gridview_Pan_Data *ppd,
                                     Evas_Coord *x,
                                     Evas_Coord *y)
{
   if (x) *x = ppd->wpd->pan.x;
   if (y) *y = ppd->wpd->pan.y;
}

EOLIAN static void
_efl_ui_gridview_pan_elm_pan_pos_max_get(Eo *obj EINA_UNUSED,
                                         Efl_Ui_Gridview_Pan_Data *ppd,
                                         Evas_Coord *x,
                                         Evas_Coord *y)
{
   Evas_Coord ow, oh;

   elm_interface_scrollable_content_viewport_geometry_get
      (ppd->wobj, NULL, NULL, &ow, &oh);
   ow = ppd->wpd->minw - ow;
   if (ow < 0) ow = 0;
   oh = ppd->wpd->minh - oh;
   if (oh < 0) oh = 0;

   if (x) *x = ow;
   if (y) *y = oh;
}

EOLIAN static void
_efl_ui_gridview_pan_elm_pan_pos_min_get(Eo *obj EINA_UNUSED,
                                         Efl_Ui_Gridview_Pan_Data * ppd EINA_UNUSED,
                                         Evas_Coord * x,
                                         Evas_Coord * y)
{
   if (x) *x = 0;
   if (y) *y = 0;
}

EOLIAN static void
_efl_ui_gridview_pan_elm_pan_content_size_get(Eo *obj EINA_UNUSED,
                                              Efl_Ui_Gridview_Pan_Data *ppd,
                                              Evas_Coord *w,
                                              Evas_Coord *h)
{
   Efl_Ui_Gridview_Data *pd = ppd->wpd;
   EINA_SAFETY_ON_NULL_RETURN(pd);

   if (w) *w = pd->minw;
   if (h) *h = pd->minh;
}

EOLIAN static void
_efl_ui_gridview_pan_efl_object_destructor(Eo *obj,
                                           Efl_Ui_Gridview_Pan_Data *ppd)
{
   efl_data_unref(ppd->wobj, ppd->wpd);
   efl_destructor(efl_super(obj, MY_PAN_CLASS));
}

EOLIAN static void
_efl_ui_gridview_pan_efl_gfx_size_set(Eo *obj,
                                      Efl_Ui_Gridview_Pan_Data *ppd EINA_UNUSED,
                                      Evas_Coord w,
                                      Evas_Coord h)
{
  if (_evas_object_intercept_call(obj, EVAS_OBJECT_INTERCEPT_CB_RESIZE, 0, w, h))
     return;

   efl_gfx_size_set(efl_super(obj, MY_PAN_CLASS), w, h);
}
EOLIAN static void
_efl_ui_gridview_pan_efl_gfx_position_set(Eo *obj,
                                          Efl_Ui_Gridview_Pan_Data *ppd EINA_UNUSED,
                                          Evas_Coord x,
                                          Evas_Coord y)
{
   if (_evas_object_intercept_call(obj, EVAS_OBJECT_INTERCEPT_CB_MOVE, 0, x, y))
     return;

   efl_gfx_position_set(efl_super(obj, MY_PAN_CLASS), x, y);
}

EOLIAN static void
_efl_ui_gridview_pan_efl_canvas_group_group_calculate(Eo *obj,
                                                      Efl_Ui_Gridview_Pan_Data *ppd)
{
   Efl_Ui_Gridview_Data *pd = ppd->wpd;
   Eina_Bool changed = EINA_FALSE;
   Eina_Bool skipped = EINA_FALSE;
   Evas_Coord ow, oh, cw;
   Efl_Ui_Gridview_Item *litem;
   Eina_Array_Iterator iterator;
   unsigned int i;

   efl_gfx_size_get(pd->obj, &ow, &oh);

   if (_horiz(pd->orient)) cw = ow / 4;
   else cw = oh / 4;

   if (abs(pd->pan.diff) > cw)
     {
        pd->pan.diff = 0;
        pd->need_reload = EINA_TRUE;
        skipped = EINA_TRUE;
     }

   SHINF("Group Need to Re-Load Items");
   if (pd->need_reload)
     {
        pd->on_load = _load_items(pd->obj, pd, pd->need_recalc);
        pd->need_reload = EINA_FALSE;
        pd->need_recalc = EINA_FALSE;
     }
   if (!pd->on_load && pd->need_update)
     {
        SHDBG("Group Need to be Update");
        changed = _view_update_internal(pd->obj);
     }

     if (skipped || (ow < 1 || oh < 1)) goto super;

     EINA_ARRAY_ITER_NEXT(pd->items, i, litem, iterator)
       efl_gfx_position_set(litem->layout, (litem->x + 0 - pd->pan.x), (litem->y + 0 - pd->pan.y));

super :
   efl_canvas_group_calculate(efl_super(obj, MY_PAN_CLASS));
}

#include "efl_ui_gridview_pan.eo.c"

static Eina_Bool
_efl_model_properties_has(Efl_Model *model,
                          Eina_Stringshare *propfind)
{
   const Eina_Array *properties;
   Eina_Array_Iterator iter_prop;
   Eina_Stringshare *property;
   Eina_Bool ret = EINA_FALSE;
   unsigned i = 0;

   EINA_SAFETY_ON_NULL_RETURN_VAL(model, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(propfind, EINA_FALSE);

   properties = efl_model_properties_get(model);

   EINA_ARRAY_ITER_NEXT(properties, i, property, iter_prop)
     {
        if (property == propfind)
          {
             ret = EINA_TRUE;
             break;
          }
     }

   return ret;
}

static void
_child_added_cb(void *data,
                const Efl_Event *event EINA_UNUSED)
{
   //Efl_Model_Children_Event* evt = event->info;
   Efl_Ui_Gridview *obj = data;
   EFL_UI_GRIDVIEW_DATA_GET_OR_RETURN(obj, pd);
   pd->item_count++;

   //FIXME: the new data is visible? is yes reload sliced children and test if have changes
   //_child_new(pd, evt->child);
   efl_canvas_group_change(pd->pan.obj);
}

static void
_child_removed_cb(void *data,
                  const Efl_Event *event)
{
   Efl_Model_Children_Event* evt = event->info;
   Efl_Ui_Gridview *obj = data;
   Efl_Ui_Gridview_Item *item;
   Eina_Array_Iterator iterator;
   unsigned int i;

   EFL_UI_GRIDVIEW_DATA_GET_OR_RETURN(obj, pd);
   pd->item_count--;

   EINA_ARRAY_ITER_NEXT(pd->items, i, item, iterator)
     {
        if (item->model == evt->child)
          {
             _child_remove(pd, item);
             //FIXME pd->items = eina_list_remove_list(pd->items, li);
             efl_canvas_group_change(pd->pan.obj);
             break;
          }
     }
}

static void
_on_item_focused(void *data,
                 const Efl_Event *event EINA_UNUSED)
{
   Efl_Ui_Gridview_Item *item = data;
   EFL_UI_GRIDVIEW_DATA_GET_OR_RETURN(item->obj, pd);

   pd->focused = item;

   if (!_elm_config->item_select_on_focus_disable && !item->selected)
     _efl_ui_gridview_item_select_set(item, EINA_TRUE);
}

static void
_on_item_unfocused(void *data,
                   const Efl_Event *event EINA_UNUSED)
{
   Efl_Ui_Gridview_Item *item = data;
   EFL_UI_GRIDVIEW_DATA_GET_OR_RETURN(item->obj, pd);

   pd->focused = NULL;
}

static void
_long_press_cb(void *data,
               const Efl_Event *event)
{
   Efl_Ui_Gridview_Item *item = data;

   item->long_timer = NULL;
   item->longpressed = EINA_TRUE;
   if (item->layout)
     efl_event_callback_legacy_call(item->layout, EFL_UI_EVENT_LONGPRESSED, item);

   efl_del(event->object);
}

static void
_on_item_mouse_down(void *data,
                    Evas *evas EINA_UNUSED,
                    Evas_Object *o EINA_UNUSED,
                    void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;
   Efl_Ui_Gridview_Item *item = data;

   EFL_UI_GRIDVIEW_DATA_GET_OR_RETURN(item->obj, pd);

   if (ev->button != 1) return;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) pd->on_hold = EINA_TRUE;
   else pd->on_hold = EINA_FALSE;
   if (pd->on_hold) return;

   item->down = EINA_TRUE;
   item->longpressed = EINA_FALSE;

   efl_del(item->long_timer);
   item->long_timer = efl_add(EFL_LOOP_TIMER_CLASS, efl_loop_main_get(EFL_LOOP_CLASS),
                              efl_event_callback_add(efl_added, EFL_LOOP_TIMER_EVENT_TICK,
                                                     _long_press_cb, item),
                              efl_loop_timer_interval_set(efl_added,
                                                          _elm_config->longpress_timeout));  
}

static void
_on_item_mouse_up(void *data,
                  Evas *evas EINA_UNUSED,
                  Evas_Object *o EINA_UNUSED,
                  void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;
   Efl_Ui_Gridview_Item *item = data;

   EFL_UI_GRIDVIEW_DATA_GET_OR_RETURN(item->obj, pd);

   if (ev->button != 1) return;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) pd->on_hold = EINA_TRUE;
   else pd->on_hold = EINA_FALSE;
   if (pd->on_hold || !item->down) return;

   item->down = EINA_FALSE;
   ELM_SAFE_FREE(item->long_timer, ecore_timer_del);

   if (item->longpressed)
     {
        item->longpressed = EINA_FALSE;
        return;
     }

   if (pd->select_mode != ELM_OBJECT_SELECT_MODE_ALWAYS && item->selected)
     return;

   _efl_ui_gridview_item_select_set(item, EINA_TRUE);
}

static void
_item_selected_then(void *data,
                    Efl_Event const *event)
{
   Efl_Ui_Gridview_Item *item = data;
   EINA_SAFETY_ON_NULL_RETURN(item);
   Eina_Stringshare *selected;
   const Eina_Value_Type *vtype;
   Eina_Value *value = (Eina_Value *)((Efl_Future_Event_Success*)event->info)->value;

   item->future = NULL;
   vtype = eina_value_type_get(value);

   if (vtype == EINA_VALUE_TYPE_STRING || vtype == EINA_VALUE_TYPE_STRINGSHARE)
     {
        eina_value_get(value, &selected);
        Eina_Bool s = (strcmp(selected, "selected") ? EINA_FALSE : EINA_TRUE);

        if (item->selected == s) return;
        item->selected = s;

        EFL_UI_GRIDVIEW_DATA_GET_OR_RETURN(item->obj, pd);
        if (item->selected)
          pd->selected = eina_list_append(pd->selected, item);
        else
          pd->selected = eina_list_remove(pd->selected, item);
     }
}

static void
_count_then(void *data,
            Efl_Event const *event)
{
   Efl_Ui_Gridview_Data *pd = data;
   EINA_SAFETY_ON_NULL_RETURN(pd);
   int *count = ((Efl_Future_Event_Success*)event->info)->value;

   pd->item_count = *count;
   //_load_items(pd->obj, pd, EINA_TRUE);
   pd->need_reload = EINA_TRUE;
   pd->need_recalc = EINA_TRUE;
   efl_canvas_group_change(pd->pan.obj);
}

static void
_count_error(void *data,
             Efl_Event const *event EINA_UNUSED)
{
   Efl_Ui_Gridview_Data *pd = data;
   EINA_SAFETY_ON_NULL_RETURN(pd);
}

static void
_item_property_then(void *data,
                    Efl_Event const *event)
{
   Efl_Ui_Gridview_Item *item = data;
   EINA_SAFETY_ON_NULL_RETURN(item);
   Eina_Value *value = (Eina_Value *)((Efl_Future_Event_Success*)event->info)->value;
   const Eina_Value_Type *vtype = eina_value_type_get(value);
   char *style = NULL;

   item->future = NULL;
   if (vtype == EINA_VALUE_TYPE_STRING || vtype == EINA_VALUE_TYPE_STRINGSHARE)
     eina_value_get(value, &style);

   elm_object_style_set(item->layout, style);
}

static void
_item_property_error(void *data,
                     Efl_Event const *event EINA_UNUSED)
{
   Efl_Ui_Gridview_Item *item = data;
   EINA_SAFETY_ON_NULL_RETURN(item);

   item->future = NULL;
}

static void
_efl_model_properties_changed_cb(void *data,
                                 const Efl_Event *event)
{
   Efl_Ui_Gridview_Item *item = data;
   EINA_SAFETY_ON_NULL_RETURN(item);
   Efl_Model_Property_Event *evt = event->info;
   Eina_Array_Iterator it;
   Eina_Stringshare *prop, *sprop;
   unsigned int i;

   if (!evt->changed_properties) return;

   sprop = eina_stringshare_add(SELECTED_PROP);
   EINA_ARRAY_ITER_NEXT(evt->changed_properties, i, prop, it)
     {
        if (prop == sprop)
          {
             item->future = efl_model_property_get(item->model, sprop);
             efl_future_then(item->future, &_item_selected_then, &_item_property_error, NULL, item);
          }
     }
}

static void
_child_remove(Efl_Ui_Gridview_Data *pd,
              Efl_Ui_Gridview_Item *item)
{
   EINA_SAFETY_ON_NULL_RETURN(item);
   EINA_SAFETY_ON_NULL_RETURN(item->model);

   _layout_unrealize(pd, item);

   elm_widget_sub_object_del(pd->obj, item->layout);
   free(item);
}

static void
_layout_realize(Efl_Ui_Gridview_Data *pd,
                Efl_Ui_Gridview_Item *item)
{
   Efl_Ui_Gridview_Item_Event evt;

   efl_ui_view_model_set(item->layout, item->model); //XXX: move to realize??

   evt.child = item->model;
   evt.layout = item->layout;
   evt.index = item->index;

   efl_event_callback_add(item->model, EFL_MODEL_EVENT_PROPERTIES_CHANGED, _efl_model_properties_changed_cb, item); //XXX move to elm_layout obj??
   efl_event_callback_call(item->obj, EFL_UI_GRIDVIEW_EVENT_ITEM_REALIZED, &evt);

   efl_gfx_visible_set(item->layout, EINA_TRUE);

   efl_event_callback_add(item->layout, ELM_WIDGET_EVENT_FOCUSED, _on_item_focused, item);
   efl_event_callback_add(item->layout, ELM_WIDGET_EVENT_UNFOCUSED, _on_item_unfocused, item);
   evas_object_event_callback_add(item->layout, EVAS_CALLBACK_MOUSE_DOWN, _on_item_mouse_down, item);
   evas_object_event_callback_add(item->layout, EVAS_CALLBACK_MOUSE_UP, _on_item_mouse_up, item);
   //efl_event_callback_add(item->layout, EFL_EVENT_POINTER_DOWN, _on_item_mouse_down, item);
   //efl_event_callback_add(item->layout, EFL_EVENT_POINTER_UP, _on_item_mouse_up, item);

   if (pd->select_mode != ELM_OBJECT_SELECT_MODE_NONE)
     efl_ui_model_connect(item->layout, "signal/elm,state,%v", "selected");

   _item_calc(pd, item);
}

static void
_layout_unrealize(Efl_Ui_Gridview_Data *pd,
                  Efl_Ui_Gridview_Item *item)
{
   Efl_Ui_Gridview_Item_Event evt;
   EINA_SAFETY_ON_NULL_RETURN(item);

   if (item->future)
     {
        efl_future_cancel(item->future);
        item->future = NULL;
     }

   if (pd->focused == item)
     pd->focused = NULL;

   if (item->selected)
     {
        item->selected = EINA_FALSE;
        pd->selected = eina_list_remove(pd->selected, item);
     }

   efl_event_callback_del(item->model, EFL_MODEL_EVENT_PROPERTIES_CHANGED, _efl_model_properties_changed_cb, item);
   efl_event_callback_del(item->layout, ELM_WIDGET_EVENT_FOCUSED, _on_item_focused, item);
   efl_event_callback_del(item->layout, ELM_WIDGET_EVENT_UNFOCUSED, _on_item_unfocused, item);
   evas_object_event_callback_del_full(item->layout, EVAS_CALLBACK_MOUSE_DOWN, _on_item_mouse_down, item);
   evas_object_event_callback_del_full(item->layout, EVAS_CALLBACK_MOUSE_UP, _on_item_mouse_up, item);
   //efl_event_callback_del(item->layout, EFL_EVENT_POINTER_DOWN, _on_item_mouse_down, item);
   //efl_event_callback_del(item->layout, EFL_EVENT_POINTER_UP, _on_item_mouse_up, item);
   efl_ui_model_connect(item->layout, "signal/elm,state,%v", NULL);

   evt.child = item->model;
   evt.layout = item->layout;
   evt.index = item->index;
   efl_event_callback_call(item->obj, EFL_UI_GRIDVIEW_EVENT_ITEM_UNREALIZED, &evt);
   efl_ui_view_model_set(item->layout, NULL);

   efl_unref(item->model);

   efl_gfx_visible_set(item->layout, EINA_FALSE);
}

Efl_Ui_Gridview_Item *
_child_new(Efl_Ui_Gridview_Data *pd,
           Efl_Model *model)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(pd, NULL);

   Efl_Ui_Gridview_Item *item = calloc(1, sizeof(Efl_Ui_Gridview_Item));

   item->obj = pd->obj;
   item->model = efl_ref(model);
   item->layout = efl_add(ELM_LAYOUT_CLASS, pd->obj);
   item->future = NULL;
   item->index = eina_array_count(pd->items) + pd->realized.start -1;

   elm_widget_sub_object_add(pd->obj, item->layout);
   efl_canvas_group_member_add(pd->pan.obj, item->layout);


//   FIXME: really need get it in model?
   Eina_Stringshare *style_prop = eina_stringshare_add("style");
   if (_efl_model_properties_has(item->model, style_prop))
     {
        item->future = efl_model_property_get(item->model, style_prop);
        efl_future_then(item->future, &_item_property_then, &_item_property_error, NULL, item);
     }
   eina_stringshare_del(style_prop);
//
   _layout_realize(pd, item);
   return item;
}

static void
_children_then(void *data,
               Efl_Event const *event)
{
   Efl_Ui_Gridview_Slice *sd = data;
   Efl_Ui_Gridview_Data *pd = sd->pd;
   Eina_Accessor *acc = (Eina_Accessor*)((Efl_Future_Event_Success*)event->info)->value;
   Eo *child_model;
   Efl_Ui_Gridview_Item *item;
   unsigned i, idx, count, diff;
   int w = 0, h = 0, line = 0, linemax = 0, linesum = 0; //cvw = 0, cvh = 0;
   Eina_Bool horz = _horiz(pd->orient);
   Eina_Array *array = pd->items;

   EINA_SAFETY_ON_NULL_RETURN(pd);

   pd->future = NULL;
   EINA_SAFETY_ON_NULL_RETURN(acc);
   ELM_WIDGET_DATA_GET_OR_RETURN(pd->obj, wd);

   count = eina_array_count(pd->items);
   efl_gfx_geometry_get(pd->obj, NULL, NULL, &w, &h);
   //elm_interface_scrollable_content_viewport_geometry_get(obj, NULL, NULL, &cvw, &cvh);

   SHDBG("children then [%p][%d] -- geo[%d,%d]", pd->items, count, w, h);

   if (sd->slicestart < pd->realized.start)
     {
        diff = pd->realized.start - sd->newstart;
        diff = diff > count ? count : diff;
        idx = count - diff;
        if (diff)
          {
             Efl_Ui_Gridview_Item **cacheit;
             size_t s = sizeof(Efl_Ui_Gridview_Item *);

             cacheit = calloc(diff, s);
             memcpy(cacheit, array->data+idx, diff * s);

             for (i = idx; i < count; ++i)
               {
                  item = eina_array_data_get(array, i);
                  _layout_unrealize(pd, item);
               }
             memmove(array->data+diff, array->data, idx * s);
             memcpy(array->data, cacheit, diff * s);
             free(cacheit);
          }
        idx = 0;
     }
   else
     {
        diff = sd->newstart - pd->realized.start;
        diff = diff > count ? count : diff;
        idx = count - diff;
        if (diff)
          {
             Efl_Ui_Gridview_Item **cacheit;
             size_t s = sizeof(Efl_Ui_Gridview_Item *);

             cacheit = calloc(diff, s);
             memcpy(cacheit, array->data, diff * s);

             for (i = 0; i < diff; ++i)
               {
                  item = eina_array_data_get(array, i);
                  _layout_unrealize(pd, item);
               }
             memmove(array->data, array->data+diff, idx * s);
             memcpy(array->data+idx, cacheit, diff * s);
             free(cacheit);
          }
     }

   pd->realized.start = sd->newstart;
   count = 1;

   EINA_ACCESSOR_FOREACH(acc, i, child_model)
     {
        if (idx < eina_array_count(array))
          {
             item = eina_array_data_get(array, idx);
             item->model = efl_ref(child_model);
             item->index = sd->newstart + idx - 1;

             if (horz)
               {
                  if (line + item->minh > h)
                    {
                       pd->realized.w -= linemax;
                       //SHINF("realw[%d] --- line[%d] linemax[%d], count[%d]", pd->realized.w, line, linemax, count);
                    }
               }
             else
               {
                  if (line + item->minw > w)
                    {
                       pd->realized.h -= linemax;
                       //SHINF("realh[%d] --- line[%d] linemax[%d], count[%d]", pd->realized.h, line, linemax, count);
                    }
               }

             SHINF("IDX[%d]-REAL[%d]:: Realized [%d][%d]", idx, item->index, pd->realized.w, pd->realized.h);

             _layout_realize(pd, item);
          }
        else
          {
             item = _child_new(pd, child_model);
             eina_array_push(pd->items, item);
              SHINF("IDX[%d]-item new [%d, %d]", idx, pd->realized.w, pd->realized.h);

          }

        if (horz)
          {
               {
                  linesum += item->minh;
                  if (line + item->minh > h)
                    {
                       pd->realized.w += linemax;
                       line = item->minh;
                       linemax = item->minw;
                       count++;
                       SHINF("realw[%d] --- line[%d] linemax[%d], count[%d]", pd->realized.w, line, linemax, count);
                    }
                  else
                    {
                       line += item->minh;
                       linemax = MAX(linemax, item->minw);
                    }
               }
          }
        else
          {
                  linesum += item->minw;
                  if (line + item->minw > w)
                    {
                       pd->realized.h += linemax;
                       line = item->minw;
                       linemax = item->minh;
                       count++;
                       SHINF("realh[%d] --- line[%d] linemax[%d], count[%d]", pd->realized.h, line, linemax, count);
                    }
                  else
                    {
                       line += item->minw;
                       linemax = MAX(linemax, item->minh);
                    }
          }
        ++idx;
     }

   pd->avgsum = horz ? pd->realized.w : pd->realized.h;
   pd->line_count = MAX(pd->line_count, count);

   count = eina_array_count(pd->items);

   if (pd->line_count && count)
     {
        int avgline = linesum / count;
        int avgit = pd->avgsum / pd->line_count;
        pd->avgitw = horz ? avgit : avgline;
        pd->avgith = horz ? avgline : avgit;
     }

   free(sd);

   pd->on_load = EINA_FALSE;
   //pd->view_update = EINA_TRUE;
   efl_ui_gridview_view_update(pd->obj, EINA_FALSE);
   efl_canvas_group_change(pd->pan.obj);
}

static void
_efl_ui_gridview_children_free(Eo *obj EINA_UNUSED,
                               Efl_Ui_Gridview_Data *pd)
{
   Eina_Array_Iterator iterator;
   Efl_Ui_Gridview_Item *item;
   unsigned int i;

   EINA_SAFETY_ON_NULL_RETURN(pd);

   if(!pd->items) return;

   EINA_ARRAY_ITER_NEXT(pd->items, i, item, iterator)
     {
        _layout_unrealize(pd, item);
        free(item);
        item = NULL;
     }

   eina_array_clean(pd->items);
}

static void
_children_error(void * data EINA_UNUSED, Efl_Event const* event EINA_UNUSED)
{
   Efl_Ui_Gridview_Slice *sd = data;
   Efl_Ui_Gridview_Data *pd = sd->pd;
   pd->future = NULL;
   free(data);
}

static void
_show_region_hook(void *data EINA_UNUSED,
                  Evas_Object *obj)
{
   Evas_Coord x, y, w, h;

   elm_widget_show_region_get(obj, &x, &y, &w, &h);
   elm_interface_scrollable_content_region_set(obj, x, y, w, h);
   elm_interface_scrollable_content_region_show(obj, x, y, w, h);
}

EOLIAN static void
_efl_ui_gridview_select_mode_set(Eo *obj EINA_UNUSED,
                                 Efl_Ui_Gridview_Data *pd,
                                 Elm_Object_Select_Mode mode)
{
   Eina_Array_Iterator iterator;
   Efl_Ui_Gridview_Item *item;
   unsigned int i;

   if (pd->select_mode == mode)
     return;

   if (pd->select_mode == ELM_OBJECT_SELECT_MODE_NONE)
     {
        EINA_ARRAY_ITER_NEXT(pd->items, i, item, iterator)
          {
             if (item->selected)
               elm_layout_signal_emit(item->layout, "elm,state,selected", "elm");

             efl_ui_model_connect(item->layout, "signal/elm,state,%v", "selected");
          }
     }
   else if (mode == ELM_OBJECT_SELECT_MODE_NONE)
     {
        EINA_ARRAY_ITER_NEXT(pd->items, i, item, iterator)
          {
             if (item->selected)
               elm_layout_signal_emit(item->layout, "elm,state,unselected", "elm");

             efl_ui_model_connect(item->layout, "signal/elm,state,%v", NULL);
          }
     }

   pd->select_mode = mode;
}

EOLIAN static Elm_Object_Select_Mode
_efl_ui_gridview_select_mode_get(Eo *obj EINA_UNUSED,
                                 Efl_Ui_Gridview_Data *pd)
{
   return pd->select_mode;
}

EOLIAN static void
_efl_ui_gridview_default_style_set(Eo *obj EINA_UNUSED,
                                   Efl_Ui_Gridview_Data *pd,
                                   Eina_Stringshare *style)
{
   eina_stringshare_replace(&pd->style, style);
}

EOLIAN static Eina_Stringshare *
_efl_ui_gridview_default_style_get(Eo *obj EINA_UNUSED,
                                   Efl_Ui_Gridview_Data *pd)
{
   return pd->style;
}

//FIXME update layout
EOLIAN static void
_efl_ui_gridview_homogeneous_set(Eo *obj EINA_UNUSED,
                                 Efl_Ui_Gridview_Data *pd,
                                 Eina_Bool homogeneous)
{
   pd->homogeneous = homogeneous;
}

EOLIAN static Eina_Bool
_efl_ui_gridview_homogeneous_get(Eo *obj EINA_UNUSED,
                                 Efl_Ui_Gridview_Data *pd)
{
   return pd->homogeneous;
}

EOLIAN static Elm_Theme_Apply
_efl_ui_gridview_elm_widget_theme_apply(Eo *obj,
                                        Efl_Ui_Gridview_Data *pd EINA_UNUSED)
{
   return elm_obj_widget_theme_apply(efl_super(obj, MY_CLASS));
}

EOLIAN static void
_efl_ui_gridview_efl_canvas_group_group_calculate(Eo *obj,
                                                  Efl_Ui_Gridview_Data *pd)
{
   if (pd->need_recalc) return;

   //efl_ui_gridview_view_update(obj, EINA_FALSE);
   efl_canvas_group_calculate(efl_super(obj, MY_CLASS));
}

EOLIAN static void
_efl_ui_gridview_efl_gfx_position_set(Eo *obj,
                                      Efl_Ui_Gridview_Data *pd,
                                      Evas_Coord x,
                                      Evas_Coord y)
{
   if (_evas_object_intercept_call(obj, EVAS_OBJECT_INTERCEPT_CB_MOVE, 0, x, y))
     return;

   efl_gfx_position_set(efl_super(obj, MY_CLASS), x, y);

   efl_gfx_position_set(pd->hit_rect, x, y);
   efl_gfx_position_set(pd->pan.obj, x - pd->pan.x, y - pd->pan.y);
   efl_ui_gridview_view_update(obj, EINA_FALSE);
}

EOLIAN static void
_efl_ui_gridview_efl_gfx_size_set(Eo *obj,
                                  Efl_Ui_Gridview_Data *pd,
                                  Evas_Coord w,
                                  Evas_Coord h)
{
   Evas_Coord oldw, oldh;
   Eina_Bool load = EINA_FALSE;
   if (_evas_object_intercept_call(obj, EVAS_OBJECT_INTERCEPT_CB_RESIZE, 0, w, h))
     return;

   efl_gfx_size_get(obj, &oldw, &oldh);
   efl_gfx_size_set(efl_super(obj, MY_CLASS), w, h);
   efl_gfx_size_set(pd->hit_rect, w, h);


   if (_horiz(pd->orient))
     {
        if (w != oldw) load = EINA_TRUE;
     }
   else
     {
        if (h != oldh) load = EINA_TRUE;
     }

   SHCRI("GFX SIZE SET [%d, %d]", w, h);

   //if (load && _load_items(obj, pd, EINA_TRUE))
   //  return;
   if (load)
     {
        pd->need_reload = EINA_TRUE;
        pd->need_recalc = EINA_TRUE;
     }
   efl_ui_gridview_view_update(obj, EINA_FALSE);

   efl_canvas_group_change(pd->pan.obj);
}

EOLIAN static void
_efl_ui_gridview_efl_canvas_group_group_member_add(Eo *obj,
                                                   Efl_Ui_Gridview_Data *pd,
                                                   Evas_Object *member)
{
   efl_canvas_group_member_add(efl_super(obj, MY_CLASS), member);

   if (pd->hit_rect)
     efl_gfx_stack_raise(pd->hit_rect);
}

EOLIAN static void
_efl_ui_gridview_elm_layout_sizing_eval(Eo *obj,
                                        Efl_Ui_Gridview_Data *pd EINA_UNUSED)
{
   Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;
   Evas_Coord vmw = 0, vmh = 0;

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   efl_gfx_size_hint_combined_min_get(obj, &minw, &minh);
   efl_gfx_size_hint_max_get(obj, &maxw, &maxh);
   edje_object_size_min_calc(wd->resize_obj, &vmw, &vmh);

   minw = vmw;
   minh = vmh;

   if ((maxw > 0) && (minw > maxw))
     minw = maxw;
   if ((maxh > 0) && (minh > maxh))
     minh = maxh;

   efl_gfx_size_hint_min_set(obj, minw, minh);
   efl_gfx_size_hint_max_set(obj, maxw, maxh);
}

EOLIAN static void
_efl_ui_gridview_efl_canvas_group_group_add(Eo *obj,
                                            Efl_Ui_Gridview_Data *pd EINA_UNUSED)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);
   Efl_Ui_Gridview_Pan_Data *pan_data;
   Evas_Coord minw, minh;

   //DEBUG CODE!!!!
   START = ecore_time_get();

   efl_canvas_group_add(efl_super(obj, MY_CLASS));
   elm_widget_sub_object_parent_add(obj);

   /* common scroller hit rectangle setup */
   pd->hit_rect = efl_add(EFL_CANVAS_RECTANGLE_CLASS, efl_provider_find(obj, EVAS_CANVAS_CLASS),
                          efl_key_data_set(efl_added, "_elm_leaveme", obj),
                          efl_canvas_group_member_add(obj, efl_added),
                          elm_widget_sub_object_add(obj, efl_added),
                          efl_canvas_object_repeat_events_set(efl_added, EINA_TRUE),
                          efl_gfx_color_set(efl_added, 0, 0, 0, 0),
                          efl_gfx_visible_set(efl_added, EINA_TRUE));

   elm_widget_on_show_region_hook_set(obj, _show_region_hook, NULL);

   if (!elm_layout_theme_set(obj, "gridview", "base", elm_widget_style_get(obj)))
     CRI("Failed to set layout!");

   pd->mode = ELM_LIST_COMPRESS;
   pd->avgitw = AVERAGE_SIZE_INIT;
   pd->avgith = AVERAGE_SIZE_INIT;

   pd->pan.obj = efl_add(MY_PAN_CLASS, efl_provider_find(obj, EVAS_CANVAS_CLASS));
   pan_data = efl_data_scope_get(pd->pan.obj, MY_PAN_CLASS);
   efl_data_ref(obj, MY_CLASS);
   pan_data->wobj = obj;
   pan_data->wpd = pd;
   pd->pan.x = 0;
   pd->pan.y = 0;
   efl_gfx_visible_set(pd->pan.obj, EINA_TRUE);

   elm_interface_scrollable_objects_set(obj, wd->resize_obj, pd->hit_rect);
   elm_interface_scrollable_extern_pan_set(obj, pd->pan.obj);
   elm_interface_scrollable_mirrored_set(obj, efl_ui_mirrored_get(obj));
   elm_interface_scrollable_bounce_allow_set(obj, EINA_FALSE,
                                             _elm_config->thumbscroll_bounce_enable);

   elm_interface_atspi_accessible_type_set(obj, ELM_ATSPI_TYPE_DISABLED);

   edje_object_size_min_calc(wd->resize_obj, &minw, &minh);
   efl_gfx_size_hint_min_set(obj, minw, minh);
   elm_layout_sizing_eval(obj);
}

EOLIAN static void
_efl_ui_gridview_efl_canvas_group_group_del(Eo *obj,
                                            Efl_Ui_Gridview_Data *pd)
{
   _efl_ui_gridview_children_free(obj, pd);

   eina_array_free(pd->items);

   ELM_SAFE_FREE(pd->pan.obj, efl_del);
   efl_canvas_group_del(efl_super(obj, MY_CLASS));
}

EOLIAN static Eo *
_efl_ui_gridview_efl_object_constructor(Eo *obj,
                                        Efl_Ui_Gridview_Data *pd)
{
   {
      Efl_Ui_Focus_Manager *manager;

      manager = efl_add(EFL_UI_FOCUS_MANAGER_CLASS, NULL,
                        efl_ui_focus_manager_root_set(efl_added, obj)
      );

      efl_composite_attach(obj, manager);
      _efl_ui_focus_manager_redirect_events_add(manager, obj);
   }

   obj = efl_constructor(efl_super(obj, MY_CLASS));
   pd->obj = obj;
   efl_canvas_object_type_set(obj, MY_CLASS_NAME);
   evas_object_smart_callbacks_descriptions_set(obj, _smart_callbacks);
   elm_interface_atspi_accessible_role_set(obj, ELM_ATSPI_ROLE_LIST);

   pd->style = eina_stringshare_add(elm_widget_style_get(obj));

   pd->orient = EFL_ORIENT_DOWN;
   pd->align.h = 0;
   pd->align.v = 0;

   return obj;
}

EOLIAN static void
_efl_ui_gridview_efl_object_destructor(Eo *obj,
                                       Efl_Ui_Gridview_Data *pd)
{
   efl_unref(pd->model);
   eina_stringshare_del(pd->style);
   efl_destructor(efl_super(obj, MY_CLASS));
}

EOLIAN static void
_efl_ui_gridview_efl_ui_view_model_set(Eo *obj,
                                       Efl_Ui_Gridview_Data *pd,
                                       Efl_Model *model)
{
   if (pd->model == model)
     return;

   if (pd->future) efl_future_cancel(pd->future);

   if (pd->model)
     {
        efl_event_callback_del(pd->model, EFL_MODEL_EVENT_CHILD_ADDED, _child_added_cb, obj);
        efl_event_callback_del(pd->model, EFL_MODEL_EVENT_CHILD_REMOVED, _child_removed_cb, obj);
        efl_unref(pd->model);
        pd->model = NULL;
     }

   _efl_ui_gridview_children_free(obj, pd);

   if (model)
     {
        pd->model = model;
        efl_ref(pd->model);
        efl_event_callback_add(pd->model, EFL_MODEL_EVENT_CHILD_ADDED, _child_added_cb, obj);
        efl_event_callback_add(pd->model, EFL_MODEL_EVENT_CHILD_REMOVED, _child_removed_cb, obj);
        efl_future_then(efl_model_children_count_get(pd->model), &_count_then, &_count_error, NULL, pd);
     }

   efl_ui_gridview_view_update(obj, EINA_TRUE);
}

EOLIAN static Efl_Model *
_efl_ui_gridview_efl_ui_view_model_get(Eo *obj EINA_UNUSED,
                                       Efl_Ui_Gridview_Data *pd)
{
   return pd->model;
}

EOLIAN const Elm_Atspi_Action *
_efl_ui_gridview_elm_interface_atspi_widget_action_elm_actions_get(Eo *obj EINA_UNUSED,
                                                                   Efl_Ui_Gridview_Data *pd EINA_UNUSED)
{
   static Elm_Atspi_Action atspi_actions[] = {
          { "move,prior", "move", "prior", _key_action_move},
          { "move,next", "move", "next", _key_action_move},
          { "move,up", "move", "up", _key_action_move},
          { "move,up,multi", "move", "up_multi", _key_action_move},
          { "move,down", "move", "down", _key_action_move},
          { "move,down,multi", "move", "down_multi", _key_action_move},
          { "move,first", "move", "first", _key_action_move},
          { "move,last", "move", "last", _key_action_move},
          { "select", "select", NULL, _key_action_select},
          { "select,multi", "select", "multi", _key_action_select},
          { "escape", "escape", NULL, _key_action_escape},
          { NULL, NULL, NULL, NULL }
   };
   return &atspi_actions[0];
}

EOLIAN Eina_List *
_efl_ui_gridview_elm_interface_atspi_accessible_children_get(Eo *obj,
                                                             Efl_Ui_Gridview_Data *pd)
{
   Efl_Ui_Gridview_Item *litem;
   Eina_Array_Iterator iterator;
   unsigned int i;
   Eina_List *ret = NULL, *ret2 = NULL;

   EINA_ARRAY_ITER_NEXT(pd->items, i, litem, iterator)
      ret = eina_list_append(ret, litem->layout);

   ret2 = elm_interface_atspi_accessible_children_get(efl_super(obj, MY_CLASS));

   return eina_list_merge(ret, ret2);
}

EOLIAN int
_efl_ui_gridview_elm_interface_atspi_selection_selected_children_count_get(Eo *obj EINA_UNUSED,
                                                                           Efl_Ui_Gridview_Data *pd)
{
   return eina_list_count(pd->selected);
}

EOLIAN Eo *
_efl_ui_gridview_elm_interface_atspi_selection_selected_child_get(Eo *obj EINA_UNUSED,
                                                                  Efl_Ui_Gridview_Data *pd,
                                                                  int child_idx)
{
   Efl_Ui_Gridview_Item *item = eina_list_nth(pd->selected, child_idx);
   if (!item)
     return NULL;

   return item->layout;
}

EOLIAN Eina_Bool
_efl_ui_gridview_elm_interface_atspi_selection_child_select(Eo *obj EINA_UNUSED,
                                                            Efl_Ui_Gridview_Data *pd,
                                                            int child_index)
{
   if (pd->select_mode != ELM_OBJECT_SELECT_MODE_NONE)
     {
        Efl_Ui_Gridview_Item *item = eina_array_data_get(pd->items, child_index);
        if (item)
          _efl_ui_gridview_item_select_set(item, EINA_TRUE);
        return EINA_TRUE;
     }
   return EINA_FALSE;
}

EOLIAN Eina_Bool
_efl_ui_gridview_elm_interface_atspi_selection_selected_child_deselect(Eo *obj EINA_UNUSED,
                                                                       Efl_Ui_Gridview_Data *pd,
                                                                       int child_index)
{
   Efl_Ui_Gridview_Item *item = eina_list_nth(pd->selected, child_index);
   if (item)
     {
        _efl_ui_gridview_item_select_set(item, EINA_FALSE);
        return EINA_TRUE;
     }
   return EINA_FALSE;
}

EOLIAN Eina_Bool
_efl_ui_gridview_elm_interface_atspi_selection_is_child_selected(Eo *obj EINA_UNUSED,
                                                                 Efl_Ui_Gridview_Data *pd,
                                                                 int child_index)
{
   Efl_Ui_Gridview_Item *item = eina_array_data_get(pd->items, child_index);
   EINA_SAFETY_ON_NULL_RETURN_VAL(item, EINA_FALSE);
   return item->selected;
}

EOLIAN Eina_Bool
_efl_ui_gridview_elm_interface_atspi_selection_all_children_select(Eo *obj EINA_UNUSED,
                                                                   Efl_Ui_Gridview_Data *pd)
{
   Efl_Ui_Gridview_Item *item;
   Eina_Array_Iterator iterator;
   unsigned int i;

   if (pd->select_mode == ELM_OBJECT_SELECT_MODE_NONE)
     return EINA_FALSE;

   EINA_ARRAY_ITER_NEXT(pd->items, i, item, iterator)
      _efl_ui_gridview_item_select_set(item, EINA_TRUE);

   return EINA_TRUE;
}

EOLIAN Eina_Bool
_efl_ui_gridview_elm_interface_atspi_selection_clear(Eo *obj EINA_UNUSED,
                                                     Efl_Ui_Gridview_Data *pd)
{
   Efl_Ui_Gridview_Item *item;
   Eina_List *l;

   if (pd->select_mode == ELM_OBJECT_SELECT_MODE_NONE)
     return EINA_FALSE;

   EINA_LIST_FOREACH(pd->selected, l, item)
      _efl_ui_gridview_item_select_set(item, EINA_FALSE);

   return EINA_TRUE;
}

EOLIAN Eina_Bool
_efl_ui_gridview_elm_interface_atspi_selection_child_deselect(Eo *obj EINA_UNUSED,
                                                              Efl_Ui_Gridview_Data *pd,
                                                              int child_index)
{
   Efl_Ui_Gridview_Item *item = eina_array_data_get(pd->items, child_index);
   if (item)
     {
        _efl_ui_gridview_item_select_set(item, EINA_FALSE);
        return EINA_TRUE;
     }
   return EINA_FALSE;
}

static Eina_Bool
_key_action_move(Evas_Object *obj,
                 const char *params)
{
   EFL_UI_GRIDVIEW_DATA_GET_OR_RETURN_VAL(obj, pd, EINA_FALSE);
   const char *dir = params;

   Evas_Coord page_x, page_y;
   Evas_Coord v_w, v_h;
   Evas_Coord x, y;

   elm_interface_scrollable_content_pos_get(obj, &x, &y);
   elm_interface_scrollable_page_size_get(obj, &page_x, &page_y);
   elm_interface_scrollable_content_viewport_geometry_get(obj, NULL, NULL, &v_w, &v_h);

/*
   Efl_Ui_Gridview_Item *item = NULL;
   Elm_Object_Item *oitem = NULL;
   Elm_Layout *eoit = NULL;
   if (!strcmp(dir, "up") || !strcmp(dir, "up_multi"))
     {
        if (!elm_widget_focus_next_get(obj, ELM_FOCUS_UP, &eoit, &oitem))
          return EINA_FALSE;
     }
   else if (!strcmp(dir, "down") || !strcmp(dir, "down_multi"))
     {
        if (!elm_widget_focus_next_get(obj, ELM_FOCUS_DOWN, &eoit, &oitem))
          return EINA_FALSE;
        printf(">> %d\n", __LINE__);
     }
   else if (!strcmp(dir, "first"))
     {
        item = eina_list_data_get(pd->items);
        x = 0;
        y = 0;
        elm_widget_focus_next_object_set(obj, item->layout, ELM_FOCUS_UP);
     }
   else if (!strcmp(dir, "last"))
     {
        item = eina_list_data_get(eina_list_last(pd->items));
        elm_obj_pan_pos_max_get(pd->pan.obj, &x, &y);
     }
   else */
   if (!strcmp(dir, "prior"))
     {
        if (_horiz(pd->orient))
          {
             if (page_x < 0)
               x -= -(page_x * v_w) / 100;
             else
               x -= page_x;
          }
        else
          {
             if (page_y < 0)
               y -= -(page_y * v_h) / 100;
             else
               y -= page_y;
          }
     }
   else if (!strcmp(dir, "next"))
     {
        if (_horiz(pd->orient))
          {
             if (page_x < 0)
               x += -(page_x * v_w) / 100;
             else
               x += page_x;
          }
        else
          {
             if (page_y < 0)
               y += -(page_y * v_h) / 100;
             else
               y += page_y;
          }
     }
   else return EINA_FALSE;

   elm_interface_scrollable_content_pos_set(obj, x, y, EINA_TRUE);
/*
   if (item)
     eoit = item->layout;

   if (!eoit) return EINA_FALSE;

   elm_object_focus_set(eoit, EINA_TRUE);
*/
   return EINA_TRUE;
}

static Eina_Bool
_key_action_select(Evas_Object *obj,
                   const char *params EINA_UNUSED)
{
   EFL_UI_GRIDVIEW_DATA_GET_OR_RETURN_VAL(obj, pd, EINA_FALSE);

   _efl_ui_gridview_item_select_set(pd->focused, EINA_TRUE);

   return EINA_TRUE;
}

static Eina_Bool
_key_action_escape(Evas_Object *obj,
                   const char *params EINA_UNUSED)
{
   return _efl_ui_gridview_item_select_clear(obj);
}

EOLIAN static Eina_Bool
_efl_ui_gridview_elm_widget_widget_event(Eo *obj, Efl_Ui_Gridview_Data *pd,
                                         Evas_Object *src,
                                         Evas_Callback_Type type,
                                         void *event_info)
{
   (void) src;
   Evas_Event_Key_Down *ev = event_info;

   if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return EINA_FALSE;
   if (!pd->items) return EINA_FALSE;

   if (!_elm_config_key_binding_call(obj, MY_CLASS_NAME, ev, key_actions))
     return EINA_FALSE;

   ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
   return EINA_TRUE;
}

static Eina_Bool
_efl_ui_gridview_item_select_clear(Eo *obj)
{
   Eina_List *li;
   Efl_Ui_Gridview_Item *item;
   EFL_UI_GRIDVIEW_DATA_GET_OR_RETURN_VAL(obj, pd, EINA_FALSE);

   EINA_LIST_FOREACH(pd->selected, li, item)
      _efl_ui_gridview_item_select_set(item, EINA_FALSE);

   return EINA_TRUE;
}

static void
_efl_ui_gridview_item_select_set(Efl_Ui_Gridview_Item *item,
                                 Eina_Bool selected)
{
   Eina_Stringshare *sprop, *svalue;

   if (!item) return;

   EFL_UI_GRIDVIEW_DATA_GET_OR_RETURN(item->obj, pd);

   if ((pd->select_mode == ELM_OBJECT_SELECT_MODE_NONE) ||
       (pd->select_mode == ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY))
     return;

   selected = !!selected;
   if (!item->model || item->selected == selected) return;

   sprop = eina_stringshare_add(SELECTED_PROP);
   svalue = (selected ? eina_stringshare_add("selected") : eina_stringshare_add("unselected"));

   if (_efl_model_properties_has(item->model, sprop))
     {
        Eina_Value v;
        eina_value_setup(&v, EINA_VALUE_TYPE_STRINGSHARE);
        eina_value_set(&v, svalue);
        efl_model_property_set(item->model, sprop, &v);
        eina_value_flush(&v);
     }
   eina_stringshare_del(sprop);
   eina_stringshare_del(svalue);

   //TODO I need call this event or catch only by model connect event?
   if (selected)
     efl_event_callback_legacy_call(item->layout, EFL_UI_EVENT_SELECTED, item);
   else
     efl_event_callback_legacy_call(item->layout, EFL_UI_EVENT_UNSELECTED, item);
}

static void
_item_calc(Efl_Ui_Gridview_Data *pd,
           Efl_Ui_Gridview_Item *item)
{
   Evas_Coord old_w, old_h;
   int pad[4];

   efl_gfx_size_hint_combined_min_get(item->layout, &old_w, &old_h);
   edje_object_size_min_calc(elm_layout_edje_get(item->layout), &item->minw, &item->minh);
   efl_gfx_size_hint_margin_get(item->layout, &pad[0], &pad[1], &pad[2], &pad[3]);
   efl_gfx_size_hint_weight_get(item->layout, &item->wx, &item->wy);

   if (old_w > item->minw) item->minw = old_w;
   if (old_h > item->minh) item->minh = old_h;

   if (item->wx < 0) item->wx = 0;
   if (item->wy < 0) item->wy = 0;
   efl_gfx_size_hint_min_set(item->layout, item->minw, item->minh);

   item->minw += pad[0] + pad[1];
   item->minh += pad[2] + pad[3];

   pd->weight.x += item->wx;
   pd->weight.y += item->wy;
}

static Eina_Bool
_load_items(Eo *obj,
            Efl_Ui_Gridview_Data *pd,
            Eina_Bool recalc)
{
   Efl_Ui_Gridview_Slice *sd;
   int slice, slicestart, newstart, count = 0, w = 0, h = 0, cvw = 0, cvh = 0;
   Efl_Ui_Gridview_Item *item;
   Eina_Bool horz = _horiz(pd->orient);
   if (pd->future)
     efl_future_cancel(pd->future);

   if (pd->items)
     count = eina_array_count(pd->items);

   SHINF("#1 -- CNT[%d] CALC[%d]", count, recalc);

   if ((!recalc && count == pd->item_count) || pd->avgitw < 1 || pd->avgith < 1)
     return EINA_FALSE;

   efl_gfx_geometry_get(obj, NULL, NULL, &w, &h);
   elm_interface_scrollable_content_viewport_geometry_get(obj, NULL, NULL, &cvw, &cvh);

   slice  = 2 * (w / pd->avgitw) * (h / pd->avgith);
   if (horz)
     {
        newstart = (pd->pan.x / pd->avgitw) - (slice / 4);
     }
   else
     {
        newstart = (pd->pan.y / pd->avgith) - (slice / 4);
     }

   SHINF("[%s] --- geo[%d,%d] cvgeo[%d,%d] count [%d], itcount[%d] avgw[%d] avgh[%d] slice[%d] newstart[%d]", SHNAME(obj), w, h, cvw, cvh, count, pd->item_count, pd->avgitw, pd->avgith, slice, newstart);
   slice = slice > 20 ? slice : 20;
   slicestart = newstart = newstart > 1 ? newstart : 1;
   SHINF(" --- rearrage slice[%d] slicestart[%d] newstart[%d]", slice, slicestart, newstart);

   if (!recalc && newstart == pd->realized.start && slice == pd->realized.slice)
     return EINA_FALSE;

   pd->realized.slice = slice;
   if (!pd->items)
     {
        pd->items = eina_array_new(20);
        SHINF("array items are Generated [%p][%d]", pd->items, eina_array_count(pd->items));
     }

   if (count)
     {
        if (newstart < pd->realized.start)
          {
             if(pd->realized.start - newstart < slice)
               slice = pd->realized.start - newstart;
          }
        else if (newstart < pd->realized.start + count)
          {
             slicestart = pd->realized.start + count;
             slice -= slicestart - newstart;
          }
     }

   if (slicestart + slice > pd->item_count)
     {
        int aux = (slicestart + slice - 1) - pd->item_count;
        slice -= aux;
        newstart = newstart - aux > 1 ? newstart - aux : 1;
     }

   if (slice > 0)
     {
       SHINF("Slice call chidren then slice[%d]sstart[%d]newstart[%d]", slice, slicestart, newstart);
        sd = malloc(sizeof(Efl_Ui_Gridview_Slice));
        sd->pd = pd;
        sd->slicestart = slicestart;
        sd->newstart = newstart;
        sd->newslice = slice;

        pd->future = efl_model_children_slice_get(pd->model, slicestart, slice);
        efl_future_then(pd->future, &_children_then, &_children_error, NULL, sd);
     }
   else
     {
        int line = 0, linemax = 0, linesum = 0, count = 1;
        while (pd->realized.slice < (int)eina_array_count(pd->items))
          {
             item = eina_array_pop(pd->items);
             if (!item) break;
             if (horz)
               {
                  linesum += item->minh;
                  if (line + item->minh > cvh)
                    {
                       pd->realized.w -= linemax;
                       line = item->minh;
                       linemax = item->minw;
                       count++;
                       SHINF("realw[%d] --- line[%d] linemax[%d], count[%d] pos[%d,%d]", pd->realized.w, line, linemax, count, item->pos[0], item->pos[1]);
                    }
                  else
                    {
                       line += item->minh;
                       linemax = MAX(linemax, item->minw);
                    }
               }
             else
               {
                  linesum += item->minw;
                  if (line + item->minw > cvw)
                    {
                       pd->realized.h -= linemax;
                       line = item->minw;
                       linemax = item->minh;
                       count++;
                       SHINF("realh[%d] --- line[%d] linemax[%d], count[%d] pos[%d,%d]", pd->realized.h, line, linemax, count, item->pos[0], item->pos[1]);
                    }
                  else
                    {
                       line += item->minw;
                       linemax = MAX(linemax, item->minh);
                    }
               }
             _child_remove(pd, item);
          }
        pd->avgsum = horz ? pd->realized.w : pd->realized.h;
        pd->line_count = MAX(pd->line_count, count);

        count = eina_array_count(pd->items);

        if (pd->line_count && count)
          {
             int avgline = linesum / count;
             int avgit = pd->avgsum / pd->line_count;
             pd->avgitw = horz ? avgit : avgline;
             pd->avgith = horz ? avgline : avgit;
          }
       SHINF("[%s] --- avgw[%d] avgh[%d] realw[%d] realh[%d] count[%d] itcount[%d]", SHNAME(obj), pd->avgitw, pd->avgith, pd->realized.w, pd->realized.h, pd->line_count, eina_array_count(pd->items));

        return EINA_FALSE;
     }

   return EINA_TRUE;
}

static Eina_Bool
_view_update_internal(Eo *ui_grid)
{
   EFL_UI_GRIDVIEW_DATA_GET_OR_RETURN_VAL(ui_grid, pd, EINA_FALSE);
   Efl_Ui_Gridview_Item *litem;
   Evas_Object *o;
   Eina_Bool horiz = _horiz(pd->orient), zeroweight = EINA_FALSE;
   Evas_Coord ow, oh, want;
   int boxx, boxy, boxw, boxh, length, pad, extra = 0, rounding = 0;
   int boxl = 0, boxr = 0, boxt = 0, boxb = 0;
   double cur_pos = 0, scale, box_align[2],  weight[2] = { 0, 0 }, linesum = 0;
   Eina_Bool box_fill[2] = { EINA_FALSE, EINA_FALSE };
   Eina_Array_Iterator iterator;
   unsigned int i;
   int id, count = 0, avgit, row = 0, col = 0;
   int oldminw, oldminh;

   ELM_WIDGET_DATA_GET_OR_RETURN(ui_grid, wd, EINA_FALSE);

   efl_gfx_geometry_get(ui_grid, &boxx, &boxy, &boxw, &boxh);
   efl_gfx_size_hint_margin_get(ui_grid, &boxl, &boxr, &boxt, &boxb);

   oldminw = pd->minw;
   oldminh = pd->minh;

   scale = efl_ui_scale_get(ui_grid);
   // Box align: used if "item has max size and fill" or "no item has a weight"
   // Note: cells always expand on the orthogonal direction
   SHINF("[%s] --- geo[%d, %d, %d, %d] margin[%d, %d, %d, %d] scale[%.2f]", SHNAME(ui_grid), boxx, boxy, boxw, boxh, boxl, boxr, boxt, boxb, scale);
   box_align[0] = pd->align.h;
   box_align[1] = pd->align.v;
   if (box_align[0] < 0)
     {
        box_fill[0] = EINA_TRUE;
        box_align[0] = 0.5;
     }
   if (box_align[1] < 0)
     {
        box_fill[1] = EINA_TRUE;
        box_align[1] = 0.5;
     }

   if (pd->items)
     count = eina_array_count(pd->items);

   if (!count)
     {
        efl_gfx_size_hint_min_set(wd->resize_obj, 0, 0);
        return EINA_FALSE;
     }

   elm_interface_scrollable_content_viewport_geometry_get
      (ui_grid, NULL, NULL, &ow, &oh);
   // box outer margin
   boxw -= boxl + boxr;
   boxh -= boxt + boxb;
   boxx += boxl;
   boxy += boxt;

   // total space & available space
   if (horiz)
     {
        length = boxw;
        want = pd->realized.w;
        pad = pd->pad.scalable ? (pd->pad.h * scale) : pd->pad.h;

        // padding can not be squeezed (note: could make it an option)
        length -= pad * (count - 1);
        // available space. if <0 we overflow
        extra = length - want;

        pd->minw = pd->realized.w + boxl + boxr + pad * (pd->line_count);
        pd->minh = pd->realized.h + boxt + boxb;
        if (!pd->item_count && pd->avgith)
          pd->minw = pd->avgitw * (oh / pd->avgith);
        avgit = pd->avgitw;
     }
   else
     {
        length = boxh;
        want = pd->realized.h;
        pad = pd->pad.scalable ? (pd->pad.v * scale) : pd->pad.v;

        // padding can not be squeezed (note: could make it an option)
        length -= pad * (count - 1);
        // available space. if <0 we overflow
        extra = length - want;

        pd->minw = pd->realized.w + boxl + boxr;
        pd->minh = pd->realized.h + pad * (pd->line_count) + boxt + boxb;
        if (!pd->item_count && pd->avgitw)
          pd->minh = pd->avgith * (ow / pd->avgitw);
        avgit = pd->avgith;
     }
   SHDBG("MIN[%d,%d], AVG[%d,%d]", pd->minw, pd->minh, pd->avgitw, pd->avgith);

   efl_gfx_size_hint_min_set(wd->resize_obj, pd->minw, pd->minh);

   if (extra < 0) extra = 0;

   weight[0] = pd->weight.x;
   weight[1] = pd->weight.y;
   if (EINA_DBL_EQ(weight[!horiz], 0))
     {
        if (box_fill[!horiz])
          {
             // box is filled, set all weights to be equal
             zeroweight = EINA_TRUE;
          }
        else
          {
             // move bounding box according to box align
             cur_pos = extra * box_align[!horiz];
          }
        weight[!horiz] = count;
     }

   cur_pos += avgit * (pd->realized.start -1);

   SHINF("--- length[%d] want[%d] pad[%d] extra[%d] :: curpos[%.0f]", length, want, pad, extra, cur_pos);
   id = 0;

   // scan all items, get their properties, calculate total weight & min size
   EINA_ARRAY_ITER_NEXT(pd->items, i, litem, iterator)
     {
        o = litem->layout;
        double cx, cy, cw, ch, x, y, w, h;
        double align[2];
        int item_pad[4], max[2];

        efl_gfx_size_hint_align_get(o, &align[0], &align[1]);
        efl_gfx_size_hint_max_get(o, &max[0], &max[1]);
        efl_gfx_size_hint_margin_get(o, &item_pad[0], &item_pad[1], &item_pad[2], &item_pad[3]);

        if (align[0] < 0) align[0] = -1;
        if (align[1] < 0) align[1] = -1;
        if (align[0] > 1) align[0] = 1;
        if (align[1] > 1) align[1] = 1;

        if (max[0] <= 0) max[0] = INT_MAX;
        if (max[1] <= 0) max[1] = INT_MAX;
        if (max[0] < litem->minw) max[0] = litem->minw;
        if (max[1] < litem->minh) max[1] = litem->minh;

        id++;

        // extra rounding up (compensate cumulative error)
        if ((id == (count - 1)) && (cur_pos - floor(cur_pos) >= 0.5))
          rounding = 1;

        cw = litem->minw + rounding + (zeroweight ? 1.0 : litem->wx) * extra / weight[0];
        ch = litem->minh + rounding + (zeroweight ? 1.0 : litem->wy) * extra / weight[1];

        if (horiz)
          {
             cx = boxx + cur_pos;
             cy = boxy;
          }
        else
          {
             cx = boxx;
             cy = boxy + cur_pos;
          }

        // horizontally
        if (max[0] < INT_MAX)
          {
             w = MIN(MAX(litem->minw - item_pad[0] - item_pad[1], max[0]), cw);
             if (align[0] < 0)
               {
                  // bad case: fill+max are not good together
                  x = cx + ((cw - w) * box_align[0]) + item_pad[0];
               }
             else
               x = cx + ((cw - w) * align[0]) + item_pad[0];
          }
        else if (align[0] < 0)
          {
             // fill x
             w = cw - item_pad[0] - item_pad[1];
             x = cx + item_pad[0];
          }
        else
          {
             w = litem->minw - item_pad[0] - item_pad[1];
             x = cx + ((cw - w) * align[0]) + item_pad[0];
          }

        // vertically
        if (max[1] < INT_MAX)
          {
             h = MIN(MAX(litem->minh - item_pad[2] - item_pad[3], max[1]), ch);
             if (align[1] < 0)
               {
                  // bad case: fill+max are not good together
                  y = cy + ((ch - h) * box_align[1]) + item_pad[2];
               }
             else
               y = cy + ((ch - h) * align[1]) + item_pad[2];
          }
        else if (align[1] < 0)
          {
             // fill y
             h = ch - item_pad[2] - item_pad[3];
             y = cy + item_pad[2];
          }
        else
          {
             h = litem->minh - item_pad[2] - item_pad[3];
             y = cy + ((ch - h) * align[1]) + item_pad[2];
          }

        if (h > oh) h = oh;
        if (w > ow) w = ow;

        if (horiz)
          {
             if (oh < linesum + h)
               {
                  cur_pos += w + pad;
                  x += w + pad;
                  linesum = h;
                  row = 0;
                  col++;
               }
             else
               {
                  y += linesum;
                  linesum += h;
                  row++;
               }
          }
        else
          {
             if (ow < linesum + w)
               {
                  cur_pos += h + pad;
                  y += h + pad;
                  linesum = w;
                  col = 0;
                  row++;
               }
             else
               {
                  x += linesum;
                  linesum += w;
                  col++;
               }
          }
        litem->x = x;
        litem->y = y;
        litem->pos[0] = row;
        litem->pos[1] = col;

        //FIMXME TEMP CODE!!!
        if (horiz) pd->minw = x + w;
        else pd->minh = y + h;

        SHINF("[%d]--- [%.0f,%.0f,%.0f,%.0f::%.0f] --> GFX[%.0f,%.0f,%.0f,%.0f][%d,%d] lsum[%.0f]",
            i, cx, cy, cw, ch, cur_pos, x, y, w, h, row,col, linesum);

        efl_gfx_geometry_set(o, (int)(x + 0 - pd->pan.x), (int)(y + 0 - pd->pan.y), (int)w, (int)h);
        //SHINF("o[%d] cur=%.2f moved to X=%.2f, Y=%.2f", litem->index, cur_pos, x, y);
     }

   SHDBG("view update done. [%d] children updated, and [%d,%d] size is expanded", id, pd->minw, pd->minh);

   pd->need_update = EINA_FALSE;

   if (pd->minw != oldminw || pd->minh != oldminh) return EINA_TRUE;
   return EINA_FALSE;
}

EOLIAN static void
_efl_ui_gridview_view_update(Eo *obj EINA_UNUSED,
                             Efl_Ui_Gridview_Data *pd EINA_UNUSED,
                             Eina_Bool sync)
{
  pd->need_update = EINA_TRUE;

  SHDBG("View will be Updated");

  if (sync)
    _view_update_internal(obj);
  efl_canvas_group_change(pd->pan.obj);
}


/* Internal EO APIs and hidden overrides */

#define EFL_UI_GRIDVIEW_EXTRA_OPS \
   EFL_CANVAS_GROUP_ADD_DEL_OPS(efl_ui_gridview)

#include "efl_ui_gridview.eo.c"
