/* @(#)node.h	1.10 13/09/27 Copyright 1986-2013 J. Schilling */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

extern	Tnode*	allocnode	__PR((long type, Tnode *lp, Tnode *rp));
extern	void	freetree	__PR((Tnode *np));
extern	Argvec*	allocvec	__PR((int len));
extern	void	freevec		__PR((Argvec *vp));
extern	int	listlen		__PR((Tnode *lp));
extern	void	printtree	__PR((FILE *f, Tnode *cmd));
extern	void	printio		__PR((FILE *f, Tnode *cmd));
extern	void	_printio	__PR((FILE *f, long type));
extern	void	printstring	__PR((FILE *f, Tnode *cmd));
