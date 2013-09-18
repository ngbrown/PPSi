/*
 * Copyright (C) 2013 CERN (www.cern.ch)
 * Author: Aurelio Colosimo
 *
 * Released according to the GNU LGPL, version 2.1 or any later version.
 */

#include <ppsi/ppsi.h>
/* This file is build in hosted environments, so following headers are Ok */
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

/* Each pre-defined keyword is handled in the keywords list, so that the
 * same code is used to parse every token in the conf file */

struct keyword_t {
	int id;
	char *key;
	int (*handle_key)(struct pp_globals *ppg, char *val);
};

enum {KW_INV = -1, KW_LINK, KW_IFACE, KW_PROTO, KW_RAW, KW_UDP, KW_ROLE,
			KW_AUTO, KW_MASTER, KW_SLAVE, KW_EXT, KW_NONE, KW_WHITERAB,
			KW_CLOCK_CLASS, KW_CLOCK_ACCURACY,
			KW_NUMBER};

static int handle_link(struct pp_globals *ppg, char *val);
static int handle_iface(struct pp_globals *ppg, char *val);
static int handle_proto(struct pp_globals *ppg, char *val);
static int handle_role(struct pp_globals *ppg, char *val);
static int handle_ext(struct pp_globals *ppg, char *val);
static int handle_clock_class(struct pp_globals *ppg, char *val);
static int handle_clock_accuracy(struct pp_globals *ppg, char *val);

static struct keyword_t keywords[KW_NUMBER] = {
	{KW_LINK, "link", handle_link},
	{KW_IFACE, "iface", handle_iface},
	{KW_PROTO, "proto", handle_proto}, {KW_RAW, "raw"}, {KW_UDP, "udp"},
	{KW_ROLE, "role", handle_role}, {KW_AUTO, "auto"}, {KW_MASTER, "master"},
		{KW_SLAVE, "slave"},
	{KW_EXT, "extension", handle_ext}, {KW_NONE, "none"},
		{KW_WHITERAB, "whiterabbit"},
	{KW_CLOCK_CLASS, "clock-class", handle_clock_class},
	{KW_CLOCK_ACCURACY, "clock-accuracy", handle_clock_accuracy},
};

static int detect_keyword(char *kw)
{
	int i;
	for (i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++)
		if (!strcmp(keywords[i].key, kw))
			return i;
	return KW_INV;
}

/* Handlers for "key" keywords: return a bool value, 1 in case of success,
 * 0 otherwise */

static int handle_link(struct pp_globals *ppg, char *val)
{
	/* ppg->nlinks is initialized to -1 and used as index for current link,
	 * so increase it before using it */
	ppg->nlinks++;
	strcpy(ppg->links[ppg->nlinks].link_name, val); /* FIXME check val len */
	return 1;
}

static int handle_iface(struct pp_globals *ppg, char *val)
{
	strcpy(ppg->links[ppg->nlinks].iface_name, val); /* FIXME check iface len */
	return 1;
}

static int handle_clock_class(struct pp_globals *ppg, char *val)
{
	GOPTS(ppg)->clock_quality.clockClass = atoi(val);
	return 1;
}

static int handle_clock_accuracy(struct pp_globals *ppg, char *val)
{
	GOPTS(ppg)->clock_quality.clockAccuracy = atoi(val);
	return 1;
}

static int handle_proto(struct pp_globals *ppg, char *val)
{
	int v_id;
	v_id = detect_keyword(val);

	switch(v_id) {
		case KW_RAW:
		case KW_UDP:
			ppg->links[ppg->nlinks].proto = v_id - KW_RAW;
			break;
		default:
			return 0;
	}
	return 1;
}

static int handle_role(struct pp_globals *ppg, char *val)
{
	int v_id;
	v_id = detect_keyword(val);

	switch(v_id) {
		case KW_AUTO:
		case KW_MASTER:
		case KW_SLAVE:
			ppg->links[ppg->nlinks].role = v_id - KW_AUTO;
			break;
		default:
			return 0;
	}
	return 1;
}

static int handle_ext(struct pp_globals *ppg, char *val)
{
	int v_id;
	v_id = detect_keyword(val);

	switch(v_id) {
		case KW_NONE:
		case KW_WHITERAB:
			ppg->links[ppg->nlinks].ext = v_id - KW_NONE;
			break;
		default:
			return 0;
	}
	return 1;
}

/* Called with the pair key-val detected by pp_parse_conf state machine */
static int handle_key_val(struct pp_globals *ppg, char *key, char *val)
{
	int k_id;
	k_id = detect_keyword(key);

	switch(k_id) {
		case KW_LINK:
		case KW_IFACE:
		case KW_PROTO:
		case KW_ROLE:
		case KW_EXT:
		case KW_CLOCK_CLASS:
		case KW_CLOCK_ACCURACY:
			return keywords[k_id].handle_key(ppg, val);

		default:
			return 0;
	}
}

/* Parse configuration (e.g. coming from /etc/ppsi.conf file) */
static int pp_parse_conf(struct pp_globals *ppg, char *conf, int len)
{
	int st = 0;
	char c;
	char key[16], val[16];
	int toklen = 0;
	int ln = 1; /* Line counter */
	int newln;
	int i;

	/* ppg->nlinks is initialized to -1 and increased at first link found,
	 * so that we can use it as array index */
	ppg->nlinks = -1;
	memset(ppg->links, 0, ppg->max_links * sizeof(struct pp_link));

	for (i = 0; i < len; i++) {
		c = conf[i];

		/* Increase line counter and check if latest line is invalid */
		newln = 0;
		if ((c == '\n') || (c == '\0')) {
			ln++;
			newln = 1;
			if (st == 2)
				goto ret_invalid_line;
		}

		switch (st) {
		case 0: /* initial status */
			if ((c == ' ') || (c == '\t') || (c == '\r') || (newln))
				continue;

			if (c == '#')
				st = 1;
			else {
				key[0] = c;
				toklen = 1;
				st = 2;
			}
			break;

		case 1: /* ignoring commented line, those starting with '#' */
			if (newln)
				st = 0;
			else
				continue;
			break;

		case 2: /* detecting key */

			if (toklen == sizeof(key))
				goto ret_invalid_line;

			if ((c != ' ') && (c != '\t'))
				key[toklen++] = c;
			else {
				st = 3;
				key[toklen] = '\0';
				toklen = 0;
			}

			break;

		case 3: /* detecting val */

			if (toklen == sizeof(val))
				goto ret_invalid_line;

			if ((c != ' ') && (c != '\t') && (c != '\r') && (!newln))
				val[toklen++] = c;
			else if (newln) {
				val[toklen] = '\0';
				if (!handle_key_val(ppg, key, val))
					goto ret_invalid_line;
				st = 0;
				toklen = 0;
			}
		}

	}

	/* finally, increase nlinks so that it is equal to the configured links
	 * count */
	ppg->nlinks++;

	return 0;

ret_invalid_line:
	ln--;
	return -ln;
}


/* Open about a file, warn if not found */
static int pp_open_conf_file(char **argv, char *name)
{
	int fd;

	if (!name)
		return -1;
	fd = open(name, O_RDONLY);
	if (fd >= 0) {
		pp_printf("%s: Using config file %s\n", argv[0], name);
		return fd;
	}
	pp_printf("%s: Warning: %s: %s\n", argv[0], name,
		  strerror(errno));
	return -1;
}

/*
 * This is the public entry point, that is passed both a default name
 * and a default value for the configuration file. This only builds
 * in hosted environment.
 *
 * The function allows "-f <config>" to appear on the command line,
 * and modifies the command line accordingly (hackish, I admit)
 */
int pp_config_file(struct pp_globals *ppg, int *argcp, char **argv,
		   char *default_name, char *default_conf)
{
	char *name = NULL;
	int conf_fd, i, conf_len = 0;
	struct stat conf_fs;
	char *conf_buf;

	/* First, look for fname on the command line (which is parsed later) */
	for (i = 1; i < *argcp - 1; i++) {
		if (strcmp(argv[i], "-f"))
			continue;
		/* found: remove it from argv */
		name = argv[i + 1];
		for (;i < *argcp - 2; i++)
			argv[i] = argv[i + 2];
		*argcp -= 2;
	}
	conf_fd = pp_open_conf_file(argv, name);

	/* Then try the caller-provided name */
	if (conf_fd < 0)
		conf_fd = pp_open_conf_file(argv, default_name);
	/* Finally, the project-wide name */
	if (conf_fd < 0)
		conf_fd = pp_open_conf_file(argv, PP_DEFAULT_CONFIGFILE);

	/* If nothing, use provided string */
	if (conf_fd < 0) {		/* Read it, if any */
		conf_buf = strdup(default_conf);
		conf_len = strlen(conf_buf) - 1;
	} else {
		int r = 0, next_r;

		fstat(conf_fd, &conf_fs);
		conf_buf = calloc(1, conf_fs.st_size + 2);

		do {
			next_r = conf_fs.st_size - conf_len;
			r = read(conf_fd, &conf_buf[conf_len], next_r);
			if (r <= 0)
				break;
			conf_len = strlen(conf_buf);
		} while (conf_len < conf_fs.st_size);

		close(conf_fd);
	}

	/* We need this trailing \n for the parser */
	conf_buf[conf_len + 1] = '\n';

	if ((i = pp_parse_conf(ppg, conf_buf, conf_len)) < 0) {
		pp_printf("%s: configuration: error at line %d\n", argv[0], -1);
		exit(1);
	}
	free(conf_buf);
	return 0;
}
