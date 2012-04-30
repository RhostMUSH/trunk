
static char *
crypt_code(code, text, type)
    char *code;
    char *text;
    int type;
{
    static char textbuff[BUFFER_LEN];
    char codebuff[BUFFER_LEN];
    int start = 32;
    int end = 126;
    int mod = end - start + 1;
    char *p, *q, *r;

    if (!text && !*text)
	return ((char *) "");
    strcpy(codebuff, crunch_code(code));
    if (!code || !*code || !codebuff || !*codebuff)
	return (text);
    strcpy(textbuff, "");

    p = text;
    q = codebuff;
    r = textbuff;
    /* Encryption: Simply go through each character of the text, get its ascii
       value, subtract start, add the ascii value (less start) of the
       code, mod the result, add start. Continue  */
    while (*p) {
	if ((*p < start) || (*p > end)) {
	    p++;
	    continue;
	}
	if (type)
	    *r++ = (((*p++ - start) + (*q++ - start)) % mod) + start;
	else
	    *r++ = (((*p++ - *q++) + 2 * mod) % mod) + start;
	if (!*q)
	    q = codebuff;
    }
    *r = '\0';
    return (textbuff);
}

/* Copy over only alphanumeric chars */
static char *
crunch_code(code)
    char *code;
{
    char *in;
    char *out;
    static char output[BUFFER_LEN];

    out = output;
    in = code;
    while (*in) {
	if ((*in >= 32) || (*in <= 126)) {
	    printf("%c", *in);
	    *out++ = *in;
	}
	in++;
    }
    *out = '\0';
    return (output);
}

FUNCTION(fun_encrypt)
{
    safe_str(crypt_code(args[1], args[0], 1), buff, bufcx);
}

FUNCTION(fun_decrypt)
{
    safe_str(crypt_code(args[1], args[0], 0), buff, bufcx);
}
