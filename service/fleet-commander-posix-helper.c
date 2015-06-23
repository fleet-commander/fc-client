#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include <grp.h>
#include <pwd.h>

#include "fleet-commander-posix-helper.h"

#define DEFAULT_LENGHT 100

GList *
get_groups_for_user (uid_t user) {
  int i;
  char ** groups;

  struct passwd * user_pw = getpwuid(user);
  int    length = DEFAULT_LENGHT;
  gid_t* gids   = malloc(sizeof(gid_t)*DEFAULT_LENGHT);

  if (getgrouplist (user_pw->pw_name, user_pw->pw_gid, gids, &length) == -1) {
    free(gids);
    gids = malloc(sizeof(gids)*length);
    getgrouplist (user_pw->pw_name, user_pw->pw_gid, gids, &length);
  }

  GList *list = NULL;
  for (i = 0; i<length; i++) {
    struct group* gr = getgrgid (gids[i]);
    list = g_list_prepend (list, strdup (gr->gr_name));
  }
  free (gids);
  return list;
}

gchar*
uid_to_user_name (uid_t uid) {
  struct passwd* pw = getpwuid (uid);
  if (pw == NULL)
    return NULL;
  return strdup (pw->pw_name);
}
