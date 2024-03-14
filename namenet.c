#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <graphviz/cgraph.h>
#include <errno.h>
#include <regex.h>
#include "rec.h"

typedef enum {
	RETURN_OK,
	INVALID_ARGS,
	FILE_NOT_FOUND,
	NO_MEM,
	OTHER,
	RETVAL_COUNT
} namenet_retval;
const char * const RETVALS[] = {
	"OK",
	"INVALID ARGS",
	"FILE NOT FOUND",
	"NO MEMORY",
	"OTHER",
	"RETVAL ARRAY OUT OF SYNC"
};
const char *namenet_errstr(namenet_retval retval) {
	if (retval > RETVAL_COUNT) retval = RETVAL_COUNT;
	return RETVALS[retval];
}
void abort_w_msg(namenet_retval err) {
	fprintf(stderr, "%s\n", namenet_errstr(err));
	abort();
}

void populate_graph(Agraph_t *graph, char **names, int count) {
	int i;
	for (i = 0; i < count; i++) agnode(graph, names[i], TRUE);
}

void complete_graph(Agraph_t *graph) {
	Agnode_t *tail, *head;

	for (tail = agfstnode(graph); tail; tail = agnxtnode(graph, tail))
	for (head = agfstnode(graph); head; head = agnxtnode(graph, head))
	if (tail != head) agedge(graph, tail, head, NULL, TRUE);
}

void extend_graph(Agraph_t *core_members, Agraph_t *graph, char **files) {
	Agnode_t *tail, *head;
	FILE *in;
	char source[BUFSIZ];
	rec_record_t record;
	rec_field_t field;
	rec_parser_t parser;
	const char *name;

	rec_init();
	for (tail = agfstnode(core_members); tail;
	     tail = agnxtnode(core_members, tail), files++) {
		if (!(in = fopen(*files, "r"))) {
			perror(NULL);
			abort();
		};
		parser = rec_parser_new(in, source);
		while(rec_parse_record(parser, &record)) {
			field = rec_record_get_field_by_name(record, "name", 0);
			name = rec_field_value(field);
			head = agnode(graph,(char *) name, TRUE);
			agedge(graph, tail, head, NULL, TRUE);
		}
		fclose(in);
	}
	rec_fini();
}

void sep_tags_vals(char **tags, char **vals, int len) {
	int i;
	char *end;
	for (i = 0; i < len; i++) {
		end = strchr(tags[i], '=');
		*end = '\0';
		vals[i] = ++end;
	}
}

int clear_strs(char **strs, int len) {
	int i;
	for (i = 0; i < len; i++) {
		while (*strs[i] == '\'' || *strs[i] == '"') {
			strs[i]++;
		}
		while (strs[i][strlen(strs[i])-1] == '\'' ||
		       strs[i][strlen(strs[i])-1] == '"') {
			strs[i][strlen(strs[i])-1] = '\0';
		}
	}
	return 0;
}

void create_graph(Agraph_t *graph, char **names, int nmembers, char **files) {
	Agraph_t *core_members;

	core_members = agsubg(graph, "cluster_Team", TRUE);
	populate_graph(core_members, names, nmembers);
	complete_graph(core_members);
	extend_graph(core_members, graph, files);
}

int check_args(char **args) {
	static const char *const re = "[^=]+=[^=]+";
	regex_t regex;

	if (!*args) return 1;

	if (regcomp(&regex, re, REG_EXTENDED | REG_NOSUB)) return 1;
	do {
		if (regexec(&regex, *args, 0, NULL, 0)) {
			regfree(&regex);
			return 1;
		}
	} while (*++args);
	regfree(&regex);
	return 0;
}


int main(int argc, char *argv[]) {
	Agraph_t *graph;
	int nmembers = argc - 1;
	char **files, **names = argv+1;
	namenet_retval err;

	if ((err = check_args(names))) abort_w_msg(err);
	if (!(files = malloc((argc - 1) * sizeof *files)))
		abort_w_msg(NO_MEM);
	sep_tags_vals(names, files, nmembers);
	clear_strs(names, nmembers); clear_strs(files, nmembers);

	graph = agopen("G", Agstrictdirected, NULL);
	create_graph(graph, names, nmembers, files);

	free(files);

	agwrite(graph, stdout);
	agclose(graph);
	exit(EXIT_SUCCESS);
}
