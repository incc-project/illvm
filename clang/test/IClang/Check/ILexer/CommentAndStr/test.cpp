// "xxx
int x = 1;
/* 'yyy */
int y = /* 'zzz'
  #include <vector> */ 1;
#define T /*
"xxx"
* R"()" */ 1
const char *s = "/*xxx*///yyy\\";
const char *s2 = R"(
 #include <vector>
 /* xxx */
 // yyy
 /n
)";
char c = '\'';
char c2 = '"';
int main() {
  return 0;
}