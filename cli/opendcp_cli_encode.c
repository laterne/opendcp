/*
    OpenDCP: Builds Digital Cinema Packages
    Copyright (c) 2010-2014 Terrence Meiczinger, All Rights Reserved

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "opendcp_cli_encode.h"


/* prototypes */
int   options_to_args(cli_t *elements, args_t *args);
int   commands_to_args(cli_t *elements, args_t *args);
int   arguments_to_args(cli_t *elements, args_t *args);
void  print_usage(cli_t *c);

/* print usage */
void print_usage(cli_t *c) {
    int i;

    fprintf(stderr, "Usage: %s [--version] [--help] [options] command <args>\n", c->app_name);
    fprintf(stderr, "\nOptions:\n");
    for (i = 0; i < c->n_options; i++) {
        option_t o = c->options[i];
        fprintf(stderr, "  --%-15.15s  %s [default %s]\n", o.name,  o.description, o.default_value);
    }
    fprintf(stderr, "\nCommands:\n");

    for (i = 0; i < c->n_commands; i++) {
        command_t o = c->commands[i];
        fprintf(stderr, "    %-15.15s  %s\n", o.name,  o.description);
    }
    fprintf(stderr, "\nCommand Usage:\n");
    for (i = 0; i < c->n_commands; i++) {
        command_t o = c->commands[i];
        char *str = strdup(o.args_list);
        char *name;
        fprintf(stderr, "    opendcp_encode [options] %s ", o.name);
        while ((name = strsep(&str, ","))) {
            fprintf(stderr, " <%s>", name);
        }
        fprintf(stderr, "\n");
    }
} 

char *cli_get_name(char *path) {
    char *base = strrchr(path, '/');

    return base ? base+1 : path;
}
     
/* create context */
argv_t argv_create(int argc, char **argv) {
    argv_t a = {argc, argv, 0, argv[0]};

    return a;
}

/* increment argv index */
argv_t *argv_increment(argv_t *a) {
    if (a->i < a->argc) {
        a->current = a->argv[++a->i];
    }

    if (a->i == a->argc) {
        a->current = NULL;
    }

    return a;
}

/* get option from cli */
int get_option(argv_t *a, cli_t *elements) {
    int i;
    int len_prefix;
    int n_options = elements->n_options;
    char *eq = strchr(a->current, '=');
    option_t *option;
    option_t *options = elements->options;
    char *name = &a->current[2];

    len_prefix = (eq-(a->current)) / sizeof(char);

    for (i = 0; i < n_options; i++) {
        option = &options[i];
        if (!strncmp(name, option->name, len_prefix)) {
            break;
        }
    }

    if (i == n_options) {
        fprintf(stderr, "%s is not recognized\n", a->current);
        return 1;
    }

    argv_increment(a);

    if (option->value_required) {
        if (eq == NULL) {
            if (a->current == NULL) {
                fprintf(stderr, "%s requires argument\n", option->name);
                return 1;
            }
            option->value = a->current;
            argv_increment(a);
        }
        else {
            option->value = eq + 1;
        }
    }
    else {
        if (eq != NULL) {
            fprintf(stderr, "%s must not have an argument\n", option->name);
            return 1;
        }
        option->value = "1";
    }

    return 0;
}

/* get commnad from cli */
int get_command(const char *a, cli_t *elements, command_t *c) {
    int i;
    int n_commands = elements->n_commands;

    command_t *command;
    command_t *commands = elements->commands;

    for (i = 0; i < n_commands; i++) {
        command = &commands[i];
        if (!strcmp(command->name, a)) {
            command->value = true;
            memcpy(c, command, sizeof(command_t));
            return true;
        }
    }

    return false;
}

/* set argument element */
int set_argument(char *name, char *value, cli_t *elements) {
    int i;
    int n_arguments = elements->n_arguments;

    argument_t *argument;
    argument_t *arguments = elements->arguments;

    for (i = 0; i < n_arguments; i++) {
        argument = &arguments[i];
        if (!strcmp(argument->name, name) && argument->value == NULL) {
            argument->value = value;
            return true;
        }
    }

    return false;
}

/* get argument from cli */
int get_argument(command_t *command, char *arg_value, cli_t *elements) {
    char *str = strdup(command->args_list);
    char *name;

    /* iterate command arg list */
    while ((name = strsep(&str, ","))) {
        if (set_argument(name, arg_value, elements) == true) {
            return true;
        }
    }

    return false;
}

/* parse input arguments and build elements */
int parse_args(argv_t *a, cli_t *elements) {
    size_t command_found = 0;
    command_t command;

    /* first argv is the command, skip that */
    argv_increment(a);

    while (a->current != NULL) {
        if (a->current[0] == '-' && a->current[1] == '-') {
            if (get_option(a, elements)) {
                return 1;
            }
        }
        else {
            if (!command_found && get_command(a->current, elements, &command) == true) {
                elements->n_commands_found++;
                command_found = 1;
            }
            else if (get_argument(&command, a->current, elements) == true){
                elements->n_arguments_found++;
            }
            argv_increment(a);
        }
    }

    return 0;
}

/* convert option elements to argument structure */
int options_to_args(cli_t *elements, args_t *args) {
    int i, value;
    option_t *option;

    for (i = 0; i < elements->n_options; i++) {
        option = &elements->options[i];
        value = (!strcmp(option->value, "1")) ? 1: 0;
        FOREACH(FLAGS, FLAG_SET)
        FOREACH(OPTIONS, OPTION_SET)
    }

    return 0;
}

int commands_to_args(cli_t *elements, args_t *args) {
    int i;
    command_t *command;

    for (i = 0; i < elements->n_commands; i++) {
        command = &elements->commands[i];
        FOREACH(COMMANDS, COMMAND_SET)
    }

    return 0;
}

/* convert argument elements to argument structure */
int arguments_to_args(cli_t *elements, args_t *args) {
    int i;
    argument_t *argument;

    for (i = 0; i < elements->n_arguments; i++) {
        argument = &elements->arguments[i];
        FOREACH(ARGUMENTS, ARGUMENT_SET)
    }

    return 0;
}

/* main parser */
args_t get_args(int argc, char *argv[]) {
    int i = 0;

    args_t args = {}; 
    argv_t a;

    /* name, desc, enum, args_list, args_required, value */
    command_t commands[] = {
        FOREACH(COMMANDS, COMMAND_INITIALIZE)
        {NULL, NULL, 0, NULL, 0, 0}
    };

    /* name, description, value */
    argument_t arguments[] = {
        FOREACH(ARGUMENTS, ARGUMENT_INITIALIZE)
        {NULL, NULL, NULL}
    };

    /* name, desc, value_required, default, value */
    option_t options[] = {
        FOREACH(OPTIONS, OPTION_INITIALIZE)
        FOREACH(FLAGS, OPTION_INITIALIZE)
        {NULL, NULL, 0, NULL, NULL}
    };
 
    cli_t elements = {0, 0, 0, 0, 0, 0, commands, arguments, options, cli_get_name(argv[0])};

    for (i = 0; commands[i].name != NULL; i++) {
        elements.n_commands++;
    }

    for (i = 0; arguments[i].name != NULL; i++) {
        elements.n_arguments++;
    }

    for (i = 0; options[i].name != NULL; i++) {
        elements.n_options++;
    }

    a = argv_create(argc, argv);
   
    if (parse_args(&a, &elements)) {
        exit(1);
    }

    options_to_args(&elements, &args);
    arguments_to_args(&elements, &args);
    commands_to_args(&elements, &args);

    /* check if help was invoked */
    if (args.help) {
        print_usage(&elements);
        exit(1);
    }

    /* check if version was invoked */
    if  (args.version) {
        return args;
    }

    /* check if command specified */
    if (elements.n_commands_found != 1) {
        //print_usage(&elements);
        fprintf(stderr, "ERROR: No command supplied\n");
        exit(1);
    }

    /* get the command issued (currently sub-commands are not supported) */
    command_t command = elements.commands[0];
    /* check if right amount of arguments was found */
    if (command.args_required != elements.n_arguments_found) {
        fprintf(stderr, "ERROR: Missing arguments %s requires %s\n", command.name, command.args_list);
        exit(1);
    }

    return args;
}

/****************************/
/***** APPLICATION CODE *****/
/****************************/

/* start of application */
int main(int argc, char *argv[]) {
    args_t args = get_args(argc, argv);

    printf("Commands:\n");
    printf("    mxf == %s\n", args.mxf ? "true" : "false");
    printf("    j2k == %s\n", args.j2k ? "true" : "false");
    printf("    stereoscopic == %s\n", args.stereoscopic ? "true" : "false");
    printf("Arguments:\n");
    printf("    input  == %s\n", args.input);
    printf("    output == %s\n", args.output);
    printf("    input_left   == %s\n", args.input_left);
    printf("    input_right  == %s\n", args.input_right);
    printf("    output_left  == %s\n", args.output_left);
    printf("    output_right == %s\n", args.output_right);

    printf("Flags:\n");
    printf("    --help         == %s\n", args.help ? "true" : "false");
    printf("    --version      == %s\n", args.version ? "true" : "false");
    printf("Options:\n");
    printf("    --bw      == %s\n", args.bw);
    printf("    --encoder == %s\n", args.encoder);
    printf("    --profile == %s\n", args.profile);
    printf("    --rate    == %s\n", args.frame_rate);
    printf("    --overwrite == %s\n", args.overwrite);

    if (args.mxf) {
        printf("mxf\n");
    }
    else if (args.j2k) {
        opendcp_command_j2k(&args);
    }

    return 0;
}