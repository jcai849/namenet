#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <graphviz/cgraph.h>
#include "rec.h"

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
	rec_record_t *record;
	rec_field_t field;
	rec_parser_t parser;
	const char *name;

	rec_init();
	record = malloc(sizeof *record);
	for (tail = agfstnode(core_members);
	     tail;
	     tail = agnxtnode(core_members, tail), files++) {
		in = fopen(*files, "r");
		parser = rec_parser_new(in, source);
		while(rec_parse_record(parser, record)) {
			field = rec_record_get_field_by_name(*record, "name", 0);
			name = rec_field_value(field);
			head = agnode(graph,(char *) name, TRUE);
			agedge(graph, tail, head, NULL, TRUE);
		}
		fclose(in);
	}
	rec_fini();
}

int main(int argc, char *argv[]) {
	Agraph_t *graph, *core_members;
	int nmembers = argc - 1;
	char **names = argv+1;

	char **files;
	char *end;
	int i;

	files = malloc((argc - 1) * sizeof *files);
	for (i = 0; i < nmembers; i++) {
		end = strchr(names[i], '=');
		*end = '\0';
		files[i] = ++end;
	}

	graph = agopen("G", Agstrictdirected, NULL);
	core_members = agsubg(graph, "cluster_Team", TRUE);
	populate_graph(core_members, names, nmembers);
	complete_graph(core_members);
	extend_graph(core_members, graph, files);

	agwrite(graph, stdout);
	agclose(graph);
	exit(EXIT_SUCCESS);
}
