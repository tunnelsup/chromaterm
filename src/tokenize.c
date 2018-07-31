/******************************************************************************
*   This program is protected under the GNU GPL (See COPYING)                 *
*                                                                             *
*   This program is free software; you can redistribute it and/or modify      *
*   it under the terms of the GNU General Public License as published by      *
*   the Free Software Foundation; either version 2 of the License, or         *
*   (at your option) any later version.                                       *
*                                                                             *
*   This program is distributed in the hope that it will be useful,           *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
*   GNU General Public License for more details.                              *
*                                                                             *
*   You should have received a copy of the GNU General Public License         *
*   along with this program; if not, write to the Free Software               *
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA *
******************************************************************************/

/******************************************************************************
*                  C H R O M A T E R M (C) 2013 (See CREDITS)                 *
******************************************************************************/

#include "defs.h"

struct scriptdata
{
	long long              min;
	long long              max;
	long long              cnt;
	int                    inc;
	char                 * cpy;
	char                 * str;
	char                 * arg;
};

struct scriptnode
{
	struct scriptnode    * next;
	struct scriptnode    * prev;
	union
	{
		struct scriptdata   * data;
		struct script_regex * regex;
	};
	char                 * str;
	short                  lvl;
	short                  type;
	short                  cmd;
};

struct script_regex
{
	char                 * str;
	char                 * bod;
	char                 * buf;
	int                    val;
};

struct scriptroot
{
	struct scriptnode    * next;
	struct scriptnode    * prev;
	struct session       * ses;
};

void debugtoken(struct session *ses, struct scriptnode *token)
{
	push_call("debugtoken(%p,%p,%d)",ses,token,token->type);

	if (ses->debug_level)
	{
		switch (token->type)
		{
			case TOKEN_TYPE_STRING:
			case TOKEN_TYPE_SESSION:
				display_printf2(ses, "[%02d] %*s%s", token->type, token->lvl * 4, "", token->str);
				break;

			case TOKEN_TYPE_ELSE:
			case TOKEN_TYPE_END:
				display_printf2(ses, "[%02d] %*s\033[1;32m%s\033[0m", token->type, token->lvl * 4, "", token->str);
				break;

			case TOKEN_TYPE_DEFAULT:
				display_printf2(ses, "[%02d] %*s\033[1;32m%s\033[0m", token->type, token->lvl * 4, "", command_table[token->cmd].name);
				break;

			case TOKEN_TYPE_BREAK:
			case TOKEN_TYPE_CONTINUE:
				display_printf2(ses, "[%02d] %*s\033[1;31m%s\033[0m", token->type, token->lvl * 4, "", command_table[token->cmd].name);
				break;

			case TOKEN_TYPE_COMMAND:
				display_printf2(ses, "[%02d] %*s\033[1;36m%s\033[0m %s\033[0m", token->type, token->lvl * 4, "", command_table[token->cmd].name, token->str);
				break;

			case TOKEN_TYPE_RETURN:
				display_printf2(ses, "[%02d] %*s\033[1;31m%s\033[0m %s\033[0m", token->type, token->lvl * 4, "", command_table[token->cmd].name, token->str);
				break;

			case TOKEN_TYPE_CASE:
			case TOKEN_TYPE_ELSEIF:
			case TOKEN_TYPE_IF:
			case TOKEN_TYPE_FOREACH:
			case TOKEN_TYPE_LOOP:
			case TOKEN_TYPE_PARSE:
			case TOKEN_TYPE_SWITCH:
			case TOKEN_TYPE_WHILE:
				display_printf2(ses, "[%02d] %*s\033[1;32m%s {\033[0m%s\033[1;32m}\033[0m", token->type, token->lvl * 4, "", command_table[token->cmd].name, token->str);
				break;

			case TOKEN_TYPE_REGEX:
				display_printf2(ses, "[%02d] %*s\033[1;32m%s {\033[0m%s\033[1;32m} {\033[0m%s\033[1;32m}\033[0m", token->type, token->lvl * 4, "", command_table[token->cmd].name, token->str, token->regex->str);
				break;

			default:
				if (token == (struct scriptnode *) ses)
				{
					display_printf2(ses, "[--] (error) token == ses");
				}
				else
				{
					display_printf2(ses, "[%02d] %*s\033[1;33m%d {\033[0m%s\033[1;32m}\033[0m", token->type, token->lvl * 4, "", token->cmd, token->str);
				}
				break;
		}
	}
	pop_call();
	return;
}


void addtoken(struct scriptroot *root, int lvl, int opr, int cmd, char *str)
{
	struct scriptnode *token;

	token = (struct scriptnode *) calloc(1, sizeof(struct scriptnode));

	token->lvl = lvl;
	token->type = opr;
	token->cmd = cmd;
	token->str = strdup(str);

	LINK(token, root->next, root->prev);
}


char *addlooptoken(struct scriptroot *root, int lvl, int opr, int cmd, char *str)
{
	struct scriptdata *data;

	char min[BUFFER_SIZE], max[BUFFER_SIZE], var[BUFFER_SIZE];

	data = (struct scriptdata *) calloc(1, sizeof(struct scriptdata));

	str = get_arg_in_braces(root->ses, str, min, FALSE);
	str = get_arg_in_braces(root->ses, str, max, FALSE);
	str = get_arg_in_braces(root->ses, str, var, FALSE);

	data->cpy = refstring(NULL, "{%s} {%s} {%s}", min, max, var);

	addtoken(root, lvl, opr, cmd, var);

	data->str = strdup("");

	root->prev->data = data;

	return str;
}

char *addswitchtoken(struct scriptroot *root, int lvl, int opr, int cmd, char *str)
{
	struct scriptdata *data;

	char arg[BUFFER_SIZE];

	data = (struct scriptdata *) calloc(1, sizeof(struct scriptdata));

	str = get_arg_in_braces(root->ses, str, arg, FALSE);

	data->cpy = strdup("");

	addtoken(root, lvl, opr, cmd, arg);

	data->str = strdup("");

	root->prev->data = data;

	return str;
}

void resetlooptoken(struct session *ses, struct scriptnode *token)
{
	char *str, min[BUFFER_SIZE], max[BUFFER_SIZE];

	str = token->data->cpy;

	str = get_arg_in_braces(ses, str, min, FALSE);
	str = get_arg_in_braces(ses, str, max, FALSE);

	token->data->min = (int) get_number(ses, min);
	token->data->max = (int) get_number(ses, max);

	token->data->inc = token->data->min <= token->data->max ? 1 : -1;
	token->data->cnt = token->data->min;
}

void breaklooptoken(struct scriptnode *token)
{
	token->data->min = token->data->max = token->data->cnt = token->data->inc = 0;
}

char *addforeachtoken(struct scriptroot *root, int lvl, int opr, int cmd, char *str)
{
	struct scriptdata *data;

	char arg[BUFFER_SIZE], var[BUFFER_SIZE];

	str = get_arg_in_braces(root->ses, str, arg, FALSE);
	str = get_arg_in_braces(root->ses, str, var, FALSE);

	addtoken(root, lvl, opr, cmd, var);

	data = (struct scriptdata *) calloc(1, sizeof(struct scriptdata));

	data->cpy = refstring(NULL, "{%s} {%s}", arg, var);
	data->str = strdup("");
	data->arg = data->str;

	root->prev->data = data;

	return str;
}

void resetforeachtoken(struct session *ses, struct scriptnode *token)
{
	char *str, arg[BUFFER_SIZE];

	str = token->data->cpy;

	str = sub_arg_in_braces(ses, str, arg, GET_ONE, SUB_VAR|SUB_FUN);

	RESTRING(token->data->str, arg);

	token->data->arg = token->data->str;
}

void breakforeachtoken(struct scriptnode *token)
{
	RESTRING(token->data->str, "");

	token->data->arg = token->data->str;
}

void handlereturntoken(struct session *ses, struct scriptnode *token)
{
	char arg[BUFFER_SIZE];

	substitute(ses, token->str, arg, SUB_VAR|SUB_FUN);

	set_nest_node(ses->list[LIST_VARIABLE], "result", "%s", arg);
}

void handleswitchtoken(struct session *ses, struct scriptnode *token)
{
	char arg[BUFFER_SIZE];

	mathexp(ses, token->str, arg);

	RESTRING(token->data->str, arg);
}

char *get_arg_foreach(struct scriptroot *root, struct scriptnode *token)
{
	static char buf[BUFFER_SIZE];

	token->data->arg = get_arg_in_braces(root->ses, token->data->arg, buf, TRUE);

	if (*token->data->arg == COMMAND_SEPARATOR)
	{
		token->data->arg++;
	}
	return buf;
}

char *addparsetoken(struct scriptroot *root, int lvl, int opr, int cmd, char *str)
{
	struct scriptdata *data;

	char arg[BUFFER_SIZE], var[BUFFER_SIZE];

	str = get_arg_in_braces(root->ses, str, arg, FALSE);
	str = get_arg_in_braces(root->ses, str, var, FALSE);

	addtoken(root, lvl, opr, cmd, var);

	data = (struct scriptdata *) calloc(1, sizeof(struct scriptdata));

	data->cpy = refstring(NULL, "{%s} {%s}", arg, var);

	data->str = strdup("");
	data->arg = data->str;

	root->prev->data = data;

	return str;
}

void resetparsetoken(struct session *ses, struct scriptnode *token)
{
	char *str, arg[BUFFER_SIZE];

	str = token->data->cpy;

	str = sub_arg_in_braces(ses, str, arg, GET_ONE, SUB_VAR|SUB_FUN);

	RESTRING(token->data->str, arg);

	token->data->arg = token->data->str;
}

void breakparsetoken(struct scriptnode *token)
{
	RESTRING(token->data->str, "");

	token->data->arg = token->data->str;
}

char *get_arg_parse(struct session *ses, struct scriptnode *token)
{
	static char buf[5];

	if (HAS_BIT(ses->flags, SES_FLAG_BIG5) && token->data->arg[0] & 128 && token->data->arg[1] != 0)
	{
		token->data->arg += sprintf(buf, "%c%c", token->data->arg[0], token->data->arg[1]);
	}
	else if (HAS_BIT(ses->flags, SES_FLAG_UTF8) && (token->data->arg[0] & 192) == 192 && token->data->arg[1] != 0)
	{
		if ((token->data->arg[0] & 240) == 240 && token->data->arg[2] != 0 && token->data->arg[3] != 0)
		{
			token->data->arg += sprintf(buf, "%c%c%c%c", token->data->arg[0], token->data->arg[1], token->data->arg[2], token->data->arg[3]);
		}
		else if ((token->data->arg[0] & 224) == 224 && token->data->arg[2] != 0)
		{
			token->data->arg += sprintf(buf, "%c%c%c", token->data->arg[0], token->data->arg[1], token->data->arg[2]);
		}
		else
		{
			token->data->arg += sprintf(buf, "%c%c", token->data->arg[0], token->data->arg[1]);
		}
	}
	else
	{
		token->data->arg += sprintf(buf, "%c", token->data->arg[0]);
	}

	return buf;
}

char *addregextoken(struct scriptroot *root, int lvl, int type, int cmd, char *str)
{
	struct script_regex *regex;

	char arg1[BUFFER_SIZE], arg2[BUFFER_SIZE], arg3[BUFFER_SIZE];

	str = get_arg_in_braces(root->ses, str, arg1, FALSE);
	str = get_arg_in_braces(root->ses, str, arg2, FALSE);
	str = get_arg_in_braces(root->ses, str, arg3, TRUE);

	addtoken(root, lvl, type, cmd, arg1);

	regex = (struct script_regex *) calloc(1, sizeof(struct script_regex));

	regex->str = strdup(arg2);
	regex->bod = strdup(arg3);
	regex->buf = calloc(1, BUFFER_SIZE);

	root->prev->regex = regex;

	return str;
}

void deltoken(struct scriptroot *root, struct scriptnode *token)
{
	push_call("deltoken(%p,%p)",root,token);

	UNLINK(token, root->next, root->prev);

	free(token->str);

	switch (token->type)
	{
		case TOKEN_TYPE_REGEX:
			free(token->regex->str);
			free(token->regex->bod);
			free(token->regex->buf);
			free(token->regex);
			break;

		case TOKEN_TYPE_LOOP:
		case TOKEN_TYPE_FOREACH:
		case TOKEN_TYPE_PARSE:
		case TOKEN_TYPE_SWITCH:
			free(token->data->cpy);
			free(token->data->str);
			free(token->data);
			break;
	}

	free(token);

	pop_call();
	return;
}


int find_command(char *command)
{
	struct session *ses;
	int cmd;

	for (ses = gts ; ses ; ses = ses->next)
	{
		if (!strcmp(ses->name, command))
		{
			return -1;
		}
	}

	if (isalpha((int) *command))
	{
		for (cmd = gtd->command_ref[tolower((int) *command) - 'a'] ; *command_table[cmd].name ; cmd++)
		{
			if (is_abbrev(command, command_table[cmd].name))
			{
				return cmd;
			}
		}
	}
	return -1;
}


void tokenize_script(struct scriptroot *root, int lvl, char *str)
{
	char *arg, *line;
	int cmd;

	if (*str == 0)
	{
		addtoken(root, lvl, TOKEN_TYPE_STRING, -1, "");

		return;
	}

	line = (char *) calloc(1, BUFFER_SIZE);

	while (*str)
	{
		if (!VERBATIM(root->ses))
		{
			str = space_out(str);
		}

		if (*str != gtd->command_char)
		{
			str = get_arg_all(root->ses, str, line, VERBATIM(root->ses));

			addtoken(root, lvl, TOKEN_TYPE_STRING, -1, line);
		}
		else
		{
			arg = get_arg_stop_spaces(root->ses, str, line, 0);

			cmd = find_command(line+1);

			if (cmd == -1)
			{
				str = get_arg_all(root->ses, str, line, FALSE);
				addtoken(root, lvl, TOKEN_TYPE_SESSION, -1, line+1);
			}
			else
			{
				switch (command_table[cmd].type)
				{
					case TOKEN_TYPE_BREAK:
						str = get_arg_with_spaces(root->ses, arg, line, 1);
						addtoken(root, lvl, TOKEN_TYPE_BREAK, cmd, line);
						break;

					case TOKEN_TYPE_CASE:
						str = get_arg_in_braces(root->ses, arg, line, FALSE);
						addtoken(root, lvl++, TOKEN_TYPE_CASE, cmd, line);

						str = get_arg_in_braces(root->ses, str, line, TRUE);
						tokenize_script(root, lvl--, line);

						addtoken(root, lvl, TOKEN_TYPE_END, -1, "endcase");
						break;

					case TOKEN_TYPE_CONTINUE:
						str = get_arg_with_spaces(root->ses, arg, line, 1);
						addtoken(root, lvl, TOKEN_TYPE_CONTINUE, cmd, line);
						break;

					case TOKEN_TYPE_DEFAULT:
						addtoken(root, lvl++, TOKEN_TYPE_DEFAULT, cmd, "");

						str = get_arg_in_braces(root->ses, arg, line, TRUE);
						tokenize_script(root, lvl--, line);

						addtoken(root, lvl, TOKEN_TYPE_END, -1, "enddefault");
						break;

					case TOKEN_TYPE_ELSE:
						addtoken(root, lvl++, TOKEN_TYPE_ELSE, cmd, "else");

						str = get_arg_in_braces(root->ses, arg, line, TRUE);
						tokenize_script(root, lvl--, line);

						addtoken(root, lvl, TOKEN_TYPE_END, -1, "endelse");
						break;

					case TOKEN_TYPE_ELSEIF:
						str = get_arg_in_braces(root->ses, arg, line, FALSE);
						addtoken(root, lvl++, TOKEN_TYPE_ELSEIF, cmd, line);

						str = get_arg_in_braces(root->ses, str, line, TRUE);
						tokenize_script(root, lvl--, line);

						addtoken(root, lvl, TOKEN_TYPE_END, -1, "endif");
						break;

					case TOKEN_TYPE_FOREACH:
						str = addforeachtoken(root, lvl++, TOKEN_TYPE_FOREACH, cmd, arg);

						str = get_arg_in_braces(root->ses, str, line, TRUE);
						tokenize_script(root, lvl--, line);

						addtoken(root, lvl, TOKEN_TYPE_END, -1, "endforeach");
						break;

					case TOKEN_TYPE_IF:
						str = get_arg_in_braces(root->ses, arg, line, FALSE);
						addtoken(root, lvl++, TOKEN_TYPE_IF, cmd, line);

						str = get_arg_in_braces(root->ses, str, line, TRUE);
						tokenize_script(root, lvl--, line);

						addtoken(root, lvl, TOKEN_TYPE_END, -1, "endif");

						if (*str && *str != COMMAND_SEPARATOR)
						{
							addtoken(root, lvl++, TOKEN_TYPE_ELSE, -1, "else");

							str = get_arg_in_braces(root->ses, str, line, TRUE);
							tokenize_script(root, lvl--, line);

							addtoken(root, lvl, TOKEN_TYPE_END, -1, "endif");
						}
						break;

					case TOKEN_TYPE_LOOP:
						str = addlooptoken(root, lvl++, TOKEN_TYPE_LOOP, cmd, arg);

						str = get_arg_in_braces(root->ses, str, line, TRUE);
						tokenize_script(root, lvl--, line);

						addtoken(root, lvl, TOKEN_TYPE_END, -1, "endloop");
						break;

					case TOKEN_TYPE_PARSE:
						str = addparsetoken(root, lvl++, TOKEN_TYPE_PARSE, cmd, arg);

						str = get_arg_in_braces(root->ses, str, line, TRUE);
						tokenize_script(root, lvl--, line);

						addtoken(root, lvl, TOKEN_TYPE_END, -1, "endparse");
						break;

					case TOKEN_TYPE_REGEX:
						str = addregextoken(root, lvl, TOKEN_TYPE_REGEX, cmd, arg);

//						addtoken(root, --lvl, TOKEN_TYPE_END, -1, "endregex");

						if (*str && *str != COMMAND_SEPARATOR)
						{
							addtoken(root, lvl++, TOKEN_TYPE_ELSE, -1, "else");

							str = get_arg_in_braces(root->ses, str, line, TRUE);
							tokenize_script(root, lvl--, line);

							addtoken(root, lvl, TOKEN_TYPE_END, -1, "endregex");
						}
						break;

					case TOKEN_TYPE_RETURN:
						str = get_arg_with_spaces(root->ses, arg, line, 1);
						addtoken(root, lvl, TOKEN_TYPE_RETURN, cmd, line);
						break;

					case TOKEN_TYPE_SWITCH:
						str = addswitchtoken(root, lvl++, TOKEN_TYPE_SWITCH, cmd, arg);

						str = get_arg_in_braces(root->ses, str, line, TRUE);
						tokenize_script(root, lvl--, line);

						addtoken(root, lvl, TOKEN_TYPE_END, -1, "end");
						break;

					case TOKEN_TYPE_WHILE:
						str = get_arg_in_braces(root->ses, arg, line, FALSE);
						addtoken(root, lvl++, TOKEN_TYPE_WHILE, cmd, line);

						str = get_arg_in_braces(root->ses, str, line, TRUE);
						tokenize_script(root, lvl--, line);

						addtoken(root, lvl, TOKEN_TYPE_END, -1, "endwhile");
						break;

					default:
						str = get_arg_with_spaces(root->ses, arg, line, 1);
						addtoken(root, lvl, TOKEN_TYPE_COMMAND, cmd, line);
						break;
				}
			}
		}
		if (*str == COMMAND_SEPARATOR)
		{
			str++;
		}

	}

	free(line);
}


struct scriptnode *parse_script(struct scriptroot *root, int lvl, struct scriptnode *token, struct scriptnode *shift)
{
	struct scriptnode *split = NULL;

	while (token)
	{
		if (token->lvl < lvl)
		{
			if (shift->lvl + 1 == lvl)
			{
				switch (shift->type)
				{
					case TOKEN_TYPE_FOREACH:
					case TOKEN_TYPE_LOOP:
					case TOKEN_TYPE_PARSE:
					case TOKEN_TYPE_WHILE:
						debugtoken(root->ses, token);
						return shift;

					case TOKEN_TYPE_BROKEN_FOREACH:
					case TOKEN_TYPE_BROKEN_LOOP:
					case TOKEN_TYPE_BROKEN_PARSE:
					case TOKEN_TYPE_BROKEN_WHILE:
						shift->type--;
						return token;
				}
			}
			return token;
		}

		debugtoken(root->ses, token);

		switch (token->type)
		{
			case TOKEN_TYPE_BREAK:
				switch (shift->type)
				{
					case TOKEN_TYPE_FOREACH:
						breakforeachtoken(shift);
						shift->type++;
						break;
					case TOKEN_TYPE_LOOP:
						breaklooptoken(shift);
						shift->type++;
						break;
					case TOKEN_TYPE_PARSE:
						breakparsetoken(shift);
						shift->type++;
						break;
					case TOKEN_TYPE_WHILE:
						shift->type++;
						break;
				}

				do
				{
					token = token->next;
				}
				while (token && token->lvl > shift->lvl);

				continue;

			case TOKEN_TYPE_CASE:
				if (shift->data && mathswitch(root->ses, shift->data->str, token->str))
				{

					token = token->next;

					token = parse_script(root, lvl + 1, token, shift);

					while (token && token->lvl >= lvl)
					{
						token = token->next;
					}
				}
				else
				{
					do
					{
						token = token->next;
					}
					while (token && token->lvl > lvl);
				}
				continue;

			case TOKEN_TYPE_COMMAND:
				root->ses = (*command_table[token->cmd].command) (root->ses, token->str);
				break;

			case TOKEN_TYPE_CONTINUE:

				do
				{
					token = token->next;
				}
				while (token && token->lvl > shift->lvl);

				continue;

			case TOKEN_TYPE_DEFAULT:
				token = token->next;

				token = parse_script(root, lvl + 1, token, shift);

				while (token && token->lvl >= lvl)
				{
					token = token->next;
				}
				continue;

			case TOKEN_TYPE_ELSE:
				if (split)
				{
					token = parse_script(root, lvl + 1, token->next, shift);

					split = NULL;
				}
				else
				{
					do
					{
						token = token->next;
					}
					while (token && token->lvl > lvl);
				}
				continue;

			case TOKEN_TYPE_ELSEIF:
				if (split && get_number(root->ses, token->str))
				{
					token = parse_script(root, lvl + 1, token->next, shift);

					split = NULL;
				}
				else
				{
					do
					{
						token = token->next;
					}
					while (token && token->lvl > lvl);
				}
				continue;

			case TOKEN_TYPE_END:
				break;

			case TOKEN_TYPE_FOREACH:
				if (*token->data->arg == 0)
				{
					resetforeachtoken(root->ses, token);
				}

				if (*token->data->arg == 0)
				{
					token->type++;

					do
					{
						token = token->next;
					}
					while (token && token->lvl > lvl);
				}
				else
				{
					set_nest_node(root->ses->list[LIST_VARIABLE], token->str, "%s", get_arg_foreach(root, token));

					if (*token->data->arg == 0)
					{
						token->type++;
					}
					token = parse_script(root, lvl + 1, token->next, token);
				}
				continue;

			case TOKEN_TYPE_IF:
				split = NULL;

				if (get_number(root->ses, token->str))
				{
					token = parse_script(root, lvl + 1, token->next, shift);
				}
				else
				{
					split = token;

					do
					{
						token = token->next;
					}
					while (token && token->lvl > lvl);
				}
				continue;

			case TOKEN_TYPE_LOOP:
				if (token->data->cnt == token->data->max + token->data->inc)
				{
					resetlooptoken(root->ses, token);
				}

				set_nest_node(root->ses->list[LIST_VARIABLE], token->str, "%lld", token->data->cnt);

				token->data->cnt += token->data->inc;

				if (token->data->cnt == token->data->max + token->data->inc)
				{
					token->type++;
				}

				token = parse_script(root, lvl + 1, token->next, token);

				continue;

			case TOKEN_TYPE_PARSE:
				if (*token->data->arg == 0)
				{
					resetparsetoken(root->ses, token);

					if (*token->data->arg == 0)
					{
						token->type++;

						do
						{
							token = token->next;
						}
						while (token && token->lvl > lvl);

						continue;
					}

				}

				set_nest_node(root->ses->list[LIST_VARIABLE], token->str, "%s", get_arg_parse(root->ses, token));

				if (*token->data->arg == 0)
				{
					token->type++;
				}
				token = parse_script(root, lvl + 1, token->next, token);

				continue;

			case TOKEN_TYPE_REGEX:
				split = NULL;

				token->regex->val = find(root->ses, token->str, token->regex->str, SUB_CMD);

				if (token->regex->val)
				{
					substitute(root->ses, token->regex->bod, token->regex->buf, SUB_CMD);

					root->ses = script_driver(root->ses, -1, token->regex->buf);
				}
				else
				{
					split = token;
				}
				break;

			case TOKEN_TYPE_RETURN:
				handlereturntoken(root->ses, token);

				if (lvl)
				{
					return NULL;
				}
				else
				{
					return (struct scriptnode *) root->ses;
				}
				break;

			case TOKEN_TYPE_SESSION:
				root->ses = parse_command(root->ses, token->str);
				break;

			case TOKEN_TYPE_STRING:
				root->ses = parse_input(root->ses, token->str);
				break;

			case TOKEN_TYPE_SWITCH:
				handleswitchtoken(root->ses, token);

				token = parse_script(root, lvl + 1, token->next, token);
				continue;

			case TOKEN_TYPE_WHILE:
				if (get_number(root->ses, token->str))
				{
					token = parse_script(root, lvl + 1, token->next, token);
				}
				else
				{
					token->type++;

					do
					{
						token = token->next;
					}
					while (token && token->lvl > lvl);
				}
				continue;
		}

		if (token)
		{
			token = token->next;
		}
	}
	if (lvl)
	{
		return NULL;
	}
	else
	{
		return (struct scriptnode *) root->ses;
	}
}


char *write_script(struct session *ses, struct scriptroot *root)
{
	struct scriptnode *token;
	static char buf[STRING_SIZE];

	token = root->next;

	buf[0] = 0;

	while (token)
	{
		switch (token->type)
		{
			case TOKEN_TYPE_STRING:
				cat_sprintf(buf, "%s%s", indent(token->lvl), token->str);
				break;

			case TOKEN_TYPE_BREAK:
			case TOKEN_TYPE_CONTINUE:
				cat_sprintf(buf, "%s%c%s", indent(token->lvl), gtd->command_char, command_table[token->cmd].name);
				break;

			case TOKEN_TYPE_COMMAND:
			case TOKEN_TYPE_RETURN:
				cat_sprintf(buf, "%s%c%s%s%s", indent(token->lvl), gtd->command_char, command_table[token->cmd].name, *token->str ? " " : "", token->str);
				break;

			case TOKEN_TYPE_ELSE:
				cat_sprintf(buf, "%s%c%s\n%s{\n", indent(token->lvl), gtd->command_char, token->str, indent(token->lvl));
				break;

			case TOKEN_TYPE_DEFAULT:
				cat_sprintf(buf, "%s%c%s\n%s{\n", indent(token->lvl), gtd->command_char, command_table[token->cmd].name, indent(token->lvl));
				break;

			case TOKEN_TYPE_FOREACH:
			case TOKEN_TYPE_LOOP:
			case TOKEN_TYPE_PARSE:
				cat_sprintf(buf, "%s%c%s %s\n%s{\n", indent(token->lvl), gtd->command_char, command_table[token->cmd].name, token->data->cpy, indent(token->lvl));
				break;

			case TOKEN_TYPE_CASE:
			case TOKEN_TYPE_ELSEIF:
			case TOKEN_TYPE_IF:
			case TOKEN_TYPE_SWITCH:
			case TOKEN_TYPE_WHILE:
				cat_sprintf(buf, "%s%c%s {%s}\n%s{\n", indent(token->lvl), gtd->command_char, command_table[token->cmd].name, token->str, indent(token->lvl));
				break;

			case TOKEN_TYPE_REGEX:
				cat_sprintf(buf, "%s%c%s {%s} {%s} {%s}", indent(token->lvl), gtd->command_char, command_table[token->cmd].name, token->str, token->regex->str, token->regex->bod);
				break;

			case TOKEN_TYPE_END:
				cat_sprintf(buf, "\n%s}", indent(token->lvl));
				break;

			case TOKEN_TYPE_SESSION:
				cat_sprintf(buf, "%s%c%s", indent(token->lvl), gtd->command_char, token->str);
				break;

			default:
				display_printf2(ses, "#WRITE: UNKNOWN TOKEN TYPE: %d", token->type);
				break;
		}

		if (token->next && token->lvl == token->next->lvl)
		{
			strcat(buf, ";\n");
		}

		token = token->next;
	}

	while (root->next)
	{
		deltoken(root, root->next);
	}

	free(root);

	return buf;
}

struct session *script_driver(struct session *ses, int list, char *str)
{
	struct scriptroot *root;
	struct session *cur_ses;
	int dlevel, ilevel;

	root = (struct scriptroot *) calloc(1, sizeof(struct scriptroot));

	root->ses = cur_ses = ses;

	dlevel = (list >= 0) ? HAS_BIT(root->ses->list[list]->flags, LIST_FLAG_DEBUG) : 0;
	ilevel = (list >= 0) ? 1 : 0;

	cur_ses->debug_level += dlevel;
	cur_ses->input_level += ilevel;

	tokenize_script(root, 0, str);

	ses = (struct session *) parse_script(root, 0, root->next, root->prev);

	cur_ses->debug_level -= dlevel;
	cur_ses->input_level -= ilevel;

	while (root->prev)
	{
		deltoken(root, root->prev);
	}

	free(root);

	return ses;
}


char *script_writer(struct session *ses, char *str)
{
	struct scriptroot *root;

	root = (struct scriptroot *) calloc(1, sizeof(struct scriptroot));

	root->ses = ses;

	tokenize_script(root, 1, str);

	return write_script(ses, root);
}
