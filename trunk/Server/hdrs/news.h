/* news.h - Hardcoded version of the original softcoded MushNews */
/* Thorin - 01/1997 */

/* Added v2 capabilities/database on 3/20/99 */

#ifndef _M_NEWS_H__
#define _M_NEWS_H__

#include "config.h"
#include "alloc.h"

#define LATEST_NEWS_DB_VERSION   2

#define DEFAULT_EXPIRE_OFFSET    (60 * 60 * 24 * 60) /* 60 days */

#define GROUP_NAME_LEN           32
#define GROUP_DESC_LEN           40
#define LOCK_TEXT_LEN            LBUF_SIZE
#define ART_TEXT_LEN             LBUF_SIZE
#define ART_TITLE_LEN            40
#define USER_NAME_LEN            PLAYER_NAME_LIMIT

#define VV_KEY_LEN               (2)
#define XX_KEY_LEN               (2)
#define XX_KEY_LEN_V1            (2)
#define GI_KEY_LEN               (2 + GROUP_NAME_LEN)
#define GI_KEY_LEN_V1            (2 + GROUP_NAME_LEN)
#define GR_KEY_LEN               (2 + GROUP_NAME_LEN)
#define GP_KEY_LEN               (2 + GROUP_NAME_LEN)
#define GX_KEY_LEN               (2 + GROUP_NAME_LEN)
#define GA_KEY_LEN               (2 + GROUP_NAME_LEN + sizeof(int))
#define GT_KEY_LEN               (2 + GROUP_NAME_LEN + sizeof(int))
#define GU_KEY_LEN               (2 + GROUP_NAME_LEN + sizeof(dbref))
#define UI_KEY_LEN               (2 + sizeof(dbref))
#define UG_KEY_LEN               (2 + sizeof(dbref) + GROUP_NAME_LEN)
#define UA_KEY_LEN               (2 + sizeof(dbref) + GROUP_NAME_LEN \
                                  + sizeof(int))

#define OWNER_NUKE_ALIVE         ' '
#define OWNER_NUKE_DEAD          'X'

typedef struct NewsVersionRec NewsVersionRec;
struct NewsVersionRec {               /* new in v2 */
  int version;
};

typedef struct NewsGlobalRec NewsGlobalRec;
struct NewsGlobalRec {
  time_t db_creation_time;
  time_t last_expire_time;
  int def_article_life;              /* new in v2 */
  int user_limit;
  char first_group[GROUP_NAME_LEN];
  dbref first_user_dbref;
};

/* legacy structure for db conversion */
typedef struct NewsGlobalRec_v1 NewsGlobalRec_v1;
struct NewsGlobalRec_v1 {
  time_t db_creation_time;
  time_t last_expire_time;
  int user_limit;
  char first_group[GROUP_NAME_LEN];
  dbref first_user_dbref;
};

typedef struct NewsGroupInfoRec NewsGroupInfoRec;
struct NewsGroupInfoRec {
  char name[GROUP_NAME_LEN];
  char desc[GROUP_DESC_LEN];
  dbref adder;
  /* dbref admin; */                /* del in v2 */
  int posting_limit;
  int def_article_life;             /* new in v2 */
  time_t create_time;
  time_t last_activity_time;
  int next_post_seq;
  int first_seq;
  dbref first_user_dbref;
  char next_group[GROUP_NAME_LEN];
};

typedef struct NewsGroupInfoRec_v1 NewsGroupInfoRec_v1;
struct NewsGroupInfoRec_v1 {
  char name[GROUP_NAME_LEN];
  char desc[GROUP_DESC_LEN];
  dbref adder;
  dbref admin;
  int posting_limit;
  time_t create_time;
  time_t last_activity_time;
  int next_post_seq;
  int first_seq;
  dbref first_user_dbref;
  char next_group[GROUP_NAME_LEN];
};

typedef struct NewsGroupReadLockRec NewsGroupReadLockRec;
struct NewsGroupReadLockRec {
  char group_name[GROUP_NAME_LEN];
  char text[LOCK_TEXT_LEN];
};

typedef struct NewsGroupPostLockRec NewsGroupPostLockRec;
struct NewsGroupPostLockRec {
  char group_name[GROUP_NAME_LEN];
  char text[LOCK_TEXT_LEN];
};

typedef struct NewsGroupAdminLockRec NewsGroupAdminLockRec;
struct NewsGroupAdminLockRec {           /* new in v2 */
  char group_name[GROUP_NAME_LEN];
  char text[LOCK_TEXT_LEN];
};

typedef struct NewsGroupArticleInfoRec NewsGroupArticleInfoRec;
struct NewsGroupArticleInfoRec {
  char group_name[GROUP_NAME_LEN];
  int seq;
  char title[ART_TITLE_LEN];
  char owner_name[USER_NAME_LEN];
  dbref owner_dbref;
  char owner_nuked_flag;
  time_t post_time;
  time_t expire_time;
  int next_seq;
};
  
typedef struct NewsGroupArticleTextRec NewsGroupArticleTextRec;
struct NewsGroupArticleTextRec {
  char group_name[GROUP_NAME_LEN];
  int seq;
  char text[ART_TEXT_LEN];
};

typedef struct NewsGroupUserRec NewsGroupUserRec;
struct NewsGroupUserRec {
  char group_name[GROUP_NAME_LEN];
  dbref user_dbref;
  dbref next_user_dbref;
};

typedef struct NewsUserInfoRec NewsUserInfoRec;
struct NewsUserInfoRec {
  dbref user_dbref;
  int total_posts;
  char first_group[GROUP_NAME_LEN];
  char last_read_group[GROUP_NAME_LEN];
  char first_art_group[GROUP_NAME_LEN];
  int first_art_seq;
  dbref next_user_dbref;
};

typedef struct NewsUserGroupRec NewsUserGroupRec;
struct NewsUserGroupRec {
  dbref user_dbref;
  char group_name[GROUP_NAME_LEN];
  int seq_marker;
  char next_group[GROUP_NAME_LEN];
};

typedef struct NewsUserArticleRec NewsUserArticleRec;
struct NewsUserArticleRec {
  dbref user_dbref;
  char group_name[GROUP_NAME_LEN];
  int seq;
  char next_art_group[GROUP_NAME_LEN];
  int next_art_seq;
};

#endif /* __NEWS_H__ */
