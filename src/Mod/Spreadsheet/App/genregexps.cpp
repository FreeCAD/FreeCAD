#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <map>
#include <algorithm>

int genUtf8(int c, unsigned char * b)
{
    if (c<0x80) { *b++=c, *b++ = '\0';
        return 1;
    }
    else if (c<0x800) { *b++=192+c/64, *b++=128+c%64, *b++ = '\0';  return 2; }
    else if (c-0xd800u < 0x800) goto error;
    else if (c<0x10000) { *b++=224+c/4096, *b++=128+c/64%64, *b++=128+c%64, *b++ = '\0';  return 3; }
    else if (c<0x110000) { *b++=240+c/262144, *b++=128+c/4096%64, *b++=128+c/64%64, *b++=128+c%64, *b++ = '\0';  return 4; }
    else goto error;
error:
    //  printf("Error! %x\n", c);
    //  exit(1);
    return -1;
}

typedef std::basic_string<unsigned char> String;
typedef std::vector<String > StringList;

StringList list;

unsigned char hex[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

static String escape(unsigned char c)
{
    switch (c) {
    case '*':
    case '+':
    case '-':
    case '(':
    case ')':
    case '\\':
    case '.':
    case '[':
    case ']':
    case '?':
    case '{':
    case '}':
    case '#':
    case '^':
    case '|':
    case ':':
    case '$':
    case '/':
    case '\'':
    case '"':
        return String((const unsigned char*)"\\") + c;
    default:
        return String((const unsigned char*)"") + c;
    }
}

static String encode(unsigned char c)
{
    if (c <= 32 || c > 126)
        return String((const unsigned char*)"\\x") + hex[c >> 4] + hex[c & 0xf];
    else {
        return String((const unsigned char*)"") + c;
    }
}

static String encodeString(String c)
{
    int i;
    String result;

    for (i = 0; i < c.size(); ++i)
        result += encode(c[i]);

    return result;
}

static String encodeRange(String r)
{
    String result;
    int i, j;

    for (i = 0; i < r.size(); ++i) {
        int n = 0;

        for (j = i; j < r.size() && r[i] + n == r[j]; ++j, ++n);

        if (n > 1) {
            result += escape(r[i]);
            result += (const unsigned char*)"-";
            result += escape(r[j - 1]);
            i = j - 1;
        }
        else
            result += escape(r[i]);
    }

    return result;
}

static String commonPrefix(int indent, StringList::const_iterator S, StringList::const_iterator E, int k)
{
    StringList::const_iterator i = S;
    String leafs;
    String branches;
    int nBranches = 0;
    bool first = true;

    if (S->size() <= k)
        return String((const unsigned char*)"");

    while (i != E) {
        StringList::const_iterator start = i;
        StringList::const_iterator end = i;
        int n = 0;

        if (i->size() == k + 1) {
            leafs += i->at(k);
            ++i;
        }
        else {
            /* Common path */
            while (end != E &&
                   end->size() >= start->size() &&
                   end->at(k) == start->at(k)) {
                ++n;
                ++end;
            }

            //if (leafs.size() > 0)
            if (!first)
                branches += (const unsigned char*)"|";
            branches += escape(start->at(k)) + commonPrefix(indent + 1, start, end, k + 1);

            first = false;
            nBranches++;
            i = end;
        }
    }

    if (leafs.size() > 1)
        leafs = (const unsigned char*)"[" + encodeRange(leafs) + (const unsigned char*)"]";

    if (nBranches == 0)
        return leafs;
    else {
        if (leafs.size() > 0)
            leafs += (const unsigned char*)"|";
        return (const unsigned char*)"(" + leafs + branches + (const unsigned char*)")";
    }
}

static void readFile(FILE * f)
{
    while (!feof(f)) {
        char line[2048];
        int start;
        int end;
        char cat[128];
        unsigned char out[8];
        int i;

        if (fgets(line, sizeof(line), f) == NULL)
            break;

        if (sscanf(line, "%4X..%4X ; %s", &start, &end, cat) == 3) {
            for (i = start; i <= end; ++i) {
                int n = genUtf8(i, out);

                if (n > 0) {
                    list.push_back(String(out, n));
                    fprintf(stderr, "%6X %d %s\n", i, n, out);
                }
            }
        }
        else if (sscanf(line, "%X ; %s", &start, cat) == 2) {
            int n = genUtf8(start, out);

            if (n > 0) {
                list.push_back(String(out, n));
                fprintf(stderr, "%6X %d %s\n", start, n, out);
            }
        }
    }
}

static String subdivide(String prefix, StringList::const_iterator S, StringList::const_iterator E, StringList & result)
{
    String regexp = commonPrefix(0, S, E, 0);

    regexp = encodeString(regexp);
    if (regexp.size() < 2000) {
        return regexp;
    }
    else {
        int n = E - S;
        StringList::const_iterator M = S + n / 2;

        result.push_back( prefix + (const unsigned char*)"1\t" + subdivide(prefix + (const unsigned char*)"1", S, M, result) + (const unsigned char*)"\n");
        result.push_back( prefix + (const unsigned char*)"2\t" + subdivide(prefix + (const unsigned char*)"2", M, E, result) + (const unsigned char*)"\n");

        return (const unsigned char*)"({" + prefix + (const unsigned char*)"1}|" +
                (const unsigned char*)"{" + prefix + (const unsigned char*)"2})";
    }
}

int main(int argc, char * argv[])
{
    for (int i = 2; i < argc; ++i) {
        FILE * f = fopen(argv[i], "r");

        if (f == NULL) {
            perror("fopen");
            return 1;
        }

        readFile(f);

        fclose(f);
    }

    sort(list.begin(), list.end());
    StringList result;

    String regexp = subdivide((const unsigned char*)argv[1], list.begin(), list.end(), result);

    for (StringList::const_iterator i = result.begin(); i != result.end(); ++i)
        printf("%s", i->c_str());

    printf("%s\t%s\n", argv[1], regexp.c_str());

}
