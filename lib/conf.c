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

/* A "port" (or "link", for compatibility) line creates or uses a pp instance */
static int f_port(int lineno, union pp_cfg_arg *arg)
{
	int i;

	/* First look for an existing port with the same name */
	for (i = 0; i < current_ppg->nlinks; i++) {
		current_ppi = current_ppg->pp_instances + i;
		if (!strcmp(arg->s, current_ppi->cfg.port_name))
			return 0;
	}
	/* Allocate a new ppi */
	if (current_ppg->nlinks >= current_ppg->max_links) {
		pp_printf("config line %i: out of available ports\n",
			  lineno);
		return -1;
	}
	current_ppi = current_ppg->pp_instances + current_ppg->nlinks;

	 /* FIXME: strncpy (it is missing in bare archs by now) */
	strcpy(current_ppi->cfg.port_name, arg->s);
	current_ppg->nlinks++;
	return 0;
}

#define CHECK_PPI(need) /* Quick hack to factorize errors later */ 	\
	({if (need && !current_ppi) {					\
		pp_printf("config line %i: no port for this config\n", lineno);\
		return -1; \
	} \
	if (!need && current_ppi) { \
		pp_printf("config line %i: global config under \"port\"\n", \
			  lineno); \
		return -1; \
	}})

static int f_if(int lineno, union pp_cfg_arg *arg)
{
	CHECK_PPI(1);
	strcpy(current_ppi->cfg.iface_name, arg->s);
	return 0;
}

/* The following ones are so similar. Bah... set a pointer somewhere? */
static int f_proto(int lineno, union pp_cfg_arg *arg)
{
	CHECK_PPI(1);
	current_ppi->cfg.proto = arg->i;
	return 0;
}

static int f_role(int lineno, union pp_cfg_arg *arg)
{
	CHECK_PPI(1);
	current_ppi->cfg.role = arg->i;
	return 0;
}

static int f_ext(int lineno, union pp_cfg_arg *arg)
{
	CHECK_PPI(1);
	current_ppi->cfg.ext = arg->i;
	return 0;
}

/* The following two are identical as well. I really need a pointer... */
static int f_class(int lineno, union pp_cfg_arg *arg)
{
	CHECK_PPI(0);
	GOPTS(current_ppg)->clock_quality.clockClass = arg->i;
	return 0;
}

static int f_accuracy(int lineno, union pp_cfg_arg *arg)
{
	CHECK_PPI(0);
	GOPTS(current_ppg)->clock_quality.clockAccuracy = arg->i;
	return 0;
}

/* Diagnostics can be per-port or global */
static int f_diag(int lineno, union pp_cfg_arg *arg)
{
	unsigned long level = pp_diag_parse(arg->s);

	if (current_ppi)
		current_ppi->flags = level;
	else
		pp_global_flags = level;
	return 0;
}

/* These are the tables for the parser */
static struct pp_argname arg_proto[] = {
	{"raw", PPSI_PROTO_RAW},
	{"udp", PPSI_PROTO_UDP},
	{},
};
static struct pp_argname arg_role[] = {
	{"auto", PPSI_ROLE_AUTO},
	{"master",PPSI_ROLE_MASTER},
	{"slave", PPSI_ROLE_SLAVE},
	{},
};
static struct pp_argname arg_ext[] = {
	{"none", PPSI_EXT_NONE},
	{"whiterabbit", PPSI_EXT_WR},
	{},
};

static struct pp_argline pp_global_arglines[] = {
	{ f_port,	"port",		ARG_STR},
	{ f_port,	"link",		ARG_STR}, /* old name for "port" */
	{ f_if,		"iface",	ARG_STR},
	{ f_proto,	"proto",	ARG_NAMES,	arg_proto},
	{ f_role,	"role",		ARG_NAMES,	arg_role},
	{ f_ext,	"extension",	ARG_NAMES,	arg_ext},
	{ f_diag,	"diagnostics",	ARG_STR},
	{ f_class,	"clock-class",	ARG_INT},
	{ f_accuracy,	"clock-accuracy", ARG_INT},
	{}
};

/* Provide default empty argument lines for architecture and extension */
struct pp_argline pp_arch_arglines[] __attribute__((weak)) = {
	{}
};
struct pp_argline pp_ext_arglines[] __attribute__((weak)) = {
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

static int pp_config_line(struct pp_globals *ppg, char *line, int lineno)
{
	union pp_cfg_arg cfg_arg;
	struct pp_argline *l;
	struct pp_argname *n;
	char *word;

	current_ppg = ppg;
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

	/* Look for the configuration keyword in global, arch, ext */
	for (l = pp_global_arglines; l->f; l++)
		if (!strcmp(word, l->keyword))
			break;
	if (!l->f)
		for (l = pp_arch_arglines; l->f; l++)
			if (!strcmp(word, l->keyword))
				break;
	if (!l->f)
		for (l = pp_ext_arglines; l->f; l++)
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
		if (sscanf(line, "%i", &(cfg_arg.i)) != 1) {
			pp_diag(NULL, config, 1, "line %i: \"%s\": not int\n",
				lineno, word);
			return -1;
		}
		if (l->f(lineno, &cfg_arg))
			return -1;
		break;

	case ARG_STR:
		while (*line && (*line == ' ' || *line == '\t'))
			line++;

		cfg_arg.s = line;
		if (l->f(lineno, &cfg_arg))
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
		cfg_arg.i = n->value;
		if (l->f(lineno, &cfg_arg))
			return -1;
		break;
	}
	return 0;
}

/* Parse a whole string by splitting lines at '\n' or ';' */
static int pp_parse_conf(struct pp_globals *ppg, char *conf, int len)
{
	int errcount = 0;
	char *line, *rest, term;
	int lineno = 1;

	line = conf;
	do {
		for (rest = line;
		     *rest && *rest != '\n' && *rest != ';'; rest++)
			;
		term = *rest;
		*rest = '\0';
		if (*line)
			errcount += pp_config_line(ppg, line, lineno) < 0;
		line = rest + 1;
		if (term == '\n')
			lineno++;
	} while (term); /* if terminator was already 0, we are done */
	pp_config_line(ppg, "", 0); /* done: clear current_ppi */
	return errcount ? -1 : 0;
}

/* Open a file, warn if not found */
static int pp_open_conf_file(char *name)
{
	int fd;

	if (!name)
		return -1;
	fd = open(name, O_RDONLY);
	if (fd >= 0) {
		pp_printf("Using config file %s\n", name);
		return fd;
	}
	pp_printf("Warning: %s: %s\n", name, strerror(errno));
	return -1;
}

/*
 * This is one of the public entry points, opening a file
 *
 * "force" means that the called wants to open *this file.
 * In this case we count it even if it fails, so the default
 * config file is not used
 */
int pp_config_file(struct pp_globals *ppg, int force, char *fname)
{
	int conf_fd, i, conf_len = 0;
	int r = 0, next_r;
	struct stat conf_fs;
	char *conf_buf;

	conf_fd = pp_open_conf_file(fname);
	if (conf_fd < 0) {
		if (force)
			ppg->cfg_items++;
		return -1;
	}
	ppg->cfg_items++;

	/* read the whole file, it is split up later on */

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

	i = pp_parse_conf(ppg, conf_buf, conf_len);
	free(conf_buf);
	return i;
}

/*
 * This is the other public entry points, opening a file
 */
int pp_config_string(struct pp_globals *ppg, char *s)
{
	return pp_parse_conf(ppg, s, strlen(s));
}

