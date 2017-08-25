#ifndef EFL_UI_GRIDVIEW_PRIVATE_H
#define EFL_UI_GRIDVIEW_PRIVATE_H

#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define ELM_INTERFACE_ATSPI_ACCESSIBLE_PROTECTED
#define ELM_INTERFACE_ATSPI_SELECTION_PROTECTED
#define ELM_INTERFACE_ATSPI_WIDGET_ACTION_PROTECTED

#include <Elementary.h>
#include "elm_priv.h"

typedef struct _Efl_Ui_Gridview_Item Efl_Ui_Gridview_Item;
typedef struct _GV_Item_Line GV_Item_Line;

struct _Efl_Ui_Gridview_Item
{
   unsigned int                 index;
   Eo                           *widget;
   Efl_Model                    *model;
   Elm_Layout                   *layout;
   Efl_Future                   *future;
   GV_Item_Line                 *line;

   Eina_Rectangle               geo;
   int                          minw, minh;
   double                       wx, wy;
   Eina_Bool                    down: 1;
   Eina_Bool                    selected: 1;
   Eina_Bool                    longpressed : 1;
   Efl_Loop_Timer               *long_timer;

   int                          matrix[2]; // 0 :row,1 :col
};

struct _GV_Item_Line
{
   EINA_INLIST;
   Eina_List                    *items; //neccesary??
   int                          matrix[2]; // 0 :row,1 :col
   int                          maxw, maxh;
   Eina_Rectangle               geo;
};

typedef struct _Efl_Ui_Gridview_Data Efl_Ui_Gridview_Data;

struct _Efl_Ui_Gridview_Data
{
   Eo                           *obj;
   Evas_Object                  *hit_rect;
   Efl_Model                    *model;
   Efl_Orient                   orient; //FIXME: Efl_Direction

   struct {
      double                    h, v;
      Eina_Bool                 scalable: 1;
   } pad;
   struct {
      double                    h, v;
   } align;
  struct {
      double                    x, y;
   } weight;

   struct {
      Evas_Coord                w, h;
      int                       start;
      int                       slice;
   } realized;
   struct {
      int                       slice_start;
      int                       slice;
   } outstanding_slice;

   struct {
      Evas_Coord                x, y, move_diff;
      Evas_Object               *obj;
   } pan;

   struct {
      Eina_Inarray              array;
      unsigned int              count;
   } items;

   struct {
      GV_Item_Line              *first, *end;
      int                       count;
      int                       mean;
      double                    align_h, align_v;
   } lines;

   Efl_Ui_Layout_Factory        *factory;
   Efl_Ui_Focus_Manager         *manager;
   Efl_Ui_Gridview_Item         *focused;

   Eina_Stringshare             *style;
   Efl_Future                   *future;
   Eina_List                    *selected_items;

   Elm_Object_Select_Mode       select_mode;
   Elm_List_Mode                mode;
   int                          minw, minh;

   struct {
   Eina_Bool                    homogeneous : 1;
   Eina_Bool                    hold : 1;
   Eina_Bool                    load : 1;
   } on;

   struct {
   Eina_Bool                    recalc : 1;
   Eina_Bool                    update : 1;
   Eina_Bool                    reload : 1;
   } need;
};

typedef struct _Efl_Ui_Gridview_Pan_Data Efl_Ui_Gridview_Pan_Data;

struct _Efl_Ui_Gridview_Pan_Data
{
   Eo                     *wobj;
   Efl_Ui_Gridview_Data   *wpd;
   Ecore_Job              *resize_job;
};

typedef struct _Efl_Ui_Gridview_Slice Efl_Ui_Gridview_Slice;

struct _Efl_Ui_Gridview_Slice
{
   Efl_Ui_Gridview_Data       *pd;
   int                    newstart, slicestart, newslice;
};



#define EFL_UI_GRIDVIEW_DATA_GET(o, ptr) \
  Efl_Ui_Gridview_Data * ptr = efl_data_scope_get(o, EFL_UI_GRIDVIEW_CLASS)

#define EFL_UI_GRIDVIEW_DATA_GET_OR_RETURN(o, ptr)       \
  EFL_UI_GRIDVIEW_DATA_GET(o, ptr);                      \
  if (EINA_UNLIKELY(!ptr))                           \
    {                                                \
       CRI("No widget data for object %p (%s)",      \
           o, evas_object_type_get(o));              \
       return;                                       \
    }

#define EFL_UI_GRIDVIEW_DATA_GET_OR_RETURN_VAL(o, ptr, val) \
  EFL_UI_GRIDVIEW_DATA_GET(o, ptr);                         \
  if (EINA_UNLIKELY(!ptr))                              \
    {                                                   \
       CRI("No widget data for object %p (%s)",         \
           o, evas_object_type_get(o));                 \
       return val;                                      \
    }

#endif
