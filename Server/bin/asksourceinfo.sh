#!/bin/bash
info::door() {
            echo "This will enable the MUSH Doors portion of @door connectivity."
            echo "This is used with the doors.txt/doors.indx files for sites."
}

info::blank() {
            echo ""
}

info::mysql() {
            echo "This enables you to automatically include the MYSQL definitions"
            echo "to the DEFs line in the Makefile.  This is to support if you have"
            echo "this package installed and should only be used then.  Otherwise,"
            echo "it's safe to leave it at the default."
}

info::sidefx() {
            echo "Sideeffects are functions like set(), pemit(), create(), and the like."
            echo "This must be enabled to be able to use these functions.  In addition,"
            echo "when enabeled, the SIDEFX flag must be individually set on anything"
            echo "you wish to use the sideeffects."
}

info::doorempire() {
            echo "This will enable the Empire (Unix Game) door.  You must have"
            echo "A currently running unix game server running and listening on a port."
}

info::doors() {
            echo "This option drills down to various door options that you can enable."
            echo "The currently supported production doors are the ones listed."
            echo "Keep in mind each one does require tweeking files."
}

info::oldu() {
            echo "RhostMUSH, by default, have u(), ufun(), and zfun() functions parse by"
            echo "relation of the enactor instead of by relation of the target.  This is"
            echo "more MUSE compatable than MUSH.  If you wish to have it more compatable"
            echo "to MUX/TinyMUSH/Penn, then you need this enabled.  Keep in mind, turning"
            echo "off this compatibility WILL BREAK MUX/PENN/MUSH COMPATIBILITY!"
}

info::doorpop3() {
	    echo "This enables the POP3 door interface.  This is still very limited"
            echo "in functionality and currently only displays total mail counts."
}

info::sbuf() {
            echo "This allows you to have 64 character attribute length.  This in effect"
            echo "will set all SBUF's to 64 bit length instead of 32.  This has not been"
            echo "fully tested or regression tested so there could be some concern if"
            echo "you choose to use this.  If you find errors, report to us immediately."
}

info::oldinc() {
	    echo "RhostMUSH, by default, uses inc() and dec() to increment and decrement"
            echo "setq registers.  This is, unfortunately, not the default behavior for"
            echo "MUX, Penn, or TinyMUSH.  By enabling this options, you switch the"
            echo "functionality of inc() and dec() to be like MUX/Penn/TinyMUSH."
}

info::sqlite() {
	    echo "This enables the SQLite library bindings. SQLite is a zero-config"
            echo "file-based SQL relational database system, similar to MySQL or"
            echo "PostgreSQL, but without the complexity (or fragility) of running a"
            echo "server."
}

info::comsys() {
            echo "RhostMUSH has a very archiac and obtuse comsystem.  It does work, and"
            echo "is very secure and solid, but it lacks significant functionality."
            echo "You probably want to toggle this on and use a softcoded comsystem."
}

info::qdbm() {
            echo "This enables the QDBM database manager instead of the default GDBM"
            echo "database manager.  This may be the preferred database eventhough"
            echo "it is considered 'beta' as this is not hampered by the attribute"
            echo "cap per dbref# (750 default) and is generally a faster and more"
            echo "robust database engine.  Be warned, however, that QDBM is NOT"
            echo "binary compatible with GDBM, so any existing databases"
            echo "WILL NOT LOAD.  You have to flatfile dump the database then"
            echo "db_load the flatfile into the database once qdbm is compiled."
}

info::ansisub() {
            echo "This submenu allows you to choose, which substitution act as ANSI"
            echo "code substitutions. These are the settings to mimic other codebases:"
            echo "       MUX: %x and %c are ANSI, %m is last command."
            echo "      PENN: %c is last command. There are no ANSI substutions."
            echo "TinyMUSH 3: %x is ANSI sub, %m is last command. %x depends on config."
}


info::lbuf() {
            echo "This selection displays the menu allowing you to change the default"
            echo "size of Rhost's text buffers. The default is traditionally 4000 for"
            echo "Rhost, and 8192 for PennMUSH, TinyMUSH and MUX."
            echo "Now you can select your favorite length yourself, with the options "
            echo "of clasic 4k, 8K like other codebases, or even 16, 32 and 64K!     "
            echo "This increases both, the max length of attribute contents as well  "
            echo "as output length going to the client.                              "
            echo "IMPORTANT: the config parameter output_length should always be     "
            echo "4 times this setting. Rhost has a default output_limit config      "
            echo "setting of '16384' with the 4K LBUFs. Increase accordingly before  "
            echo "starting your game."
}

info::crypt() {
            echo "RhostMUSH supports crypt() and decrypt() functions.  Toggle this"
            echo "if you wish to use them."
}

info::plushelp() {
         echo "RhostMUSH allows you to use a plushelp.txt file for +help.  This"
         echo "supports MUX/TinyMUSH3 in how +help is hardcoded to a text file."
         echo "Enable this if you wish to have a plushelp.txt file be used."
}

info::program() {
         echo "RhostMUSH, by default, allows multiple arguments to be passed"
         echo "to @program.  This, unfortunately, is not how MUX does it, so"
         echo "if you wish a MUX compatable @program, enable this."
}

info::command() {
   	 echo "RhostMUSH, optionally, allows you to use the COMMAND flag to"
         echo "specify what objects are allowed to run \$comands/^listens."
         echo "If you wish to have this flag enabled, toggle this option."
 }

info::hackattr() { 
         echo "RhostMUSH allows 'special' attributes that start with _ or ~."
         echo "By default, _ attributes are wiz settable and seeable only."
         echo "If you wish to have these special attributes, enable this option."
}

info::reality() {
         echo "Reality Levels is a document onto itself.  In essence, reality"
         echo "levels allow a single location to have multiple 'realities' where"
         echo "you can belong to one or more realities and interact with one or"
         echo "more realities.  It's an excellent way to handle invisibility,"
         echo "shrouding, or the like.  Enable this if you want it."
}

info::registers() {
         echo "Sometimes, the standard 10 (0-9) setq registers just arn't enough."
         echo "If you, too, find this to be the case, enable this option and it"
         echo "will allow you to have a-z as well."
}

info::ansienhance() {
         echo "RhostMUSH handles ansi totally securely.  It does this by stripping"
         echo "ansi from all evaluation.  This does, however, have the unfortunate"
         echo "effect of making ansi very cumbersome and difficult to use.  If you"
         echo "have this uncontrollable desire to have color everywhere, then you"
         echo "probably want this enabled as it allows you to use ansi in most"
         echo "places without worry of stripping."
}

info::marker() {
         echo "Marker flags are your 10 dummy flags (marker0 to marker9).  These"
         echo "flags, are essentially 'markers' that you can rename at leasure."
         echo "If you have a desire for marker flags, enable this option."
}

info::bang() {
         echo "Bang support.  Very cool stuff.  It allows you to use ! for false"
         echo "and !! for true.  An example would be [!match(this,that)].  It"
         echo "also allows $! for 'not a string' and $!! for 'is a string'."
         echo "Such an example would be [$!get(me/blah)].  If you like this"
         echo "feature, enable this option.  You want it.  Really."
}

info::who() {
         echo "This is an alternate WHO listing.  It's a tad longer for the"
         echo "display and will switch ports to Total Cmds on the WHO listings."
}

info::oldregisters() {
         echo "This goes back to an old implementation of the setq/setr register"
         echo "emplementation.  This may make it more difficult to use ansi but"
         echo "the newer functions would still exist.  In essence, this will"
         echo "switch SETQ with SETQ_OLD and SETR with SETR_OLD in the code."
 }

info:securesidefx() {
    	 echo "This shores up sideeffects for pemit(), remit(), zemit(), and"
         echo "others to avoid a double evaluation.  You probably want this"
         echo "enabled as it mimics MUX/PENN, but if you have an old Rhost"
         echo "that used sideeffects, this can essentially break compatabiliity."
}

info::debugmon() {
         echo "This disables the debug monitor (debugmon) from working within Rhost."
         echo "There is really no need to disable this, unless you are in a chroot"
         echo "jail or in some way are on a restricted unix shell that does not allow"
         echo "you to use shared memory segments."
}

info::signals() {
         echo "This is intended to disable signal handling in RhostMUSH.  Usually,"
         echo "you can send TERM, USR1, and USR2 signals to reboot, flatfile, or"
         echo "shutdown the mush."
}

info::oldreality() {
         echo "The Reality Levels in use prior required the chkreality toggle to"
         echo "be used for options 2 & 3 in locktypes.  If the lock did not exist"
         echo "it would assume failure even if no chkreality toggle was set.  This"
         echo "was obviously an error.  However, if this is enabled, then this"
         echo "behavior will remain so that any preexisting code will work as is."
}

info::muxpasswords() {
         echo "This allows you to natively read MUX2 set SHA1 passwords in a"
         echo "converted database.  If you change the password, it will use the"
         echo "Rhost specific password system and overwrite the SHA1 password."
         echo "You only need to use this if you are using a MUX2 converted flatfile."
}

info::lowmem() {
    	 echo "This is a low memory compile option.  If you are running under a"
         echo "Virtual Machine, or have low available memory, the compile may"
         echo "error out saying out of memory, unable to allocate memory, or"
         echo "similiar messages.  Enabling this option should bypass this."
}

info::ssl() {
	 echo "Sometimes, you may have a third party SSL package that is"
         echo "incompatible with the development library for OpenSSL.  In such"
         echo "a case, select this option to disable OpenSSL from compiling."
}

info::pcre() {
    	 echo "The system dependant PCRE Library will be much  much faster"
         echo "than the one included with the source.  Enabling this option"
         echo "uses the system's PCRE Library. Disabling it uses a PCRE"
         echo "library supplied by RhostMUSH that should only be used if the"
         echo "system library caused compilation issues."
}

info::shapass() {
         echo "This option encrypts your passwords using a random seed and"
         echo "the SHA512 encryption method.  It will fall back to standard"
         echo "DES encryption for compatibility regardless."
}

info::websockets() {
         echo "This option enables the support for Websockets introduced in"
         echo "RhostMUSH release 4.1.0p2. This option allows you to use the"
         echo "Websocket protocol for connections with the games you run."
}
