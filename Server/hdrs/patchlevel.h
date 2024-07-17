/* patchlevel.h - version information */
#ifndef _M_PATCHLEVEL_H_
#define _M_PATCHLEVEL_H_

#include "copyright.h"

/* PATCHLEVEL_CHANGED 1 */

/* RHOST VERSIONING HOWTO:
 * -----------------------
 * -  When FIRST commiting new major rewrite code, change the Rhost version to
 *    'NEWMAJOR.0.0' and specify a 'VERSION_EXT' of 'ALPHA'. Later move on to
 *    'BETA' when thins mature, and finally the Release Candidate 'RC#' stage
 *    once it is ready for general public testing.
 *    Examples for this are major DB system rewrites, big overhauls of
 *    internal structures, etc.
 *    Read: You are changing Rhost's very heart.
 *
 * -  The final go-live commit once the major rewrite is mature and tested
 *    should remain at 'NEWMAJOR.0.0', but without any 'VERSION_EXT' set.
 *
 * -  Normal new features and feature changes should increment 'MINOR' by one.
 *
 * -  Bugfixes and crashfixes should increment 'PATCH' by one.
 *
 * -  All these version changes should happen only when commiting on the
 *    MASTER BRANCH.
 * -  Commits to your own branches, feature branches, etc. MUST NOT change the
 *    version number.
 *
 * -  In all cases, before commiting, set the 'PATCHLEVEL_CHECKED' comment
 *    above to 1. The Git commit/push hooks will work off that value to
 *    make sure to show up a reminder to check the version number in this file
 *    in case that comment is left at 0. You need to set it to 1 in order to
 *    push successfully. It will re-set it to 0 during a successful commit.
 *
 * -  The 'MUSH_RELEASE_DATE' gets automatically filled out in the patchlevel
 *    file. 
 */

#define MAJOR_VERSION           "4" /* Major rewrites, DB system changes..   */
#define MINOR_VERSION           "5" /* Normal new features and changes       */
#define PATCH_VERSION           "0" /* Bugfixes, crashfixes etc.             */
#define VERSION_EXT             ""  /* "" OR "ALPHA", "BETA, "RC1", "RC2"... */

#define MUSH_RELEASE_DATE       "2024-07-16"          /* Source release date */

#endif
