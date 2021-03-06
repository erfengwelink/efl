#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_ACCESS_BETA
#include <Elementary.h>
#include "elm_suite.h"


START_TEST (elm_atspi_role_get)
{
   Evas_Object *win, *idx;
   Efl_Access_Role role;

   elm_init(1, NULL);
   win = elm_win_add(NULL, "index", ELM_WIN_BASIC);

   idx = elm_index_add(win);
   role = efl_access_role_get(idx);

   ck_assert(role == EFL_ACCESS_ROLE_SCROLL_BAR);

   elm_shutdown();
}
END_TEST

void elm_test_index(TCase *tc)
{
 tcase_add_test(tc, elm_atspi_role_get);
}
