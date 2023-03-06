/*
This file is part of vermillion.

Vermillion is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published
by the Free Software Foundation, version 3.

Vermillion is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with vermillion. If not, see <https://www.gnu.org/licenses/>.
*/

extern int
islower(int c)
{
    return (c >= 'a' && c <= 'z');
}

extern int
isupper(int c)
{
    return (c >= 'A' && c <= 'Z');
}

extern int
isalpha(int c)
{
    return islower(c) || isupper(c);
}

extern int
isdigit(int c)
{
    return (c >= '0' && c <= '9');
}

extern int
isalnum(int c)
{
    return isalpha(c) || isdigit(c);
}

extern int
isxdigit(int c)
{
    return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

extern int
iscntrl(int c)
{
    return (c >= 0 && c <= 31) || (c == 127);
}

extern int
isprint(int c)
{
    return !(iscntrl(c));
}

extern int
isgraph(int c)
{
    return (c != ' ') && isprint(c);
}

extern int
ispunct(int c)
{
    return isgraph(c) && !(isalnum(c));
}

extern int
isspace(int c)
{
    int ret = 0;

    switch (c)
    {
        case '\t': case '\n': case '\v':
        case '\f': case '\r': case ' ':
            ret = 1;
    }

    return ret;
}

extern int
toupper(int c)
{
    return (islower(c)) ? c - ('a' - 'A') : c;
}

extern int
tolower(int c)
{
    return (isupper(c)) ? c + ('a' - 'A') : c;
}
