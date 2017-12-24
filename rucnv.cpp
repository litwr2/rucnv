#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#define b 65536 /* 8192, must be even */

using namespace std;

struct BUF {
  unsigned char *buf, back[12];
  int bufp, bufl, eob, backl;
  FILE *xi, *xo;
  BUF() {
    buf = new unsigned char [b];
  }
  ~BUF() {
    delete [] buf;
  }
  int wopen(char*);
  int ropen(char*);
  void wclose();
  void rclose();
  unsigned char get_char();
  void back_char(int c);
  void put_char(int c);
} in, out;

int BUF::wopen(char *s) {
  eob = bufp = bufl = 0;
  if (s == 0)
    xo = stdout;
  else if ((xo = fopen(s, "wb")) == 0)
    return 0;
  return 1;  
}

int BUF::ropen(char *s) {
  eob = bufp = bufl = backl = 0;
  if (s == 0)
    xi = stdin;
  else if ((xi = fopen(s, "rb")) == 0) 
    return 0;
  return 1;  
}

void BUF::wclose() {
  fwrite(buf, 1, bufp, xo);
  fclose(xo);
}

void BUF::rclose() {
  fclose(xi);
}

unsigned char BUF::get_char() {
  if (backl > 0) 
    return back[--backl];
  if (bufp >= bufl) {
    bufl = fread(buf, 1, b, xi);
    bufp = 0;
  }
  if (bufp >= bufl) {
    eob = 1;
    return 0;
  }
  return buf[bufp++];
} 

void BUF::back_char(int c) {
  back[backl++] = c;
  eob = 0;
}

void BUF::put_char(int c) {
  if (bufp >= b) {
    fwrite(buf, 1, b, xo);
    bufp = 0;
  }
  buf[bufp++] = c;
}

map<int,char> errset;
typedef map<int,char>::const_iterator CI;
int utf8skips = 0, ai = 0, ctab[256], unierr[256];
map<int,const char*> uninames, asciisub;
map<int,int> icat;

#include "uninames.h"
#include "trantabs.h"
#include "subtab.h"

void ini_uninames() {
  for (int i = 0; i < 355; i++)
    uninames[uninum[i]] = unidata + uniindex[i];
}

void ini_subtab() {
  unsigned i = 0;
  while (i < sizeof(subtab)/sizeof(SubTab)) { 
    asciisub[subtab[i].uc] = subtab[i].sn;
    i++;
  }
}

void backstr(char *s) {
  for (int i = strlen(s) - 1; i >= 0; i--)
    in.back_char(s[i]);
}

void addstr(char *s, int &c) {
  int i;
  s[i = strlen(s)] = c = in.get_char();
  s[i + 1] = 0;
}

void u_cnv(int &t, int &w) {
  if (ai == 2 && t == '[') {
    char s[12] = "", pre[] = "U+";
    int c, i;
    for (i = 0; i < 2; i++) {
      addstr(s, c);
      if (c != pre[i] || in.eob) 
        goto L2;
    }
    for (i = 0; i < 4; i++) {
      addstr(s, c);
      if (((c > '9' || c < '0') && (c > 'F' || c < 'A')) || in.eob) 
        goto L2;
    }
    if ((c = in.get_char()) == ']') {
      char *p;
      w = strtol(s + 2, &p, 16);
      t = 0;
    }
    else
L2:   backstr(s);
  }
}

void c00() {
  int l, h;
  while (1) {
    l = in.get_char();
    h = in.get_char();
    if (in.eob)
      break;
    out.put_char(l);
    out.put_char(h);
  }
}

void c0g() {
  int l, h;
  while (1) {
    l = in.get_char();
    h = in.get_char();
    if (in.eob)
      break;
    out.put_char(h);
    out.put_char(l);
  }
}

void c01() {
  unsigned int t;
  while (1) {
    t = in.get_char();
    t += in.get_char() << 8;
    if (in.eob)
      break;
    if (t <= 0x7f)
      out.put_char(t);
    else if (t <= 0x7ff) {
      out.put_char(0xc0 + (t >> 6));
      out.put_char(0x80 + (t & 0x3f));
    } else {
      out.put_char(0xe0 + (t >> 12));
      out.put_char(0x80 + ((t >> 6) & 0x3f));
      out.put_char(0x80 + (t & 0x3f));
    }
  }
}

void c10() {
  unsigned int c, c1, c2, t;
  while (1) {
    c = in.get_char();
    if (c <= 0x7f) 
      t = c;
    else {
      c1 = in.get_char();
      if (c <= 0xdf)
        t = ((c & 0x1f) << 6) + (c1 & 0x3f);
      else if (c <= 0xef) {
        c2 = in.get_char();
        t = ((c & 0xf) << 12) + ((c1 & 0x3f) << 6) + (c2 & 0x3f);
      }
      else {
        utf8skips++;
        in.buf[--in.bufp] = c1;
        continue;
      }
    }
    if (in.eob)
      break;
    out.put_char(t & 0xff);
    out.put_char(t >> 8);
  }
}

void c2toB0() {
  int t, w;
  while (1) {
    w = ctab[t = in.get_char()];
    if (in.eob)
      break;
    u_cnv(t, w);
    if (w == 0 && t != 0) {
      w = '?';
      unierr[t] = 1;
    }
    else if (t != 0)
      icat[w] = t;
    out.put_char(w & 0xff);
    out.put_char(w >> 8);
  }
}

void c02toB() {
  map<int,int> m;
  int w, t;
  for (w = 0; w < 256; w++)
    m[ctab[w]] = w;
  while (1) {
    w = in.get_char();
    w += in.get_char() << 8;
    t = m[w];
    if (in.eob)
      break;
    if (t == 0) 
      if (ai == 1)
        if (asciisub[w] != 0) {
          unsigned i;
          for (i = 0; i < strlen(asciisub[w]) - 1; i++)
            out.put_char(asciisub[w][i]);
          t = asciisub[w][i];
        } 
        else
          goto L1;
      else if (ai == 2) {
        char s[12];
        sprintf(s, "[U+%04X", w);
        for (int i = 0; i < strlen(s); i++)
           out.put_char(s[i]);
        t = ']';
      }
      else {
L1:     t = '?';
        errset[w] = 1;
      }
    out.put_char(t);
  }
}

void cc0() {
  int t, w;
  while (1) {
    w = t = in.get_char();
    if (t > 128) {
      unierr[t] = 1;
      t = '?';
    }
    u_cnv(t, w);
    if (t < 128)
      if (t > 63)
        w = ctab[t + 128];
      else if (t == 36)
        w = 0xa4;
    if (in.eob)
      break;
    out.put_char(w & 0xff);
    out.put_char(w >> 8);
  }
}

void c0c() {
  map<int,int> m;
  int w, t;
  for (w = 0; w < 256; w++)
    m[ctab[w]] = w;
  while (1) {
    w = in.get_char();
    w += in.get_char() << 8;
    t = m[w];
    if (in.eob)
      break;
    if (t > 128 && t < 192 && t != 163 && t != 179 || t == 0) 
      if (ai == 1) {
        if (asciisub[w] != 0) {
          unsigned i;
          for (i = 0; i < strlen(asciisub[w]) - 1; i++)
            out.put_char(asciisub[w][i]);
          t = asciisub[w][i];
        } 
        else
          goto L1;
      }
      else if (ai == 2) {
        char s[12];
        sprintf(s, "[U+%04X", w);
        for (int i = 0; i < strlen(s); i++)
           out.put_char(s[i]);
        t = ']';
      }
      else {
L1:     t = '?';
        errset[w] = 1;
      }
    out.put_char(t & 0x7f);
  }
}

void cd0() {
  int j, i, t, w;
  char r[6];
  while (1) {
    r[0] = 0;
    t = in.get_char();
    if (in.eob)
      break;
    if (t > 128) {
      unierr[t] = 1;
      t = '?';
    }  
    u_cnv(t, w = 0);
    if (t == 0) 
      goto l1;
    if (t == '<') {
      addstr(r, w);
      switch (w) {
        case 'C':
          j = '~';
          goto l2; 
        case 'c':
          j = '^';
l2:       addstr(r, i);
          if (i != 'h') 
            goto l4;
          addstr(r, i);
          if (i != '>') 
            goto l4;
          t = j;
          break;    
        case 'S':
          j = '{';
          goto l5;
        case 's':
          j = '[';
l5:       addstr(r, i);
          if (i != 'h') 
            goto l4;
          addstr(r, i);
          if (i == 'c') {
            j += 2;
            goto l2;
          } 
          if (i != '>') 
            goto l4;
          t = j;
          break;    
        case 'E':
          j = '|'; 
          goto l3;
        case 'e':
          j = '\\'; 
          goto l3;
        case 'U': 
          j = '`'; 
          goto l3;
        case 'u':
          j = '@'; 
          goto l3;
        case '`':
          j = 127;
          goto l3;
        case '\'':
          j = '_';
          goto l3;
        case 'O':
          j = '3'; 
          goto l3;
        case 'o':
          j = '#'; 
l3:       addstr(r, i);
          if (i != '>') 
            goto l4;
          t = j;
          break;
        default:
l4:       backstr(r);
          goto l7;
      }
      t += 128;
    } 
    else if (t >= 'A' && t <= 'Z')
      t += 160;
    else if (t >= 'a' && t <= 'z')  
      t += 96;
l7: w = ctab[t];
l1: out.put_char(w & 0xff);
    out.put_char(w >> 8);
  }
}

void c0d() {
  map<int,int> m;
  int w, t;
  char c;
  for (w = 0; w < 256; w++)
    m[ctab[w]] = w;
  while (1) {
    w = in.get_char();
    w += in.get_char() << 8;
    if (in.eob)
      break;
    t = m[w];
    if (t > 128 && t < 192 && t != 163 && t != 179 || t == 0)
      if (ai == 1) {
        if (asciisub[w] != 0) {
          unsigned i;
          for (i = 0; i < strlen(asciisub[w]) - 1; i++)
            out.put_char(asciisub[w][i]);
          t = asciisub[w][i];
        } 
	else
          goto L1;
      } 
      else if (ai == 2) {
        char s[12];
        sprintf(s, "[U+%04X", w);
        for (int i = 0; i < strlen(s); i++)
           out.put_char(s[i]);
        t = ']';
      }
      else {
L1:     errset[w] = 1;
        t = '?';
      }
    if (t >= 192 && t <= 223) 
      t += 32;
    else if (t >= 224)
      t -= 32;
    c = w = t;
    switch (c) {
      case 'Þ': case 'þ': case 'Û': case 'û': case 'Ý': case 'ý': case 'ß': 
      case 'ÿ': case 'À': case 'à': case 'Ü': case 'ü': case '£': case '³':
             out.put_char('<');
    }
    switch (c) {
      case 'Þ': out.put_char('C'); t = 'h'; break;
      case 'þ': out.put_char('c'); t = 'h'; break;
      case 'Û': out.put_char('S'); t = 'h'; break;
      case 'û': out.put_char('s'); t = 'h'; break;
      case 'Ý': out.put_char('S'); out.put_char('h'); 
             out.put_char('c'); t = 'h'; break;
      case 'ý': out.put_char('s'); out.put_char('h'); 
             out.put_char('c'); t = 'h'; break;
      case 'ß': t = '`'; break;
      case 'ÿ': t = '\''; break;
      case 'À': t = 'U'; break;
      case 'à': t = 'u'; break;
      case 'Ü': t = 'E'; break;
      case 'ü': t = 'e'; break;
      case '£': t = 'o'; break;
      case '³': t = 'O'; break;
    }
    out.put_char(t & 0x7f);
    switch (c) {
      case 'Þ': case 'þ': case 'Û': case 'û': case 'Ý': case 'ý': case 'ß': 
      case 'ÿ': case 'À': case 'à': case 'Ü': case 'ü': case '£': case '³':
             out.put_char('>');
    }
  }
}

void cf0() {
}

void c0f() {
  unsigned l, h;
  const char *s;
  while (1) {
    l = in.get_char();
    h = in.get_char()*256 + l;
    if (in.eob)
      break;
    s = uninames[h];
    if (s == 0) {
      errset[h] = 1;
      out.put_char('?');
    }
    else
      for (l = 0; l < strlen(s); l++)
        out.put_char(s[l]);
    out.put_char('\n');
  }
}

void c0e() {
  map<int,int> m;
  int w, t;
  char c;
  for (w = 0; w < 256; w++)
    m[ctab[w]] = w;
  while (1) {
    w = in.get_char();
    w += in.get_char() << 8;
    if (in.eob)
      break;
    t = m[w];
    if (t > 128 && t < 192 && t != 163 && t != 179 || t == 0)
      if (ai == 1) {
        if (asciisub[w] != 0) {
          unsigned i;
          for (i = 0; i < strlen(asciisub[w]) - 1; i++)
            out.put_char(asciisub[w][i]);
          t = asciisub[w][i];
        } 
        else
          goto L1;
      } 
      else if (ai == 2) {
        char s[12];
        sprintf(s, "[U+%04X", w);
        for (int i = 0; i < strlen(s); i++)
           out.put_char(s[i]);
        t = ']';
      }
      else {
L1:     errset[w] = 1;
        t = '?';
      }
    if (t >= 192 && t <= 223) 
      t += 32;
    else if (t >= 224)
      t -= 32;
    c = w = t;
    switch (c) {
      case 'à': out.put_char('y'); t = 'u'; break;
      case 'À': out.put_char('Y'); t = 'u'; break;
      case 'ñ': out.put_char('y'); t = 'a'; break;
      case 'Ñ': out.put_char('Y'); t = 'a'; break;
      case 'ö': out.put_char('z'); t = 'h'; break;
      case 'Ö': out.put_char('Z'); t = 'h'; break;
      case '÷': t = 'v'; break;
      case '×': t = 'V'; break;
      case 'è': t = 'x'; break;
      case 'È': t = 'X'; break;
      case 'ø': 
      case 'Ø': t = '`'; break;
      case 'ù': out.put_char('y'); t = '`'; break;
      case 'Ù': out.put_char('Y'); t = '`'; break;
      case 'û': out.put_char('s'); t = 'h'; break;
      case 'Û': out.put_char('S'); t = 'h'; break;
      case 'ü': out.put_char('e'); t = '`'; break;
      case 'Ü': out.put_char('E'); t = '`'; break;
      case 'ý': out.put_char('s'); out.put_char('h'); 
             t = 'h'; break;
      case 'Ý': out.put_char('S'); out.put_char('h'); 
             t = 'h'; break;
      case 'þ': out.put_char('c'); t = 'h'; break;
      case 'Þ': out.put_char('C'); t = 'h'; break;
      case 'ÿ': 
      case 'ß': out.put_char('`'); t = '`'; break;
      case '³': out.put_char('Y'); t = 'o'; break;
      case '£': out.put_char('y'); t = 'o'; break;
    }
    out.put_char(t & 0x7f);
  }
}

void ce0() {
  int j, t, w;
  char r[3];
  while (1) {
    r[0] = 0;
    t = in.get_char();
    if (in.eob)
      break;
    if (t > 128) {
      unierr[t] = 1;
      t = '?';
    }  
    u_cnv(t, w = 0);
    if (t == 0) 
      goto l1;
    switch (t) {
      case 'Y': 
          addstr(r, j);
          if (j == 'o' || j == 'O') 
            t = (unsigned char) '³';
          else if (j == 'a' || j == 'A') 
            t = (unsigned char) 'ñ';
          else if (j == 'u' || j == 'U') 
            t = (unsigned char) 'à';
          else if (j == '`') 
            t = (unsigned char) 'ù';
          else
            backstr(r);
          break;
      case 'y': 
          addstr(r, j);
          if (j == 'o') 
            t = (unsigned char) '£';
          else if (j == 'a') 
            t = (unsigned char) 'Ñ';
          else if (j == 'u') 
            t = (unsigned char) 'À';
          else if (j == '`') 
            t = (unsigned char) 'Ù';
          else
            backstr(r);
          break;
      case 'Z': 
          addstr(r, j);
          if (j == 'h' || j == 'H') 
            t = (unsigned char) 'ö';
          else
            backstr(r);
          break;
      case 'z': 
          addstr(r, j);
          if (j == 'h') 
            t = (unsigned char) 'Ö';
          else
            backstr(r);
          break;
      case 'C': 
          addstr(r, j);
          if (j == 'h' || j == 'H') 
            t = (unsigned char) 'þ';
          else if (j == 'z' || j == 'Z') 
            t = (unsigned char) 'ã';
          else
            backstr(r);
          break;
      case 'c': 
          addstr(r, j);
          if (j == 'h') 
            t = (unsigned char) 'Þ';
          else if (j == 'z') 
            t = (unsigned char) 'Ã';
          else
            backstr(r);
          break;
      case 'S': 
          addstr(r, j);
          if (j == 'h' || j == 'H') {
            addstr(r, j);
            if (j == 'h' || j == 'H')
              t = (unsigned char) 'ý';
            else {
              backstr(r);
              addstr(r, j);
              t = (unsigned char) 'û';
            }
          }
          else
            backstr(r);
          break;
      case 's': 
          addstr(r, j);
          if (j == 'h') {
            addstr(r, j);
            if (j == 'h')
              t = (unsigned char) 'Ý';
            else {
              backstr(r);
              addstr(r, j);
              t = (unsigned char) 'Û';
            }
          }
          else
            backstr(r);
          break;
      case 'E': 
          addstr(r, j);
          if (j == '`') 
            t = (unsigned char) 'ü';
          else
            backstr(r);
          break;
      case 'e': 
          addstr(r, j);
          if (j == '`') 
            t = (unsigned char) 'Ü';
          else
            backstr(r);
          break;
      case '`': 
          addstr(r, j);
          if (j == '`') 
            t = (unsigned char) 'ß';
          else {
            backstr(r);
            t = (unsigned char) 'Ø';
          }
          break;
      case 'V':
          t = (unsigned char) '÷';
          break;
      case 'v':
          t = (unsigned char) '×';
          break;
      case 'X':
          t = (unsigned char) 'è';
          break;
      case 'x':
          t = (unsigned char) 'È';
          break;
    }
    if (t >= 'A' && t <= 'Z')
      t += 160;
    else if (t >= 'a' && t <= 'z')  
      t += 96;
    w = ctab[j = t];
l1: out.put_char(w & 0xff);
    out.put_char(w >> 8);
  }
}

void (*cnvto[])(void) = {c00, c01, c02toB, c02toB, c02toB, c02toB, c02toB, 
  c02toB, c02toB, c02toB, c02toB, c02toB, c0c, c0d, c0e, c0f, c0g}, 
  (*cnvti[])(void) = {c00, c10, c2toB0, c2toB0, c2toB0, c2toB0, c2toB0, 
  c2toB0, c2toB0, c2toB0, c2toB0, c2toB0, cc0, cd0, ce0, cf0, c0g};
int *ictab[] = {0, 0, koi8r_ucs, cp866_ucs, iso8859_5_ucs, cp1251_ucs, 
  maccyr_ucs, alt_m_ucs, alt_ucs, main_ucs, bulg_ucs, koi8_ucs, koi8r_ucs,
  koi8r_ucs, koi8r_ucs, 0, 0};

void mesg(void) {
  printf("USAGE: rucnv <translation> <source> <destination>\n");
  printf("       rucnv -V       output version information and exit\n");
  printf("       rucnv --help   display this help and exit\n");
  printf("<translation> has form <source coding><result coding>{s}{u}\n");
  printf("  's' ('u') makes (unicode) substitutions for missing characters\n");
  printf("codings:\n");
  printf("  0: Unicode (UCS-2 Little Endian)\t");
  printf("  1: UTF-8\n");
  printf("  2: Koi8-r\t\t\t\t");
  printf("  3: CP866 (MS-DOS)\n");
  printf("  4: ISO 8859-5\t\t\t\t");
  printf("  5: CP1251 (Microsoft Windows)\n");
  printf("  6: Macintosh Cyrillic (CP10007)\t");
  printf("  7: Modified Alternative (MS-DOS)\n");
  printf("  8: Alternative (MS-DOS)\t\t");
  printf("  9: Main (MS-DOS)\n");
  printf("  A: Bulgarian, MIC, Interprog (MS-DOS)\t");
  printf("  B: Koi-8\n");
  printf("  C: Koi-7\t\t\t\t");
  printf("  D: Transliteration (Koi-7 based)\n");
  printf("  E: Transliteration (GOST 7.79-2000)\t");
  printf("  F: Unicode Names\n");
  printf("  G: UTF-16BE\n");
  printf("Examples:\n");
  printf("  rucnv 52s wintext.txt lintext.txt\n");
  printf("  rucnv 31 dostext.txt utf8text.txt\n");
  printf("  rucnv 12u utf8text.txt koi8rtext.txt\n");
  printf("  rucnv 50u wintext.txt ucs2text.txt\n");
}

int main(int argc, char *argv[]) {
  char tmp[255] = "rucnvXXXXXX", *fnin, *fnout;
  int mi, mo;
  switch (argc) {
    case 4:
      fnin = argv[2];
      fnout = argv[3];
      break;
    case 3:
      fnin = argv[2];
      fnout = 0;
      break;
    case 2:
      if (argv[1][0] == '-') {
        if (!strcmp(argv[1], "-V"))
          printf("Russian texts convertor V1.08 (C) 2008 V.Lidovski\n");
        else if (!strcmp(argv[1], "--help"))
          mesg();
        else
          goto l5;
        exit(0);
      }
      else
        fnin = fnout = 0;
      break;
    case 1:
      mesg(); //printf("try rucnv --help\n");
      exit(0);
    default:
      fprintf(stderr, "Incorrect number of parameters\n"); 
      exit(3);
  }
  if (strlen(argv[1]) == 3)
    if (argv[1][2] == 's') {
      ai = 1;
      ini_subtab();
    }
    else if (argv[1][2] == 'u') 
      ai = 2;
    else
      goto l5;
  else if (strlen(argv[1]) != 2)
    goto l5;
  mi = argv[1][0];
  mo = argv[1][1];
  if (mi >= 'a')
    mi -= 'a' - 10;
  else if (mi >= 'A') 
    mi -= 'A' - 10;
  else
    mi -= '0';
  if (mo >= 'a')
    mo -= 'a' - 10;
  else if (mo >= 'A')
    mo -= 'A' - 10;
  else
    mo -= '0';
  if (mi < 0 || (mi > 14 && mi != 16) || mo < 0 || mo > 16) {
l5: fprintf(stderr, "%dIncorrect translation mode\n", mi);
    exit(4);
  }
  for (int i = 0; i < 128; i++)
    ctab[i] = i;
  ini_uninames();
  if (in.ropen(fnin) == 0) {
    fprintf(stderr, "Can't open %s for read\n", fnin);
    return 2;
  }
  fclose(fdopen(mkstemp(tmp),"r+"));
  out.wopen(tmp);
  if (ictab[mi])
    memcpy(ctab + 128, ictab[mi], 128*sizeof(int));
  (*cnvti[mi])();
  out.wclose();
  in.rclose();
  in.ropen(tmp);
  if (out.wopen(fnout) == 0) {
    fprintf(stderr, "Can't open %s for write\n", fnout);
    return 1;
  }
  if (ictab[mo]) 
    memcpy(ctab + 128, ictab[mo], 128*sizeof(int));
  (*cnvto[mo])();
  out.wclose();
  in.rclose();
  remove(tmp);
  if (utf8skips > 0)
    fprintf(stderr, "%d UTF-8 skips produced\n", utf8skips);
  for (int i = 0; i < 256; i++)
    if (unierr[i]) {
      fprintf(stderr, "Not mapped to Unicode:");
      for (int i = 0; i < 256; i++)
        if (unierr[i])
          fprintf(stderr, "%4d", i);
      fprintf(stderr, "\n");
      break;
    }
  for (CI p = errset.begin(); p != errset.end(); ++p) {
    fprintf(stderr, "Not converted:");
    for (CI q = errset.begin(); q != errset.end(); ++q) {
      if (uninames[q->first])
        fprintf(stderr, " %s", uninames[q->first]);
      if (mi > 1)
        fprintf(stderr, " (%d)", icat[q->first]);
      else
        fprintf(stderr, " (U+%04X)", q->first);
    }
    fprintf(stderr, "\n");
    break;
  }
}
