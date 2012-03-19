require 'mkmf'

dir_config("villa")

home = ENV['HOME']
$CFLAGS = "-I. -I../.. -I#{home}/include -I/usr/local/include"
$LDFLAGS = "-L../.. -L#{home}/lib -L/usr/local/lib"
$LIBS = "-L../.. -L#{home}/lib -L/usr/local/lib"

have_library("c", "main")
have_library("pthread", "main")
have_library("z", "main")
have_library("bz2", "main")
have_library("lzo2", "main")
have_library("iconv", "main")
have_library("qdbm", "main")

create_makefile("mod_villa")
