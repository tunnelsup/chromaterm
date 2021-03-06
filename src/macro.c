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

DO_COMMAND(do_macro)
{
	char arg1[BUFFER_SIZE], arg2[BUFFER_SIZE], arg3[BUFFER_SIZE];

	arg = sub_arg_in_braces(ses, arg, arg1, 0, SUB_VAR|SUB_FUN);
	arg = get_arg_in_braces(ses, arg, arg2, 1);

	if (*arg1 == 0)
	{
		show_list(ses->list[LIST_MACRO], 0);
	}
	else if (*arg1 && *arg2 == 0)
	{
		if (show_node_with_wild(ses, arg1, LIST_MACRO) == FALSE)
		{
			show_message(ses, LIST_MACRO, "#MACRO: NO MATCH(ES) FOUND FOR {%s}.", arg1);
		}
	}
	else
	{
		unconvert_meta(arg1, arg3);

		update_node_list(ses->list[LIST_MACRO], arg1, arg2, arg3);

		show_message(ses, LIST_MACRO, "#OK. MACRO {%s} HAS BEEN SET TO {%s}.", arg1, arg2);
	}
	return ses;
}


DO_COMMAND(do_unmacro)
{
	delete_node_with_wild(ses, LIST_MACRO, arg);

	return ses;
}

