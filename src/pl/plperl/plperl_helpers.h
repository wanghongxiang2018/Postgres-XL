#ifndef PL_PERL_HELPERS_H
#define PL_PERL_HELPERS_H

/*
 * convert from utf8 to database encoding
 */
static inline char *
utf_u2e(const char *utf8_str, size_t len)
{
	int 	    enc = GetDatabaseEncoding();

	char	   *ret = (char *) pg_do_encoding_conversion((unsigned char *) utf8_str, len, PG_UTF8, enc);

	/*
	* when we are a PG_UTF8 or SQL_ASCII database
	* pg_do_encoding_conversion() will not do any conversion or
	* verification. we need to do it manually instead.
	*/
	if (enc == PG_UTF8 || enc == PG_SQL_ASCII)
		pg_verify_mbstr_len(PG_UTF8, utf8_str, len, false);

	if (ret == utf8_str)
		ret = pstrdup(ret);

	return ret;
}

/*
 * convert from database encoding to utf8
 */
static inline char *
utf_e2u(const char *str)
{
	char	   *ret = (char *) pg_do_encoding_conversion((unsigned char *) str, strlen(str), GetDatabaseEncoding(), PG_UTF8);

	if (ret == str)
		ret = pstrdup(ret);
	return ret;
}


/*
 * Convert an SV to a char * in the current database encoding
 */
static inline char *
sv2cstr(SV *sv)
{
	char	   *val;
	STRLEN		len;

	/*
	 * get a utf8 encoded char * out of perl. *note* it may not be valid utf8!
	 *
	 * SvPVutf8() croaks nastily on certain things, like typeglobs and
	 * readonly object such as $^V. That's a perl bug - it's not supposed to
	 * happen. To avoid crashing the backend, we make a mortal copy of the
	 * sv before passing it to SvPVutf8(). The copy will be garbage collected
	 * very soon (see perldoc perlguts).
	 */
	val = SvPVutf8(sv_mortalcopy(sv), len);

	/*
	 * we use perls length in the event we had an embedded null byte to ensure
	 * we error out properly
	 */
	return utf_u2e(val, len);
}

/*
 * Create a new SV from a string assumed to be in the current database's
 * encoding.
 */

static inline SV *
cstr2sv(const char *str)
{
	SV		   *sv;
	char	   *utf8_str = utf_e2u(str);

	sv = newSVpv(utf8_str, 0);
	SvUTF8_on(sv);

	pfree(utf8_str);

	return sv;
}

#endif   /* PL_PERL_HELPERS_H */
