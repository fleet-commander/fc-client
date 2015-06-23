#include <glib.h>
#include <pwd.h>

/* Returns a NULL terminated list of group names */
GList* get_groups_for_user (uid_t);
char*  uid_to_user_name    (uid_t);
