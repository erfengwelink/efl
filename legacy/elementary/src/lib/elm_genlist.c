#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <fnmatch.h>

#define ELM_INTERFACE_ATSPI_ACCESSIBLE_PROTECTED
#define ELM_INTERFACE_ATSPI_WIDGET_ACTION_PROTECTED
#define ELM_WIDGET_ITEM_PROTECTED

#include <Elementary.h>
#include <Elementary_Cursor.h>

#include "elm_priv.h"
#include "elm_widget_genlist.h"
#include "elm_interface_scrollable.h"

#define MY_PAN_CLASS ELM_GENLIST_PAN_CLASS

#define MY_PAN_CLASS_NAME "Elm_Genlist_Pan"
#define MY_PAN_CLASS_NAME_LEGACY "elm_genlist_pan"

#define MY_CLASS ELM_GENLIST_CLASS

#define MY_CLASS_NAME "Elm_Genlist"
#define MY_CLASS_NAME_LEGACY "elm_genlist"

// internally allocated
#define CLASS_ALLOCATED     0x3a70f11f

#define MAX_ITEMS_PER_BLOCK 32
#define REORDER_EFFECT_TIME 0.5
#define MULTI_DOWN_TIME 1.0
#define SWIPE_TIME 0.4
#define SCR_HOLD_TIME 0.1

#define ERR_ABORT(_msg)                         \
   do {                                         \
        ERR(_msg);                              \
        if (getenv("ELM_ERROR_ABORT")) abort(); \
   } while (0)

#define ELM_PRIV_GENLIST_SIGNALS(cmd) \
    cmd(SIG_ACTIVATED, "activated", "") \
    cmd(SIG_CLICKED_DOUBLE, "clicked,double", "") \
    cmd(SIG_CLICKED_RIGHT, "clicked,right", "") \
    cmd(SIG_SELECTED, "selected", "") \
    cmd(SIG_UNSELECTED, "unselected", "") \
    cmd(SIG_EXPANDED, "expanded", "") \
    cmd(SIG_CONTRACTED, "contracted", "") \
    cmd(SIG_EXPAND_REQUEST, "expand,request", "") \
    cmd(SIG_CONTRACT_REQUEST, "contract,request", "") \
    cmd(SIG_REALIZED, "realized", "") \
    cmd(SIG_UNREALIZED, "unrealized", "") \
    cmd(SIG_DRAG_START_UP, "drag,start,up", "") \
    cmd(SIG_DRAG_START_DOWN, "drag,start,down", "") \
    cmd(SIG_DRAG_START_LEFT, "drag,start,left", "") \
    cmd(SIG_DRAG_START_RIGHT, "drag,start,right", "") \
    cmd(SIG_DRAG_STOP, "drag,stop", "") \
    cmd(SIG_DRAG, "drag", "") \
    cmd(SIG_LONGPRESSED, "longpressed", "") \
    cmd(SIG_SCROLL, "scroll", "") \
    cmd(SIG_SCROLL_ANIM_START, "scroll,anim,start", "") \
    cmd(SIG_SCROLL_ANIM_STOP, "scroll,anim,stop", "") \
    cmd(SIG_SCROLL_DRAG_START, "scroll,drag,start", "") \
    cmd(SIG_SCROLL_DRAG_STOP, "scroll,drag,stop", "") \
    cmd(SIG_EDGE_TOP, "edge,top", "") \
    cmd(SIG_EDGE_BOTTOM, "edge,bottom", "") \
    cmd(SIG_EDGE_LEFT, "edge,left", "") \
    cmd(SIG_EDGE_RIGHT, "edge,right", "") \
    cmd(SIG_VBAR_DRAG, "vbar,drag", "") \
    cmd(SIG_VBAR_PRESS, "vbar,press", "") \
    cmd(SIG_VBAR_UNPRESS, "vbar,unpress", "") \
    cmd(SIG_HBAR_DRAG, "hbar,drag", "") \
    cmd(SIG_HBAR_PRESS, "hbar,press", "") \
    cmd(SIG_HBAR_UNPRESS, "hbar,unpress", "") \
    cmd(SIG_MULTI_SWIPE_LEFT, "multi,swipe,left", "") \
    cmd(SIG_MULTI_SWIPE_RIGHT, "multi,swipe,right", "") \
    cmd(SIG_MULTI_SWIPE_UP, "multi,swipe,up", "") \
    cmd(SIG_MULTI_SWIPE_DOWN, "multi,swipe,down", "") \
    cmd(SIG_MULTI_PINCH_OUT, "multi,pinch,out", "") \
    cmd(SIG_MULTI_PINCH_IN, "multi,pinch,in", "") \
    cmd(SIG_SWIPE, "swipe", "") \
    cmd(SIG_MOVED, "moved", "") \
    cmd(SIG_MOVED_AFTER, "moved,after", "") \
    cmd(SIG_MOVED_BEFORE, "moved,before", "") \
    cmd(SIG_INDEX_UPDATE, "index,update", "") \
    cmd(SIG_TREE_EFFECT_FINISHED , "tree,effect,finished", "") \
    cmd(SIG_HIGHLIGHTED, "highlighted", "") \
    cmd(SIG_UNHIGHLIGHTED, "unhighlighted", "") \
    cmd(SIG_ITEM_FOCUSED, "item,focused", "") \
    cmd(SIG_ITEM_UNFOCUSED, "item,unfocused", "") \
    cmd(SIG_PRESSED, "pressed", "") \
    cmd(SIG_RELEASED, "released", "") \
    cmd(SIG_ITEM_REORDER_START, "item,reorder,anim,start", "") \
    cmd(SIG_ITEM_REORDER_STOP, "item,reorder,anim,stop", "")

ELM_PRIV_GENLIST_SIGNALS(ELM_PRIV_STATIC_VARIABLE_DECLARE);

static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   ELM_PRIV_GENLIST_SIGNALS(ELM_PRIV_SMART_CALLBACKS_DESC)
   {SIG_WIDGET_LANG_CHANGED, ""}, /**< handled by elm_widget */
   {SIG_WIDGET_ACCESS_CHANGED, ""}, /**< handled by elm_widget */
   {SIG_LAYOUT_FOCUSED, ""}, /**< handled by elm_layout */
   {SIG_LAYOUT_UNFOCUSED, ""}, /**< handled by elm_layout */

   {NULL, NULL}
};
#undef ELM_PRIV_GENLIST_SIGNALS

/* edje signals internally used */
static const char SIGNAL_ENABLED[] = "elm,state,enabled";
static const char SIGNAL_DISABLED[] = "elm,state,disabled";
static const char SIGNAL_SELECTED[] = "elm,state,selected";
static const char SIGNAL_UNSELECTED[] = "elm,state,unselected";
static const char SIGNAL_EXPANDED[] = "elm,state,expanded";
static const char SIGNAL_CONTRACTED[] = "elm,state,contracted";
static const char SIGNAL_FLIP_ENABLED[] = "elm,state,flip,enabled";
static const char SIGNAL_FLIP_DISABLED[] = "elm,state,flip,disabled";
static const char SIGNAL_DECORATE_ENABLED[] = "elm,state,decorate,enabled";
static const char SIGNAL_DECORATE_DISABLED[] = "elm,state,decorate,disabled";
static const char SIGNAL_DECORATE_ENABLED_EFFECT[] = "elm,state,decorate,enabled,effect";
static const char SIGNAL_REORDER_ENABLED[] = "elm,state,reorder,enabled";
static const char SIGNAL_REORDER_DISABLED[] = "elm,state,reorder,disabled";
static const char SIGNAL_REORDER_MODE_SET[] = "elm,state,reorder,mode_set";
static const char SIGNAL_REORDER_MODE_UNSET[] = "elm,state,reorder,mode_unset";
static const char SIGNAL_CONTRACT_FLIP[] = "elm,state,contract_flip";
static const char SIGNAL_SHOW[] = "elm,state,show";
static const char SIGNAL_HIDE[] = "elm,state,hide";
static const char SIGNAL_FLIP_ITEM[] = "elm,action,flip_item";
static const char SIGNAL_ODD[] = "elm,state,odd";
static const char SIGNAL_EVEN[] = "elm,state,even";
static const char SIGNAL_FOCUSED[] = "elm,state,focused";
static const char SIGNAL_UNFOCUSED[] = "elm,state,unfocused";
static const char SIGNAL_LIST_SINGLE[] = "elm,state,list,single";
static const char SIGNAL_LIST_FIRST[] = "elm,state,list,first";
static const char SIGNAL_LIST_LAST[] = "elm,state,list,last";
static const char SIGNAL_LIST_MIDDLE[] = "elm,state,list,middle";
static const char SIGNAL_GROUP_SINGLE[] = "elm,state,group,single";
static const char SIGNAL_GROUP_FIRST[] = "elm,state,group,first";
static const char SIGNAL_GROUP_LAST[] = "elm,state,group,last";
static const char SIGNAL_GROUP_MIDDLE[] = "elm,state,group,middle";

static Eina_Bool _key_action_move(Evas_Object *obj, const char *params);
static Eina_Bool _key_action_select(Evas_Object *obj, const char *params);
static Eina_Bool _key_action_escape(Evas_Object *obj, const char *params);
static void _item_move_after(Elm_Gen_Item *it, Elm_Gen_Item *after);
static void _item_move_before(Elm_Gen_Item *it, Elm_Gen_Item *before);
static void  _calc_job(void *data);
static Eina_Bool _item_block_recalc(Item_Block *itb, int in, Eina_Bool qadd);
static void _item_mouse_callbacks_add(Elm_Gen_Item *it, Evas_Object *view);
static void _item_mouse_callbacks_del(Elm_Gen_Item *it, Evas_Object *view);
static void _access_activate_cb(void *data EINA_UNUSED,
                                Evas_Object *part_obj EINA_UNUSED,
                                Elm_Object_Item *item);
static void _decorate_item_set(Elm_Gen_Item *);
static void _internal_elm_genlist_clear(Evas_Object *obj, Eina_Bool standby);

static const Elm_Action key_actions[] = {
   {"move", _key_action_move},
   {"select", _key_action_select},
   {"escape", _key_action_escape},
   {NULL, NULL}
};


static Eina_Bool
_is_no_select(Elm_Gen_Item *it)
{
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   if ((sd->select_mode == ELM_OBJECT_SELECT_MODE_NONE) ||
       (sd->select_mode == ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY) ||
       (it->select_mode == ELM_OBJECT_SELECT_MODE_NONE) ||
       (it->select_mode == ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY))
     return EINA_TRUE;
   return EINA_FALSE;
}

EOLIAN static void
_elm_genlist_pan_elm_pan_pos_set(Eo *obj, Elm_Genlist_Pan_Data *psd, Evas_Coord x, Evas_Coord y)
{
   Item_Block *itb;

   Elm_Genlist_Data *sd = psd->wsd;

   if ((x == sd->pan_x) && (y == sd->pan_y)) return;
   sd->pan_x = x;
   sd->pan_y = y;

   EINA_INLIST_FOREACH(sd->blocks, itb)
     {
        if ((itb->y + itb->h) > y)
          {
             Elm_Gen_Item *it;
             Eina_List *l2;

             EINA_LIST_FOREACH(itb->items, l2, it)
               {
                  if ((itb->y + it->y) >= y)
                    {
                       sd->anchor_item = it;
                       sd->anchor_y = -(itb->y + it->y - y);
                       goto done;
                    }
               }
          }
     }
done:
   if (!sd->reorder_move_animator) evas_object_smart_changed(obj);
}

EOLIAN static void
_elm_genlist_pan_elm_pan_pos_get(Eo *obj EINA_UNUSED, Elm_Genlist_Pan_Data *psd, Evas_Coord *x, Evas_Coord *y)
{
   if (x) *x = psd->wsd->pan_x;
   if (y) *y = psd->wsd->pan_y;
}

EOLIAN static void
_elm_genlist_pan_elm_pan_pos_max_get(Eo *obj, Elm_Genlist_Pan_Data *psd, Evas_Coord *x, Evas_Coord *y)
{
   Evas_Coord ow, oh;

   evas_object_geometry_get(obj, NULL, NULL, &ow, &oh);
   ow = psd->wsd->minw - ow;
   if (ow < 0) ow = 0;
   oh = psd->wsd->minh - oh;
   if (oh < 0) oh = 0;
   if (x) *x = ow;
   if (y) *y = oh;
}

EOLIAN static void
_elm_genlist_pan_elm_pan_pos_min_get(Eo *obj EINA_UNUSED, Elm_Genlist_Pan_Data *_pd EINA_UNUSED, Evas_Coord *x, Evas_Coord *y)
{
   if (x) *x = 0;
   if (y) *y = 0;
}

EOLIAN static void
_elm_genlist_pan_elm_pan_content_size_get(Eo *obj EINA_UNUSED, Elm_Genlist_Pan_Data *psd, Evas_Coord *w, Evas_Coord *h)
{
   if (w) *w = psd->wsd->minw;
   if (h) *h = psd->wsd->minh;
}

EOLIAN static void
_elm_genlist_pan_evas_object_smart_del(Eo *obj, Elm_Genlist_Pan_Data *psd)
{
   ecore_job_del(psd->resize_job);

   eo_do_super(obj, MY_PAN_CLASS, evas_obj_smart_del());
}

EOLIAN static void
_elm_genlist_pan_evas_object_smart_move(Eo *obj, Elm_Genlist_Pan_Data *psd, Evas_Coord _gen_param2 EINA_UNUSED, Evas_Coord _gen_param3 EINA_UNUSED)
{
   psd->wsd->pan_changed = EINA_TRUE;
   evas_object_smart_changed(obj);
   ELM_SAFE_FREE(psd->wsd->calc_job, ecore_job_del);
}

static void
_elm_genlist_pan_smart_resize_job(void *data)
{
   ELM_GENLIST_PAN_DATA_GET(data, psd);

   elm_layout_sizing_eval(psd->wobj);
   psd->resize_job = NULL;
}

EOLIAN static void
_elm_genlist_pan_evas_object_smart_resize(Eo *obj, Elm_Genlist_Pan_Data *psd, Evas_Coord w, Evas_Coord h)
{
   Evas_Coord ow, oh;

   Elm_Genlist_Data *sd = psd->wsd;

   evas_object_geometry_get(obj, NULL, NULL, &ow, &oh);
   if ((ow == w) && (oh == h)) return;
   if ((sd->mode == ELM_LIST_COMPRESS) && (ow != w))
     {
        /* fix me later */
        ecore_job_del(psd->resize_job);
        psd->resize_job =
          ecore_job_add(_elm_genlist_pan_smart_resize_job, obj);
     }
   sd->pan_changed = EINA_TRUE;
   evas_object_smart_changed(obj);
   ecore_job_del(sd->calc_job);
   // if the width changed we may have to resize content if scrollbar went
   // away or appeared to queue a job to deal with it. it should settle in
   // the end to a steady-state
   if (ow != w)
     sd->calc_job = ecore_job_add(_calc_job, psd->wobj);
   else
     sd->calc_job = NULL;
}

static void
_item_text_realize(Elm_Gen_Item *it,
                   Evas_Object *target,
                   Eina_List **source,
                   const char *parts)
{
   if (it->itc->func.text_get)
     {
        const Eina_List *l;
        const char *key;

        if (!(*source))
          *source = elm_widget_stringlist_get
              (edje_object_data_get(target, "texts"));
        EINA_LIST_FOREACH(*source, l, key)
          {
             if (parts && fnmatch(parts, key, FNM_PERIOD))
               continue;

             char *s = it->itc->func.text_get
                 ((void *)WIDGET_ITEM_DATA_GET(EO_OBJ(it)), WIDGET(it), key);

             if (s)
               {
                  edje_object_part_text_escaped_set(target, key, s);
                  free(s);
               }
             else
               {
                  edje_object_part_text_set(target, key, "");
               }
          }
     }
}

static Eina_List *
_item_content_realize(Elm_Gen_Item *it,
                      Evas_Object *target,
                      Eina_List **source,
                      const char *parts,
                      Eina_List **contents_list)
{
   Eina_List *res = *contents_list;

   if (it->itc->func.content_get)
     {
        const Eina_List *l;
        const char *key;
        Evas_Object *ic = NULL;

        if (!(*source))
          *source = elm_widget_stringlist_get
              (edje_object_data_get(target, "contents"));

        EINA_LIST_FOREACH(*source, l, key)
          {
             if (parts && fnmatch(parts, key, FNM_PERIOD))
               continue;

             if (it->itc->func.content_get)
               ic = it->itc->func.content_get
                   ((void *)WIDGET_ITEM_DATA_GET(EO_OBJ(it)), WIDGET(it), key);
             if (ic)
               {
                  res = eina_list_append(res, ic);
                  edje_object_part_swallow(target, key, ic);
                  evas_object_show(ic);
                  elm_widget_sub_object_add(WIDGET(it), ic);
                  if (eo_do(EO_OBJ(it), elm_wdg_item_disabled_get()))
                    elm_widget_disabled_set(ic, EINA_TRUE);
               }
          }
     }

   return res;
}

static void
_item_state_realize(Elm_Gen_Item *it,
                    Evas_Object *target,
                    Eina_List **source,
                    const char *parts)
{
   if (it->itc->func.state_get)
     {
        const Eina_List *l;
        const char *key;
        char buf[4096];

        if (!(*source))
          *source = elm_widget_stringlist_get
              (edje_object_data_get(target, "states"));
        EINA_LIST_FOREACH(*source, l, key)
          {
             if (parts && fnmatch(parts, key, FNM_PERIOD))
               continue;

             Eina_Bool on = it->itc->func.state_get
                 ((void *)WIDGET_ITEM_DATA_GET(EO_OBJ(it)), WIDGET(it), key);

             if (on)
               {
                  snprintf(buf, sizeof(buf), "elm,state,%s,active", key);
                  edje_object_signal_emit(target, buf, "elm");
               }
             else
               {
                  snprintf(buf, sizeof(buf), "elm,state,%s,passive", key);
                  edje_object_signal_emit(target, buf, "elm");
               }
          }
        edje_object_message_signal_process(target);
     }
}

/**
 * Apply the right style for the created item view.
 */
static void
_view_style_update(Elm_Gen_Item *it, Evas_Object *view, const char *style)
{
   char buf[1024];
   const char *stacking_even;
   const char *stacking;
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   // FIXME:  There exists
   // item, item_compress, item_odd, item_compress_odd,
   // tree, tree_compress, tree_odd, tree_odd_compress
   // But those used case by case. :-(
   // Anyway, belows codes are for backward..
   if (it->decorate_it_set)
     {
        // item, item_compress, item_odd, item_compress_odd
        strncpy(buf, "item", sizeof(buf));
        if (sd->mode == ELM_LIST_COMPRESS)
           strncat(buf, "_compress", sizeof(buf) - strlen(buf) - 1);

        if (it->item->order_num_in & 0x1)
           strncat(buf, "_odd", sizeof(buf) - strlen(buf) - 1);
        strncat(buf, "/", sizeof(buf) - strlen(buf) - 1);
        strncat(buf, style, sizeof(buf) - strlen(buf) - 1);
     }
   else
     {
        // item, item_compress, tree, tree_compress
        if (it->item->type & ELM_GENLIST_ITEM_TREE)
           snprintf(buf, sizeof(buf), "tree%s/%s",
                    sd->mode == ELM_LIST_COMPRESS ? "_compress" :
                    "", style ? : "default");
        else
           snprintf(buf, sizeof(buf), "item%s/%s",
                    sd->mode == ELM_LIST_COMPRESS ? "_compress" :
                    "",style ? : "default");
     }

   if (!elm_widget_theme_object_set(WIDGET(it), view,
                                    "genlist", buf,
                                    elm_widget_style_get(WIDGET(it))))
     {
        ERR("%s is not a valid genlist item style. "
            "Automatically falls back into default style.",
            style);
        elm_widget_theme_object_set
          (WIDGET(it), VIEW(it), "genlist", "item/default", "default");
     }

   edje_object_mirrored_set(view, elm_widget_mirrored_get(WIDGET(it)));
   edje_object_scale_set(view, elm_widget_scale_get(WIDGET(it))
                         * elm_config_scale_get());

   stacking_even = edje_object_data_get(VIEW(it), "stacking_even");
   if (!stacking_even) stacking_even = "above";
   it->item->stacking_even = !!strcmp("above", stacking_even);

   stacking = edje_object_data_get(VIEW(it), "stacking");
   if (!stacking) stacking = "yes";
   it->item->nostacking = !!strcmp("yes", stacking);
}

/**
 * Create a VIEW(it) during _item_realize()
 */
static Evas_Object *
_view_create(Elm_Gen_Item *it, const char *style)
{
   Evas_Object *view = edje_object_add(evas_object_evas_get(WIDGET(it)));
   evas_object_smart_member_add(view, GL_IT(it)->wsd->pan_obj);
   elm_widget_sub_object_add(WIDGET(it), view);
   edje_object_scale_set(view, elm_widget_scale_get(WIDGET(it)) *
                         elm_config_scale_get());

   _view_style_update(it, view, style);
   return view;
}

static void
_view_clear(Evas_Object *view, Eina_List **texts, Eina_List **contents,
            Eina_List **states, Eina_List **content_objs)
{
   const char *part;
   Evas_Object *c;
   const Eina_List *l;

   EINA_LIST_FOREACH(*texts, l, part)
     edje_object_part_text_set(view, part, NULL);

   ELM_SAFE_FREE(*texts, elm_widget_stringlist_free);
   ELM_SAFE_FREE(*contents, elm_widget_stringlist_free);
   ELM_SAFE_FREE(*states, elm_widget_stringlist_free);

   EINA_LIST_FREE(*content_objs, c)
     evas_object_del(c);
}

static void
_item_scroll(Elm_Genlist_Data *sd)
{
   Evas_Coord gith = 0;
   Elm_Gen_Item *it = NULL;
   Evas_Coord ow, oh, dx = 0, dy = 0, dw = 0, dh = 0;

   if (!sd->show_item) return;

   evas_object_geometry_get(sd->pan_obj, NULL, NULL, &ow, &oh);
   it = sd->show_item;
   dx = it->x + it->item->block->x;
   dy = it->y + it->item->block->y;
   dw = it->item->block->w;
   dh = oh;

   switch (sd->scroll_to_type)
     {
      case ELM_GENLIST_ITEM_SCROLLTO_TOP:
        if (it->item->group_item) gith = it->item->group_item->item->h;
        dy -= gith;
        break;

      case ELM_GENLIST_ITEM_SCROLLTO_MIDDLE:
        dy += ((it->item->h / 2) - (oh / 2));
        break;

      case ELM_GENLIST_ITEM_SCROLLTO_IN:
      default:
        if ((sd->expanded_item) &&
            ((sd->show_item->y + sd->show_item->item->block->y
              + sd->show_item->item->h) -
             (sd->expanded_item->y + sd->expanded_item->item->block->y) > oh))
          {
             it = sd->expanded_item;
             if (it->item->group_item) gith = it->item->group_item->item->h;
             dx = it->x + it->item->block->x;
             dy = it->y + it->item->block->y - gith;
             dw = it->item->block->w;
          }
        else
          {
             if ((it->item->group_item) &&
                 (sd->pan_y > (it->y + it->item->block->y)))
               gith = it->item->group_item->item->h;
             dy -= gith;
             dh = it->item->h;
          }
        break;
     }
   if (sd->bring_in)
      eo_do(sd->obj, elm_interface_scrollable_region_bring_in(dx, dy, dw, dh));
   else
      eo_do(sd->obj, elm_interface_scrollable_content_region_show
            (dx, dy, dw, dh));

   it->item->show_me = EINA_FALSE;
   sd->show_item = NULL;
   sd->auto_scroll_enabled = EINA_FALSE;
   sd->check_scroll = EINA_FALSE;
}

static void
_elm_genlist_item_unrealize(Elm_Gen_Item *it,
                            Eina_Bool calc)
{
   if (!it->realized) return;
   if (GL_IT(it)->wsd->reorder_it == it) return;

   evas_event_freeze(evas_object_evas_get(WIDGET(it)));
   if (!calc)
     evas_object_smart_callback_call(WIDGET(it), SIG_UNREALIZED, EO_OBJ(it));
   ELM_SAFE_FREE(it->long_timer, ecore_timer_del);

   _view_clear(VIEW(it), &(it->texts), &(it->contents), &(it->states), &(it->content_objs));

   ELM_SAFE_FREE(it->item_focus_chain, eina_list_free);

   eo_do(EO_OBJ(it), elm_wdg_item_track_cancel());

   it->unrealize_cb(it);

   it->realized = EINA_FALSE;
   it->want_unrealize = EINA_FALSE;

   evas_event_thaw(evas_object_evas_get(WIDGET(it)));
   evas_event_thaw_eval(evas_object_evas_get(WIDGET(it)));
}

static void
_item_block_unrealize(Item_Block *itb)
{
   Elm_Gen_Item *it;
   const Eina_List *l;
   Eina_Bool dragging = EINA_FALSE;

   if (!itb->realized) return;
   evas_event_freeze(evas_object_evas_get((itb->sd)->obj));

   EINA_LIST_FOREACH(itb->items, l, it)
     {
        if (itb->must_recalc || !it->group)
          {
             if (it->dragging)
               {
                  dragging = EINA_TRUE;
                  it->want_unrealize = EINA_TRUE;
               }
             else
               _elm_genlist_item_unrealize(it, EINA_FALSE);
          }
     }
   if (!dragging)
     {
        itb->realized = EINA_FALSE;
        itb->want_unrealize = EINA_TRUE;
     }
   else
     itb->want_unrealize = EINA_FALSE;
   evas_event_thaw(evas_object_evas_get((itb->sd)->obj));
   evas_event_thaw_eval(evas_object_evas_get((itb->sd)->obj));
}

static Eina_Bool
_must_recalc_idler(void *data)
{
   ELM_GENLIST_DATA_GET(data, sd);

   ecore_job_del(sd->calc_job);
   sd->calc_job = ecore_job_add(_calc_job, data);
   sd->must_recalc_idler = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_calc_job(void *data)
{
   int in = 0;
   Item_Block *itb, *chb = NULL;
   Evas_Coord pan_w = 0, pan_h = 0;
   ELM_GENLIST_DATA_GET(data, sd);
   Eina_Bool minw_change = EINA_FALSE;
   Eina_Bool did_must_recalc = EINA_FALSE;
   Evas_Coord minw = -1, minh = 0, y = 0, ow, dy = 0, vw = 0;

   evas_object_geometry_get(sd->pan_obj, NULL, NULL, &ow, &sd->h);
   if (sd->mode == ELM_LIST_COMPRESS)
      eo_do(sd->obj, elm_interface_scrollable_content_viewport_geometry_get
            (NULL, NULL, &vw, NULL));

   if (sd->w != ow) sd->w = ow;

   evas_event_freeze(evas_object_evas_get(sd->obj));
   EINA_INLIST_FOREACH(sd->blocks, itb)
     {
        Eina_Bool show_me = EINA_FALSE;

        itb->num = in;
        show_me = itb->show_me;
        itb->show_me = EINA_FALSE;
        if (chb)
          {
             if (itb->realized) _item_block_unrealize(itb);
          }
        if ((itb->changed) || ((itb->must_recalc) && (!did_must_recalc)))
          {
             if (itb->must_recalc)
               {
                  Eina_List *l;
                  Elm_Gen_Item *it;

                  EINA_LIST_FOREACH(itb->items, l, it)
                    it->item->mincalcd = EINA_FALSE;
                  itb->changed = EINA_TRUE;
                  did_must_recalc = EINA_TRUE;
                  if (itb->realized) _item_block_unrealize(itb);
                  itb->must_recalc = EINA_FALSE;
               }
             show_me = _item_block_recalc(itb, in, EINA_FALSE);
             chb = itb;
          }
        itb->y = y;
        itb->x = 0;
        minh += itb->minh;
        if (minw < itb->minw)
          {
             minw = itb->minw;
             if (minw != -1)
               minw_change = EINA_TRUE;
          }
        if ((sd->mode == ELM_LIST_COMPRESS) && (minw > vw))
          {
             minw = vw;
             minw_change = EINA_TRUE;
          }
        itb->w = minw;
        itb->h = itb->minh;
        y += itb->h;
        in += itb->count;
        if ((show_me) && (sd->show_item) && (!sd->show_item->item->queued))
          sd->check_scroll = EINA_TRUE;
     }
   if (minw_change)
     {
        EINA_INLIST_FOREACH(sd->blocks, itb)
          {
             itb->minw = minw;
             itb->w = itb->minw;
          }
     }
   if ((chb) && (EINA_INLIST_GET(chb)->next))
     {
        EINA_INLIST_FOREACH(EINA_INLIST_GET(chb)->next, itb)
          {
             if (itb->realized) _item_block_unrealize(itb);
          }
     }
   sd->realminw = minw;
   if (minw < sd->w) minw = sd->w;
   if ((minw != sd->minw) || (minh != sd->minh))
     {
        sd->minw = minw;
        sd->minh = minh;
        evas_object_smart_callback_call(sd->pan_obj, "changed", NULL);
        elm_layout_sizing_eval(sd->obj);
        if (sd->reorder_it)
          {
              Elm_Gen_Item *it;
              it = sd->reorder_it;
              it->item->w = minw;
          }
        if ((sd->anchor_item) && (sd->anchor_item->item->block)
            && (!sd->auto_scroll_enabled))
          {
             Elm_Gen_Item *it;
             Evas_Coord it_y;

             it = sd->anchor_item;
             it_y = sd->anchor_y;
             eo_do(sd->obj, elm_interface_scrollable_content_pos_set
               (sd->pan_x, it->item->block->y
               + it->y + it_y, EINA_TRUE));
             sd->anchor_item = it;
             sd->anchor_y = it_y;
          }
     }
   if (did_must_recalc)
     {
        if (!sd->must_recalc_idler)
          sd->must_recalc_idler = ecore_idler_add(_must_recalc_idler, data);
     }
   if (sd->check_scroll)
     {
        eo_do(sd->pan_obj, elm_obj_pan_content_size_get(&pan_w, &pan_h));
        if (EINA_INLIST_GET(sd->show_item) == sd->items->last)
          sd->scroll_to_type = ELM_GENLIST_ITEM_SCROLLTO_IN;

        switch (sd->scroll_to_type)
          {
           case ELM_GENLIST_ITEM_SCROLLTO_TOP:
             dy = sd->h;
             break;

           case ELM_GENLIST_ITEM_SCROLLTO_MIDDLE:
             dy = sd->h / 2;
             break;

           case ELM_GENLIST_ITEM_SCROLLTO_IN:
           default:
             dy = 0;
             break;
          }
        if ((sd->show_item) && (sd->show_item->item->block))
          {
             if ((pan_w > (sd->show_item->x + sd->show_item->item->block->x))
                 && (pan_h > (sd->show_item->y + sd->show_item->item->block->y
                              + dy)))
               {
                  _item_scroll(sd);
               }
          }
     }

   sd->calc_job = NULL;
   evas_object_smart_changed(sd->pan_obj);
   evas_event_thaw(evas_object_evas_get(sd->obj));
   evas_event_thaw_eval(evas_object_evas_get(sd->obj));
}

EOLIAN static void
_elm_genlist_elm_layout_sizing_eval(Eo *obj, Elm_Genlist_Data *sd)
{
   Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;
   Evas_Coord vmw = 0, vmh = 0;

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (sd->on_sub_del) return;;

   evas_object_size_hint_min_get(obj, &minw, NULL);
   evas_object_size_hint_max_get(obj, &maxw, &maxh);

   edje_object_size_min_calc(wd->resize_obj, &vmw, &vmh);

   if (sd->mode == ELM_LIST_COMPRESS)
     {
        Evas_Coord vw = 0, vh = 0;

        eo_do(obj, elm_interface_scrollable_content_viewport_geometry_get
              (NULL, NULL, &vw, &vh));
        if ((vw != 0) && (vw != sd->prev_viewport_w))
          {
             Item_Block *itb;

             sd->prev_viewport_w = vw;

             EINA_INLIST_FOREACH(sd->blocks, itb)
               {
                  itb->must_recalc = EINA_TRUE;
               }
             ecore_job_del(sd->calc_job);
             sd->calc_job = ecore_job_add(_calc_job, obj);
          }
        minw = vmw;
        minh = vmh;
     }
   else if (sd->mode == ELM_LIST_LIMIT)
     {
        maxw = -1;
        minw = vmw + sd->realminw;
     }
   else
     {
        minw = vmw;
        minh = vmh;
     }

   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, maxw, maxh);
}

static void
_item_contract_emit(Elm_Object_Item *eo_it)
{
   ELM_GENLIST_ITEM_DATA_GET(eo_it, it);
   Elm_Object_Item *eo_it2;
   Eina_List *l;

   //XXX: for compat
   edje_object_signal_emit(VIEW(it), SIGNAL_CONTRACT_FLIP, "");
   edje_object_signal_emit(VIEW(it), SIGNAL_CONTRACT_FLIP, "elm");
   it->item->tree_effect_finished = EINA_FALSE;

   EINA_LIST_FOREACH(it->item->items, l, eo_it2)
     if (eo_it2) _item_contract_emit(eo_it2);
}

static int
_item_tree_effect_before(Elm_Gen_Item *it)
{
   Elm_Object_Item *eo_it2;
   Eina_List *l;
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   EINA_LIST_FOREACH(it->item->items, l, eo_it2)
     {
        ELM_GENLIST_ITEM_DATA_GET(eo_it2, it2);
        if (it2->parent && (it == it2->parent))
          {
             if (!it2->realized)
               it2->item->tree_effect_hide_me = EINA_TRUE;
             if (sd->move_effect_mode ==
                 ELM_GENLIST_TREE_EFFECT_EXPAND)
               {
                  //XXX: for compat
                  edje_object_signal_emit(VIEW(it2), SIGNAL_HIDE, "");
                  edje_object_signal_emit(VIEW(it2), SIGNAL_HIDE, "elm");
               }
             else if (sd->move_effect_mode ==
                      ELM_GENLIST_TREE_EFFECT_CONTRACT)
               _item_contract_emit(eo_it2);
          }
     }
   return ECORE_CALLBACK_CANCEL;
}

static void
_item_position(Elm_Gen_Item *it,
               Evas_Object *view,
               Evas_Coord it_x,
               Evas_Coord it_y)
{
   if (!it) return;
   if (!view) return;
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   evas_event_freeze
     (evas_object_evas_get(sd->obj));
   evas_object_resize(view, it->item->w, it->item->h);
   evas_object_move(view, it_x, it_y);
   evas_object_show(view);
   evas_event_thaw(evas_object_evas_get(sd->obj));
   evas_event_thaw_eval
     (evas_object_evas_get(sd->obj));
}

static void
_item_tree_effect(Elm_Genlist_Data *sd,
                  int y)
{
   Elm_Gen_Item *expanded_next_it;
   Elm_Object_Item *eo_it;

   expanded_next_it = sd->expanded_next_item;

   if (sd->move_effect_mode == ELM_GENLIST_TREE_EFFECT_EXPAND)
     {
        eo_it = elm_genlist_item_prev_get(EO_OBJ(expanded_next_it));
        while (eo_it)
          {
             ELM_GENLIST_ITEM_DATA_GET(eo_it, it);
             if (it->item->expanded_depth <=
                 expanded_next_it->item->expanded_depth) break;
             if (it->item->scrl_y &&
                 (it->item->scrl_y <= expanded_next_it->item->old_scrl_y + y)
                 && (it->item->expanded_depth >
                     expanded_next_it->item->expanded_depth))
               {
                  if (!it->item->tree_effect_finished)
                    {
                       //XXX: for compat
                       edje_object_signal_emit(VIEW(it), "flip_item", "");
                       edje_object_signal_emit(VIEW(it), SIGNAL_FLIP_ITEM,
                                               "elm");
                       _item_position
                         (it, VIEW(it), it->item->scrl_x, it->item->scrl_y);
                       it->item->tree_effect_finished = EINA_TRUE;
                    }
               }
             eo_it = elm_genlist_item_prev_get(eo_it);
          }
     }
   else if (sd->move_effect_mode == ELM_GENLIST_TREE_EFFECT_CONTRACT)
     {
        eo_it = elm_genlist_item_prev_get(EO_OBJ(expanded_next_it));
        while (eo_it)
          {
             ELM_GENLIST_ITEM_DATA_GET(eo_it, it);
             if ((it->item->scrl_y > expanded_next_it->item->old_scrl_y + y) &&
                 (it->item->expanded_depth >
                  expanded_next_it->item->expanded_depth))
               {
                  if (!it->item->tree_effect_finished)
                    {
                       //XXX: for compat
                       edje_object_signal_emit(VIEW(it), SIGNAL_HIDE, "");
                       edje_object_signal_emit(VIEW(it), SIGNAL_HIDE, "elm");
                       it->item->tree_effect_finished = EINA_TRUE;
                    }
               }
             else
               break;
             eo_it = elm_genlist_item_prev_get(eo_it);
          }
     }
}

static void
_item_sub_items_clear(Elm_Gen_Item *it)
{
   Eina_List *tl = NULL, *l;
   Elm_Object_Item *eo_it2;

   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it);

   EINA_LIST_FOREACH(it->item->items, l, eo_it2)
     tl = eina_list_append(tl, eo_it2);
   EINA_LIST_FREE(tl, eo_it2)
     eo_do(eo_it2, elm_wdg_item_del());
}

static void
_item_auto_scroll(Elm_Genlist_Data *sd)
{
   Elm_Object_Item *eo_tmp_item = NULL;

   if ((sd->expanded_item) && (sd->auto_scroll_enabled))
     {
        eo_tmp_item = eina_list_data_get
            (eina_list_last(sd->expanded_item->item->items));
        if (!eo_tmp_item) return;
        ELM_GENLIST_ITEM_DATA_GET(eo_tmp_item, tmp_item);
        sd->show_item = tmp_item;
        sd->bring_in = EINA_TRUE;
        sd->scroll_to_type = ELM_GENLIST_ITEM_SCROLLTO_IN;
        if ((sd->show_item->item->queued) || (!sd->show_item->item->mincalcd))
          {
             sd->show_item->item->show_me = EINA_TRUE;
             sd->auto_scroll_enabled = EINA_FALSE;
          }
        else
          _item_scroll(sd);
     }
}

static void
_item_tree_effect_finish(Elm_Genlist_Data *sd)
{
   Elm_Object_Item *eo_it = NULL;
   const Eina_List *l;

   if (sd->tree_effect_animator)
     {
        if (sd->move_effect_mode == ELM_GENLIST_TREE_EFFECT_CONTRACT)
          _item_sub_items_clear(sd->expanded_item);

        EINA_LIST_FOREACH(sd->expanded_item->item->items, l, eo_it)
          {
             ELM_GENLIST_ITEM_DATA_GET(eo_it, it);
             it->item->tree_effect_finished = EINA_TRUE;
             it->item->old_scrl_y = it->item->scrl_y;
             if (GL_IT(it)->wsd->move_effect_mode ==
                 ELM_GENLIST_TREE_EFFECT_EXPAND)
               {
                  //XXX: for compat
                  edje_object_signal_emit(VIEW(it), SIGNAL_SHOW, "");
                  edje_object_signal_emit(VIEW(it), SIGNAL_SHOW, "elm");
               }
          }
     }

   _item_auto_scroll(sd);
   evas_object_lower(sd->event_block_rect);
   evas_object_hide(sd->event_block_rect);
   sd->move_effect_mode = ELM_GENLIST_TREE_EFFECT_NONE;
   sd->move_items = eina_list_free(sd->move_items);

   evas_object_smart_callback_call(sd->pan_obj, "changed", NULL);
   evas_object_smart_callback_call
     (sd->obj, SIG_TREE_EFFECT_FINISHED, NULL);
   evas_object_smart_changed(sd->pan_obj);

   sd->tree_effect_animator = NULL;
}

static void
_elm_genlist_item_position_state_update(Elm_Gen_Item *it)
{
   unsigned idx = it->item->order_num_in;

   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   if (!it->item->nostacking)
     {
        if ((idx & 0x1) ^ it->item->stacking_even)
          {
             if (it->deco_all_view) evas_object_stack_below(it->deco_all_view, sd->stack[0]);
             else evas_object_stack_below(VIEW(it), sd->stack[0]);
          }
        else
          {
             if (it->deco_all_view) evas_object_stack_above(it->deco_all_view, sd->stack[0]);
             else evas_object_stack_above(VIEW(it), sd->stack[0]);
          }
     }

   if (idx & 0x1)
     {
        edje_object_signal_emit(VIEW(it), SIGNAL_ODD, "elm");
        if (it->deco_all_view)
          edje_object_signal_emit(it->deco_all_view, SIGNAL_ODD, "elm");
     }
   else
     {
        edje_object_signal_emit(VIEW(it), SIGNAL_EVEN, "elm");
        if (it->deco_all_view)
          edje_object_signal_emit(it->deco_all_view, SIGNAL_EVEN, "elm");
     }

   if (sd->item_count == 1)
     {
        edje_object_signal_emit(VIEW(it), SIGNAL_LIST_SINGLE, "elm");
        if (it->deco_all_view)
          edje_object_signal_emit(it->deco_all_view, SIGNAL_LIST_SINGLE, "elm");
     }
   else if (idx == 0)
     {
        edje_object_signal_emit(VIEW(it), SIGNAL_LIST_FIRST, "elm");
        if (it->deco_all_view)
          edje_object_signal_emit(it->deco_all_view, SIGNAL_LIST_FIRST, "elm");
     }
   else if (idx == sd->item_count - 1)
     {
        edje_object_signal_emit(VIEW(it), SIGNAL_LIST_LAST, "elm");
        if (it->deco_all_view)
          edje_object_signal_emit(it->deco_all_view, SIGNAL_LIST_LAST, "elm");
     }
   else if (idx > 0)
     {
        edje_object_signal_emit(VIEW(it), SIGNAL_LIST_MIDDLE, "elm");
        if (it->deco_all_view)
          edje_object_signal_emit(it->deco_all_view, SIGNAL_LIST_MIDDLE, "elm");
     }

   if (it->parent)
     {
        unsigned first_idx = it->parent->item->order_num_in + 1;
        unsigned count = eina_list_count(it->parent->item->items);

        if (count == 1)
          {
             edje_object_signal_emit(VIEW(it), SIGNAL_GROUP_SINGLE, "elm");
             if (it->deco_all_view)
               edje_object_signal_emit(it->deco_all_view, SIGNAL_GROUP_SINGLE,
                                       "elm");
          }
        else if (idx == first_idx)
          {
             edje_object_signal_emit(VIEW(it), SIGNAL_GROUP_FIRST, "elm");
             if (it->deco_all_view)
               edje_object_signal_emit(it->deco_all_view, SIGNAL_GROUP_FIRST,
                                       "elm");
          }
        else if (EO_OBJ(it) == eina_list_data_get(eina_list_last(it->parent->item->items)))
          {
             edje_object_signal_emit(VIEW(it), SIGNAL_GROUP_LAST, "elm");
             if (it->deco_all_view)
               edje_object_signal_emit(it->deco_all_view, SIGNAL_GROUP_LAST,
                                       "elm");
          }
        else if (idx > first_idx)
          {
             edje_object_signal_emit(VIEW(it), SIGNAL_GROUP_MIDDLE, "elm");
             if (it->deco_all_view)
               edje_object_signal_emit(it->deco_all_view, SIGNAL_GROUP_MIDDLE,
                                       "elm");
          }
     }
}

static void
_item_order_update(const Eina_Inlist *l,
                   int start)
{
   Elm_Gen_Item *it, *it2;

   /*
    * always update position state of previous item, it may have been
    * marked as "single" if it was the only element at the time, or
    * "middle", "first" or "last" in the case of insert into different
    * positions.
    */
   if ((l->prev) && (start > 0))
     {
        it = ELM_GEN_ITEM_FROM_INLIST(l->prev);
        it->item->order_num_in = start - 1;
        _elm_genlist_item_position_state_update(it);
     }

   for (it = ELM_GEN_ITEM_FROM_INLIST(l); l; l = l->next,
        it = ELM_GEN_ITEM_FROM_INLIST(l))
     {
        it->item->order_num_in = start++;
        _elm_genlist_item_position_state_update(it);
        it2 = ELM_GEN_ITEM_FROM_INLIST(l->next);
        if (it2 && (it->item->order_num_in != it2->item->order_num_in))
          return;
     }
}

static void
_elm_genlist_item_state_update(Elm_Gen_Item *it,
                               Item_Cache *itc)
{
   if (itc)
     {
        if (it->selected != itc->selected)
          {
             if (it->selected)
               {
                  edje_object_signal_emit(VIEW(it), SIGNAL_SELECTED, "elm");
                  if (it->deco_all_view)
                    edje_object_signal_emit
                      (it->deco_all_view, SIGNAL_SELECTED, "elm");
               }
          }
        if (eo_do(EO_OBJ(it), elm_wdg_item_disabled_get()) != itc->disabled)
          {
             if (eo_do(EO_OBJ(it), elm_wdg_item_disabled_get()))
               edje_object_signal_emit(VIEW(it), SIGNAL_DISABLED,"elm");
             if (it->deco_all_view)
               edje_object_signal_emit
                 (it->deco_all_view, SIGNAL_DISABLED, "elm");
          }
        if (it->item->expanded != itc->expanded)
          {
             if (it->item->expanded)
               edje_object_signal_emit(VIEW(it), SIGNAL_EXPANDED, "elm");
             if (it->deco_all_view)
               edje_object_signal_emit
                 (it->deco_all_view, SIGNAL_EXPANDED, "elm");
          }
     }
   else
     {
        if (it->selected)
          {
             edje_object_signal_emit(VIEW(it), SIGNAL_SELECTED, "elm");
             if (it->deco_all_view)
               edje_object_signal_emit
                 (it->deco_all_view, SIGNAL_SELECTED, "elm");
          }
        if (eo_do(EO_OBJ(it), elm_wdg_item_disabled_get()))
          {
             edje_object_signal_emit(VIEW(it), SIGNAL_DISABLED, "elm");
             if (it->deco_all_view)
               edje_object_signal_emit
                 (it->deco_all_view, SIGNAL_DISABLED, "elm");
          }
        if (it->item->expanded)
          {
             edje_object_signal_emit(VIEW(it), SIGNAL_EXPANDED, "elm");
             if (it->deco_all_view)
               edje_object_signal_emit
                 (it->deco_all_view, SIGNAL_EXPANDED, "elm");
          }
     }
}

static void
_elm_genlist_item_index_update(Elm_Gen_Item *it)
{
   if (it->position_update || it->item->block->position_update)
     {
        evas_object_smart_callback_call(WIDGET(it), SIG_INDEX_UPDATE, EO_OBJ(it));
        it->position_update = EINA_FALSE;
     }
}

static void
_decorate_all_item_position(Elm_Gen_Item *it,
                            int itx,
                            int ity)
{
   if ((!it) || (!GL_IT(it)->wsd->decorate_all_mode)) return;
   evas_object_resize(it->deco_all_view, it->item->w, it->item->h);
   evas_object_move(it->deco_all_view, itx, ity);
}

static void
_decorate_all_item_realize(Elm_Gen_Item *it,
                           Eina_Bool effect_on)
{
   char buf[1024];
   const char *stacking;
   const char *stacking_even;

   if (!it) return;
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   if ((it->item->decorate_all_item_realized) ||
       (it->generation < sd->generation))
     return;

   it->deco_all_view = edje_object_add(evas_object_evas_get(WIDGET(it)));
   edje_object_scale_set(it->deco_all_view, elm_widget_scale_get(WIDGET(it)) *
                         elm_config_scale_get());
   evas_object_smart_member_add(it->deco_all_view, sd->pan_obj);
   elm_widget_sub_object_add(WIDGET(it), it->deco_all_view);

   if (it->item->type & ELM_GENLIST_ITEM_TREE)
     strncpy(buf, "tree", sizeof(buf));
   else strncpy(buf, "item", sizeof(buf));
   if (sd->mode == ELM_LIST_COMPRESS)
     strncat(buf, "_compress", sizeof(buf) - strlen(buf) - 1);

   strncat(buf, "/", sizeof(buf) - strlen(buf) - 1);
   strncat(buf, it->itc->decorate_all_item_style, sizeof(buf) - strlen(buf) - 1);

   elm_widget_theme_object_set(WIDGET(it), it->deco_all_view, "genlist", buf,
                               elm_widget_style_get(WIDGET(it)));

   stacking_even = edje_object_data_get(VIEW(it), "stacking_even");
   if (!stacking_even) stacking_even = "above";
   it->item->stacking_even = !!strcmp("above", stacking_even);

   stacking = edje_object_data_get(VIEW(it), "stacking");
   if (!stacking) stacking = "yes";
   it->item->nostacking = !!strcmp("yes", stacking);

   edje_object_mirrored_set
     (it->deco_all_view, elm_widget_mirrored_get(WIDGET(it)));

   _elm_genlist_item_position_state_update(it);
   _elm_genlist_item_state_update(it, NULL);

   if (effect_on)
     edje_object_signal_emit
        (it->deco_all_view, SIGNAL_DECORATE_ENABLED_EFFECT, "elm");
   else
     edje_object_signal_emit
        (it->deco_all_view, SIGNAL_DECORATE_ENABLED, "elm");

   _item_mouse_callbacks_del(it, VIEW(it));
   _item_mouse_callbacks_add(it, it->deco_all_view);

   _item_text_realize(it, it->deco_all_view, &it->item->deco_all_texts, NULL);
   if (it->flipped)
     edje_object_signal_emit
       (it->deco_all_view, SIGNAL_FLIP_ENABLED, "elm");
   if (!it->item->deco_all_contents)
     it->item->deco_all_contents = elm_widget_stringlist_get
         (edje_object_data_get(it->deco_all_view, "contents"));
   it->item->deco_all_content_objs =
      _item_content_realize(it, it->deco_all_view,
                            &it->item->deco_all_contents, NULL,
                            &it->item->deco_all_content_objs);
   _item_state_realize
      (it, it->deco_all_view, &it->item->deco_all_states, NULL);
   edje_object_part_swallow
     (it->deco_all_view, "elm.swallow.decorate.content", VIEW(it));

   _decorate_all_item_position(it, it->item->scrl_x, it->item->scrl_y);
   evas_object_show(it->deco_all_view);

   if (it->selected)
     edje_object_signal_emit(it->deco_all_view, SIGNAL_SELECTED, "elm");

   it->item->decorate_all_item_realized = EINA_TRUE;
   it->want_unrealize = EINA_FALSE;
}

//-- tree expand/contract signal handle routine --//
static void
_expand_toggle_signal_cb(void *data,
                         Evas_Object *obj EINA_UNUSED,
                         const char *emission EINA_UNUSED,
                         const char *source EINA_UNUSED)
{
   Elm_Gen_Item *it = data;

   if (it->item->expanded)
     evas_object_smart_callback_call(WIDGET(it), SIG_CONTRACT_REQUEST, EO_OBJ(it));
   else
     evas_object_smart_callback_call(WIDGET(it), SIG_EXPAND_REQUEST, EO_OBJ(it));
}

static void
_expand_signal_cb(void *data,
                  Evas_Object *obj EINA_UNUSED,
                  const char *emission EINA_UNUSED,
                  const char *source EINA_UNUSED)
{
   Elm_Gen_Item *it = data;

   if (!it->item->expanded)
     evas_object_smart_callback_call(WIDGET(it), SIG_EXPAND_REQUEST, EO_OBJ(it));
}

static void
_contract_signal_cb(void *data,
                    Evas_Object *obj EINA_UNUSED,
                    const char *emission EINA_UNUSED,
                    const char *source EINA_UNUSED)
{
   Elm_Gen_Item *it = data;

   if (it->item->expanded)
     evas_object_smart_callback_call(WIDGET(it), SIG_CONTRACT_REQUEST, EO_OBJ(it));
}

//-- item cache handle routine --//
// clean up item cache by removing overflowed caches
static void
_item_cache_clean(Elm_Genlist_Data *sd)
{
   evas_event_freeze(evas_object_evas_get(sd->obj));

   while ((sd->item_cache) && (sd->item_cache_count > sd->item_cache_max))
     {
        Item_Cache *itc;

        itc = EINA_INLIST_CONTAINER_GET(sd->item_cache->last, Item_Cache);
        sd->item_cache = eina_inlist_remove
            (sd->item_cache, sd->item_cache->last);
        sd->item_cache_count--;
        evas_object_del(itc->spacer);
        evas_object_del(itc->base_view);
        eina_stringshare_del(itc->item_style);
        free(itc);
     }
   evas_event_thaw(evas_object_evas_get(sd->obj));
   evas_event_thaw_eval(evas_object_evas_get(sd->obj));
}

// free one item cache
static void
_item_cache_free(Item_Cache *itc)
{
   evas_object_del(itc->spacer);
   evas_object_del(itc->base_view);
   eina_stringshare_del(itc->item_style);
   free(itc);
}

// empty all item caches
static void
_item_cache_zero(Elm_Genlist_Data *sd)
{
   int pmax = sd->item_cache_max;

   sd->item_cache_max = 0;
   _item_cache_clean(sd);
   sd->item_cache_max = pmax;
}

// add an item to item cache
static void
_item_cache_add(Elm_Gen_Item *it)
{
   Item_Cache *itc;
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);
   Evas_Object *obj = sd->obj;

   evas_event_freeze(evas_object_evas_get(obj));
   if (sd->item_cache_max <= 0)
     {
        ELM_SAFE_FREE(VIEW(it), evas_object_del);
        ELM_SAFE_FREE(it->spacer, evas_object_del);

        evas_event_thaw(evas_object_evas_get(obj));
        evas_event_thaw_eval(evas_object_evas_get(obj));

        return;
     }

   sd->item_cache_count++;
   itc = calloc(1, sizeof(Item_Cache));
   if (!itc)
     {
        evas_event_thaw(evas_object_evas_get(obj));
        evas_event_thaw_eval(evas_object_evas_get(obj));
        return;
     }
   sd->item_cache =
     eina_inlist_prepend(sd->item_cache, EINA_INLIST_GET(itc));
   itc->spacer = it->spacer;
   it->spacer = NULL;
   itc->base_view = VIEW(it);

   VIEW(it) = NULL;
   edje_object_signal_emit(itc->base_view, SIGNAL_UNSELECTED, "elm");
   evas_object_hide(itc->base_view);
   evas_object_move(itc->base_view, -9999, -9999);
   itc->item_style = eina_stringshare_add(it->itc->item_style);
   if (it->item->type & ELM_GENLIST_ITEM_TREE) itc->tree = 1;
   itc->selected = it->selected;
   itc->disabled = eo_do(EO_OBJ(it), elm_wdg_item_disabled_get());
   itc->expanded = it->item->expanded;
   ELM_SAFE_FREE(it->long_timer, ecore_timer_del);
   ELM_SAFE_FREE(it->item->swipe_timer, ecore_timer_del);

   // FIXME: other callbacks?
   edje_object_signal_callback_del_full
     (itc->base_view, "elm,action,expand,toggle", "elm",
     _expand_toggle_signal_cb, it);
   edje_object_signal_callback_del_full
     (itc->base_view, "elm,action,expand", "elm", _expand_signal_cb, it);
   edje_object_signal_callback_del_full
     (itc->base_view, "elm,action,contract", "elm", _contract_signal_cb, it);
   _item_mouse_callbacks_del(it, itc->base_view);
   _item_cache_clean(sd);

   evas_event_thaw(evas_object_evas_get(obj));
   evas_event_thaw_eval(evas_object_evas_get(obj));
}

// find an item from item cache and remove it from the cache
static Item_Cache *
_item_cache_find(Elm_Gen_Item *it)
{
   Item_Cache *itc = NULL;
   Eina_Inlist *l;
   Eina_Bool tree = 0;
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   if (it->item->type & ELM_GENLIST_ITEM_TREE) tree = 1;
   EINA_INLIST_FOREACH_SAFE(sd->item_cache, l, itc)
     {
        if ((itc->selected) || (itc->disabled) || (itc->expanded))
          continue;

        if ((itc->tree == tree) &&
            (((!it->itc->item_style) && (!itc->item_style)) ||
             (it->itc->item_style && itc->item_style &&
              (!strcmp(it->itc->item_style, itc->item_style)))))
          {
             sd->item_cache =
                eina_inlist_remove (sd->item_cache, EINA_INLIST_GET(itc));
             sd->item_cache_count--;

             return itc;
          }
     }
   return NULL;
}

static char *
_access_info_cb(void *data, Evas_Object *obj EINA_UNUSED)
{
   char *ret;
   Eina_Strbuf *buf;

   Elm_Gen_Item *it = (Elm_Gen_Item *)data;
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it, NULL);

   buf = eina_strbuf_new();

   if (it->itc->func.text_get)
     {
        const Eina_List *l;
        const char *key;

        if (!(it->texts)) it->texts =
          elm_widget_stringlist_get(edje_object_data_get(VIEW(it), "texts"));

        EINA_LIST_FOREACH(it->texts, l, key)
          {
             char *s = it->itc->func.text_get
                ((void *)WIDGET_ITEM_DATA_GET(EO_OBJ(it)), WIDGET(it), key);

             if (s)
               {
                  if (eina_strbuf_length_get(buf) > 0) eina_strbuf_append(buf, ", ");
                  eina_strbuf_append(buf, s);
                  free(s);
               }
          }
     }

   ret = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
   return ret;
}

static char *
_access_state_cb(void *data, Evas_Object *obj EINA_UNUSED)
{
   Elm_Gen_Item *it = (Elm_Gen_Item *)data;
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it, NULL);

   if (it->base->disabled)
     return strdup(E_("State: Disabled"));

   return NULL;
}

static void
_access_on_highlight_cb(void *data)
{
   Evas_Coord x, y, w, h;
   Evas_Coord sx, sy, sw, sh;
   Elm_Gen_Item *it = (Elm_Gen_Item *)data;
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it);

   evas_object_geometry_get(it->base->view, &x, &y, &w, &h);
   // XXX There would be a reason.
   if ((w == 0) && (h == 0)) return;

   evas_object_geometry_get(it->base->widget, &sx, &sy, &sw, &sh);
   if ((x < sx) || (y < sy) || ((x + w) > (sx + sw)) || ((y + h) > (sy + sh)))
     elm_genlist_item_bring_in(EO_OBJ(it),
                               ELM_GENLIST_ITEM_SCROLLTO_IN);
}

static void
_access_widget_item_register(Elm_Gen_Item *it)
{
   Elm_Access_Info *ai;

   _elm_access_widget_item_register(it->base);

   ai = _elm_access_info_get(it->base->access_obj);

   _elm_access_callback_set(ai, ELM_ACCESS_INFO, _access_info_cb, it);
   _elm_access_callback_set(ai, ELM_ACCESS_STATE, _access_state_cb, it);
   _elm_access_on_highlight_hook_set(ai, _access_on_highlight_cb, it);
   _elm_access_activate_callback_set(ai, _access_activate_cb, EO_OBJ(it));
}

static void
_item_realize(Elm_Gen_Item *it,
              int in,
              Eina_Bool calc)
{
   Item_Cache *itc = NULL;
   const char *treesize;
   int tsize = 20;
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   if (it->generation < sd->generation) return;
   if (it->realized)
     {
        if (it->item->order_num_in != in)
          {
             _item_order_update(EINA_INLIST_GET(it), in);
             _elm_genlist_item_state_update(it, NULL);
             _elm_genlist_item_index_update(it);
          }
        return;
     }
   it->item->order_num_in = in;

   if (it->item->nocache_once)
     it->item->nocache_once = EINA_FALSE;
   else if (!it->item->nocache)
     itc = _item_cache_find(it);
   if (itc && (!sd->tree_effect_enabled))
     {
        VIEW(it) = itc->base_view;
        itc->base_view = NULL;
        it->spacer = itc->spacer;
        itc->spacer = NULL;
     }
   else
     VIEW(it) = _view_create(it, it->itc->item_style);

   /* access */
   if (_elm_config->access_mode) _access_widget_item_register(it);

   _item_order_update(EINA_INLIST_GET(it), in);

   if (!(it->deco_all_view) && (it->item->type != ELM_GENLIST_ITEM_GROUP))
     {
        if (sd->reorder_mode)
          edje_object_signal_emit(VIEW(it), SIGNAL_REORDER_MODE_SET, "elm");
        else
          edje_object_signal_emit(VIEW(it), SIGNAL_REORDER_MODE_UNSET, "elm");
    }
   treesize = edje_object_data_get(VIEW(it), "treesize");
   if (treesize) tsize = atoi(treesize);

   if (edje_object_part_exists(VIEW(it), "elm.swallow.pad"))
     {
        if (!it->spacer && treesize)
          {
             it->spacer =
                evas_object_rectangle_add(evas_object_evas_get(WIDGET(it)));
             evas_object_color_set(it->spacer, 0, 0, 0, 0);
             elm_widget_sub_object_add(WIDGET(it), it->spacer);
          }

        evas_object_size_hint_min_set
           (it->spacer, (it->item->expanded_depth * tsize) *
            elm_config_scale_get(), 1);
        edje_object_part_swallow(VIEW(it), "elm.swallow.pad", it->spacer);
     }
   else
     {
        ELM_SAFE_FREE(it->spacer, evas_object_del);
     }

   if (!calc)
     {
        edje_object_signal_callback_add
          (VIEW(it), "elm,action,expand,toggle", "elm",
          _expand_toggle_signal_cb, it);
        edje_object_signal_callback_add
          (VIEW(it), "elm,action,expand", "elm", _expand_signal_cb, it);
        edje_object_signal_callback_add
          (VIEW(it), "elm,action,contract", "elm", _contract_signal_cb, it);
        _item_mouse_callbacks_add(it, VIEW(it));

        if ((sd->decorate_all_mode) && (!it->deco_all_view) &&
            (it->item->type != ELM_GENLIST_ITEM_GROUP) &&
            (it->itc->decorate_all_item_style))
          _decorate_all_item_realize(it, EINA_FALSE);

        _elm_genlist_item_state_update(it, itc);
        _elm_genlist_item_index_update(it);
     }

   /* homogeneous genlist shortcut */
   if ((calc) && (sd->homogeneous) && (!it->item->mincalcd) &&
       ((it->group && sd->group_item_width) ||
        (!it->group && sd->item_width)))
     {
        if (it->group)
          {
             it->item->w = it->item->minw = sd->group_item_width;
             it->item->h = it->item->minh = sd->group_item_height;
          }
        else
          {
             it->item->w = it->item->minw = sd->item_width;
             it->item->h = it->item->minh = sd->item_height;
          }
        it->item->mincalcd = EINA_TRUE;
     }
   else
     {
        if (eina_list_count(it->content_objs) != 0)
          ERR_ABORT("If you see this error, please notify us and we"
                    "will fix it");

        _item_text_realize(it, VIEW(it), &it->texts, NULL);
        it->content_objs =
           _item_content_realize(it, VIEW(it), &it->contents, NULL,
                                 &it->content_objs);
        if (it->has_contents != (!!it->content_objs))
          it->item->mincalcd = EINA_FALSE;
        it->has_contents = !!it->content_objs;
        _item_state_realize(it, VIEW(it), &it->states, NULL);
        if (it->flipped)
          {
             edje_object_signal_emit(VIEW(it), SIGNAL_FLIP_ENABLED, "elm");
             if (!(it->item->flip_contents))
               it->item->flip_contents = elm_widget_stringlist_get
                   (edje_object_data_get(VIEW(it), "flips"));
             it->item->flip_content_objs = _item_content_realize
                 (it, VIEW(it), &it->item->flip_contents, NULL,
                 &it->item->flip_content_objs);
          }

        /* access: unregister item which have no text and content */
        if (_elm_config->access_mode && !it->texts && !it->contents)
          _elm_access_widget_item_unregister(it->base);

        if (!it->item->mincalcd)
          {
             Evas_Coord mw = -1, mh = -1;

             if (it->select_mode != ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY)
               elm_coords_finger_size_adjust(1, &mw, 1, &mh);
             if (sd->mode == ELM_LIST_COMPRESS)
               mw = sd->prev_viewport_w;
             edje_object_size_min_restricted_calc(VIEW(it), &mw, &mh, mw, mh);
             if (it->select_mode != ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY)
               elm_coords_finger_size_adjust(1, &mw, 1, &mh);
             it->item->w = it->item->minw = mw;
             it->item->h = it->item->minh = mh;
             it->item->mincalcd = EINA_TRUE;

             if ((!sd->group_item_width) && (it->group))
               {
                  sd->group_item_width = mw;
                  sd->group_item_height = mh;
               }
             else if ((!sd->item_width) &&
                      (it->item->type == ELM_GENLIST_ITEM_NONE))
               {
                  sd->item_width = mw;
                  sd->item_height = mh;
               }
          }
        if (!calc) evas_object_show(VIEW(it));
     }

   if (it->tooltip.content_cb)
     {
        eo_do(EO_OBJ(it),
              elm_wdg_item_tooltip_content_cb_set(
                 it->tooltip.content_cb, it->tooltip.data, NULL),
              elm_wdg_item_tooltip_style_set(it->tooltip.style),
              elm_wdg_item_tooltip_window_mode_set(it->tooltip.free_size));
     }

   if (it->mouse_cursor)
     eo_do(EO_OBJ(it),
            elm_wdg_item_cursor_set(it->mouse_cursor));

   it->realized = EINA_TRUE;
   it->want_unrealize = EINA_FALSE;

   if (itc) _item_cache_free(itc);
   if (!calc)
     {
        if (it->item->tree_effect_hide_me)
          {
             if (sd->move_effect_mode
                 != ELM_GENLIST_TREE_EFFECT_NONE)
               {
                  //XXX: for compat
                  edje_object_signal_emit(VIEW(it), SIGNAL_HIDE, "");
                  edje_object_signal_emit(VIEW(it), SIGNAL_HIDE, "elm");
               }
             it->item->tree_effect_hide_me = EINA_FALSE;
          }

        if (it->item->type == ELM_GENLIST_ITEM_NONE)
          {
             Evas_Object* eobj;
             Eina_List* l;
             EINA_LIST_FOREACH(it->content_objs, l, eobj)
                if (elm_widget_is(eobj) && elm_object_focus_allow_get(eobj))
                  it->item_focus_chain = eina_list_append
                      (it->item_focus_chain, eobj);

          }

        if (it->item->type == ELM_GENLIST_ITEM_TREE)
          {
             Evas_Object* t_eobj;
             Eina_List* tl;
             EINA_LIST_FOREACH(it->content_objs, tl, t_eobj)
                if (elm_widget_is(t_eobj) && elm_object_focus_allow_get(t_eobj))
                  it->item_focus_chain = eina_list_append
                      (it->item_focus_chain, t_eobj);

          }

        evas_object_smart_callback_call(WIDGET(it), SIG_REALIZED, EO_OBJ(it));
     }

   //Send a signal so that an item changes its style according to its expand depth
   if (GL_IT(it)->expanded_depth > 0)
     {
        char buf[126];
        snprintf(buf, sizeof(buf), "elm,state,expanded_depth,%d",
                 GL_IT(it)->expanded_depth);
        edje_object_signal_emit(VIEW(it), buf, "elm");
     }

   if ((!calc) && (sd->decorate_all_mode) &&
       (it->item->type != ELM_GENLIST_ITEM_GROUP))
     {
        if (it->itc->decorate_all_item_style)
          {
             if (!it->deco_all_view)
               _decorate_all_item_realize(it, EINA_FALSE);
             edje_object_message_signal_process(it->deco_all_view);
          }
     }

   if (it->decorate_it_set) _decorate_item_set(it);

   edje_object_message_signal_process(VIEW(it));
}

static Eina_Bool
_tree_effect_animator_cb(void *data)
{
   int in = 0;
   const Eina_List *l;
   int y = 0, dy = 0, dh = 0;
   double effect_duration = 0.3, t;
   ELM_GENLIST_DATA_GET(data, sd);
   Eina_Bool end = EINA_FALSE, vis = EINA_TRUE;
   Evas_Coord ox, oy, ow, oh, cvx, cvy, cvw, cvh;
   Elm_Gen_Item *it = NULL, *it2, *expanded_next_it;
   Elm_Object_Item *eo_it = NULL, *eo_it2;

   t = ((0.0 > (t = ecore_time_get() - sd->start_time)) ? 0.0 : t);
   evas_object_geometry_get(sd->pan_obj, &ox, &oy, &ow, &oh);
   evas_output_viewport_get
     (evas_object_evas_get(sd->pan_obj), &cvx, &cvy, &cvw, &cvh);
   if (t > effect_duration) end = EINA_TRUE;

   // Below while statement is needed, when the genlist is resized.
   it2 = sd->expanded_item;
   eo_it2 = EO_OBJ(it2);
   while (eo_it2 && vis)
     {
        it2 = eo_data_scope_get(eo_it2, ELM_GENLIST_ITEM_CLASS);
        evas_object_move(VIEW(it2), it2->item->scrl_x, it2->item->scrl_y);
        vis = (ELM_RECTS_INTERSECT(it2->item->scrl_x, it2->item->scrl_y,
                                   it2->item->w, it2->item->h, cvx, cvy, cvw,
                                   cvh));
        eo_it2 = elm_genlist_item_prev_get(eo_it2);
     }

   if (sd->expanded_next_item)
     {
        expanded_next_it = sd->expanded_next_item;

        /* move items */
        EINA_LIST_FOREACH(sd->move_items, l, it)
          {
             if (sd->move_effect_mode == ELM_GENLIST_TREE_EFFECT_EXPAND)
               {
                  expanded_next_it->item->old_scrl_y =
                    sd->expanded_item->item->old_scrl_y
                    + sd->expanded_item->item->h;
                  if (expanded_next_it->item->scrl_y <=
                      expanded_next_it->item->old_scrl_y) /* did not
                                                           * calculate
                                                           * next item
                                                           * position */
                    expanded_next_it->item->scrl_y = cvy + cvh;

                  dy = ((expanded_next_it->item->scrl_y >= (cvy + cvh)) ?
                        cvy + cvh : expanded_next_it->item->scrl_y) -
                    expanded_next_it->item->old_scrl_y;
               }
             else if (sd->move_effect_mode == ELM_GENLIST_TREE_EFFECT_CONTRACT)
               {
                  if (expanded_next_it->item->scrl_y >
                      expanded_next_it->item->old_scrl_y) /* did not
                                                           * calculate
                                                           * next item
                                                           * position */
                    expanded_next_it->item->old_scrl_y = cvy + cvh;

                  if (expanded_next_it->item->old_scrl_y > (cvy + cvh))
                    {
                       dy = (sd->expanded_item->item->scrl_y +
                             sd->expanded_item->item->h) - cvy + cvh;
                       expanded_next_it->item->old_scrl_y = cvy + cvh;
                    }
                  else
                    {
                       dy = (sd->expanded_item->item->scrl_y +
                             sd->expanded_item->item->h) -
                         expanded_next_it->item->old_scrl_y;
                    }
               }

             if (t <= effect_duration)
               {
                  y = ((1 - (1 - (t / effect_duration)) *
                        (1 - (t / effect_duration))) * dy);
               }
             else
               {
                  end = EINA_TRUE;
                  y = dy;
               }

             if (!it->realized && !it->item->queued)
               _item_realize(it, in, 0);
             in++;

             if (it != expanded_next_it)
               {
                  it->item->old_scrl_y =
                    expanded_next_it->item->old_scrl_y +
                    expanded_next_it->item->h + dh;

                  dh += it->item->h;
               }

             if ((it->item->old_scrl_y + y) < (cvy + cvh))
               _item_position(it, VIEW(it), it->item->scrl_x,
                              it->item->old_scrl_y + y);
          }
        /* tree effect */
        _item_tree_effect(sd, y);
     }
   else
     {
        int expanded_item_num = 0;
        int num = 0;

        if (sd->expanded_item)
          eo_it = elm_genlist_item_next_get(EO_OBJ(sd->expanded_item));

        eo_it2 = eo_it;
        while (eo_it2)
          {
             expanded_item_num++;
             eo_it2 = elm_genlist_item_next_get(eo_it2);
          }

        while (eo_it)
          {
             it = eo_data_scope_get(eo_it, ELM_GENLIST_ITEM_CLASS);
             num++;
             if (sd->expanded_item->item->expanded_depth >=
                 it->item->expanded_depth) break;
             if (sd->move_effect_mode == ELM_GENLIST_TREE_EFFECT_EXPAND)
               {
                  if (!it->item->tree_effect_finished)
                    {
                       if (t >= (((num - 1) * effect_duration) /
                                 expanded_item_num))
                         {
                            //XXX: for compat
                            edje_object_signal_emit(VIEW(it), "flip_item", "");
                            edje_object_signal_emit(VIEW(it), SIGNAL_FLIP_ITEM,
                                                    "elm");
                            _item_position(it, VIEW(it), it->item->scrl_x,
                                           it->item->scrl_y);
                            it->item->tree_effect_finished = EINA_TRUE;
                         }
                    }
               }
             eo_it = elm_genlist_item_next_get(eo_it);
          }
     }

   if (end)
     {
        _item_tree_effect_finish(sd);
        return ECORE_CALLBACK_CANCEL;
     }

   return ECORE_CALLBACK_RENEW;
}

static void
_group_items_recalc(void *data)
{
   Eina_List *l;
   Elm_Gen_Item *git;
   Elm_Genlist_Data *sd = data;

   evas_event_freeze(evas_object_evas_get(sd->obj));
   EINA_LIST_FOREACH(sd->group_items, l, git)
     {
        if (git->item->want_realize)
          {
             if (!git->realized) _item_realize(git, 0, EINA_FALSE);
             evas_object_resize(VIEW(git), sd->minw, git->item->h);
             evas_object_move(VIEW(git), git->item->scrl_x, git->item->scrl_y);
             evas_object_stack_above(VIEW(git), sd->stack[1]);
             evas_object_show(VIEW(git));
          }
        else if (!git->item->want_realize && git->realized)
          {
             if (!git->dragging)
               _elm_genlist_item_unrealize(git, EINA_FALSE);
          }
     }
   evas_event_thaw(evas_object_evas_get(sd->obj));
   evas_event_thaw_eval(evas_object_evas_get(sd->obj));
}

static Eina_Bool
_reorder_move_animator_cb(void *data)
{
   double t;
   Elm_Gen_Item *it = data;
   Eina_Bool down = EINA_FALSE;
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);
   int y, dy = it->item->h / 10 * elm_config_scale_get(), diff;

   t = ((0.0 > (t = ecore_loop_time_get()
                  - sd->start_time)) ? 0.0 : t);

   if (t <= REORDER_EFFECT_TIME)
     y = (1 * sin((t / REORDER_EFFECT_TIME) * (M_PI / 2)) * dy);
   else y = dy;

   diff = abs(it->item->old_scrl_y - it->item->scrl_y);
   if (diff < dy) y = diff;
   else if (diff > it->item->h) y = diff / 2;

   if (it->item->old_scrl_y < it->item->scrl_y)
     {
        it->item->old_scrl_y += y;
        down = EINA_TRUE;
     }
   else if (it->item->old_scrl_y > it->item->scrl_y)
     {
        it->item->old_scrl_y -= y;
        down = EINA_FALSE;
     }

   if (it->deco_all_view)
     _item_position
       (it, it->deco_all_view, it->item->scrl_x, it->item->old_scrl_y);
   else
     _item_position(it, VIEW(it), it->item->scrl_x, it->item->old_scrl_y);
   _group_items_recalc(sd);

   if ((sd->reorder_pan_move) ||
       (down && it->item->old_scrl_y >= it->item->scrl_y) ||
       (!down && it->item->old_scrl_y <= it->item->scrl_y))
     {
        it->item->old_scrl_y = it->item->scrl_y;
        it->item->move_effect_enabled = EINA_FALSE;
        sd->reorder_move_animator = NULL;
        return ECORE_CALLBACK_CANCEL;
     }

   return ECORE_CALLBACK_RENEW;
}

static int
_reorder_item_space_get(Elm_Gen_Item *it)
{
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);
   Elm_Gen_Item *reorder_it = sd->reorder_it;
   Evas_Coord rox, roy, row, roh, oy, oh;
   Eina_Bool top = EINA_FALSE;

   if (!reorder_it) return 0;

   evas_object_geometry_get(sd->pan_obj, NULL, &oy, NULL, &oh);
   evas_object_geometry_get
     (sd->VIEW(reorder_it), &rox, &roy, &row, &roh);

   if ((sd->reorder_start_y < it->item->block->y) &&
       (roy - oy + (roh / 2) >= it->item->block->y - sd->pan_y))
     {
        it->item->block->reorder_offset =
          sd->reorder_it->item->h * -1;
        if (it->item->block->count == 1)
          sd->reorder_rel = it;
     }
   else if ((sd->reorder_start_y >= it->item->block->y) &&
            (roy - oy + (roh / 2) <= it->item->block->y - sd->pan_y))
     {
        it->item->block->reorder_offset = sd->reorder_it->item->h;
     }
   else
     it->item->block->reorder_offset = 0;

   it->item->scrl_y += it->item->block->reorder_offset;

   top = (ELM_RECTS_INTERSECT
            (it->item->scrl_x, it->item->scrl_y, it->item->w, it->item->h,
            rox, roy + (roh / 2), row, 1));
   if (top)
     {
        sd->reorder_rel = it;
        it->item->scrl_y += sd->reorder_it->item->h;
        return sd->reorder_it->item->h;
     }
   else
     return 0;
}

static void
_item_block_position(Item_Block *itb,
                     int in)
{
   Elm_Gen_Item *it;
   Elm_Gen_Item *git;
   const Eina_List *l;
   Eina_Bool vis = EINA_FALSE;
   Evas_Coord y = 0, ox, oy, ow, oh, cvx, cvy, cvw, cvh;
   Elm_Genlist_Data *sd = NULL;

   evas_event_freeze(evas_object_evas_get((itb->sd)->obj));
   evas_object_geometry_get(itb->sd->pan_obj, &ox, &oy, &ow, &oh);
   evas_output_viewport_get
     (evas_object_evas_get((itb->sd)->obj),
     &cvx, &cvy, &cvw, &cvh);

   EINA_LIST_FOREACH(itb->items, l, it)
     {
        sd = GL_IT(it)->wsd;
        if (it->generation < sd->generation) continue;
        else if (sd->reorder_it == it)
          continue;

        it->x = 0;
        it->y = y;
        it->item->w = itb->w;
        it->item->scrl_x = itb->x + it->x - sd->pan_x + ox;
        it->item->scrl_y = itb->y + it->y - sd->pan_y + oy;

        vis = (ELM_RECTS_INTERSECT
                 (it->item->scrl_x, it->item->scrl_y, it->item->w, it->item->h,
                 cvx, cvy, cvw, cvh));
        if (!it->group)
          {
             if ((itb->realized) && (!it->realized))
               {
                  if (vis) _item_realize(it, in, EINA_FALSE);
               }
             if (it->realized)
               {
                  if (vis || it->dragging)
                    {
                       if (sd->reorder_mode)
                         y += _reorder_item_space_get(it);
                       git = it->item->group_item;
                       if (git)
                         {
                            if (git->item->scrl_y < oy)
                              git->item->scrl_y = oy;
                            if ((git->item->scrl_y + git->item->h) >
                                (it->item->scrl_y + it->item->h))
                              git->item->scrl_y = (it->item->scrl_y +
                                                   it->item->h) - git->item->h;
                            git->item->scrl_x = it->item->scrl_x;
                            git->item->want_realize = EINA_TRUE;
                         }
                       if ((sd->reorder_it) &&
                           (it->item->old_scrl_y != it->item->scrl_y))
                         {
                            if (!it->item->move_effect_enabled)
                              {
                                 it->item->move_effect_enabled = EINA_TRUE;
                                 sd->reorder_move_animator =
                                   ecore_animator_add(
                                     _reorder_move_animator_cb, it);
                              }
                         }
                       if (!it->item->move_effect_enabled)
                         {
                            if ((sd->decorate_all_mode) &&
                                (it->itc->decorate_all_item_style))
                              _decorate_all_item_position
                                (it, it->item->scrl_x, it->item->scrl_y);
                            else
                              {
                                 if (!sd->tree_effect_enabled ||
                                     (sd->move_effect_mode ==
                                      ELM_GENLIST_TREE_EFFECT_NONE) ||
                                     ((sd->move_effect_mode !=
                                       ELM_GENLIST_TREE_EFFECT_NONE) &&
                                      (it->item->old_scrl_y ==
                                       it->item->scrl_y)))
                                   {
                                      if (it->item->deco_it_view)
                                        _item_position
                                          (it, it->item->deco_it_view,
                                          it->item->scrl_x,
                                          it->item->scrl_y);
                                      else
                                        _item_position
                                          (it, VIEW(it), it->item->scrl_x,
                                          it->item->scrl_y);
                                   }
                              }
                            it->item->old_scrl_y = it->item->scrl_y;
                         }
                    }
                  else
                    {
                       if (!sd->tree_effect_animator)
                         _elm_genlist_item_unrealize(it, EINA_FALSE);
                    }
               }
             in++;
          }
        else
          {
             if (vis) it->item->want_realize = EINA_TRUE;
          }
        y += it->item->h;
     }
   evas_event_thaw(evas_object_evas_get((itb->sd)->obj));
   evas_event_thaw_eval(evas_object_evas_get((itb->sd)->obj));
}

static void
_item_block_realize(Item_Block *itb)
{
   if (itb->realized) return;

   itb->realized = EINA_TRUE;
   itb->want_unrealize = EINA_FALSE;
}

EOLIAN static void
_elm_genlist_pan_evas_object_smart_calculate(Eo *obj, Elm_Genlist_Pan_Data *psd)
{
   Evas_Coord ox, oy, ow, oh, cvx, cvy, cvw, cvh;
   Evas_Coord vx = 0, vy = 0, vw = 0, vh = 0;
   Elm_Gen_Item *git;
   Item_Block *itb;
   Eina_List *l;
   int in = 0;

   Elm_Genlist_Data *sd = psd->wsd;

   evas_event_freeze(evas_object_evas_get(obj));

   if (sd->pan_changed)
     {
        ecore_job_del(sd->calc_job);
        sd->calc_job = NULL;
        _calc_job(sd->obj);
        sd->pan_changed = EINA_FALSE;
     }

   evas_object_geometry_get(obj, &ox, &oy, &ow, &oh);
   evas_output_viewport_get(evas_object_evas_get(obj), &cvx, &cvy, &cvw, &cvh);
   EINA_LIST_FOREACH(sd->group_items, l, git)
     {
        git->item->want_realize = EINA_FALSE;
     }

   if (sd->tree_effect_enabled &&
       (sd->move_effect_mode != ELM_GENLIST_TREE_EFFECT_NONE))
     {
        if (!sd->tree_effect_animator)
          {
             _item_tree_effect_before(sd->expanded_item);
             evas_object_raise(sd->event_block_rect);
             evas_object_stack_below(sd->event_block_rect, sd->stack[1]);
             evas_object_show(sd->event_block_rect);
             sd->start_time = ecore_time_get();
             sd->tree_effect_animator =
               ecore_animator_add(_tree_effect_animator_cb, sd->obj);
          }
     }

   EINA_INLIST_FOREACH(sd->blocks, itb)
     {
        itb->w = sd->minw;
        if (ELM_RECTS_INTERSECT(itb->x - sd->pan_x + ox,
                                itb->y - sd->pan_y + oy,
                                itb->w, itb->h,
                                cvx, cvy, cvw, cvh))
          {
             if ((!itb->realized) || (itb->changed))
               _item_block_realize(itb);
             _item_block_position(itb, in);
          }
        else
          {
             if (itb->realized) _item_block_unrealize(itb);
          }
        in += itb->count;
     }
   if ((!sd->reorder_it) || (sd->reorder_pan_move))
     _group_items_recalc(sd);
   if ((sd->reorder_mode) && (sd->reorder_it))
     {
        if (sd->pan_y != sd->reorder_old_pan_y)
          sd->reorder_pan_move = EINA_TRUE;
        else sd->reorder_pan_move = EINA_FALSE;

        evas_object_raise(sd->VIEW(reorder_it));
        evas_object_stack_below(sd->VIEW(reorder_it), sd->stack[1]);
        sd->reorder_old_pan_y = sd->pan_y;
        sd->start_time = ecore_loop_time_get();
     }

   if (!sd->tree_effect_enabled ||
       (sd->move_effect_mode == ELM_GENLIST_TREE_EFFECT_NONE))
     _item_auto_scroll(sd);

   eo_do((sd)->obj,
         elm_interface_scrollable_content_pos_get(&vx, &vy),
         elm_interface_scrollable_content_viewport_geometry_get
         (NULL, NULL, &vw, &vh));

   if (sd->reorder_fast == 1)
      eo_do((sd)->obj, elm_interface_scrollable_content_region_show(vx, vy - 10, vw, vh));
   else if (sd->reorder_fast == -1)
      eo_do((sd)->obj, elm_interface_scrollable_content_region_show(vx, vy + 10, vw, vh));

   if (sd->focused_item && !sd->item_loop_enable)
     _elm_widget_focus_highlight_start(psd->wobj);

   evas_event_thaw(evas_object_evas_get(obj));
   evas_event_thaw_eval(evas_object_evas_get(obj));
}

EOLIAN static void
_elm_genlist_pan_eo_base_destructor(Eo *obj, Elm_Genlist_Pan_Data *psd)
{
   eo_data_unref(psd->wobj, psd->wsd);
   eo_do_super(obj, MY_PAN_CLASS, eo_destructor());
}

EOLIAN static void
_elm_genlist_pan_class_constructor(Eo_Class *klass)
{
      evas_smart_legacy_type_register(MY_PAN_CLASS_NAME_LEGACY, klass);
}

#include "elm_genlist_pan.eo.c"

static Eina_Bool
_item_multi_select_up(Elm_Genlist_Data *sd)
{
   Elm_Object_Item *eo_prev;

   if (!sd->selected) return EINA_FALSE;
   if (!sd->multi) return EINA_FALSE;

   eo_prev = elm_genlist_item_prev_get(sd->last_selected_item);
   if (!eo_prev) return EINA_TRUE;

   while (eo_prev)
     {
        ELM_GENLIST_ITEM_DATA_GET(eo_prev, prev);
        if ((prev->generation == sd->generation) &&
            (!_is_no_select(prev)) &&
            (!elm_object_item_disabled_get(eo_prev)))
          break;
        eo_prev = EO_OBJ(ELM_GEN_ITEM_FROM_INLIST(EINA_INLIST_GET(prev)->prev));
     }

   if (elm_genlist_item_selected_get(eo_prev))
     {
        elm_genlist_item_selected_set(sd->last_selected_item, EINA_FALSE);
        sd->last_selected_item = eo_prev;
     }
   else
     {
        elm_genlist_item_selected_set(eo_prev, EINA_TRUE);
     }
   return EINA_TRUE;
}

static Eina_Bool
_item_multi_select_down(Elm_Genlist_Data *sd)
{
   Elm_Object_Item *eo_next;

   if (!sd->selected) return EINA_FALSE;
   if (!sd->multi) return EINA_FALSE;

   eo_next = elm_genlist_item_next_get(sd->last_selected_item);
   while ((eo_next))
     {
        ELM_GENLIST_ITEM_DATA_GET(eo_next, next);
        if ((next->generation == sd->generation) &&
            (!_is_no_select(next)) &&
            (!elm_object_item_disabled_get(eo_next)))
          break;
        eo_next = EO_OBJ(ELM_GEN_ITEM_FROM_INLIST(EINA_INLIST_GET(next)->next));
     }
   if (!eo_next) return EINA_TRUE;

   if (elm_genlist_item_selected_get(eo_next))
     {
        elm_genlist_item_selected_set(sd->last_selected_item, EINA_FALSE);
        sd->last_selected_item = eo_next;
     }
   else
     {
        elm_genlist_item_selected_set(eo_next, EINA_TRUE);
     }

   return EINA_TRUE;
}

static Eina_Bool
_all_items_deselect(Elm_Genlist_Data *sd)
{
   if (!sd->selected) return EINA_FALSE;

   sd->deselecting = eina_list_clone(sd->selected);
   while (sd->deselecting)
     {
        Elm_Object_Item *it = eina_list_data_get(sd->deselecting);

        sd->deselecting = eina_list_remove_list(sd->deselecting, sd->deselecting);
        elm_genlist_item_selected_set(it, EINA_FALSE);
     }

   return EINA_TRUE;
}

static Eina_Bool
_item_single_select_up(Elm_Genlist_Data *sd)
{
   Elm_Gen_Item *prev = NULL;

   if (!sd->selected)
     prev = ELM_GEN_ITEM_FROM_INLIST(sd->items->last);
   else
     {
        Elm_Object_Item *eo_prev = elm_genlist_item_prev_get
           (sd->last_selected_item);
        prev = eo_data_scope_get(eo_prev, ELM_GENLIST_ITEM_CLASS);
     }

   while (prev)
     {
        if ((prev->generation == sd->generation) &&
            (!_is_no_select(prev)) &&
            (!elm_object_item_disabled_get(EO_OBJ(prev))))
          break;
        prev = ELM_GEN_ITEM_FROM_INLIST(EINA_INLIST_GET(prev)->prev);
     }

   if (!prev) return EINA_FALSE;

   _all_items_deselect(sd);

   elm_genlist_item_selected_set(EO_OBJ(prev), EINA_TRUE);
   return EINA_TRUE;
}

static Eina_Bool
_item_single_select_down(Elm_Genlist_Data *sd)
{
   Elm_Gen_Item *next = NULL;
   Elm_Object_Item *eo_next = NULL;

   if (!sd->selected)
     next = ELM_GEN_ITEM_FROM_INLIST(sd->items);
   else
     {
        eo_next = elm_genlist_item_next_get(sd->last_selected_item);
        next = eo_data_scope_get(eo_next, ELM_GENLIST_ITEM_CLASS);
     }

   while ((next))
     {
        if ((next->generation == sd->generation) &&
            (!_is_no_select(next)) &&
            (!elm_object_item_disabled_get(EO_OBJ(next))))
          break;
        next = ELM_GEN_ITEM_FROM_INLIST(EINA_INLIST_GET(next)->next);
     }

   if (!next) return EINA_FALSE;

   _all_items_deselect(sd);

   elm_genlist_item_selected_set(EO_OBJ(next), EINA_TRUE);

   return EINA_TRUE;
}

static void
_elm_genlist_item_focused(Elm_Object_Item *eo_it)
{
   ELM_GENLIST_ITEM_DATA_GET(eo_it, it);
   Evas_Object *obj = WIDGET(it);
   ELM_GENLIST_DATA_GET(obj, sd);
   const char *focus_raise;

   if (it->generation < sd->generation)
     return;

   if (_is_no_select(it) ||
       (eo_it == sd->focused_item) ||
       (eo_do(eo_it, elm_wdg_item_disabled_get())))
     return;

   switch (_elm_config->focus_autoscroll_mode)
     {
      case ELM_FOCUS_AUTOSCROLL_MODE_SHOW:
         elm_genlist_item_show(eo_it,
                               ELM_GENLIST_ITEM_SCROLLTO_IN);
         break;
      case ELM_FOCUS_AUTOSCROLL_MODE_BRING_IN:
         elm_genlist_item_bring_in(eo_it,
                                   ELM_GENLIST_ITEM_SCROLLTO_IN);
         break;
      default:
         break;
     }

   sd->focused_item = eo_it;

   if (elm_widget_focus_highlight_enabled_get(obj))
     edje_object_signal_emit(VIEW(it), SIGNAL_FOCUSED, "elm");

   focus_raise = edje_object_data_get(VIEW(it), "focusraise");
   if ((focus_raise) && (!strcmp(focus_raise, "on")))
     evas_object_raise(VIEW(it));
   evas_object_smart_callback_call(obj, SIG_ITEM_FOCUSED, eo_it);
}

static void
_elm_genlist_item_unfocused(Elm_Object_Item *eo_it)
{
   ELM_GENLIST_ITEM_DATA_GET(eo_it, it);
   Evas_Object *obj = WIDGET(it);
   ELM_GENLIST_DATA_GET(obj, sd);

   if (it->generation < sd->generation)
     return;

   if (_is_no_select(it))
     return;

   if ((!sd->focused_item) ||
       (eo_it != sd->focused_item))
     return;

   if (elm_widget_focus_highlight_enabled_get(obj))
     {
        ELM_GENLIST_ITEM_DATA_GET(sd->focused_item, focus_it);
        edje_object_signal_emit(VIEW(focus_it), SIGNAL_UNFOCUSED, "elm");
     }

   sd->focused_item = NULL;
   evas_object_smart_callback_call(obj, SIG_ITEM_UNFOCUSED, eo_it);
}

static Eina_Bool
_item_focused_next(Evas_Object *obj, Elm_Focus_Direction dir)
{
   ELM_GENLIST_DATA_GET(obj, sd);
   Elm_Gen_Item *next;
   Elm_Object_Item *eo_next;
   Elm_Object_Item *eo_first_item;
   Elm_Object_Item *eo_last_item;

   if (!sd->focused_item)
     {
        if (dir == ELM_FOCUS_UP)
          next = ELM_GEN_ITEM_FROM_INLIST(sd->items->last);
        else if (dir == ELM_FOCUS_DOWN)
          next = ELM_GEN_ITEM_FROM_INLIST(sd->items);
        else
          return EINA_FALSE;

        while ((next) &&
               (next->generation < sd->generation) &&
               (!eo_do(EO_OBJ(next), elm_wdg_item_disabled_get())))
          next = ELM_GEN_ITEM_FROM_INLIST(EINA_INLIST_GET(next)->next);
     }
   else
     {
        ELM_GENLIST_ITEM_DATA_GET(sd->focused_item, focus_it);
        if (dir == ELM_FOCUS_UP)
          {
             eo_first_item = elm_genlist_first_item_get(WIDGET(focus_it));
             if (eo_first_item == sd->focused_item) return EINA_FALSE;
             eo_next = elm_genlist_item_prev_get(sd->focused_item);
             next = eo_data_scope_get(eo_next, ELM_GENLIST_ITEM_CLASS);

             while (eo_do(eo_next, elm_wdg_item_disabled_get()))
               {
                  eo_next = elm_genlist_item_prev_get(eo_next);
                  next = eo_data_scope_get(eo_next, ELM_GENLIST_ITEM_CLASS);
               }
          }
        else if (dir == ELM_FOCUS_DOWN)
          {
             eo_last_item = elm_genlist_last_item_get(WIDGET(focus_it));
             if (eo_last_item == sd->focused_item) return EINA_FALSE;
             eo_next = elm_genlist_item_next_get(sd->focused_item);
             next = eo_data_scope_get(eo_next, ELM_GENLIST_ITEM_CLASS);

             while (eo_do(eo_next, elm_wdg_item_disabled_get()))
               {
                  eo_next = elm_genlist_item_next_get(eo_next);
                  next = eo_data_scope_get(eo_next, ELM_GENLIST_ITEM_CLASS);
               }
          }
        else
          return EINA_FALSE;

        if (!next) return EINA_FALSE;
     }

   elm_object_item_focus_set(EO_OBJ(next), EINA_TRUE);

   return EINA_TRUE;
}

static void
_elm_genlist_item_content_focus_set(Elm_Gen_Item *it, Elm_Focus_Direction dir)
{
   Evas_Object *focused_obj = NULL;
   Eina_List *l;
   if (!it) return;

   if (!GL_IT(it)->wsd->focus_on_selection_enabled) return;

   if (!it->item_focus_chain)
     {
        elm_object_focus_set(VIEW(it), EINA_TRUE);
        return;
     }

   EINA_LIST_FOREACH(it->item_focus_chain, l, focused_obj)
     if (elm_object_focus_get(focused_obj)) break;

   if (focused_obj && (dir != ELM_FOCUS_PREVIOUS))
     {
        Evas_Object *nextfocus;
        if (elm_widget_focus_next_get(focused_obj, dir, &nextfocus))
          {
             elm_object_focus_set(nextfocus, EINA_TRUE);
             return;
          }
     }

   if (!l) l = it->item_focus_chain;

   if (dir == ELM_FOCUS_RIGHT)
     {
        l = eina_list_next(l);
        if (!l) l = it->item_focus_chain;
     }
   else if (dir == ELM_FOCUS_LEFT)
     {
        l = eina_list_prev(l);
        if (!l) l = eina_list_last(it->item_focus_chain);
     }

   elm_object_focus_set(eina_list_data_get(l), EINA_TRUE);
}

static Eina_Bool
_key_action_move_dir(Evas_Object *obj, Elm_Focus_Direction dir, Eina_Bool multi)
{
   ELM_GENLIST_DATA_GET(obj, sd);
   Elm_Object_Item *it = NULL;
   Eina_Bool ret = EINA_FALSE;
   Evas_Coord v = 0;
   Evas_Coord min = 0;
   Eina_Bool focus_only = EINA_FALSE;

   // get content size and viewport size
   eo_do(obj,
         elm_interface_scrollable_content_viewport_geometry_get
         (NULL, NULL, NULL, &v),
         elm_interface_scrollable_content_size_get(NULL, &min));

   if (_elm_config->item_select_on_focus_disable)
     {
        ret = _item_focused_next(obj, dir);
     }
   else
     {
        if (multi)
          {
             if (dir == ELM_FOCUS_UP)
               ret = _item_multi_select_up(sd);
             else if (dir == ELM_FOCUS_DOWN)
               ret = _item_multi_select_down(sd);
          }
        else
          {
             if (dir == ELM_FOCUS_UP)
               ret = _item_single_select_up(sd);
             else if (dir == ELM_FOCUS_DOWN)
               ret = _item_single_select_down(sd);
          }
     }
   if (ret)
     return EINA_TRUE;

   focus_only = _elm_config->item_select_on_focus_disable && elm_widget_focus_highlight_enabled_get(obj);
   // handle item loop feature
   if (sd->item_loop_enable && !sd->item_looping_on)
     {
        if (min > v)
          {
             if (dir == ELM_FOCUS_UP)
               {
                  elm_layout_signal_emit(obj, "elm,action,looping,up", "elm");
                  sd->item_looping_on = EINA_TRUE;
               }
             else if (dir == ELM_FOCUS_DOWN)
               {
                  elm_layout_signal_emit(obj, "elm,action,looping,down", "elm");
                  sd->item_looping_on = EINA_TRUE;
               }
          }
        else
          {
             if (dir == ELM_FOCUS_UP)
               it = elm_genlist_last_item_get(obj);
            else if (dir == ELM_FOCUS_DOWN)
               it = elm_genlist_first_item_get(obj);

             if (it && focus_only)
               elm_object_item_focus_set(it, EINA_TRUE);
             else if (it)
               elm_genlist_item_selected_set(it, EINA_TRUE);
          }
        return EINA_TRUE;
     }
   else if (sd->item_looping_on)
     return EINA_TRUE;

   return EINA_FALSE;
}

static void
_anim_end(Elm_Genlist_Data *sd)
{
   if (sd->reorder.dir == ELM_FOCUS_UP)
     _item_move_after(sd->reorder.it2, sd->reorder.it1);
   else if (sd->reorder.dir == ELM_FOCUS_DOWN)
     _item_move_after(sd->reorder.it1, sd->reorder.it2);

   ecore_job_del(sd->calc_job);
   sd->calc_job = ecore_job_add(_calc_job, sd->obj);
}

static Eina_Bool
_item_swap_cb(void *data, double pos)
{
   Elm_Genlist_Data *sd = data;
   double frame = pos;
   Evas_Coord xx1, yy1, xx2, yy2;
   double dx, dy;

   switch (sd->reorder.tween_mode)
     {
        case ECORE_POS_MAP_LINEAR:
           frame = ecore_animator_pos_map(frame, sd->reorder.tween_mode, 0, 0);
           break;
        case ECORE_POS_MAP_ACCELERATE:
        case ECORE_POS_MAP_DECELERATE:
        case ECORE_POS_MAP_SINUSOIDAL:
           frame = ecore_animator_pos_map(frame, sd->reorder.tween_mode, 1.0, 0);
           break;
        case ECORE_POS_MAP_DIVISOR_INTERP:
        case ECORE_POS_MAP_BOUNCE:
        case ECORE_POS_MAP_SPRING:
           frame = ecore_animator_pos_map(frame, sd->reorder.tween_mode, 1.0, 1.0);
           break;
        default:
           // use ECORE_POS_MAP_LINEAR as default tween mode
           frame = ecore_animator_pos_map(frame, sd->reorder.tween_mode, 0, 0);
           break;
     }

   dx = sd->reorder.x2 - sd->reorder.x1;
   dy = sd->reorder.y2 - sd->reorder.y1;

   xx1 = sd->reorder.x1 + (dx * frame);
   yy1 = sd->reorder.y1 + (dy * frame);
   xx2 = sd->reorder.x2 - (dx * frame);
   yy2 = sd->reorder.y2 - (dy * frame);

   evas_object_move(VIEW(sd->reorder.it2), xx2, yy2);
   evas_object_move(VIEW(sd->reorder.it1), xx1, yy1);

   if (pos == 1.0)
     {
        elm_genlist_item_bring_in((Elm_Object_Item *)sd->reorder.it2, ELM_GENLIST_ITEM_SCROLLTO_IN);
        _anim_end(sd);
        evas_object_smart_callback_call(sd->obj, SIG_ITEM_REORDER_STOP,
                                        EO_OBJ(sd->reorder.it1));
        evas_object_smart_callback_call(sd->obj, SIG_MOVED, EO_OBJ(sd->reorder.it1));
        sd->reorder.running = EINA_FALSE;
     }

   _elm_widget_focus_highlight_start(sd->obj);

   return EINA_TRUE;
}


static void
_swap_items(Elm_Gen_Item *it1, Elm_Gen_Item *it2, Elm_Focus_Direction dir)
{
   if (!it1 || !it2) return;

   ELM_GENLIST_DATA_GET_FROM_ITEM(it1, sd);
   Evas_Coord xx1, yy1, xx2, yy2;

   sd->reorder.running = EINA_TRUE;
   sd->reorder.dir = dir;
   sd->reorder.it1 = it1;
   sd->reorder.it2 = it2;

   evas_object_geometry_get(VIEW(it1), &xx1, &yy1, NULL, NULL);
   evas_object_geometry_get(VIEW(it2), &xx2, &yy2, NULL, NULL);
   sd->reorder.x1 = xx1;
   sd->reorder.y1 = yy1;
   sd->reorder.x2 = xx2;
   sd->reorder.y2 = yy2;

   evas_object_raise(VIEW(it1));
   evas_object_smart_callback_call(sd->obj, SIG_ITEM_REORDER_START,
                                   EO_OBJ(sd->reorder.it1));
   //TODO: Add elm config for time
   ecore_animator_timeline_add(0.3, _item_swap_cb, sd);
}

static Eina_Bool
_key_action_move(Evas_Object *obj, const char *params)
{
   ELM_GENLIST_DATA_GET(obj, sd);
   const char *dir = params;

   Evas_Coord x = 0;
   Evas_Coord y = 0;
   Evas_Coord v_w = 0;
   Evas_Coord v_h = 0;
   Evas_Coord step_x = 0;
   Evas_Coord step_y = 0;
   Evas_Coord page_x = 0;
   Evas_Coord page_y = 0;
   Elm_Object_Item *it = NULL;
   Evas_Coord pan_max_x = 0, pan_max_y = 0;

   eo_do(obj,
         elm_interface_scrollable_content_pos_get(&x, &y),
         elm_interface_scrollable_step_size_get(&step_x, &step_y),
         elm_interface_scrollable_page_size_get(&page_x, &page_y),
         elm_interface_scrollable_content_viewport_geometry_get
         (NULL, NULL, &v_w, &v_h));

   if (!strcmp(dir, "left"))
     {
        x -= step_x;

        Elm_Object_Item *eo_gt = elm_genlist_selected_item_get(obj);
        ELM_GENLIST_ITEM_DATA_GET(eo_gt, gt);
        _elm_genlist_item_content_focus_set(gt, ELM_FOCUS_LEFT);

        return EINA_FALSE;
     }
   else if (!strcmp(dir, "right"))
     {
        x += step_x;

        Elm_Object_Item *eo_gt = elm_genlist_selected_item_get(obj);
        ELM_GENLIST_ITEM_DATA_GET(eo_gt, gt);
        _elm_genlist_item_content_focus_set(gt, ELM_FOCUS_RIGHT);

        return EINA_FALSE;
     }
   else if (!strcmp(dir, "up"))
     {
        if (sd->reorder.running) return EINA_TRUE;
        if (sd->reorder_mode)
          {
             Elm_Object_Item *up = elm_genlist_item_prev_get(sd->focused_item);
             ELM_GENLIST_ITEM_DATA_GET(sd->focused_item, focused_it);
             ELM_GENLIST_ITEM_DATA_GET(up, up2);

             _swap_items(focused_it, up2, ELM_FOCUS_UP);

             return EINA_TRUE;
          }

        if (_key_action_move_dir(obj, ELM_FOCUS_UP, EINA_FALSE)) return EINA_TRUE;
        else return EINA_FALSE;
     }
   else if (!strcmp(dir, "up_multi"))
     {
        if (_key_action_move_dir(obj, ELM_FOCUS_UP, EINA_TRUE)) return EINA_TRUE;
        else if (_key_action_move_dir(obj, ELM_FOCUS_UP, EINA_FALSE)) return EINA_TRUE;
        else return EINA_FALSE;
     }
   else if (!strcmp(dir, "down"))
     {
        if (sd->reorder.running) return EINA_TRUE;
        if (sd->reorder_mode)
          {
             Elm_Object_Item *down = elm_genlist_item_next_get(sd->focused_item);
             ELM_GENLIST_ITEM_DATA_GET(sd->focused_item, focused_it);
             ELM_GENLIST_ITEM_DATA_GET(down, down2);

             _swap_items(focused_it, down2, ELM_FOCUS_DOWN);

             return EINA_TRUE;
          }

        if (_key_action_move_dir(obj, ELM_FOCUS_DOWN, EINA_FALSE)) return EINA_TRUE;
        else return EINA_FALSE;
     }
   else if (!strcmp(dir, "down_multi"))
     {
        if (_key_action_move_dir(obj, ELM_FOCUS_DOWN, EINA_TRUE)) return EINA_TRUE;
        else if (_key_action_move_dir(obj, ELM_FOCUS_DOWN, EINA_FALSE)) return EINA_TRUE;
        else return EINA_FALSE;
     }
   else if (!strcmp(dir, "first"))
     {
        it = elm_genlist_first_item_get(obj);
        if (it)
          {
             elm_genlist_item_selected_set(it, EINA_TRUE);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(dir, "last"))
     {
        it = elm_genlist_last_item_get(obj);
        if (it)
          {
             elm_genlist_item_selected_set(it, EINA_TRUE);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(dir, "prior"))
     {
        if (page_y < 0)
          y -= -(page_y * v_h) / 100;
        else
          y -= page_y;
     }
   else if (!strcmp(dir, "next"))
     {
        if (page_y < 0)
          y += -(page_y * v_h) / 100;
        else
          y += page_y;
     }
   else return EINA_FALSE;

   eo_do(sd->pan_obj, elm_obj_pan_pos_max_get(&pan_max_x, &pan_max_y));
   if (x < 0) x = 0;
   if (x > pan_max_x) x = pan_max_x;
   if (y < 0) y = 0;
   if (y > pan_max_y) y = pan_max_y;

   eo_do(obj, elm_interface_scrollable_content_pos_set(x, y, EINA_TRUE));

   return EINA_TRUE;
}

static Eina_Bool
_key_action_select(Evas_Object *obj, const char *params EINA_UNUSED)
{
   Elm_Object_Item *eo_it = NULL;

   eo_it = elm_object_focused_item_get(obj);
   if (!eo_it) return EINA_TRUE;
   elm_genlist_item_expanded_set(eo_it, !elm_genlist_item_expanded_get(eo_it));
   ELM_GENLIST_ITEM_DATA_GET(eo_it, it);
   evas_object_smart_callback_call(WIDGET(it), SIG_ACTIVATED, EO_OBJ(it));

   return EINA_TRUE;
}

static Eina_Bool
_key_action_escape(Evas_Object *obj, const char *params EINA_UNUSED)
{
   ELM_GENLIST_DATA_GET(obj, sd);

   if (!_all_items_deselect(sd)) return EINA_FALSE;
   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_elm_genlist_elm_widget_event(Eo *obj, Elm_Genlist_Data *sd, Evas_Object *src, Evas_Callback_Type type, void *event_info)
{
   (void) src;
   Evas_Event_Key_Down *ev = event_info;

   if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return EINA_FALSE;
   if (!sd->items) return EINA_FALSE;

   if (!_elm_config_key_binding_call(obj, ev, key_actions))
     return EINA_FALSE;

   ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
   return EINA_TRUE;
}

/* This function disables the specific code of the layout sub object add.
 * Only the widget sub_object_add is called.
 */
EOLIAN static Eina_Bool
_elm_genlist_elm_layout_sub_object_add_enable(Eo *obj EINA_UNUSED, Elm_Genlist_Data *_pd EINA_UNUSED)
{
   return EINA_FALSE;
}

EOLIAN static Eina_Bool
_elm_genlist_elm_widget_sub_object_add(Eo *obj, Elm_Genlist_Data *_pd EINA_UNUSED, Evas_Object *sobj)
{
   Eina_Bool int_ret = EINA_FALSE;

   /* skipping layout's code, which registers size hint changing
    * callback on sub objects. this is here because items'
    * content_get() routines may change hints on the objects after
    * creation, thus issuing TOO MANY sizing_eval()'s here. they are
    * not needed at here anyway, so let's skip listening to those
    * hints changes */
   eo_do_super(obj, MY_CLASS, int_ret = elm_obj_widget_sub_object_add(sobj));
   if (!int_ret) return EINA_FALSE;

   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_elm_genlist_elm_widget_sub_object_del(Eo *obj, Elm_Genlist_Data *sd, Evas_Object *sobj)
{
   Eina_Bool int_ret = EINA_FALSE;


   /* XXX: hack -- also skipping sizing recalculation on
    * sub-object-del. genlist's crazy code paths (like groups and
    * such) seem to issue a whole lot of deletions and Evas bitches
    * about too many recalculations */
   sd->on_sub_del = EINA_TRUE;

   eo_do_super(obj, MY_CLASS, int_ret = elm_obj_widget_sub_object_del(sobj));
   if (!int_ret) return EINA_FALSE;

   sd->on_sub_del = EINA_FALSE;

   return EINA_TRUE;
}

/*
 * This function searches the nearest visible item based on the given item.
 * If the given item is in the genlist viewport, this returns the given item.
 * Or this searches the realized items and checks the nearest fully visible item
 * according to the given item's position.
 */
static Elm_Object_Item *
_elm_genlist_nearest_visible_item_get(Evas_Object *obj, Elm_Object_Item *eo_it)
{
   Evas_Coord vx = 0, vy = 0, vw = 0, vh = 0; // genlist viewport geometry
   Evas_Coord ix = 0, iy = 0, iw = 0, ih = 0; // given item geometry
   Evas_Coord cx = 0, cy = 0, cw = 0, ch = 0; // candidate item geometry
   Eina_List *item_list = NULL, *l = NULL;
   Elm_Object_Item *eo_item = NULL;
   ELM_GENLIST_DATA_GET(obj, sd);
   Eina_Bool search_next = EINA_FALSE;

   if (!eo_it) return NULL;
   ELM_GENLIST_ITEM_DATA_GET(eo_it, it);

   evas_object_geometry_get(sd->pan_obj, &vx, &vy, &vw, &vh);
   evas_object_geometry_get(VIEW(it), &ix, &iy, &iw, &ih); // FIXME: check if the item is realized or not

   if (ELM_RECTS_INCLUDE(vx, vy, vw, vh, ix, iy, iw, ih))
     {
        if (!elm_object_item_disabled_get(eo_it))
          return eo_it;
        else
          search_next = EINA_TRUE;
     }

   item_list = elm_genlist_realized_items_get(obj);

   if ((iy < vy) || search_next)
     {
        EINA_LIST_FOREACH(item_list, l, eo_item)
          {
             ELM_GENLIST_ITEM_DATA_GET(eo_item, item);
             evas_object_geometry_get(VIEW(item), &cx, &cy, &cw, &ch);
             if (ELM_RECTS_INCLUDE(vx, vy, vw, vh, cx, cy, cw, ch) &&
                 !elm_object_item_disabled_get(eo_item))
               {
                  eina_list_free(item_list);
                  return eo_item;
               }
          }
     }
   else
     {
        EINA_LIST_REVERSE_FOREACH(item_list, l, eo_item)
          {
             ELM_GENLIST_ITEM_DATA_GET(eo_item, item);
             evas_object_geometry_get(VIEW(item), &cx, &cy, &cw, &ch);
             if (ELM_RECTS_INCLUDE(vx, vy, vw, vh, cx, cy, cw, ch) &&
                 !elm_object_item_disabled_get(eo_item))
               {
                  eina_list_free(item_list);
                  return eo_item;
               }
          }
     }
   eina_list_free(item_list);

   return eo_it;
}

EOLIAN static Eina_Bool
_elm_genlist_elm_widget_on_focus(Eo *obj, Elm_Genlist_Data *sd)
{
   Eina_Bool int_ret = EINA_FALSE;
   Elm_Object_Item *eo_it = NULL;
   Eina_Bool is_sel = EINA_FALSE;

   eo_do_super(obj, MY_CLASS, int_ret = elm_obj_widget_on_focus());
   if (!int_ret) return EINA_FALSE;

   if (elm_widget_focus_get(obj) && (sd->items) && (sd->selected) &&
       (!sd->last_selected_item))
     {
        sd->last_selected_item = eina_list_data_get(sd->selected);
     }

   if (elm_widget_focus_get(obj) && !sd->mouse_down)
     {
        if (sd->last_focused_item)
          eo_it = sd->last_focused_item;
        else if (sd->last_selected_item)
          eo_it = sd->last_selected_item;
        else if (_elm_config->first_item_focus_on_first_focus_in &&
                 elm_widget_focus_highlight_enabled_get(obj))
          {
             eo_it = elm_genlist_first_item_get(obj);
             is_sel = EINA_TRUE;
          }

        if (eo_it)
          {
             eo_it = _elm_genlist_nearest_visible_item_get(obj, eo_it);
             if (eo_it)
               {
                  if (!_elm_config->item_select_on_focus_disable && is_sel)
                    elm_genlist_item_selected_set(eo_it, EINA_TRUE);
                  else
                    elm_object_item_focus_set(eo_it, EINA_TRUE);
                  _elm_widget_focus_highlight_start(obj);
               }
          }
     }
   else
     {
        if (sd->focused_item)
          {
             sd->last_focused_item = sd->focused_item;
             _elm_genlist_item_unfocused(sd->focused_item);
          }
     }

   return EINA_TRUE;
}

static Eina_Bool _elm_genlist_smart_focus_next_enable = EINA_FALSE;

EOLIAN static Eina_Bool
_elm_genlist_elm_widget_focus_next_manager_is(Eo *obj EINA_UNUSED, Elm_Genlist_Data *_pd EINA_UNUSED)
{
   return _elm_genlist_smart_focus_next_enable;
}

EOLIAN static Eina_Bool
_elm_genlist_elm_widget_focus_direction_manager_is(Eo *obj EINA_UNUSED, Elm_Genlist_Data *_pd EINA_UNUSED)
{
   return EINA_FALSE;
}

EOLIAN static Eina_Bool
_elm_genlist_elm_widget_focus_next(Eo *obj, Elm_Genlist_Data *sd, Elm_Focus_Direction dir, Evas_Object **next)
{
   Evas_Coord x, y, w, h;
   Evas_Coord sx, sy, sw, sh;
   Item_Block *itb;
   Eina_List *items = NULL;
   Eina_Bool done = EINA_FALSE;

   evas_object_geometry_get(sd->obj, &sx, &sy, &sw, &sh);

   EINA_INLIST_FOREACH(sd->blocks, itb)
     {
        if (itb->realized)
          {
             Eina_List *l;
             Elm_Gen_Item *it;

             done = EINA_TRUE;
             EINA_LIST_FOREACH(itb->items, l, it)
               {
                  if (it->realized)
                    {
                       evas_object_geometry_get(it->base->view, &x, &y, &w, &h);

                       /* check item which displays more than half of its size */
                       if (it->base->access_obj &&
                           ELM_RECTS_INTERSECT
                             (x + (w / 2), y + (h / 2), 0, 0, sx, sy, sw, sh))
                         items = eina_list_append(items, it->base->access_obj);

                       if (!it->base->access_order) continue;

                       Eina_List *subl;
                       Evas_Object *subo;
                       EINA_LIST_FOREACH(it->base->access_order, subl, subo)
                         items = eina_list_append(items, subo);
                    }
               }
          }
        else if (done) break;
     }

   return elm_widget_focus_list_next_get
            (obj, items, eina_list_data_get, dir, next);
}

static void
_mirrored_set(Evas_Object *obj,
              Eina_Bool rtl)
{
   ELM_GENLIST_DATA_GET(obj, sd);

   _item_cache_zero(sd);
   eo_do(obj, elm_interface_scrollable_mirrored_set(rtl));
}

EOLIAN static Eina_Bool
_elm_genlist_elm_widget_theme_apply(Eo *obj, Elm_Genlist_Data *sd)
{
   Item_Block *itb;
   Eina_Bool int_ret = EINA_FALSE;
   Eina_List *l;
   Elm_Gen_Item *it;

   eo_do_super(obj, MY_CLASS, int_ret = elm_obj_widget_theme_apply());
   if (!int_ret) return EINA_FALSE;

   evas_event_freeze(evas_object_evas_get(obj));
   _item_cache_zero(sd);
   _mirrored_set(obj, elm_widget_mirrored_get(obj));

   sd->item_width = sd->item_height = 0;
   sd->group_item_width = sd->group_item_height = 0;
   sd->minw = sd->minh = sd->realminw = 0;

   EINA_INLIST_FOREACH(sd->blocks, itb)
     {
        if (itb->realized) _item_block_unrealize(itb);
        EINA_LIST_FOREACH(itb->items, l, it)
          it->item->mincalcd = EINA_FALSE;

        itb->changed = EINA_TRUE;
     }
   ecore_job_del(sd->calc_job);
   sd->calc_job = ecore_job_add(_calc_job, obj);
   elm_layout_sizing_eval(obj);
   evas_event_thaw(evas_object_evas_get(obj));
   evas_event_thaw_eval(evas_object_evas_get(obj));

   return EINA_TRUE;
}

/* FIXME: take off later. maybe this show region coords belong in the
 * interface (new api functions, set/get)? */
static void
_show_region_hook(void *data EINA_UNUSED,
                  Evas_Object *obj)
{
   Evas_Coord x, y, w, h;

   ELM_GENLIST_DATA_GET_OR_RETURN(obj, sd);

   elm_widget_show_region_get(obj, &x, &y, &w, &h);
   //x & y are screen coordinates, Add with pan coordinates
   x += sd->pan_x;
   y += sd->pan_y;
   eo_do(obj, elm_interface_scrollable_content_region_show(x, y, w, h));
}

static void
_item_highlight(Elm_Gen_Item *it)
{
   const char *selectraise;
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   if (_is_no_select(it) ||
       (!sd->highlight) ||
       (it->generation < sd->generation) ||
       (it->highlighted) || eo_do(EO_OBJ(it), elm_wdg_item_disabled_get()) ||
       (it->item->deco_it_view))
     return;

   edje_object_signal_emit(VIEW(it), SIGNAL_SELECTED, "elm");
   if (it->deco_all_view)
     edje_object_signal_emit(it->deco_all_view, SIGNAL_SELECTED, "elm");
   evas_object_smart_callback_call(WIDGET(it), SIG_HIGHLIGHTED, EO_OBJ(it));

   selectraise = edje_object_data_get(VIEW(it), "selectraise");
   if ((selectraise) && (!strcmp(selectraise, "on")))
     {
        if (it->deco_all_view) evas_object_stack_below(it->deco_all_view, sd->stack[1]);
        else evas_object_stack_below(VIEW(it), sd->stack[1]);
        if ((it->item->group_item) && (it->item->group_item->realized))
          evas_object_stack_above(it->item->VIEW(group_item), sd->stack[1]);
     }
   it->highlighted = EINA_TRUE;
}

static void
_item_unhighlight(Elm_Gen_Item *it)
{
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   if ((it->generation < GL_IT(it)->wsd->generation) || (!it->highlighted))
     return;

   edje_object_signal_emit(VIEW(it), SIGNAL_UNSELECTED, "elm");
   evas_object_smart_callback_call(WIDGET(it), SIG_UNHIGHLIGHTED, EO_OBJ(it));
   if (it->deco_all_view)
     edje_object_signal_emit
       (it->deco_all_view, SIGNAL_UNSELECTED, "elm");

   if (!it->item->nostacking)
     {
        if ((it->item->order_num_in & 0x1) ^ it->item->stacking_even)
          {
             if (it->deco_all_view) evas_object_stack_below(it->deco_all_view, sd->stack[0]);
             else evas_object_stack_below(VIEW(it), sd->stack[0]);
          }
        else
          {
             if (it->deco_all_view) evas_object_stack_above(it->deco_all_view, sd->stack[0]);
             else evas_object_stack_above(VIEW(it), sd->stack[0]);
          }
     }
   it->highlighted = EINA_FALSE;
}

static void
_item_block_position_update(Eina_Inlist *list,
                            int idx)
{
   Item_Block *tmp;

   EINA_INLIST_FOREACH(list, tmp)
     {
        tmp->position = idx++;
        tmp->position_update = EINA_TRUE;
     }
}

static void
_item_position_update(Eina_List *list,
                      int idx)
{
   Elm_Gen_Item *it;
   Eina_List *l;

   EINA_LIST_FOREACH(list, l, it)
     {
        it->position = idx++;
        it->position_update = EINA_TRUE;
     }
}

static void
_item_block_merge(Item_Block *left,
                  Item_Block *right)
{
   Eina_List *l;
   Elm_Gen_Item *it2;

   EINA_LIST_FOREACH(right->items, l, it2)
     {
        it2->item->block = left;
        left->count++;
        left->changed = EINA_TRUE;
     }
   left->items = eina_list_merge(left->items, right->items);
}

static void
_item_block_del(Elm_Gen_Item *it)
{
   Eina_Inlist *il;
   Item_Block *itb = it->item->block;
   Eina_Bool block_changed = EINA_FALSE;
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   itb->items = eina_list_remove(itb->items, it);
   itb->count--;
   itb->changed = EINA_TRUE;
   ecore_job_del(sd->calc_job);
   sd->calc_job = ecore_job_add(_calc_job, sd->obj);
   if (itb->count < 1)
     {
        Item_Block *itbn;

        il = EINA_INLIST_GET(itb);
        itbn = (Item_Block *)(il->next);
        if (it->parent)
          it->parent->item->items =
            eina_list_remove(it->parent->item->items, EO_OBJ(it));
        else
          {
             _item_block_position_update(il->next, itb->position);
             sd->blocks = eina_inlist_remove(sd->blocks, il);
          }
        free(itb);
        if (itbn) itbn->changed = EINA_TRUE;
     }
   else
     {
        if (itb->count < (sd->max_items_per_block / 2))
          {
             Item_Block *itbp;
             Item_Block *itbn;

             il = EINA_INLIST_GET(itb);
             itbp = (Item_Block *)(il->prev);
             itbn = (Item_Block *)(il->next);

             /* merge block with previous */
             if ((itbp) &&
                 ((itbp->count + itb->count) <
                  (sd->max_items_per_block +
                   (sd->max_items_per_block / 2))))
               {
                  _item_block_merge(itbp, itb);
                  _item_block_position_update
                    (EINA_INLIST_GET(itb)->next, itb->position);
                  sd->blocks = eina_inlist_remove
                      (sd->blocks, EINA_INLIST_GET(itb));
                  free(itb);
                  block_changed = EINA_TRUE;
               }
             /* merge block with next */
             else if ((itbn) &&
                      ((itbn->count + itb->count) <
                       (sd->max_items_per_block +
                        (sd->max_items_per_block / 2))))
               {
                  _item_block_merge(itb, itbn);
                  _item_block_position_update
                    (EINA_INLIST_GET(itbn)->next, itbn->position);
                  sd->blocks =
                    eina_inlist_remove(sd->blocks, EINA_INLIST_GET(itbn));
                  free(itbn);
                  block_changed = EINA_TRUE;
               }
          }
     }

   if (block_changed)
     {
        sd->pan_changed = EINA_TRUE;
        evas_object_smart_changed(sd->pan_obj);
        ecore_job_del(sd->calc_job);
        sd->calc_job = NULL;
     }
}

static void
_decorate_all_item_unrealize(Elm_Gen_Item *it)
{
   if ((!it) || (!it->item->decorate_all_item_realized)) return;

   edje_object_part_unswallow(it->deco_all_view, VIEW(it));
   evas_object_smart_member_add(VIEW(it), GL_IT(it)->wsd->pan_obj);
   elm_widget_sub_object_add(WIDGET(it), VIEW(it));
   _elm_genlist_item_position_state_update(it);
   _elm_genlist_item_state_update(it, NULL);

   if (it->item->wsd->reorder_mode)
     {
        edje_object_signal_emit(VIEW(it), SIGNAL_REORDER_MODE_SET, "elm");
        edje_object_signal_emit(it->deco_all_view, SIGNAL_REORDER_MODE_UNSET,
                                "elm");
     }

   _view_clear(it->deco_all_view, &(GL_IT(it)->deco_all_texts),
               &(GL_IT(it)->deco_all_contents), &(GL_IT(it)->deco_all_states),
               &(GL_IT(it)->deco_all_content_objs));

   edje_object_signal_emit(VIEW(it), SIGNAL_DECORATE_DISABLED, "elm");

   edje_object_message_signal_process(it->deco_all_view);
   _item_mouse_callbacks_del(it, it->deco_all_view);
   _item_mouse_callbacks_add(it, VIEW(it));

   ELM_SAFE_FREE(it->deco_all_view, evas_object_del);

   it->item->decorate_all_item_realized = EINA_FALSE;
}

static void
_elm_genlist_item_del_not_serious(Elm_Gen_Item *it)
{
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);
   Elm_Object_Item *eo_it = EO_OBJ(it);

   eo_do(eo_it, elm_wdg_item_pre_notify_del());
   it->generation = sd->generation - 1; /* This means that
                                                     * the item is
                                                     * deleted */

   if (it->walking > 0) return;

   if (it->selected)
     {
        sd->selected = eina_list_remove(sd->selected, EO_OBJ(it));
        if (sd->deselecting)
          sd->deselecting = eina_list_remove(sd->deselecting, it);
     }
   if (sd->last_focused_item == eo_it)
     sd->last_focused_item = NULL;
   if (sd->focused_item == eo_it)
     sd->focused_item = NULL;
   if (sd->last_selected_item == eo_it)
     sd->last_selected_item = NULL;

   if (it->itc->func.del)
     it->itc->func.del((void *)WIDGET_ITEM_DATA_GET(EO_OBJ(it)), WIDGET(it));
}

static void
_elm_genlist_item_del_serious(Elm_Gen_Item *it)
{
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   _elm_genlist_item_del_not_serious(it);

   sd->items = eina_inlist_remove(sd->items, EINA_INLIST_GET(it));
   if (it->tooltip.del_cb)
     it->tooltip.del_cb((void *)it->tooltip.data, WIDGET(it), it);
   sd->walking -= it->walking;
   ELM_SAFE_FREE(it->long_timer, ecore_timer_del);
   if (it->group)
     sd->group_items = eina_list_remove(sd->group_items, it);

   ELM_SAFE_FREE(sd->state, eina_inlist_sorted_state_free);
   ecore_job_del(sd->calc_job);
   sd->calc_job = ecore_job_add(_calc_job, sd->obj);

   sd->item_count--;

   ELM_SAFE_FREE(it->item, free);
}

static void
_item_del(Elm_Gen_Item *it)
{
   Evas_Object *obj = WIDGET(it);
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   evas_event_freeze(evas_object_evas_get(obj));

   // FIXME: relative will be better to be fixed. it is too harsh.
   if (it->item->rel && it->item->rel->item)
      it->item->rel->item->rel_revs =
         eina_list_remove(it->item->rel->item->rel_revs, it);
   if (it->item->rel_revs)
     {
        Elm_Gen_Item *tmp;
        EINA_LIST_FREE(it->item->rel_revs, tmp) tmp->item->rel = NULL;
     }
   elm_genlist_item_subitems_clear(EO_OBJ(it));
   if (sd->show_item == it) sd->show_item = NULL;
   if (it->realized) _elm_genlist_item_unrealize(it, EINA_FALSE);
   if (it->item->decorate_all_item_realized) _decorate_all_item_unrealize(it);
   if (it->item->block) _item_block_del(it);
   if (it->item->queued)
     sd->queue = eina_list_remove(sd->queue, it);
   if (sd->anchor_item == it)
     {
        sd->anchor_item = ELM_GEN_ITEM_FROM_INLIST(EINA_INLIST_GET(it)->next);
        if (!sd->anchor_item)
          sd->anchor_item =
            ELM_GEN_ITEM_FROM_INLIST(EINA_INLIST_GET(it)->prev);
     }
   if (sd->expanded_item == it)
     {
        if (sd->tree_effect_animator)
          {
             _item_tree_effect_finish(sd);
             ELM_SAFE_FREE(sd->tree_effect_animator, ecore_animator_del);
          }
        sd->expanded_item = NULL;
        sd->move_effect_mode = ELM_GENLIST_TREE_EFFECT_NONE;
     }
   if (sd->expanded_next_item == it) sd->expanded_next_item = NULL;
   if (sd->move_items) sd->move_items = eina_list_remove(sd->move_items, it);
   if (it->parent)
     it->parent->item->items = eina_list_remove(it->parent->item->items, EO_OBJ(it));
   ELM_SAFE_FREE(it->item->swipe_timer, ecore_timer_del);
   _elm_genlist_item_del_serious(it);
   elm_genlist_item_class_unref((Elm_Genlist_Item_Class *)it->itc);
   evas_event_thaw(evas_object_evas_get(obj));
   evas_event_thaw_eval(evas_object_evas_get(obj));
   if (!sd->queue) _item_scroll(sd);
}

static void
_item_unselect(Elm_Gen_Item *it)
{
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   if ((it->generation < sd->generation)) return;
   it->unhighlight_cb(it); /* unhighlight the item first */
   if (!it->selected) return; /* then check whether the item is selected */

  if (GL_IT(it)->wsd->focus_on_selection_enabled)
     {
        Evas_Object* eobj;
        Eina_List* l;
        EINA_LIST_FOREACH(it->item_focus_chain, l, eobj)
          elm_object_focus_set(eobj, EINA_FALSE);
     }

   it->selected = EINA_FALSE;
   sd->selected = eina_list_remove(sd->selected, EO_OBJ(it));
   evas_object_smart_callback_call(WIDGET(it), SIG_UNSELECTED, EO_OBJ(it));
}

static void
_item_mouse_in_cb(void *data,
                  Evas *evas EINA_UNUSED,
                  Evas_Object *obj EINA_UNUSED,
                  void *event_info EINA_UNUSED)
{
   Elm_Gen_Item *it = data;
   if (!elm_object_item_disabled_get(EO_OBJ(it)) &&
       (_elm_config->focus_move_policy == ELM_FOCUS_MOVE_POLICY_IN))
     elm_object_item_focus_set(EO_OBJ(it), EINA_TRUE);
}

static void
_item_mouse_move_cb(void *data,
                    Evas *evas EINA_UNUSED,
                    Evas_Object *obj,
                    void *event_info)
{
   Elm_Gen_Item *it = data;
   Evas_Event_Mouse_Move *ev = event_info;
   Evas_Coord ox, oy, ow, oh, it_scrl_y, y_pos;
   Evas_Coord minw = 0, minh = 0, x, y, w, h, dx, dy, adx, ady;
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);
   Elm_Object_Item *eo_it = EO_OBJ(it);

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
     {
        if (!sd->on_hold)
          {
             sd->on_hold = EINA_TRUE;
             if ((!sd->wasselected) && (!it->flipped))
               it->unsel_cb(it);
          }
     }
   if (sd->multi_touched)
     {
        sd->cur_x = ev->cur.canvas.x;
        sd->cur_y = ev->cur.canvas.y;
        return;
     }
   if ((it->dragging) && (it->down))
     {
        if (sd->movements == SWIPE_MOVES)
          sd->swipe = EINA_TRUE;
        else
          {
             sd->history[sd->movements].x = ev->cur.canvas.x;
             sd->history[sd->movements].y = ev->cur.canvas.y;
             if (abs((sd->history[sd->movements].x -
                      sd->history[0].x)) > 40)
               sd->swipe = EINA_TRUE;
             else
               sd->movements++;
          }
        ELM_SAFE_FREE(it->long_timer, ecore_timer_del);
        evas_object_smart_callback_call(WIDGET(it), SIG_DRAG, eo_it);
        return;
     }
   if ((!it->down) || (sd->longpressed))
     {
        ELM_SAFE_FREE(it->long_timer, ecore_timer_del);
        if ((sd->reorder_mode) && (sd->reorder_it))
          {
             evas_object_geometry_get(sd->pan_obj, &ox, &oy, &ow, &oh);

             if (ev->cur.canvas.y < (oy + (sd->reorder_it->item->h / 2)))
                sd->reorder_fast = 1;
             else if (ev->cur.canvas.y > (oy + oh - (sd->reorder_it->item->h  / 2)))
                sd->reorder_fast = -1;
             else sd->reorder_fast = 0;

             it_scrl_y = ev->cur.canvas.y - sd->reorder_it->dy;

             if (!sd->reorder_start_y)
               sd->reorder_start_y = it->item->block->y + it->y;

             if (it_scrl_y < oy)
               y_pos = oy;
             else if (it_scrl_y + sd->reorder_it->item->h > oy + oh)
               y_pos = oy + oh - sd->reorder_it->item->h;
             else
               y_pos = it_scrl_y;

             if (it->deco_all_view)
               _item_position(it, it->deco_all_view, it->item->scrl_x, y_pos);
             else
               _item_position(it, VIEW(it), it->item->scrl_x, y_pos);

             ecore_job_del(sd->calc_job);
             sd->calc_job = ecore_job_add(_calc_job, sd->obj);
          }
        return;
     }
   if (it->select_mode != ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY)
     elm_coords_finger_size_adjust(1, &minw, 1, &minh);

   evas_object_geometry_get(obj, &x, &y, &w, &h);
   x = ev->cur.canvas.x - x;
   y = ev->cur.canvas.y - y;
   dx = x - it->dx;
   adx = dx;
   if (adx < 0) adx = -dx;

   dy = y - it->dy;
   ady = dy;
   if (ady < 0) ady = -dy;

   minw /= 2;
   minh /= 2;

   // gah! annoying drag detection - leave this alone
   if (h < w)
     {
        if (minw < h) minw = h;
        if (minh < h) minh = h;
     }
   else
     {
        if (minw < w) minw = w;
        if (minh < w) minh = w;
     }
   if (minw < 5) minw = 5;
   if (minh < 5) minh = 5;

   if ((adx > minw) || (ady > minh))
     {
        it->dragging = EINA_TRUE;
        ELM_SAFE_FREE(it->long_timer, ecore_timer_del);
        if (!sd->wasselected)
          it->unsel_cb(it);
        if (dy < 0)
          {
             if (ady > adx)
               evas_object_smart_callback_call
                 (WIDGET(it), SIG_DRAG_START_UP, eo_it);
             else
               {
                  if (dx < 0)
                    evas_object_smart_callback_call
                      (WIDGET(it), SIG_DRAG_START_LEFT, eo_it);
                  else
                    evas_object_smart_callback_call
                      (WIDGET(it), SIG_DRAG_START_RIGHT, eo_it);
               }
          }
        else
          {
             if (ady > adx)
               evas_object_smart_callback_call
                 (WIDGET(it), SIG_DRAG_START_DOWN, eo_it);
             else
               {
                  if (dx < 0)
                    evas_object_smart_callback_call
                      (WIDGET(it), SIG_DRAG_START_LEFT, eo_it);
                  else
                    evas_object_smart_callback_call
                      (WIDGET(it), SIG_DRAG_START_RIGHT, eo_it);
               }
          }
     }
}

static Eina_Bool
_long_press_cb(void *data)
{
   Elm_Gen_Item *it = data;
   Elm_Object_Item *eo_it_tmp;
   Eina_List *list;
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   it->long_timer = NULL;
   if (_is_no_select(it) ||
       eo_do(EO_OBJ(it), elm_wdg_item_disabled_get()) || (it->dragging))
     return ECORE_CALLBACK_CANCEL;

   sd->longpressed = EINA_TRUE;
   evas_object_smart_callback_call(WIDGET(it), SIG_LONGPRESSED, EO_OBJ(it));
   if ((sd->reorder_mode) && (!it->group))
     {
        sd->reorder_it = it;
        sd->reorder_start_y = 0;
        if (it->deco_all_view)
          evas_object_stack_below(it->deco_all_view, sd->stack[1]);
        else
          evas_object_stack_below(VIEW(it), sd->stack[1]);

        eo_do(sd->obj, elm_interface_scrollable_hold_set(EINA_TRUE));
        eo_do(sd->obj, elm_interface_scrollable_bounce_allow_set
              (EINA_FALSE, EINA_FALSE));

        list = elm_genlist_realized_items_get
            ((sd)->obj);
        EINA_LIST_FREE(list, eo_it_tmp)
          {
             ELM_GENLIST_ITEM_DATA_GET(eo_it_tmp, it_tmp);
             if (it != it_tmp) it->unsel_cb(it_tmp);
          }

        if (elm_genlist_item_expanded_get(EO_OBJ(it)))
          {
             elm_genlist_item_expanded_set(EO_OBJ(it), EINA_FALSE);
             return ECORE_CALLBACK_RENEW;
          }

        if (!sd->decorate_all_mode)
          edje_object_signal_emit(VIEW(it), SIGNAL_REORDER_ENABLED, "elm");
     }

   return ECORE_CALLBACK_CANCEL;
}

static void
_swipe_do(Elm_Gen_Item *it)
{
   int i, sum = 0;
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   if (_is_no_select(it) ||
       eo_do(EO_OBJ(it), elm_wdg_item_disabled_get())) return;

   sd->swipe = EINA_FALSE;
   for (i = 0; i < sd->movements; i++)
     {
        sum += sd->history[i].x;
        if (abs(sd->history[0].y - sd->history[i].y) > 10)
          return;
     }

   sum /= sd->movements;
   if (abs(sum - sd->history[0].x) <= 10) return;
   evas_object_smart_callback_call(WIDGET(it), SIG_SWIPE, EO_OBJ(it));
}

static Eina_Bool
_swipe_cancel(void *data)
{
   Elm_Gen_Item *it = data;
   if (!it) return ECORE_CALLBACK_CANCEL;
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   sd->swipe = EINA_FALSE;
   sd->movements = 0;

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_multi_cancel(void *data)
{
   ELM_GENLIST_DATA_GET(data, sd);

   if (!sd) return ECORE_CALLBACK_CANCEL;
   sd->multi_timeout = EINA_TRUE;

   return ECORE_CALLBACK_RENEW;
}

static void
_multi_touch_gesture_eval(Elm_Gen_Item *it)
{
   Evas_Coord minw = 0, minh = 0;
   Evas_Coord off_x, off_y, off_mx, off_my;
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);
   Elm_Object_Item *eo_it = EO_OBJ(it);

   sd->multi_touched = EINA_FALSE;
   ELM_SAFE_FREE(sd->multi_timer, ecore_timer_del);
   if (sd->multi_timeout)
     {
        sd->multi_timeout = EINA_FALSE;
        return;
     }

   elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   off_x = abs(sd->cur_x - sd->prev_x);
   off_y = abs(sd->cur_y - sd->prev_y);
   off_mx = abs(sd->cur_mx - sd->prev_mx);
   off_my = abs(sd->cur_my - sd->prev_my);

   if (((off_x > minw) || (off_y > minh)) && ((off_mx > minw)
                                              || (off_my > minh)))
     {
        if ((off_x + off_mx) > (off_y + off_my))
          {
             if ((sd->cur_x > sd->prev_x) && (sd->cur_mx > sd->prev_mx))
               evas_object_smart_callback_call
                 (WIDGET(it), SIG_MULTI_SWIPE_RIGHT, eo_it);
             else if ((sd->cur_x < sd->prev_x) && (sd->cur_mx < sd->prev_mx))
               evas_object_smart_callback_call
                 (WIDGET(it), SIG_MULTI_SWIPE_LEFT, eo_it);
             else if (abs(sd->cur_x - sd->cur_mx) >
                      abs(sd->prev_x - sd->prev_mx))
               evas_object_smart_callback_call
                 (WIDGET(it), SIG_MULTI_PINCH_OUT, eo_it);
             else
               evas_object_smart_callback_call
                 (WIDGET(it), SIG_MULTI_PINCH_IN, eo_it);
          }
        else
          {
             if ((sd->cur_y > sd->prev_y) && (sd->cur_my > sd->prev_my))
               evas_object_smart_callback_call
                 (WIDGET(it), SIG_MULTI_SWIPE_DOWN, eo_it);
             else if ((sd->cur_y < sd->prev_y) && (sd->cur_my < sd->prev_my))
               evas_object_smart_callback_call
                 (WIDGET(it), SIG_MULTI_SWIPE_UP, eo_it);
             else if (abs(sd->cur_y - sd->cur_my) >
                      abs(sd->prev_y - sd->prev_my))
               evas_object_smart_callback_call
                 (WIDGET(it), SIG_MULTI_PINCH_OUT, eo_it);
             else
               evas_object_smart_callback_call
                 (WIDGET(it), SIG_MULTI_PINCH_IN, eo_it);
          }
     }

   sd->multi_timeout = EINA_FALSE;
}

static void
_item_multi_down_cb(void *data,
                    Evas *evas EINA_UNUSED,
                    Evas_Object *obj EINA_UNUSED,
                    void *event_info)
{
   Elm_Gen_Item *it = data;
   Evas_Event_Multi_Down *ev = event_info;
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   if ((sd->multi_device != 0) || (sd->multi_touched)
       || (sd->multi_timeout))
     return;

   sd->multi_device = ev->device;
   sd->multi_down = EINA_TRUE;
   sd->multi_touched = EINA_TRUE;
   sd->prev_mx = ev->canvas.x;
   sd->prev_my = ev->canvas.y;
   if (!sd->wasselected)
     it->unsel_cb(it);
   sd->wasselected = EINA_FALSE;
   sd->longpressed = EINA_FALSE;
   ELM_SAFE_FREE(it->long_timer, ecore_timer_del);
   if (it->dragging)
     {
        it->dragging = EINA_FALSE;
        evas_object_smart_callback_call(WIDGET(it), SIG_DRAG_STOP, EO_OBJ(it));
     }
   ELM_SAFE_FREE(it->item->swipe_timer, ecore_timer_del);
   if (sd->on_hold)
     {
        sd->swipe = EINA_FALSE;
        sd->movements = 0;
        sd->on_hold = EINA_FALSE;
     }
}

static void
_item_multi_up_cb(void *data,
                  Evas *evas EINA_UNUSED,
                  Evas_Object *obj EINA_UNUSED,
                  void *event_info)
{
   Elm_Gen_Item *it = data;
   Evas_Event_Multi_Up *ev = event_info;
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   if (sd->multi_device != ev->device) return;
   sd->multi_device = 0;
   sd->multi_down = EINA_FALSE;
   if (sd->mouse_down) return;
   _multi_touch_gesture_eval(data);
}

static void
_item_multi_move_cb(void *data,
                    Evas *evas EINA_UNUSED,
                    Evas_Object *obj EINA_UNUSED,
                    void *event_info)
{
   Elm_Gen_Item *it = data;
   Evas_Event_Multi_Move *ev = event_info;
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   if (sd->multi_device != ev->device) return;
   sd->cur_mx = ev->cur.canvas.x;
   sd->cur_my = ev->cur.canvas.y;
}

static void
_item_mouse_down_cb(void *data,
                    Evas *evas EINA_UNUSED,
                    Evas_Object *obj,
                    void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;
   Elm_Gen_Item *it = data;
   Evas_Coord x, y;
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);
   Elm_Object_Item *eo_it = EO_OBJ(it);

   if (ev->button == 3)
     {
        evas_object_geometry_get(obj, &x, &y, NULL, NULL);
        it->dx = ev->canvas.x - x;
        it->dy = ev->canvas.y - y;
        return;
     }
   if (ev->button != 1) return;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
     {
        sd->on_hold = EINA_TRUE;
     }

   it->down = EINA_TRUE;
   it->dragging = EINA_FALSE;
   evas_object_geometry_get(obj, &x, &y, NULL, NULL);
   it->dx = ev->canvas.x - x;
   it->dy = ev->canvas.y - y;
   sd->mouse_down = EINA_TRUE;
   if (!sd->multi_touched)
     {
        sd->prev_x = ev->canvas.x;
        sd->prev_y = ev->canvas.y;
        sd->multi_timeout = EINA_FALSE;
        ecore_timer_del(sd->multi_timer);
        sd->multi_timer = ecore_timer_add(MULTI_DOWN_TIME, _multi_cancel, sd->obj);
     }
   sd->longpressed = EINA_FALSE;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) sd->on_hold = EINA_TRUE;
   else sd->on_hold = EINA_FALSE;
   if (sd->on_hold) return;
   sd->wasselected = it->selected;
   
   ecore_timer_del(it->item->swipe_timer);
   it->item->swipe_timer = ecore_timer_add(SWIPE_TIME, _swipe_cancel, it);
   ELM_SAFE_FREE(it->long_timer, ecore_timer_del);
   if (it->realized)
     it->long_timer = ecore_timer_add
         (sd->longpress_timeout, _long_press_cb, it);
   else
     it->long_timer = NULL;
   sd->swipe = EINA_FALSE;
   sd->movements = 0;

   if (_is_no_select(it) ||
        eo_do((Eo *)eo_it, elm_wdg_item_disabled_get()))
     return;

   // and finally call the user callbacks.
   // NOTE: keep this code at the bottom, as the user can change the
   //       list at this point (clear, delete, etc...)
   it->highlight_cb(it);
   if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK)
     {
        evas_object_smart_callback_call(WIDGET(it), SIG_CLICKED_DOUBLE, eo_it);
        evas_object_smart_callback_call(WIDGET(it), SIG_ACTIVATED, eo_it);
     }
   evas_object_smart_callback_call(WIDGET(it), SIG_PRESSED, eo_it);
}

static Item_Block *
_item_block_new(Elm_Genlist_Data *sd,
                Eina_Bool prepend)
{
   Item_Block *itb;

   itb = calloc(1, sizeof(Item_Block));
   if (!itb) return NULL;
   itb->sd = sd;
   if (prepend)
     {
        sd->blocks = eina_inlist_prepend(sd->blocks, EINA_INLIST_GET(itb));
        _item_block_position_update(sd->blocks, 0);
     }
   else
     {
        sd->blocks = eina_inlist_append(sd->blocks, EINA_INLIST_GET(itb));
        itb->position_update = EINA_TRUE;
        if (sd->blocks != EINA_INLIST_GET(itb))
          {
             itb->position =
               ((Item_Block *)(EINA_INLIST_GET(itb)->prev))->position + 1;
          }
        else
          {
             itb->position = 0;
          }
     }

   return itb;
}

/**
 * @internal
 *
 * This function adds an item to a block's item list. This may or may not
 * rearrange existing blocks and create a new block.
 *
 */
static Eina_Bool
_item_block_add(Elm_Genlist_Data *sd,
                Elm_Gen_Item *it)
{
   Item_Block *itb = NULL;

   // when a new item does not depend on another item
   if (!it->item->rel)
     {
newblock:
        if (it->item->rel)
          {
             itb = calloc(1, sizeof(Item_Block));
             if (!itb) return EINA_FALSE;
             itb->sd = sd;
             if (!it->item->rel->item->block)
               {
                  sd->blocks =
                    eina_inlist_append(sd->blocks, EINA_INLIST_GET(itb));
                  itb->items = eina_list_append(itb->items, it);
                  itb->position_update = EINA_TRUE;
                  it->position = eina_list_count(itb->items);
                  it->position_update = EINA_TRUE;

                  if (sd->blocks != EINA_INLIST_GET(itb))
                    {
                       itb->position =
                         ((Item_Block *)
                          (EINA_INLIST_GET(itb)->prev))->position + 1;
                    }
                  else
                    {
                       itb->position = 0;
                    }
               }
             else
               {
                  Eina_List *tmp;

                  tmp = eina_list_data_find_list(itb->items, it->item->rel);
                  if (it->item->before)
                    {
                       sd->blocks = eina_inlist_prepend_relative
                           (sd->blocks, EINA_INLIST_GET(itb),
                           EINA_INLIST_GET(it->item->rel->item->block));
                       itb->items =
                         eina_list_prepend_relative_list(itb->items, it, tmp);

                       /* Update index from where we prepended */
                       _item_position_update
                         (eina_list_prev(tmp), it->item->rel->position);
                       _item_block_position_update
                         (EINA_INLIST_GET(itb),
                         it->item->rel->item->block->position);
                    }
                  else
                    {
                       sd->blocks = eina_inlist_append_relative
                           (sd->blocks, EINA_INLIST_GET(itb),
                           EINA_INLIST_GET(it->item->rel->item->block));
                       itb->items =
                         eina_list_append_relative_list(itb->items, it, tmp);

                       /* Update block index from where we appended */
                       _item_position_update
                         (eina_list_next(tmp), it->item->rel->position + 1);
                       _item_block_position_update
                         (EINA_INLIST_GET(itb),
                         it->item->rel->item->block->position + 1);
                    }
               }
          }
        else
          {
             // item move_before, prepend, insert_before, sorted_insert with before
             if (it->item->before)
               {
                  if (sd->blocks)
                    {
                       itb = (Item_Block *)(sd->blocks);
                       if (itb->count >= sd->max_items_per_block)
                         {
                            itb = _item_block_new(sd, EINA_TRUE);
                            if (!itb) return EINA_FALSE;
                         }
                    }
                  else
                    {
                       itb = _item_block_new(sd, EINA_TRUE);
                       if (!itb) return EINA_FALSE;
                    }
                  itb->items = eina_list_prepend(itb->items, it);

                  _item_position_update(itb->items, 0);
               }
             // item move_after, append, insert_after, sorted_insert without before
             else
               {
                  if (sd->blocks)
                    {
                       itb = (Item_Block *)(sd->blocks->last);
                       if (itb->count >= sd->max_items_per_block)
                         {
                            itb = _item_block_new(sd, EINA_FALSE);
                            if (!itb) return EINA_FALSE;
                         }
                    }
                  else
                    {
                       itb = _item_block_new(sd, EINA_FALSE);
                       if (!itb) return EINA_FALSE;
                    }
                  itb->items = eina_list_append(itb->items, it);
                  it->position = eina_list_count(itb->items);
               }
          }
     }
   // when a new item depends on another item
   else
     {
        Eina_List *tmp;

        if (it->item->rel->item->queued)
          {
             /* NOTE: for a strange reason eina_list and eina_inlist
                don't have the same property on sorted insertion
                order, so the queue is not always ordered like the
                item list.  This lead to issue where we depend on an
                item that is not yet created. As a quick work around,
                we reschedule the calc of the item and stop reordering
                the list to prevent any nasty issue to show up here.
              */
             sd->queue = eina_list_append(sd->queue, it);
             sd->requeued = EINA_TRUE;
             it->item->queued = EINA_TRUE;

             return EINA_FALSE;
          }
        itb = it->item->rel->item->block;
        if (!itb) goto newblock;
        tmp = eina_list_data_find_list(itb->items, it->item->rel);
        if (it->item->before)
          {
             itb->items = eina_list_prepend_relative_list(itb->items, it, tmp);
             _item_position_update
               (eina_list_prev(tmp), it->item->rel->position);
          }
        else
          {
             itb->items = eina_list_append_relative_list(itb->items, it, tmp);
             _item_position_update
               (eina_list_next(tmp), it->item->rel->position + 1);
          }
     }

   itb->count++;
   itb->changed = EINA_TRUE;
   it->item->block = itb;
   ecore_job_del(itb->sd->calc_job);
   itb->sd->calc_job = ecore_job_add(_calc_job, itb->sd->obj);

   if (itb->count > itb->sd->max_items_per_block)
     {
        int newc;
        Item_Block *itb2;
        Elm_Gen_Item *it2;
        Eina_Bool done = EINA_FALSE;

        newc = itb->count / 2;

        if (EINA_INLIST_GET(itb)->prev)
          {
             Item_Block *itbp = (Item_Block *)(EINA_INLIST_GET(itb)->prev);

             if (itbp->count + newc < sd->max_items_per_block / 2)
               {
                  /* moving items to previous block */
                  while ((itb->count > newc) && (itb->items))
                    {
                       it2 = eina_list_data_get(itb->items);
                       itb->items = eina_list_remove_list
                           (itb->items, itb->items);
                       itb->count--;

                       itbp->items = eina_list_append(itbp->items, it2);
                       it2->item->block = itbp;
                       itbp->count++;
                    }

                  done = EINA_TRUE;
               }
          }

        if (!done && EINA_INLIST_GET(itb)->next)
          {
             Item_Block *itbn = (Item_Block *)(EINA_INLIST_GET(itb)->next);

             if (itbn->count + newc < sd->max_items_per_block / 2)
               {
                  /* moving items to next block */
                  while ((itb->count > newc) && (itb->items))
                    {
                       Eina_List *l;

                       l = eina_list_last(itb->items);
                       it2 = eina_list_data_get(l);
                       itb->items = eina_list_remove_list(itb->items, l);
                       itb->count--;

                       itbn->items = eina_list_prepend(itbn->items, it2);
                       it2->item->block = itbn;
                       itbn->count++;
                    }

                  done = EINA_TRUE;
               }
          }

        if (!done)
          {
             /* moving items to new block */
             itb2 = calloc(1, sizeof(Item_Block));
             if (!itb2) return EINA_FALSE;
             itb2->sd = sd;
             sd->blocks =
               eina_inlist_append_relative(sd->blocks, EINA_INLIST_GET(itb2),
                                           EINA_INLIST_GET(itb));
             itb2->changed = EINA_TRUE;
             while ((itb->count > newc) && (itb->items))
               {
                  Eina_List *l;

                  l = eina_list_last(itb->items);
                  it2 = l->data;
                  itb->items = eina_list_remove_list(itb->items, l);
                  itb->count--;

                  itb2->items = eina_list_prepend(itb2->items, it2);
                  it2->item->block = itb2;
                  itb2->count++;
               }
          }
     }

   return EINA_TRUE;
}

static int
_item_process(Elm_Genlist_Data *sd,
              Elm_Gen_Item *it)
{
   if (!_item_block_add(sd, it)) return 1;
   if (!sd->blocks)
     _item_block_realize(it->item->block);

   return 0;
}

static void
_item_process_post(Elm_Genlist_Data *sd,
                   Elm_Gen_Item *it,
                   Eina_Bool qadd)
{
   Eina_Bool show_me = EINA_FALSE;

   if (it->item->block->changed)
     {
        show_me = _item_block_recalc
            (it->item->block, it->item->block->num, qadd);
        it->item->block->changed = 0;
        if (sd->pan_changed)
          {
             ELM_SAFE_FREE(sd->calc_job, ecore_job_del);
             _calc_job(sd->obj);
             sd->pan_changed = EINA_FALSE;
          }
     }
   if (show_me) it->item->block->show_me = EINA_TRUE;

   /* when prepending, move the scroller along with the first selected
    * item to create the illusion that we're watching the selected
    * item this prevents the selected item being scrolled off the
    * viewport
    */
   if (sd->selected && it->item->before)
     {
        int y = 0, h;
        Elm_Object_Item *eo_it2;

        eo_it2 = sd->selected->data;
        ELM_GENLIST_ITEM_DATA_GET(eo_it2, it2);
        if (!it2->item->block) return;
        eo_do(sd->obj, elm_interface_scrollable_content_pos_get(NULL, &y));
        evas_object_geometry_get(sd->pan_obj, NULL, NULL, NULL, &h);
        if ((it->y + it->item->block->y > y + h) ||
            (it->y + it->item->block->y + it->item->h < y))
          /* offscreen, just update */
           eo_do(sd->obj, elm_interface_scrollable_content_region_show
            (it2->x + it2->item->block->x, y,
            it2->item->block->w, h));
        else
           eo_do(sd->obj, elm_interface_scrollable_content_region_show
            (it->x + it->item->block->x,
            y + it->item->h, it->item->block->w, h));
     }
}

static int
_queue_process(Elm_Genlist_Data *sd)
{
   int n;
   double t0, t;

   t0 = ecore_time_get();
   for (n = 0; (sd->queue) && (n < 128); n++)
     {
        Elm_Gen_Item *it;

        it = eina_list_data_get(sd->queue);
        sd->queue = eina_list_remove_list(sd->queue, sd->queue);
        it->item->queued = EINA_FALSE;
        if (_item_process(sd, it)) continue;
        t = ecore_time_get();
        _item_process_post(sd, it, EINA_TRUE);
        /* same as eina_inlist_count > 1 */
        if (sd->blocks && sd->blocks->next)
          {
             if ((t - t0) > (ecore_animator_frametime_get())) break;
          }
     }
   if (!sd->queue) _item_scroll(sd);
   return n;
}

static Eina_Bool
_idle_process(void *data,
              Eina_Bool *wakeup)
{
   Elm_Genlist_Data *sd = data;

   if (_queue_process(sd) > 0) *wakeup = EINA_TRUE;
   if (!sd->queue)
     {
        return ECORE_CALLBACK_CANCEL;
     }
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_item_idle_enterer(void *data)
{
   Eina_Bool wakeup = EINA_FALSE;
   ELM_GENLIST_DATA_GET(data, sd);
   Eina_Bool ok = _idle_process(sd, &wakeup);

   if (wakeup)
     {
        // wake up mainloop
        ecore_job_del(sd->calc_job);
        sd->calc_job = ecore_job_add(_calc_job, data);
     }
   if (ok == ECORE_CALLBACK_CANCEL) sd->queue_idle_enterer = NULL;

   return ok;
}

static void
_requeue_idle_enterer(Elm_Genlist_Data *sd)
{
   ecore_idle_enterer_del(sd->queue_idle_enterer);
   sd->queue_idle_enterer = ecore_idle_enterer_add(_item_idle_enterer, sd->obj);
}

static void
_item_queue(Elm_Genlist_Data *sd,
            Elm_Gen_Item *it,
            Eina_Compare_Cb cb)
{
   Evas_Coord w = 0;

   if (it->item->queued) return;
   it->item->queued = EINA_TRUE;
   if (cb && !sd->requeued)
     sd->queue = eina_list_sorted_insert(sd->queue, cb, it);
   else
     sd->queue = eina_list_append(sd->queue, it);
// FIXME: why does a freeze then thaw here cause some genlist
// elm_genlist_item_append() to be much much slower?
//   evas_event_freeze(evas_object_evas_get(sd->obj));
   while ((sd->queue) && ((!sd->blocks) || (!sd->blocks->next)))
     {
        ELM_SAFE_FREE(sd->queue_idle_enterer, ecore_idle_enterer_del);
        _queue_process(sd);
     }
   while ((sd->queue) && (sd->blocks) &&
          (sd->homogeneous) && (sd->mode == ELM_LIST_COMPRESS))
     {
        ELM_SAFE_FREE(sd->queue_idle_enterer, ecore_idle_enterer_del);
        _queue_process(sd);
     }

//   evas_event_thaw(evas_object_evas_get(sd->obj));
//   evas_event_thaw_eval(evas_object_evas_get(sd->obj));
   evas_object_geometry_get(sd->obj, NULL, NULL, &w, NULL);
   if (w > 0) _requeue_idle_enterer(sd);
}

/* If the application wants to know the relative item, use
 * elm_genlist_item_prev_get(it)*/
static void
_item_move_after(Elm_Gen_Item *it,
                 Elm_Gen_Item *after)
{
   if (!it) return;
   if (!after) return;
   if (it == after) return;
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   sd->items =
     eina_inlist_remove(sd->items, EINA_INLIST_GET(it));
   if (it->item->block) _item_block_del(it);

   sd->items = eina_inlist_append_relative
       (sd->items, EINA_INLIST_GET(it), EINA_INLIST_GET(after));
   if (it->item->rel)
      it->item->rel->item->rel_revs =
         eina_list_remove(it->item->rel->item->rel_revs, it);
   it->item->rel = after;
   after->item->rel_revs = eina_list_append(after->item->rel_revs, it);
   it->item->before = EINA_FALSE;
   if (after->item->group_item) it->item->group_item = after->item->group_item;
   _item_queue(sd, it, NULL);

   evas_object_smart_callback_call(WIDGET(it), SIG_MOVED_AFTER, EO_OBJ(it));
}

static void
_access_activate_cb(void *data EINA_UNUSED,
                    Evas_Object *part_obj EINA_UNUSED,
                    Elm_Object_Item *eo_it)
{
   ELM_GENLIST_ITEM_DATA_GET(eo_it, it);
   if (!it) return;

   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);
   if (!sd) return;

   if (sd->multi)
     {
        if (!it->selected)
          {
             it->highlight_cb(it);
             it->sel_cb(it);
          }
        else
          it->unsel_cb(it);
     }
   else
     {
        if (!it->selected)
          {
             while (sd->selected)
               {
                  Elm_Object_Item *eo_sel = sd->selected->data;
                  Elm_Gen_Item *sel = eo_data_scope_get(eo_sel, ELM_GENLIST_ITEM_CLASS);
                  it->unsel_cb(sel);
               }
          }
        else
          {
             const Eina_List *l, *l_next;
             Elm_Object_Item *eo_it2;

             EINA_LIST_FOREACH_SAFE(sd->selected, l, l_next, eo_it2)
               {
                  ELM_GENLIST_ITEM_DATA_GET(eo_it2, it2);
                  if (it2 != it)
                    it->unsel_cb(it2);
               }
          }
        it->highlight_cb(it);
        it->sel_cb(it);
     }
}

/* If the application wants to know the relative item, use
 * elm_genlist_item_next_get(it)*/
static void
_item_move_before(Elm_Gen_Item *it,
                  Elm_Gen_Item *before)
{
   if (!it) return;
   if (!before) return;
   if (it == before) return;
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   sd->items =
     eina_inlist_remove(sd->items, EINA_INLIST_GET(it));
   if (it->item->block) _item_block_del(it);
   sd->items = eina_inlist_prepend_relative
       (sd->items, EINA_INLIST_GET(it), EINA_INLIST_GET(before));
   if (it->item->rel)
      it->item->rel->item->rel_revs =
         eina_list_remove(it->item->rel->item->rel_revs, it);
   it->item->rel = before;
   before->item->rel_revs = eina_list_append(before->item->rel_revs, it);
   it->item->before = EINA_TRUE;
   if (before->item->group_item)
     it->item->group_item = before->item->group_item;
   _item_queue(sd, it, NULL);

   evas_object_smart_callback_call(WIDGET(it), SIG_MOVED_BEFORE, EO_OBJ(it));
}

static void
_item_mouse_up_cb(void *data,
                  Evas *evas EINA_UNUSED,
                  Evas_Object *obj EINA_UNUSED,
                  void *event_info)
{
   Evas_Event_Mouse_Up *ev = event_info;
   Eina_Bool dragged = EINA_FALSE;
   Elm_Gen_Item *it = data;
   Evas_Coord x, y, dx, dy;

   if ((ev->button == 3) && (!it->dragging))
     {
        evas_object_geometry_get(obj, &x, &y, NULL, NULL);
        dx = it->dx - (ev->canvas.x - x);
        dy = it->dy - (ev->canvas.y - y);
        if (dx < 0) dx = -dx;
        if (dy < 0) dy = -dy;
        if ((dx < 5) && (dy < 5))
          evas_object_smart_callback_call(WIDGET(it), SIG_CLICKED_RIGHT, EO_OBJ(it));
        return;
     }

   if (ev->button != 1) return;
   it->down = EINA_FALSE;
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   sd->mouse_down = EINA_FALSE;
   evas_object_smart_callback_call(WIDGET(it), SIG_RELEASED, EO_OBJ(it));
   if (sd->multi_touched)
     {
        if ((!sd->multi) && (!it->selected) && (it->highlighted))
          it->unhighlight_cb(it);
        if (sd->multi_down) return;
        _multi_touch_gesture_eval(it);
        return;
     }
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
     sd->on_hold = EINA_TRUE;
   else sd->on_hold = EINA_FALSE;
   ELM_SAFE_FREE(it->long_timer, ecore_timer_del);
   if (it->dragging)
     {
        it->dragging = EINA_FALSE;
        evas_object_smart_callback_call(WIDGET(it), SIG_DRAG_STOP, EO_OBJ(it));
        dragged = 1;
     }
   ELM_SAFE_FREE(it->item->swipe_timer, ecore_timer_del);
   if (sd->multi_timer)
     {
        ELM_SAFE_FREE(sd->multi_timer, ecore_timer_del);
        sd->multi_timeout = EINA_FALSE;
     }
   if (sd->on_hold)
     {
        if (sd->swipe) _swipe_do(it);
        sd->longpressed = EINA_FALSE;
        sd->on_hold = EINA_FALSE;
        return;
     }
   if ((sd->reorder_mode) && (sd->reorder_it))
     {
        Evas_Coord it_scrl_y = ev->canvas.y - sd->reorder_it->dy;
        sd->reorder_fast = 0;

        if (sd->reorder_rel &&
            (sd->reorder_it->parent == sd->reorder_rel->parent))
          {
             if (it_scrl_y <= sd->reorder_rel->item->scrl_y)
               _item_move_before(sd->reorder_it, sd->reorder_rel);
             else
               _item_move_after(sd->reorder_it, sd->reorder_rel);
             evas_object_smart_callback_call(WIDGET(it), SIG_MOVED, EO_OBJ(it));
          }
        else
          {
             ecore_job_del(sd->calc_job);
             sd->calc_job = ecore_job_add(_calc_job, sd->obj);
          }
        edje_object_signal_emit(VIEW(it), SIGNAL_REORDER_DISABLED, "elm");
        sd->reorder_it = sd->reorder_rel = NULL;
        eo_do(sd->obj, elm_interface_scrollable_hold_set(EINA_FALSE));
        eo_do(sd->obj, elm_interface_scrollable_bounce_allow_set
              (sd->h_bounce, sd->v_bounce));
     }
   if (sd->longpressed)
     {
        sd->longpressed = EINA_FALSE;
        if ((!sd->wasselected) && (!it->flipped))
          it->unsel_cb(it);
        sd->wasselected = EINA_FALSE;
        return;
     }
   if (dragged)
     {
        if (it->want_unrealize)
          {
             _elm_genlist_item_unrealize(it, EINA_FALSE);
             if (it->item->block->want_unrealize)
               _item_block_unrealize(it->item->block);
          }
     }

   if (_is_no_select(it) ||
       (eo_do(EO_OBJ(it), elm_wdg_item_disabled_get()) || (dragged)))
     return;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;

   it->walking++;
   sd->walking++;
   if (sd->focused_item != EO_OBJ(it))
     elm_object_item_focus_set(EO_OBJ(it), EINA_TRUE);
   it->walking--;
   sd->walking--;

   if ((sd->clear_me) && (!sd->walking))
     {
        _internal_elm_genlist_clear(WIDGET(it), EINA_TRUE);
        return;
     }

   if (sd->multi &&
       ((sd->multi_select_mode != ELM_OBJECT_MULTI_SELECT_MODE_WITH_CONTROL) ||
        (evas_key_modifier_is_set(ev->modifiers, "Control"))))
     {
        if (!it->selected)
          {
             it->highlight_cb(it);
             it->sel_cb(it);
          }
        else
          it->unsel_cb(it);
     }
   else
     {
        if (!it->selected)
          {
             while (sd->selected)
               {
                  Elm_Object_Item *eo_sel = sd->selected->data;
                  Elm_Gen_Item *sel = eo_data_scope_get(eo_sel, ELM_GENLIST_ITEM_CLASS);
                  it->unsel_cb(sel);
               }
          }
        else
          {
             const Eina_List *l, *l_next;
             Elm_Object_Item *eo_it2;

             EINA_LIST_FOREACH_SAFE(sd->selected, l, l_next, eo_it2)
               {
                  ELM_GENLIST_ITEM_DATA_GET(eo_it2, it2);
                  if (it2 != it)
                    it->unsel_cb(it2);
               }
          }
        it->highlight_cb(it);
        it->sel_cb(it);
     }
}

static void
_item_mouse_callbacks_add(Elm_Gen_Item *it,
                          Evas_Object *view)
{
   evas_object_event_callback_add
     (view, EVAS_CALLBACK_MOUSE_DOWN, _item_mouse_down_cb, it);
   evas_object_event_callback_add
     (view, EVAS_CALLBACK_MOUSE_UP, _item_mouse_up_cb, it);
   evas_object_event_callback_add
     (view, EVAS_CALLBACK_MOUSE_MOVE, _item_mouse_move_cb, it);
   evas_object_event_callback_add
     (view, EVAS_CALLBACK_MULTI_DOWN, _item_multi_down_cb, it);
   evas_object_event_callback_add
     (view, EVAS_CALLBACK_MULTI_UP, _item_multi_up_cb, it);
   evas_object_event_callback_add
     (view, EVAS_CALLBACK_MULTI_MOVE, _item_multi_move_cb, it);
   evas_object_event_callback_add
     (view, EVAS_CALLBACK_MOUSE_IN, _item_mouse_in_cb, it);
}

static void
_item_mouse_callbacks_del(Elm_Gen_Item *it,
                          Evas_Object *view)
{
   evas_object_event_callback_del_full
     (view, EVAS_CALLBACK_MOUSE_DOWN, _item_mouse_down_cb, it);
   evas_object_event_callback_del_full
     (view, EVAS_CALLBACK_MOUSE_UP, _item_mouse_up_cb, it);
   evas_object_event_callback_del_full
     (view, EVAS_CALLBACK_MOUSE_MOVE, _item_mouse_move_cb, it);
   evas_object_event_callback_del_full
     (view, EVAS_CALLBACK_MULTI_DOWN, _item_multi_down_cb, it);
   evas_object_event_callback_del_full
     (view, EVAS_CALLBACK_MULTI_UP, _item_multi_up_cb, it);
   evas_object_event_callback_del_full
     (view, EVAS_CALLBACK_MULTI_MOVE, _item_multi_move_cb, it);
   evas_object_event_callback_del_full
     (view, EVAS_CALLBACK_MOUSE_IN, _item_mouse_in_cb, it);
}

static Eina_Bool
_scroll_hold_timer_cb(void *data)
{
   ELM_GENLIST_DATA_GET(data, sd);

   if (!data) return ECORE_CALLBACK_CANCEL;

   eo_do(sd->obj, elm_interface_scrollable_hold_set(EINA_FALSE));
   sd->scr_hold_timer = NULL;

   return ECORE_CALLBACK_CANCEL;
}

static void
_decorate_item_unrealize(Elm_Gen_Item *it)
{
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);
   Evas_Object *obj = sd->obj;

   if (!it->item->deco_it_view) return;

   evas_event_freeze(evas_object_evas_get(obj));

   _view_clear(GL_IT(it)->deco_it_view, &(GL_IT(it)->deco_it_texts),
               &(GL_IT(it)->deco_it_contents), &(GL_IT(it)->deco_it_states),
               &(GL_IT(it)->deco_it_content_objs));

   edje_object_part_unswallow(it->item->deco_it_view, VIEW(it));
   evas_object_smart_member_add(VIEW(it), sd->pan_obj);
   ELM_SAFE_FREE(it->item->deco_it_view, evas_object_del);

   if (sd->mode_item == it)
     sd->mode_item = NULL;
   evas_event_thaw(evas_object_evas_get(obj));
   evas_event_thaw_eval(evas_object_evas_get(obj));
}

static void
_decorate_item_finished_signal_cb(void *data,
                                  Evas_Object *obj,
                                  const char *emission EINA_UNUSED,
                                  const char *source EINA_UNUSED)
{
   Elm_Gen_Item *it = data;
   char buf[1024];
   Evas *te;

   if (!it || !obj) return;
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   te = evas_object_evas_get(obj);

   if ((it->generation < sd->generation) || (!it->realized)
       || (!it->item->deco_it_view)) return;

   evas_event_freeze(te);
   it->item->nocache_once = EINA_FALSE;
   _decorate_item_unrealize(it);
   if (it->item->group_item)
     evas_object_stack_above(it->item->VIEW(group_item), sd->stack[1]);

   snprintf(buf, sizeof(buf), "elm,state,%s,passive,finished",
            sd->decorate_it_type);
   edje_object_signal_callback_del_full
     (obj, buf, "elm", _decorate_item_finished_signal_cb, it);
   evas_event_thaw(te);
   evas_event_thaw_eval(te);
}

static Eina_List *
_item_mode_content_unrealize(Elm_Gen_Item *it,
                             Evas_Object *target,
                             Eina_List **source,
                             const char *parts,
                             Eina_List **contents_list)
{
   Eina_List *res = *contents_list;

   if (it->itc->func.content_get)
     {
        const Eina_List *l;
        const char *key;
        Evas_Object *ic = NULL;

        EINA_LIST_FOREACH(*source, l, key)
          {
             if (parts && fnmatch(parts, key, FNM_PERIOD))
               continue;

             ic = edje_object_part_swallow_get(target, key);
             if (ic)
               {
                  res = eina_list_remove(res, ic);
                  edje_object_part_unswallow(target, ic);
                  evas_object_del(ic);
               }
          }
     }

   return res;
}

static Eina_List *
_item_content_unrealize(Elm_Gen_Item *it,
                        Evas_Object *target,
                        Eina_List **source,
                        const char *parts)
{
   Eina_List *res = it->content_objs;

   if (it->itc->func.content_get)
     {
        const Eina_List *l;
        const char *key;
        Evas_Object *ic = NULL;

        EINA_LIST_FOREACH(*source, l, key)
          {
             if (parts && fnmatch(parts, key, FNM_PERIOD))
               continue;

             ic = edje_object_part_swallow_get(target, key);
             if (ic)
               {
                  res = eina_list_remove(res, ic);
                  edje_object_part_unswallow(target, ic);
                  evas_object_del(ic);
               }
          }
     }

   return res;
}

static void
_item_unrealize_cb(Elm_Gen_Item *it)
{
   Evas_Object *content;
   elm_widget_stringlist_free(it->item->flip_contents);
   it->item->flip_contents = NULL;
   EINA_LIST_FREE(it->item->flip_content_objs, content)
     evas_object_del(content);

   /* access */
   if (_elm_config->access_mode == ELM_ACCESS_MODE_ON)
     _elm_access_widget_item_unregister(it->base);

   // unswallow VIEW(it) first then manipulate VIEW(it)
   _decorate_item_unrealize(it);
   if (GL_IT(it)->wsd->decorate_all_mode) _decorate_all_item_unrealize(it);

   if (it->item->nocache_once || it->item->nocache)
     {
        ELM_SAFE_FREE(VIEW(it), evas_object_del);
        ELM_SAFE_FREE(it->spacer, evas_object_del);
     }
   else
     {
        edje_object_mirrored_set(VIEW(it),
                                 elm_widget_mirrored_get(WIDGET(it)));
        edje_object_scale_set(VIEW(it),
                              elm_widget_scale_get(WIDGET(it))
                              * elm_config_scale_get());
        _item_cache_add(it);
     }

   it->states = NULL;
   it->realized = EINA_FALSE;
   it->want_unrealize = EINA_FALSE;
}

static Eina_Bool
_item_block_recalc(Item_Block *itb,
                   int in,
                   Eina_Bool qadd)
{
   const Eina_List *l;
   Elm_Gen_Item *it;
   Evas_Coord minw = 0, minh = 0;
   Eina_Bool show_me = EINA_FALSE, changed = EINA_FALSE;
   Evas_Coord y = 0;
   Elm_Genlist_Data *sd = NULL;

   itb->num = in;
   EINA_LIST_FOREACH(itb->items, l, it)
     {
        sd = GL_IT(it)->wsd;
        if (it->generation < sd->generation) continue;
        show_me |= it->item->show_me;
        if (!itb->realized)
          {
             if (qadd || (itb->sd->homogeneous &&
                          ((it->group && (!itb->sd->group_item_height)) ||
                          ((!it->group) && (!itb->sd->item_height)))))
               {
                  if (!it->item->mincalcd) changed = EINA_TRUE;
                  if (changed)
                    {
                       Eina_Bool doit = EINA_TRUE;

                       if (itb->sd->homogeneous)
                         {
                            if ((it->group) && (itb->sd->group_item_height == 0))
                              doit = EINA_TRUE;
                            else if (itb->sd->item_height == 0)
                              doit = EINA_TRUE;
                            else
                              doit = EINA_FALSE;
                         }
                       if (doit)
                         {
                            _item_realize(it, in, EINA_TRUE);
                            _elm_genlist_item_unrealize(it, EINA_TRUE);
                         }
                       else
                         {
                            if (it->group)
                              {
                                 it->item->w = it->item->minw =
                                   sd->group_item_width;
                                 it->item->h = it->item->minh =
                                   sd->group_item_height;
                              }
                            else
                              {
                                 it->item->w = it->item->minw =
                                   sd->item_width;
                                 it->item->h = it->item->minh =
                                   sd->item_height;
                              }
                            it->item->mincalcd = EINA_TRUE;
                         }
                    }
               }
             else
               {
                  if ((itb->sd->homogeneous) &&
                      (itb->sd->mode == ELM_LIST_COMPRESS))
                    {
                       if (it->group)
                         {
                            it->item->w = it->item->minw =
                                sd->group_item_width;
                            it->item->h = it->item->minh =
                                sd->group_item_height;
                         }
                       else
                         {
                            it->item->w = it->item->minw =
                                sd->item_width;
                            it->item->h = it->item->minh =
                                sd->item_height;
                         }
                       it->item->mincalcd = EINA_TRUE;
                    }
                  else
                    {
                       _item_realize(it, in, EINA_TRUE);
                       _elm_genlist_item_unrealize(it, EINA_TRUE);
                    }
               }
          }
        else
          {
             if (!it->item->mincalcd) changed = EINA_TRUE;
             _item_realize(it, in, EINA_FALSE);
          }
        minh += it->item->minh;
        if (minw < it->item->minw) minw = it->item->minw;
        in++;
        it->x = 0;
        it->y = y;
        y += it->item->h;
     }
   if (changed) itb->sd->pan_changed = changed;
   itb->minw = minw;
   itb->minh = minh;
   itb->changed = EINA_FALSE;
   itb->position_update = EINA_FALSE;

   return show_me;
}

static void
_update_job(void *data)
{
   Eina_Bool position = EINA_FALSE, recalc = EINA_FALSE;
   ELM_GENLIST_DATA_GET(data, sd);
   Item_Block *itb;
   Eina_List *l2;
   int num, num0;

   sd->update_job = NULL;
   num = 0;

   evas_event_freeze(evas_object_evas_get(sd->obj));
   EINA_INLIST_FOREACH(sd->blocks, itb)
     {
        Evas_Coord itminw, itminh;
        Elm_Gen_Item *it;

        if (!itb->updateme)
          {
             num += itb->count;
             if (position)
               _item_block_position(itb, num);
             continue;
          }
        num0 = num;
        recalc = EINA_FALSE;
        EINA_LIST_FOREACH(itb->items, l2, it)
          {
             if (it->item->updateme)
               {
                  itminw = it->item->minw;
                  itminh = it->item->minh;

                  it->item->updateme = EINA_FALSE;
                  if (it->realized)
                    {
                       _elm_genlist_item_unrealize(it, EINA_FALSE);
                       _item_realize(it, num, EINA_FALSE);
                       position = EINA_TRUE;
                    }
                  else
                    {
                       _item_realize(it, num, EINA_TRUE);
                       _elm_genlist_item_unrealize(it, EINA_TRUE);
                    }
                  if ((it->item->minw != itminw) || (it->item->minh != itminh))
                    recalc = EINA_TRUE;
               }
             num++;
          }
        itb->updateme = EINA_FALSE;
        if (recalc)
          {
             position = EINA_TRUE;
             itb->changed = EINA_TRUE;
             _item_block_recalc(itb, num0, EINA_FALSE);
             _item_block_position(itb, num0);
          }
     }
   if (position)
     {
        ecore_job_del(sd->calc_job);
        sd->calc_job = ecore_job_add(_calc_job, sd->obj);
     }
   evas_event_thaw(evas_object_evas_get(sd->obj));
   evas_event_thaw_eval(evas_object_evas_get(sd->obj));
}

static void
_scroll_animate_start_cb(Evas_Object *obj,
                         void *data EINA_UNUSED)
{
   evas_object_smart_callback_call(obj, SIG_SCROLL_ANIM_START, NULL);
}

static void
_scroll_animate_stop_cb(Evas_Object *obj,
                        void *data EINA_UNUSED)
{
   evas_object_smart_callback_call(obj, SIG_SCROLL_ANIM_STOP, NULL);
}

static void
_scroll_drag_start_cb(Evas_Object *obj,
                      void *data EINA_UNUSED)
{
   evas_object_smart_callback_call(obj, SIG_SCROLL_DRAG_START, NULL);
}

static void
_scroll_cb(Evas_Object *obj,
           void *data EINA_UNUSED)
{
   evas_object_smart_callback_call(obj, SIG_SCROLL, NULL);
}

static void
_scroll_drag_stop_cb(Evas_Object *obj,
                     void *data EINA_UNUSED)
{
   evas_object_smart_callback_call(obj, SIG_SCROLL_DRAG_STOP, NULL);
}

static void
_edge_left_cb(Evas_Object *obj,
              void *data EINA_UNUSED)
{
   evas_object_smart_callback_call(obj, SIG_EDGE_LEFT, NULL);
}

static void
_edge_right_cb(Evas_Object *obj,
               void *data EINA_UNUSED)
{
   evas_object_smart_callback_call(obj, SIG_EDGE_RIGHT, NULL);
}

static void
_edge_top_cb(Evas_Object *obj,
             void *data EINA_UNUSED)
{
   evas_object_smart_callback_call(obj, SIG_EDGE_TOP, NULL);
}

static void
_edge_bottom_cb(Evas_Object *obj,
                void *data EINA_UNUSED)
{
   evas_object_smart_callback_call(obj, SIG_EDGE_BOTTOM, NULL);
}

static void
_vbar_drag_cb(Evas_Object *obj,
                void *data EINA_UNUSED)
{
   evas_object_smart_callback_call(obj, SIG_VBAR_DRAG, NULL);
}

static void
_vbar_press_cb(Evas_Object *obj,
                void *data EINA_UNUSED)
{
   evas_object_smart_callback_call(obj, SIG_VBAR_PRESS, NULL);
}

static void
_vbar_unpress_cb(Evas_Object *obj,
                void *data EINA_UNUSED)
{
   evas_object_smart_callback_call(obj, SIG_VBAR_UNPRESS, NULL);
}

static void
_hbar_drag_cb(Evas_Object *obj,
                void *data EINA_UNUSED)
{
   evas_object_smart_callback_call(obj, SIG_HBAR_DRAG, NULL);
}

static void
_hbar_press_cb(Evas_Object *obj,
                void *data EINA_UNUSED)
{
   evas_object_smart_callback_call(obj, SIG_HBAR_PRESS, NULL);
}

static void
_hbar_unpress_cb(Evas_Object *obj,
                void *data EINA_UNUSED)
{
   evas_object_smart_callback_call(obj, SIG_HBAR_UNPRESS, NULL);
}

static void
_decorate_item_realize(Elm_Gen_Item *it)
{
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);
   Evas_Object *obj = sd->obj;

   if ((it->item->deco_it_view) || (it->generation <
                                    sd->generation)) return;

   evas_event_freeze(evas_object_evas_get(obj));
   it->item->deco_it_view = _view_create(it, it->itc->decorate_item_style);

   /* signal callback add */
   evas_object_event_callback_add
     (it->item->deco_it_view, EVAS_CALLBACK_MOUSE_DOWN, _item_mouse_down_cb,
     it);
   evas_object_event_callback_add
     (it->item->deco_it_view, EVAS_CALLBACK_MOUSE_UP, _item_mouse_up_cb, it);
   evas_object_event_callback_add
     (it->item->deco_it_view, EVAS_CALLBACK_MOUSE_MOVE, _item_mouse_move_cb,
     it);

   if (eina_list_count(it->item->deco_it_content_objs) != 0)
     ERR_ABORT("If you see this error, please notify us and we"
               "will fix it");

   /* text_get, content_get, state_get */
   _item_text_realize
     (it, it->item->deco_it_view, &it->item->deco_it_texts, NULL);
   if (!it->item->deco_it_contents)
     it->item->deco_it_contents =
       elm_widget_stringlist_get
         (edje_object_data_get(it->item->deco_it_view, "contents"));
   it->item->deco_it_content_objs =
      _item_content_realize
      (it, it->item->deco_it_view, &it->item->deco_it_contents, NULL,
       &it->item->deco_it_content_objs);
   _item_state_realize
     (it, it->item->deco_it_view, &it->item->deco_it_states, NULL);
   edje_object_part_swallow
     (it->item->deco_it_view,
     edje_object_data_get(it->item->deco_it_view, "mode_part"), VIEW(it));

   it->want_unrealize = EINA_FALSE;
   evas_event_thaw(evas_object_evas_get(obj));
   evas_event_thaw_eval(evas_object_evas_get(obj));
}

static void
_decorate_item_set(Elm_Gen_Item *it)
{
   char buf[1024];

   if (!it) return;

   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   sd->mode_item = it;
   it->item->nocache_once = EINA_TRUE;

   ELM_SAFE_FREE(sd->scr_hold_timer, ecore_timer_del);
   eo_do(sd->obj, elm_interface_scrollable_hold_set(EINA_TRUE));
   sd->scr_hold_timer = ecore_timer_add(SCR_HOLD_TIME, _scroll_hold_timer_cb, sd->obj);

   evas_event_freeze(evas_object_evas_get(sd->obj));
   _decorate_item_realize(it);
   if (it->item->group_item)
     evas_object_stack_above(it->item->VIEW(group_item), sd->stack[1]);
   _item_position
     (it, it->item->deco_it_view, it->item->scrl_x, it->item->scrl_y);
   evas_event_thaw(evas_object_evas_get(sd->obj));
   evas_event_thaw_eval(evas_object_evas_get(sd->obj));

   snprintf(buf, sizeof(buf), "elm,state,%s,active", sd->decorate_it_type);
   edje_object_signal_emit(it->item->deco_it_view, buf, "elm");
}

static void
_decorate_item_unset(Elm_Genlist_Data *sd)
{
   char buf[1024], buf2[1024];
   Elm_Gen_Item *it;

   if (!sd->mode_item) return;

   it = sd->mode_item;
   it->item->nocache_once = EINA_TRUE;

   snprintf(buf, sizeof(buf), "elm,state,%s,passive", sd->decorate_it_type);
   snprintf(buf2, sizeof(buf2), "elm,state,%s,passive,finished",
            sd->decorate_it_type);
   edje_object_signal_emit(it->item->deco_it_view, buf, "elm");
   edje_object_signal_callback_add
     (it->item->deco_it_view, buf2, "elm", _decorate_item_finished_signal_cb,
     it);
   sd->mode_item = NULL;
}

static void
_elm_genlist_looping_up_cb(void *data,
                           Evas_Object *obj EINA_UNUSED,
                           const char *emission EINA_UNUSED,
                           const char *source EINA_UNUSED)
{
   Evas_Object *genlist = data;

   ELM_GENLIST_DATA_GET(genlist, sd);

   Elm_Object_Item *eo_it = elm_genlist_last_item_get(genlist);

   elm_genlist_item_show(eo_it, ELM_GENLIST_ITEM_SCROLLTO_IN);
   _elm_widget_focus_highlight_signal_emit(genlist, "elm,action,focus,move,up", "elm");
   elm_layout_signal_emit(genlist, "elm,action,looping,up,end", "elm");
   sd->item_looping_on = EINA_FALSE;
}

static void
_elm_genlist_looping_down_cb(void *data,
                             Evas_Object *obj EINA_UNUSED,
                             const char *emission EINA_UNUSED,
                             const char *source EINA_UNUSED)
{
   Evas_Object *genlist = data;

   ELM_GENLIST_DATA_GET(genlist, sd);

   Elm_Object_Item *eo_it = elm_genlist_first_item_get(genlist);

   elm_genlist_item_show(eo_it, ELM_GENLIST_ITEM_SCROLLTO_IN);
   _elm_widget_focus_highlight_signal_emit(genlist, "elm,action,focus,move,down", "elm");
   elm_layout_signal_emit(genlist, "elm,action,looping,down,end", "elm");
   sd->item_looping_on = EINA_FALSE;
}

static void
_elm_genlist_focus_highlight_move_down_end_cb(void *data,
                                              Evas_Object *obj EINA_UNUSED,
                                              const char *emission EINA_UNUSED,
                                              const char *source EINA_UNUSED)
{
      Evas_Object *gl = data;
      Elm_Object_Item *eo_it = elm_genlist_first_item_get(gl);

      if (!_elm_config->item_select_on_focus_disable)
        elm_genlist_item_selected_set(eo_it, EINA_TRUE);
      else
        elm_object_item_focus_set(eo_it, EINA_TRUE);

      _elm_widget_focus_highlight_signal_emit(gl, "elm,action,focus,move,home,up", "elm");
}

static void
_elm_genlist_focus_highlight_move_up_end_cb(void *data,
                                            Evas_Object *obj EINA_UNUSED,
                                            const char *emission EINA_UNUSED,
                                            const char *source EINA_UNUSED)
{
      Evas_Object *gl = data;
      Elm_Object_Item *eo_it = elm_genlist_last_item_get(gl);

      if (!_elm_config->item_select_on_focus_disable)
        elm_genlist_item_selected_set(eo_it, EINA_TRUE);
      else
        elm_object_item_focus_set(eo_it, EINA_TRUE);

      _elm_widget_focus_highlight_signal_emit(gl, "elm,action,focus,move,home,down", "elm");
}

EOLIAN static void
_elm_genlist_evas_object_smart_add(Eo *obj, Elm_Genlist_Data *priv)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);
   Elm_Genlist_Pan_Data *pan_data;
   Evas_Coord minw, minh;
   int i;

   eo_do_super(obj, MY_CLASS, evas_obj_smart_add());
   elm_widget_sub_object_parent_add(obj);

   priv->hit_rect = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(priv->hit_rect, obj);
   elm_widget_sub_object_add(obj, priv->hit_rect);

   /* common scroller hit rectangle setup */
   evas_object_color_set(priv->hit_rect, 0, 0, 0, 0);
   evas_object_show(priv->hit_rect);
   evas_object_repeat_events_set(priv->hit_rect, EINA_TRUE);

   elm_widget_can_focus_set(obj, EINA_TRUE);
   elm_widget_on_show_region_hook_set(obj, _show_region_hook, NULL);

   priv->generation = 1;

   if (!elm_layout_theme_set
       (obj, "genlist", "base", elm_widget_style_get(obj)))
     CRI("Failed to set layout!");

   eo_do(obj, elm_interface_scrollable_objects_set(wd->resize_obj, priv->hit_rect));

   eo_do(obj, elm_interface_scrollable_bounce_allow_set
         (EINA_FALSE, _elm_config->thumbscroll_bounce_enable));
   priv->v_bounce = _elm_config->thumbscroll_bounce_enable;

   eo_do(obj,
         elm_interface_scrollable_animate_start_cb_set(_scroll_animate_start_cb),
         elm_interface_scrollable_animate_stop_cb_set(_scroll_animate_stop_cb),
         elm_interface_scrollable_scroll_cb_set(_scroll_cb),
         elm_interface_scrollable_drag_start_cb_set(_scroll_drag_start_cb),
         elm_interface_scrollable_drag_stop_cb_set(_scroll_drag_stop_cb),
         elm_interface_scrollable_edge_left_cb_set(_edge_left_cb),
         elm_interface_scrollable_edge_right_cb_set(_edge_right_cb),
         elm_interface_scrollable_edge_top_cb_set(_edge_top_cb),
         elm_interface_scrollable_edge_bottom_cb_set(_edge_bottom_cb),
         elm_interface_scrollable_vbar_drag_cb_set(_vbar_drag_cb),
         elm_interface_scrollable_vbar_press_cb_set(_vbar_press_cb),
         elm_interface_scrollable_vbar_unpress_cb_set(_vbar_unpress_cb),
         elm_interface_scrollable_hbar_drag_cb_set(_hbar_drag_cb),
         elm_interface_scrollable_hbar_press_cb_set(_hbar_press_cb),
         elm_interface_scrollable_hbar_unpress_cb_set(_hbar_unpress_cb));

   priv->mode = ELM_LIST_SCROLL;
   priv->max_items_per_block = MAX_ITEMS_PER_BLOCK;
   priv->item_cache_max = priv->max_items_per_block * 2;
   priv->longpress_timeout = _elm_config->longpress_timeout;
   priv->highlight = EINA_TRUE;

   priv->pan_obj = eo_add(MY_PAN_CLASS, evas_object_evas_get(obj));
   pan_data = eo_data_scope_get(priv->pan_obj, MY_PAN_CLASS);
   eo_data_ref(obj, NULL);
   pan_data->wobj = obj;
   pan_data->wsd = priv;

   for (i = 0; i < 2; i++)
     {
        priv->stack[i] = evas_object_rectangle_add(evas_object_evas_get(obj));
        evas_object_smart_member_add(priv->stack[i], priv->pan_obj);
     }

   eo_do(obj, elm_interface_scrollable_extern_pan_set(priv->pan_obj));

   edje_object_size_min_calc(wd->resize_obj, &minw, &minh);
   evas_object_size_hint_min_set(obj, minw, minh);

   _mirrored_set(obj, elm_widget_mirrored_get(obj));

   elm_layout_sizing_eval(obj);

   edje_object_signal_callback_add(wd->resize_obj, "elm,looping,up,done", "elm", _elm_genlist_looping_up_cb, obj);
   edje_object_signal_callback_add(wd->resize_obj, "elm,looping,down,done", "elm", _elm_genlist_looping_down_cb, obj);
}

EOLIAN static void
_elm_genlist_evas_object_smart_del(Eo *obj, Elm_Genlist_Data *sd)
{
   int i;

   if (sd->decorate_all_mode)
     elm_genlist_decorate_mode_set(sd->obj, EINA_FALSE);
   sd->queue = eina_list_free(sd->queue);
   elm_genlist_clear(obj);
   for (i = 0; i < 2; i++)
     ELM_SAFE_FREE(sd->stack[i], evas_object_del);
   ELM_SAFE_FREE(sd->pan_obj, evas_object_del);

   _item_cache_zero(sd);
   ecore_job_del(sd->calc_job);
   ecore_job_del(sd->update_job);
   ecore_idle_enterer_del(sd->queue_idle_enterer);
   ecore_idler_del(sd->must_recalc_idler);
   ecore_timer_del(sd->multi_timer);
   ecore_timer_del(sd->scr_hold_timer);
   eina_stringshare_del(sd->decorate_it_type);
   ecore_animator_del(sd->tree_effect_animator);

   eo_do_super(obj, MY_CLASS, evas_obj_smart_del());
}

EOLIAN static void
_elm_genlist_evas_object_smart_move(Eo *obj, Elm_Genlist_Data *sd, Evas_Coord x, Evas_Coord y)
{
   eo_do_super(obj, MY_CLASS, evas_obj_smart_move(x, y));

   evas_object_move(sd->hit_rect, x, y);
}

EOLIAN static void
_elm_genlist_evas_object_smart_resize(Eo *obj, Elm_Genlist_Data *sd, Evas_Coord w, Evas_Coord h)
{
   eo_do_super(obj, MY_CLASS, evas_obj_smart_resize(w, h));

   evas_object_resize(sd->hit_rect, w, h);
   if ((sd->queue) && (!sd->queue_idle_enterer) && (w > 0))
     _requeue_idle_enterer(sd);
}

EOLIAN static void
_elm_genlist_evas_object_smart_member_add(Eo *obj, Elm_Genlist_Data *sd, Evas_Object *member)
{
   eo_do_super(obj, MY_CLASS, evas_obj_smart_member_add(member));

   if (sd->hit_rect)
     evas_object_raise(sd->hit_rect);
}

static void
_access_obj_process(Elm_Genlist_Data *sd, Eina_Bool is_access)
{
   Item_Block *itb;
   Eina_Bool done = EINA_FALSE;

   EINA_INLIST_FOREACH(sd->blocks, itb)
     {
        if (itb->realized)
          {
             Eina_List *l;
             Elm_Gen_Item *it;

             done = EINA_TRUE;
             EINA_LIST_FOREACH(itb->items, l, it)
               {
                  if (!it->realized) continue;
                  if (is_access) _access_widget_item_register(it);
                  else
                    _elm_access_widget_item_unregister(it->base);
               }
          }
        else if (done) break;
     }
}

EOLIAN static void
_elm_genlist_elm_widget_access(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd, Eina_Bool acs)
{
   _elm_genlist_smart_focus_next_enable = acs;
   _access_obj_process(sd, _elm_genlist_smart_focus_next_enable);
}

EAPI Evas_Object *
elm_genlist_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   Evas_Object *obj = eo_add(MY_CLASS, parent);
   return obj;
}

EOLIAN static void
_elm_genlist_eo_base_constructor(Eo *obj, Elm_Genlist_Data *sd)
{
   sd->obj = obj;

   eo_do_super(obj, MY_CLASS, eo_constructor());
   eo_do(obj,
         evas_obj_type_set(MY_CLASS_NAME_LEGACY),
         evas_obj_smart_callbacks_descriptions_set(_smart_callbacks),
         elm_interface_atspi_accessible_role_set(ELM_ATSPI_ROLE_LIST));
}

static void
_clear(Elm_Genlist_Data *sd)
{
   sd->anchor_item = NULL;
   ELM_SAFE_FREE(sd->queue_idle_enterer, ecore_idle_enterer_del);
   ELM_SAFE_FREE(sd->must_recalc_idler, ecore_idler_del);
   ELM_SAFE_FREE(sd->queue, eina_list_free);
   ELM_SAFE_FREE(sd->reorder_move_animator, ecore_animator_del);
   sd->show_item = NULL;
   sd->reorder_old_pan_y = 0;
}

static void
_internal_elm_genlist_clear(Evas_Object *obj,
                            Eina_Bool standby)
{
   Eina_Inlist *next, *l;

   ELM_GENLIST_DATA_GET(obj, sd);

   if (!standby) sd->generation++;

   ELM_SAFE_FREE(sd->state, eina_inlist_sorted_state_free);

   if (sd->walking > 0)
     {
        sd->clear_me = EINA_TRUE;
        return;
     }

   evas_event_freeze(evas_object_evas_get(sd->obj));
   for (l = sd->items, next = l ? l->next : NULL;
        l;
        l = next, next = next ? next->next : NULL)
     {
        Elm_Gen_Item *it = ELM_GEN_ITEM_FROM_INLIST(l);

        if (it->generation < sd->generation)
          {
             Elm_Gen_Item *itn = NULL;

             if (next) itn = ELM_GEN_ITEM_FROM_INLIST(next);
             if (itn) itn->walking++;  /* prevent early death of subitem */
             it->del_cb(it);
             eo_del(EO_OBJ(it));
             if (itn) itn->walking--;
          }
     }
   sd->clear_me = EINA_FALSE;
   sd->pan_changed = EINA_TRUE;
   if (!sd->queue)
     {
        ELM_SAFE_FREE(sd->calc_job, ecore_job_del);
        _clear(sd);
     }
   sd->pan_x = 0;
   sd->pan_y = 0;
   sd->minw = 0;
   sd->minh = 0;

   ELM_SAFE_FREE(sd->event_block_rect, evas_object_del);

   if (sd->pan_obj)
     {
        evas_object_size_hint_min_set(sd->pan_obj, sd->minw, sd->minh);
        evas_object_smart_callback_call(sd->pan_obj, "changed", NULL);
     }
   elm_layout_sizing_eval(sd->obj);
   eo_do(obj, elm_interface_scrollable_content_region_show(0, 0, 0, 0));
   evas_event_thaw(evas_object_evas_get(sd->obj));
   evas_event_thaw_eval(evas_object_evas_get(sd->obj));
}

static void
_item_select(Elm_Gen_Item *it)
{
   Evas_Object *obj = WIDGET(it);
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);
   Elm_Object_Item *eo_it = EO_OBJ(it);

   if (_is_no_select(it) ||
       (it->generation < sd->generation) ||
       (it->decorate_it_set))
     return;

   if (!it->selected)
     {
        it->selected = EINA_TRUE;
        sd->selected =
          eina_list_append(sd->selected, eo_it);
     }
   else if ((sd->select_mode != ELM_OBJECT_SELECT_MODE_ALWAYS) &&
            (it->select_mode != ELM_OBJECT_SELECT_MODE_ALWAYS))
     return;

   evas_object_ref(obj);
   it->walking++;
   sd->walking++;
   if (it->func.func) it->func.func((void *)it->func.data, WIDGET(it), eo_it);
   if (it->generation == sd->generation)
     {
        evas_object_smart_callback_call(WIDGET(it), SIG_SELECTED, eo_it);
        elm_object_item_focus_set(eo_it, EINA_TRUE);
        _elm_genlist_item_content_focus_set(it, ELM_FOCUS_PREVIOUS);
     }

   if (!(sd->focus_on_selection_enabled || _elm_config->item_select_on_focus_disable))
     {
        Evas_Object *swallow_obj;
        Eina_List *l;
        EINA_LIST_FOREACH(it->content_objs, l, swallow_obj)
          {
             if (elm_widget_is(swallow_obj) && elm_object_focus_get(swallow_obj))
               {
                  elm_object_focus_set(obj, EINA_FALSE);
                  elm_object_focus_set(obj, EINA_TRUE);
                  break;
               }
          }
     }

   it->walking--;
   sd->walking--;
   if ((sd->clear_me) && (!sd->walking))
     _internal_elm_genlist_clear(WIDGET(it), EINA_TRUE);
   else
     {
        if ((!it->walking) && (it->generation < sd->generation))
          {
             it->del_cb(it);
             eo_del(eo_it);
          }
        else
          sd->last_selected_item = eo_it;
     }

   evas_object_unref(obj);
}

EOLIAN static Evas_Object *
_elm_genlist_item_elm_widget_item_part_content_get(Eo *eo_it EINA_UNUSED, Elm_Gen_Item *it, const char * part)
{
   Evas_Object *ret = NULL;
   if (it->deco_all_view)
     ret = edje_object_part_swallow_get(it->deco_all_view, part);
   else if (it->decorate_it_set)
     ret = edje_object_part_swallow_get(it->item->deco_it_view, part);
   if (!ret)
     {
        if (part)
          ret = edje_object_part_swallow_get(VIEW(it), part);
        else
          ret = edje_object_part_swallow_get(VIEW(it), "elm.swallow.icon");
     }
   return ret;
}

EOLIAN static const char *
_elm_genlist_item_elm_widget_item_part_text_get(Eo *eo_it EINA_UNUSED, Elm_Gen_Item *it, const char * part)
{
   if (!it->itc->func.text_get) return NULL;
   const char *ret = NULL;
   if (it->deco_all_view)
     ret = edje_object_part_text_get(it->deco_all_view, part);
   else if (it->decorate_it_set)
     ret = edje_object_part_text_get(it->item->deco_it_view, part);
   if (!ret)
     {
        if (part)
          ret = edje_object_part_text_get(VIEW(it), part);
        else
          ret = edje_object_part_text_get(VIEW(it), "elm.text");
     }
   return ret;
}

EOLIAN static void
_elm_genlist_item_elm_widget_item_disable(Eo *eo_it EINA_UNUSED, Elm_Gen_Item *it)
{
   Eina_List *l;
   Evas_Object *obj;

   if (it->generation < GL_IT(it)->wsd->generation) return;

   if (it->selected)
     elm_genlist_item_selected_set(EO_OBJ(it), EINA_FALSE);

   if (it->realized)
     {
        if (eo_do(EO_OBJ(it), elm_wdg_item_disabled_get()))
          {
             edje_object_signal_emit(VIEW(it), SIGNAL_DISABLED, "elm");
             if (it->deco_all_view)
               edje_object_signal_emit
                 (it->deco_all_view, SIGNAL_DISABLED, "elm");
          }
        else
          {
             edje_object_signal_emit(VIEW(it), SIGNAL_ENABLED, "elm");
             if (it->deco_all_view)
               edje_object_signal_emit
                 (it->deco_all_view, SIGNAL_ENABLED, "elm");
          }
        EINA_LIST_FOREACH(it->content_objs, l, obj)
          elm_widget_disabled_set(obj, eo_do(EO_OBJ(it), elm_wdg_item_disabled_get()));
     }
}

EOLIAN static Eina_Bool
_elm_genlist_item_elm_widget_item_del_pre(Eo *eo_it, Elm_Gen_Item *it)
{
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   if (it->walking > 0)
     {
        // FIXME: relative will be better to be fixed. it is too harsh.
        if (it->item->rel)
          {
             it->item->rel->item->rel_revs =
               eina_list_remove(it->item->rel->item->rel_revs, it);
             it->item->rel = NULL;
          }
        if (it->item->rel_revs)
          {
             Elm_Gen_Item *tmp;
             EINA_LIST_FREE(it->item->rel_revs, tmp) tmp->item->rel = NULL;
          }
        elm_genlist_item_subitems_clear(eo_it);
        if (sd->show_item == it)
          sd->show_item = NULL;

        _elm_genlist_item_del_not_serious(it);
        if (it->item->block)
          {
             if (it->realized) _elm_genlist_item_unrealize(it, EINA_FALSE);
             it->item->block->changed = EINA_TRUE;
             ecore_job_del(sd->calc_job);
             sd->calc_job = ecore_job_add(_calc_job, sd->obj);
          }
        if (it->parent)
          {
             it->parent->item->items =
               eina_list_remove(it->parent->item->items, eo_it);
             it->parent = NULL;
          }
        return EINA_FALSE;
     }

   _item_del(it);
   return EINA_TRUE;
}

EOLIAN static void
_elm_genlist_item_elm_widget_item_signal_emit(Eo *eo_it EINA_UNUSED, Elm_Gen_Item *it, const char *emission, const char *source)
{
   edje_object_signal_emit(VIEW(it), emission, source);
}

EOLIAN static void
_elm_genlist_item_elm_widget_item_focus_set(Eo *eo_it, Elm_Gen_Item *it, Eina_Bool focused)
{
   Evas_Object *obj = WIDGET(it);
   ELM_GENLIST_DATA_GET(obj, sd);

   if (focused)
     {
        sd->last_focused_item = eo_it;
        if (!elm_object_focus_get(obj))
          elm_object_focus_set(obj, EINA_TRUE);

        if (eo_it != sd->focused_item)
          {
             if (sd->focused_item)
               _elm_genlist_item_unfocused(sd->focused_item);
             _elm_genlist_item_focused(eo_it);

             _elm_widget_item_highlight_in_theme(obj, EO_OBJ(it));
             _elm_widget_highlight_in_theme_update(obj);
             _elm_widget_focus_highlight_start(obj);
          }
     }
   else
     _elm_genlist_item_unfocused(eo_it);
}

EOLIAN static Eina_Bool
_elm_genlist_item_elm_widget_item_focus_get(Eo *eo_it, Elm_Gen_Item *it)
{
   Evas_Object *obj = WIDGET(it);
   ELM_GENLIST_DATA_GET(obj, sd);

   if (eo_it == sd->focused_item)
     return EINA_TRUE;

   return EINA_FALSE;
}

EOLIAN static void
_elm_genlist_item_eo_base_constructor(Eo *eo_it, Elm_Gen_Item *it)
{
   eo_do_super(eo_it, ELM_GENLIST_ITEM_CLASS, eo_constructor());
   it->base = eo_data_scope_get(eo_it, ELM_WIDGET_ITEM_CLASS);
}

static Elm_Gen_Item *
_elm_genlist_item_new(Elm_Genlist_Data *sd,
                      const Elm_Genlist_Item_Class *itc,
                      const void *data,
                      Elm_Object_Item *eo_parent,
                      Elm_Genlist_Item_Type type,
                      Evas_Smart_Cb func,
                      const void *func_data)
{
   Elm_Gen_Item *it2;
   int depth = 0;

   if (!itc) return NULL;

   Eo *eo_it = eo_add(ELM_GENLIST_ITEM_CLASS, sd->obj);
   if (!eo_it) return NULL;
   ELM_GENLIST_ITEM_DATA_GET(eo_it, it);

   it->generation = sd->generation;
   it->itc = itc;
   elm_genlist_item_class_ref((Elm_Genlist_Item_Class *)itc);

   ELM_GENLIST_ITEM_DATA_GET(eo_parent, parent);
   WIDGET_ITEM_DATA_SET(EO_OBJ(it), data);
   it->parent = parent;
   it->func.func = func;
   it->func.data = func_data;

   it->del_cb = (Ecore_Cb)_item_del;
   it->highlight_cb = (Ecore_Cb)_item_highlight;
   it->unhighlight_cb = (Ecore_Cb)_item_unhighlight;
   it->sel_cb = (Ecore_Cb)_item_select;
   it->unsel_cb = (Ecore_Cb)_item_unselect;
   it->unrealize_cb = (Ecore_Cb)_item_unrealize_cb;

   GL_IT(it) = ELM_NEW(Elm_Gen_Item_Type);
   GL_IT(it)->wsd = sd;
   GL_IT(it)->type = type;
   if (type & ELM_GENLIST_ITEM_GROUP) it->group++;

   if (it->parent)
     {
        if (it->parent->group)
          GL_IT(it)->group_item = parent;
        else if (GL_IT(it->parent)->group_item)
          GL_IT(it)->group_item = GL_IT(it->parent)->group_item;
     }
   for (it2 = it, depth = 0; it2->parent; it2 = it2->parent)
     {
        if (!it2->parent->group) depth += 1;
     }
   GL_IT(it)->expanded_depth = depth;
   sd->item_count++;

   return it;
}

static int
_elm_genlist_item_compare(const void *data,
                          const void *data1)
{
   const Elm_Gen_Item *it, *item1;

   it = ELM_GEN_ITEM_FROM_INLIST(data);
   item1 = ELM_GEN_ITEM_FROM_INLIST(data1);
   return GL_IT(it)->wsd->item_compare_cb(EO_OBJ(it), EO_OBJ(item1));
}

static int
_elm_genlist_item_list_compare(const void *data,
                               const void *data1)
{
   const Elm_Gen_Item *it = data;
   const Elm_Gen_Item *item1 = data1;

   return GL_IT(it)->wsd->item_compare_cb(EO_OBJ(it), EO_OBJ(item1));
}

static int
_elm_genlist_eo_item_list_compare(const void *data,
                               const void *data1)
{
   const Elm_Object_Item *eo_it = data;
   const Elm_Object_Item *eo_item1 = data1;
   ELM_GENLIST_ITEM_DATA_GET(eo_it, it);

   return GL_IT(it)->wsd->item_compare_cb(eo_it, eo_item1);
}

EOLIAN static unsigned int
_elm_genlist_items_count(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd)
{
   return sd->item_count;
}

static Eina_List *
_list_last_recursive(Eina_List *list)
{
   Eina_List *ll, *ll2;
   Elm_Object_Item *eo_it2;

   ll = eina_list_last(list);
   if (!ll) return NULL;

   eo_it2 = ll->data;
   ELM_GENLIST_ITEM_DATA_GET(eo_it2, it2);

   if (it2->item->items)
     {
        ll2 = _list_last_recursive(it2->item->items);
        if (ll2)
          {
             return ll2;
          }
     }

   return ll;
}

EOLIAN static Elm_Object_Item*
_elm_genlist_item_append(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd, const Elm_Genlist_Item_Class *itc, const void *data, Elm_Object_Item *parent, Elm_Genlist_Item_Type type, Evas_Smart_Cb func, const void *func_data)
{
   Elm_Gen_Item *it;

   it = _elm_genlist_item_new
       (sd, itc, data, parent, type, func, func_data);
   if (!it) return NULL;

   if (!it->parent)
     {
        if (it->group)
          sd->group_items = eina_list_append(sd->group_items, it);
        sd->items = eina_inlist_append(sd->items, EINA_INLIST_GET(it));
        it->item->rel = NULL;
     }
   else
     {
        Elm_Object_Item *eo_it2 = NULL;
        Eina_List *ll = _list_last_recursive(it->parent->item->items);

        if (ll) eo_it2 = ll->data;
        it->parent->item->items =
          eina_list_append(it->parent->item->items, EO_OBJ(it));
        if (!eo_it2) eo_it2 = EO_OBJ(it->parent);
        ELM_GENLIST_ITEM_DATA_GET(eo_it2, it2);
        sd->items = eina_inlist_append_relative
            (sd->items, EINA_INLIST_GET(it), EINA_INLIST_GET(it2));
        it->item->rel = it2;
        it2->item->rel_revs = eina_list_append(it2->item->rel_revs, it);
     }
   it->item->before = EINA_FALSE;
   _item_queue(sd, it, NULL);

   return EO_OBJ(it);
}

EOLIAN static Elm_Object_Item*
_elm_genlist_item_prepend(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd, const Elm_Genlist_Item_Class *itc, const void *data, Elm_Object_Item *parent, Elm_Genlist_Item_Type type, Evas_Smart_Cb func, const void *func_data)
{
   Elm_Gen_Item *it;

   it = _elm_genlist_item_new
       (sd, itc, data, parent, type, func, func_data);
   if (!it) return NULL;

   if (!it->parent)
     {
        if (it->group)
          sd->group_items = eina_list_prepend(sd->group_items, it);
        sd->items = eina_inlist_prepend(sd->items, EINA_INLIST_GET(it));
        it->item->rel = NULL;
     }
   else
     {
        Elm_Object_Item *eo_it2 = NULL;
        Eina_List *ll = it->parent->item->items;

        if (ll) eo_it2 = ll->data;
        it->parent->item->items =
          eina_list_prepend(it->parent->item->items, EO_OBJ(it));
        if (!eo_it2) eo_it2 = EO_OBJ(it->parent);
        ELM_GENLIST_ITEM_DATA_GET(eo_it2, it2);
        sd->items = eina_inlist_prepend_relative
            (sd->items, EINA_INLIST_GET(it), EINA_INLIST_GET(it2));
        it->item->rel = it2;
        it2->item->rel_revs = eina_list_append(it2->item->rel_revs, it);
     }
   it->item->before = EINA_TRUE;
   _item_queue(sd, it, NULL);

   return EO_OBJ(it);
}

EOLIAN static Elm_Object_Item*
_elm_genlist_item_insert_after(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd, const Elm_Genlist_Item_Class *itc, const void *data, Elm_Object_Item *parent, Elm_Object_Item *eo_after, Elm_Genlist_Item_Type type, Evas_Smart_Cb func, const void *func_data)
{
   ELM_GENLIST_ITEM_DATA_GET(eo_after, after);
   Elm_Gen_Item *it;

   ELM_GENLIST_ITEM_CHECK_OR_RETURN(after, NULL);

   /* It makes no sense to insert after in an empty list with after !=
    * NULL, something really bad is happening in your app. */
   EINA_SAFETY_ON_NULL_RETURN_VAL(sd->items, NULL);

   it = _elm_genlist_item_new
       (sd, itc, data, parent, type, func, func_data);
   if (!it) return NULL;

   if (!it->parent)
     {
        if ((it->group) && (after->group))
          sd->group_items = eina_list_append_relative
              (sd->group_items, it, after);
     }
   else
     {
        it->parent->item->items =
          eina_list_append_relative(it->parent->item->items, EO_OBJ(it), eo_after);
     }
   sd->items = eina_inlist_append_relative
       (sd->items, EINA_INLIST_GET(it), EINA_INLIST_GET(after));

   it->item->rel = after;
   after->item->rel_revs = eina_list_append(after->item->rel_revs, it);
   it->item->before = EINA_FALSE;
   _item_queue(sd, it, NULL);

   return EO_OBJ(it);
}

EOLIAN static Elm_Object_Item*
_elm_genlist_item_insert_before(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd, const Elm_Genlist_Item_Class *itc, const void *data, Elm_Object_Item *parent, Elm_Object_Item *eo_before, Elm_Genlist_Item_Type type, Evas_Smart_Cb func, const void *func_data)
{
   ELM_GENLIST_ITEM_DATA_GET(eo_before, before);
   Elm_Gen_Item *it;

   ELM_GENLIST_ITEM_CHECK_OR_RETURN(before, NULL);

   /* It makes no sense to insert before in an empty list with before
    * != NULL, something really bad is happening in your app. */
   EINA_SAFETY_ON_NULL_RETURN_VAL(sd->items, NULL);

   it = _elm_genlist_item_new
       (sd, itc, data, parent, type, func, func_data);
   if (!it) return NULL;

   if (!it->parent)
     {
        if (it->group && before->group)
          sd->group_items =
            eina_list_prepend_relative(sd->group_items, it, before);
     }
   else
     {
        it->parent->item->items =
          eina_list_prepend_relative(it->parent->item->items, EO_OBJ(it), eo_before);
     }
   sd->items = eina_inlist_prepend_relative
       (sd->items, EINA_INLIST_GET(it), EINA_INLIST_GET(before));

   it->item->rel = before;
   it->item->before = EINA_TRUE;
   _item_queue(sd, it, NULL);

   return EO_OBJ(it);
}

EOLIAN static Elm_Object_Item*
_elm_genlist_item_sorted_insert(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd, const Elm_Genlist_Item_Class *itc, const void *data, Elm_Object_Item *parent, Elm_Genlist_Item_Type type, Eina_Compare_Cb comp, Evas_Smart_Cb func, const void *func_data)
{
   Elm_Gen_Item *rel = NULL;
   Elm_Gen_Item *it;

   it = _elm_genlist_item_new
       (sd, itc, data, parent, type, func, func_data);
   if (!it) return NULL;
   Elm_Object_Item *eo_it = EO_OBJ(it);

   sd->item_compare_cb = comp;

   if (it->parent)
     {
        Elm_Object_Item *eo_rel = NULL;
        Eina_List *l;
        int cmp_result;

        l = eina_list_search_sorted_near_list
            (it->parent->item->items, _elm_genlist_eo_item_list_compare, eo_it,
            &cmp_result);

        if (l)
          {
             eo_rel = eina_list_data_get(l);
             rel = eo_data_scope_get(eo_rel, ELM_GENLIST_ITEM_CLASS);

             if (cmp_result >= 0)
               {
                  it->parent->item->items = eina_list_prepend_relative_list
                      (it->parent->item->items, eo_it, l);
                  sd->items = eina_inlist_prepend_relative
                      (sd->items, EINA_INLIST_GET(it), EINA_INLIST_GET(rel));
                  it->item->before = EINA_TRUE;
               }
             else if (cmp_result < 0)
               {
                  it->parent->item->items = eina_list_append_relative_list
                      (it->parent->item->items, eo_it, l);
                  sd->items = eina_inlist_append_relative
                      (sd->items, EINA_INLIST_GET(it), EINA_INLIST_GET(rel));
                  it->item->before = EINA_FALSE;
               }
          }
        else
          {
             rel = it->parent;

             // ignoring the comparison
             it->parent->item->items = eina_list_prepend_relative_list
                 (it->parent->item->items, eo_it, l);
             sd->items = eina_inlist_prepend_relative
                 (sd->items, EINA_INLIST_GET(it), EINA_INLIST_GET(rel));
             it->item->before = EINA_FALSE;
          }
     }
   else
     {
        if (!sd->state)
          {
             sd->state = eina_inlist_sorted_state_new();
             eina_inlist_sorted_state_init(sd->state, sd->items);
             sd->requeued = EINA_FALSE;
          }

        if (it->group)
          sd->group_items = eina_list_append(sd->group_items, it);

        sd->items = eina_inlist_sorted_state_insert
            (sd->items, EINA_INLIST_GET(it), _elm_genlist_item_compare,
            sd->state);

        if (EINA_INLIST_GET(it)->next)
          {
             rel = ELM_GEN_ITEM_FROM_INLIST(EINA_INLIST_GET(it)->next);
             it->item->before = EINA_TRUE;
          }
        else if (EINA_INLIST_GET(it)->prev)
          {
             rel = ELM_GEN_ITEM_FROM_INLIST(EINA_INLIST_GET(it)->prev);
             it->item->before = EINA_FALSE;
          }
     }

   if (rel)
     {
        it->item->rel = rel;
        rel->item->rel_revs = eina_list_append(rel->item->rel_revs, it);
     }

   _item_queue(sd, it, _elm_genlist_item_list_compare);

   return eo_it;
}

EOLIAN static void
_elm_genlist_clear(Eo *obj, Elm_Genlist_Data *sd EINA_UNUSED)
{
   _internal_elm_genlist_clear(obj, EINA_FALSE);
}

EOLIAN static void
_elm_genlist_multi_select_set(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd, Eina_Bool multi)
{
   sd->multi = !!multi;
}

EOLIAN static Eina_Bool
_elm_genlist_multi_select_get(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd)
{
   return sd->multi;
}

EOLIAN static void
_elm_genlist_multi_select_mode_set(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd, Elm_Object_Multi_Select_Mode mode)
{
   if (mode >= ELM_OBJECT_MULTI_SELECT_MODE_MAX)
     return;

   if (sd->multi_select_mode != mode)
     sd->multi_select_mode = mode;
}

EOLIAN static Elm_Object_Multi_Select_Mode
_elm_genlist_multi_select_mode_get(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd)
{
   return sd->multi_select_mode;
}

EOLIAN static Elm_Object_Item*
_elm_genlist_selected_item_get(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd)
{
   if (sd->selected)
      return sd->selected->data;
   else
      return NULL;
}

EOLIAN static const Eina_List*
_elm_genlist_selected_items_get(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd)
{
   return sd->selected;
}

EOLIAN static Eina_List*
_elm_genlist_realized_items_get(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd)
{
   Item_Block *itb;
   Eina_Bool done = EINA_FALSE;

   Eina_List *ret = NULL;

   EINA_INLIST_FOREACH(sd->blocks, itb)
     {
        if (itb->realized)
          {
             Eina_List *l;
             Elm_Gen_Item *it;

             done = EINA_TRUE;
             EINA_LIST_FOREACH(itb->items, l, it)
               {
                  if (it->realized) ret = eina_list_append(ret, EO_OBJ(it));
               }
          }
        else
          {
             if (done) break;
          }
     }

   return ret;
}

EOLIAN static Elm_Object_Item*
_elm_genlist_at_xy_item_get(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd, Evas_Coord x, Evas_Coord y, int *posret)
{
   Evas_Coord ox, oy, ow, oh;
   Evas_Coord lasty;
   Item_Block *itb;

   evas_object_geometry_get(sd->pan_obj, &ox, &oy, &ow, &oh);
   lasty = oy;
   EINA_INLIST_FOREACH(sd->blocks, itb)
     {
        Eina_List *l;
        Elm_Gen_Item *it;

        if (!ELM_RECTS_INTERSECT(ox + itb->x - itb->sd->pan_x,
                                 oy + itb->y - itb->sd->pan_y,
                                 itb->w, itb->h, x, y, 1, 1))
          continue;
        EINA_LIST_FOREACH(itb->items, l, it)
          {
             Evas_Coord itx, ity;

             itx = ox + itb->x + it->x - itb->sd->pan_x;
             ity = oy + itb->y + it->y - itb->sd->pan_y;
             if (ELM_RECTS_INTERSECT
                   (itx, ity, it->item->w, it->item->h, x, y, 1, 1))
               {
                  if (posret)
                    {
                       if (y <= (ity + (it->item->h / 4))) *posret = -1;
                       else if (y >= (ity + it->item->h - (it->item->h / 4)))
                         *posret = 1;
                       else *posret = 0;
                    }

                  return EO_OBJ(it);
               }
             lasty = ity + it->item->h;
          }
     }
   if (posret)
     {
        if (y > lasty) *posret = 1;
        else *posret = -1;
     }

   return NULL;
}

EOLIAN static Elm_Object_Item*
_elm_genlist_first_item_get(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd)
{
   Elm_Gen_Item *it;

   if (!sd->items) return NULL;

   it = ELM_GEN_ITEM_FROM_INLIST(sd->items);
   while ((it) && (it->generation < sd->generation))
     it = ELM_GEN_ITEM_FROM_INLIST(EINA_INLIST_GET(it)->next);

   return EO_OBJ(it);
}

EOLIAN static Elm_Object_Item*
_elm_genlist_last_item_get(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd)
{
   Elm_Gen_Item *it;

   if (!sd->items) return NULL;

   it = ELM_GEN_ITEM_FROM_INLIST(sd->items->last);
   while ((it) && (it->generation < sd->generation))
     it = ELM_GEN_ITEM_FROM_INLIST(EINA_INLIST_GET(it)->prev);

   return EO_OBJ(it);
}

EOLIAN static Elm_Object_Item *
_elm_genlist_item_next_get(Eo *eo_it EINA_UNUSED, Elm_Gen_Item *it)
{
   while (it)
     {
        it = ELM_GEN_ITEM_FROM_INLIST(EINA_INLIST_GET(it)->next);
        if ((it) && (it->generation == GL_IT(it)->wsd->generation)) break;
     }

   if (it) return EO_OBJ(it);
   else return NULL;
}

EOLIAN static Elm_Object_Item *
_elm_genlist_item_prev_get(Eo *eo_it EINA_UNUSED, Elm_Gen_Item *it)
{
   while (it)
     {
        it = ELM_GEN_ITEM_FROM_INLIST(EINA_INLIST_GET(it)->prev);
        if ((it) && (it->generation == GL_IT(it)->wsd->generation)) break;
     }

   if (it) return EO_OBJ(it);
   else return NULL;
}

EOLIAN static Elm_Object_Item *
_elm_genlist_item_parent_get(Eo *eo_it EINA_UNUSED, Elm_Gen_Item *it)
{
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it, NULL);

   return EO_OBJ(it->parent);
}

EOLIAN static unsigned int
_elm_genlist_item_subitems_count(Eo *eo_item EINA_UNUSED, Elm_Gen_Item *item)
{
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(item, 0);

   return eina_list_count(item->item->items);
}

EOLIAN static const Eina_List *
_elm_genlist_item_subitems_get(Eo *eo_item EINA_UNUSED, Elm_Gen_Item *item)
{
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(item, NULL);

   return item->item->items;
}

EOLIAN static void
_elm_genlist_item_subitems_clear(Eo *eo_item EINA_UNUSED, Elm_Gen_Item *it)
{
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it);
   ELM_GENLIST_DATA_GET(WIDGET(it), sd);

   if (!sd->tree_effect_enabled || !sd->move_effect_mode)
     _item_sub_items_clear(it);
   else
     {
        if (!sd->tree_effect_animator)
          {
             sd->expanded_item = it;
             _item_tree_effect_before(it);
             evas_object_stack_below(sd->event_block_rect, sd->stack[1]);
             evas_object_show(sd->event_block_rect);
             sd->start_time = ecore_time_get();
             sd->tree_effect_animator =
               ecore_animator_add(_tree_effect_animator_cb, sd->obj);
          }
        else
          _item_sub_items_clear(it);
     }
}

EOLIAN static void
_elm_genlist_item_selected_set(Eo *eo_item EINA_UNUSED, Elm_Gen_Item *it,
      Eina_Bool selected)
{
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it);
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   if ((it->generation < sd->generation) || eo_do(EO_OBJ(it), elm_wdg_item_disabled_get()))
     return;
   selected = !!selected;
   if (it->selected == selected) return;

   if (selected)
     {
        if (!sd->multi)
          {
             while (sd->selected)
               {
                  Elm_Object_Item *eo_sel = sd->selected->data;
                  ELM_GENLIST_ITEM_DATA_GET(eo_sel, sel);
                  if (it->unhighlight_cb)
                    it->unhighlight_cb(sel);
                  it->unsel_cb(sel);
               }
          }
        it->highlight_cb(it);
        it->sel_cb(it);

        return;
     }
   if (it->unhighlight_cb) it->unhighlight_cb(it);
   it->unsel_cb(it);
}

EOLIAN static Eina_Bool
_elm_genlist_item_selected_get(Eo *eo_item EINA_UNUSED, Elm_Gen_Item *it)
{
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it, EINA_FALSE);

   return it->selected;
}

static Elm_Gen_Item *
_elm_genlist_expanded_next_item_get(Elm_Gen_Item *it)
{
   Elm_Object_Item *eo_it = EO_OBJ(it);
   Elm_Object_Item *eo_it2;

   if (it->item->expanded)
     {
        eo_it2 = elm_genlist_item_next_get(eo_it);
        ELM_GENLIST_ITEM_DATA_GET(eo_it2, it2);
        return it2;
     }
   else
     {
        eo_it2 = elm_genlist_item_next_get(eo_it);
        while (eo_it2)
          {
             ELM_GENLIST_ITEM_DATA_GET(eo_it2, it2);
             if (it->item->expanded_depth >= it2->item->expanded_depth) return it2;
             eo_it2 = elm_genlist_item_next_get(eo_it2);
          }
     }
   return eo_data_scope_get(eo_it2, ELM_GENLIST_ITEM_CLASS);
}

static void
_elm_genlist_move_items_set(Elm_Gen_Item *it)
{
   Eina_List *l, *ll;
   Elm_Gen_Item *it2 = NULL;
   Evas_Coord ox, oy, ow, oh, dh = 0;
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   sd->expanded_next_item =
     _elm_genlist_expanded_next_item_get(it);

   if (it->item->expanded)
     {
        Elm_Object_Item *eo_item;
        l = elm_genlist_realized_items_get((sd)->obj);
        EINA_LIST_FREE(l, eo_item)
          {
             ELM_GENLIST_ITEM_DATA_GET(eo_item, item);
             sd->move_items = eina_list_append(sd->move_items, item);
          }

        EINA_LIST_FOREACH_SAFE(sd->move_items, l, ll, it2)
          {
             if (it2 == sd->expanded_next_item) break;
             sd->move_items = eina_list_remove(sd->move_items, it2);
          }
     }
   else
     {
        Elm_Object_Item *eo_it2 = NULL;
        evas_object_geometry_get(sd->pan_obj, &ox, &oy, &ow, &oh);
        if (sd->expanded_next_item) eo_it2 = EO_OBJ(sd->expanded_next_item);

        while (eo_it2 && (dh < oy + oh))
          {
             it2 = eo_data_scope_get(eo_it2, ELM_GENLIST_ITEM_CLASS);
             dh += it2->item->h;
             sd->move_items = eina_list_append(sd->move_items, it2);
             eo_it2 = elm_genlist_item_next_get(eo_it2);
          }
     }
}

static void
_event_block_rect_update(const Evas_Object *obj)
{
   Evas_Coord ox, oy, ow, oh;

   ELM_GENLIST_CHECK(obj);
   ELM_GENLIST_DATA_GET(obj, sd);

   if (!sd->event_block_rect)
     {
        sd->event_block_rect = evas_object_rectangle_add(
           evas_object_evas_get(sd->obj));
        evas_object_smart_member_add(sd->event_block_rect, sd->pan_obj);
     }

   evas_object_geometry_get(sd->pan_obj, &ox, &oy, &ow, &oh);
   evas_object_color_set(sd->event_block_rect, 0, 0, 0, 0);
   evas_object_resize(sd->event_block_rect, ow, oh);
   evas_object_move(sd->event_block_rect, ox, oy);
}

EOLIAN static void
_elm_genlist_item_expanded_set(Eo *eo_item EINA_UNUSED, Elm_Gen_Item *it, Eina_Bool expanded)
{
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it);
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   expanded = !!expanded;
   if (it->item->expanded == expanded) return;
   if (it->item->type != ELM_GENLIST_ITEM_TREE) return;
   it->item->expanded = expanded;
   sd->expanded_item = it;
   _elm_genlist_move_items_set(it);

   if (sd->tree_effect_enabled)
     _event_block_rect_update(WIDGET(it));

   if (it->item->expanded)
     {
        sd->move_effect_mode = ELM_GENLIST_TREE_EFFECT_EXPAND;
        if (it->realized)
          edje_object_signal_emit(VIEW(it), SIGNAL_EXPANDED, "elm");
        evas_object_smart_callback_call(WIDGET(it), SIG_EXPANDED, EO_OBJ(it));
        sd->auto_scroll_enabled = EINA_TRUE;
     }
   else
     {
        sd->move_effect_mode = ELM_GENLIST_TREE_EFFECT_CONTRACT;
        if (it->realized)
          edje_object_signal_emit(VIEW(it), SIGNAL_CONTRACTED, "elm");
        evas_object_smart_callback_call(WIDGET(it), SIG_CONTRACTED, EO_OBJ(it));
        sd->auto_scroll_enabled = EINA_FALSE;
     }
}

EOLIAN static Eina_Bool
_elm_genlist_item_expanded_get(Eo *eo_item EINA_UNUSED, Elm_Gen_Item *it)
{
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it, EINA_FALSE);

   return it->item->expanded;
}

EOLIAN static int
_elm_genlist_item_expanded_depth_get(Eo *eo_item EINA_UNUSED, Elm_Gen_Item *it)
{
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it, 0);

   return it->item->expanded_depth;
}

static Eina_Bool
_elm_genlist_item_coordinates_calc(Elm_Gen_Item *it,
                                   Elm_Genlist_Item_Scrollto_Type type,
                                   Eina_Bool bring_in,
                                   Evas_Coord *x,
                                   Evas_Coord *y,
                                   Evas_Coord *w,
                                   Evas_Coord *h)
{
   Evas_Coord gith = 0;
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   if (it->generation < sd->generation) return EINA_FALSE;
   if ((sd->queue) ||
       (!((sd->homogeneous) &&
          (sd->mode == ELM_LIST_COMPRESS))))
     {
        if ((it->item->queued) || (!it->item->mincalcd) || (sd->queue))
          {
             sd->show_item = it;
             sd->bring_in = bring_in;
             sd->scroll_to_type = type;
             it->item->show_me = EINA_TRUE;
             return EINA_FALSE;
          }
     }
   if (sd->show_item)
     {
        sd->show_item->item->show_me = EINA_FALSE;
        sd->show_item = NULL;
     }

   evas_object_geometry_get(sd->pan_obj, NULL, NULL, w, h);
   switch (type)
     {
      case ELM_GENLIST_ITEM_SCROLLTO_IN:
        if ((it->item->group_item) &&
            (sd->pan_y > (it->y + it->item->block->y)))
          gith = it->item->group_item->item->h;
        *h = it->item->h;
        *y = it->y + it->item->block->y - gith;
        break;

      case ELM_GENLIST_ITEM_SCROLLTO_TOP:
        if (it->item->group_item) gith = it->item->group_item->item->h;
        *y = it->y + it->item->block->y - gith;
        break;

      case ELM_GENLIST_ITEM_SCROLLTO_MIDDLE:
        *y = it->y + it->item->block->y - (*h / 2) + (it->item->h / 2);
        break;

      default:
        return EINA_FALSE;
     }

   *x = it->x + it->item->block->x;
   *w = it->item->block->w;

   return EINA_TRUE;
}

EOLIAN static void
_elm_genlist_item_promote(Eo *eo_it EINA_UNUSED, Elm_Gen_Item *it)
{
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it);

   if (it->generation < GL_IT(it)->wsd->generation) return;
   Elm_Object_Item *eo_first_item = elm_genlist_first_item_get(WIDGET(it));
   ELM_GENLIST_ITEM_DATA_GET(eo_first_item, first_item);
   _item_move_before(it, first_item);
}

EOLIAN static void
_elm_genlist_item_demote(Eo *eo_it EINA_UNUSED, Elm_Gen_Item *it)
{
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it);
   if (it->generation < GL_IT(it)->wsd->generation) return;
   Elm_Object_Item *eo_last_item = elm_genlist_last_item_get(WIDGET(it));
   ELM_GENLIST_ITEM_DATA_GET(eo_last_item, last_item);
   _item_move_after(it, last_item);
}

EOLIAN static void
_elm_genlist_item_show(Eo *eo_item EINA_UNUSED, Elm_Gen_Item *item, Elm_Genlist_Item_Scrollto_Type type)
{
   Evas_Coord x, y, w, h;

   ELM_GENLIST_ITEM_CHECK_OR_RETURN(item);

   if (_elm_genlist_item_coordinates_calc
         (item, type, EINA_FALSE, &x, &y, &w, &h))
      eo_do(WIDGET(item), elm_interface_scrollable_content_region_show
            (x, y, w, h));
}

EOLIAN static void
_elm_genlist_item_bring_in(Eo *eo_item EINA_UNUSED, Elm_Gen_Item *item, Elm_Genlist_Item_Scrollto_Type type)
{
   Evas_Coord x, y, w, h;

   ELM_GENLIST_ITEM_CHECK_OR_RETURN(item);

   if (_elm_genlist_item_coordinates_calc
         (item, type, EINA_TRUE, &x, &y, &w, &h))
      eo_do(WIDGET(item), elm_interface_scrollable_region_bring_in(x, y, w, h));
}

EOLIAN static void
_elm_genlist_item_all_contents_unset(Eo *eo_item EINA_UNUSED, Elm_Gen_Item *it, Eina_List **l)
{
   Evas_Object *content;

   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it);

   EINA_LIST_FREE(it->content_objs, content)
     {
        elm_widget_sub_object_del(WIDGET(it), content);
        evas_object_smart_member_del(content);
        evas_object_hide(content);
        if (l) *l = eina_list_append(*l, content);
     }
}

EOLIAN static void
_elm_genlist_item_update(Eo *eo_item EINA_UNUSED, Elm_Gen_Item *it)
{
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it);
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   if (!it->item->block) return;
   if (it->generation < sd->generation) return;
   it->item->mincalcd = EINA_FALSE;
   it->item->updateme = EINA_TRUE;
   it->item->block->updateme = EINA_TRUE;
   ecore_job_del(sd->update_job);
   sd->update_job = ecore_job_add(_update_job, sd->obj);
}

EOLIAN static void
_elm_genlist_item_fields_update(Eo *eo_item EINA_UNUSED, Elm_Gen_Item *it,
                               const char *parts,
                               Elm_Genlist_Item_Field_Type itf)
{
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it);

   if (!it->item->block) return;
   if (it->generation < GL_IT(it)->wsd->generation) return;

   if ((!itf) || (itf & ELM_GENLIST_ITEM_FIELD_TEXT))
     {
        _item_text_realize(it, VIEW(it), &it->texts, parts);
     }
   if ((!itf) || (itf & ELM_GENLIST_ITEM_FIELD_CONTENT))
     {
        it->content_objs = _item_content_unrealize(it, VIEW(it),
                                                   &it->contents, parts);
        it->content_objs = _item_content_realize(it, VIEW(it),
                                                 &it->contents, parts,
                                                 &it->content_objs);
        if (it->has_contents != (!!it->content_objs))
          it->item->mincalcd = EINA_FALSE;
        it->has_contents = !!it->content_objs;
        if (it->item->type == ELM_GENLIST_ITEM_NONE)
          {
             Evas_Object* eobj;
             Eina_List* l;
             it->item_focus_chain = eina_list_free(it->item_focus_chain);
             EINA_LIST_FOREACH(it->content_objs, l, eobj)
               if (elm_widget_is(eobj) && elm_object_focus_allow_get(eobj))
                 it->item_focus_chain = eina_list_append(it->item_focus_chain, eobj);
          }

        if (it->flipped)
          {
             it->item->flip_content_objs =
               _item_mode_content_unrealize(it, VIEW(it),
                                            &it->item->flip_contents, parts,
                                            &it->item->flip_content_objs);
             it->item->flip_content_objs =
                _item_content_realize(it, VIEW(it),
                                      &it->item->flip_contents, parts,
                                      &it->item->flip_content_objs);
          }
        if (it->item->deco_it_view)
          {
             it->item->deco_it_content_objs =
               _item_mode_content_unrealize(it, it->item->deco_it_view,
                                            &it->item->deco_it_contents, parts,
                                            &it->item->deco_it_content_objs);
             it->item->deco_it_content_objs =
                _item_content_realize(it, it->item->deco_it_view,
                                      &it->item->deco_it_contents, parts,
                                      &it->item->deco_it_content_objs);
          }
        if (GL_IT(it)->wsd->decorate_all_mode)
          {
             it->item->deco_all_content_objs =
               _item_mode_content_unrealize
                 (it, it->deco_all_view, &it->item->deco_all_contents, parts,
                 &it->item->deco_all_content_objs);
             it->item->deco_all_content_objs =
                _item_content_realize(it, it->deco_all_view,
                                      &it->item->deco_all_contents, parts,
                                      &it->item->deco_all_content_objs);
          }
     }

   if ((!itf) || (itf & ELM_GENLIST_ITEM_FIELD_STATE))
     _item_state_realize(it, VIEW(it), &it->states, parts);
   if (!it->item->mincalcd)
     elm_genlist_item_update(eo_item);
}

EOLIAN static void
_elm_genlist_item_item_class_update(Eo *eo_it, Elm_Gen_Item *it,
                                   const Elm_Genlist_Item_Class *itc)
{
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it);

   if (!it->item->block) return;
   EINA_SAFETY_ON_NULL_RETURN(itc);
   if (it->generation < GL_IT(it)->wsd->generation) return;
   it->itc = itc;
   it->item->nocache_once = EINA_TRUE;

   ELM_SAFE_FREE(it->texts, elm_widget_stringlist_free);
   ELM_SAFE_FREE(it->contents, elm_widget_stringlist_free);
   ELM_SAFE_FREE(it->states, elm_widget_stringlist_free);

   if (it->flipped)
     {
        elm_widget_stringlist_free(it->item->flip_contents);
        it->item->flip_contents = NULL;
     }
   if (it->item->deco_it_view)
     {
        elm_widget_stringlist_free(it->item->deco_it_texts);
        it->item->deco_it_texts = NULL;
        elm_widget_stringlist_free(it->item->deco_it_contents);
        it->item->deco_it_contents = NULL;
     }
   if (GL_IT(it)->wsd->decorate_all_mode)
     {
        elm_widget_stringlist_free(it->item->deco_all_texts);
        it->item->deco_all_texts = NULL;
        elm_widget_stringlist_free(it->item->deco_all_contents);
        it->item->deco_all_contents = NULL;
     }

   elm_genlist_item_update(eo_it);
}

EOLIAN static const Elm_Genlist_Item_Class *
_elm_genlist_item_item_class_get(Eo *eo_item EINA_UNUSED, Elm_Gen_Item *it)
{
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it, NULL);
   if (it->generation < GL_IT(it)->wsd->generation) return NULL;

   return it->itc;
}

static Evas_Object *
_elm_genlist_item_label_create(void *data,
                               Evas_Object *obj EINA_UNUSED,
                               Evas_Object *tooltip,
                               void *it EINA_UNUSED)
{
   Evas_Object *label = elm_label_add(tooltip);

   if (!label)
     return NULL;

   elm_object_style_set(label, "tooltip");
   elm_object_text_set(label, data);

   return label;
}

static void
_elm_genlist_item_label_del_cb(void *data,
                               Evas_Object *obj EINA_UNUSED,
                               void *event_info EINA_UNUSED)
{
   eina_stringshare_del(data);
}

EAPI void
elm_genlist_item_tooltip_text_set(Elm_Object_Item *it,
                                  const char *text)
{
   eo_do(it, elm_wdg_item_tooltip_text_set(text));
}

EOLIAN static void
_elm_genlist_item_elm_widget_item_tooltip_text_set(Eo *eo_it, Elm_Gen_Item *it, const char *text)
{
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it);

   text = eina_stringshare_add(text);
   elm_genlist_item_tooltip_content_cb_set
     (eo_it, _elm_genlist_item_label_create, text,
     _elm_genlist_item_label_del_cb);
}

EAPI void
elm_genlist_item_tooltip_content_cb_set(Elm_Object_Item *item,
                                        Elm_Tooltip_Item_Content_Cb func,
                                        const void *data,
                                        Evas_Smart_Cb del_cb)
{
   eo_do(item, elm_wdg_item_tooltip_content_cb_set(func, data, del_cb));
}

EOLIAN static void
_elm_genlist_item_elm_widget_item_tooltip_content_cb_set(Eo *eo_it, Elm_Gen_Item *it,
                                        Elm_Tooltip_Item_Content_Cb func,
                                        const void *data,
                                        Evas_Smart_Cb del_cb)
{
   ELM_GENLIST_ITEM_CHECK_OR_GOTO(it, error);

   if ((it->tooltip.content_cb != func) || (it->tooltip.data != data))
     {
        if (it->tooltip.del_cb)
           it->tooltip.del_cb((void *)it->tooltip.data, WIDGET(it), it);

        it->tooltip.content_cb = func;
        it->tooltip.data = data;
        it->tooltip.del_cb = del_cb;
     }

   if (VIEW(it))
     {
        eo_do_super(eo_it, ELM_GENLIST_ITEM_CLASS,
              elm_wdg_item_tooltip_content_cb_set
              (it->tooltip.content_cb, it->tooltip.data, NULL));
        eo_do(eo_it,
              elm_wdg_item_tooltip_style_set(it->tooltip.style),
              elm_wdg_item_tooltip_window_mode_set(it->tooltip.free_size));
     }

   return;

error:
   if (del_cb) del_cb((void *)data, NULL, NULL);
}

EAPI void
elm_genlist_item_tooltip_unset(Elm_Object_Item *item)
{
   eo_do(item, elm_wdg_item_tooltip_unset());
}

EOLIAN static void
_elm_genlist_item_elm_widget_item_tooltip_unset(Eo *eo_it, Elm_Gen_Item *it)
{
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it);

   if ((VIEW(it)) && (it->tooltip.content_cb))
     eo_do_super(eo_it, ELM_GENLIST_ITEM_CLASS,
           elm_wdg_item_tooltip_unset());

   if (it->tooltip.del_cb)
     it->tooltip.del_cb((void *)it->tooltip.data, WIDGET(it), it);
   it->tooltip.del_cb = NULL;
   it->tooltip.content_cb = NULL;
   it->tooltip.data = NULL;
   it->tooltip.free_size = EINA_FALSE;
   if (it->tooltip.style)
     eo_do(eo_it, elm_wdg_item_tooltip_style_set(NULL));
}

EAPI void
elm_genlist_item_tooltip_style_set(Elm_Object_Item *item,
                                   const char *style)
{
   eo_do(item, elm_wdg_item_tooltip_style_set(style));
}

EOLIAN static void
_elm_genlist_item_elm_widget_item_tooltip_style_set(Eo *eo_it, Elm_Gen_Item *it,
                                   const char *style)
{
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it);

   eina_stringshare_replace(&it->tooltip.style, style);
   if (VIEW(it)) eo_do_super(eo_it, ELM_GENLIST_ITEM_CLASS,
         elm_wdg_item_tooltip_style_set(style));
}

EAPI const char *
elm_genlist_item_tooltip_style_get(const Elm_Object_Item *it)
{
   return eo_do(it, elm_wdg_item_tooltip_style_get());
}

EAPI Eina_Bool
elm_genlist_item_tooltip_window_mode_set(Elm_Object_Item *item,
                                         Eina_Bool disable)
{
   return eo_do(item, elm_wdg_item_tooltip_window_mode_set(disable));
}

EOLIAN static Eina_Bool
_elm_genlist_item_elm_widget_item_tooltip_window_mode_set(Eo *eo_it, Elm_Gen_Item *it,
                                   Eina_Bool disable)
{
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it, EINA_FALSE);

   it->tooltip.free_size = disable;
   if (VIEW(it))
      return eo_do_super(eo_it, ELM_GENLIST_ITEM_CLASS,
            elm_wdg_item_tooltip_window_mode_set(disable));

   return EINA_TRUE;
}

EAPI Eina_Bool
elm_genlist_item_tooltip_window_mode_get(const Elm_Object_Item *eo_it)
{
   ELM_GENLIST_ITEM_DATA_GET(eo_it, it);
   return eo_do(VIEW(it), elm_wdg_item_tooltip_window_mode_get());
}

EAPI void
elm_genlist_item_cursor_set(Elm_Object_Item *item,
                            const char *cursor)
{
   eo_do(item, elm_wdg_item_cursor_set(cursor));
}

EOLIAN static void
_elm_genlist_item_elm_widget_item_cursor_set(Eo *eo_it, Elm_Gen_Item *it,
                            const char *cursor)
{
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it);
   eina_stringshare_replace(&it->mouse_cursor, cursor);
   if (VIEW(it)) eo_do_super(eo_it, ELM_GENLIST_ITEM_CLASS,
         elm_wdg_item_cursor_set(cursor));
}

EAPI const char *
elm_genlist_item_cursor_get(const Elm_Object_Item *eo_it)
{
   return eo_do(eo_it, elm_wdg_item_cursor_get());
}

EAPI void
elm_genlist_item_cursor_unset(Elm_Object_Item *item)
{
   eo_do(item, elm_wdg_item_cursor_unset());
}

EOLIAN static void
_elm_genlist_item_elm_widget_item_cursor_unset(Eo *eo_it, Elm_Gen_Item *it)
{
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it);

   if (!it->mouse_cursor) return;

   if (VIEW(it)) eo_do_super(eo_it, ELM_GENLIST_ITEM_CLASS,
         elm_wdg_item_cursor_unset());

   ELM_SAFE_FREE(it->mouse_cursor, eina_stringshare_del);
}

EAPI void
elm_genlist_item_cursor_style_set(Elm_Object_Item *eo_it,
                                  const char *style)
{
   eo_do(eo_it, elm_wdg_item_cursor_style_set(style));
}

EAPI const char *
elm_genlist_item_cursor_style_get(const Elm_Object_Item *eo_it)
{
   return eo_do( eo_it, elm_wdg_item_cursor_style_get());
}

EAPI void
elm_genlist_item_cursor_engine_only_set(Elm_Object_Item *eo_it,
                                        Eina_Bool engine_only)
{
   eo_do(eo_it, elm_wdg_item_cursor_engine_only_set(engine_only));
}

EAPI Eina_Bool
elm_genlist_item_cursor_engine_only_get(const Elm_Object_Item *eo_it)
{
   return eo_do( eo_it, elm_wdg_item_cursor_engine_only_get());
}

EOLIAN static int
_elm_genlist_item_index_get(Eo *eo_it EINA_UNUSED, Elm_Gen_Item *it)
{
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it, -1);

   if (it->item->block)
     return it->position + (it->item->block->position *
                            GL_IT(it)->wsd->max_items_per_block);
   return -1;
}

EOLIAN static void
_elm_genlist_mode_set(Eo *obj, Elm_Genlist_Data *sd, Elm_List_Mode mode)
{
   if (sd->mode == mode) return;
   sd->mode = mode;
   elm_layout_sizing_eval(obj);
}

EOLIAN static Elm_List_Mode
_elm_genlist_mode_get(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd)
{
   return sd->mode;
}

EAPI void
elm_genlist_bounce_set(Evas_Object *obj,
                       Eina_Bool h_bounce,
                       Eina_Bool v_bounce)
{
   ELM_GENLIST_CHECK(obj);
   eo_do(obj, elm_interface_scrollable_bounce_allow_set(h_bounce, v_bounce));
}

EOLIAN static void
_elm_genlist_elm_interface_scrollable_bounce_allow_set(Eo *obj, Elm_Genlist_Data *sd, Eina_Bool h_bounce, Eina_Bool v_bounce)
{
   sd->h_bounce = !!h_bounce;
   sd->v_bounce = !!v_bounce;
   eo_do_super(obj, MY_CLASS, elm_interface_scrollable_bounce_allow_set
         (sd->h_bounce, sd->v_bounce));
}

EAPI void
elm_genlist_bounce_get(const Evas_Object *obj,
                       Eina_Bool *h_bounce,
                       Eina_Bool *v_bounce)
{
   ELM_GENLIST_CHECK(obj);
   eo_do( obj, elm_interface_scrollable_bounce_allow_get
         (h_bounce, v_bounce));
}

EOLIAN static void
_elm_genlist_elm_interface_scrollable_bounce_allow_get(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd, Eina_Bool *h_bounce, Eina_Bool *v_bounce)
{
   if (h_bounce) *h_bounce = sd->h_bounce;
   if (v_bounce) *v_bounce = sd->v_bounce;
}

EOLIAN static void
_elm_genlist_homogeneous_set(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd, Eina_Bool homogeneous)
{
   sd->homogeneous = !!homogeneous;
}

EOLIAN static Eina_Bool
_elm_genlist_homogeneous_get(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd)
{
   return sd->homogeneous;
}

EOLIAN static void
_elm_genlist_block_count_set(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd, int count)
{
   EINA_SAFETY_ON_TRUE_RETURN(count < 1);

   sd->max_items_per_block = count;
   sd->item_cache_max = sd->max_items_per_block * 2;
   _item_cache_clean(sd);
}

EOLIAN static int
_elm_genlist_block_count_get(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd)
{
   return sd->max_items_per_block;
}

EOLIAN static void
_elm_genlist_longpress_timeout_set(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd, double timeout)
{
   sd->longpress_timeout = timeout;
}

EOLIAN static double
_elm_genlist_longpress_timeout_get(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd)
{
   return sd->longpress_timeout;
}

EAPI void
elm_genlist_scroller_policy_set(Evas_Object *obj,
                                Elm_Scroller_Policy policy_h,
                                Elm_Scroller_Policy policy_v)
{
   ELM_GENLIST_CHECK(obj);
   eo_do(obj, elm_interface_scrollable_policy_set(policy_h, policy_v));
}

EOLIAN static void
_elm_genlist_elm_interface_scrollable_policy_set(Eo *obj, Elm_Genlist_Data *sd EINA_UNUSED, Elm_Scroller_Policy policy_h, Elm_Scroller_Policy policy_v)
{
   if ((policy_h >= ELM_SCROLLER_POLICY_LAST) ||
       (policy_v >= ELM_SCROLLER_POLICY_LAST))
     return;

   eo_do_super(obj, MY_CLASS, elm_interface_scrollable_policy_set(policy_h, policy_v));
}

EAPI void
elm_genlist_scroller_policy_get(const Evas_Object *obj,
                                Elm_Scroller_Policy *policy_h,
                                Elm_Scroller_Policy *policy_v)
{
   ELM_GENLIST_CHECK(obj);
   eo_do( obj, elm_interface_scrollable_policy_get(policy_h, policy_v));
}

EOLIAN static void
_elm_genlist_elm_interface_scrollable_policy_get(Eo *obj, Elm_Genlist_Data *sd EINA_UNUSED, Elm_Scroller_Policy *policy_h, Elm_Scroller_Policy *policy_v)
{
   Elm_Scroller_Policy s_policy_h, s_policy_v;

   eo_do_super(obj, MY_CLASS, elm_interface_scrollable_policy_get
         (&s_policy_h, &s_policy_v));
   if (policy_h) *policy_h = (Elm_Scroller_Policy)s_policy_h;
   if (policy_v) *policy_v = (Elm_Scroller_Policy)s_policy_v;
}

EOLIAN static void
_elm_genlist_realized_items_update(Eo *obj, Elm_Genlist_Data *_pd EINA_UNUSED)
{
   Eina_List *list;
   Elm_Object_Item *it;

   list = elm_genlist_realized_items_get(obj);
   EINA_LIST_FREE(list, it)
     elm_genlist_item_update(it);
}

EOLIAN static void
_elm_genlist_item_decorate_mode_set(Eo *eo_it EINA_UNUSED, Elm_Gen_Item *it,
                                   const char *decorate_it_type,
                                   Eina_Bool decorate_it_set)
{
   Elm_Genlist_Data *sd;
   Eina_List *l;
   Elm_Object_Item *eo_it2 = NULL;

   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it);
   sd = GL_IT(it)->wsd;

   if (!decorate_it_type) return;
   if ((it->generation < sd->generation) ||
       eo_do(eo_it, elm_wdg_item_disabled_get())) return;
   if (sd->decorate_all_mode) return;

   if ((sd->mode_item == it) &&
       (!strcmp(decorate_it_type, sd->decorate_it_type)) &&
       (decorate_it_set))
     return;
   if (!it->itc->decorate_item_style) return;
   it->decorate_it_set = decorate_it_set;

   if (sd->multi)
     {
        EINA_LIST_FOREACH(sd->selected, l, eo_it2)
          {
             ELM_GENLIST_ITEM_DATA_GET(eo_it2, it2);
             if (it2->realized)
                elm_genlist_item_selected_set(eo_it2, EINA_FALSE);
          }
     }
   else
     {
        Elm_Gen_Item *it2 = NULL;
        eo_it2 = elm_genlist_selected_item_get(sd->obj);
        if (eo_it2) it2 = eo_data_scope_get(eo_it2, ELM_GENLIST_ITEM_CLASS);
        if (it2 && it2->realized)
          elm_genlist_item_selected_set(eo_it2, EINA_FALSE);
     }

   if (((sd->decorate_it_type)
        && (strcmp(decorate_it_type, sd->decorate_it_type))) ||
       (decorate_it_set) || ((it == sd->mode_item) && (!decorate_it_set)))
     _decorate_item_unset(sd);

   eina_stringshare_replace(&sd->decorate_it_type, decorate_it_type);
   if (decorate_it_set) _decorate_item_set(it);
}

EOLIAN static const char *
_elm_genlist_item_decorate_mode_get(Eo *eo_i EINA_UNUSED, Elm_Gen_Item *i)
{
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(i, NULL);
   return GL_IT(i)->wsd->decorate_it_type;
}

EOLIAN static Elm_Object_Item *
_elm_genlist_decorated_item_get(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd)
{
   return EO_OBJ(sd->mode_item);
}

EOLIAN static Eina_Bool
_elm_genlist_decorate_mode_get(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd)
{
   return sd->decorate_all_mode;
}

EOLIAN static void
_elm_genlist_decorate_mode_set(Eo *obj, Elm_Genlist_Data *sd, Eina_Bool decorated)
{
   Elm_Object_Item *eo_it;
   Eina_List *list;
   Elm_Object_Item *deco_it;

   decorated = !!decorated;
   if (sd->decorate_all_mode == decorated) return;
   sd->decorate_all_mode = decorated;

   ELM_SAFE_FREE(sd->tree_effect_animator, ecore_animator_del);
   sd->move_effect_mode = ELM_GENLIST_TREE_EFFECT_NONE;

   list = elm_genlist_realized_items_get(obj);
   if (!sd->decorate_all_mode)
     {
        EINA_LIST_FREE(list, eo_it)
          {
             ELM_GENLIST_ITEM_DATA_GET(eo_it, it);
             if (it->item->type != ELM_GENLIST_ITEM_GROUP)
               _decorate_all_item_unrealize(it);
          }
        _item_cache_zero(sd);
     }
   else
     {
        // unset decorated item
        deco_it = elm_genlist_decorated_item_get(obj);
        if (deco_it)
          {
             elm_genlist_item_decorate_mode_set
               (deco_it, elm_genlist_item_decorate_mode_get
                 (deco_it), EINA_FALSE);
             _decorate_item_finished_signal_cb(deco_it, obj, NULL, NULL);
          }

        EINA_LIST_FREE(list, eo_it)
          {
             ELM_GENLIST_ITEM_DATA_GET(eo_it, it);
             if (it->item->type != ELM_GENLIST_ITEM_GROUP)
               {
                  if (it->itc->decorate_all_item_style)
                    _decorate_all_item_realize(it, EINA_TRUE);
               }
          }
     }

   ecore_job_del(sd->calc_job);
   sd->calc_job = ecore_job_add(_calc_job, sd->obj);
}

EOLIAN static void
_elm_genlist_reorder_mode_set(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd, Eina_Bool reorder_mode)
{
   Eina_List *realized;
   Elm_Object_Item *eo_it;

   if (sd->reorder_mode == !!reorder_mode) return;
   sd->reorder_mode = !!reorder_mode;
   realized = elm_genlist_realized_items_get(obj);
   EINA_LIST_FREE(realized, eo_it)
    {
       ELM_GENLIST_ITEM_DATA_GET(eo_it, it);
       if (it->item->type != ELM_GENLIST_ITEM_GROUP)
         {
            Evas_Object *view;
            if (it->deco_all_view) view = it->deco_all_view;
            else view = VIEW(it);

            if (sd->reorder_mode)
              edje_object_signal_emit(view, SIGNAL_REORDER_MODE_SET, "elm");
            else
              edje_object_signal_emit(view, SIGNAL_REORDER_MODE_UNSET, "elm");
        }
   }
}

EAPI void
elm_genlist_reorder_mode_start(Evas_Object *obj, Ecore_Pos_Map tween_mode)
{
   ELM_GENLIST_CHECK(obj);
   ELM_GENLIST_DATA_GET(obj, sd);

   sd->reorder.tween_mode = tween_mode;
   _elm_genlist_reorder_mode_set(obj, sd, EINA_TRUE);
}

EAPI void
elm_genlist_reorder_mode_stop(Evas_Object *obj)
{
   ELM_GENLIST_CHECK(obj);
   ELM_GENLIST_DATA_GET(obj, sd);

   _elm_genlist_reorder_mode_set(obj, sd, EINA_FALSE);
}

EOLIAN static Eina_Bool
_elm_genlist_reorder_mode_get(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd)
{
   return sd->reorder_mode;
}

EOLIAN static Elm_Genlist_Item_Type
_elm_genlist_item_type_get(Eo *eo_it EINA_UNUSED, Elm_Gen_Item *it)
{
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it, ELM_GENLIST_ITEM_MAX);

   return it->item->type;
}

EAPI Elm_Genlist_Item_Class *
elm_genlist_item_class_new(void)
{
   Elm_Genlist_Item_Class *itc;

   itc = calloc(1, sizeof(Elm_Genlist_Item_Class));
   if (!itc)
     return NULL;

   itc->version = CLASS_ALLOCATED;
   itc->refcount = 1;
   itc->delete_me = EINA_FALSE;

   return itc;
}

EAPI void
elm_genlist_item_class_free(Elm_Genlist_Item_Class *itc)
{
   if (itc && (itc->version == CLASS_ALLOCATED))
     {
        if (!itc->delete_me) itc->delete_me = EINA_TRUE;
        if (itc->refcount > 0) elm_genlist_item_class_unref(itc);
        else
          {
             itc->version = 0;
             free(itc);
          }
     }
}

EAPI void
elm_genlist_item_class_ref(Elm_Genlist_Item_Class *itc)
{
   if (itc && (itc->version == CLASS_ALLOCATED))
     {
        itc->refcount++;
        if (itc->refcount == 0) itc->refcount--;
     }
}

EAPI void
elm_genlist_item_class_unref(Elm_Genlist_Item_Class *itc)
{
   if (itc && (itc->version == CLASS_ALLOCATED))
     {
        if (itc->refcount > 0) itc->refcount--;
        if (itc->delete_me && (!itc->refcount))
          elm_genlist_item_class_free(itc);
     }
}

static void
_flip_job(void *data)
{
   Elm_Gen_Item *it = (Elm_Gen_Item *)data;
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   it->unsel_cb(it);
   _elm_genlist_item_unrealize(it, EINA_FALSE);

   it->flipped = EINA_TRUE;
   it->item->nocache = EINA_TRUE;
   ecore_job_del(sd->calc_job);
   sd->calc_job = ecore_job_add(_calc_job, sd->obj);
}

EOLIAN static void
_elm_genlist_item_flip_set(Eo *eo_it, Elm_Gen_Item *it, Eina_Bool flip)
{
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it);

   Eina_Bool prev_flip = it->flipped;

   flip = !!flip;
   if (it->flipped == flip) return;

   if (flip)
     {
        ecore_job_add(_flip_job, it);
     }
   else
     {
        if (prev_flip != flip)
          {
             edje_object_signal_emit(VIEW(it), SIGNAL_FLIP_DISABLED, "elm");
             if (GL_IT(it)->wsd->decorate_all_mode)
               edje_object_signal_emit(it->deco_all_view, SIGNAL_FLIP_DISABLED,
                                       "elm");
          }

        it->flipped = flip;
        _item_cache_zero(GL_IT(it)->wsd);
        elm_genlist_item_update(eo_it);
        it->item->nocache = EINA_FALSE;
     }
}

EOLIAN static Eina_Bool
_elm_genlist_item_flip_get(Eo *eo_it EINA_UNUSED, Elm_Gen_Item *it)
{
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it, EINA_FALSE);

   return it->flipped;
}

EOLIAN static void
_elm_genlist_select_mode_set(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd, Elm_Object_Select_Mode mode)
{
   if (mode >= ELM_OBJECT_SELECT_MODE_MAX)
     return;

   if (sd->select_mode != mode)
     sd->select_mode = mode;
}

EOLIAN static Elm_Object_Select_Mode
_elm_genlist_select_mode_get(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd)
{
   return sd->select_mode;
}

EOLIAN static void
_elm_genlist_highlight_mode_set(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd, Eina_Bool highlight)
{
   sd->highlight = !!highlight;
}

EOLIAN static Eina_Bool
_elm_genlist_highlight_mode_get(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd)
{
   return sd->highlight;
}

EOLIAN static void
_elm_genlist_item_select_mode_set(Eo *eo_it EINA_UNUSED, Elm_Gen_Item *it,
                                 Elm_Object_Select_Mode mode)
{
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it);
   ELM_GENLIST_DATA_GET_FROM_ITEM(it, sd);

   if (it->generation < sd->generation) return;
   if (mode >= ELM_OBJECT_SELECT_MODE_MAX)
     return;
   if (it->select_mode != mode)
     it->select_mode = mode;

   if (it->select_mode == ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY)
     {
        it->item->mincalcd = EINA_FALSE;
        it->item->updateme = EINA_TRUE;
        if (it->item->block) it->item->block->updateme = EINA_TRUE;
        ecore_job_del(sd->update_job);
        sd->update_job = ecore_job_add(_update_job, sd->obj);

        // reset homogeneous item size
        if (sd->homogeneous)
          {
             if (it->group)
               sd->group_item_width = sd->group_item_height = 0;
             else
               sd->item_width = sd->item_height = 0;
          }
     }
}

EOLIAN static Elm_Object_Select_Mode
_elm_genlist_item_select_mode_get(Eo *eo_it EINA_UNUSED, Elm_Gen_Item *it)
{
   ELM_GENLIST_ITEM_CHECK_OR_RETURN(it, ELM_OBJECT_SELECT_MODE_MAX);

   return it->select_mode;
}

EOLIAN static void
_elm_genlist_tree_effect_enabled_set(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd, Eina_Bool enabled)
{
   sd->tree_effect_enabled = !!enabled;
}

EOLIAN static Eina_Bool
_elm_genlist_tree_effect_enabled_get(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd)
{
   return sd->tree_effect_enabled;
}

EOLIAN static void
_elm_genlist_focus_on_selection_set(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd, Eina_Bool enabled)
{
   sd->focus_on_selection_enabled = !!enabled;
}

EOLIAN static Eina_Bool
_elm_genlist_focus_on_selection_get(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd)
{
   return sd->focus_on_selection_enabled;
}

EAPI Elm_Object_Item *
elm_genlist_nth_item_get(const Evas_Object *obj, unsigned int nth)
{
   Elm_Gen_Item *it = NULL;
   Eina_Accessor *a;
   void *data;

   ELM_GENLIST_CHECK(obj) NULL;
   ELM_GENLIST_DATA_GET(obj, sd);

   if (!sd->items) return NULL;

   a = eina_inlist_accessor_new(sd->items);
   if (!a) return NULL;
   if (eina_accessor_data_get(a, nth, &data))
     it = ELM_GEN_ITEM_FROM_INLIST(data);
   eina_accessor_free(a);
   return EO_OBJ(it);
}

EOLIAN static void
_elm_genlist_elm_widget_focus_highlight_geometry_get(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h)
{
   Evas_Coord ox, oy, oh, ow, item_x = 0, item_y = 0, item_w = 0, item_h = 0;

   evas_object_geometry_get(sd->pan_obj, &ox, &oy, &ow, &oh);

   if (sd->focused_item)
     {
        ELM_GENLIST_ITEM_DATA_GET(sd->focused_item, focus_it);
        evas_object_geometry_get(VIEW(focus_it), &item_x, &item_y, &item_w, &item_h);
        elm_widget_focus_highlight_focus_part_geometry_get(VIEW(focus_it), &item_x, &item_y, &item_w, &item_h);
     }

   *x = item_x;
   *y = item_y;
   *w = item_w;
   *h = item_h;

   if (item_y < oy)
     {
        *y = oy;
     }
   if (item_y > (oy + oh - item_h))
     {
        *y = oy + oh - item_h;
     }

   if ((item_x + item_w) > (ox + ow))
     {
        *w = ow;
     }
   if (item_x < ox)
     {
        *x = ox;
     }
}

EOLIAN static Elm_Object_Item *
_elm_genlist_search_by_text_item_get(Eo *obj EINA_UNUSED,
                                     Elm_Genlist_Data *sd,
                                     Elm_Object_Item *item_to_search_from,
                                     const char *part_name,
                                     const char *pattern,
                                     Elm_Glob_Match_Flags flags)
{
   Elm_Gen_Item *it = NULL;
   char *str = NULL;
   Eina_Inlist *start = NULL;
   int fnflags = 0;

   if (!pattern) return NULL;
   if (!sd->items) return NULL;

   if (flags & ELM_GLOB_MATCH_NO_ESCAPE) fnflags |= FNM_NOESCAPE;
   if (flags & ELM_GLOB_MATCH_PATH) fnflags |= FNM_PATHNAME;
   if (flags & ELM_GLOB_MATCH_PERIOD) fnflags |= FNM_PERIOD;
#ifdef FNM_CASEFOLD
   if (flags & ELM_GLOB_MATCH_NOCASE) fnflags |= FNM_CASEFOLD;
#endif

   start = (item_to_search_from) ?
   EINA_INLIST_GET((Elm_Gen_Item *)eo_data_scope_get(item_to_search_from, ELM_GENLIST_ITEM_CLASS)) :
   sd->items;
   EINA_INLIST_FOREACH(start, it)
     {
        if (!it->itc->func.text_get) continue;
        str = it->itc->func.text_get((void *)WIDGET_ITEM_DATA_GET(EO_OBJ(it)), WIDGET(it), part_name);
        if (!str) continue;
        if (!fnmatch(pattern, str, fnflags))
          {
             free(str);
             return EO_OBJ(it);
          }
        free(str);
     }
   return NULL;
}

EOLIAN static Elm_Object_Item*
_elm_genlist_elm_widget_focused_item_get(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd)
{
   return sd->focused_item;
}

EOLIAN static void
_elm_genlist_elm_widget_item_loop_enabled_set(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd, Eina_Bool enable)
{
   if (enable == EINA_TRUE)
      {
         _elm_widget_focus_highlight_signal_callback_add(obj, "elm,focus,move,down,end",
                                                         "elm", _elm_genlist_focus_highlight_move_down_end_cb, obj);
         _elm_widget_focus_highlight_signal_callback_add(obj, "elm,focus,move,up,end",
                                                         "elm", _elm_genlist_focus_highlight_move_up_end_cb, obj);
      }
   else
     {
        _elm_widget_focus_highlight_signal_callback_del(obj, "elm,focus,move,down,end",
                                                        "elm", _elm_genlist_focus_highlight_move_down_end_cb);
        _elm_widget_focus_highlight_signal_callback_del(obj, "elm,focus,move,up,end",
                                                        "elm", _elm_genlist_focus_highlight_move_up_end_cb);
     }

   sd->item_loop_enable = !!enable;
}

EOLIAN static Eina_Bool
_elm_genlist_elm_widget_item_loop_enabled_get(Eo *obj EINA_UNUSED, Elm_Genlist_Data *sd)
{
   return sd->item_loop_enable;
}

EOLIAN static void
_elm_genlist_class_constructor(Eo_Class *klass)
{
   if (_elm_config->access_mode)
      _elm_genlist_smart_focus_next_enable = EINA_TRUE;

   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

EOLIAN const Elm_Atspi_Action *
_elm_genlist_elm_interface_atspi_widget_action_elm_actions_get(Eo *obj EINA_UNUSED, Elm_Genlist_Data *pd EINA_UNUSED)
{
   static Elm_Atspi_Action atspi_actions[] = {
          { "move,prior", "move", "prior", _key_action_move},
          { "move,next", "move", "next", _key_action_move},
          { "move,left", "move", "left", _key_action_move},
          { "move,right", "move", "right", _key_action_move},
          { "move,up", "move", "up", _key_action_move},
          { "move,up,multi", "move", "up_multi", _key_action_move},
          { "move,down", "move", "down", _key_action_move},
          { "move,down,multi", "move", "down_multi", _key_action_move},
          { "move,first", "move", "first", _key_action_move},
          { "move,last", "move", "last", _key_action_move},
          { "select", "select", NULL, _key_action_select},
          { "escape", "escape", NULL, _key_action_escape},
          { NULL, NULL, NULL, NULL }
   };
   return &atspi_actions[0];
}

#include "elm_genlist.eo.c"
#include "elm_genlist_item.eo.c"
