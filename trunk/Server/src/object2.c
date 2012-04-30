void 
do_reclist(dbref player, dbref cause, int key, char *buff)
{
    dbref i, comp;
    int count, typecomp;
    char cstat;

    if (key & REC_COUNT) {
	key &= ~REC_COUNT;
	cstat = 1;
    } else {
	cstat = 0;
    }
    count = 0;
    typecomp = -1;
    if (key == REC_TYPE) {
	switch (toupper(*buff)) {
	case 'P':
	    typecomp = TYPE_PLAYER;
	    break;
	case 'T':
	    typecomp = TYPE_THING;
	    break;
	case 'R':
	    typecomp = TYPE_ROOM;
	    break;
	case 'E':
	    typecomp = TYPE_EXIT;
	    break;
	default:
	    typecomp = NOTYPE;
	}
    } else if (key == REC_OWNER) {
	if ((*buff == '#') && (is_number(buff + 1)) {
	    comp = atoi(buff + 1);
	    if (!Good_obj(comp)) comp = -1;
	    }
	    else {
	    comp = lookup_player(player, buff, 0);
	    }
	    }
	    i = mudstate.recoverlist;
	    while (i != NOTHING) {
	    if ((typecomp > -1) && (Typeof(i) != typecomp)) {
	    i = Link(i);
	    continue;
	    }
	    else
	    if ((comp > -1) && (Next(i) != comp)) {
	    i = Link(i);
	    continue;
	    }
#ifndef STANDALONE
	    notify(player, unsafe_tprintf("Dbref: #%d, Name: %s, Former Owner: %s", i, Name(i), Name(Next(i))));
#endif
	    count++;
	    i = Link(i);
	    }
#ifndef STANDALONE
	    if (cstat)
	    notify(player, unsafe_tprintf("Total number of items: %d", count));
	    notify(player, "Done.");
#endif
	    }
