/*
 * CDDL HEADER START
 *
 * This file and its contents are supplied under the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may use this file only in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 1993 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * @(#)chmod.cc 1.4 06/12/12
 */

#pragma	ident	"@(#)chmod.cc	1.4	06/12/12"

/*
 * Copyright 2017 J. Schilling
 *
 * @(#)chmod.cc	1.3 21/08/16 2017 J. Schilling
 */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)chmod.cc	1.3 21/08/16 2017 J. Schilling";
#endif

#include <vroot/vroot.h>
#include <vroot/args.h>

extern int chmod(const char *path, mode_t mode);


static int	chmod_thunk(char *path)
{
	vroot_result= chmod(path, vroot_args.chmod.mode);
	return(vroot_result == 0);
}

int	chmod_vroot(char *path, int mode, pathpt vroot_path, pathpt vroot_vroot)
{
	vroot_args.chmod.mode= mode;
	translate_with_thunk(path, chmod_thunk, vroot_path, vroot_vroot, rw_read);
	return(vroot_result);
}
