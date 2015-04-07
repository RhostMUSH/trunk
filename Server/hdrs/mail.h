#ifndef _M_MAIL_H_
#define _M_MAIL_H_

/* mail switch defines */

#define M_SEND		0x00000001
#define	M_REPLY		0x00000002
#define M_FORWARD	0x00000004
#define M_READM		0x00000008
#define M_LOCK		0x00000010
#define M_STATUS	0x00000020
#define M_QUICK		0x00000040
#define M_DELETE	0x00000080
#define M_RECALL	0x00000100
#define M_MARK		0x00000200
#define M_SAVE		0x00000400
#define M_WRITE		0x00000800
#define M_NUMBER	0x00001000
#define M_CHECK		0x00002000
#define M_UNMARK	0x00004000
#define M_PASS		0x00008000
#define M_REJECT	0x00010000
#define M_SHARE		0x00020000
#define M_PAGE		0x00040000
#define M_QUOTA		0x00080000
#define M_FSEND		0x00100000
#define M_ZAP		0x00200000
#define M_NEXT		0x00400000
#define M_ALIAS		0x00800000
#define M_AUTOFOR	0x01000000
#define M_VERSION	0x02000000
#define M_ALL		0x04000000
#define M_PREV		0x08000000 /* This is NOT a switch.  It's a mask for mail/next */
#define M_ANON		0x10000000

#define WM_CLEAN	0x00000001
#define WM_LOAD		0x00000002
#define WM_UNLOAD	0x00000004
#define WM_WIPE		0x00000008
#define WM_ON		0x00000010
#define WM_OFF		0x00000020
#define WM_ACCESS	0x00000040
#define WM_OVER		0x00000080
#define WM_RESTART	0x00000100
#define WM_SIZE		0x00000200
#define WM_FIX		0x00000400
#define WM_LOADFIX	0x00000800
#define WM_ALIAS	0x00001000
#define WM_REMOVE	0x00002000
#define WM_ALOCK	0x00004000
#define WM_MTIME	0x00008000
#define WM_DTIME	0x00010000
#define WM_SMAX		0x00020000
#define WM_VERIFY	0x00040000

/* mail 2 */
#define M2_CREATE	0x00000001
#define M2_DELETE	0x00000002
#define M2_RENAME	0x00000004
#define M2_MOVE		0x00000008
#define M2_LIST		0x00000010
#define M2_CURRENT	0x00000020
#define M2_CHANGE	0x00000040
#define M2_PLIST	0x00000080
#define M2_SHARE	0x00000200
#define M2_CSHARE	0x00000400

#define MIND_IRCV	1
#define MIND_ISND	2
#define MIND_HRCV	3
#define MIND_HSND	4
#define MIND_MSG	5
#define MIND_BSIZE	6
#define MIND_REJM	7
#define MIND_WRTM	8
#define MIND_WRTL	9
#define MIND_WRTS	10
#define MIND_GA		11
#define MIND_GAL	12
#define MIND_ACS	13
#define MIND_PAGE	14
#define MIND_AFOR	15
#define MIND_SMAX	16

#define FIND_LST	1
#define FIND_BOX	2
#define FIND_CURR	3
#define FIND_CSHR	4

#define MERR_HRCV	0
#define MERR_HSND	1
#define MERR_MSG	2
#define MERR_ISND	3
#define MERR_DUMP	4
#define MERR_DB		5

#define MAILVERSION	"Seawolf Mail: v2.1.2"

#endif
