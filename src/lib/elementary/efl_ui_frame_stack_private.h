#ifndef EFL_UI_WIDGET_FRAME_STACK_H
#define EFL_UI_WIDGET_FRAME_STACK_H

typedef struct _Efl_Ui_Frame_Stack_Data Efl_Ui_Frame_Stack_Data;
struct _Efl_Ui_Frame_Stack_Data
{
   Eina_Inlist *stack; /* the last item is the top item */
};

typedef struct _Frame_Data Frame_Data;
struct _Frame_Data
{
   EINA_INLIST;

   Eo          *frame;
};

#endif
