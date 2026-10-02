#
# cross build for edit
#
# cmd/edit/GNUmakefile
#
# edit is the Software Tools line editor, translated into C from the
# Ratfor source beside this file.  The *.r files and the common blocks
# (ctxt cpat cbuf cfile cscrat clines common) ship as reference; only
# edit.c is compiled.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= edit
CLASS	= cmd

include $(DEPTH)/GNUmakefile.inc

SRCS += ctxt cpat cbuf cfile cscrat clines common append.r ckglob.r \
	ckp.r clrbuf.r defalt.r delete.r docmd.r doglob.r doprnt.r \
	doread.r dowrit.r edit.r getfn.r getind.r getlst.r getnum.r \
	getone.r getrhs.r gettxt.r inject.r maklin.r move.r nextln.r \
	optpat.r prevln.r ptscan.r relink.r setbuf.r skipbl.r subst.r
