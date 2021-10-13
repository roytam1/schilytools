#! /bin/sh

# abspath.sh:  Testing for running admin when the s-file 
#              is specified by an absolute path name.

# Import common functions & definitions.
. ../common/test-common

LC_ALL=C

#d=`../testutils/realpwd`

s=../../testscripts/ustar-all-filetypes.tar
docommand L1 "${tar} -t f=${s}" 0 "\
!-type-old-file
0-type-file
1-type-hardlink link to 0-type-file
2-type-symlink -> file
3-type-cdev
4-type-bdev
5-type-dir/
6-type-fifo
7-type-contfile
END
" "\
star: 1 blocks + 8192 bytes (total of 18432 bytes = 18.00k).
"

s=../../testscripts/ustar-bad-filetypes.tar
docommand L2 "TZ=GMT ${tar} -tv f=${s}" 0 "\
      0 -rw-r--r--  jes/glone Jun 15 14:41 2002 file
      0 -rw-r--r--  jes/glone Jun 15 14:41 2002 bad1
      0 -rw-r--r--  jes/glone Jun 15 14:41 2002 bad2
      0 -rw-r--r--  jes/glone Jun 15 14:41 2002 bad3
      0 -rw-r--r--  jes/glone Jun 15 14:41 2002 bad4
      0 -rw-r--r--  end/endgrp Jun 19 16:18 2002 END
" IGNORE

s=../../testscripts/not_a_tar_file
docommand L3 "${tar} -tv f=${s}" "!=0" "" IGNORE

s=../../testscripts/ustar-big-2g.tar.bz2
docommand L4 "${tar} -t f=${s}" 0 "\
big
file
" IGNORE

s=../../testscripts/ustar-big-8g.tar.bz2
docommand L5 "${tar} -t f=${s}" 0 "\
8gb-1
file
" IGNORE

s=../../testscripts/types-star-oldsparse.tar.gz
docommand L6 "${tar} -t f=${s}" 0 "\
./
file
file2
hardlink link to file2
symlink -> file
cdev
bdev
dir/
fifo
socket
test.c
sparsefile
test
unreadable
rest/
rest/devi
rest/devi1
rest/devi2
rest/devi3
rest/testfile
rest/testfile_link link to rest/testfile
rest/testnobod
rest/xtestfile
rest/xxtestfile
unreadablehardlink link to unreadable
unreadableempty
cdevhardlink link to cdev
" IGNORE

docommand L7 "${tar} -t -tpath f=${s}" 0 "\
./
file
file2
hardlink
symlink
cdev
bdev
dir/
fifo
socket
test.c
sparsefile
test
unreadable
rest/
rest/devi
rest/devi1
rest/devi2
rest/devi3
rest/testfile
rest/testfile_link
rest/testnobod
rest/xtestfile
rest/xxtestfile
unreadablehardlink
unreadableempty
cdevhardlink
" IGNORE

docommand L8 "TZ=GMT ${tar} -tv f=${s}" 0 "      0 drwxr-sr-x  joerg/bs  Nov  6 14:40 1994 ./
   1024 -rw-r--r-T  root/bs  Jun 15 20:31 1994 file
   1024 -rw-r--r-T  root/bs  Jun 15 20:31 1994 file2
   1024 Hrw-r--r-T  root/bs  Jun 15 20:31 1994 hardlink link to file2
      0 lrwxrwxrwx  root/bs  Jun 15 20:31 1994 symlink -> file
127 254 crw-r--r--  root/bs  Jun 15 20:32 1994 cdev
127 254 brw-r--r--  root/bs  Jun 15 20:32 1994 bdev
      0 drwxr-sr-x  root/bs  Jun 15 20:32 1994 dir/
  0   0 prw-r--r--  root/bs  Jun 15 20:32 1994 fifo
  0   0 srwxrwxrwx  root/staff Jun  4 12:51 1994 socket
     87 -rw-r--r--  root/bs  Jun 15 20:35 1994 test.c
 147456 Sr--r--r--  joerg/bs  Oct 17 17:29 1994 sparsefile
  24576 -rwxr-xr-x  root/bs  Jun 15 20:36 1994 test
   1024 ---------T  root/bs  Oct 27 21:13 1994 unreadable
      0 drwxr-sr-x  joerg/bs  Oct 26 14:12 1994 rest/
255 255 crw-r--r--  root/bs  Jun  8 15:54 1994 rest/devi
127 255 crw-r--r--  root/bs  Jun  8 15:54 1994 rest/devi1
  3 255 crw-r--r--  root/bs  Jun  8 15:54 1994 rest/devi2
  1 255 crw-r--r--  root/daemon Jun  8 15:54 1994 rest/devi3
      0 -rw-r--r--  1234/bs  Aug 17 13:07 1993 rest/testfile
      0 Hrw-r--r--  1234/bs  Aug 17 13:07 1993 rest/testfile_link link to rest/testfile
      0 -rw-r--r--  nobody/nogroup Oct 14 13:50 1993 rest/testnobod
      0 -rw-r--r--  sechzehnchar_uid/sechzehnchar_gi Sep 28 12:15 1993 rest/xtestfile
      0 -rw-r--r--  eine_zwei_und_dr/eine_zwei_und_d Sep 28 13:22 1993 rest/xxtestfile
   1024 H--------T  root/bs  Oct 27 21:13 1994 unreadablehardlink link to unreadable
      0 ----------  root/bs  Oct 27 20:53 1994 unreadableempty
127 254 Hrw-r--r--  root/bs  Jun 15 20:32 1994 cdevhardlink link to cdev
" IGNORE

s=../../testscripts/100char_longlink.tar
docommand L9 "${tar} -t f=${s}" 0 "\
12345678901234/12345678901234/12345678901234/12345678901234/12345678901234/12345678901234/1234567890
tar-longlink link to 12345678901234/12345678901234/12345678901234/12345678901234/12345678901234/12345678901234/1234567890
" IGNORE

s=../../testscripts/long.ustar.gz
docommand L10 "${tar} -t f=${s}" 0 "\
_004/
________________+020/
________________+020/_______________+040/
________________+020/_______________+040/_______________+060/
________________+020/_______________+040/_______________+060/_______________+080/
________________+020/_______________+040/_______________+060/_______________+080/_______________+100/
________________+020/_______________+040/_______________+060/_______________+080/_______________+100/_______________+120/
________________+020/_______________+040/_______________+060/_______________+080/_______________+100/_______________+120/_______________+140/
________________+020/_______________+040/_______________+060/_______________+080/_______________+100/_______________+120/_______________+140/_______________+160/
________________+020/_______________+040/_______________+060/_______________+080/_______________+100/_______________+120/_______________+140/_______________+160/_______________+180/
________________+020/_______________+040/_______________+060/_______________+080/_______________+100/_______________+120/_______________+140/_______________+160/_______________+180/_______________+200/
________________+020/_______________+040/_______________+060/_______________+080/_______________+100/_______________+120/_______________+140/_______________+160/_______________+180/_______________+200/_______________+220/
________________+020/_______________+040/_______________+060/_______________+080/_______________+100/_______________+120/_______________+140/_______________+160/_______________+180/_______________+200/_______________+220/_______________+240/
________________+020/_______________+040/_______________+060/_______________+080/_______________+100/_______________+120/_______________+140/_______________+160/_______________+180/_______________+200/_______________+220/___________014
________________+020/_______________+040/_______________+060/_______________+080/_______________+100/_______________+120/_______________+140/_______________+160/_______________+180/_______________+200/___________014
________________+020/_______________+040/_______________+060/_______________+080/_______________+100/_______________+120/_______________+140/_______________+160/_______________+180/___________014
________________+020/_______________+040/_______________+060/_______________+080/_______________+100/_______________+120/_______________+140/_______________+160/___________014
________________+020/_______________+040/_______________+060/_______________+080/_______________+100/_______________+120/_______________+140/___________014
________________+020/_______________+040/_______________+060/_______________+080/_______________+100/_______________+120/_______________+140/________+153/
________________+020/_______________+040/_______________+060/_______________+080/_______________+100/_______________+120/_______________+140/________+153/________________________________________________________________________________________________099
________________+020/_______________+040/_______________+060/_______________+080/_______________+100/_______________+120/_______________+140/________+153/_________________________________________________________________________________________________100
________________+020/_______________+040/_______________+060/_______________+080/_______________+100/_______________+120/_______________+140/_________+154/
________________+020/_______________+040/_______________+060/_______________+080/_______________+100/_______________+120/_______________+140/_________+154/________________________________________________________________________________________________099
________________+020/_______________+040/_______________+060/_______________+080/_______________+100/_______________+120/_______________+140/_________+154/_________________________________________________________________________________________________100
________________+020/_______________+040/_______________+060/_______________+080/_______________+100/_______________+120/_______________+140/__________+155/
________________+020/_______________+040/_______________+060/_______________+080/_______________+100/_______________+120/_______________+140/__________+155/________________________________________________________________________________________________099
________________+020/_______________+040/_______________+060/_______________+080/_______________+100/_______________+120/_______________+140/__________+155/_________________________________________________________________________________________________100
________________+020/_______________+040/_______________+060/_______________+080/_______________+100/_______________+120/_______________+140/___________+156/
________________+020/_______________+040/_______________+060/_______________+080/_______________+100/_______________+120/___________014
________________+020/_______________+040/_______________+060/_______________+080/_______________+100/___________014
________________+020/_______________+040/_______________+060/_______________+080/___________014
________________+020/_______________+040/_______________+060/___________014
________________+020/_______________+040/___________014
________________+020/___________014
______________________________________________________________________________________+090/
______________________________________________________________________________________+090/____________________________________________________________+155/
______________________________________________________________________________________+090/____________________________________________________________+155/_____________________________________040
______________________________________________________________________________________+090/_____________________________________________________________+156/
______________________________________________________________________________________________D_099/
______________________________________________________________________________________________D_099/________________________________________________________________________________________________099
______________________________________________________________________________________________D_099/_________________________________________________________________________________________________100
________________________________________________________________________________________________099
_________________________________________________________________________________________________100
" IGNORE

success
