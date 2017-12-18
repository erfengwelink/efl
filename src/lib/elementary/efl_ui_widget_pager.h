#ifndef EFL_UI_WIDGET_PAGER_H
#define EFL_UI_WIDGET_PAGER_H


#include <Elementary.h>

#include "efl_page_transition.h"

typedef struct _Efl_Ui_Pager_Data
{
   Eina_List               *page_infos;
   Eina_List               *content_list;

   Evas_Object             *event;
   Ecore_Animator          *animator;
   Ecore_Job               *job;
   Ecore_Job               *page_info_job;

   Evas_Coord               x, y, w, h;
   Evas_Coord               mouse_x, mouse_y;

   struct {
      Eina_Size2D           sz;
      Evas_Coord            padding;
   } page_spec;

   struct {
      Evas_Coord            x, y;
      int                   page;
      double                pos;
      Eina_Bool             enabled;
   } down;

   struct {
      int                   page;
      double                pos;
   } curr;

   struct {
      Ecore_Animator       *animator;
      double                src;
      double                dst;
      double                delta;
      Eina_Bool             jump;
   } change;

   int                      cnt;

   Efl_Ui_Pager_Loop        loop;
   Efl_Page_Transition     *transition;

   Eina_Bool                move_started : 1;
   Eina_Bool                prev_block : 1;
   Eina_Bool                next_block: 1;

} Efl_Ui_Pager_Data;

#define EFL_UI_PAGER_DATA_GET(o, sd) \
   Efl_Ui_Pager_Data *sd = efl_data_scope_get(o, EFL_UI_PAGER_CLASS)

#endif
