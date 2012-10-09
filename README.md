NAME
       rr - retain / recall file and directory paths

SYNOPSIS
       rr
       rr [/path/to/filename | /path/to/directory/]
       rr [filename | directory/]
       rr [exec command] [args ...]

DESCRIPTION
       rr  is  a basic command-line utility designed to retain/recall file and
       directory paths.  This is done by treating the  filename  itself  as  a
       unique  key  to be referenced for future rr program calls.  The purpose
       of this is to assist the user in shorthand typing and/or not having  to
       remember arbitrary full paths.

       All  retained  values are stored in ~/.rr, and are unique to each user.
       If you are attempting to retain a path that has  a  filename  that  has
       already  been retained before, rr will retain the new path over the old
       path.

OPTIONS
       rr
              Pass a file or directory  value  to  stdin  to  be  retained  or
              recalled,  then  verbosely  print  it  to stdout.  This does not
              include the ability to execute commands listed below  as  it  is
              thought to be a potential security risk.

       rr [/path/to/filename | /path/to/directory/]
              Retain  a  file or directory, then verbosely print it to stdout.
              Note that directories are signified by a trailing slash.

       rr [filename | directory/]
              Recall a file or directory that has  been  previously  retained,
              then  verbosely  print  it to stdout.  Note that directories are
              signified by a trailing slash.

       rr [exec command] [args ...]
              Execute a command using retained files and directories.  In this
              form, retained values are referenced by a // prefix.

EXAMPLE
       # Retain the path of the desired file, and verbosely print it out.
       rr /etc/httpd/conf/httpd.conf

       # Recall the retained value, and verbosely print it out.
       rr httpd.conf

       # Execute vi on /etc/httpd/conf/httpd.conf. (designated by "//")
       rr vi //httpd.conf

       # Retain a directory path.
       rr /etc/rc.d/init.d/

       # Change directory to /etc/rc.d/init.d/
       cd `rr init.d/`

       # List files matching: /etc/rc.d/init.d/s*
       # (the '*' may need to be quoted depending on your shell)
       rr ls -l //init.d/s*

FILES
       ~/.rr

AUTHOR
       Written by v9/fakehalo. [v9@fakehalo.us]

BUGS
       Report bugs to <v9@fakehalo.us>.

COPYRIGHT
       Copyright . 2007 fakehalo.
       This is free software; see the source for copying conditions.  There is
       NO warranty; not even for MERCHANTABILITY or FITNESS FOR  A  PARTICULAR
       PURPOSE.

