/*
 * Copyright (C) 2013 CERN (www.cern.ch)
 * Authors: Aurelio Colosimo, Alessandro Rubini
 *
 * Released according to the GNU LGPL, version 2.1 or any later version.
 */

#include <ppsi/ppsi.h>
/* This file is built in hosted environments, so following headers are Ok */
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

/* config lines are global or ppi-specific. Keep track of globals and ppi */
static struct pp_globals *current_ppg;
static struct pp_instance *current_ppi;

/* A "port" (or "link", for compatibility) line creates a ppi instance */
static int f_port(int lineno, int iarg, char *sarg)
{
	if (current_ppg->nlinks >= current_ppg->max_links) {
		pp_printf("config line %i: out of available ports\n",
			  lineno);
		return -1;
	}
	current_ppi = current_ppg->pp_instances + current_ppg->nlinks;

	 /* FIXME: strncpy (it is missing in bare archs by now) */
	strcpy(current_ppi->cfg.port_name, sarg);
	current_ppg->nlinks++;
	return 0;
}

static int f_if(int lineno, int iarg, char *sarg)
{
	if (!current_ppi) {
		pp_printf("config line %i: no port for this config\n", lineno);
		return -1;
	}
	strcpy(current_ppi->cfg.iface_name, sarg);
	return 0;
}

/* The following ones are so similar. Bah... set a pointer somewhere? */
static int f_proto(int lineno, int iarg, char *sarg)
{
	if (!current_ppi) {
		pp_printf("config line %i: no port for this config\n", lineno);
		return -1;
	}
	current_ppi->cfg.proto = iarg;
	return 0;
}

static int f_role(int lineno, int iarg, char *sarg)
{
	if (!current_ppi) {
		pp_printf("config line %i: no port for this config\n", lineno);
		return -1;
	}
	current_ppi->cfg.role = iarg;
	return 0;
}

static int f_ext(int lineno, int iarg, char *sarg)
{
	if (!current_ppi) {
		pp_printf("config line %i: no port for this config\n", lineno);
		return -1;
	}
	current_ppi->cfg.ext = iarg;
	return 0;
}

/* The following two are identical as well. I really need a pointer... */
static int f_class(int lineno, int iarg, char *sarg)
{
	if (current_ppi) {
		pp_printf("config line %i: global config under \"port\"\n",
			  lineno);
		return -1;
	}
	GOPTS(current_ppg)->clock_quality.clockClass = iarg;
	return 0;
}

static int f_accuracy(int lineno, int iarg, char *sarg)
{
	if (current_ppi) {
		pp_printf("config line %i: global config under \"port\"\n",
			  lineno);
		return -1;
	}
	GOPTS(current_ppg)->clock_quality.clockAccuracy = iarg;
	return 0;
}


typedef int (*cfg_handler)(int lineno, int iarg, char *sarg); /* /me lazy... */
/*
 * The parser is using a table, built up by the following structures
 */

struct argname {
	char *name;
	int value;
};
enum argtype {
	ARG_NONE,
	ARG_INT,
	ARG_STR,
	ARG_NAMES,
};
struct argline {
	cfg_handler f;
	char *keyword;	/* Each line starts with a keyword */
	enum argtype t;
	struct argname *args;
};

/* These are the tables for the parser */
static struct argname arg_proto[] = {
	{"raw", PPSI_PROTO_RAW},
	{"udp", PPSI_PROTO_UDP},
	{},
};
static struct argname arg_role[] = {
	{"auto", PPSI_ROLE_AUTO},
	{"master",PPSI_ROLE_MASTER},
	{"slave", PPSI_ROLE_SLAVE},
	{},
};
static struct argname arg_ext[] = {
	{"none", PPSI_EXT_NONE},
	{"whiterabbit", PPSI_EXT_WR},
	{},
};

static struct argline arglines[] = {
	{ f_port,	"port",		ARG_STR},
	{ f_port,	"link",		ARG_STR}, /* old name for "port" */
	{ f_if,		"iface",	ARG_STR},
	{ f_proto,	"proto",	ARG_NAMES,	arg_proto},
	{ f_role,	"role",		ARG_NAMES,	arg_role},
	{ f_ext,	"extension",	ARG_NAMES,	arg_ext},
	{ f_class,	"clock-class",	ARG_INT},
	{ f_accuracy,	"clock-accuracy", ARG_INT},
	{}
};

/* local implementation of isblank() for bare-metal users */
static int blank(int c)
{
	return c == ' '  || c == '\t' || c == '\n';
}

static char *first_word(char *line, char **rest)
{
	char *ret;
	int l = strlen(line) -1;

	/* remove trailing blanks */
	while (l >= 0 && blank(line[l]))
		line [l--] = '\0';

	/* skip leading blanks to find first word */
	while (*line &&  blank(*line))
		line++;
	ret = line;
	/* find next blank and thim there*/
	while (*line && !blank(*line))
		line++;
	if (*line) {
		*line = '\0';
		line++;
	}
	*rest = line;
	return ret;
}

static int pp_parse_line(char *line, int lineno)
{
	struct argline *l;
	struct argname *n;
	char *word;
	int i;

	pp_diag(NULL, config, 2, "parsing line %i: \"%s\"\n", lineno, line);
	word = first_word(line, &line);
	/* now line points to the next word, with no leading blanks */

	if (word[0] == '#')
		return 0;
	if (!*word) {
		/* empty or blank-only */
		current_ppi = NULL;
		return 0;
	}
	for (l = arglines; l->f; l++)
		if (!strcmp(word, l->keyword))
			break;
	if (!l->f) {
		pp_diag(NULL, config, 1, "line %i: no such keyword \"%s\"\n",
			lineno, word);
		return -1;
	}

	switch(l->t) {

	case ARG_NONE:
		break;

	case ARG_INT:
		if (sscanf(line, "%i", &i) != 1) {
			pp_diag(NULL, config, 1, "line %i: \"%s\": not int\n",
				lineno, word);
			return -1;
		}
		if (l->f(lineno, i, NULL))
			return -1;
		break;

	case ARG_STR:
		while (*line && *line == ' ' && *line == '\t')
			line++;
		if (l->f(lineno, 0, line))
			return -1;
		break;

	case ARG_NAMES:
		for (n = l->args; n->name; n++)
			if (!strcmp(line, n->name))
				break;
		if (!n->name) {
			pp_diag(NULL, config, 1, "line %i: wrong arg \"%s\""
				" for \"%s\"\n", lineno, line, word);
			return -1;
		}
		if (l->f(lineno, n->value, NULL))
			return -1;
		break;
	}
	return 0;
}

/* Parse a whole string by splitting line (shoud I fgets instead?) */
static int pp_parse_conf(struct pp_globals *ppg, char *conf, int len)
{
	current_ppg = ppg;
	int errcount = 0;
	char *line, *rest, term;
	int lineno = 0;

	line = conf;
	do {
		lineno++;
		for (rest = line; *rest && *rest != '\n'; rest++)
			;
		term = *rest;
		*rest = '\0';
		if (*line)
			errcount += pp_parse_line(line, lineno) < 0;
		line = rest + 1;
	} while (term); /* if terminator was already 0, we are done */

	return errcount ? -1 : 0;
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

	current_ppg = ppg;

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

	/* We need this trailing \0 for the parser */
	conf_buf[conf_len + 1] = '\0';

	i = pp_parse_conf(ppg, conf_buf, conf_len);
	free(conf_buf);
	return i;
}
