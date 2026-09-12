typedef int __int128 __attribute__ ((__mode__ (TI)));
typedef __builtin_va_list __gnuc_va_list;
typedef __gnuc_va_list va_list;
void __cdecl __debugbreak(void);
extern inline __attribute__((__always_inline__,__gnu_inline__)) void __cdecl __debugbreak(void)
{
__asm__ volatile("int {$}3":);
}
void __cdecl __attribute__ ((__noreturn__)) __fastfail(unsigned int _Code);
extern inline __attribute__((__always_inline__,__gnu_inline__)) void __cdecl __attribute__ ((__noreturn__)) __fastfail(unsigned int _Code)
{
__asm__ volatile("int {$}0x29"::"c"(_Code));
__builtin_unreachable();
}
const char *__mingw_get_crt_info (void);
__extension__ typedef unsigned long long size_t;
__extension__ typedef long long ssize_t;
typedef size_t rsize_t;
__extension__ typedef long long intptr_t;
__extension__ typedef unsigned long long uintptr_t;
__extension__ typedef long long ptrdiff_t;
typedef unsigned short wchar_t;
typedef unsigned short wint_t;
typedef unsigned short wctype_t;
typedef int errno_t;
typedef long __time32_t;
__extension__ typedef long long __time64_t;
typedef __time64_t time_t;
void __cdecl _invalid_parameter_noinfo(void);
__attribute__ ((__noreturn__)) void __cdecl _invalid_parameter_noinfo_noreturn(void);
__attribute__ ((__noreturn__)) void __cdecl _invoke_watson(const wchar_t *expression, const wchar_t *function_name, const wchar_t *file_name, unsigned int line_number, unsigned long long reserved);
struct threadlocaleinfostruct;
struct threadmbcinfostruct;
typedef struct threadlocaleinfostruct *pthreadlocinfo;
typedef struct threadmbcinfostruct *pthreadmbcinfo;
struct __lc_time_data;
typedef struct localeinfo_struct {
pthreadlocinfo locinfo;
pthreadmbcinfo mbcinfo;
} _locale_tstruct,*_locale_t;
typedef struct tagLC_ID {
unsigned short wLanguage;
unsigned short wCountry;
unsigned short wCodePage;
} LC_ID,*LPLC_ID;
typedef struct threadlocaleinfostruct {
const unsigned short *_locale_pctype;
int _locale_mb_cur_max;
unsigned int _locale_lc_codepage;
} threadlocinfo;
const unsigned short* __cdecl __pctype_func(void);
const wctype_t * __cdecl __pwctype_func(void);
extern const unsigned short *_wctype;
int __cdecl iswctype(wint_t _C,wctype_t _Type);
int __cdecl iswalnum(wint_t _C);
int __cdecl iswalpha(wint_t _C);
int __cdecl iswblank(wint_t _C);
int __cdecl iswcntrl(wint_t _C);
int __cdecl iswdigit(wint_t _C);
int __cdecl iswgraph(wint_t _C);
int __cdecl iswlower(wint_t _C);
int __cdecl iswprint(wint_t _C);
int __cdecl iswpunct(wint_t _C);
int __cdecl iswspace(wint_t _C);
int __cdecl iswupper(wint_t _C);
int __cdecl iswxdigit(wint_t _C);
wint_t __cdecl towlower(wint_t _C);
wint_t __cdecl towupper(wint_t _C);
int __cdecl _iswctype_l(wint_t _C,wctype_t _Type,_locale_t _Locale);
int __cdecl _iswalnum_l(wint_t _C,_locale_t _Locale);
int __cdecl _iswalpha_l(wint_t _C,_locale_t _Locale);
int __cdecl _iswblank_l(wint_t _C,_locale_t _Locale);
int __cdecl _iswcntrl_l(wint_t _C,_locale_t _Locale);
int __cdecl _iswdigit_l(wint_t _C,_locale_t _Locale);
int __cdecl _iswgraph_l(wint_t _C,_locale_t _Locale);
int __cdecl _iswlower_l(wint_t _C,_locale_t _Locale);
int __cdecl _iswprint_l(wint_t _C,_locale_t _Locale);
int __cdecl _iswpunct_l(wint_t _C,_locale_t _Locale);
int __cdecl _iswspace_l(wint_t _C,_locale_t _Locale);
int __cdecl _iswupper_l(wint_t _C,_locale_t _Locale);
int __cdecl _iswxdigit_l(wint_t _C,_locale_t _Locale);
wint_t __cdecl _towlower_l(wint_t _C,_locale_t _Locale);
wint_t __cdecl _towupper_l(wint_t _C,_locale_t _Locale);
int __cdecl __iswcsym(wint_t _C);
int __cdecl __iswcsymf(wint_t _C);
int __cdecl iswascii(wint_t _C);
int __cdecl is_wctype(wint_t _C,wctype_t _Type);
int __cdecl isleadbyte(int _C);
int __cdecl _iswcsym_l(wint_t _C,_locale_t _Locale);
int __cdecl _iswcsymf_l(wint_t _C,_locale_t _Locale);
int __cdecl _isleadbyte_l(int _C,_locale_t _Locale);
int __cdecl ___mb_cur_max_func(void);
int __cdecl isalnum(int _C);
int __cdecl isalpha(int _C);
int __cdecl isblank(int _C);
int __cdecl iscntrl(int _C);
int __cdecl isdigit(int _C);
int __cdecl isgraph(int _C);
int __cdecl islower(int _C);
int __cdecl isprint(int _C);
int __cdecl ispunct(int _C);
int __cdecl isspace(int _C);
int __cdecl isupper(int _C);
int __cdecl isxdigit(int _C);
int __cdecl tolower(int _C);
int __cdecl toupper(int _C);
int __cdecl _isalnum_l(int _C,_locale_t _Locale);
int __cdecl _isalpha_l(int _C,_locale_t _Locale);
int __cdecl _isblank_l(int _C,_locale_t _Locale);
int __cdecl _iscntrl_l(int _C,_locale_t _Locale);
int __cdecl _isdigit_l(int _C,_locale_t _Locale);
int __cdecl _isgraph_l(int _C,_locale_t _Locale);
int __cdecl _islower_l(int _C,_locale_t _Locale);
int __cdecl _isprint_l(int _C,_locale_t _Locale);
int __cdecl _ispunct_l(int _C,_locale_t _Locale);
int __cdecl _isspace_l(int _C,_locale_t _Locale);
int __cdecl _isupper_l(int _C,_locale_t _Locale);
int __cdecl _isxdigit_l(int _C,_locale_t _Locale);
int __cdecl _tolower_l(int _C,_locale_t _Locale);
int __cdecl _toupper_l(int _C,_locale_t _Locale);
int __cdecl __isascii(int _C);
int __cdecl __toascii(int _C);
int __cdecl _tolower(int _C);
int __cdecl _toupper(int _C);
int __cdecl _isctype(int _C,int _Type);
int __cdecl __iscsym(int _C);
int __cdecl __iscsymf(int _C);
int __cdecl _iscsym_l(wint_t _C, _locale_t _Locale);
int __cdecl _iscsymf_l(wint_t _C, _locale_t _Locale);
int __cdecl _isctype_l(int _C,int _Type,_locale_t _Locale);
extern int *__cdecl _errno(void);
errno_t __cdecl _set_errno(int _Value);
errno_t __cdecl _get_errno(int *_Value);
extern unsigned long __cdecl __threadid(void);
extern uintptr_t __cdecl __threadhandle(void);
typedef struct {
long long __max_align_ll __attribute__((__aligned__(_Alignof(long long))));
long double __max_align_ld __attribute__((__aligned__(_Alignof(long double))));
} max_align_t;
typedef struct {
size_t gl_pathc;
char **gl_pathv;
} glob_t;
int glob(const char *pattern,
int flags,
int (*errfunc)(const char *, int),
glob_t *pglob);
void globfree(glob_t *pglob);
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned uint32_t;
__extension__ typedef long long int64_t;
__extension__ typedef unsigned long long uint64_t;
typedef signed char int_least8_t;
typedef unsigned char uint_least8_t;
typedef short int_least16_t;
typedef unsigned short uint_least16_t;
typedef int int_least32_t;
typedef unsigned uint_least32_t;
__extension__ typedef long long int_least64_t;
__extension__ typedef unsigned long long uint_least64_t;
typedef signed char int_fast8_t;
typedef unsigned char uint_fast8_t;
typedef short int_fast16_t;
typedef unsigned short uint_fast16_t;
typedef int int_fast32_t;
typedef unsigned int uint_fast32_t;
__extension__ typedef long long int_fast64_t;
__extension__ typedef unsigned long long uint_fast64_t;
__extension__ typedef long long intmax_t;
__extension__ typedef unsigned long long uintmax_t;
unsigned long long* __cdecl __local_stdio_printf_options(void);
unsigned long long* __cdecl __local_stdio_scanf_options(void);
struct _iobuf {
void *_Placeholder;
};
typedef struct _iobuf FILE;
typedef long _off_t;
typedef long off32_t;
__extension__ typedef long long _off64_t;
__extension__ typedef long long off64_t;
typedef off32_t off_t;
FILE *__cdecl __acrt_iob_func(unsigned index);
FILE *__cdecl __iob_func(void);
__extension__ typedef long long fpos_t;
extern
__attribute__((__format__(__gnu_scanf__, 2,3))) int __cdecl __mingw_sscanf(const char * __restrict__ _Src,const char * __restrict__ _Format,...);
extern
__attribute__((__format__(__gnu_scanf__, 2,0))) int __cdecl __mingw_vsscanf (const char * __restrict__ _Str,const char * __restrict__ Format,va_list argp);
extern
__attribute__((__format__(__gnu_scanf__, 1,2))) int __cdecl __mingw_scanf(const char * __restrict__ _Format,...);
extern
__attribute__((__format__(__gnu_scanf__, 1,0))) int __cdecl __mingw_vscanf(const char * __restrict__ Format, va_list argp);
extern
__attribute__((__format__(__gnu_scanf__, 2,3))) int __cdecl __mingw_fscanf(FILE * __restrict__ _File,const char * __restrict__ _Format,...);
extern
__attribute__((__format__(__gnu_scanf__, 2,0))) int __cdecl __mingw_vfscanf (FILE * __restrict__ fp, const char * __restrict__ Format,va_list argp);
extern
__attribute__((__format__(__gnu_printf__,3,0))) int __cdecl __mingw_vsnprintf(char * __restrict__ _DstBuf,size_t _MaxCount,const char * __restrict__ _Format,
va_list _ArgList);
extern
__attribute__((__format__(__gnu_printf__,3,4))) int __cdecl __mingw_snprintf(char * __restrict__ s, size_t n, const char * __restrict__ format, ...);
extern
__attribute__((__format__(__gnu_printf__,1,2))) int __cdecl __mingw_printf(const char * __restrict__ , ... ) ;
extern
__attribute__((__format__(__gnu_printf__,1,0))) int __cdecl __mingw_vprintf (const char * __restrict__ , va_list) ;
extern
__attribute__((__format__(__gnu_printf__,2,3))) int __cdecl __mingw_fprintf (FILE * __restrict__ , const char * __restrict__ , ...) ;
extern
__attribute__((__format__(__gnu_printf__,2,0))) int __cdecl __mingw_vfprintf (FILE * __restrict__ , const char * __restrict__ , va_list) ;
extern
__attribute__((__format__(__gnu_printf__,2,3))) int __cdecl __mingw_sprintf (char * __restrict__ , const char * __restrict__ , ...) ;
extern
__attribute__((__format__(__gnu_printf__,2,0))) int __cdecl __mingw_vsprintf (char * __restrict__ , const char * __restrict__ , va_list) ;
extern
__attribute__((__format__(__gnu_printf__,2,3))) __attribute__((nonnull (1,2)))
int __cdecl __mingw_asprintf(char ** __restrict__ , const char * __restrict__ , ...) ;
extern
__attribute__((__format__(__gnu_printf__,2,0))) __attribute__((nonnull (1,2)))
int __cdecl __mingw_vasprintf(char ** __restrict__ , const char * __restrict__ , va_list) ;
extern
__attribute__((__format__(__ms_scanf__, 2,3))) int __cdecl __ms_sscanf(const char * __restrict__ _Src,const char * __restrict__ _Format,...)
__asm__("sscanf");
extern
__attribute__((__format__(__ms_scanf__, 2,0))) int __cdecl __ms_vsscanf(const char * __restrict__ _Str,const char * __restrict__ _Format,va_list argp)
__asm__("vsscanf");
extern
__attribute__((__format__(__ms_scanf__, 1,2))) int __cdecl __ms_scanf(const char * __restrict__ _Format,...)
__asm__("scanf");
extern
__attribute__((__format__(__ms_scanf__, 1,0))) int __cdecl __ms_vscanf(const char * __restrict__ _Format,va_list argp)
__asm__("vscanf");
extern
__attribute__((__format__(__ms_scanf__, 2,3))) int __cdecl __ms_fscanf(FILE * __restrict__ _File,const char * __restrict__ _Format,...)
__asm__("fscanf");
extern
__attribute__((__format__(__ms_scanf__, 2,0))) int __cdecl __ms_vfscanf(FILE * __restrict__ _File,const char * __restrict__ _Format,va_list argp)
__asm__("vfscanf");
extern
__attribute__((__format__(__ms_printf__, 1,2))) int __cdecl __ms_printf(const char * __restrict__ , ... )
__asm__("printf") ;
extern
__attribute__((__format__(__ms_printf__, 1,0))) int __cdecl __ms_vprintf (const char * __restrict__ , va_list)
__asm__("vprintf") ;
extern
__attribute__((__format__(__ms_printf__, 2,3))) int __cdecl __ms_fprintf (FILE * __restrict__ , const char * __restrict__ , ...)
__asm__("fprintf") ;
extern
__attribute__((__format__(__ms_printf__, 2,0))) int __cdecl __ms_vfprintf (FILE * __restrict__ , const char * __restrict__ , va_list)
__asm__("vfprintf") ;
extern
__attribute__((__format__(__ms_printf__, 2,3))) int __cdecl __ms_sprintf (char * __restrict__ , const char * __restrict__ , ...)
__asm__("sprintf") ;
extern
__attribute__((__format__(__ms_printf__, 2,0))) int __cdecl __ms_vsprintf (char * __restrict__ , const char * __restrict__ , va_list)
__asm__("vsprintf") ;
extern
__attribute__((__format__(__ms_printf__, 3,4))) int __cdecl __ms_snprintf (char * __restrict__ , size_t , const char * __restrict__ , ...)
__asm__("snprintf") ;
extern
__attribute__((__format__(__ms_printf__, 3,0))) int __cdecl __ms_vsnprintf (char * __restrict__ , size_t , const char * __restrict__ , va_list)
__asm__("vsnprintf") ;
int __cdecl __stdio_common_vsprintf(unsigned long long options, char *str, size_t len, const char *format, _locale_t locale, va_list valist);
int __cdecl __stdio_common_vfprintf(unsigned long long options, FILE *file, const char *format, _locale_t locale, va_list valist);
int __cdecl __stdio_common_vsscanf(unsigned long long options, const char *input, size_t length, const char *format, _locale_t locale, va_list valist);
int __cdecl __stdio_common_vfscanf(unsigned long long options, FILE *file, const char *format, _locale_t locale, va_list valist);
__attribute__((__format__(__gnu_scanf__, 2,3))) int sscanf(const char *__source, const char *__format, ...)
__asm__("__mingw_sscanf");
__attribute__((__format__(__gnu_scanf__, 1,2))) int scanf(const char *__format, ...)
__asm__("__mingw_scanf");
__attribute__((__format__(__gnu_scanf__, 2,3))) int fscanf(FILE *__stream, const char *__format, ...)
__asm__("__mingw_fscanf");
__attribute__((__format__(__gnu_scanf__, 2,0))) int vsscanf (const char *__source, const char *__format, __builtin_va_list __local_argv)
__asm__("__mingw_vsscanf");
__attribute__((__format__(__gnu_scanf__, 1,0))) int vscanf(const char *__format, __builtin_va_list __local_argv)
__asm__("__mingw_vscanf");
__attribute__((__format__(__gnu_scanf__, 2,0))) int vfscanf (FILE *__stream, const char *__format, __builtin_va_list __local_argv)
__asm__("__mingw_vfscanf");
__attribute__((__format__(__gnu_printf__,2,3))) int fprintf (FILE *__stream, const char *__format, ...)
__asm__("__mingw_fprintf");
__attribute__((__format__(__gnu_printf__,1,2))) int printf (const char *__format, ...)
__asm__("__mingw_printf");
__attribute__((__format__(__gnu_printf__,2,3))) int sprintf (char *__stream, const char *__format, ...)
__asm__("__mingw_sprintf");
__attribute__((__format__(__gnu_printf__,2,0))) int vfprintf (FILE *__stream, const char *__format, __builtin_va_list __local_argv)
__asm__("__mingw_vfprintf");
__attribute__((__format__(__gnu_printf__,1,0))) int vprintf (const char *__format, __builtin_va_list __local_argv)
__asm__("__mingw_vprintf");
static __attribute__ ((__unused__)) inline __cdecl
__attribute__((__format__(__gnu_printf__,2,0))) int vsprintf (char *__stream, const char *__format, __builtin_va_list __local_argv)
{
return __mingw_vsprintf( __stream, __format, __local_argv );
}
__attribute__((__format__(__gnu_printf__,3,4))) int snprintf (char *__stream, size_t __n, const char *__format, ...)
__asm__("__mingw_snprintf");
static __attribute__ ((__unused__)) inline __cdecl
__attribute__((__format__(__gnu_printf__,3,0))) int vsnprintf (char *__stream, size_t __n, const char *__format, __builtin_va_list __local_argv)
{
return __mingw_vsnprintf( __stream, __n, __format, __local_argv );
}
int __cdecl _filbuf(FILE *_File);
int __cdecl _flsbuf(int _Ch,FILE *_File);
FILE *__cdecl _fsopen(const char *_Filename,const char *_Mode,int _ShFlag);
void __cdecl clearerr(FILE *_File);
int __cdecl fclose(FILE *_File);
int __cdecl _fcloseall(void);
FILE *__cdecl _fdopen(int _FileHandle,const char *_Mode);
int __cdecl feof(FILE *_File);
int __cdecl ferror(FILE *_File);
int __cdecl fflush(FILE *_File);
int __cdecl fgetc(FILE *_File);
int __cdecl _fgetchar(void);
int __cdecl fgetpos(FILE * __restrict__ _File ,fpos_t * __restrict__ _Pos);
int __cdecl fgetpos64(FILE * __restrict__ _File ,fpos_t * __restrict__ _Pos);
char *__cdecl fgets(char * __restrict__ _Buf,int _MaxCount,FILE * __restrict__ _File);
int __cdecl _fileno(FILE *_File);
char *__cdecl _tempnam(const char *_DirName,const char *_FilePrefix);
int __cdecl _flushall(void);
FILE *__cdecl fopen(const char * __restrict__ _Filename,const char * __restrict__ _Mode) ;
FILE *__cdecl fopen64(const char * __restrict__ filename,const char * __restrict__ mode);
int __cdecl fputc(int _Ch,FILE *_File);
int __cdecl _fputchar(int _Ch);
int __cdecl fputs(const char * __restrict__ _Str,FILE * __restrict__ _File);
size_t __cdecl fread(void * __restrict__ _DstBuf,size_t _ElementSize,size_t _Count,FILE * __restrict__ _File);
FILE *__cdecl freopen(const char * __restrict__ _Filename,const char * __restrict__ _Mode,FILE * __restrict__ _File) ;
FILE *__cdecl freopen64(const char * __restrict__ _Filename,const char * __restrict__ _Mode,FILE * __restrict__ _File);
int __cdecl fsetpos(FILE *_File,const fpos_t *_Pos);
int __cdecl fsetpos64(FILE *_File,const fpos_t *_Pos);
int __cdecl fseek(FILE *_File,long _Offset,int _Origin);
long __cdecl ftell(FILE *_File);
int __cdecl _fseeki64(FILE *_File,long long _Offset,int _Origin);
long long __cdecl _ftelli64(FILE *_File);
int __cdecl fseeko(FILE *_File, _off_t _Offset, int _Origin);
int __cdecl fseeko64(FILE *_File, _off64_t _Offset, int _Origin);
_off_t __cdecl ftello(FILE *_File);
_off64_t __cdecl ftello64(FILE *_File);
size_t __cdecl fwrite(const void * __restrict__ _Str,size_t _Size,size_t _Count,FILE * __restrict__ _File);
int __cdecl getc(FILE *_File);
int __cdecl getchar(void);
int __cdecl _getmaxstdio(void);
char *__cdecl gets(char *_Buffer)
__attribute__((__warning__("Using gets() is always unsafe - use fgets() instead")))
;
int __cdecl _getw(FILE *_File);
void __cdecl perror(const char *_ErrMsg);
int __cdecl _pclose(FILE *_File);
FILE *__cdecl _popen(const char *_Command,const char *_Mode);
int __cdecl putc(int _Ch,FILE *_File);
int __cdecl putchar(int _Ch);
int __cdecl puts(const char *_Str);
int __cdecl _putw(int _Word,FILE *_File);
int __cdecl remove(const char *_Filename);
int __cdecl rename(const char *_OldFilename,const char *_NewFilename);
int __cdecl _unlink(const char *_Filename);
int __cdecl unlink(const char *_Filename) ;
void __cdecl rewind(FILE *_File);
int __cdecl _rmtmp(void);
void __cdecl setbuf(FILE * __restrict__ _File,char * __restrict__ _Buffer) ;
int __cdecl _setmaxstdio(int _Max);
unsigned int __cdecl _set_output_format(unsigned int _Format);
unsigned int __cdecl _get_output_format(void);
int __cdecl setvbuf(FILE * __restrict__ _File,char * __restrict__ _Buf,int _Mode,size_t _Size);
__attribute__((__format__ (__gnu_printf__, 1, 2))) int __cdecl _scprintf(const char * __restrict__ _Format,...);
__attribute__((__format__ (__gnu_scanf__, 3, 4))) int __cdecl _snscanf(const char * __restrict__ _Src,size_t _MaxCount,const char * __restrict__ _Format,...) ;
__attribute__((__format__(__ms_printf__, 1,0))) int __cdecl _vscprintf(const char * __restrict__ _Format,va_list _ArgList);
FILE *__cdecl tmpfile(void) ;
FILE *__cdecl tmpfile64(void);
char *__cdecl tmpnam(char *_Buffer);
int __cdecl ungetc(int _Ch,FILE *_File);
__attribute__((__format__ (__gnu_printf__, 3, 0))) int __cdecl _vsnprintf(char * __restrict__ _Dest,size_t _Count,const char * __restrict__ _Format,va_list _Args) ;
__attribute__((__format__ (__gnu_printf__, 3, 4))) int __cdecl _snprintf(char * __restrict__ _Dest,size_t _Count,const char * __restrict__ _Format,...) ;
int __cdecl _set_printf_count_output(int _Value);
int __cdecl _get_printf_count_output(void);
int __cdecl __mingw_swscanf(const wchar_t * __restrict__ _Src,const wchar_t * __restrict__ _Format,...);
int __cdecl __mingw_vswscanf (const wchar_t * __restrict__ _Str,const wchar_t * __restrict__ Format,va_list argp);
int __cdecl __mingw_wscanf(const wchar_t * __restrict__ _Format,...);
int __cdecl __mingw_vwscanf(const wchar_t * __restrict__ Format, va_list argp);
int __cdecl __mingw_fwscanf(FILE * __restrict__ _File,const wchar_t * __restrict__ _Format,...);
int __cdecl __mingw_vfwscanf (FILE * __restrict__ fp, const wchar_t * __restrict__ Format,va_list argp);
int __cdecl __mingw_fwprintf(FILE * __restrict__ _File,const wchar_t * __restrict__ _Format,...);
int __cdecl __mingw_wprintf(const wchar_t * __restrict__ _Format,...);
int __cdecl __mingw_vfwprintf(FILE * __restrict__ _File,const wchar_t * __restrict__ _Format,va_list _ArgList);
int __cdecl __mingw_vwprintf(const wchar_t * __restrict__ _Format,va_list _ArgList);
int __cdecl __mingw_snwprintf (wchar_t * __restrict__ s, size_t n, const wchar_t * __restrict__ format, ...);
int __cdecl __mingw_vsnwprintf (wchar_t * __restrict__ , size_t, const wchar_t * __restrict__ , va_list);
int __cdecl __mingw_swprintf(wchar_t * __restrict__ , size_t, const wchar_t * __restrict__ , ...);
int __cdecl __mingw_vswprintf(wchar_t * __restrict__ , size_t, const wchar_t * __restrict__ ,va_list);
int __cdecl __ms_swscanf(const wchar_t * __restrict__ _Src,const wchar_t * __restrict__ _Format,...)
__asm__("swscanf");
int __cdecl __ms_vswscanf(const wchar_t * __restrict__ _Src,const wchar_t * __restrict__ _Format,va_list)
__asm__("vswscanf");
int __cdecl __ms_wscanf(const wchar_t * __restrict__ _Format,...)
__asm__("wscanf");
int __cdecl __ms_vwscanf(const wchar_t * __restrict__ _Format, va_list)
__asm__("vwscanf");
int __cdecl __ms_fwscanf(FILE * __restrict__ _File,const wchar_t * __restrict__ _Format,...)
__asm__("fwscanf");
int __cdecl __ms_vfwscanf(FILE * __restrict__ _File,const wchar_t * __restrict__ _Format,va_list)
__asm__("vfwscanf");
int __cdecl __ms_fwprintf(FILE * __restrict__ _File,const wchar_t * __restrict__ _Format,...);
int __cdecl __ms_wprintf(const wchar_t * __restrict__ _Format,...)
__asm__("wprintf");
int __cdecl __ms_vfwprintf(FILE * __restrict__ _File,const wchar_t * __restrict__ _Format,va_list _ArgList)
__asm__("vfwprintf");
int __cdecl __ms_vwprintf(const wchar_t * __restrict__ _Format,va_list _ArgList)
__asm__("vwprintf");
int __cdecl __ms_swprintf(wchar_t * __restrict__ , size_t, const wchar_t * __restrict__ , ...)
__asm__("swprintf");
int __cdecl __ms_vswprintf(wchar_t * __restrict__ , size_t, const wchar_t * __restrict__ ,va_list)
__asm__("vswprintf");
int __cdecl __ms_snwprintf(wchar_t * __restrict__ , size_t, const wchar_t * __restrict__ , ...)
__asm__("snwprintf");
int __cdecl __ms_vsnwprintf(wchar_t * __restrict__ , size_t, const wchar_t * __restrict__ , va_list)
__asm__("vsnwprintf");
int __cdecl __stdio_common_vswprintf(unsigned long long options, wchar_t *str, size_t len, const wchar_t *format, _locale_t locale, va_list valist);
int __cdecl __stdio_common_vfwprintf(unsigned long long options, FILE *file, const wchar_t *format, _locale_t locale, va_list valist);
int __cdecl __stdio_common_vswscanf(unsigned long long options, const wchar_t *input, size_t length, const wchar_t *format, _locale_t locale, va_list valist);
int __cdecl __stdio_common_vfwscanf(unsigned long long options, FILE *file, const wchar_t *format, _locale_t locale, va_list valist);
int swscanf(const wchar_t *__source, const wchar_t *__format, ...)
__asm__("__mingw_swscanf");
int wscanf(const wchar_t *__format, ...)
__asm__("__mingw_wscanf");
int fwscanf(FILE *__stream, const wchar_t *__format, ...)
__asm__("__mingw_fwscanf");
int vswscanf (const wchar_t * __restrict__ __source, const wchar_t * __restrict__ __format, __builtin_va_list __local_argv)
__asm__("__mingw_vswscanf");
int vwscanf(const wchar_t *__format, __builtin_va_list __local_argv)
__asm__("__mingw_vwscanf");
int vfwscanf (FILE *__stream, const wchar_t *__format, __builtin_va_list __local_argv)
__asm__("__mingw_vfwscanf");
int fwprintf (FILE *__stream, const wchar_t *__format, ...)
__asm__("__mingw_fwprintf");
int wprintf (const wchar_t *__format, ...)
__asm__("__mingw_wprintf");
int vfwprintf (FILE *__stream, const wchar_t *__format, __builtin_va_list __local_argv)
__asm__("__mingw_vfwprintf");
int vwprintf (const wchar_t *__format, __builtin_va_list __local_argv)
__asm__("__mingw_vwprintf");
int swprintf (wchar_t *__stream, size_t __n, const wchar_t *__format, ...)
__asm__("__mingw_swprintf");
static __attribute__ ((__unused__)) inline __cdecl
int vswprintf (wchar_t *__stream, size_t __n, const wchar_t *__format, __builtin_va_list __local_argv)
{
return __mingw_vswprintf( __stream, __n, __format, __local_argv );
}
int snwprintf (wchar_t *__stream, size_t __n, const wchar_t *__format, ...)
__asm__("__mingw_snwprintf");
static __attribute__ ((__unused__)) inline __cdecl
int vsnwprintf (wchar_t *__stream, size_t __n, const wchar_t *__format, __builtin_va_list __local_argv)
{
return __mingw_vsnwprintf( __stream, __n, __format, __local_argv );
}
FILE *__cdecl _wfsopen(const wchar_t *_Filename,const wchar_t *_Mode,int _ShFlag);
wint_t __cdecl fgetwc(FILE *_File);
wint_t __cdecl _fgetwchar(void);
wint_t __cdecl fputwc(wchar_t _Ch,FILE *_File);
wint_t __cdecl _fputwchar(wchar_t _Ch);
wint_t __cdecl getwc(FILE *_File);
wint_t __cdecl getwchar(void);
wint_t __cdecl putwc(wchar_t _Ch,FILE *_File);
wint_t __cdecl putwchar(wchar_t _Ch);
wint_t __cdecl ungetwc(wint_t _Ch,FILE *_File);
wchar_t *__cdecl fgetws(wchar_t * __restrict__ _Dst,int _SizeInWords,FILE * __restrict__ _File);
int __cdecl fputws(const wchar_t * __restrict__ _Str,FILE * __restrict__ _File);
wchar_t *__cdecl _getws(wchar_t *_String) ;
int __cdecl _putws(const wchar_t *_Str);
int __cdecl _scwprintf(const wchar_t * __restrict__ _Format,...);
int __cdecl _snwprintf(wchar_t * __restrict__ _Dest,size_t _Count,const wchar_t * __restrict__ _Format,...) ;
int __cdecl _vsnwprintf(wchar_t * __restrict__ _Dest,size_t _Count,const wchar_t * __restrict__ _Format,va_list _Args) ;
int __cdecl _vscwprintf(const wchar_t * __restrict__ _Format,va_list _ArgList);
int __cdecl _swprintf(wchar_t * __restrict__ _Dest,const wchar_t * __restrict__ _Format,...);
int __cdecl _vswprintf(wchar_t * __restrict__ _Dest,const wchar_t * __restrict__ _Format,va_list _Args);
wchar_t *__cdecl _wtempnam(const wchar_t *_Directory,const wchar_t *_FilePrefix);
int __cdecl _snwscanf(const wchar_t * __restrict__ _Src,size_t _MaxCount,const wchar_t * __restrict__ _Format,...);
FILE *__cdecl _wfdopen(int _FileHandle ,const wchar_t *_Mode);
FILE *__cdecl _wfopen(const wchar_t * __restrict__ _Filename,const wchar_t *__restrict__ _Mode) ;
FILE *__cdecl _wfreopen(const wchar_t * __restrict__ _Filename,const wchar_t * __restrict__ _Mode,FILE * __restrict__ _OldFile) ;
void __cdecl _wperror(const wchar_t *_ErrMsg);
FILE *__cdecl _wpopen(const wchar_t *_Command,const wchar_t *_Mode);
int __cdecl _wremove(const wchar_t *_Filename);
wchar_t *__cdecl _wtmpnam(wchar_t *_Buffer);
wint_t __cdecl _fgetwc_nolock(FILE *_File);
wint_t __cdecl _fputwc_nolock(wchar_t _Ch,FILE *_File);
wint_t __cdecl _ungetwc_nolock(wint_t _Ch,FILE *_File);
int __cdecl _fgetc_nolock(FILE *_File);
int __cdecl _fputc_nolock(int _Char, FILE *_File);
int __cdecl _getc_nolock(FILE *_File);
int __cdecl _putc_nolock(int _Char, FILE *_File);
void __cdecl _lock_file(FILE *_File);
void __cdecl _unlock_file(FILE *_File);
int __cdecl _fclose_nolock(FILE *_File);
int __cdecl _fflush_nolock(FILE *_File);
size_t __cdecl _fread_nolock(void * __restrict__ _DstBuf,size_t _ElementSize,size_t _Count,FILE * __restrict__ _File);
int __cdecl _fseek_nolock(FILE *_File,long _Offset,int _Origin);
long __cdecl _ftell_nolock(FILE *_File);
__extension__ int __cdecl _fseeki64_nolock(FILE *_File,long long _Offset,int _Origin);
__extension__ long long __cdecl _ftelli64_nolock(FILE *_File);
size_t __cdecl _fwrite_nolock(const void * __restrict__ _DstBuf,size_t _Size,size_t _Count,FILE * __restrict__ _File);
int __cdecl _ungetc_nolock(int _Ch,FILE *_File);
char *__cdecl tempnam(const char *_Directory,const char *_FilePrefix) ;
int __cdecl fcloseall(void) ;
FILE *__cdecl fdopen(int _FileHandle,const char *_Format) ;
int __cdecl fgetchar(void) ;
int __cdecl fileno(FILE *_File) ;
int __cdecl flushall(void) ;
int __cdecl fputchar(int _Ch) ;
int __cdecl getw(FILE *_File) ;
int __cdecl putw(int _Ch,FILE *_File) ;
int __cdecl rmtmp(void) ;
int __cdecl __mingw_str_wide_utf8 (const wchar_t * const wptr, char **mbptr, size_t * buflen);
int __cdecl __mingw_str_utf8_wide (const char *const mbptr, wchar_t ** wptr, size_t * buflen);
void __cdecl __mingw_str_free(void *ptr);
intptr_t __cdecl _wspawnl(int _Mode,const wchar_t *_Filename,const wchar_t *_ArgList,...);
intptr_t __cdecl _wspawnle(int _Mode,const wchar_t *_Filename,const wchar_t *_ArgList,...);
intptr_t __cdecl _wspawnlp(int _Mode,const wchar_t *_Filename,const wchar_t *_ArgList,...);
intptr_t __cdecl _wspawnlpe(int _Mode,const wchar_t *_Filename,const wchar_t *_ArgList,...);
intptr_t __cdecl _wspawnv(int _Mode,const wchar_t *_Filename,const wchar_t *const *_ArgList);
intptr_t __cdecl _wspawnve(int _Mode,const wchar_t *_Filename,const wchar_t *const *_ArgList,const wchar_t *const *_Env);
intptr_t __cdecl _wspawnvp(int _Mode,const wchar_t *_Filename,const wchar_t *const *_ArgList);
intptr_t __cdecl _wspawnvpe(int _Mode,const wchar_t *_Filename,const wchar_t *const *_ArgList,const wchar_t *const *_Env);
intptr_t __cdecl _spawnv(int _Mode,const char *_Filename,const char *const *_ArgList);
intptr_t __cdecl _spawnve(int _Mode,const char *_Filename,const char *const *_ArgList,const char *const *_Env);
intptr_t __cdecl _spawnvp(int _Mode,const char *_Filename,const char *const *_ArgList);
intptr_t __cdecl _spawnvpe(int _Mode,const char *_Filename,const char *const *_ArgList,const char *const *_Env);
errno_t __cdecl clearerr_s(FILE *_File);
size_t __cdecl fread_s(void *_DstBuf,size_t _DstSize,size_t _ElementSize,size_t _Count,FILE *_File);
int __cdecl __stdio_common_vsprintf_s(unsigned long long _Options, char *_Str, size_t _Len, const char *_Format, _locale_t _Locale, va_list _ArgList);
int __cdecl __stdio_common_vsprintf_p(unsigned long long _Options, char *_Str, size_t _Len, const char *_Format, _locale_t _Locale, va_list _ArgList);
int __cdecl __stdio_common_vsnprintf_s(unsigned long long _Options, char *_Str, size_t _Len, size_t _MaxCount, const char *_Format, _locale_t _Locale, va_list _ArgList);
int __cdecl __stdio_common_vfprintf_s(unsigned long long _Options, FILE *_File, const char *_Format, _locale_t _Locale, va_list _ArgList);
int __cdecl __stdio_common_vfprintf_p(unsigned long long _Options, FILE *_File, const char *_Format, _locale_t _Locale, va_list _ArgList);
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vfscanf_s_l(FILE *_File, const char *_Format, _locale_t _Locale, va_list _ArgList)
{
return __stdio_common_vfscanf(0x0001ULL, _File, _Format, _Locale, _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl vfscanf_s(FILE *_File, const char *_Format, va_list _ArgList)
{
return _vfscanf_s_l(_File, _Format, ((void *)0), _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vscanf_s_l(const char *_Format, _locale_t _Locale, va_list _ArgList)
{
return _vfscanf_s_l((__acrt_iob_func(0)), _Format, _Locale, _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl vscanf_s(const char *_Format, va_list _ArgList)
{
return _vfscanf_s_l((__acrt_iob_func(0)), _Format, ((void *)0), _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _fscanf_s_l(FILE *_File, const char *_Format, _locale_t _Locale, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Locale);
_Ret = _vfscanf_s_l(_File, _Format, _Locale, _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl fscanf_s(FILE *_File, const char *_Format, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Format);
_Ret = _vfscanf_s_l(_File, _Format, ((void *)0), _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _scanf_s_l(const char *_Format, _locale_t _Locale ,...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Locale);
_Ret = _vfscanf_s_l((__acrt_iob_func(0)), _Format, _Locale, _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl scanf_s(const char *_Format, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Format);
_Ret = _vfscanf_s_l((__acrt_iob_func(0)), _Format, ((void *)0), _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vfscanf_l(FILE *_File, const char *_Format, _locale_t _Locale, va_list _ArgList)
{
return __stdio_common_vfscanf(0, _File, _Format, _Locale, _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vscanf_l(const char *_Format, _locale_t _Locale, va_list _ArgList)
{
return _vfscanf_l((__acrt_iob_func(0)), _Format, _Locale, _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _fscanf_l(FILE *_File, const char *_Format, _locale_t _Locale, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Locale);
_Ret = _vfscanf_l(_File, _Format, _Locale, _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _scanf_l(const char *_Format, _locale_t _Locale, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Locale);
_Ret = _vfscanf_l((__acrt_iob_func(0)), _Format, _Locale, _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vsscanf_s_l(const char *_Src, const char *_Format, _locale_t _Locale, va_list _ArgList)
{
return __stdio_common_vsscanf(0x0001ULL, _Src, (size_t)-1, _Format, _Locale, _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl vsscanf_s(const char *_Src, const char *_Format, va_list _ArgList)
{
return _vsscanf_s_l(_Src, _Format, ((void *)0), _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _sscanf_s_l(const char *_Src, const char *_Format, _locale_t _Locale, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Locale);
_Ret = _vsscanf_s_l(_Src, _Format, _Locale, _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl sscanf_s(const char *_Src, const char *_Format, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Format);
_Ret = _vsscanf_s_l(_Src, _Format, ((void *)0), _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vsscanf_l(const char *_Src, const char *_Format, _locale_t _Locale, va_list _ArgList)
{
return __stdio_common_vsscanf(0, _Src, (size_t)-1, _Format, _Locale, _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _sscanf_l(const char *_Src, const char *_Format, _locale_t _Locale, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Locale);
_Ret = _vsscanf_l(_Src, _Format, _Locale, _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _snscanf_s_l(const char *_Src, size_t _MaxCount, const char *_Format, _locale_t _Locale, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Locale);
_Ret = __stdio_common_vsscanf(0x0001ULL, _Src, _MaxCount, _Format, _Locale, _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _snscanf_s(const char *_Src, size_t _MaxCount, const char *_Format, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Format);
_Ret = __stdio_common_vsscanf(0x0001ULL, _Src, _MaxCount, _Format, ((void *)0), _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _snscanf_l(const char *_Src, size_t _MaxCount, const char *_Format, _locale_t _Locale, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Locale);
_Ret = __stdio_common_vsscanf(0, _Src, _MaxCount, _Format, _Locale, _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vfprintf_s_l(FILE *_File, const char *_Format, _locale_t _Locale, va_list _ArgList)
{
return __stdio_common_vfprintf_s((*__local_stdio_printf_options()), _File, _Format, _Locale, _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl vfprintf_s(FILE *_File, const char *_Format, va_list _ArgList)
{
return _vfprintf_s_l(_File, _Format, ((void *)0), _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vprintf_s_l(const char *_Format, _locale_t _Locale, va_list _ArgList)
{
return _vfprintf_s_l((__acrt_iob_func(1)), _Format, _Locale, _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl vprintf_s(const char *_Format, va_list _ArgList)
{
return _vfprintf_s_l((__acrt_iob_func(1)), _Format, ((void *)0), _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _fprintf_s_l(FILE *_File, const char *_Format, _locale_t _Locale, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Locale);
_Ret = _vfprintf_s_l(_File, _Format, _Locale, _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _printf_s_l(const char *_Format, _locale_t _Locale, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Locale);
_Ret = _vfprintf_s_l((__acrt_iob_func(1)), _Format, _Locale, _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl fprintf_s(FILE *_File, const char *_Format, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Format);
_Ret = _vfprintf_s_l(_File, _Format, ((void *)0), _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl printf_s(const char *_Format, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Format);
_Ret = _vfprintf_s_l((__acrt_iob_func(1)), _Format, ((void *)0), _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vsnprintf_c_l(char *_DstBuf, size_t _MaxCount, const char *_Format, _locale_t _Locale, va_list _ArgList)
{
return __stdio_common_vsprintf((*__local_stdio_printf_options()), _DstBuf, _MaxCount, _Format, _Locale, _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vsnprintf_c(char *_DstBuf, size_t _MaxCount, const char *_Format, va_list _ArgList)
{
return _vsnprintf_c_l(_DstBuf, _MaxCount, _Format, ((void *)0), _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _snprintf_c_l(char *_DstBuf, size_t _MaxCount, const char *_Format, _locale_t _Locale, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Locale);
_Ret = _vsnprintf_c_l(_DstBuf, _MaxCount, _Format, _Locale, _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _snprintf_c(char *_DstBuf, size_t _MaxCount, const char *_Format, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Format);
_Ret = _vsnprintf_c_l(_DstBuf, _MaxCount, _Format, ((void *)0), _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vsnprintf_s_l(char *_DstBuf, size_t _DstSize, size_t _MaxCount, const char *_Format, _locale_t _Locale, va_list _ArgList)
{
return __stdio_common_vsnprintf_s((*__local_stdio_printf_options()), _DstBuf, _DstSize, _MaxCount, _Format, _Locale, _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl vsnprintf_s(char *_DstBuf, size_t _DstSize, size_t _MaxCount, const char *_Format, va_list _ArgList)
{
return _vsnprintf_s_l(_DstBuf, _DstSize, _MaxCount, _Format, ((void *)0), _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vsnprintf_s(char *_DstBuf, size_t _DstSize, size_t _MaxCount, const char *_Format, va_list _ArgList)
{
return _vsnprintf_s_l(_DstBuf, _DstSize, _MaxCount, _Format, ((void *)0), _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _snprintf_s_l(char *_DstBuf, size_t _DstSize, size_t _MaxCount, const char *_Format, _locale_t _Locale, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Locale);
_Ret = _vsnprintf_s_l(_DstBuf, _DstSize, _MaxCount, _Format, _Locale, _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _snprintf_s(char *_DstBuf, size_t _DstSize, size_t _MaxCount, const char *_Format, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Format);
_Ret = _vsnprintf_s_l(_DstBuf, _DstSize, _MaxCount, _Format, ((void *)0), _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vsprintf_s_l(char *_DstBuf, size_t _DstSize, const char *_Format, _locale_t _Locale, va_list _ArgList)
{
return __stdio_common_vsprintf_s((*__local_stdio_printf_options()), _DstBuf, _DstSize, _Format, _Locale, _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl vsprintf_s(char *_DstBuf, size_t _Size, const char *_Format, va_list _ArgList)
{
return _vsprintf_s_l(_DstBuf, _Size, _Format, ((void *)0), _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _sprintf_s_l(char *_DstBuf, size_t _DstSize, const char *_Format, _locale_t _Locale, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Locale);
_Ret = _vsprintf_s_l(_DstBuf, _DstSize, _Format, _Locale, _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl sprintf_s(char *_DstBuf, size_t _DstSize, const char *_Format, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Format);
_Ret = _vsprintf_s_l(_DstBuf, _DstSize, _Format, ((void *)0), _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vfprintf_p_l(FILE *_File, const char *_Format, _locale_t _Locale, va_list _ArgList)
{
return __stdio_common_vfprintf_p((*__local_stdio_printf_options()), _File, _Format, _Locale, _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vfprintf_p(FILE *_File, const char *_Format, va_list _ArgList)
{
return _vfprintf_p_l(_File, _Format, ((void *)0), _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vprintf_p_l(const char *_Format, _locale_t _Locale, va_list _ArgList)
{
return _vfprintf_p_l((__acrt_iob_func(1)), _Format, _Locale, _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vprintf_p(const char *_Format, va_list _ArgList)
{
return _vfprintf_p_l((__acrt_iob_func(1)), _Format, ((void *)0), _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _fprintf_p_l(FILE *_File, const char *_Format, _locale_t _Locale, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Locale);
_Ret = __stdio_common_vfprintf_p((*__local_stdio_printf_options()), _File, _Format, _Locale, _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _fprintf_p(FILE *_File, const char *_Format, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Format);
_Ret = _vfprintf_p_l(_File, _Format, ((void *)0), _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _printf_p_l(const char *_Format, _locale_t _Locale, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Locale);
_Ret = _vfprintf_p_l((__acrt_iob_func(1)), _Format, _Locale, _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _printf_p(const char *_Format, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Format);
_Ret = _vfprintf_p_l((__acrt_iob_func(1)), _Format, ((void *)0), _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vsprintf_p_l(char *_DstBuf, size_t _MaxCount, const char *_Format, _locale_t _Locale, va_list _ArgList)
{
return __stdio_common_vsprintf_p((*__local_stdio_printf_options()), _DstBuf, _MaxCount, _Format, _Locale, _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vsprintf_p(char *_Dst, size_t _MaxCount, const char *_Format, va_list _ArgList)
{
return _vsprintf_p_l(_Dst, _MaxCount, _Format, ((void *)0), _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _sprintf_p_l(char *_DstBuf, size_t _MaxCount, const char *_Format, _locale_t _Locale, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Locale);
_Ret = _vsprintf_p_l(_DstBuf, _MaxCount, _Format, _Locale, _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _sprintf_p(char *_Dst, size_t _MaxCount, const char *_Format, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Format);
_Ret = _vsprintf_p_l(_Dst, _MaxCount, _Format, ((void *)0), _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vscprintf_p_l(const char *_Format, _locale_t _Locale, va_list _ArgList)
{
return __stdio_common_vsprintf_p(0x0002ULL, ((void *)0), 0, _Format, _Locale, _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vscprintf_p(const char *_Format, va_list _ArgList)
{
return _vscprintf_p_l(_Format, ((void *)0), _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _scprintf_p_l(const char *_Format, _locale_t _Locale, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Locale);
_Ret = _vscprintf_p_l(_Format, _Locale, _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _scprintf_p(const char *_Format, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Format);
_Ret = _vscprintf_p_l(_Format, ((void *)0), _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vfprintf_l(FILE *_File, const char *_Format, _locale_t _Locale, va_list _ArgList)
{
return __stdio_common_vfprintf((*__local_stdio_printf_options()), _File, _Format, _Locale, _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vprintf_l(const char *_Format, _locale_t _Locale, va_list _ArgList)
{
return _vfprintf_l((__acrt_iob_func(1)), _Format, _Locale, _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _fprintf_l(FILE *_File, const char *_Format, _locale_t _Locale, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Locale);
_Ret = _vfprintf_l(_File, _Format, _Locale, _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _printf_l(const char *_Format, _locale_t _Locale, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Locale);
_Ret = _vfprintf_l((__acrt_iob_func(1)), _Format, _Locale, _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vsnprintf_l(char *_DstBuf, size_t _MaxCount, const char *_Format, _locale_t _Locale, va_list _ArgList)
{
return __stdio_common_vsprintf(0x0001ULL, _DstBuf, _MaxCount, _Format, _Locale, _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _snprintf_l(char *_DstBuf, size_t _MaxCount, const char *_Format, _locale_t _Locale, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Locale);
_Ret = _vsnprintf_l(_DstBuf, _MaxCount, _Format, _Locale, _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vsprintf_l(char *_DstBuf, const char *_Format, _locale_t _Locale, va_list _ArgList)
{
return _vsnprintf_l(_DstBuf, (size_t)-1, _Format, _Locale, _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _sprintf_l(char *_DstBuf, const char *_Format, _locale_t _Locale, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Locale);
_Ret = _vsprintf_l(_DstBuf, _Format, _Locale, _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vscprintf_l(const char *_Format, _locale_t _Locale, va_list _ArgList)
{
return __stdio_common_vsprintf(0x0002ULL, ((void *)0), 0, _Format, _Locale, _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _scprintf_l(const char *_Format, _locale_t _Locale, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Locale);
_Ret = _vscprintf_l(_Format, _Locale, _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
errno_t __cdecl fopen_s(FILE **_File,const char *_Filename,const char *_Mode);
errno_t __cdecl freopen_s(FILE** _File, const char *_Filename, const char *_Mode, FILE *_Stream);
char* __cdecl gets_s(char*,rsize_t);
errno_t __cdecl tmpfile_s(FILE **_File);
errno_t __cdecl tmpnam_s(char*,rsize_t);
wchar_t *__cdecl _getws_s(wchar_t *_Str,size_t _SizeInWords);
int __cdecl __stdio_common_vswprintf_s(unsigned long long _Options, wchar_t *_Str, size_t _Len, const wchar_t *_Format, _locale_t _Locale, va_list _ArgList);
int __cdecl __stdio_common_vsnwprintf_s(unsigned long long _Options, wchar_t *_Str, size_t _Len, size_t _MaxCount, const wchar_t *_Format, _locale_t _Locale, va_list _ArgList);
int __cdecl __stdio_common_vfwprintf_s(unsigned long long _Options, FILE *_File, const wchar_t *_Format, _locale_t _Locale, va_list _ArgList);
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vfwscanf_s_l(FILE *_File, const wchar_t *_Format, _locale_t _Locale, va_list _ArgList)
{
return __stdio_common_vfwscanf((*__local_stdio_scanf_options()) | 0x0001ULL, _File, _Format, _Locale, _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl vfwscanf_s(FILE* _File, const wchar_t *_Format, va_list _ArgList)
{
return _vfwscanf_s_l(_File, _Format, ((void *)0), _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vwscanf_s_l(const wchar_t *_Format, _locale_t _Locale, va_list _ArgList)
{
return _vfwscanf_s_l((__acrt_iob_func(0)), _Format, _Locale, _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl vwscanf_s(const wchar_t *_Format, va_list _ArgList)
{
return _vfwscanf_s_l((__acrt_iob_func(0)), _Format, ((void *)0), _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _fwscanf_s_l(FILE *_File, const wchar_t *_Format, _locale_t _Locale, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Locale);
_Ret = _vfwscanf_s_l(_File, _Format, _Locale, _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl fwscanf_s(FILE *_File, const wchar_t *_Format, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Format);
_Ret = _vfwscanf_s_l(_File, _Format, ((void *)0), _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _wscanf_s_l(const wchar_t *_Format, _locale_t _Locale, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Locale);
_Ret = _vfwscanf_s_l((__acrt_iob_func(0)), _Format, _Locale, _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl wscanf_s(const wchar_t *_Format, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Format);
_Ret = _vfwscanf_s_l((__acrt_iob_func(0)), _Format, ((void *)0), _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vswscanf_s_l(const wchar_t *_Src, const wchar_t *_Format, _locale_t _Locale, va_list _ArgList)
{
return __stdio_common_vswscanf((*__local_stdio_scanf_options()) | 0x0001ULL, _Src, (size_t)-1, _Format, _Locale, _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl vswscanf_s(const wchar_t *_Src, const wchar_t *_Format, va_list _ArgList)
{
return _vswscanf_s_l(_Src, _Format, ((void *)0), _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _swscanf_s_l(const wchar_t *_Src, const wchar_t *_Format, _locale_t _Locale, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Locale);
_Ret = _vswscanf_s_l(_Src, _Format, _Locale, _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl swscanf_s(const wchar_t *_Src, const wchar_t *_Format, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Format);
_Ret = _vswscanf_s_l(_Src, _Format, ((void *)0), _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vsnwscanf_s_l(const wchar_t *_Src, size_t _MaxCount, const wchar_t *_Format, _locale_t _Locale, va_list _ArgList)
{
return __stdio_common_vswscanf((*__local_stdio_scanf_options()) | 0x0001ULL, _Src, _MaxCount, _Format, _Locale, _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _snwscanf_s_l(const wchar_t *_Src, size_t _MaxCount, const wchar_t *_Format, _locale_t _Locale, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Locale);
_Ret = _vsnwscanf_s_l(_Src, _MaxCount, _Format, _Locale, _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _snwscanf_s(const wchar_t *_Src, size_t _MaxCount, const wchar_t *_Format, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Format);
_Ret = _vsnwscanf_s_l(_Src, _MaxCount, _Format, ((void *)0), _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vfwprintf_s_l(FILE *_File, const wchar_t *_Format, _locale_t _Locale, va_list _ArgList)
{
return __stdio_common_vfwprintf_s((*__local_stdio_printf_options()), _File, _Format, _Locale, _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vwprintf_s_l(const wchar_t *_Format, _locale_t _Locale, va_list _ArgList)
{
return _vfwprintf_s_l((__acrt_iob_func(1)), _Format, _Locale, _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl vfwprintf_s(FILE *_File, const wchar_t *_Format, va_list _ArgList)
{
return _vfwprintf_s_l(_File, _Format, ((void *)0), _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl vwprintf_s(const wchar_t *_Format, va_list _ArgList)
{
return _vfwprintf_s_l((__acrt_iob_func(1)), _Format, ((void *)0), _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _fwprintf_s_l(FILE *_File, const wchar_t *_Format, _locale_t _Locale, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Locale);
_Ret = _vfwprintf_s_l(_File, _Format, _Locale, _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _wprintf_s_l(const wchar_t *_Format, _locale_t _Locale, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Locale);
_Ret = _vfwprintf_s_l((__acrt_iob_func(1)), _Format, _Locale, _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl fwprintf_s(FILE *_File, const wchar_t *_Format, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Format);
_Ret = _vfwprintf_s_l(_File, _Format, ((void *)0), _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl wprintf_s(const wchar_t *_Format, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Format);
_Ret = _vfwprintf_s_l((__acrt_iob_func(1)), _Format, ((void *)0), _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vswprintf_s_l(wchar_t *_DstBuf, size_t _DstSize, const wchar_t *_Format, _locale_t _Locale, va_list _ArgList)
{
return __stdio_common_vswprintf_s((*__local_stdio_printf_options()), _DstBuf, _DstSize, _Format, _Locale, _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl vswprintf_s(wchar_t *_DstBuf, size_t _DstSize, const wchar_t *_Format, va_list _ArgList)
{
return _vswprintf_s_l(_DstBuf, _DstSize, _Format, ((void *)0), _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _swprintf_s_l(wchar_t *_DstBuf, size_t _DstSize, const wchar_t *_Format, _locale_t _Locale, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Locale);
_Ret = _vswprintf_s_l(_DstBuf, _DstSize, _Format, _Locale, _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl swprintf_s(wchar_t *_DstBuf, size_t _DstSize, const wchar_t *_Format, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Format);
_Ret = _vswprintf_s_l(_DstBuf, _DstSize, _Format, ((void *)0), _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vsnwprintf_s_l(wchar_t *_DstBuf, size_t _DstSize, size_t _MaxCount, const wchar_t *_Format, _locale_t _Locale, va_list _ArgList)
{
return __stdio_common_vsnwprintf_s((*__local_stdio_printf_options()), _DstBuf, _DstSize, _MaxCount, _Format, _Locale, _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _vsnwprintf_s(wchar_t *_DstBuf, size_t _DstSize, size_t _MaxCount, const wchar_t *_Format, va_list _ArgList)
{
return _vsnwprintf_s_l(_DstBuf, _DstSize, _MaxCount, _Format, ((void *)0), _ArgList);
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _snwprintf_s_l(wchar_t *_DstBuf, size_t _DstSize, size_t _MaxCount, const wchar_t *_Format, _locale_t _Locale, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Locale);
_Ret = _vsnwprintf_s_l(_DstBuf, _DstSize, _MaxCount, _Format, _Locale, _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
static __attribute__ ((__unused__)) inline __cdecl int __cdecl _snwprintf_s(wchar_t *_DstBuf, size_t _DstSize, size_t _MaxCount, const wchar_t *_Format, ...)
{
__builtin_va_list _ArgList;
int _Ret;
__builtin_va_start(_ArgList, _Format);
_Ret = _vsnwprintf_s_l(_DstBuf, _DstSize, _MaxCount, _Format, ((void *)0), _ArgList);
__builtin_va_end(_ArgList);
return _Ret;
}
errno_t __cdecl _wfopen_s(FILE **_File,const wchar_t *_Filename,const wchar_t *_Mode);
errno_t __cdecl _wfreopen_s(FILE **_File,const wchar_t *_Filename,const wchar_t *_Mode,FILE *_OldFile);
errno_t __cdecl _wtmpnam_s(wchar_t *_DstBuf,size_t _SizeInWords);
size_t __cdecl _fread_nolock_s(void *_DstBuf,size_t _DstSize,size_t _ElementSize,size_t _Count,FILE *_File);
errno_t __cdecl _wdupenv_s(wchar_t **_Buffer,size_t *_BufferSizeInWords,const wchar_t *_VarName);
errno_t __cdecl _itow_s (int _Val,wchar_t *_DstBuf,size_t _SizeInWords,int _Radix);
errno_t __cdecl _ltow_s (long _Val,wchar_t *_DstBuf,size_t _SizeInWords,int _Radix);
errno_t __cdecl _ultow_s (unsigned long _Val,wchar_t *_DstBuf,size_t _SizeInWords,int _Radix);
errno_t __cdecl _wgetenv_s(size_t *_ReturnSize,wchar_t *_DstBuf,size_t _DstSizeInWords,const wchar_t *_VarName);
errno_t __cdecl _i64tow_s(long long _Val,wchar_t *_DstBuf,size_t _SizeInWords,int _Radix);
errno_t __cdecl _ui64tow_s(unsigned long long _Val,wchar_t *_DstBuf,size_t _SizeInWords,int _Radix);
errno_t __cdecl _wmakepath_s(wchar_t *_PathResult,size_t _SizeInWords,const wchar_t *_Drive,const wchar_t *_Dir,const wchar_t *_Filename,const wchar_t *_Ext);
errno_t __cdecl _wputenv_s(const wchar_t *_Name,const wchar_t *_Value);
errno_t __cdecl _wsearchenv_s(const wchar_t *_Filename,const wchar_t *_EnvVar,wchar_t *_ResultPath,size_t _SizeInWords);
errno_t __cdecl _wsplitpath_s(const wchar_t *_FullPath,wchar_t *_Drive,size_t _DriveSizeInWords,wchar_t *_Dir,size_t _DirSizeInWords,wchar_t *_Filename,size_t _FilenameSizeInWords,wchar_t *_Ext,size_t _ExtSizeInWords);
typedef int (__cdecl *_onexit_t)(void);
typedef struct _div_t {
int quot;
int rem;
} div_t;
typedef struct _ldiv_t {
long quot;
long rem;
} ldiv_t;
typedef struct {
unsigned char ld[10];
} _LDOUBLE;
typedef struct {
double x;
} _CRT_DOUBLE;
typedef struct {
float f;
} _CRT_FLOAT;
typedef struct {
long double x;
} _LONGDOUBLE;
typedef struct {
unsigned char ld12[12];
} _LDBL12;
typedef void (__cdecl *_purecall_handler)(void);
_purecall_handler __cdecl _set_purecall_handler(_purecall_handler _Handler);
_purecall_handler __cdecl _get_purecall_handler(void);
typedef void (__cdecl *_invalid_parameter_handler)(const wchar_t *,const wchar_t *,const wchar_t *,unsigned int,uintptr_t);
_invalid_parameter_handler __cdecl _set_invalid_parameter_handler(_invalid_parameter_handler _Handler);
_invalid_parameter_handler __cdecl _get_invalid_parameter_handler(void);
unsigned long *__cdecl __doserrno(void);
errno_t __cdecl _set_doserrno(unsigned long _Value);
errno_t __cdecl _get_doserrno(unsigned long *_Value);
char **__cdecl __sys_errlist(void);
int *__cdecl __sys_nerr(void);
char ***__cdecl __p___argv(void);
int *__cdecl __p__fmode(void);
int *__cdecl __p___argc(void);
wchar_t ***__cdecl __p___wargv(void);
char **__cdecl __p__pgmptr(void);
wchar_t **__cdecl __p__wpgmptr(void);
errno_t __cdecl _get_pgmptr(char **_Value);
errno_t __cdecl _get_wpgmptr(wchar_t **_Value);
errno_t __cdecl _set_fmode(int _Mode);
errno_t __cdecl _get_fmode(int *_PMode);
char ***__cdecl __p__environ(void);
wchar_t ***__cdecl __p__wenviron(void);
unsigned int *__cdecl __p__osplatform(void);
unsigned int *__cdecl __p__osver(void);
unsigned int *__cdecl __p__winver(void);
unsigned int *__cdecl __p__winmajor(void);
unsigned int *__cdecl __p__winminor(void);
errno_t __cdecl _get_osplatform(unsigned int *_Value);
errno_t __cdecl _get_osver(unsigned int *_Value);
errno_t __cdecl _get_winver(unsigned int *_Value);
errno_t __cdecl _get_winmajor(unsigned int *_Value);
errno_t __cdecl _get_winminor(unsigned int *_Value);
void __cdecl exit(int _Code) __attribute__ ((__noreturn__));
void __cdecl _exit(int _Code) __attribute__ ((__noreturn__));
void __cdecl quick_exit(int _Code) __attribute__ ((__noreturn__));
void __cdecl _Exit(int) __attribute__ ((__noreturn__));
extern inline __attribute__ ((__noreturn__)) void __cdecl _Exit(int status)
{ _exit(status); }
void __cdecl __attribute__ ((__noreturn__)) abort(void);
unsigned int __cdecl _set_abort_behavior(unsigned int _Flags,unsigned int _Mask);
int __cdecl abs(int _X);
long __cdecl labs(long _X);
__extension__ long long __cdecl _abs64(long long);
extern inline __attribute__((__always_inline__,__gnu_inline__)) long long __cdecl _abs64(long long x) {
return __builtin_llabs(x);
}
int __cdecl atexit(void (__cdecl *)(void));
int __cdecl at_quick_exit(void (__cdecl *)(void));
double __cdecl atof(const char *_String);
double __cdecl _atof_l(const char *_String,_locale_t _Locale);
int __cdecl atoi(const char *_Str);
int __cdecl _atoi_l(const char *_Str,_locale_t _Locale);
long __cdecl atol(const char *_Str);
long __cdecl _atol_l(const char *_Str,_locale_t _Locale);
void *__cdecl bsearch(const void *_Key,const void *_Base,size_t _NumOfElements,size_t _SizeOfElements,int (__cdecl *_PtFuncCompare)(const void *,const void *));
void __cdecl qsort(void *_Base,size_t _NumOfElements,size_t _SizeOfElements,int (__cdecl *_PtFuncCompare)(const void *,const void *));
unsigned short __cdecl _byteswap_ushort(unsigned short _Short);
unsigned long __cdecl _byteswap_ulong (unsigned long _Long);
__extension__ unsigned long long __cdecl _byteswap_uint64(unsigned long long _Int64);
div_t __cdecl div(int _Numerator,int _Denominator);
char *__cdecl getenv(const char *_VarName) ;
char *__cdecl _itoa(int _Value,char *_Dest,int _Radix);
__extension__ char *__cdecl _i64toa(long long _Val,char *_DstBuf,int _Radix) ;
__extension__ char *__cdecl _ui64toa(unsigned long long _Val,char *_DstBuf,int _Radix) ;
__extension__ long long __cdecl _atoi64(const char *_String);
__extension__ long long __cdecl _atoi64_l(const char *_String,_locale_t _Locale);
__extension__ long long __cdecl _strtoi64(const char *_String,char **_EndPtr,int _Radix);
__extension__ long long __cdecl _strtoi64_l(const char *_String,char **_EndPtr,int _Radix,_locale_t _Locale);
__extension__ unsigned long long __cdecl _strtoui64(const char *_String,char **_EndPtr,int _Radix);
__extension__ unsigned long long __cdecl _strtoui64_l(const char *_String,char **_EndPtr,int _Radix,_locale_t _Locale);
ldiv_t __cdecl ldiv(long _Numerator,long _Denominator);
char *__cdecl _ltoa(long _Value,char *_Dest,int _Radix) ;
int __cdecl mblen(const char *_Ch,size_t _MaxCount);
int __cdecl _mblen_l(const char *_Ch,size_t _MaxCount,_locale_t _Locale);
size_t __cdecl _mbstrlen(const char *_Str);
size_t __cdecl _mbstrlen_l(const char *_Str,_locale_t _Locale);
size_t __cdecl _mbstrnlen(const char *_Str,size_t _MaxCount);
size_t __cdecl _mbstrnlen_l(const char *_Str,size_t _MaxCount,_locale_t _Locale);
int __cdecl mbtowc(wchar_t * __restrict__ _DstCh,const char * __restrict__ _SrcCh,size_t _SrcSizeInBytes);
int __cdecl _mbtowc_l(wchar_t * __restrict__ _DstCh,const char * __restrict__ _SrcCh,size_t _SrcSizeInBytes,_locale_t _Locale);
size_t __cdecl mbstowcs(wchar_t * __restrict__ _Dest,const char * __restrict__ _Source,size_t _MaxCount);
size_t __cdecl _mbstowcs_l(wchar_t * __restrict__ _Dest,const char * __restrict__ _Source,size_t _MaxCount,_locale_t _Locale);
int __cdecl mkstemp(char *_TemplateName);
char *__cdecl mkdtemp(char *_TemplateName);
int __cdecl rand(void);
int __cdecl _set_error_mode(int _Mode);
void __cdecl srand(unsigned int _Seed);
static __attribute__ ((__unused__)) inline __cdecl
double __cdecl strtod(const char * __restrict__ _Str,char ** __restrict__ _EndPtr)
{
double __cdecl __mingw_strtod (const char * __restrict__, char ** __restrict__);
return __mingw_strtod( _Str, _EndPtr);
}
static __attribute__ ((__unused__)) inline __cdecl
float __cdecl strtof(const char * __restrict__ _Str,char ** __restrict__ _EndPtr)
{
float __cdecl __mingw_strtof (const char * __restrict__, char ** __restrict__);
return __mingw_strtof( _Str, _EndPtr);
}
long double __cdecl strtold(const char * __restrict__ , char ** __restrict__ );
extern double __cdecl __strtod (const char * __restrict__ , char ** __restrict__);
float __cdecl __mingw_strtof (const char * __restrict__, char ** __restrict__);
double __cdecl __mingw_strtod (const char * __restrict__, char ** __restrict__);
long double __cdecl __mingw_strtold(const char * __restrict__, char ** __restrict__);
float __cdecl _strtof_l(const char * __restrict__ _Str,char ** __restrict__ _EndPtr,_locale_t _Locale);
double __cdecl _strtod_l(const char * __restrict__ _Str,char ** __restrict__ _EndPtr,_locale_t _Locale);
long __cdecl strtol(const char * __restrict__ _Str,char ** __restrict__ _EndPtr,int _Radix);
long __cdecl _strtol_l(const char * __restrict__ _Str,char ** __restrict__ _EndPtr,int _Radix,_locale_t _Locale);
unsigned long __cdecl strtoul(const char * __restrict__ _Str,char ** __restrict__ _EndPtr,int _Radix);
unsigned long __cdecl _strtoul_l(const char * __restrict__ _Str,char ** __restrict__ _EndPtr,int _Radix,_locale_t _Locale);
int __cdecl system(const char *_Command);
char *__cdecl _ultoa(unsigned long _Value,char *_Dest,int _Radix) ;
int __cdecl wctomb(char *_MbCh,wchar_t _WCh) ;
int __cdecl _wctomb_l(char *_MbCh,wchar_t _WCh,_locale_t _Locale) ;
size_t __cdecl wcstombs(char * __restrict__ _Dest,const wchar_t * __restrict__ _Source,size_t _MaxCount) ;
size_t __cdecl _wcstombs_l(char * __restrict__ _Dest,const wchar_t * __restrict__ _Source,size_t _MaxCount,_locale_t _Locale) ;
void *__cdecl calloc(size_t _NumOfElements,size_t _SizeOfElements);
void __cdecl free(void *_Memory);
void *__cdecl malloc(size_t _Size);
void *__cdecl realloc(void *_Memory,size_t _NewSize);
void __cdecl _aligned_free(void *_Memory);
void *__cdecl _aligned_malloc(size_t _Size,size_t _Alignment);
void *__cdecl _aligned_offset_malloc(size_t _Size,size_t _Alignment,size_t _Offset);
void *__cdecl _aligned_realloc(void *_Memory,size_t _Size,size_t _Alignment);
void *__cdecl _aligned_offset_realloc(void *_Memory,size_t _Size,size_t _Alignment,size_t _Offset);
void *__cdecl _recalloc(void *_Memory,size_t _Count,size_t _Size);
void *__cdecl _aligned_recalloc(void *_Memory,size_t _Count,size_t _Size,size_t _Alignment);
void *__cdecl _aligned_offset_recalloc(void *_Memory,size_t _Count,size_t _Size,size_t _Alignment,size_t _Offset);
size_t __cdecl _aligned_msize(void *_Memory,size_t _Alignment,size_t _Offset);
wchar_t *__cdecl _itow(int _Value,wchar_t *_Dest,int _Radix) ;
wchar_t *__cdecl _ltow(long _Value,wchar_t *_Dest,int _Radix) ;
wchar_t *__cdecl _ultow(unsigned long _Value,wchar_t *_Dest,int _Radix) ;
double __cdecl __mingw_wcstod(const wchar_t * __restrict__ _Str,wchar_t ** __restrict__ _EndPtr);
float __cdecl __mingw_wcstof(const wchar_t * __restrict__ nptr, wchar_t ** __restrict__ endptr);
long double __cdecl __mingw_wcstold(const wchar_t * __restrict__, wchar_t ** __restrict__);
double __cdecl wcstod(const wchar_t * __restrict__ _Str,wchar_t ** __restrict__ _EndPtr);
float __cdecl wcstof(const wchar_t * __restrict__ nptr, wchar_t ** __restrict__ endptr);
long double __cdecl wcstold(const wchar_t * __restrict__, wchar_t ** __restrict__);
double __cdecl _wcstod_l(const wchar_t * __restrict__ _Str,wchar_t ** __restrict__ _EndPtr,_locale_t _Locale);
float __cdecl _wcstof_l(const wchar_t * __restrict__ _Str,wchar_t ** __restrict__ _EndPtr,_locale_t _Locale);
long __cdecl wcstol(const wchar_t * __restrict__ _Str,wchar_t ** __restrict__ _EndPtr,int _Radix);
long __cdecl _wcstol_l(const wchar_t * __restrict__ _Str,wchar_t ** __restrict__ _EndPtr,int _Radix,_locale_t _Locale);
unsigned long __cdecl wcstoul(const wchar_t * __restrict__ _Str,wchar_t ** __restrict__ _EndPtr,int _Radix);
unsigned long __cdecl _wcstoul_l(const wchar_t * __restrict__ _Str,wchar_t ** __restrict__ _EndPtr,int _Radix,_locale_t _Locale);
wchar_t *__cdecl _wgetenv(const wchar_t *_VarName) ;
int __cdecl _wsystem(const wchar_t *_Command);
double __cdecl _wtof(const wchar_t *_Str);
double __cdecl _wtof_l(const wchar_t *_Str,_locale_t _Locale);
int __cdecl _wtoi(const wchar_t *_Str);
int __cdecl _wtoi_l(const wchar_t *_Str,_locale_t _Locale);
long __cdecl _wtol(const wchar_t *_Str);
long __cdecl _wtol_l(const wchar_t *_Str,_locale_t _Locale);
__extension__ wchar_t *__cdecl _i64tow(long long _Val,wchar_t *_DstBuf,int _Radix) ;
__extension__ wchar_t *__cdecl _ui64tow(unsigned long long _Val,wchar_t *_DstBuf,int _Radix) ;
__extension__ long long __cdecl _wtoi64(const wchar_t *_Str);
__extension__ long long __cdecl _wtoi64_l(const wchar_t *_Str,_locale_t _Locale);
__extension__ long long __cdecl _wcstoi64(const wchar_t *_Str,wchar_t **_EndPtr,int _Radix);
__extension__ long long __cdecl _wcstoi64_l(const wchar_t *_Str,wchar_t **_EndPtr,int _Radix,_locale_t _Locale);
__extension__ unsigned long long __cdecl _wcstoui64(const wchar_t *_Str,wchar_t **_EndPtr,int _Radix);
__extension__ unsigned long long __cdecl _wcstoui64_l(const wchar_t *_Str ,wchar_t **_EndPtr,int _Radix,_locale_t _Locale);
int __cdecl _putenv(const char *_EnvString);
int __cdecl _wputenv(const wchar_t *_EnvString);
char *__cdecl _fullpath(char *_FullPath,const char *_Path,size_t _SizeInBytes);
char *__cdecl _ecvt(double _Val,int _NumOfDigits,int *_PtDec,int *_PtSign) ;
char *__cdecl _fcvt(double _Val,int _NumOfDec,int *_PtDec,int *_PtSign) ;
char *__cdecl _gcvt(double _Val,int _NumOfDigits,char *_DstBuf) ;
int __cdecl _atodbl(_CRT_DOUBLE *_Result,char *_Str);
int __cdecl _atoldbl(_LDOUBLE *_Result,char *_Str);
int __cdecl _atoflt(_CRT_FLOAT *_Result,char *_Str);
int __cdecl _atodbl_l(_CRT_DOUBLE *_Result,char *_Str,_locale_t _Locale);
int __cdecl _atoldbl_l(_LDOUBLE *_Result,char *_Str,_locale_t _Locale);
int __cdecl _atoflt_l(_CRT_FLOAT *_Result,char *_Str,_locale_t _Locale);
unsigned long __cdecl _lrotl(unsigned long,int);
unsigned long __cdecl _lrotr(unsigned long,int);
void __cdecl _makepath(char *_Path,const char *_Drive,const char *_Dir,const char *_Filename,const char *_Ext);
_onexit_t __cdecl _onexit(_onexit_t _Func);
__extension__ unsigned long long __cdecl _rotl64(unsigned long long _Val,int _Shift);
__extension__ unsigned long long __cdecl _rotr64(unsigned long long Value,int Shift);
unsigned int __cdecl _rotr(unsigned int _Val,int _Shift);
unsigned int __cdecl _rotl(unsigned int _Val,int _Shift);
__extension__ unsigned long long __cdecl _rotr64(unsigned long long _Val,int _Shift);
void __cdecl _searchenv(const char *_Filename,const char *_EnvVar,char *_ResultPath) ;
void __cdecl _splitpath(const char *_FullPath,char *_Drive,char *_Dir,char *_Filename,char *_Ext) ;
void __cdecl _swab(char *_Buf1,char *_Buf2,int _SizeInBytes);
wchar_t *__cdecl _wfullpath(wchar_t *_FullPath,const wchar_t *_Path,size_t _SizeInWords);
void __cdecl _wmakepath(wchar_t *_ResultPath,const wchar_t *_Drive,const wchar_t *_Dir,const wchar_t *_Filename,const wchar_t *_Ext);
void __cdecl _wsearchenv(const wchar_t *_Filename,const wchar_t *_EnvVar,wchar_t *_ResultPath) ;
void __cdecl _wsplitpath(const wchar_t *_FullPath,wchar_t *_Drive,wchar_t *_Dir,wchar_t *_Filename,wchar_t *_Ext) ;
void __cdecl _beep(unsigned _Frequency,unsigned _Duration) ;
void __cdecl _seterrormode(int _Mode) ;
void __cdecl _sleep(unsigned long _Duration) ;
char *__cdecl ecvt(double _Val,int _NumOfDigits,int *_PtDec,int *_PtSign) ;
char *__cdecl fcvt(double _Val,int _NumOfDec,int *_PtDec,int *_PtSign) ;
char *__cdecl gcvt(double _Val,int _NumOfDigits,char *_DstBuf) ;
char *__cdecl itoa(int _Val,char *_DstBuf,int _Radix) ;
char *__cdecl ltoa(long _Val,char *_DstBuf,int _Radix) ;
int __cdecl putenv(const char *_EnvString) ;
void __cdecl swab(char *_Buf1,char *_Buf2,int _SizeInBytes) ;
char *__cdecl ultoa(unsigned long _Val,char *_Dstbuf,int _Radix) ;
_onexit_t __cdecl onexit(_onexit_t _Func);
typedef struct { __extension__ long long quot, rem; } lldiv_t;
__extension__ lldiv_t __cdecl lldiv(long long, long long);
__extension__ long long __cdecl llabs(long long);
__extension__ extern inline long long __cdecl llabs(long long _j) { return (_j >= 0 ? _j : -_j); }
__extension__ long long __cdecl strtoll(const char * __restrict__, char ** __restrict, int);
__extension__ unsigned long long __cdecl strtoull(const char * __restrict__, char ** __restrict__, int);
__extension__ long long __cdecl atoll (const char *);
__extension__ long long __cdecl wtoll (const wchar_t *);
__extension__ char *__cdecl lltoa (long long, char *, int);
__extension__ char *__cdecl ulltoa (unsigned long long , char *, int);
__extension__ wchar_t *__cdecl lltow (long long, wchar_t *, int);
__extension__ wchar_t *__cdecl ulltow (unsigned long long, wchar_t *, int);
__extension__ extern inline char *__cdecl lltoa (long long _n, char * _c, int _i) { return _i64toa (_n, _c, _i); }
__extension__ extern inline char *__cdecl ulltoa (unsigned long long _n, char * _c, int _i) { return _ui64toa (_n, _c, _i); }
__extension__ extern inline long long __cdecl wtoll (const wchar_t * _w) { return _wtoi64 (_w); }
__extension__ extern inline wchar_t *__cdecl lltow (long long _n, wchar_t * _w, int _i) { return _i64tow (_n, _w, _i); }
__extension__ extern inline wchar_t *__cdecl ulltow (unsigned long long _n, wchar_t * _w, int _i) { return _ui64tow (_n, _w, _i); }
errno_t __cdecl _dupenv_s(char **_PBuffer,size_t *_PBufferSizeInBytes,const char *_VarName);
void * __cdecl bsearch_s(const void *_Key,const void *_Base,rsize_t _NumOfElements,rsize_t _SizeOfElements,int (__cdecl * _PtFuncCompare)(void *, const void *, const void *), void *_Context);
errno_t __cdecl getenv_s(size_t *_ReturnSize,char *_DstBuf,rsize_t _DstSize,const char *_VarName);
errno_t __cdecl _itoa_s(int _Value,char *_DstBuf,size_t _Size,int _Radix);
errno_t __cdecl _i64toa_s(long long _Val,char *_DstBuf,size_t _Size,int _Radix);
errno_t __cdecl _ui64toa_s(unsigned long long _Val,char *_DstBuf,size_t _Size,int _Radix);
errno_t __cdecl _ltoa_s(long _Val,char *_DstBuf,size_t _Size,int _Radix);
errno_t __cdecl mbstowcs_s(size_t *_PtNumOfCharConverted,wchar_t *_DstBuf,size_t _SizeInWords,const char *_SrcBuf,size_t _MaxCount);
errno_t __cdecl _mbstowcs_s_l(size_t *_PtNumOfCharConverted,wchar_t *_DstBuf,size_t _SizeInWords,const char *_SrcBuf,size_t _MaxCount,_locale_t _Locale);
errno_t __cdecl _ultoa_s(unsigned long _Val,char *_DstBuf,size_t _Size,int _Radix);
errno_t __cdecl wctomb_s(int *_SizeConverted,char *_MbCh,rsize_t _SizeInBytes,wchar_t _WCh);
errno_t __cdecl _wctomb_s_l(int *_SizeConverted,char *_MbCh,size_t _SizeInBytes,wchar_t _WCh,_locale_t _Locale);
errno_t __cdecl wcstombs_s(size_t *_PtNumOfCharConverted,char *_Dst,size_t _DstSizeInBytes,const wchar_t *_Src,size_t _MaxCountInBytes);
errno_t __cdecl _wcstombs_s_l(size_t *_PtNumOfCharConverted,char *_Dst,size_t _DstSizeInBytes,const wchar_t *_Src,size_t _MaxCountInBytes,_locale_t _Locale);
errno_t __cdecl _ecvt_s(char *_DstBuf,size_t _Size,double _Val,int _NumOfDights,int *_PtDec,int *_PtSign);
errno_t __cdecl _fcvt_s(char *_DstBuf,size_t _Size,double _Val,int _NumOfDec,int *_PtDec,int *_PtSign);
errno_t __cdecl _gcvt_s(char *_DstBuf,size_t _Size,double _Val,int _NumOfDigits);
errno_t __cdecl _makepath_s(char *_PathResult,size_t _Size,const char *_Drive,const char *_Dir,const char *_Filename,const char *_Ext);
errno_t __cdecl _putenv_s(const char *_Name,const char *_Value);
errno_t __cdecl _searchenv_s(const char *_Filename,const char *_EnvVar,char *_ResultPath,size_t _SizeInBytes);
errno_t __cdecl _splitpath_s(const char *_FullPath,char *_Drive,size_t _DriveSize,char *_Dir,size_t _DirSize,char *_Filename,size_t _FilenameSize,char *_Ext,size_t _ExtSize);
void __cdecl qsort_s(void *_Base,size_t _NumOfElements,size_t _SizeOfElements,int (__cdecl *_PtFuncCompare)(void *,const void *,const void *),void *_Context);
typedef struct _heapinfo {
int *_pentry;
size_t _size;
int _useflag;
} _HEAPINFO;
unsigned int *__cdecl __p__amblksiz(void);
void * __mingw_aligned_malloc (size_t _Size, size_t _Alignment);
void __mingw_aligned_free (void *_Memory);
void * __mingw_aligned_offset_realloc (void *_Memory, size_t _Size, size_t _Alignment, size_t _Offset);
void * __mingw_aligned_offset_malloc (size_t, size_t, size_t);
void * __mingw_aligned_realloc (void *_Memory, size_t _Size, size_t _Offset);
size_t __mingw_aligned_msize (void *memblock, size_t alignment, size_t offset);
static inline void *
_mm_malloc (size_t __size, size_t __align)
{
void * __malloc_ptr;
void * __aligned_ptr;
if (__align & (__align - 1))
{
(*_errno()) = 22;
return ((void *) 0);
}
if (__size == 0)
return ((void *) 0);
if (__align < 2 * sizeof (void *))
__align = 2 * sizeof (void *);
__malloc_ptr = malloc (__size + __align);
if (!__malloc_ptr)
return ((void *) 0);
__aligned_ptr = (void *) (((size_t) __malloc_ptr + __align)
& ~((size_t) (__align) - 1));
((void **) __aligned_ptr)[-1] = __malloc_ptr;
return __aligned_ptr;
}
static inline void
_mm_free (void *__aligned_ptr)
{
if (__aligned_ptr)
free (((void **) __aligned_ptr)[-1]);
}
int __cdecl _resetstkoflw (void);
unsigned long __cdecl _set_malloc_crt_max_wait(unsigned long _NewValue);
void *__cdecl _expand(void *_Memory,size_t _NewSize);
size_t __cdecl _msize(void *_Memory);
size_t __cdecl _get_sbh_threshold(void);
int __cdecl _set_sbh_threshold(size_t _NewValue);
errno_t __cdecl _set_amblksiz(size_t _Value);
errno_t __cdecl _get_amblksiz(size_t *_Value);
int __cdecl _heapadd(void *_Memory,size_t _Size);
int __cdecl _heapchk(void);
int __cdecl _heapmin(void);
int __cdecl _heapset(unsigned int _Fill);
int __cdecl _heapwalk(_HEAPINFO *_EntryInfo);
size_t __cdecl _heapused(size_t *_Used,size_t *_Commit);
intptr_t __cdecl _get_heap_handle(void);
static __inline void *_MarkAllocaS(void *_Ptr,unsigned int _Marker) {
if(_Ptr) {
*((unsigned int*)_Ptr) = _Marker;
_Ptr = (char*)_Ptr + 16;
}
return _Ptr;
}
static __inline void __cdecl _freea(void *_Memory) {
unsigned int _Marker;
if(_Memory) {
_Memory = (char*)_Memory - 16;
_Marker = *(unsigned int *)_Memory;
if(_Marker==0xDDDD) {
free(_Memory);
}
}
}
void *__cdecl _memccpy(void *_Dst,const void *_Src,int _Val,size_t _MaxCount);
void *__cdecl memchr(const void *_Buf ,int _Val,size_t _MaxCount);
int __cdecl _memicmp(const void *_Buf1,const void *_Buf2,size_t _Size);
int __cdecl _memicmp_l(const void *_Buf1,const void *_Buf2,size_t _Size,_locale_t _Locale);
int __cdecl memcmp(const void *_Buf1,const void *_Buf2,size_t _Size);
void * __cdecl memcpy(void * __restrict__ _Dst,const void * __restrict__ _Src,size_t _Size) ;
__declspec(dllimport) errno_t __cdecl memcpy_s (void *_dest,size_t _numberOfElements,const void *_src,size_t _count);
void * __cdecl mempcpy (void *_Dst, const void *_Src, size_t _Size);
void * __cdecl memset(void *_Dst,int _Val,size_t _Size);
void * __cdecl memccpy(void *_Dst,const void *_Src,int _Val,size_t _Size) ;
int __cdecl memicmp(const void *_Buf1,const void *_Buf2,size_t _Size) ;
char * __cdecl _strset(char *_Str,int _Val) ;
char * __cdecl _strset_l(char *_Str,int _Val,_locale_t _Locale) ;
char * __cdecl strcpy(char * __restrict__ _Dest,const char * __restrict__ _Source);
char * __cdecl strcat(char * __restrict__ _Dest,const char * __restrict__ _Source);
int __cdecl strcmp(const char *_Str1,const char *_Str2);
size_t __cdecl strlen(const char *_Str);
size_t __cdecl strnlen(const char *_Str,size_t _MaxCount);
void *__cdecl memmove(void *_Dst,const void *_Src,size_t _Size) ;
char *__cdecl _strdup(const char *_Src);
char *__cdecl strchr(const char *_Str,int _Val);
int __cdecl _stricmp(const char *_Str1,const char *_Str2);
int __cdecl _strcmpi(const char *_Str1,const char *_Str2);
int __cdecl _stricmp_l(const char *_Str1,const char *_Str2,_locale_t _Locale);
int __cdecl strcoll(const char *_Str1,const char *_Str2);
int __cdecl _strcoll_l(const char *_Str1,const char *_Str2,_locale_t _Locale);
int __cdecl _stricoll(const char *_Str1,const char *_Str2);
int __cdecl _stricoll_l(const char *_Str1,const char *_Str2,_locale_t _Locale);
int __cdecl _strncoll (const char *_Str1,const char *_Str2,size_t _MaxCount);
int __cdecl _strncoll_l(const char *_Str1,const char *_Str2,size_t _MaxCount,_locale_t _Locale);
int __cdecl _strnicoll (const char *_Str1,const char *_Str2,size_t _MaxCount);
int __cdecl _strnicoll_l(const char *_Str1,const char *_Str2,size_t _MaxCount,_locale_t _Locale);
size_t __cdecl strcspn(const char *_Str,const char *_Control);
char *__cdecl _strerror(const char *_ErrMsg) ;
char *__cdecl strerror(int) ;
char *__cdecl _strlwr(char *_String) ;
char *strlwr_l(char *_String,_locale_t _Locale) ;
char *__cdecl strncat(char * __restrict__ _Dest,const char * __restrict__ _Source,size_t _Count) ;
int __cdecl strncmp(const char *_Str1,const char *_Str2,size_t _MaxCount);
int __cdecl _strnicmp(const char *_Str1,const char *_Str2,size_t _MaxCount);
int __cdecl _strnicmp_l(const char *_Str1,const char *_Str2,size_t _MaxCount,_locale_t _Locale);
char *strncpy(char * __restrict__ _Dest,const char * __restrict__ _Source,size_t _Count) ;
char *__cdecl _strnset(char *_Str,int _Val,size_t _MaxCount) ;
char *__cdecl _strnset_l(char *str,int c,size_t count,_locale_t _Locale) ;
char *__cdecl strpbrk(const char *_Str,const char *_Control);
char *__cdecl strrchr(const char *_Str,int _Ch);
char *__cdecl _strrev(char *_Str);
size_t __cdecl strspn(const char *_Str,const char *_Control);
char *__cdecl strstr(const char *_Str,const char *_SubStr);
char *__cdecl strtok(char * __restrict__ _Str,const char * __restrict__ _Delim) ;
char *strtok_r(char * __restrict__ _Str, const char * __restrict__ _Delim, char ** __restrict__ __last);
char *__cdecl _strupr(char *_String) ;
char *_strupr_l(char *_String,_locale_t _Locale) ;
size_t __cdecl strxfrm(char * __restrict__ _Dst,const char * __restrict__ _Src,size_t _MaxCount);
size_t __cdecl _strxfrm_l(char * __restrict__ _Dst,const char * __restrict__ _Src,size_t _MaxCount,_locale_t _Locale);
char *__cdecl strdup(const char *_Src) ;
int __cdecl strcmpi(const char *_Str1,const char *_Str2) ;
int __cdecl stricmp(const char *_Str1,const char *_Str2) ;
char *__cdecl strlwr(char *_Str) ;
int __cdecl strnicmp(const char *_Str1,const char *_Str,size_t _MaxCount) ;
int __cdecl strncasecmp (const char *, const char *, size_t);
int __cdecl strcasecmp (const char *, const char *);
char *__cdecl strnset(char *_Str,int _Val,size_t _MaxCount) ;
char *__cdecl strrev(char *_Str) ;
char *__cdecl strset(char *_Str,int _Val) ;
char *__cdecl strupr(char *_Str) ;
wchar_t *__cdecl _wcsdup(const wchar_t *_Str);
wchar_t *__cdecl wcscat(wchar_t * __restrict__ _Dest,const wchar_t * __restrict__ _Source) ;
wchar_t *__cdecl wcschr(const wchar_t *_Str,wchar_t _Ch);
int __cdecl wcscmp(const wchar_t *_Str1,const wchar_t *_Str2);
wchar_t *__cdecl wcscpy(wchar_t * __restrict__ _Dest,const wchar_t * __restrict__ _Source) ;
size_t __cdecl wcscspn(const wchar_t *_Str,const wchar_t *_Control);
size_t __cdecl wcslen(const wchar_t *_Str);
size_t __cdecl wcsnlen(const wchar_t *_Src,size_t _MaxCount);
wchar_t *wcsncat(wchar_t * __restrict__ _Dest,const wchar_t * __restrict__ _Source,size_t _Count) ;
int __cdecl wcsncmp(const wchar_t *_Str1,const wchar_t *_Str2,size_t _MaxCount);
wchar_t *wcsncpy(wchar_t * __restrict__ _Dest,const wchar_t * __restrict__ _Source,size_t _Count) ;
wchar_t *__cdecl _wcsncpy_l(wchar_t * __restrict__ _Dest,const wchar_t * __restrict__ _Source,size_t _Count,_locale_t _Locale) ;
wchar_t *__cdecl wcspbrk(const wchar_t *_Str,const wchar_t *_Control);
wchar_t *__cdecl wcsrchr(const wchar_t *_Str,wchar_t _Ch);
size_t __cdecl wcsspn(const wchar_t *_Str,const wchar_t *_Control);
wchar_t *__cdecl wcsstr(const wchar_t *_Str,const wchar_t *_SubStr);
wchar_t *__cdecl wcstok(wchar_t * __restrict__ _Str,const wchar_t * __restrict__ _Delim,wchar_t ** __restrict__ _Ptr) ;
wchar_t *__cdecl _wcstok(wchar_t * __restrict__ _Str,const wchar_t * __restrict__ _Delim) ;
wchar_t *__cdecl _wcserror(int _ErrNum) ;
wchar_t *__cdecl __wcserror(const wchar_t *_Str) ;
int __cdecl _wcsicmp(const wchar_t *_Str1,const wchar_t *_Str2);
int __cdecl _wcsicmp_l(const wchar_t *_Str1,const wchar_t *_Str2,_locale_t _Locale);
int __cdecl _wcsnicmp(const wchar_t *_Str1,const wchar_t *_Str2,size_t _MaxCount);
int __cdecl _wcsnicmp_l(const wchar_t *_Str1,const wchar_t *_Str2,size_t _MaxCount,_locale_t _Locale);
wchar_t *__cdecl _wcsnset(wchar_t *_Str,wchar_t _Val,size_t _MaxCount) ;
wchar_t *__cdecl _wcsrev(wchar_t *_Str);
wchar_t *__cdecl _wcsset(wchar_t *_Str,wchar_t _Val) ;
wchar_t *__cdecl _wcslwr(wchar_t *_String) ;
wchar_t *_wcslwr_l(wchar_t *_String,_locale_t _Locale) ;
wchar_t *__cdecl _wcsupr(wchar_t *_String) ;
wchar_t *_wcsupr_l(wchar_t *_String,_locale_t _Locale) ;
size_t __cdecl wcsxfrm(wchar_t * __restrict__ _Dst,const wchar_t * __restrict__ _Src,size_t _MaxCount);
size_t __cdecl _wcsxfrm_l(wchar_t * __restrict__ _Dst,const wchar_t * __restrict__ _Src,size_t _MaxCount,_locale_t _Locale);
int __cdecl wcscoll(const wchar_t *_Str1,const wchar_t *_Str2);
int __cdecl _wcscoll_l(const wchar_t *_Str1,const wchar_t *_Str2,_locale_t _Locale);
int __cdecl _wcsicoll(const wchar_t *_Str1,const wchar_t *_Str2);
int __cdecl _wcsicoll_l(const wchar_t *_Str1,const wchar_t *_Str2,_locale_t _Locale);
int __cdecl _wcsncoll(const wchar_t *_Str1,const wchar_t *_Str2,size_t _MaxCount);
int __cdecl _wcsncoll_l(const wchar_t *_Str1,const wchar_t *_Str2,size_t _MaxCount,_locale_t _Locale);
int __cdecl _wcsnicoll(const wchar_t *_Str1,const wchar_t *_Str2,size_t _MaxCount);
int __cdecl _wcsnicoll_l(const wchar_t *_Str1,const wchar_t *_Str2,size_t _MaxCount,_locale_t _Locale);
wchar_t *__cdecl wcsdup(const wchar_t *_Str) ;
int __cdecl wcsicmp(const wchar_t *_Str1,const wchar_t *_Str2) ;
int __cdecl wcsnicmp(const wchar_t *_Str1,const wchar_t *_Str2,size_t _MaxCount) ;
wchar_t *__cdecl wcsnset(wchar_t *_Str,wchar_t _Val,size_t _MaxCount) ;
wchar_t *__cdecl wcsrev(wchar_t *_Str) ;
wchar_t *__cdecl wcsset(wchar_t *_Str,wchar_t _Val) ;
wchar_t *__cdecl wcslwr(wchar_t *_Str) ;
wchar_t *__cdecl wcsupr(wchar_t *_Str) ;
int __cdecl wcsicoll(const wchar_t *_Str1,const wchar_t *_Str2) ;
errno_t __cdecl _strset_s(char *_Dst,size_t _DstSize,int _Value);
errno_t __cdecl _strerror_s(char *_Buf,size_t _SizeInBytes,const char *_ErrMsg);
__declspec(dllimport) errno_t __cdecl strerror_s(char *_Buf,size_t _SizeInBytes,int _ErrNum);
errno_t __cdecl _strlwr_s(char *_Str,size_t _Size);
errno_t __cdecl _strlwr_s_l(char *_Str,size_t _Size,_locale_t _Locale);
errno_t __cdecl _strnset_s(char *_Str,size_t _Size,int _Val,size_t _MaxCount);
errno_t __cdecl _strupr_s(char *_Str,size_t _Size);
errno_t __cdecl _strupr_s_l(char *_Str,size_t _Size,_locale_t _Locale);
errno_t __cdecl strncat_s(char *_Dst,size_t _DstSizeInChars,const char *_Src,size_t _MaxCount);
errno_t __cdecl _strncat_s_l(char *_Dst,size_t _DstSizeInChars,const char *_Src,size_t _MaxCount,_locale_t _Locale);
errno_t __cdecl strcpy_s(char *_Dst, rsize_t _SizeInBytes, const char *_Src);
errno_t __cdecl strncpy_s(char *_Dst, size_t _DstSizeInChars, const char *_Src, size_t _MaxCount);
errno_t __cdecl _strncpy_s_l(char *_Dst, size_t _DstSizeInChars, const char *_Src, size_t _MaxCount, _locale_t _Locale);
char *__cdecl strtok_s(char *_Str,const char *_Delim,char **_Context);
char *__cdecl _strtok_s_l(char *_Str,const char *_Delim,char **_Context,_locale_t _Locale);
errno_t __cdecl strcat_s(char *_Dst, rsize_t _SizeInBytes, const char * _Src);
extern inline __attribute__((__always_inline__,__gnu_inline__)) size_t __cdecl strnlen_s(const char * _src, size_t _count) {
return _src ? strnlen(_src, _count) : 0;
}
__declspec(dllimport) errno_t __cdecl memmove_s(void *_dest,size_t _numberOfElements,const void *_src,size_t _count);
wchar_t *__cdecl wcstok_s(wchar_t *_Str,const wchar_t *_Delim,wchar_t **_Context);
errno_t __cdecl _wcserror_s(wchar_t *_Buf,size_t _SizeInWords,int _ErrNum);
errno_t __cdecl __wcserror_s(wchar_t *_Buffer,size_t _SizeInWords,const wchar_t *_ErrMsg);
errno_t __cdecl _wcsnset_s(wchar_t *_Dst,size_t _DstSizeInWords,wchar_t _Val,size_t _MaxCount);
errno_t __cdecl _wcsset_s(wchar_t *_Str,size_t _SizeInWords,wchar_t _Val);
errno_t __cdecl _wcslwr_s(wchar_t *_Str,size_t _SizeInWords);
errno_t __cdecl _wcslwr_s_l(wchar_t *_Str,size_t _SizeInWords,_locale_t _Locale);
errno_t __cdecl _wcsupr_s(wchar_t *_Str,size_t _Size);
errno_t __cdecl _wcsupr_s_l(wchar_t *_Str,size_t _Size,_locale_t _Locale);
errno_t __cdecl wcscpy_s(wchar_t *_Dst, rsize_t _SizeInWords, const wchar_t *_Src);
errno_t __cdecl wcscat_s(wchar_t * _Dst, rsize_t _SizeInWords, const wchar_t *_Src);
errno_t __cdecl wcsncat_s(wchar_t *_Dst,size_t _DstSizeInChars,const wchar_t *_Src,size_t _MaxCount);
errno_t __cdecl _wcsncat_s_l(wchar_t *_Dst,size_t _DstSizeInChars,const wchar_t *_Src,size_t _MaxCount,_locale_t _Locale);
errno_t __cdecl wcsncpy_s(wchar_t *_Dst, size_t _DstSizeInChars, const wchar_t *_Src, size_t _MaxCount);
errno_t __cdecl _wcsncpy_s_l(wchar_t *_Dst, size_t _DstSizeInChars, const wchar_t *_Src, size_t _MaxCount, _locale_t _Locale);
wchar_t *__cdecl _wcstok_s_l(wchar_t *_Str,const wchar_t *_Delim,wchar_t **_Context,_locale_t _Locale);
errno_t __cdecl _wcsset_s_l(wchar_t *_Str,size_t _SizeInChars,wchar_t _Val,_locale_t _Locale);
errno_t __cdecl _wcsnset_s_l(wchar_t *_Str,size_t _SizeInChars,wchar_t _Val, size_t _Count,_locale_t _Locale);
extern inline __attribute__((__always_inline__,__gnu_inline__)) size_t __cdecl wcsnlen_s(const wchar_t * _src, size_t _count) {
return _src ? wcsnlen(_src, _count) : 0;
}
char *format(char *fmt, ...);
char* __cdecl _getcwd (char*, int);
typedef unsigned long _fsize_t;
struct _finddata32_t {
unsigned attrib;
__time32_t time_create;
__time32_t time_access;
__time32_t time_write;
_fsize_t size;
char name[260];
};
struct _finddata32i64_t {
unsigned attrib;
__time32_t time_create;
__time32_t time_access;
__time32_t time_write;
__extension__ long long size;
char name[260];
};
struct _finddata64i32_t {
unsigned attrib;
__time64_t time_create;
__time64_t time_access;
__time64_t time_write;
_fsize_t size;
char name[260];
};
struct __finddata64_t {
unsigned attrib;
__time64_t time_create;
__time64_t time_access;
__time64_t time_write;
__extension__ long long size;
char name[260];
};
struct _wfinddata32_t {
unsigned attrib;
__time32_t time_create;
__time32_t time_access;
__time32_t time_write;
_fsize_t size;
wchar_t name[260];
};
struct _wfinddata32i64_t {
unsigned attrib;
__time32_t time_create;
__time32_t time_access;
__time32_t time_write;
__extension__ long long size;
wchar_t name[260];
};
struct _wfinddata64i32_t {
unsigned attrib;
__time64_t time_create;
__time64_t time_access;
__time64_t time_write;
_fsize_t size;
wchar_t name[260];
};
struct _wfinddata64_t {
unsigned attrib;
__time64_t time_create;
__time64_t time_access;
__time64_t time_write;
__extension__ long long size;
wchar_t name[260];
};
int __cdecl _access(const char *_Filename,int _AccessMode);
__declspec(dllimport) errno_t __cdecl _access_s(const char *_Filename,int _AccessMode);
int __cdecl _chmod(const char *_Filename,int _Mode);
int __cdecl _chsize(int _FileHandle,long _Size) ;
__declspec(dllimport) errno_t __cdecl _chsize_s (int _FileHandle,long long _Size);
int __cdecl _close(int _FileHandle);
int __cdecl _commit(int _FileHandle);
int __cdecl _creat(const char *_Filename,int _PermissionMode) ;
int __cdecl _dup(int _FileHandle);
int __cdecl _dup2(int _FileHandleSrc,int _FileHandleDst);
int __cdecl _eof(int _FileHandle);
long __cdecl _filelength(int _FileHandle);
intptr_t __cdecl _findfirst32(const char *_Filename,struct _finddata32_t *_FindData);
int __cdecl _findnext32(intptr_t _FindHandle,struct _finddata32_t *_FindData);
int __cdecl _findclose(intptr_t _FindHandle);
int __cdecl _isatty(int _FileHandle);
int __cdecl _locking(int _FileHandle,int _LockMode,long _NumOfBytes);
long __cdecl _lseek(int _FileHandle,long _Offset,int _Origin);
_off64_t lseek64(int fd,_off64_t offset, int whence);
char *__cdecl _mktemp(char *_TemplateName) ;
__declspec(dllimport) errno_t __cdecl _mktemp_s (char *_TemplateName,size_t _Size);
int __cdecl _pipe(int *_PtHandles,unsigned int _PipeSize,int _TextMode);
int __cdecl _read(int _FileHandle,void *_DstBuf,unsigned int _MaxCharCount);
int __cdecl _setmode(int _FileHandle,int _Mode);
long __cdecl _tell(int _FileHandle);
int __cdecl _umask(int _Mode) ;
__declspec(dllimport) errno_t __cdecl _umask_s (int _NewMode,int *_OldMode);
int __cdecl _write(int _FileHandle,const void *_Buf,unsigned int _MaxCharCount);
__extension__ long long __cdecl _filelengthi64(int _FileHandle);
intptr_t __cdecl _findfirst32i64(const char *_Filename,struct _finddata32i64_t *_FindData);
intptr_t __cdecl _findfirst64(const char *_Filename,struct __finddata64_t *_FindData);
intptr_t __cdecl _findfirst64i32(const char *_Filename,struct _finddata64i32_t *_FindData);
int __cdecl _findnext32i64(intptr_t _FindHandle,struct _finddata32i64_t *_FindData);
int __cdecl _findnext64(intptr_t _FindHandle,struct __finddata64_t *_FindData);
int __cdecl _findnext64i32(intptr_t _FindHandle,struct _finddata64i32_t *_FindData);
__extension__ long long __cdecl _lseeki64(int _FileHandle,long long _Offset,int _Origin);
__extension__ long long __cdecl _telli64(int _FileHandle);
int __cdecl chdir (const char *) ;
char *__cdecl getcwd (char *, int) ;
int __cdecl mkdir (const char *) ;
char *__cdecl mktemp(char *) ;
int __cdecl rmdir (const char*) ;
int __cdecl chmod (const char *, int) ;
__declspec(dllimport) errno_t __cdecl _sopen_s(int *_FileHandle,const char *_Filename,int _OpenFlag,int _ShareFlag,int _PermissionMode);
int __cdecl _open(const char *_Filename,int _OpenFlag,...) ;
int __cdecl _sopen(const char *_Filename,int _OpenFlag,int _ShareFlag,...) ;
int __cdecl _waccess(const wchar_t *_Filename,int _AccessMode);
__declspec(dllimport) errno_t __cdecl _waccess_s (const wchar_t *_Filename,int _AccessMode);
int __cdecl _wchmod(const wchar_t *_Filename,int _Mode);
int __cdecl _wcreat(const wchar_t *_Filename,int _PermissionMode) ;
intptr_t __cdecl _wfindfirst32(const wchar_t *_Filename,struct _wfinddata32_t *_FindData);
int __cdecl _wfindnext32(intptr_t _FindHandle,struct _wfinddata32_t *_FindData);
int __cdecl _wunlink(const wchar_t *_Filename);
int __cdecl _wrename(const wchar_t *_OldFilename,const wchar_t *_NewFilename);
wchar_t *__cdecl _wmktemp(wchar_t *_TemplateName) ;
__declspec(dllimport) errno_t __cdecl _wmktemp_s (wchar_t *_TemplateName, size_t _SizeInWords);
intptr_t __cdecl _wfindfirst32i64(const wchar_t *_Filename,struct _wfinddata32i64_t *_FindData);
intptr_t __cdecl _wfindfirst64i32(const wchar_t *_Filename,struct _wfinddata64i32_t *_FindData);
intptr_t __cdecl _wfindfirst64(const wchar_t *_Filename,struct _wfinddata64_t *_FindData);
int __cdecl _wfindnext32i64(intptr_t _FindHandle,struct _wfinddata32i64_t *_FindData);
int __cdecl _wfindnext64i32(intptr_t _FindHandle,struct _wfinddata64i32_t *_FindData);
int __cdecl _wfindnext64(intptr_t _FindHandle,struct _wfinddata64_t *_FindData);
errno_t __cdecl _wsopen_s(int *_FileHandle,const wchar_t *_Filename,int _OpenFlag,int _ShareFlag,int _PermissionFlag);
int __cdecl _wopen(const wchar_t *_Filename,int _OpenFlag,...) ;
int __cdecl _wsopen(const wchar_t *_Filename,int _OpenFlag,int _ShareFlag,...) ;
int __cdecl __lock_fhandle(int _Filehandle);
void __cdecl _unlock_fhandle(int _Filehandle);
intptr_t __cdecl _get_osfhandle(int _FileHandle);
int __cdecl _open_osfhandle(intptr_t _OSFileHandle,int _Flags);
int __cdecl access(const char *_Filename,int _AccessMode) ;
int __cdecl chmod(const char *_Filename,int _AccessMode) ;
int __cdecl chsize(int _FileHandle,long _Size) ;
int __cdecl close(int _FileHandle) ;
int __cdecl creat(const char *_Filename,int _PermissionMode) ;
int __cdecl creat64(const char *_Filename,int _PermissionMode);
int __cdecl dup(int _FileHandle) ;
int __cdecl dup2(int _FileHandleSrc,int _FileHandleDst) ;
int __cdecl eof(int _FileHandle) ;
long __cdecl filelength(int _FileHandle) ;
int __cdecl isatty(int _FileHandle) ;
int __cdecl locking(int _FileHandle,int _LockMode,long _NumOfBytes) ;
off_t __cdecl lseek(int _FileHandle,off_t _Offset,int _Origin)
;
char *__cdecl mktemp(char *_TemplateName) ;
int __cdecl open(const char *_Filename,int _OpenFlag,...) ;
int __cdecl open64(const char *_Filename,int _OpenFlag,...);
int __cdecl read(int _FileHandle,void *_DstBuf,unsigned int _MaxCharCount) ;
int __cdecl setmode(int _FileHandle,int _Mode) ;
int __cdecl sopen(const char *_Filename,int _OpenFlag,int _ShareFlag,...) ;
long __cdecl tell(int _FileHandle) ;
int __cdecl umask(int _Mode) ;
int __cdecl write(int _Filehandle,const void *_Buf,unsigned int _MaxCharCount) ;
typedef unsigned short _ino_t;
typedef unsigned short ino_t;
typedef unsigned int _dev_t;
typedef unsigned int dev_t;
__extension__
typedef long long _pid_t;
typedef _pid_t pid_t;
typedef unsigned short _mode_t;
typedef _mode_t mode_t;
typedef unsigned int useconds_t;
struct timespec {
time_t tv_sec;
long tv_nsec;
};
struct itimerspec {
struct timespec it_interval;
struct timespec it_value;
};
__extension__
typedef unsigned long long _sigset_t;
struct _stat32 {
_dev_t st_dev;
_ino_t st_ino;
unsigned short st_mode;
short st_nlink;
short st_uid;
short st_gid;
_dev_t st_rdev;
_off_t st_size;
__time32_t st_atime;
__time32_t st_mtime;
__time32_t st_ctime;
};
struct _stat32i64 {
_dev_t st_dev;
_ino_t st_ino;
unsigned short st_mode;
short st_nlink;
short st_uid;
short st_gid;
_dev_t st_rdev;
__extension__ long long st_size;
__time32_t st_atime;
__time32_t st_mtime;
__time32_t st_ctime;
};
struct _stat64i32 {
_dev_t st_dev;
_ino_t st_ino;
unsigned short st_mode;
short st_nlink;
short st_uid;
short st_gid;
_dev_t st_rdev;
_off_t st_size;
__time64_t st_atime;
__time64_t st_mtime;
__time64_t st_ctime;
};
struct _stat64 {
_dev_t st_dev;
_ino_t st_ino;
unsigned short st_mode;
short st_nlink;
short st_uid;
short st_gid;
_dev_t st_rdev;
__extension__ long long st_size;
__time64_t st_atime;
__time64_t st_mtime;
__time64_t st_ctime;
};
int __cdecl _fstat32(int _FileDes,struct _stat32 *_Stat);
int __cdecl _stat32(const char *_Name,struct _stat32 *_Stat);
int __cdecl _fstat64(int _FileDes,struct _stat64 *_Stat);
int __cdecl _fstat32i64(int _FileDes,struct _stat32i64 *_Stat);
int __cdecl _fstat64i32(int _FileDes,struct _stat64i32 *_Stat);
int __cdecl _stat64(const char *_Name,struct _stat64 *_Stat);
int __cdecl _stat32i64(const char *_Name,struct _stat32i64 *_Stat);
int __cdecl _stat64i32(const char *_Name,struct _stat64i32 *_Stat);
int __cdecl _wstat32(const wchar_t *_Name,struct _stat32 *_Stat);
int __cdecl _wstat32i64(const wchar_t *_Name,struct _stat32i64 *_Stat);
int __cdecl _wstat64i32(const wchar_t *_Name,struct _stat64i32 *_Stat);
int __cdecl _wstat64(const wchar_t *_Name,struct _stat64 *_Stat);
struct stat {
_dev_t st_dev;
_ino_t st_ino;
unsigned short st_mode;
short st_nlink;
short st_uid;
short st_gid;
_dev_t st_rdev;
off_t st_size;
time_t st_atime;
time_t st_mtime;
time_t st_ctime;
};
int __cdecl fstat(int _Desc, struct stat *_Stat) __asm__("fstat64i32");
int __cdecl stat(const char *_Filename, struct stat *_Stat) __asm__("stat64i32");
int __cdecl wstat(const wchar_t *_Filename, struct stat *_Stat) __asm__("wstat64i32");
struct stat64 {
_dev_t st_dev;
_ino_t st_ino;
unsigned short st_mode;
short st_nlink;
short st_uid;
short st_gid;
_dev_t st_rdev;
__extension__ long long st_size;
__time64_t st_atime;
__time64_t st_mtime;
__time64_t st_ctime;
};
int __cdecl fstat64(int _Desc, struct stat64 *_Stat);
int __cdecl stat64(const char *_Filename, struct stat64 *_Stat);
int __cdecl wstat64(const wchar_t *_Filename, struct stat64 *_Stat);
int waitpid(int pid, int *status, int options);
struct __timeb32 {
__time32_t time;
unsigned short millitm;
short timezone;
short dstflag;
};
struct timeb {
time_t time;
unsigned short millitm;
short timezone;
short dstflag;
};
struct __timeb64 {
__time64_t time;
unsigned short millitm;
short timezone;
short dstflag;
};
void __cdecl _ftime64(struct __timeb64 *_Time);
void __cdecl _ftime32(struct __timeb32 *_Time);
struct _timespec32 {
__time32_t tv_sec;
long tv_nsec;
};
struct _timespec64 {
__time64_t tv_sec;
long tv_nsec;
};
int __cdecl ftime (struct timeb *) __asm__("ftime64");
errno_t __cdecl _ftime32_s(struct __timeb32 *_Time);
errno_t __cdecl _ftime64_s(struct __timeb64 *_Time);
typedef long clock_t;
struct tm {
int tm_sec;
int tm_min;
int tm_hour;
int tm_mday;
int tm_mon;
int tm_year;
int tm_wday;
int tm_yday;
int tm_isdst;
};
int *__cdecl __daylight(void);
long *__cdecl __dstbias(void);
long *__cdecl __timezone(void);
char **__cdecl __tzname(void);
errno_t __cdecl _get_daylight(int *_Daylight);
errno_t __cdecl _get_dstbias(long *_Daylight_savings_bias);
errno_t __cdecl _get_timezone(long *_Timezone);
errno_t __cdecl _get_tzname(size_t *_ReturnValue,char *_Buffer,size_t _SizeInBytes,int _Index);
char *__cdecl asctime(const struct tm *_Tm) ;
__declspec(dllimport) errno_t __cdecl asctime_s (char *_Buf,size_t _SizeInWords,const struct tm *_Tm);
char *__cdecl _ctime32(const __time32_t *_Time) ;
__declspec(dllimport) errno_t __cdecl _ctime32_s (char *_Buf,size_t _SizeInBytes,const __time32_t *_Time);
clock_t __cdecl clock(void);
double __cdecl _difftime32(__time32_t _Time1,__time32_t _Time2);
struct tm *__cdecl _gmtime32(const __time32_t *_Time) ;
__declspec(dllimport) errno_t __cdecl _gmtime32_s (struct tm *_Tm,const __time32_t *_Time);
struct tm *__cdecl _localtime32(const __time32_t *_Time) ;
__declspec(dllimport) errno_t __cdecl _localtime32_s (struct tm *_Tm,const __time32_t *_Time);
size_t __cdecl strftime(char * __restrict__ _Buf,size_t _SizeInBytes,const char * __restrict__ _Format,const struct tm * __restrict__ _Tm) __attribute__((__format__ (gnu_strftime, 3, 0)));
size_t __cdecl _strftime_l(char * __restrict__ _Buf,size_t _Max_size,const char * __restrict__ _Format,const struct tm * __restrict__ _Tm,_locale_t _Locale);
char *__cdecl _strdate(char *_Buffer) ;
__declspec(dllimport) errno_t __cdecl _strdate_s (char *_Buf,size_t _SizeInBytes);
char *__cdecl _strtime(char *_Buffer) ;
__declspec(dllimport) errno_t __cdecl _strtime_s (char *_Buf ,size_t _SizeInBytes);
__time32_t __cdecl _time32(__time32_t *_Time);
int __cdecl _timespec32_get(struct _timespec32 *_Ts, int _Base);
__time32_t __cdecl _mktime32(struct tm *_Tm);
__time32_t __cdecl _mkgmtime32(struct tm *_Tm);
void __cdecl tzset(void) ;
void __cdecl _tzset(void);
double __cdecl _difftime64(__time64_t _Time1,__time64_t _Time2);
char *__cdecl _ctime64(const __time64_t *_Time) ;
__declspec(dllimport) errno_t __cdecl _ctime64_s (char *_Buf,size_t _SizeInBytes,const __time64_t *_Time);
struct tm *__cdecl _gmtime64(const __time64_t *_Time) ;
__declspec(dllimport) errno_t __cdecl _gmtime64_s (struct tm *_Tm,const __time64_t *_Time);
struct tm *__cdecl _localtime64(const __time64_t *_Time) ;
__declspec(dllimport) errno_t __cdecl _localtime64_s (struct tm *_Tm,const __time64_t *_Time);
__time64_t __cdecl _mktime64(struct tm *_Tm);
__time64_t __cdecl _mkgmtime64(struct tm *_Tm);
__time64_t __cdecl _time64(__time64_t *_Time);
int __cdecl _timespec64_get(struct _timespec64 *_Ts, int _Base);
unsigned __cdecl _getsystime(struct tm *_Tm);
unsigned __cdecl _setsystime(struct tm *_Tm,unsigned _MilliSec);
wchar_t *__cdecl _wasctime(const struct tm *_Tm);
__declspec(dllimport) errno_t __cdecl _wasctime_s (wchar_t *_Buf,size_t _SizeInWords,const struct tm *_Tm);
wchar_t *__cdecl _wctime32(const __time32_t *_Time) ;
__declspec(dllimport) errno_t __cdecl _wctime32_s (wchar_t *_Buf,size_t _SizeInWords,const __time32_t *_Time);
size_t __cdecl wcsftime(wchar_t * __restrict__ _Buf,size_t _SizeInWords,const wchar_t * __restrict__ _Format,const struct tm * __restrict__ _Tm);
size_t __cdecl _wcsftime_l(wchar_t * __restrict__ _Buf,size_t _SizeInWords,const wchar_t * __restrict__ _Format,const struct tm * __restrict__ _Tm,_locale_t _Locale);
wchar_t *__cdecl _wstrdate(wchar_t *_Buffer) ;
__declspec(dllimport) errno_t __cdecl _wstrdate_s (wchar_t *_Buf,size_t _SizeInWords);
wchar_t *__cdecl _wstrtime(wchar_t *_Buffer) ;
__declspec(dllimport) errno_t __cdecl _wstrtime_s (wchar_t *_Buf,size_t _SizeInWords);
wchar_t *__cdecl _wctime64(const __time64_t *_Time) ;
__declspec(dllimport) errno_t __cdecl _wctime64_s (wchar_t *_Buf,size_t _SizeInWords,const __time64_t *_Time);
wchar_t *__cdecl _wctime(const time_t *_Time) __asm__("_wctime64") ;
errno_t __cdecl _wctime_s (wchar_t *_Buffer,size_t _SizeInWords,const time_t *_Time) __asm__("_wctime64_s");
time_t __cdecl time(time_t *_Time) __asm__("_time64");
int __cdecl timespec_get(struct timespec* _Ts, int _Base) __asm__("_timespec64_get");
double __cdecl difftime(time_t _Time1,time_t _Time2) __asm__("_difftime64");
struct tm *__cdecl localtime(const time_t *_Time) __asm__("_localtime64");
errno_t __cdecl localtime_s(struct tm *_Tm,const time_t *_Time) __asm__("_localtime64_s");
struct tm *__cdecl gmtime(const time_t *_Time) __asm__("_gmtime64");
errno_t __cdecl gmtime_s(struct tm *_Tm, const time_t *_Time) __asm__("_gmtime64_s");
char *__cdecl ctime(const time_t *_Time) __asm__("_ctime64");
errno_t __cdecl ctime_s(char *_Buf,size_t _SizeInBytes,const time_t *_Time) __asm__("_ctime64_s");
time_t __cdecl mktime(struct tm *_Tm) __asm__("_mktime64");
time_t __cdecl _mkgmtime(struct tm *_Tm) __asm__("_mkgmtime64");
extern int daylight ;
extern long timezone ;
extern char *tzname[2] ;
void __cdecl tzset(void) ;
struct timeval
{
long tv_sec;
long tv_usec;
};
struct timezone {
int tz_minuteswest;
int tz_dsttime;
};
extern int __cdecl mingw_gettimeofday (struct timeval *p, struct timezone *z);
extern inline __attribute__((__always_inline__,__gnu_inline__)) struct tm *__cdecl localtime_r(const time_t *_Time, struct tm *_Tm) {
return localtime_s(_Tm, _Time) ? ((void *)0) : _Tm;
}
extern inline __attribute__((__always_inline__,__gnu_inline__)) struct tm *__cdecl gmtime_r(const time_t *_Time, struct tm *_Tm) {
return gmtime_s(_Tm, _Time) ? ((void *)0) : _Tm;
}
extern inline __attribute__((__always_inline__,__gnu_inline__)) char *__cdecl ctime_r(const time_t *_Time, char *_Str) {
return ctime_s(_Str, 0x7fffffff, _Time) ? ((void *)0) : _Str;
}
extern inline __attribute__((__always_inline__,__gnu_inline__)) char *__cdecl asctime_r(const struct tm *_Tm, char * _Str) {
return asctime_s(_Str, 0x7fffffff, _Tm) ? ((void *)0) : _Str;
}
typedef int clockid_t;
int __cdecl nanosleep32(const struct _timespec32 *request, struct _timespec32 *remain);
int __cdecl nanosleep64(const struct _timespec64 *request, struct _timespec64 *remain);
static inline __attribute__((__always_inline__)) int __cdecl nanosleep(const struct timespec *request, struct timespec *remain)
{
return nanosleep64 ((struct _timespec64 *)request, (struct _timespec64 *)remain);
}
int __cdecl clock_nanosleep32(clockid_t clock_id, int flags, const struct _timespec32 *request, struct _timespec32 *remain);
int __cdecl clock_nanosleep64(clockid_t clock_id, int flags, const struct _timespec64 *request, struct _timespec64 *remain);
static inline __attribute__((__always_inline__)) int __cdecl clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *request, struct timespec *remain)
{
return clock_nanosleep64 (clock_id, flags, (struct _timespec64 *)request, (struct _timespec64 *)remain);
}
int __cdecl clock_getres32(clockid_t clock_id, struct _timespec32 *res);
int __cdecl clock_getres64(clockid_t clock_id, struct _timespec64 *res);
static inline __attribute__((__always_inline__)) int __cdecl clock_getres(clockid_t clock_id, struct timespec *res)
{
return clock_getres64 (clock_id, (struct _timespec64 *)res);
}
int __cdecl clock_gettime32(clockid_t clock_id, struct _timespec32 *tp);
int __cdecl clock_gettime64(clockid_t clock_id, struct _timespec64 *tp);
static inline __attribute__((__always_inline__)) int __cdecl clock_gettime(clockid_t clock_id, struct timespec *tp)
{
return clock_gettime64 (clock_id, (struct _timespec64 *)tp);
}
int __cdecl clock_settime32(clockid_t clock_id, const struct _timespec32 *tp);
int __cdecl clock_settime64(clockid_t clock_id, const struct _timespec64 *tp);
static inline __attribute__((__always_inline__)) int __cdecl clock_settime(clockid_t clock_id, const struct timespec *tp)
{
return clock_settime64 (clock_id, (struct _timespec64 *)tp);
}
char *dirname(char *path);
char *basename(char *path);
char *ctime_r(const time_t *timer, char *buf);
int strncasecmp(const char *a, const char *b, size_t n);
void chibicc_assert_fail(const char *expr, const char *file, int line, const char *func);
static char *chibicc_strndup(const char *s, size_t n)
{
size_t len = strlen(s);
if (len > n)
len = n;
char *p = malloc(len + 1);
if (!p)
return ((void *)0);
memcpy(p, s, len);
p[len] = '\0';
return p;
}
typedef struct Type Type;
typedef struct Node Node;
typedef struct Member Member;
typedef struct Relocation Relocation;
typedef struct Hideset Hideset;
typedef struct {
char **data;
int capacity;
int len;
} StringArray;
void strarray_push(StringArray *arr, char *s);
char *format(char *fmt, ...) __attribute__((format(printf, 1, 2)));
extern StringArray tmpfiles;
char *create_tmpfile(void);
typedef enum {
TK_IDENT,
TK_PUNCT,
TK_KEYWORD,
TK_STR,
TK_NUM,
TK_PP_NUM,
TK_EOF,
} TokenKind;
typedef struct {
char *name;
int file_no;
char *contents;
char *display_name;
int line_delta;
} File;
typedef struct Token Token;
struct Token {
TokenKind kind;
Token *next;
int64_t val;
long double fval;
char *loc;
int len;
Type *ty;
char *str;
File *file;
char *filename;
int line_no;
int line_delta;
_Bool at_bol;
_Bool has_space;
Hideset *hideset;
Token *origin;
};
_Noreturn void error(char *fmt, ...) __attribute__((format(printf, 1, 2)));
_Noreturn void error_at(char *loc, char *fmt, ...) __attribute__((format(printf, 2, 3)));
_Noreturn void error_tok(Token *tok, char *fmt, ...) __attribute__((format(printf, 2, 3)));
void warn_tok(Token *tok, char *fmt, ...) __attribute__((format(printf, 2, 3)));
_Bool equal(Token *tok, char *op);
Token *skip(Token *tok, char *op);
_Bool consume(Token **rest, Token *tok, char *str);
void convert_pp_tokens(Token *tok);
File **get_input_files(void);
File *new_file(char *name, int file_no, char *contents);
Token *tokenize_string_literal(Token *tok, Type *basety);
Token *tokenize(File *file);
Token *tokenize_file(char *filename);
char *search_include_paths(char *filename);
void init_macros(void);
void define_macro(char *name, char *buf);
void undef_macro(char *name);
Token *preprocess(Token *tok);
typedef struct ABI ABI;
typedef struct Obj Obj;
struct Obj {
Obj *next;
char *name;
Type *ty;
Token *tok;
_Bool is_local;
int align;
int offset;
_Bool is_function;
_Bool is_definition;
_Bool is_static;
_Bool is_tentative;
_Bool is_tls;
_Bool is_readonly;
char *init_data;
Relocation *rel;
_Bool is_inline;
Obj *params;
Node *body;
Obj *locals;
Obj *va_area;
Obj *alloca_bottom;
int stack_size;
int callee_saved_mask;
ABI *abi;
_Bool is_live;
_Bool is_root;
StringArray refs;
};
typedef struct Relocation Relocation;
struct Relocation {
Relocation *next;
int offset;
char **label;
long addend;
};
typedef enum {
ND_NULL_EXPR,
ND_ADD,
ND_SUB,
ND_MUL,
ND_DIV,
ND_NEG,
ND_MOD,
ND_BITAND,
ND_BITOR,
ND_BITXOR,
ND_SHL,
ND_SHR,
ND_EQ,
ND_NE,
ND_LT,
ND_LE,
ND_ASSIGN,
ND_COND,
ND_COMMA,
ND_MEMBER,
ND_ADDR,
ND_DEREF,
ND_NOT,
ND_BITNOT,
ND_LOGAND,
ND_LOGOR,
ND_RETURN,
ND_IF,
ND_FOR,
ND_DO,
ND_SWITCH,
ND_CASE,
ND_BLOCK,
ND_GOTO,
ND_GOTO_EXPR,
ND_LABEL,
ND_LABEL_VAL,
ND_FUNCALL,
ND_EXPR_STMT,
ND_STMT_EXPR,
ND_VAR,
ND_VLA_PTR,
ND_NUM,
ND_CAST,
ND_MEMZERO,
ND_ASM,
ND_CAS,
ND_EXCH,
} NodeKind;
struct Node {
NodeKind kind;
Node *next;
Type *ty;
Token *tok;
Node *lhs;
Node *rhs;
union {
struct {
Node *cond;
Node *then;
Node *els;
Node *init;
Node *inc;
char *brk_label;
char *cont_label;
Node *case_next;
Node *default_case;
long begin;
long end;
char *label;
char *unique_label;
Node *goto_next;
};
struct {
Node *body;
};
struct {
Member *member;
};
struct {
Type *func_ty;
Node *args;
Obj *ret_buffer;
_Bool pass_by_stack;
};
struct {
char *asm_str;
};
struct {
Node *cas_addr;
Node *cas_old;
Node *cas_new;
Obj *atomic_addr;
Node *atomic_expr;
};
struct {
Obj *var;
};
struct {
int64_t val;
long double fval;
};
};
};
typedef struct VarScope VarScope;
struct VarScope {
Obj *var;
Type *type_def;
Type *enum_ty;
int enum_val;
char *func_name;
};
Node *new_cast(Node *expr, Type *ty);
Node *new_node(NodeKind kind, Token *tok);
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs, Token *tok);
Node *new_unary(NodeKind kind, Node *expr, Token *tok);
Node *new_num(int64_t val, Token *tok);
Node *new_long(int64_t val, Token *tok);
Node *new_ulong(long val, Token *tok);
Node *new_var_node(Obj *var, Token *tok);
Node *new_vla_ptr(Obj *var, Token *tok);
Node *new_add(Node *lhs, Node *rhs, Token *tok);
Node *new_sub(Node *lhs, Node *rhs, Token *tok);
Node *new_if_node(Node *cond, Node *then, Node *els, Token *tok);
Node *new_for_node(Node *init, Node *cond, Node *inc, Node *then, Token *tok);
Node *new_do_node(Node *then, Node *cond, Token *tok);
Node *new_switch_node(Node *cond, Node *then, Token *tok);
Node *new_case_node(long begin, long end, Token *tok);
Node *new_block_node(Node *body, Token *tok);
Node *new_goto_node(char *label, Token *tok);
Node *new_goto_expr_node(Node *expr, Token *tok);
Node *new_label_node(char *label, Token *tok);
Node *new_return_node(Node *expr, Token *tok);
Node *new_expr_stmt_node(Node *expr, Token *tok);
Node *new_stmt_expr_node(Node *body, Token *tok);
Node *new_member_node(Node *lhs, Member *member, Token *tok);
Node *new_funcall_node(Token *tok, Type *func_ty, Node *args);
Node *new_asm_node(char *asm_str, Token *tok);
Node *new_cas_node(Node *addr, Node *old_val, Node *new_val, Token *tok);
Node *new_exch_node(Node *addr, Node *val, Token *tok);
const char *node_kind_name(NodeKind kind);
_Bool node_is_binary(NodeKind kind);
_Bool node_is_unary(NodeKind kind);
_Bool node_is_control_flow(NodeKind kind);
_Bool node_is_atomic(NodeKind kind);
Obj *new_lvar(char *name, Type *ty);
VarScope *find_var(Token *tok);
VarScope *push_scope(char *name);
int64_t const_expr(Token **rest, Token *tok);
Obj *parse(Token *tok);
typedef enum {
TY_VOID,
TY_BOOL,
TY_CHAR,
TY_SHORT,
TY_INT,
TY_LONG,
TY_LONGLONG,
TY_FLOAT,
TY_DOUBLE,
TY_LDOUBLE,
TY_ENUM,
TY_PTR,
TY_FUNC,
TY_ARRAY,
TY_VLA,
TY_STRUCT,
TY_UNION,
} TypeKind;
struct Type {
TypeKind kind;
int size;
int align;
_Bool is_unsigned;
_Bool is_atomic;
Type *origin;
Type *base;
Token *name;
Token *name_pos;
int array_len;
Node *vla_len;
Obj *vla_size;
Member *members;
_Bool is_flexible;
_Bool is_packed;
Type *return_ty;
Type *params;
_Bool is_variadic;
Type *next;
ABI *abi;
};
struct Member {
Member *next;
Type *ty;
Token *tok;
Token *name;
int idx;
int align;
int offset;
_Bool is_bitfield;
int bit_offset;
int bit_width;
};
extern Type *ty_void;
extern Type *ty_bool;
extern Type *ty_char;
extern Type *ty_short;
extern Type *ty_int;
extern Type *ty_long;
extern Type *ty_llong;
extern Type *ty_uchar;
extern Type *ty_ushort;
extern Type *ty_uint;
extern Type *ty_ulong;
extern Type *ty_ullong;
extern Type *ty_float;
extern Type *ty_double;
extern Type *ty_ldouble;
_Bool is_integer(Type *ty);
_Bool is_flonum(Type *ty);
_Bool is_numeric(Type *ty);
_Bool is_compatible(Type *t1, Type *t2);
Type *copy_type(Type *ty);
Type *pointer_to(Type *base);
Type *func_type(Type *return_ty);
Type *array_of(Type *base, int size);
Type *vla_of(Type *base, Node *expr);
Type *enum_type(void);
Type *struct_type(void);
void add_type(Node *node);
typedef struct Obj Obj;
typedef struct ObjFmt ObjFmt;
struct ObjFmt {
const char *name;
const char *description;
void (*emit_var_decl)(Obj *var, FILE *out);
void (*emit_var_type_size)(Obj *var, FILE *out);
void (*emit_fn_decl)(Obj *fn, FILE *out);
void (*emit_fn_type)(Obj *fn, FILE *out);
};
extern ObjFmt *current_objfmt;
extern ObjFmt objfmt_elf;
extern ObjFmt objfmt_coff;
extern ObjFmt objfmt_macho;
extern ObjFmt objfmt_flat;
void set_objfmt(const char *name);
ObjFmt *get_objfmt(const char *name);
void init_objfmts(void);
typedef struct CallConv CallConv;
struct CallConv {
const char *name;
int num_gp_regs;
const char *gp_regs64[8];
const char *gp_regs32[8];
const char *gp_regs16[8];
const char *gp_regs8[8];
int num_fp_regs;
int shadow_space;
_Bool paired_slots;
_Bool pass_struct_by_ref;
int stack_align;
};
typedef struct Type Type;
typedef struct Node Node;
typedef struct Obj Obj;
typedef struct ABI ABI;
typedef struct LLIRInsn LLIRInsn;
struct ABI {
const char *name;
const char *description;
const CallConv *callconv;
ObjFmt *default_objfmt;
int size_bool;
int size_char;
int size_short;
int size_int;
int size_long;
int size_llong;
int size_ptr;
int size_float;
int size_double;
int size_ldouble;
int align_bool;
int align_char;
int align_short;
int align_int;
int align_long;
int align_llong;
int align_ptr;
int align_float;
int align_double;
int align_ldouble;
int align_stack;
int va_area_size;
int va_area_align;
_Bool (*returns_by_reference)(Type *ty);
int (*classify_reg)(Type *ty);
void (*assign_lvar_offsets)(Obj *prog);
int (*get_spill_base)(Obj *fn);
void (*finalize_stack)(Obj *fn, int spill_offset);
int (*push_args)(Node *node, FILE *out, int *depth);
void (*copy_ret_buffer)(Obj *var, FILE *out);
void (*copy_struct_reg)(Obj *fn, FILE *out);
void (*copy_struct_mem)(Obj *fn, FILE *out);
void (*builtin_alloca)(Obj *fn, FILE *out);
void (*pre_call)(Node *node, FILE *out);
Node *(*builtin_va_start)(Node *ap, Node *last, Token *tok);
Node *(*builtin_va_arg)(Node *ap, Type *ty, Token *tok);
Node *(*builtin_va_copy)(Node *dest, Node *src, Token *tok);
Node *(*builtin_va_end)(Node *ap, Token *tok);
void (*emit_prologue)(Obj *fn, FILE *out);
void (*emit_epilogue)(Obj *fn, FILE *out);
void (*emit_return)(Obj *fn, Type *return_ty, FILE *out);
void (*emit_call)(LLIRInsn *insn, FILE *out);
void (*define_macros)(void);
void (*init_types)(void);
void (*declare_builtin_types)(void);
};
extern ABI *current_abi;
void register_abi(ABI *abi);
ABI *get_abi(const char *name);
void init_abis(void);
void set_abi(const char *name);
void declare_abi_builtin_types(void);
const char *abi_x86_reg32(const char *r64);
const char *abi_x86_reg16(const char *r64);
const char *abi_x86_reg8(const char *r64);
Node *abi_va_start_ptr(Node *ap, Node *last, Token *tok);
Node *abi_va_arg_ptr(Node *ap, Type *ty, int slot_size, Token *tok);
Node *abi_va_copy_ptr(Node *dest, Node *src, Token *tok);
Node *abi_va_end_nop(Node *ap, Token *tok);
extern ABI abi_sysv64;
extern ABI abi_win64;
extern ABI abi_win32;
extern ABI abi_sys6;
extern ABI abi_pascal;
extern ABI abi_z80;
typedef struct Obj Obj;
typedef struct Node Node;
typedef struct Codegen Codegen;
typedef struct LLIRProg LLIRProg;
typedef struct LLIRInsn LLIRInsn;
struct Codegen {
const char *name;
const char *description;
const char *default_abi_name;
void (*init)(FILE *out);
void (*codegen_llir)(LLIRProg *prog, FILE *out);
void (*emit_data)(Obj *prog, FILE *out);
void (*emit_text)(LLIRProg *prog, FILE *out);
void (*gen_insn)(LLIRInsn *insn, FILE *out);
void (*gen_expr)(LLIRInsn *insn, FILE *out);
};
extern Codegen *current_codegen;
void register_codegen(Codegen *cg);
Codegen *get_codegen(const char *name);
void init_codegens(void);
void set_codegen(const char *name);
extern Codegen codegen_x86_64;
extern Codegen codegen_m68k;
extern Codegen codegen_z80;
int align_to(int n, int align);
int codegen_label_count(void);
void codegen_set_print_hook(void (*hook)(const char *s));
void codegen_println(FILE *out, char *fmt, ...);
void codegen(Obj *prog, FILE *out);
void init_target(const char *target_name, const char *abi_name);
void init_all_targets_and_abis(void);
static inline ABI *get_node_abi(Node *node) {
if (node && node->func_ty && node->func_ty->abi)
return node->func_ty->abi;
return current_abi;
}
static inline ABI *get_fn_abi(Obj *fn) {
if (fn && fn->abi)
return fn->abi;
if (fn && fn->ty && fn->ty->abi)
return fn->ty->abi;
return current_abi;
}
int encode_utf8(char *buf, uint32_t c);
uint32_t decode_utf8(char **new_pos, char *p);
_Bool is_ident1(uint32_t c);
_Bool is_ident2(uint32_t c);
int display_width(char *p, int len);
typedef struct {
char *key;
int keylen;
void *val;
} HashEntry;
typedef struct {
HashEntry *buckets;
int capacity;
int used;
} HashMap;
void *hashmap_get(HashMap *map, char *key);
void *hashmap_get2(HashMap *map, char *key, int keylen);
void hashmap_put(HashMap *map, char *key, void *val);
void hashmap_put2(HashMap *map, char *key, int keylen, void *val);
void hashmap_delete(HashMap *map, char *key);
void hashmap_delete2(HashMap *map, char *key, int keylen);
void hashmap_test(void);
_Bool file_exists(const char *path);
extern StringArray include_paths;
extern _Bool opt_fpic;
extern _Bool opt_fcommon;
extern _Bool opt_ffunction_sections;
extern _Bool opt_fdata_sections;
extern _Bool opt_g;
extern int opt_O;
extern _Bool opt_dump_ir;
extern char *base_file;
typedef struct Scope Scope;
struct Scope {
Scope *next;
HashMap vars;
HashMap tags;
};
typedef struct {
_Bool is_typedef;
_Bool is_static;
_Bool is_extern;
_Bool is_inline;
_Bool is_tls;
_Bool is_const;
_Bool is_packed;
int align;
ABI *abi;
char *asm_name;
} VarAttr;
typedef struct Initializer Initializer;
struct Initializer {
Initializer *next;
Type *ty;
Token *tok;
_Bool is_flexible;
Node *expr;
Initializer **children;
Member *mem;
};
typedef struct InitDesg InitDesg;
struct InitDesg {
InitDesg *next;
int idx;
Member *member;
Obj *var;
};
static Obj *locals;
static Obj *globals;
static Scope *scope = &(Scope){};
static Obj *current_fn;
static Node *gotos;
static Node *labels;
static char *brk_label;
static char *cont_label;
static Node *current_switch;
static Obj *builtin_alloca;
int64_t const_expr(Token **rest, Token *tok);
static _Bool is_typename(Token *tok);
static Type *declspec(Token **rest, Token *tok, VarAttr *attr);
static Type *typename(Token **rest, Token *tok);
static Type *enum_specifier(Token **rest, Token *tok);
static Type *typeof_specifier(Token **rest, Token *tok);
static Type *type_suffix(Token **rest, Token *tok, Type *ty);
static Type *declarator(Token **rest, Token *tok, Type *ty);
static Node *declaration(Token **rest, Token *tok, Type *basety, VarAttr *attr);
static void static_assertion(Token **rest, Token *tok);
static void array_initializer2(Token **rest, Token *tok, Initializer *init, int i);
static void struct_initializer2(Token **rest, Token *tok, Initializer *init, Member *mem);
static void initializer2(Token **rest, Token *tok, Initializer *init);
static Initializer *initializer(Token **rest, Token *tok, Type *ty, Type **new_ty);
static Node *lvar_initializer(Token **rest, Token *tok, Obj *var);
static void gvar_initializer(Token **rest, Token *tok, Obj *var);
static Node *compound_stmt(Token **rest, Token *tok);
static Node *stmt(Token **rest, Token *tok);
static Node *expr_stmt(Token **rest, Token *tok);
static Node *expr(Token **rest, Token *tok);
static int64_t eval(Node *node);
static int64_t eval2(Node *node, char ***label);
static int64_t eval_rval(Node *node, char ***label);
static _Bool is_const_expr(Node *node);
static Node *assign(Token **rest, Token *tok);
static Node *logor(Token **rest, Token *tok);
static double eval_double(Node *node);
static Node *conditional(Token **rest, Token *tok);
static Node *logand(Token **rest, Token *tok);
static Node *bitor(Token **rest, Token *tok);
static Node *bitxor(Token **rest, Token *tok);
static Node *bitand(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *tok);
static Node *relational(Token **rest, Token *tok);
static Node *shift(Token **rest, Token *tok);
static Node *add(Token **rest, Token *tok);
Node *new_add(Node *lhs, Node *rhs, Token *tok);
Node *new_sub(Node *lhs, Node *rhs, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *cast(Token **rest, Token *tok);
static Member *get_struct_member(Type *ty, Token *tok);
static Type *struct_decl(Token **rest, Token *tok);
static Type *union_decl(Token **rest, Token *tok);
static Node *postfix(Token **rest, Token *tok);
static Node *funcall(Token **rest, Token *tok, Node *node);
static Node *unary(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);
static Token *parse_typedef(Token *tok, Type *basety);
static _Bool is_function(Token *tok);
static Token *function(Token *tok, Type *basety, VarAttr *attr);
static Token *global_variable(Token *tok, Type *basety, VarAttr *attr);
static int align_down(int n, int align) {
return align_to(n - align + 1, align);
}
static void enter_scope(void) {
Scope *sc = calloc(1, sizeof(Scope));
sc->next = scope;
scope = sc;
}
static void leave_scope(void) {
scope = scope->next;
}
VarScope *find_var(Token *tok) {
for (Scope *sc = scope; sc; sc = sc->next) {
VarScope *sc2 = hashmap_get2(&sc->vars, tok->loc, tok->len);
if (sc2)
return sc2;
}
return ((void *)0);
}
static Type *find_tag(Token *tok) {
for (Scope *sc = scope; sc; sc = sc->next) {
Type *ty = hashmap_get2(&sc->tags, tok->loc, tok->len);
if (ty)
return ty;
}
return ((void *)0);
}
typedef struct NodeChunk NodeChunk;
struct NodeChunk {
NodeChunk *next;
Node nodes[1024];
};
static NodeChunk *node_chunks;
static int node_chunk_idx = 1024;
Node *new_node(NodeKind kind, Token *tok) {
((tok != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"tok != ((void *)0)",
"parse.c",
201, __func__));
if (node_chunk_idx >= 1024) {
NodeChunk *chunk = calloc(1, sizeof(NodeChunk));
chunk->next = node_chunks;
node_chunks = chunk;
node_chunk_idx = 0;
}
Node *node = &node_chunks->nodes[node_chunk_idx++];
node->kind = kind;
node->tok = tok;
return node;
}
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs, Token *tok) {
((lhs != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"lhs != ((void *)0)",
"parse.c",
215, __func__));
((rhs != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"rhs != ((void *)0)",
"parse.c",
216, __func__));
((tok != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"tok != ((void *)0)",
"parse.c",
217, __func__));
Node *node = new_node(kind, tok);
node->lhs = lhs;
node->rhs = rhs;
return node;
}
Node *new_unary(NodeKind kind, Node *expr, Token *tok) {
((expr != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"expr != ((void *)0)",
"parse.c",
225, __func__));
((tok != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"tok != ((void *)0)",
"parse.c",
226, __func__));
Node *node = new_node(kind, tok);
node->lhs = expr;
return node;
}
Node *new_num(int64_t val, Token *tok) {
((tok != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"tok != ((void *)0)",
"parse.c",
233, __func__));
Node *node = new_node(ND_NUM, tok);
node->val = val;
return node;
}
Node *new_long(int64_t val, Token *tok) {
((tok != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"tok != ((void *)0)",
"parse.c",
240, __func__));
Node *node = new_node(ND_NUM, tok);
node->val = val;
node->ty = ty_long;
return node;
}
Node *new_ulong(long val, Token *tok) {
((tok != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"tok != ((void *)0)",
"parse.c",
248, __func__));
Node *node = new_node(ND_NUM, tok);
node->val = val;
node->ty = ty_ulong;
return node;
}
Node *new_var_node(Obj *var, Token *tok) {
((var != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"var != ((void *)0)",
"parse.c",
256, __func__));
((tok != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"tok != ((void *)0)",
"parse.c",
257, __func__));
Node *node = new_node(ND_VAR, tok);
node->var = var;
return node;
}
Node *new_vla_ptr(Obj *var, Token *tok) {
((var != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"var != ((void *)0)",
"parse.c",
264, __func__));
((tok != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"tok != ((void *)0)",
"parse.c",
265, __func__));
Node *node = new_node(ND_VLA_PTR, tok);
node->var = var;
return node;
}
Node *new_cast(Node *expr, Type *ty) {
((expr != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"expr != ((void *)0)",
"parse.c",
272, __func__));
((ty != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"ty != ((void *)0)",
"parse.c",
273, __func__));
add_type(expr);
Node *node = new_node(ND_CAST, expr->tok);
node->lhs = expr;
node->ty = copy_type(ty);
return node;
}
Node *new_if_node(Node *cond, Node *then, Node *els, Token *tok) {
((tok != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"tok != ((void *)0)",
"parse.c",
283, __func__));
Node *node = new_node(ND_IF, tok);
node->cond = cond;
node->then = then;
node->els = els;
return node;
}
Node *new_for_node(Node *init, Node *cond, Node *inc, Node *then, Token *tok) {
((tok != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"tok != ((void *)0)",
"parse.c",
292, __func__));
Node *node = new_node(ND_FOR, tok);
node->init = init;
node->cond = cond;
node->inc = inc;
node->then = then;
return node;
}
Node *new_do_node(Node *then, Node *cond, Token *tok) {
((tok != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"tok != ((void *)0)",
"parse.c",
302, __func__));
Node *node = new_node(ND_DO, tok);
node->then = then;
node->cond = cond;
return node;
}
Node *new_switch_node(Node *cond, Node *then, Token *tok) {
((tok != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"tok != ((void *)0)",
"parse.c",
310, __func__));
Node *node = new_node(ND_SWITCH, tok);
node->cond = cond;
node->then = then;
return node;
}
Node *new_case_node(long begin, long end, Token *tok) {
((tok != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"tok != ((void *)0)",
"parse.c",
318, __func__));
Node *node = new_node(ND_CASE, tok);
node->begin = begin;
node->end = end;
return node;
}
Node *new_block_node(Node *body, Token *tok) {
((tok != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"tok != ((void *)0)",
"parse.c",
326, __func__));
Node *node = new_node(ND_BLOCK, tok);
node->body = body;
return node;
}
Node *new_goto_node(char *label, Token *tok) {
((tok != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"tok != ((void *)0)",
"parse.c",
333, __func__));
Node *node = new_node(ND_GOTO, tok);
node->label = label;
return node;
}
Node *new_goto_expr_node(Node *expr, Token *tok) {
((tok != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"tok != ((void *)0)",
"parse.c",
340, __func__));
Node *node = new_node(ND_GOTO_EXPR, tok);
node->lhs = expr;
return node;
}
Node *new_label_node(char *label, Token *tok) {
((tok != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"tok != ((void *)0)",
"parse.c",
347, __func__));
Node *node = new_node(ND_LABEL, tok);
node->label = label;
return node;
}
Node *new_return_node(Node *expr, Token *tok) {
((tok != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"tok != ((void *)0)",
"parse.c",
354, __func__));
Node *node = new_node(ND_RETURN, tok);
node->lhs = expr;
return node;
}
Node *new_expr_stmt_node(Node *expr, Token *tok) {
((tok != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"tok != ((void *)0)",
"parse.c",
361, __func__));
Node *node = new_node(ND_EXPR_STMT, tok);
node->lhs = expr;
return node;
}
Node *new_stmt_expr_node(Node *body, Token *tok) {
((tok != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"tok != ((void *)0)",
"parse.c",
368, __func__));
Node *node = new_node(ND_STMT_EXPR, tok);
node->body = body;
return node;
}
Node *new_member_node(Node *lhs, Member *member, Token *tok) {
((tok != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"tok != ((void *)0)",
"parse.c",
375, __func__));
Node *node = new_node(ND_MEMBER, tok);
node->lhs = lhs;
node->member = member;
return node;
}
Node *new_funcall_node(Token *tok, Type *func_ty, Node *args) {
((tok != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"tok != ((void *)0)",
"parse.c",
383, __func__));
Node *node = new_node(ND_FUNCALL, tok);
node->func_ty = func_ty;
node->args = args;
return node;
}
Node *new_asm_node(char *asm_str, Token *tok) {
((tok != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"tok != ((void *)0)",
"parse.c",
391, __func__));
Node *node = new_node(ND_ASM, tok);
node->asm_str = asm_str;
return node;
}
Node *new_cas_node(Node *addr, Node *old_val, Node *new_val, Token *tok) {
((tok != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"tok != ((void *)0)",
"parse.c",
398, __func__));
Node *node = new_node(ND_CAS, tok);
node->cas_addr = addr;
node->cas_old = old_val;
node->cas_new = new_val;
return node;
}
Node *new_exch_node(Node *addr, Node *val, Token *tok) {
((tok != ((void *)0)) ? (void)0 : chibicc_assert_fail(
"tok != ((void *)0)",
"parse.c",
407, __func__));
Node *node = new_node(ND_EXCH, tok);
node->lhs = addr;
node->rhs = val;
return node;
}
const char *node_kind_name(NodeKind kind) {
switch (kind) {
case ND_NULL_EXPR: return "ND_NULL_EXPR";
case ND_ADD: return "ND_ADD";
case ND_SUB: return "ND_SUB";
case ND_MUL: return "ND_MUL";
case ND_DIV: return "ND_DIV";
case ND_NEG: return "ND_NEG";
case ND_MOD: return "ND_MOD";
case ND_BITAND: return "ND_BITAND";
case ND_BITOR: return "ND_BITOR";
case ND_BITXOR: return "ND_BITXOR";
case ND_SHL: return "ND_SHL";
case ND_SHR: return "ND_SHR";
case ND_EQ: return "ND_EQ";
case ND_NE: return "ND_NE";
case ND_LT: return "ND_LT";
case ND_LE: return "ND_LE";
case ND_ASSIGN: return "ND_ASSIGN";
case ND_COND: return "ND_COND";
case ND_COMMA: return "ND_COMMA";
case ND_MEMBER: return "ND_MEMBER";
case ND_ADDR: return "ND_ADDR";
case ND_DEREF: return "ND_DEREF";
case ND_NOT: return "ND_NOT";
case ND_BITNOT: return "ND_BITNOT";
case ND_LOGAND: return "ND_LOGAND";
case ND_LOGOR: return "ND_LOGOR";
case ND_RETURN: return "ND_RETURN";
case ND_IF: return "ND_IF";
case ND_FOR: return "ND_FOR";
case ND_DO: return "ND_DO";
case ND_SWITCH: return "ND_SWITCH";
case ND_CASE: return "ND_CASE";
case ND_BLOCK: return "ND_BLOCK";
case ND_GOTO: return "ND_GOTO";
case ND_GOTO_EXPR: return "ND_GOTO_EXPR";
case ND_LABEL: return "ND_LABEL";
case ND_LABEL_VAL: return "ND_LABEL_VAL";
case ND_FUNCALL: return "ND_FUNCALL";
case ND_EXPR_STMT: return "ND_EXPR_STMT";
case ND_STMT_EXPR: return "ND_STMT_EXPR";
case ND_VAR: return "ND_VAR";
case ND_VLA_PTR: return "ND_VLA_PTR";
case ND_NUM: return "ND_NUM";
case ND_CAST: return "ND_CAST";
case ND_MEMZERO: return "ND_MEMZERO";
case ND_ASM: return "ND_ASM";
case ND_CAS: return "ND_CAS";
case ND_EXCH: return "ND_EXCH";
default: return "ND_UNKNOWN";
}
}
_Bool node_is_binary(NodeKind kind) {
switch (kind) {
case ND_ADD:
case ND_SUB:
case ND_MUL:
case ND_DIV:
case ND_MOD:
case ND_BITAND:
case ND_BITOR:
case ND_BITXOR:
case ND_SHL:
case ND_SHR:
case ND_EQ:
case ND_NE:
case ND_LT:
case ND_LE:
case ND_ASSIGN:
case ND_COMMA:
case ND_LOGAND:
case ND_LOGOR:
return 1;
default:
return 0;
}
}
_Bool node_is_unary(NodeKind kind) {
switch (kind) {
case ND_NEG:
case ND_ADDR:
case ND_DEREF:
case ND_NOT:
case ND_BITNOT:
case ND_CAST:
case ND_EXPR_STMT:
return 1;
default:
return 0;
}
}
_Bool node_is_control_flow(NodeKind kind) {
switch (kind) {
case ND_RETURN:
case ND_IF:
case ND_FOR:
case ND_DO:
case ND_SWITCH:
case ND_CASE:
case ND_BLOCK:
case ND_GOTO:
case ND_GOTO_EXPR:
case ND_LABEL:
return 1;
default:
return 0;
}
}
_Bool node_is_atomic(NodeKind kind) {
return kind == ND_CAS || kind == ND_EXCH;
}
VarScope *push_scope(char *name) {
VarScope *sc = calloc(1, sizeof(VarScope));
hashmap_put(&scope->vars, name, sc);
return sc;
}
static Initializer *new_initializer(Type *ty, _Bool is_flexible) {
Initializer *init = calloc(1, sizeof(Initializer));
init->ty = ty;
if (ty->kind == TY_ARRAY) {
if (is_flexible && ty->size < 0) {
init->is_flexible = 1;
return init;
}
init->children = calloc(ty->array_len, sizeof(Initializer *));
for (int i = 0; i < ty->array_len; i++)
init->children[i] = new_initializer(ty->base, 0);
return init;
}
if (ty->kind == TY_STRUCT || ty->kind == TY_UNION) {
int len = 0;
for (Member *mem = ty->members; mem; mem = mem->next)
len++;
init->children = calloc(len, sizeof(Initializer *));
for (Member *mem = ty->members; mem; mem = mem->next) {
if (is_flexible && ty->is_flexible && !mem->next) {
Initializer *child = calloc(1, sizeof(Initializer));
child->ty = mem->ty;
child->is_flexible = 1;
init->children[mem->idx] = child;
} else {
init->children[mem->idx] = new_initializer(mem->ty, 0);
}
}
return init;
}
return init;
}
static Obj *new_var(char *name, Type *ty) {
Obj *var = calloc(1, sizeof(Obj));
var->name = name;
var->ty = ty;
var->align = ty->align;
push_scope(name)->var = var;
return var;
}
Obj *new_lvar(char *name, Type *ty) {
Obj *var = new_var(name, ty);
var->is_local = 1;
var->next = locals;
locals = var;
return var;
}
static Obj *new_gvar(char *name, Type *ty) {
Obj *var = new_var(name, ty);
var->next = globals;
var->is_static = 1;
var->is_definition = 1;
globals = var;
return var;
}
static char *new_unique_name(void) {
static int id = 0;
return format(".L..%d", id++);
}
static Obj *new_anon_gvar(Type *ty) {
return new_gvar(new_unique_name(), ty);
}
static HashMap str_pool;
static Obj *new_string_literal(char *p, Type *ty) {
if (ty && ty->kind == TY_ARRAY && ty->base == ty_char && ty->size > 0 && p) {
Obj *cached = hashmap_get2(&str_pool, p, ty->size);
if (cached)
return cached;
}
Obj *var = new_anon_gvar(ty);
var->init_data = p;
var->is_readonly = 1;
if (ty && ty->kind == TY_ARRAY && ty->base == ty_char && ty->size > 0 && p) {
hashmap_put2(&str_pool, p, ty->size, var);
}
return var;
}
static char *get_ident(Token *tok) {
if (tok->kind != TK_IDENT)
error_tok(tok, "expected an identifier");
return chibicc_strndup(tok->loc, tok->len);
}
static Type *find_typedef(Token *tok) {
if (tok->kind == TK_IDENT) {
VarScope *sc = find_var(tok);
if (sc)
return sc->type_def;
}
return ((void *)0);
}
static void push_tag_scope(Token *tok, Type *ty) {
hashmap_put2(&scope->tags, tok->loc, tok->len, ty);
}
static Token *skip_parentheses(Token *tok) {
int level = 1;
while (tok && level > 0) {
if (equal(tok, "("))
level++;
else if (equal(tok, ")"))
level--;
tok = tok->next;
}
return tok;
}
static void apply_attr_to_type(Type *ty, const VarAttr *attr) {
if (!attr || !ty)
return;
if (attr->is_packed)
ty->is_packed = 1;
if (attr->abi) {
if (ty->kind == TY_FUNC)
ty->abi = attr->abi;
else if (ty->kind == TY_PTR && ty->base && ty->base->kind == TY_FUNC)
ty->base->abi = attr->abi;
}
}
static _Bool is_attribute_token(Token *tok) {
if (!tok)
return 0;
static HashMap map;
if (map.capacity == 0) {
static char *kw[] = {
"__attribute__", "__attribute", "__declspec", "__callingconv",
"__cdecl", "__stdcall", "__fastcall", "__thiscall", "__vectorcall",
"_cdecl", "_stdcall", "_fastcall", "__pascal", "pascal",
"__ms_abi", "__sysv_abi", "__forceinline", "__inline", "__inline__",
"__gnu_inline", "__gnu_inline__", "gnu_inline",
"__restrict", "__restrict__", "__ptr32", "__ptr64", "__unaligned", "__w64",
};
for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++)
hashmap_put(&map, kw[i], (void *)1);
}
return hashmap_get2(&map, tok->loc, tok->len);
}
static _Bool attr_is(Token *tok, const char *name) {
if (!tok)
return 0;
char *loc = tok->loc;
int len = tok->len;
if (len > 2 && loc[0] == '_' && loc[1] == '_') {
loc += 2;
len -= 2;
} else if (len > 1 && loc[0] == '_') {
loc += 1;
len -= 1;
}
if (len > 2 && loc[len - 2] == '_' && loc[len - 1] == '_')
len -= 2;
else if (len > 1 && loc[len - 1] == '_')
len -= 1;
return (int)strlen(name) == len && strncmp(loc, name, len) == 0;
}
static _Bool parse_single_attribute(Token **rest, Token *tok, VarAttr *attr) {
if (attr_is(tok, "packed")) {
if (attr) attr->is_packed = 1;
*rest = tok->next;
return 1;
}
if (attr_is(tok, "aligned") || attr_is(tok, "align")) {
tok = tok->next;
if (equal(tok, "(")) {
tok = tok->next;
int align = (int)const_expr(&tok, tok);
if (attr) attr->align = align;
tok = skip(tok, ")");
}
*rest = tok;
return 1;
}
if (attr_is(tok, "callingconv")) {
tok = skip(tok->next, "(");
char name[64] = {0};
if (tok->kind == TK_STR) {
strncpy(name, tok->str, sizeof(name) - 1);
} else if (tok->kind == TK_IDENT) {
int len = tok->len < (int)sizeof(name) - 1 ? tok->len : (int)sizeof(name) - 1;
strncpy(name, tok->loc, len);
}
tok = tok->next;
tok = skip(tok, ")");
if (attr) attr->abi = get_abi(name);
*rest = tok;
return 1;
}
if (attr_is(tok, "ms_abi")) {
if (attr) attr->abi = get_abi("win64");
*rest = tok->next;
return 1;
}
if (attr_is(tok, "sysv_abi")) {
if (attr) attr->abi = get_abi("sysv64");
*rest = tok->next;
return 1;
}
if (attr_is(tok, "pascal")) {
if (attr) attr->abi = get_abi("pascal");
*rest = tok->next;
return 1;
}
if (attr_is(tok, "cdecl") || attr_is(tok, "stdcall") ||
attr_is(tok, "fastcall") || attr_is(tok, "thiscall") || attr_is(tok, "vectorcall")) {
if (current_abi && current_abi->size_ptr == 4) {
if (attr) attr->abi = get_abi("win32");
}
*rest = tok->next;
return 1;
}
if (attr_is(tok, "always_inline") || attr_is(tok, "inline") || attr_is(tok, "forceinline") || attr_is(tok, "gnu_inline")) {
if (attr) attr->is_inline = 1;
*rest = tok->next;
return 1;
}
if (attr_is(tok, "restrict") || attr_is(tok, "ptr32") || attr_is(tok, "ptr64") ||
attr_is(tok, "unaligned") || attr_is(tok, "w64")) {
*rest = tok->next;
return 1;
}
return 0;
}
static Token *consume_attributes(Token *tok, VarAttr *attr) {
while (tok && (is_attribute_token(tok) || equal(tok, "asm") || equal(tok, "__asm__") || equal(tok, "__asm"))) {
if (equal(tok, "asm") || equal(tok, "__asm__") || equal(tok, "__asm")) {
tok = tok->next;
while (equal(tok, "volatile") || equal(tok, "__volatile") || equal(tok, "__volatile__") ||
equal(tok, "inline") || equal(tok, "__inline") || equal(tok, "__inline__") ||
equal(tok, "goto"))
tok = tok->next;
if (equal(tok, "(")) {
tok = tok->next;
if (tok->kind == TK_STR) {
if (attr)
attr->asm_name = tok->str;
tok = tok->next;
if (equal(tok, ")"))
tok = tok->next;
} else {
tok = skip_parentheses(tok);
}
}
continue;
}
if (equal(tok, "__callingconv")) {
tok = skip(tok->next, "(");
char name[64] = {0};
if (tok->kind == TK_STR) {
strncpy(name, tok->str, sizeof(name) - 1);
} else if (tok->kind == TK_IDENT) {
int len = tok->len < (int)sizeof(name) - 1 ? tok->len : (int)sizeof(name) - 1;
strncpy(name, tok->loc, len);
}
tok = tok->next;
tok = skip(tok, ")");
if (attr) attr->abi = get_abi(name);
continue;
}
if (equal(tok, "__attribute__") || equal(tok, "__attribute")) {
tok = tok->next;
while (equal(tok, "("))
tok = tok->next;
while (tok && !equal(tok, ")")) {
if (consume(&tok, tok, ","))
continue;
if (!parse_single_attribute(&tok, tok, attr)) {
tok = tok->next;
if (equal(tok, "(")) {
tok = tok->next;
tok = skip_parentheses(tok);
}
}
}
while (equal(tok, ")"))
tok = tok->next;
continue;
}
if (equal(tok, "__declspec")) {
tok = tok->next;
if (equal(tok, "(")) {
tok = tok->next;
while (tok && !equal(tok, ")")) {
if (consume(&tok, tok, ","))
continue;
if (!parse_single_attribute(&tok, tok, attr)) {
tok = tok->next;
if (equal(tok, "(")) {
tok = tok->next;
tok = skip_parentheses(tok);
}
}
}
if (equal(tok, ")"))
tok = tok->next;
}
continue;
}
Token *next_tok = tok;
if (parse_single_attribute(&next_tok, tok, attr)) {
tok = next_tok;
continue;
}
tok = tok->next;
}
return tok;
}
static Type *declspec(Token **rest, Token *tok, VarAttr *attr) {
enum {
VOID = 1 << 0,
BOOL = 1 << 2,
CHAR = 1 << 4,
SHORT = 1 << 6,
INT = 1 << 8,
LONG = 1 << 10,
FLOAT = 1 << 12,
DOUBLE = 1 << 14,
OTHER = 1 << 16,
SIGNED = 1 << 17,
UNSIGNED = 1 << 18,
};
Type *ty = ty_int;
int counter = 0;
_Bool is_atomic = 0;
while (is_typename(tok)) {
if (is_attribute_token(tok)) {
tok = consume_attributes(tok, attr);
continue;
}
if (equal(tok, "typedef") || equal(tok, "static") || equal(tok, "extern") ||
equal(tok, "inline") || equal(tok, "_Thread_local") || equal(tok, "__thread")) {
if (!attr)
error_tok(tok, "storage class specifier is not allowed in this context");
if (equal(tok, "typedef"))
attr->is_typedef = 1;
else if (equal(tok, "static"))
attr->is_static = 1;
else if (equal(tok, "extern"))
attr->is_extern = 1;
else if (equal(tok, "inline"))
attr->is_inline = 1;
else
attr->is_tls = 1;
if (attr->is_typedef &&
attr->is_static + attr->is_extern + attr->is_inline + attr->is_tls > 1)
error_tok(tok, "typedef may not be used together with static,");
tok = tok->next;
continue;
}
if (equal(tok, "const") || equal(tok, "__const") || equal(tok, "__const__")) {
if (attr)
attr->is_const = 1;
tok = tok->next;
continue;
}
if (consume(&tok, tok, "volatile") || consume(&tok, tok, "__volatile") || consume(&tok, tok, "__volatile__") ||
consume(&tok, tok, "auto") || consume(&tok, tok, "register") ||
consume(&tok, tok, "restrict") || consume(&tok, tok, "__restrict") || consume(&tok, tok, "__restrict__") ||
consume(&tok, tok, "_Noreturn") ||
consume(&tok, tok, "__extension__") || consume(&tok, tok, "__extension"))
continue;
if (equal(tok, "_Atomic")) {
tok = tok->next;
if (equal(tok , "(")) {
ty = typename(&tok, tok->next);
tok = skip(tok, ")");
}
is_atomic = 1;
continue;
}
if (equal(tok, "_Alignas")) {
if (!attr)
error_tok(tok, "_Alignas is not allowed in this context");
tok = skip(tok->next, "(");
if (is_typename(tok))
attr->align = typename(&tok, tok)->align;
else
attr->align = const_expr(&tok, tok);
tok = skip(tok, ")");
continue;
}
Type *ty2 = find_typedef(tok);
if (equal(tok, "struct") || equal(tok, "union") || equal(tok, "enum") ||
equal(tok, "typeof") || equal(tok, "__typeof") || equal(tok, "__typeof__") || ty2) {
if (counter)
break;
if (equal(tok, "struct")) {
ty = struct_decl(&tok, tok->next);
} else if (equal(tok, "union")) {
ty = union_decl(&tok, tok->next);
} else if (equal(tok, "enum")) {
ty = enum_specifier(&tok, tok->next);
} else if (equal(tok, "typeof") || equal(tok, "__typeof") || equal(tok, "__typeof__")) {
ty = typeof_specifier(&tok, tok->next);
} else {
ty = ty2;
tok = tok->next;
}
counter += OTHER;
continue;
}
if (equal(tok, "void"))
counter += VOID;
else if (equal(tok, "_Bool"))
counter += BOOL;
else if (equal(tok, "char") || equal(tok, "__int8"))
counter += CHAR;
else if (equal(tok, "short") || equal(tok, "__int16"))
counter += SHORT;
else if (equal(tok, "int") || equal(tok, "__int32"))
counter += INT;
else if (equal(tok, "long"))
counter += LONG;
else if (equal(tok, "__int64"))
counter += LONG + LONG;
else if (equal(tok, "float"))
counter += FLOAT;
else if (equal(tok, "double"))
counter += DOUBLE;
else if (equal(tok, "signed") || equal(tok, "__signed") || equal(tok, "__signed__"))
counter |= SIGNED;
else if (equal(tok, "unsigned") || equal(tok, "__unsigned") || equal(tok, "__unsigned__"))
counter |= UNSIGNED;
else
error("internal error at %s:%d",
"parse.c",
1034);
switch (counter) {
case VOID:
ty = ty_void;
break;
case BOOL:
ty = ty_bool;
break;
case CHAR:
case SIGNED + CHAR:
ty = ty_char;
break;
case UNSIGNED + CHAR:
ty = ty_uchar;
break;
case SHORT:
case SHORT + INT:
case SIGNED + SHORT:
case SIGNED + SHORT + INT:
ty = ty_short;
break;
case UNSIGNED + SHORT:
case UNSIGNED + SHORT + INT:
ty = ty_ushort;
break;
case INT:
case SIGNED:
case SIGNED + INT:
ty = ty_int;
break;
case UNSIGNED:
case UNSIGNED + INT:
ty = ty_uint;
break;
case LONG:
case LONG + INT:
case SIGNED + LONG:
case SIGNED + LONG + INT:
ty = ty_long;
break;
case LONG + LONG:
case LONG + LONG + INT:
case SIGNED + LONG + LONG:
case SIGNED + LONG + LONG + INT:
ty = ty_llong;
break;
case UNSIGNED + LONG:
case UNSIGNED + LONG + INT:
ty = ty_ulong;
break;
case UNSIGNED + LONG + LONG:
case UNSIGNED + LONG + LONG + INT:
ty = ty_ullong;
break;
case FLOAT:
ty = ty_float;
break;
case DOUBLE:
ty = ty_double;
break;
case LONG + DOUBLE:
ty = ty_ldouble;
break;
default:
error_tok(tok, "invalid type");
}
tok = tok->next;
}
if (is_atomic) {
ty = copy_type(ty);
ty->is_atomic = 1;
}
*rest = tok;
return ty;
}
static Type *func_params(Token **rest, Token *tok, Type *ty) {
if (equal(tok, "void") && equal(tok->next, ")")) {
*rest = tok->next->next;
return func_type(ty);
}
Type head = {};
Type *cur = &head;
_Bool is_variadic = 0;
while (!equal(tok, ")")) {
if (cur != &head)
tok = skip(tok, ",");
if (equal(tok, "...")) {
is_variadic = 1;
tok = tok->next;
skip(tok, ")");
break;
}
Type *ty2 = declspec(&tok, tok, ((void *)0));
ty2 = declarator(&tok, tok, ty2);
Token *name = ty2->name;
if (ty2->kind == TY_ARRAY) {
ty2 = pointer_to(ty2->base);
ty2->name = name;
} else if (ty2->kind == TY_FUNC) {
ty2 = pointer_to(ty2);
ty2->name = name;
}
cur = cur->next = copy_type(ty2);
}
if (cur == &head)
is_variadic = 1;
ty = func_type(ty);
ty->params = head.next;
ty->is_variadic = is_variadic;
*rest = tok->next;
return ty;
}
static Type *array_dimensions(Token **rest, Token *tok, Type *ty) {
while (equal(tok, "static") || equal(tok, "restrict"))
tok = tok->next;
if (equal(tok, "]")) {
ty = type_suffix(rest, tok->next, ty);
return array_of(ty, -1);
}
Node *expr = conditional(&tok, tok);
tok = skip(tok, "]");
ty = type_suffix(rest, tok, ty);
if (ty->kind == TY_VLA || !is_const_expr(expr))
return vla_of(ty, expr);
return array_of(ty, eval(expr));
}
static Type *type_suffix(Token **rest, Token *tok, Type *ty) {
if (equal(tok, "("))
return func_params(rest, tok->next, ty);
if (equal(tok, "["))
return array_dimensions(rest, tok->next, ty);
*rest = tok;
return ty;
}
static Type *pointers(Token **rest, Token *tok, Type *ty) {
while (consume(&tok, tok, "*")) {
ty = pointer_to(ty);
while (equal(tok, "const") || equal(tok, "volatile") || equal(tok, "restrict") || is_attribute_token(tok)) {
if (is_attribute_token(tok))
tok = consume_attributes(tok, ((void *)0));
else
tok = tok->next;
}
}
*rest = tok;
return ty;
}
static Type *declarator(Token **rest, Token *tok, Type *ty) {
VarAttr attr = {};
tok = consume_attributes(tok, &attr);
ty = pointers(&tok, tok, ty);
tok = consume_attributes(tok, &attr);
if (equal(tok, "(")) {
Token *start = tok;
Type dummy = {};
declarator(&tok, start->next, &dummy);
tok = skip(tok, ")");
ty = type_suffix(rest, tok, ty);
apply_attr_to_type(ty, &attr);
return declarator(&tok, start->next, ty);
}
Token *name = ((void *)0);
Token *name_pos = tok;
if (tok->kind == TK_IDENT) {
name = tok;
tok = tok->next;
}
tok = consume_attributes(tok, &attr);
ty = type_suffix(rest, tok, ty);
ty->name = name;
ty->name_pos = name_pos;
apply_attr_to_type(ty, &attr);
return ty;
}
static Type *abstract_declarator(Token **rest, Token *tok, Type *ty) {
VarAttr attr = {};
tok = consume_attributes(tok, &attr);
ty = pointers(&tok, tok, ty);
tok = consume_attributes(tok, &attr);
if (equal(tok, "(")) {
Token *start = tok;
Type dummy = {};
abstract_declarator(&tok, start->next, &dummy);
tok = skip(tok, ")");
ty = type_suffix(rest, tok, ty);
apply_attr_to_type(ty, &attr);
return abstract_declarator(&tok, start->next, ty);
}
tok = consume_attributes(tok, &attr);
ty = type_suffix(rest, tok, ty);
apply_attr_to_type(ty, &attr);
return ty;
}
static Type *typename(Token **rest, Token *tok) {
Type *ty = declspec(&tok, tok, ((void *)0));
return abstract_declarator(rest, tok, ty);
}
static _Bool is_end(Token *tok) {
return equal(tok, "}") || (equal(tok, ",") && equal(tok->next, "}"));
}
static _Bool consume_end(Token **rest, Token *tok) {
if (equal(tok, "}")) {
*rest = tok->next;
return 1;
}
if (equal(tok, ",") && equal(tok->next, "}")) {
*rest = tok->next->next;
return 1;
}
return 0;
}
static Type *enum_specifier(Token **rest, Token *tok) {
Type *ty = enum_type();
VarAttr attr = {};
tok = consume_attributes(tok, &attr);
apply_attr_to_type(ty, &attr);
Token *tag = ((void *)0);
if (tok->kind == TK_IDENT) {
tag = tok;
tok = tok->next;
}
VarAttr attr_mid = {};
tok = consume_attributes(tok, &attr_mid);
apply_attr_to_type(ty, &attr_mid);
if (tag && !equal(tok, "{")) {
Type *ty2 = find_tag(tag);
if (!ty2)
error_tok(tag, "unknown enum type");
if (ty2->kind != TY_ENUM)
error_tok(tag, "not an enum tag");
*rest = tok;
return ty2;
}
tok = skip(tok, "{");
int i = 0;
int val = 0;
while (!consume_end(rest, tok)) {
if (i++ > 0)
tok = skip(tok, ",");
Token *tok_id = tok;
char *name = get_ident(tok);
tok = tok->next;
VarScope *sc_exist = hashmap_get2(&scope->vars, tok_id->loc, tok_id->len);
if (sc_exist && (sc_exist->var || sc_exist->enum_ty))
error_tok(tok_id, "redefinition of enumerator '%s'", name);
if (equal(tok, "="))
val = const_expr(&tok, tok->next);
VarScope *sc = push_scope(name);
sc->enum_ty = ty;
sc->enum_val = val++;
}
VarAttr attr_post = {};
*rest = consume_attributes(*rest, &attr_post);
apply_attr_to_type(ty, &attr_post);
if (tag) {
Type *ty2 = hashmap_get2(&scope->tags, tag->loc, tag->len);
if (ty2)
error_tok(tag, "redefinition of enum '%s'", get_ident(tag));
push_tag_scope(tag, ty);
}
return ty;
}
static Type *typeof_specifier(Token **rest, Token *tok) {
tok = skip(tok, "(");
Type *ty;
if (is_typename(tok)) {
ty = typename(&tok, tok);
} else {
Node *node = expr(&tok, tok);
add_type(node);
ty = node->ty;
}
*rest = skip(tok, ")");
return ty;
}
static Node *compute_vla_size(Type *ty, Token *tok) {
Node *node = new_node(ND_NULL_EXPR, tok);
if (ty->base)
node = new_binary(ND_COMMA, node, compute_vla_size(ty->base, tok), tok);
if (ty->kind != TY_VLA)
return node;
Node *base_sz;
if (ty->base->kind == TY_VLA)
base_sz = new_var_node(ty->base->vla_size, tok);
else
base_sz = new_num(ty->base->size, tok);
ty->vla_size = new_lvar("", ty_ulong);
Node *expr = new_binary(ND_ASSIGN, new_var_node(ty->vla_size, tok),
new_binary(ND_MUL, ty->vla_len, base_sz, tok),
tok);
return new_binary(ND_COMMA, node, expr, tok);
}
static Node *new_alloca(Node *sz) {
Node *node = new_unary(ND_FUNCALL, new_var_node(builtin_alloca, sz->tok), sz->tok);
node->func_ty = builtin_alloca->ty;
node->ty = builtin_alloca->ty->return_ty;
node->args = sz;
add_type(sz);
return node;
}
static Node *declaration(Token **rest, Token *tok, Type *basety, VarAttr *attr) {
Node head = {};
Node *cur = &head;
int i = 0;
while (!equal(tok, ";")) {
if (i++ > 0)
tok = skip(tok, ",");
Type *ty = declarator(&tok, tok, basety);
if (ty->kind == TY_VOID)
error_tok(tok, "variable declared void");
if (!ty->name)
error_tok(ty->name_pos, "variable name omitted");
tok = consume_attributes(tok, attr);
apply_attr_to_type(ty, attr);
char *name = get_ident(ty->name);
VarScope *sc = hashmap_get2(&scope->vars, ty->name->loc, ty->name->len);
if (sc && sc->var)
error_tok(ty->name, "redefinition of '%s'", name);
if (attr && attr->is_static) {
Obj *var = new_anon_gvar(ty);
push_scope(name)->var = var;
if (equal(tok, "="))
gvar_initializer(&tok, tok->next, var);
continue;
}
cur = cur->next = new_unary(ND_EXPR_STMT, compute_vla_size(ty, tok), tok);
if (ty->kind == TY_VLA) {
if (equal(tok, "="))
error_tok(tok, "variable-sized object may not be initialized");
Obj *var = new_lvar(name, ty);
Token *tok = ty->name;
Node *expr = new_binary(ND_ASSIGN, new_vla_ptr(var, tok),
new_alloca(new_var_node(ty->vla_size, tok)),
tok);
cur = cur->next = new_unary(ND_EXPR_STMT, expr, tok);
continue;
}
Obj *var = new_lvar(name, ty);
if (attr && attr->asm_name)
var->name = attr->asm_name;
if (attr && attr->align)
var->align = attr->align;
if (equal(tok, "=")) {
Node *expr = lvar_initializer(&tok, tok->next, var);
cur = cur->next = new_unary(ND_EXPR_STMT, expr, tok);
}
if (var->ty->size < 0)
error_tok(ty->name, "variable has incomplete type");
if (var->ty->kind == TY_VOID)
error_tok(ty->name, "variable declared void");
}
Node *node = new_node(ND_BLOCK, tok);
node->body = head.next;
*rest = tok->next;
return node;
}
static Token *skip_excess_element(Token *tok) {
if (equal(tok, "{")) {
tok = skip_excess_element(tok->next);
return skip(tok, "}");
}
assign(&tok, tok);
return tok;
}
static void string_initializer(Token **rest, Token *tok, Initializer *init) {
if (init->is_flexible)
*init = *new_initializer(array_of(init->ty->base, tok->ty->array_len), 0);
int len = ((init->ty->array_len) < (tok->ty->array_len) ? (init->ty->array_len) : (tok->ty->array_len));
switch (init->ty->base->size) {
case 1: {
char *str = tok->str;
for (int i = 0; i < len; i++)
init->children[i]->expr = new_num(str[i], tok);
break;
}
case 2: {
uint16_t *str = (uint16_t *)tok->str;
for (int i = 0; i < len; i++)
init->children[i]->expr = new_num(str[i], tok);
break;
}
case 4: {
uint32_t *str = (uint32_t *)tok->str;
for (int i = 0; i < len; i++)
init->children[i]->expr = new_num(str[i], tok);
break;
}
default:
error("internal error at %s:%d",
"parse.c",
1533);
}
*rest = tok->next;
}
static void array_designator(Token **rest, Token *tok, Type *ty, int *begin, int *end) {
*begin = const_expr(&tok, tok->next);
if (*begin >= ty->array_len)
error_tok(tok, "array designator index exceeds array bounds");
if (equal(tok, "...")) {
*end = const_expr(&tok, tok->next);
if (*end >= ty->array_len)
error_tok(tok, "array designator index exceeds array bounds");
if (*end < *begin)
error_tok(tok, "array designator range [%d, %d] is empty", *begin, *end);
} else {
*end = *begin;
}
*rest = skip(tok, "]");
}
static Member *struct_designator(Token **rest, Token *tok, Type *ty) {
Token *start = tok;
tok = skip(tok, ".");
if (tok->kind != TK_IDENT)
error_tok(tok, "expected a field designator");
for (Member *mem = ty->members; mem; mem = mem->next) {
if ((mem->ty->kind == TY_STRUCT || mem->ty->kind == TY_UNION) && !mem->name) {
if (get_struct_member(mem->ty, tok)) {
*rest = start;
return mem;
}
continue;
}
if (mem->name && mem->name->len == tok->len && !strncmp(mem->name->loc, tok->loc, tok->len)) {
*rest = tok->next;
return mem;
}
}
error_tok(tok, "struct has no such member");
}
static void designation(Token **rest, Token *tok, Initializer *init) {
if (equal(tok, "[")) {
if (init->ty->kind != TY_ARRAY)
error_tok(tok, "array index in non-array initializer");
int begin, end;
array_designator(&tok, tok, init->ty, &begin, &end);
Token *tok2;
for (int i = begin; i <= end; i++)
designation(&tok2, tok, init->children[i]);
array_initializer2(rest, tok2, init, begin + 1);
return;
}
if (equal(tok, ".") && init->ty->kind == TY_STRUCT) {
Member *mem = struct_designator(&tok, tok, init->ty);
designation(&tok, tok, init->children[mem->idx]);
init->expr = ((void *)0);
struct_initializer2(rest, tok, init, mem->next);
return;
}
if (equal(tok, ".") && init->ty->kind == TY_UNION) {
Member *mem = struct_designator(&tok, tok, init->ty);
init->mem = mem;
designation(rest, tok, init->children[mem->idx]);
return;
}
if (equal(tok, "."))
error_tok(tok, "field name not in struct or union initializer");
if (equal(tok, "="))
tok = tok->next;
initializer2(rest, tok, init);
}
static int count_array_init_elements(Token *tok, Type *ty) {
_Bool first = 1;
Initializer *dummy = new_initializer(ty->base, 1);
int i = 0, max = 0;
while (!consume_end(&tok, tok)) {
if (!first)
tok = skip(tok, ",");
first = 0;
if (equal(tok, "[")) {
i = const_expr(&tok, tok->next);
if (equal(tok, "..."))
i = const_expr(&tok, tok->next);
tok = skip(tok, "]");
designation(&tok, tok, dummy);
} else {
initializer2(&tok, tok, dummy);
}
i++;
max = ((max) < (i) ? (i) : (max));
}
return max;
}
static void array_initializer1(Token **rest, Token *tok, Initializer *init) {
tok = skip(tok, "{");
if (init->is_flexible) {
int len = count_array_init_elements(tok, init->ty);
*init = *new_initializer(array_of(init->ty->base, len), 0);
}
_Bool first = 1;
if (init->is_flexible) {
int len = count_array_init_elements(tok, init->ty);
*init = *new_initializer(array_of(init->ty->base, len), 0);
}
for (int i = 0; !consume_end(rest, tok); i++) {
if (!first)
tok = skip(tok, ",");
first = 0;
if (equal(tok, "[")) {
int begin, end;
array_designator(&tok, tok, init->ty, &begin, &end);
Token *tok2;
for (int j = begin; j <= end; j++)
designation(&tok2, tok, init->children[j]);
tok = tok2;
i = end;
continue;
}
if (i < init->ty->array_len)
initializer2(&tok, tok, init->children[i]);
else
tok = skip_excess_element(tok);
}
}
static void array_initializer2(Token **rest, Token *tok, Initializer *init, int i) {
if (init->is_flexible) {
int len = count_array_init_elements(tok, init->ty);
*init = *new_initializer(array_of(init->ty->base, len), 0);
}
_Bool first = 1;
for (; i < init->ty->array_len && !is_end(tok); i++) {
Token *start = tok;
if (!first || equal(tok, ","))
tok = skip(tok, ",");
first = 0;
if (equal(tok, "[") || equal(tok, ".")) {
*rest = start;
return;
}
initializer2(&tok, tok, init->children[i]);
}
*rest = tok;
}
static void struct_initializer1(Token **rest, Token *tok, Initializer *init) {
tok = skip(tok, "{");
Member *mem = init->ty->members;
_Bool first = 1;
while (!consume_end(rest, tok)) {
if (!first)
tok = skip(tok, ",");
first = 0;
if (equal(tok, ".")) {
mem = struct_designator(&tok, tok, init->ty);
designation(&tok, tok, init->children[mem->idx]);
mem = mem->next;
continue;
}
if (mem) {
initializer2(&tok, tok, init->children[mem->idx]);
mem = mem->next;
} else {
tok = skip_excess_element(tok);
}
}
}
static void struct_initializer2(Token **rest, Token *tok, Initializer *init, Member *mem) {
_Bool first = 1;
for (; mem && !is_end(tok); mem = mem->next) {
Token *start = tok;
if (!first || equal(tok, ","))
tok = skip(tok, ",");
first = 0;
if (equal(tok, "[") || equal(tok, ".")) {
*rest = start;
return;
}
initializer2(&tok, tok, init->children[mem->idx]);
}
*rest = tok;
}
static void union_initializer(Token **rest, Token *tok, Initializer *init) {
if (equal(tok, "{") && equal(tok->next, ".")) {
Member *mem = struct_designator(&tok, tok->next, init->ty);
init->mem = mem;
designation(&tok, tok, init->children[mem->idx]);
*rest = skip(tok, "}");
return;
}
init->mem = init->ty->members;
if (equal(tok, "{")) {
initializer2(&tok, tok->next, init->children[0]);
consume(&tok, tok, ",");
*rest = skip(tok, "}");
} else {
initializer2(rest, tok, init->children[0]);
}
}
static void initializer2(Token **rest, Token *tok, Initializer *init) {
if (init->ty->kind == TY_ARRAY && tok->kind == TK_STR) {
string_initializer(rest, tok, init);
return;
}
if (init->ty->kind == TY_ARRAY) {
if (equal(tok, "{"))
array_initializer1(rest, tok, init);
else
array_initializer2(rest, tok, init, 0);
return;
}
if (init->ty->kind == TY_STRUCT) {
if (equal(tok, "{")) {
struct_initializer1(rest, tok, init);
return;
}
Node *expr = assign(rest, tok);
add_type(expr);
if (expr->ty->kind == TY_STRUCT) {
init->expr = expr;
return;
}
struct_initializer2(rest, tok, init, init->ty->members);
return;
}
if (init->ty->kind == TY_UNION) {
if (equal(tok, "{")) {
union_initializer(rest, tok, init);
return;
}
Node *expr = assign(rest, tok);
add_type(expr);
if (expr->ty->kind == TY_UNION) {
init->expr = expr;
return;
}
init->mem = init->ty->members;
init->children[0]->expr = expr;
return;
}
if (equal(tok, "{")) {
initializer2(&tok, tok->next, init);
*rest = skip(tok, "}");
return;
}
init->expr = assign(rest, tok);
}
static Type *copy_struct_type(Type *ty) {
ty = copy_type(ty);
Member head = {};
Member *cur = &head;
for (Member *mem = ty->members; mem; mem = mem->next) {
Member *m = calloc(1, sizeof(Member));
*m = *mem;
cur = cur->next = m;
}
ty->members = head.next;
return ty;
}
static Initializer *initializer(Token **rest, Token *tok, Type *ty, Type **new_ty) {
Initializer *init = new_initializer(ty, 1);
initializer2(rest, tok, init);
if ((ty->kind == TY_STRUCT || ty->kind == TY_UNION) && ty->is_flexible) {
ty = copy_struct_type(ty);
Member *mem = ty->members;
while (mem->next)
mem = mem->next;
mem->ty = init->children[mem->idx]->ty;
ty->size += mem->ty->size;
*new_ty = ty;
return init;
}
*new_ty = init->ty;
return init;
}
static Node *init_desg_expr(InitDesg *desg, Token *tok) {
if (desg->var)
return new_var_node(desg->var, tok);
if (desg->member) {
Node *node = new_unary(ND_MEMBER, init_desg_expr(desg->next, tok), tok);
node->member = desg->member;
return node;
}
Node *lhs = init_desg_expr(desg->next, tok);
Node *rhs = new_num(desg->idx, tok);
return new_unary(ND_DEREF, new_add(lhs, rhs, tok), tok);
}
static Node *create_lvar_init(Initializer *init, Type *ty, InitDesg *desg, Token *tok) {
if (ty->kind == TY_ARRAY) {
Node *node = new_node(ND_NULL_EXPR, tok);
for (int i = 0; i < ty->array_len; i++) {
InitDesg desg2 = {desg, i};
Node *rhs = create_lvar_init(init->children[i], ty->base, &desg2, tok);
node = new_binary(ND_COMMA, node, rhs, tok);
}
return node;
}
if (ty->kind == TY_STRUCT && !init->expr) {
Node *node = new_node(ND_NULL_EXPR, tok);
for (Member *mem = ty->members; mem; mem = mem->next) {
InitDesg desg2 = {desg, 0, mem};
Node *rhs = create_lvar_init(init->children[mem->idx], mem->ty, &desg2, tok);
node = new_binary(ND_COMMA, node, rhs, tok);
}
return node;
}
if (ty->kind == TY_UNION && !init->expr) {
Member *mem = init->mem ? init->mem : ty->members;
InitDesg desg2 = {desg, 0, mem};
return create_lvar_init(init->children[mem->idx], mem->ty, &desg2, tok);
}
if (!init->expr)
return new_node(ND_NULL_EXPR, tok);
Node *lhs = init_desg_expr(desg, tok);
return new_binary(ND_ASSIGN, lhs, init->expr, tok);
}
static Node *lvar_initializer(Token **rest, Token *tok, Obj *var) {
Initializer *init = initializer(rest, tok, var->ty, &var->ty);
InitDesg desg = {((void *)0), 0, ((void *)0), var};
Node *lhs = new_node(ND_MEMZERO, tok);
lhs->var = var;
Node *rhs = create_lvar_init(init, var->ty, &desg, tok);
return new_binary(ND_COMMA, lhs, rhs, tok);
}
static uint64_t read_buf(char *buf, int sz) {
if (sz == 1)
return *buf;
if (sz == 2)
return *(uint16_t *)buf;
if (sz == 4)
return *(uint32_t *)buf;
if (sz == 8)
return *(uint64_t *)buf;
error("internal error at %s:%d",
"parse.c",
2003);
}
static void write_buf(char *buf, uint64_t val, int sz) {
if (sz == 1)
*buf = val;
else if (sz == 2)
*(uint16_t *)buf = val;
else if (sz == 4)
*(uint32_t *)buf = val;
else if (sz == 8)
*(uint64_t *)buf = val;
else
error("internal error at %s:%d",
"parse.c",
2016);
}
static Relocation *
write_gvar_data(Relocation *cur, Initializer *init, Type *ty, char *buf, int offset) {
if (ty->kind == TY_ARRAY) {
int sz = ty->base->size;
for (int i = 0; i < ty->array_len; i++)
cur = write_gvar_data(cur, init->children[i], ty->base, buf, offset + sz * i);
return cur;
}
if (ty->kind == TY_STRUCT) {
for (Member *mem = ty->members; mem; mem = mem->next) {
if (mem->is_bitfield) {
Node *expr = init->children[mem->idx]->expr;
if (!expr)
break;
char *loc = buf + offset + mem->offset;
uint64_t oldval = read_buf(loc, mem->ty->size);
uint64_t newval = eval(expr);
uint64_t mask = (1L << mem->bit_width) - 1;
uint64_t combined = oldval | ((newval & mask) << mem->bit_offset);
write_buf(loc, combined, mem->ty->size);
} else {
cur = write_gvar_data(cur, init->children[mem->idx], mem->ty, buf,
offset + mem->offset);
}
}
return cur;
}
if (ty->kind == TY_UNION) {
if (!init->mem)
return cur;
return write_gvar_data(cur, init->children[init->mem->idx],
init->mem->ty, buf, offset);
}
if (!init->expr)
return cur;
if (ty->kind == TY_FLOAT) {
*(float *)(buf + offset) = eval_double(init->expr);
return cur;
}
if (ty->kind == TY_DOUBLE) {
*(double *)(buf + offset) = eval_double(init->expr);
return cur;
}
char **label = ((void *)0);
uint64_t val = eval2(init->expr, &label);
if (!label) {
write_buf(buf + offset, val, ty->size);
return cur;
}
Relocation *rel = calloc(1, sizeof(Relocation));
rel->offset = offset;
rel->label = label;
rel->addend = val;
cur->next = rel;
return cur->next;
}
static void gvar_initializer(Token **rest, Token *tok, Obj *var) {
Initializer *init = initializer(rest, tok, var->ty, &var->ty);
Relocation head = {};
char *buf = calloc(1, var->ty->size);
write_gvar_data(&head, init, var->ty, buf, 0);
var->init_data = buf;
var->rel = head.next;
}
static _Bool is_typename(Token *tok) {
static HashMap map;
if (map.capacity == 0) {
static char *kw[] = {
"void", "_Bool", "char", "short", "int", "long", "struct", "union",
"typedef", "enum", "static", "extern", "_Alignas", "signed", "__signed", "__signed__",
"unsigned", "__unsigned", "__unsigned__",
"const", "__const", "__const__", "volatile", "__volatile", "__volatile__",
"auto", "register", "restrict", "__restrict",
"__restrict__", "_Noreturn", "float", "double", "typeof", "__typeof", "__typeof__",
"inline", "__inline", "__inline__",
"_Thread_local", "__thread", "_Atomic",
"__int8", "__int16", "__int32", "__int64",
"__extension__", "__extension",
};
for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++)
hashmap_put(&map, kw[i], (void *)1);
}
return hashmap_get2(&map, tok->loc, tok->len) || find_typedef(tok) || is_attribute_token(tok);
}
static char *format_asm_dialect(const char *s) {
if (!s || !strchr(s, '{'))
return (char *)s;
char *buf = calloc(strlen(s) + 1, 1);
char *d = buf;
const char *p = s;
while (*p) {
if (*p == '{') {
p++;
while (*p && *p != '|' && *p != '}')
*d++ = *p++;
while (*p && *p != '}')
p++;
if (*p == '}')
p++;
} else {
*d++ = *p++;
}
}
return buf;
}
static Node *asm_stmt(Token **rest, Token *tok) {
Token *start = tok;
tok = tok->next;
while (equal(tok, "volatile") || equal(tok, "inline") ||
equal(tok, "__volatile__") || equal(tok, "__volatile") ||
equal(tok, "__inline__") || equal(tok, "__inline") ||
equal(tok, "goto"))
tok = tok->next;
tok = skip(tok, "(");
if (tok->kind != TK_STR || tok->ty->base->kind != TY_CHAR)
error_tok(tok, "expected string literal");
char *asm_str = format_asm_dialect(tok->str);
tok = tok->next;
while (tok && !equal(tok, ")")) {
if (equal(tok, "(")) {
tok = skip_parentheses(tok->next);
continue;
}
tok = tok->next;
}
tok = skip(tok, ")");
consume(&tok, tok, ";");
*rest = tok;
return new_asm_node(asm_str, start);
}
static Node *stmt(Token **rest, Token *tok) {
if (equal(tok, "return")) {
Node *node = new_return_node(((void *)0), tok);
if (consume(rest, tok->next, ";"))
return node;
Node *exp = expr(&tok, tok->next);
*rest = skip(tok, ";");
add_type(exp);
Type *ty = current_fn->ty->return_ty;
if (ty->kind != TY_STRUCT && ty->kind != TY_UNION)
exp = new_cast(exp, current_fn->ty->return_ty);
node->lhs = exp;
return node;
}
if (equal(tok, "if")) {
Node *node = new_if_node(((void *)0), ((void *)0), ((void *)0), tok);
tok = skip(tok->next, "(");
node->cond = expr(&tok, tok);
tok = skip(tok, ")");
node->then = stmt(&tok, tok);
if (equal(tok, "else"))
node->els = stmt(&tok, tok->next);
*rest = tok;
return node;
}
if (equal(tok, "switch")) {
Node *node = new_switch_node(((void *)0), ((void *)0), tok);
tok = skip(tok->next, "(");
node->cond = expr(&tok, tok);
tok = skip(tok, ")");
Node *sw = current_switch;
current_switch = node;
char *brk = brk_label;
brk_label = node->brk_label = new_unique_name();
node->then = stmt(rest, tok);
current_switch = sw;
brk_label = brk;
return node;
}
if (equal(tok, "case")) {
if (!current_switch)
error_tok(tok, "stray case");
int begin = const_expr(&tok, tok->next);
int end;
if (equal(tok, "...")) {
end = const_expr(&tok, tok->next);
if (end < begin)
error_tok(tok, "empty case range specified");
} else {
end = begin;
}
Node *node = new_case_node(begin, end, tok);
tok = skip(tok, ":");
node->label = new_unique_name();
node->lhs = stmt(rest, tok);
node->case_next = current_switch->case_next;
current_switch->case_next = node;
return node;
}
if (equal(tok, "default")) {
if (!current_switch)
error_tok(tok, "stray default");
Node *node = new_case_node(0, 0, tok);
tok = skip(tok->next, ":");
node->label = new_unique_name();
node->lhs = stmt(rest, tok);
current_switch->default_case = node;
return node;
}
if (equal(tok, "for")) {
Node *node = new_for_node(((void *)0), ((void *)0), ((void *)0), ((void *)0), tok);
tok = skip(tok->next, "(");
enter_scope();
char *brk = brk_label;
char *cont = cont_label;
brk_label = node->brk_label = new_unique_name();
cont_label = node->cont_label = new_unique_name();
if (is_typename(tok)) {
Type *basety = declspec(&tok, tok, ((void *)0));
node->init = declaration(&tok, tok, basety, ((void *)0));
} else {
node->init = expr_stmt(&tok, tok);
}
if (!equal(tok, ";"))
node->cond = expr(&tok, tok);
tok = skip(tok, ";");
if (!equal(tok, ")"))
node->inc = expr(&tok, tok);
tok = skip(tok, ")");
node->then = stmt(rest, tok);
leave_scope();
brk_label = brk;
cont_label = cont;
return node;
}
if (equal(tok, "while")) {
Node *node = new_for_node(((void *)0), ((void *)0), ((void *)0), ((void *)0), tok);
tok = skip(tok->next, "(");
node->cond = expr(&tok, tok);
tok = skip(tok, ")");
char *brk = brk_label;
char *cont = cont_label;
brk_label = node->brk_label = new_unique_name();
cont_label = node->cont_label = new_unique_name();
node->then = stmt(rest, tok);
brk_label = brk;
cont_label = cont;
return node;
}
if (equal(tok, "do")) {
Node *node = new_do_node(((void *)0), ((void *)0), tok);
char *brk = brk_label;
char *cont = cont_label;
brk_label = node->brk_label = new_unique_name();
cont_label = node->cont_label = new_unique_name();
node->then = stmt(&tok, tok->next);
brk_label = brk;
cont_label = cont;
tok = skip(tok, "while");
tok = skip(tok, "(");
node->cond = expr(&tok, tok);
tok = skip(tok, ")");
*rest = skip(tok, ";");
return node;
}
if (equal(tok, "asm") || equal(tok, "__asm__") || equal(tok, "__asm"))
return asm_stmt(rest, tok);
if (equal(tok, "goto")) {
if (equal(tok->next, "*")) {
Node *node = new_goto_expr_node(expr(&tok, tok->next->next), tok);
*rest = skip(tok, ";");
return node;
}
Node *node = new_goto_node(get_ident(tok->next), tok);
node->goto_next = gotos;
gotos = node;
*rest = skip(tok->next->next, ";");
return node;
}
if (equal(tok, "break")) {
if (!brk_label)
error_tok(tok, "stray break");
Node *node = new_goto_node(((void *)0), tok);
node->unique_label = brk_label;
*rest = skip(tok->next, ";");
return node;
}
if (equal(tok, "continue")) {
if (!cont_label)
error_tok(tok, "stray continue");
Node *node = new_goto_node(((void *)0), tok);
node->unique_label = cont_label;
*rest = skip(tok->next, ";");
return node;
}
if (tok->kind == TK_IDENT && equal(tok->next, ":")) {
Node *node = new_label_node(chibicc_strndup(tok->loc, tok->len), tok);
node->unique_label = new_unique_name();
node->lhs = stmt(rest, tok->next->next);
node->goto_next = labels;
labels = node;
return node;
}
if (equal(tok, "{"))
return compound_stmt(rest, tok->next);
return expr_stmt(rest, tok);
}
static Node *compound_stmt(Token **rest, Token *tok) {
Node *node = new_block_node(((void *)0), tok);
Node head = {};
Node *cur = &head;
enter_scope();
while (!equal(tok, "}")) {
if (equal(tok, "_Static_assert") || equal(tok, "static_assert")) {
static_assertion(&tok, tok);
continue;
}
if (is_typename(tok) && !equal(tok->next, ":")) {
VarAttr attr = {};
Type *basety = declspec(&tok, tok, &attr);
if (attr.is_typedef) {
tok = parse_typedef(tok, basety);
continue;
}
if (is_function(tok)) {
tok = function(tok, basety, &attr);
continue;
}
if (attr.is_extern) {
tok = global_variable(tok, basety, &attr);
continue;
}
cur = cur->next = declaration(&tok, tok, basety, &attr);
} else {
cur = cur->next = stmt(&tok, tok);
}
add_type(cur);
}
leave_scope();
node->body = head.next;
*rest = tok->next;
return node;
}
static Node *expr_stmt(Token **rest, Token *tok) {
if (equal(tok, ";")) {
*rest = tok->next;
return new_block_node(((void *)0), tok);
}
Node *exp = expr(&tok, tok);
*rest = skip(tok, ";");
return new_expr_stmt_node(exp, tok);
}
static Node *expr(Token **rest, Token *tok) {
Node *node = assign(&tok, tok);
if (equal(tok, ","))
return new_binary(ND_COMMA, node, expr(rest, tok->next), tok);
*rest = tok;
return node;
}
static int64_t eval(Node *node) {
return eval2(node, ((void *)0));
}
static int64_t eval2(Node *node, char ***label) {
add_type(node);
if (is_flonum(node->ty))
return eval_double(node);
switch (node->kind) {
case ND_ADD:
return eval2(node->lhs, label) + eval(node->rhs);
case ND_SUB:
return eval2(node->lhs, label) - eval(node->rhs);
case ND_MUL:
return eval(node->lhs) * eval(node->rhs);
case ND_DIV: {
int64_t rhs = eval(node->rhs);
if (rhs == 0)
return 0;
if (node->ty->is_unsigned)
return (uint64_t)eval(node->lhs) / (uint64_t)rhs;
return eval(node->lhs) / rhs;
}
case ND_NEG:
return -eval(node->lhs);
case ND_MOD: {
int64_t rhs = eval(node->rhs);
if (rhs == 0)
return 0;
if (node->ty->is_unsigned)
return (uint64_t)eval(node->lhs) % (uint64_t)rhs;
return eval(node->lhs) % rhs;
}
case ND_BITAND:
return eval(node->lhs) & eval(node->rhs);
case ND_BITOR:
return eval(node->lhs) | eval(node->rhs);
case ND_BITXOR:
return eval(node->lhs) ^ eval(node->rhs);
case ND_SHL:
return eval(node->lhs) << eval(node->rhs);
case ND_SHR:
if (node->ty->is_unsigned && node->ty->size == 8)
return (uint64_t)eval(node->lhs) >> eval(node->rhs);
return eval(node->lhs) >> eval(node->rhs);
case ND_EQ:
return eval(node->lhs) == eval(node->rhs);
case ND_NE:
return eval(node->lhs) != eval(node->rhs);
case ND_LT:
if (node->lhs->ty->is_unsigned)
return (uint64_t)eval(node->lhs) < eval(node->rhs);
return eval(node->lhs) < eval(node->rhs);
case ND_LE:
if (node->lhs->ty->is_unsigned)
return (uint64_t)eval(node->lhs) <= eval(node->rhs);
return eval(node->lhs) <= eval(node->rhs);
case ND_COND:
return eval(node->cond) ? eval2(node->then, label) : eval2(node->els, label);
case ND_COMMA:
return eval2(node->rhs, label);
case ND_NOT:
return !eval(node->lhs);
case ND_BITNOT:
return ~eval(node->lhs);
case ND_LOGAND:
return eval(node->lhs) && eval(node->rhs);
case ND_LOGOR:
return eval(node->lhs) || eval(node->rhs);
case ND_CAST: {
int64_t val = eval2(node->lhs, label);
if (is_integer(node->ty)) {
switch (node->ty->size) {
case 1: return node->ty->is_unsigned ? (uint8_t)val : (int8_t)val;
case 2: return node->ty->is_unsigned ? (uint16_t)val : (int16_t)val;
case 4: return node->ty->is_unsigned ? (uint32_t)val : (int32_t)val;
}
}
return val;
}
case ND_ADDR:
return eval_rval(node->lhs, label);
case ND_LABEL_VAL:
*label = &node->unique_label;
return 0;
case ND_MEMBER:
if (!label)
error_tok(node->tok, "not a compile-time constant");
if (node->ty->kind != TY_ARRAY)
error_tok(node->tok, "invalid initializer");
return eval_rval(node->lhs, label) + node->member->offset;
case ND_VAR:
if (!label)
error_tok(node->tok, "not a compile-time constant");
if (node->var->ty->kind != TY_ARRAY && node->var->ty->kind != TY_FUNC)
error_tok(node->tok, "invalid initializer");
*label = &node->var->name;
return 0;
case ND_NUM:
return node->val;
}
error_tok(node->tok, "not a compile-time constant");
}
static int64_t eval_rval(Node *node, char ***label) {
switch (node->kind) {
case ND_VAR:
if (node->var->is_local)
error_tok(node->tok, "not a compile-time constant");
*label = &node->var->name;
return 0;
case ND_DEREF:
return eval2(node->lhs, label);
case ND_MEMBER:
return eval_rval(node->lhs, label) + node->member->offset;
}
error_tok(node->tok, "invalid initializer");
}
static _Bool is_const_expr(Node *node) {
add_type(node);
switch (node->kind) {
case ND_ADD:
case ND_SUB:
case ND_MUL:
case ND_DIV:
case ND_MOD:
case ND_BITAND:
case ND_BITOR:
case ND_BITXOR:
case ND_SHL:
case ND_SHR:
case ND_EQ:
case ND_NE:
case ND_LT:
case ND_LE:
case ND_LOGAND:
case ND_LOGOR:
return is_const_expr(node->lhs) && is_const_expr(node->rhs);
case ND_COND:
if (!is_const_expr(node->cond))
return 0;
return is_const_expr(eval(node->cond) ? node->then : node->els);
case ND_COMMA:
return is_const_expr(node->rhs);
case ND_NEG:
case ND_NOT:
case ND_BITNOT:
case ND_CAST:
return is_const_expr(node->lhs);
case ND_NUM:
return 1;
}
return 0;
}
int64_t const_expr(Token **rest, Token *tok) {
Node *node = conditional(rest, tok);
return eval(node);
}
static double eval_double(Node *node) {
add_type(node);
if (is_integer(node->ty)) {
if (node->ty->is_unsigned)
return (unsigned long)eval(node);
return eval(node);
}
switch (node->kind) {
case ND_ADD:
return eval_double(node->lhs) + eval_double(node->rhs);
case ND_SUB:
return eval_double(node->lhs) - eval_double(node->rhs);
case ND_MUL:
return eval_double(node->lhs) * eval_double(node->rhs);
case ND_DIV:
return eval_double(node->lhs) / eval_double(node->rhs);
case ND_NEG:
return -eval_double(node->lhs);
case ND_COND:
return eval_double(node->cond) ? eval_double(node->then) : eval_double(node->els);
case ND_COMMA:
return eval_double(node->rhs);
case ND_CAST:
if (is_flonum(node->lhs->ty))
return eval_double(node->lhs);
return eval(node->lhs);
case ND_NUM:
return node->fval;
}
error_tok(node->tok, "not a compile-time constant");
}
static Node *to_assign(Node *binary) {
add_type(binary->lhs);
add_type(binary->rhs);
Token *tok = binary->tok;
if (binary->lhs->kind == ND_MEMBER) {
Obj *var = new_lvar("", pointer_to(binary->lhs->lhs->ty));
Node *expr1 = new_binary(ND_ASSIGN, new_var_node(var, tok),
new_unary(ND_ADDR, binary->lhs->lhs, tok), tok);
Node *expr2 = new_unary(ND_MEMBER,
new_unary(ND_DEREF, new_var_node(var, tok), tok),
tok);
expr2->member = binary->lhs->member;
Node *expr3 = new_unary(ND_MEMBER,
new_unary(ND_DEREF, new_var_node(var, tok), tok),
tok);
expr3->member = binary->lhs->member;
Node *expr4 = new_binary(ND_ASSIGN, expr2,
new_binary(binary->kind, expr3, binary->rhs, tok),
tok);
return new_binary(ND_COMMA, expr1, expr4, tok);
}
if (binary->lhs->ty->is_atomic) {
Node head = {};
Node *cur = &head;
Obj *addr = new_lvar("", pointer_to(binary->lhs->ty));
Obj *val = new_lvar("", binary->rhs->ty);
Obj *old = new_lvar("", binary->lhs->ty);
Obj *new = new_lvar("", binary->lhs->ty);
cur = cur->next =
new_unary(ND_EXPR_STMT,
new_binary(ND_ASSIGN, new_var_node(addr, tok),
new_unary(ND_ADDR, binary->lhs, tok), tok),
tok);
cur = cur->next =
new_unary(ND_EXPR_STMT,
new_binary(ND_ASSIGN, new_var_node(val, tok), binary->rhs, tok),
tok);
cur = cur->next =
new_unary(ND_EXPR_STMT,
new_binary(ND_ASSIGN, new_var_node(old, tok),
new_unary(ND_DEREF, new_var_node(addr, tok), tok), tok),
tok);
Node *loop = new_node(ND_DO, tok);
loop->brk_label = new_unique_name();
loop->cont_label = new_unique_name();
Node *body = new_binary(ND_ASSIGN,
new_var_node(new, tok),
new_binary(binary->kind, new_var_node(old, tok),
new_var_node(val, tok), tok),
tok);
loop->then = new_node(ND_BLOCK, tok);
loop->then->body = new_unary(ND_EXPR_STMT, body, tok);
Node *cas = new_node(ND_CAS, tok);
cas->cas_addr = new_var_node(addr, tok);
cas->cas_old = new_unary(ND_ADDR, new_var_node(old, tok), tok);
cas->cas_new = new_var_node(new, tok);
loop->cond = new_unary(ND_NOT, cas, tok);
cur = cur->next = loop;
cur = cur->next = new_unary(ND_EXPR_STMT, new_var_node(new, tok), tok);
Node *node = new_node(ND_STMT_EXPR, tok);
node->body = head.next;
return node;
}
Obj *var = new_lvar("", pointer_to(binary->lhs->ty));
Node *expr1 = new_binary(ND_ASSIGN, new_var_node(var, tok),
new_unary(ND_ADDR, binary->lhs, tok), tok);
Node *expr2 =
new_binary(ND_ASSIGN,
new_unary(ND_DEREF, new_var_node(var, tok), tok),
new_binary(binary->kind,
new_unary(ND_DEREF, new_var_node(var, tok), tok),
binary->rhs,
tok),
tok);
return new_binary(ND_COMMA, expr1, expr2, tok);
}
static Node *assign(Token **rest, Token *tok) {
Node *node = conditional(&tok, tok);
if (equal(tok, "="))
return new_binary(ND_ASSIGN, node, assign(rest, tok->next), tok);
if (equal(tok, "+="))
return to_assign(new_add(node, assign(rest, tok->next), tok));
if (equal(tok, "-="))
return to_assign(new_sub(node, assign(rest, tok->next), tok));
if (equal(tok, "*="))
return to_assign(new_binary(ND_MUL, node, assign(rest, tok->next), tok));
if (equal(tok, "/="))
return to_assign(new_binary(ND_DIV, node, assign(rest, tok->next), tok));
if (equal(tok, "%="))
return to_assign(new_binary(ND_MOD, node, assign(rest, tok->next), tok));
if (equal(tok, "&="))
return to_assign(new_binary(ND_BITAND, node, assign(rest, tok->next), tok));
if (equal(tok, "|="))
return to_assign(new_binary(ND_BITOR, node, assign(rest, tok->next), tok));
if (equal(tok, "^="))
return to_assign(new_binary(ND_BITXOR, node, assign(rest, tok->next), tok));
if (equal(tok, "<<="))
return to_assign(new_binary(ND_SHL, node, assign(rest, tok->next), tok));
if (equal(tok, ">>="))
return to_assign(new_binary(ND_SHR, node, assign(rest, tok->next), tok));
*rest = tok;
return node;
}
static Node *conditional(Token **rest, Token *tok) {
Node *cond = logor(&tok, tok);
if (!equal(tok, "?")) {
*rest = tok;
return cond;
}
if (equal(tok->next, ":")) {
add_type(cond);
Obj *var = new_lvar("", cond->ty);
Node *lhs = new_binary(ND_ASSIGN, new_var_node(var, tok), cond, tok);
Node *rhs = new_node(ND_COND, tok);
rhs->cond = new_var_node(var, tok);
rhs->then = new_var_node(var, tok);
rhs->els = conditional(rest, tok->next->next);
return new_binary(ND_COMMA, lhs, rhs, tok);
}
Node *node = new_node(ND_COND, tok);
node->cond = cond;
node->then = expr(&tok, tok->next);
tok = skip(tok, ":");
node->els = conditional(rest, tok);
return node;
}
static Node *logor(Token **rest, Token *tok) {
Node *node = logand(&tok, tok);
while (equal(tok, "||")) {
Token *start = tok;
node = new_binary(ND_LOGOR, node, logand(&tok, tok->next), start);
}
*rest = tok;
return node;
}
static Node *logand(Token **rest, Token *tok) {
Node *node = bitor(&tok, tok);
while (equal(tok, "&&")) {
Token *start = tok;
node = new_binary(ND_LOGAND, node, bitor(&tok, tok->next), start);
}
*rest = tok;
return node;
}
static Node *bitor(Token **rest, Token *tok) {
Node *node = bitxor(&tok, tok);
while (equal(tok, "|")) {
Token *start = tok;
node = new_binary(ND_BITOR, node, bitxor(&tok, tok->next), start);
}
*rest = tok;
return node;
}
static Node *bitxor(Token **rest, Token *tok) {
Node *node = bitand(&tok, tok);
while (equal(tok, "^")) {
Token *start = tok;
node = new_binary(ND_BITXOR, node, bitand(&tok, tok->next), start);
}
*rest = tok;
return node;
}
static Node *bitand(Token **rest, Token *tok) {
Node *node = equality(&tok, tok);
while (equal(tok, "&")) {
Token *start = tok;
node = new_binary(ND_BITAND, node, equality(&tok, tok->next), start);
}
*rest = tok;
return node;
}
static Node *equality(Token **rest, Token *tok) {
Node *node = relational(&tok, tok);
for (;;) {
Token *start = tok;
if (equal(tok, "==")) {
node = new_binary(ND_EQ, node, relational(&tok, tok->next), start);
continue;
}
if (equal(tok, "!=")) {
node = new_binary(ND_NE, node, relational(&tok, tok->next), start);
continue;
}
*rest = tok;
return node;
}
}
static Node *relational(Token **rest, Token *tok) {
Node *node = shift(&tok, tok);
for (;;) {
Token *start = tok;
if (equal(tok, "<")) {
node = new_binary(ND_LT, node, shift(&tok, tok->next), start);
continue;
}
if (equal(tok, "<=")) {
node = new_binary(ND_LE, node, shift(&tok, tok->next), start);
continue;
}
if (equal(tok, ">")) {
node = new_binary(ND_LT, shift(&tok, tok->next), node, start);
continue;
}
if (equal(tok, ">=")) {
node = new_binary(ND_LE, shift(&tok, tok->next), node, start);
continue;
}
*rest = tok;
return node;
}
}
static Node *shift(Token **rest, Token *tok) {
Node *node = add(&tok, tok);
for (;;) {
Token *start = tok;
if (equal(tok, "<<")) {
node = new_binary(ND_SHL, node, add(&tok, tok->next), start);
continue;
}
if (equal(tok, ">>")) {
node = new_binary(ND_SHR, node, add(&tok, tok->next), start);
continue;
}
*rest = tok;
return node;
}
}
Node *new_add(Node *lhs, Node *rhs, Token *tok) {
add_type(lhs);
add_type(rhs);
if (is_numeric(lhs->ty) && is_numeric(rhs->ty))
return new_binary(ND_ADD, lhs, rhs, tok);
if (lhs->ty->base && rhs->ty->base)
error_tok(tok, "invalid operands");
if (!lhs->ty->base && rhs->ty->base) {
Node *tmp = lhs;
lhs = rhs;
rhs = tmp;
}
if (lhs->ty->base->kind == TY_VLA) {
rhs = new_binary(ND_MUL, rhs, new_var_node(lhs->ty->base->vla_size, tok), tok);
return new_binary(ND_ADD, lhs, rhs, tok);
}
rhs = new_cast(rhs, ty_llong);
if (lhs->ty->base->size != 1)
rhs = new_binary(ND_MUL, rhs, new_num(lhs->ty->base->size, tok), tok);
return new_binary(ND_ADD, lhs, rhs, tok);
}
Node *new_sub(Node *lhs, Node *rhs, Token *tok) {
add_type(lhs);
add_type(rhs);
if (is_numeric(lhs->ty) && is_numeric(rhs->ty))
return new_binary(ND_SUB, lhs, rhs, tok);
if (lhs->ty->base->kind == TY_VLA) {
rhs = new_binary(ND_MUL, rhs, new_var_node(lhs->ty->base->vla_size, tok), tok);
add_type(rhs);
Node *node = new_binary(ND_SUB, lhs, rhs, tok);
node->ty = lhs->ty;
return node;
}
if (lhs->ty->base && is_integer(rhs->ty)) {
rhs = new_cast(rhs, ty_llong);
if (lhs->ty->base->size != 1)
rhs = new_binary(ND_MUL, rhs, new_num(lhs->ty->base->size, tok), tok);
add_type(rhs);
Node *node = new_binary(ND_SUB, lhs, rhs, tok);
node->ty = lhs->ty;
return node;
}
if (lhs->ty->base && rhs->ty->base) {
Node *node = new_binary(ND_SUB, lhs, rhs, tok);
node->ty = ty_llong;
if (lhs->ty->base->size == 1)
return node;
return new_binary(ND_DIV, node, new_num(lhs->ty->base->size, tok), tok);
}
error_tok(tok, "invalid operands");
}
static Node *add(Token **rest, Token *tok) {
Node *node = mul(&tok, tok);
for (;;) {
Token *start = tok;
if (equal(tok, "+")) {
node = new_add(node, mul(&tok, tok->next), start);
continue;
}
if (equal(tok, "-")) {
node = new_sub(node, mul(&tok, tok->next), start);
continue;
}
*rest = tok;
return node;
}
}
static Node *mul(Token **rest, Token *tok) {
Node *node = cast(&tok, tok);
for (;;) {
Token *start = tok;
if (equal(tok, "*")) {
node = new_binary(ND_MUL, node, cast(&tok, tok->next), start);
continue;
}
if (equal(tok, "/")) {
node = new_binary(ND_DIV, node, cast(&tok, tok->next), start);
continue;
}
if (equal(tok, "%")) {
node = new_binary(ND_MOD, node, cast(&tok, tok->next), start);
continue;
}
*rest = tok;
return node;
}
}
static Node *cast(Token **rest, Token *tok) {
if (equal(tok, "(") && is_typename(tok->next)) {
Token *start = tok;
Type *ty = typename(&tok, tok->next);
tok = skip(tok, ")");
if (equal(tok, "{"))
return unary(rest, start);
Node *node = new_cast(cast(rest, tok), ty);
node->tok = start;
return node;
}
return unary(rest, tok);
}
static Node *unary(Token **rest, Token *tok) {
if (equal(tok, "+"))
return cast(rest, tok->next);
if (equal(tok, "-"))
return new_unary(ND_NEG, cast(rest, tok->next), tok);
if (equal(tok, "&")) {
Node *lhs = cast(rest, tok->next);
add_type(lhs);
if (lhs->kind == ND_MEMBER && lhs->member->is_bitfield)
error_tok(tok, "cannot take address of bitfield");
return new_unary(ND_ADDR, lhs, tok);
}
if (equal(tok, "*")) {
Node *node = cast(rest, tok->next);
add_type(node);
if (node->ty->kind == TY_FUNC)
return node;
return new_unary(ND_DEREF, node, tok);
}
if (equal(tok, "!"))
return new_unary(ND_NOT, cast(rest, tok->next), tok);
if (equal(tok, "~"))
return new_unary(ND_BITNOT, cast(rest, tok->next), tok);
if (equal(tok, "++"))
return to_assign(new_add(unary(rest, tok->next), new_num(1, tok), tok));
if (equal(tok, "--"))
return to_assign(new_sub(unary(rest, tok->next), new_num(1, tok), tok));
if (equal(tok, "&&")) {
Node *node = new_node(ND_LABEL_VAL, tok);
node->label = get_ident(tok->next);
node->goto_next = gotos;
gotos = node;
*rest = tok->next->next;
return node;
}
return postfix(rest, tok);
}
static void struct_members(Token **rest, Token *tok, Type *ty) {
Member head = {};
Member *cur = &head;
int idx = 0;
while (!equal(tok, "}")) {
VarAttr attr = {};
Type *basety = declspec(&tok, tok, &attr);
_Bool first = 1;
if ((basety->kind == TY_STRUCT || basety->kind == TY_UNION) &&
consume(&tok, tok, ";")) {
Member *mem = calloc(1, sizeof(Member));
mem->ty = basety;
mem->idx = idx++;
mem->align = attr.align ? attr.align : mem->ty->align;
cur = cur->next = mem;
continue;
}
while (!consume(&tok, tok, ";")) {
if (!first)
tok = skip(tok, ",");
first = 0;
Member *mem = calloc(1, sizeof(Member));
mem->ty = declarator(&tok, tok, basety);
mem->name = mem->ty->name;
mem->idx = idx++;
mem->align = attr.align ? attr.align : mem->ty->align;
if (mem->name) {
for (Member *m = head.next; m; m = m->next) {
if (m != mem && m->name && m->name->len == mem->name->len &&
!strncmp(m->name->loc, mem->name->loc, mem->name->len)) {
error_tok(mem->name, "duplicate member '%s'", get_ident(mem->name));
}
}
}
if (consume(&tok, tok, ":")) {
mem->is_bitfield = 1;
mem->bit_width = const_expr(&tok, tok);
}
cur = cur->next = mem;
}
}
if (cur != &head && cur->ty->kind == TY_ARRAY && cur->ty->array_len < 0) {
cur->ty = array_of(cur->ty->base, 0);
ty->is_flexible = 1;
}
*rest = tok->next;
ty->members = head.next;
}
static Type *struct_union_decl(Token **rest, Token *tok) {
Type *ty = struct_type();
VarAttr attr = {};
tok = consume_attributes(tok, &attr);
apply_attr_to_type(ty, &attr);
if (attr.align)
ty->align = attr.align;
Token *tag = ((void *)0);
if (tok->kind == TK_IDENT) {
tag = tok;
tok = tok->next;
}
VarAttr attr_mid = {};
tok = consume_attributes(tok, &attr_mid);
apply_attr_to_type(ty, &attr_mid);
if (attr_mid.align)
ty->align = attr_mid.align;
if (tag && !equal(tok, "{")) {
*rest = tok;
Type *ty2 = find_tag(tag);
if (ty2)
return ty2;
ty->size = -1;
push_tag_scope(tag, ty);
return ty;
}
tok = skip(tok, "{");
struct_members(&tok, tok, ty);
VarAttr attr2 = {};
*rest = consume_attributes(tok, &attr2);
apply_attr_to_type(ty, &attr2);
if (attr2.align)
ty->align = attr2.align;
if (tag) {
Type *ty2 = hashmap_get2(&scope->tags, tag->loc, tag->len);
if (ty2 && ty2->size >= 0)
error_tok(tag, "redefinition of struct or union '%s'", get_ident(tag));
if (ty2) {
*ty2 = *ty;
return ty2;
}
push_tag_scope(tag, ty);
}
return ty;
}
static Type *struct_decl(Token **rest, Token *tok) {
Type *ty = struct_union_decl(rest, tok);
ty->kind = TY_STRUCT;
if (ty->size < 0)
return ty;
int bits = 0;
for (Member *mem = ty->members; mem; mem = mem->next) {
if (mem->is_bitfield && mem->bit_width == 0) {
bits = align_to(bits, mem->ty->size * 8);
} else if (mem->is_bitfield) {
int sz = mem->ty->size;
if (bits / (sz * 8) != (bits + mem->bit_width - 1) / (sz * 8))
bits = align_to(bits, sz * 8);
mem->offset = align_down(bits / 8, sz);
mem->bit_offset = bits % (sz * 8);
bits += mem->bit_width;
} else {
if (!ty->is_packed)
bits = align_to(bits, mem->align * 8);
mem->offset = bits / 8;
bits += mem->ty->size * 8;
}
if (!ty->is_packed && ty->align < mem->align)
ty->align = mem->align;
}
ty->size = align_to(bits, ty->align * 8) / 8;
return ty;
}
static Type *union_decl(Token **rest, Token *tok) {
Type *ty = struct_union_decl(rest, tok);
ty->kind = TY_UNION;
if (ty->size < 0)
return ty;
for (Member *mem = ty->members; mem; mem = mem->next) {
if (ty->align < mem->align)
ty->align = mem->align;
if (ty->size < mem->ty->size)
ty->size = mem->ty->size;
}
ty->size = align_to(ty->size, ty->align);
return ty;
}
static Member *get_struct_member(Type *ty, Token *tok) {
for (Member *mem = ty->members; mem; mem = mem->next) {
if ((mem->ty->kind == TY_STRUCT || mem->ty->kind == TY_UNION) &&
!mem->name) {
if (get_struct_member(mem->ty, tok))
return mem;
continue;
}
if (mem->name && mem->name->len == tok->len &&
!strncmp(mem->name->loc, tok->loc, tok->len))
return mem;
}
return ((void *)0);
}
static Node *struct_ref(Node *node, Token *tok) {
add_type(node);
if (node->ty->kind != TY_STRUCT && node->ty->kind != TY_UNION)
error_tok(node->tok, "not a struct nor a union");
Type *ty = node->ty;
for (;;) {
Member *mem = get_struct_member(ty, tok);
if (!mem)
error_tok(tok, "no such member");
node = new_unary(ND_MEMBER, node, tok);
node->member = mem;
if (mem->name)
break;
ty = mem->ty;
}
return node;
}
static Node *new_inc_dec(Node *node, Token *tok, int addend) {
add_type(node);
return new_cast(new_add(to_assign(new_add(node, new_num(addend, tok), tok)),
new_num(-addend, tok), tok),
node->ty);
}
static Node *postfix(Token **rest, Token *tok) {
if (equal(tok, "(") && is_typename(tok->next)) {
Token *start = tok;
Type *ty = typename(&tok, tok->next);
tok = skip(tok, ")");
if (scope->next == ((void *)0)) {
Obj *var = new_anon_gvar(ty);
gvar_initializer(rest, tok, var);
return new_var_node(var, start);
}
Obj *var = new_lvar("", ty);
Node *lhs = lvar_initializer(rest, tok, var);
Node *rhs = new_var_node(var, tok);
return new_binary(ND_COMMA, lhs, rhs, start);
}
Node *node = primary(&tok, tok);
for (;;) {
if (equal(tok, "(")) {
node = funcall(&tok, tok->next, node);
continue;
}
if (equal(tok, "[")) {
Token *start = tok;
Node *idx = expr(&tok, tok->next);
tok = skip(tok, "]");
node = new_unary(ND_DEREF, new_add(node, idx, start), start);
continue;
}
if (equal(tok, ".")) {
node = struct_ref(node, tok->next);
tok = tok->next->next;
continue;
}
if (equal(tok, "->")) {
node = new_unary(ND_DEREF, node, tok);
node = struct_ref(node, tok->next);
tok = tok->next->next;
continue;
}
if (equal(tok, "++")) {
node = new_inc_dec(node, tok, 1);
tok = tok->next;
continue;
}
if (equal(tok, "--")) {
node = new_inc_dec(node, tok, -1);
tok = tok->next;
continue;
}
*rest = tok;
return node;
}
}
static Node *funcall(Token **rest, Token *tok, Node *fn) {
add_type(fn);
if (fn->ty->kind != TY_FUNC &&
(fn->ty->kind != TY_PTR || fn->ty->base->kind != TY_FUNC))
error_tok(fn->tok, "not a function");
Type *ty = (fn->ty->kind == TY_FUNC) ? fn->ty : fn->ty->base;
Type *param_ty = ty->params;
Node head = {};
Node *cur = &head;
while (!equal(tok, ")")) {
if (cur != &head)
tok = skip(tok, ",");
Node *arg = assign(&tok, tok);
add_type(arg);
if (!param_ty && !ty->is_variadic)
error_tok(tok, "too many arguments");
if (param_ty) {
if (param_ty->kind != TY_STRUCT && param_ty->kind != TY_UNION)
arg = new_cast(arg, param_ty);
param_ty = param_ty->next;
} else if (arg->ty->kind == TY_FLOAT) {
arg = new_cast(arg, ty_double);
}
cur = cur->next = arg;
}
if (param_ty)
error_tok(tok, "too few arguments");
*rest = skip(tok, ")");
Node *node = new_unary(ND_FUNCALL, fn, tok);
node->func_ty = ty;
node->ty = ty->return_ty;
node->args = head.next;
if (node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION) {
ABI *callee_abi = ty->abi ? ty->abi : (current_fn ? get_fn_abi(current_fn) : current_abi);
if (callee_abi && callee_abi->returns_by_reference && callee_abi->returns_by_reference(node->ty))
node->ret_buffer = new_lvar("", node->ty);
}
return node;
}
static Node *generic_selection(Token **rest, Token *tok) {
Token *start = tok;
tok = skip(tok, "(");
Node *ctrl = assign(&tok, tok);
add_type(ctrl);
Type *t1 = ctrl->ty;
if (t1->kind == TY_FUNC)
t1 = pointer_to(t1);
else if (t1->kind == TY_ARRAY)
t1 = pointer_to(t1->base);
Node *ret = ((void *)0);
while (!consume(rest, tok, ")")) {
tok = skip(tok, ",");
if (equal(tok, "default")) {
tok = skip(tok->next, ":");
Node *node = assign(&tok, tok);
if (!ret)
ret = node;
continue;
}
Type *t2 = typename(&tok, tok);
tok = skip(tok, ":");
Node *node = assign(&tok, tok);
if (is_compatible(t1, t2))
ret = node;
}
if (!ret)
error_tok(start, "controlling expression type not compatible with");
return ret;
}
static Node *primary(Token **rest, Token *tok) {
Token *start = tok;
if (equal(tok, "(") && equal(tok->next, "{")) {
Node *node = new_node(ND_STMT_EXPR, tok);
node->body = compound_stmt(&tok, tok->next->next)->body;
*rest = skip(tok, ")");
return node;
}
if (equal(tok, "(")) {
Node *node = expr(&tok, tok->next);
*rest = skip(tok, ")");
return node;
}
if (equal(tok, "sizeof") && equal(tok->next, "(") && is_typename(tok->next->next)) {
Type *ty = typename(&tok, tok->next->next);
*rest = skip(tok, ")");
if (ty->kind == TY_VLA) {
if (ty->vla_size)
return new_var_node(ty->vla_size, tok);
Node *lhs = compute_vla_size(ty, tok);
Node *rhs = new_var_node(ty->vla_size, tok);
return new_binary(ND_COMMA, lhs, rhs, tok);
}
return new_ulong(ty->size, start);
}
if (equal(tok, "sizeof")) {
Node *node = unary(rest, tok->next);
add_type(node);
if (node->ty->kind == TY_VLA)
return new_var_node(node->ty->vla_size, tok);
return new_ulong(node->ty->size, tok);
}
if (equal(tok, "_Alignof") && equal(tok->next, "(") && is_typename(tok->next->next)) {
Type *ty = typename(&tok, tok->next->next);
*rest = skip(tok, ")");
return new_ulong(ty->align, tok);
}
if (equal(tok, "_Alignof")) {
Node *node = unary(rest, tok->next);
add_type(node);
return new_ulong(node->ty->align, tok);
}
if (equal(tok, "_Generic"))
return generic_selection(rest, tok->next);
if (equal(tok, "__builtin_offsetof")) {
tok = skip(tok->next, "(");
Type *ty = typename(&tok, tok);
tok = skip(tok, ",");
int offset = 0;
while (tok->kind != TK_EOF) {
if (tok->kind == TK_IDENT) {
Member *mem = get_struct_member(ty, tok);
if (!mem)
error_tok(tok, "no such member");
offset += mem->offset;
ty = mem->ty;
tok = tok->next;
} else if (equal(tok, ".")) {
tok = tok->next;
Member *mem = get_struct_member(ty, tok);
if (!mem)
error_tok(tok, "no such member");
offset += mem->offset;
ty = mem->ty;
tok = tok->next;
} else if (equal(tok, "->")) {
tok = tok->next;
if (ty->kind == TY_PTR)
ty = ty->base;
Member *mem = get_struct_member(ty, tok);
if (!mem)
error_tok(tok, "no such member");
offset += mem->offset;
ty = mem->ty;
tok = tok->next;
} else if (equal(tok, "[")) {
tok = tok->next;
int idx = const_expr(&tok, tok);
tok = skip(tok, "]");
if (!ty->base)
error_tok(tok, "subscripted value is not an array or pointer");
offset += idx * ty->base->size;
ty = ty->base;
} else {
break;
}
}
*rest = skip(tok, ")");
Node *node = new_num(offset, start);
node->ty = ty_ulong;
return node;
}
if (equal(tok, "__builtin_types_compatible_p")) {
tok = skip(tok->next, "(");
Type *t1 = typename(&tok, tok);
tok = skip(tok, ",");
Type *t2 = typename(&tok, tok);
*rest = skip(tok, ")");
return new_num(is_compatible(t1, t2), start);
}
if (equal(tok, "__builtin_reg_class")) {
tok = skip(tok->next, "(");
Type *ty = typename(&tok, tok);
*rest = skip(tok, ")");
ABI *fn_abi = current_fn ? get_fn_abi(current_fn) : current_abi;
if (fn_abi && fn_abi->classify_reg)
return new_num(fn_abi->classify_reg(ty), start);
if (is_integer(ty) || ty->kind == TY_PTR)
return new_num(0, start);
if (is_flonum(ty))
return new_num(1, start);
return new_num(2, start);
}
if (equal(tok, "__builtin_compare_and_swap") ||
equal(tok, "__sync_val_compare_and_swap") ||
equal(tok, "__sync_bool_compare_and_swap")) {
Node *node = new_node(ND_CAS, tok);
tok = skip(tok->next, "(");
node->cas_addr = assign(&tok, tok);
tok = skip(tok, ",");
node->cas_old = assign(&tok, tok);
tok = skip(tok, ",");
node->cas_new = assign(&tok, tok);
*rest = skip(tok, ")");
return node;
}
if (equal(tok, "__builtin_atomic_exchange") ||
equal(tok, "__sync_lock_test_and_set")) {
Node *node = new_node(ND_EXCH, tok);
tok = skip(tok->next, "(");
node->lhs = assign(&tok, tok);
tok = skip(tok, ",");
node->rhs = assign(&tok, tok);
*rest = skip(tok, ")");
return node;
}
if (equal(tok, "__sync_synchronize")) {
tok = skip(tok->next, "(");
*rest = skip(tok, ")");
Node *node = new_node(ND_NULL_EXPR, start);
node->ty = ty_void;
return node;
}
if (equal(tok, "__sync_lock_release")) {
tok = skip(tok->next, "(");
Node *ptr = assign(&tok, tok);
*rest = skip(tok, ")");
Node *deref = new_unary(ND_DEREF, ptr, start);
return new_binary(ND_ASSIGN, deref, new_num(0, start), start);
}
if (equal(tok, "__sync_fetch_and_add") ||
equal(tok, "__sync_fetch_and_sub") ||
equal(tok, "__sync_fetch_and_or") ||
equal(tok, "__sync_fetch_and_and") ||
equal(tok, "__sync_fetch_and_xor") ||
equal(tok, "__sync_fetch_and_nand") ||
equal(tok, "__sync_add_and_fetch") ||
equal(tok, "__sync_sub_and_fetch") ||
equal(tok, "__sync_or_and_fetch") ||
equal(tok, "__sync_and_and_fetch") ||
equal(tok, "__sync_xor_and_fetch") ||
equal(tok, "__sync_nand_and_fetch")) {
char *name = tok->loc;
int len = tok->len;
tok = skip(tok->next, "(");
Node *ptr = assign(&tok, tok);
tok = skip(tok, ",");
Node *val = assign(&tok, tok);
*rest = skip(tok, ")");
Node *deref = new_unary(ND_DEREF, ptr, start);
NodeKind op;
if (strncmp(name, "__sync_fetch_and_add", len) == 0 || strncmp(name, "__sync_add_and_fetch", len) == 0)
op = ND_ADD;
else if (strncmp(name, "__sync_fetch_and_sub", len) == 0 || strncmp(name, "__sync_sub_and_fetch", len) == 0)
op = ND_SUB;
else if (strncmp(name, "__sync_fetch_and_or", len) == 0 || strncmp(name, "__sync_or_and_fetch", len) == 0)
op = ND_BITOR;
else if (strncmp(name, "__sync_fetch_and_and", len) == 0 || strncmp(name, "__sync_and_and_fetch", len) == 0)
op = ND_BITAND;
else
op = ND_BITXOR;
_Bool is_fetch_and_op = (strncmp(name, "__sync_fetch_and_", 17) == 0);
if (!is_fetch_and_op) {
return new_binary(ND_ASSIGN, deref, new_binary(op, deref, val, start), start);
} else {
if (op == ND_ADD)
return new_binary(ND_SUB, new_binary(ND_ASSIGN, deref, new_binary(ND_ADD, deref, val, start), start), val, start);
if (op == ND_SUB)
return new_binary(ND_ADD, new_binary(ND_ASSIGN, deref, new_binary(ND_SUB, deref, val, start), start), val, start);
if (op == ND_BITXOR)
return new_binary(ND_BITXOR, new_binary(ND_ASSIGN, deref, new_binary(ND_BITXOR, deref, val, start), start), val, start);
return new_binary(ND_ASSIGN, deref, new_binary(op, deref, val, start), start);
}
}
if (equal(tok, "__builtin_unreachable")) {
tok = skip(tok->next, "(");
*rest = skip(tok, ")");
Node *node = new_node(ND_NULL_EXPR, start);
node->ty = ty_void;
return node;
}
if (equal(tok, "__builtin_expect")) {
tok = skip(tok->next, "(");
Node *exp = assign(&tok, tok);
tok = skip(tok, ",");
(void)assign(&tok, tok);
*rest = skip(tok, ")");
return exp;
}
if (equal(tok, "__builtin_assume")) {
tok = skip(tok->next, "(");
(void)assign(&tok, tok);
*rest = skip(tok, ")");
Node *node = new_node(ND_NULL_EXPR, start);
node->ty = ty_void;
return node;
}
if (equal(tok, "__builtin_trap")) {
tok = skip(tok->next, "(");
*rest = skip(tok, ")");
Node *node = new_node(ND_NULL_EXPR, start);
node->ty = ty_void;
return node;
}
if (equal(tok, "__builtin_ia32_sfence") ||
equal(tok, "__builtin_ia32_lfence") ||
equal(tok, "__builtin_ia32_mfence") ||
equal(tok, "__builtin_ia32_pause")) {
tok = skip(tok->next, "(");
*rest = skip(tok, ")");
Node *node = new_node(ND_NULL_EXPR, start);
node->ty = ty_void;
return node;
}
if (equal(tok, "__builtin_ia32_rdtsc")) {
tok = skip(tok->next, "(");
*rest = skip(tok, ")");
Node *node = new_node(ND_NUM, start);
node->val = 0;
node->ty = ty_ulong;
return node;
}
if (equal(tok, "__builtin_abs") || equal(tok, "__builtin_labs") || equal(tok, "__builtin_llabs") ||
equal(tok, "__builtin_fabs") || equal(tok, "__builtin_fabsf") || equal(tok, "__builtin_fabsl")) {
tok = skip(tok->next, "(");
Node *val = assign(&tok, tok);
*rest = skip(tok, ")");
add_type(val);
Node *cond = new_binary(ND_LT, val, new_num(0, start), start);
Node *neg = new_unary(ND_NEG, val, start);
Node *node = new_node(ND_COND, start);
node->cond = cond;
node->then = neg;
node->els = val;
node->ty = val->ty;
return node;
}
if (equal(tok, "__builtin_prefetch")) {
tok = skip(tok->next, "(");
(void)assign(&tok, tok);
if (consume(&tok, tok, ",")) {
(void)assign(&tok, tok);
if (consume(&tok, tok, ","))
(void)assign(&tok, tok);
}
*rest = skip(tok, ")");
Node *node = new_node(ND_NULL_EXPR, start);
node->ty = ty_void;
return node;
}
if (equal(tok, "__builtin_debugbreak")) {
tok = skip(tok->next, "(");
*rest = skip(tok, ")");
Node *node = new_node(ND_NULL_EXPR, start);
node->ty = ty_void;
return node;
}
if (equal(tok, "__builtin_constant_p")) {
tok = skip(tok->next, "(");
Node *node = assign(&tok, tok);
*rest = skip(tok, ")");
return new_num(is_const_expr(node), start);
}
if (equal(tok, "__builtin_va_start")) {
tok = skip(tok->next, "(");
Node *ap = assign(&tok, tok);
tok = skip(tok, ",");
Node *last = assign(&tok, tok);
*rest = skip(tok, ")");
ABI *fn_abi = current_fn ? get_fn_abi(current_fn) : current_abi;
if (fn_abi && fn_abi->builtin_va_start)
return fn_abi->builtin_va_start(ap, last, start);
VarScope *sc = find_var(&(Token){.loc = "__va_area__", .len = 11});
if (!sc || !sc->var)
error_tok(start, "__builtin_va_start used outside variadic function");
Node *va_var = new_var_node(sc->var, start);
add_type(ap);
if (ap->ty->kind == TY_PTR && (ap->ty->base->kind != TY_STRUCT && ap->ty->base->kind != TY_UNION)) {
Node *addr = new_unary(ND_ADDR, va_var, start);
Node *val = new_unary(ND_DEREF, new_cast(addr, pointer_to(ap->ty)), start);
return new_binary(ND_ASSIGN, ap, val, start);
} else {
VarScope *va_elem_sc = find_var(&(Token){.loc = "__va_elem", .len = 9});
Type *va_elem_ty = (va_elem_sc && va_elem_sc->type_def) ? va_elem_sc->type_def : ty_void;
Node *cast = new_cast(new_unary(ND_ADDR, va_var, start), pointer_to(va_elem_ty));
Node *deref_va = new_unary(ND_DEREF, cast, start);
Node *deref_ap = new_unary(ND_DEREF, ap, start);
return new_binary(ND_ASSIGN, deref_ap, deref_va, start);
}
}
if (equal(tok, "__builtin_va_arg")) {
tok = skip(tok->next, "(");
Node *ap = assign(&tok, tok);
tok = skip(tok, ",");
Type *ty = typename(&tok, tok);
*rest = skip(tok, ")");
ABI *fn_abi = current_fn ? get_fn_abi(current_fn) : current_abi;
if (fn_abi && fn_abi->builtin_va_arg)
return fn_abi->builtin_va_arg(ap, ty, start);
error_tok(start, "__builtin_va_arg not supported for current ABI");
}
if (equal(tok, "__builtin_va_end")) {
tok = skip(tok->next, "(");
Node *ap = assign(&tok, tok);
*rest = skip(tok, ")");
ABI *fn_abi = current_fn ? get_fn_abi(current_fn) : current_abi;
if (fn_abi && fn_abi->builtin_va_end)
return fn_abi->builtin_va_end(ap, start);
Node *node = new_node(ND_NULL_EXPR, start);
node->ty = ty_void;
return node;
}
if (equal(tok, "__builtin_va_copy")) {
tok = skip(tok->next, "(");
Node *dest = assign(&tok, tok);
tok = skip(tok, ",");
Node *src = assign(&tok, tok);
*rest = skip(tok, ")");
ABI *fn_abi = current_fn ? get_fn_abi(current_fn) : current_abi;
if (fn_abi && fn_abi->builtin_va_copy)
return fn_abi->builtin_va_copy(dest, src, start);
add_type(dest);
if (dest->ty->kind == TY_PTR && (dest->ty->base->kind != TY_STRUCT && dest->ty->base->kind != TY_UNION)) {
return new_binary(ND_ASSIGN, dest, src, start);
} else {
Node *deref_dest = new_unary(ND_DEREF, dest, start);
Node *deref_src = new_unary(ND_DEREF, src, start);
return new_binary(ND_ASSIGN, deref_dest, deref_src, start);
}
}
if (tok->kind == TK_IDENT) {
VarScope *sc = find_var(tok);
*rest = tok->next;
if (sc && sc->var && sc->var->is_function) {
if (current_fn)
strarray_push(&current_fn->refs, sc->var->name);
else
sc->var->is_root = 1;
}
if (sc) {
if (sc->func_name && !sc->var)
sc->var = new_string_literal(sc->func_name, array_of(ty_char, strlen(sc->func_name) + 1));
if (sc->var)
return new_var_node(sc->var, tok);
if (sc->enum_ty)
return new_num(sc->enum_val, tok);
}
if (equal(tok->next, "(")) {
if (tok->len > 10 && strncmp(tok->loc, "__builtin_", 10) == 0) {
char *name = chibicc_strndup(tok->loc + 10, tok->len - 10);
Type *ty = func_type(ty_int);
ty->is_variadic = 1;
Obj *fn = new_gvar(name, ty);
fn->is_function = 1;
fn->is_definition = 0;
return new_var_node(fn, tok);
}
error_tok(tok, "implicit declaration of a function");
}
error_tok(tok, "undefined variable");
}
if (tok->kind == TK_STR) {
Obj *var = new_string_literal(tok->str, tok->ty);
*rest = tok->next;
return new_var_node(var, tok);
}
if (tok->kind == TK_NUM) {
Node *node;
if (is_flonum(tok->ty)) {
node = new_node(ND_NUM, tok);
node->fval = tok->fval;
} else {
node = new_num(tok->val, tok);
}
node->ty = tok->ty;
*rest = tok->next;
return node;
}
error_tok(tok, "expected an expression");
}
static Token *parse_typedef(Token *tok, Type *basety) {
_Bool first = 1;
while (!consume(&tok, tok, ";")) {
if (!first)
tok = skip(tok, ",");
first = 0;
Type *ty = declarator(&tok, tok, basety);
if (!ty->name)
error_tok(ty->name_pos, "typedef name omitted");
push_scope(get_ident(ty->name))->type_def = ty;
}
return tok;
}
static void create_param_lvars(Type *param) {
if (param) {
create_param_lvars(param->next);
if (!param->name)
error_tok(param->name_pos, "parameter name omitted");
char *name = get_ident(param->name);
VarScope *sc = hashmap_get2(&scope->vars, param->name->loc, param->name->len);
if (sc && sc->var)
error_tok(param->name, "duplicate parameter '%s'", name);
new_lvar(name, param);
}
}
static void resolve_goto_labels(void) {
for (Node *x = gotos; x; x = x->goto_next) {
for (Node *y = labels; y; y = y->goto_next) {
if (!strcmp(x->label, y->label)) {
x->unique_label = y->unique_label;
break;
}
}
if (x->unique_label == ((void *)0))
error_tok(x->tok->next, "use of undeclared label");
}
gotos = labels = ((void *)0);
}
static Obj *find_func(char *name) {
if (!name || !*name)
return ((void *)0);
Scope *sc = scope;
while (sc && sc->next)
sc = sc->next;
if (!sc)
return ((void *)0);
VarScope *sc2 = hashmap_get(&sc->vars, name);
if (sc2 && sc2->var && sc2->var->is_function)
return sc2->var;
return ((void *)0);
}
static void mark_live(Obj *var) {
if (!var->is_function || var->is_live)
return;
var->is_live = 1;
for (int i = 0; i < var->refs.len; i++) {
Obj *fn = find_func(var->refs.data[i]);
if (fn)
mark_live(fn);
}
}
static Token *function(Token *tok, Type *basety, VarAttr *attr) {
Type *ty = declarator(&tok, tok, basety);
if (!ty->name)
error_tok(ty->name_pos, "function name omitted");
char *name_str = get_ident(ty->name);
tok = consume_attributes(tok, attr);
apply_attr_to_type(ty, attr);
ABI *fn_abi = (attr && attr->abi) ? attr->abi : (ty->abi ? ty->abi : current_abi);
ty->abi = fn_abi;
Obj *fn = find_func(name_str);
if (fn) {
if (!fn->is_function)
error_tok(tok, "redeclared as a different kind of symbol");
if (fn->is_definition && equal(tok, "{"))
error_tok(tok, "redefinition of %s", name_str);
if (!fn->is_static && attr->is_static && !attr->is_inline)
error_tok(tok, "static declaration follows a non-static declaration");
fn->is_definition = fn->is_definition || equal(tok, "{");
if (attr->is_static || attr->is_inline)
fn->is_static = 1;
if (attr->is_inline)
fn->is_inline = 1;
if (attr->asm_name)
fn->name = attr->asm_name;
fn->abi = fn_abi;
} else {
fn = new_gvar(name_str, ty);
fn->is_function = 1;
fn->is_definition = equal(tok, "{");
fn->is_static = attr->is_static || attr->is_inline;
fn->is_inline = attr->is_inline;
if (attr->asm_name)
fn->name = attr->asm_name;
fn->abi = fn_abi;
}
fn->is_root = fn->is_definition && !fn->is_inline && !fn->is_static;
if (consume(&tok, tok, ";"))
return tok;
current_fn = fn;
locals = ((void *)0);
enter_scope();
create_param_lvars(ty->params);
Type *rty = ty->return_ty;
if ((rty->kind == TY_STRUCT || rty->kind == TY_UNION) && (fn_abi ? fn_abi->returns_by_reference(rty) : rty->size > 16))
new_lvar("", pointer_to(rty));
fn->params = locals;
if (ty->is_variadic)
fn->va_area = new_lvar("__va_area__", array_of(ty_char, fn_abi ? fn_abi->va_area_size : 136));
fn->alloca_bottom = new_lvar("__alloca_size__", pointer_to(ty_char));
tok = skip(tok, "{");
push_scope("__func__")->func_name = fn->name;
push_scope("__FUNCTION__")->func_name = fn->name;
fn->body = compound_stmt(&tok, tok);
fn->locals = locals;
leave_scope();
resolve_goto_labels();
return tok;
}
static Token *global_variable(Token *tok, Type *basety, VarAttr *attr) {
_Bool first = 1;
while (!consume(&tok, tok, ";")) {
if (!first)
tok = skip(tok, ",");
first = 0;
Type *ty = declarator(&tok, tok, basety);
if (!ty->name)
error_tok(ty->name_pos, "variable name omitted");
tok = consume_attributes(tok, attr);
apply_attr_to_type(ty, attr);
char *name = get_ident(ty->name);
VarScope *sc = find_var(ty->name);
if (sc && sc->var) {
Obj *var = sc->var;
if (var->is_function)
error_tok(ty->name, "redeclared as a different kind of symbol");
if (!is_compatible(var->ty, ty))
error_tok(ty->name, "conflicting types for '%s'", name);
if (!var->is_static && attr->is_static)
error_tok(ty->name, "static declaration follows a non-static declaration");
if (var->is_static && !attr->is_static && !attr->is_extern)
error_tok(ty->name, "non-static declaration follows a static declaration");
if (var->ty->kind == TY_ARRAY && var->ty->array_len < 0 && ty->kind == TY_ARRAY && ty->array_len >= 0) {
var->ty = ty;
}
_Bool was_def = var->is_definition && !var->is_tentative;
if (!attr->is_extern) {
var->is_definition = 1;
if (!attr->is_static)
var->is_static = 0;
}
if (equal(tok, "=")) {
if (was_def && !var->is_static)
error_tok(ty->name, "redefinition of '%s'", name);
var->is_definition = 1;
var->is_tentative = 0;
gvar_initializer(&tok, tok->next, var);
} else if (!var->init_data && !attr->is_extern && !attr->is_tls && !was_def) {
var->is_tentative = 1;
}
continue;
}
Obj *var = new_gvar(name, ty);
var->is_definition = !attr->is_extern;
var->is_static = attr->is_static;
var->is_tls = attr->is_tls;
if (attr->is_const && !attr->is_tls)
var->is_readonly = 1;
if (attr->asm_name)
var->name = attr->asm_name;
if (attr->align)
var->align = attr->align;
if (equal(tok, "="))
gvar_initializer(&tok, tok->next, var);
else if (!attr->is_extern && !attr->is_tls)
var->is_tentative = 1;
}
return tok;
}
static _Bool is_function(Token *tok) {
if (equal(tok, ";"))
return 0;
Type dummy = {};
Type *ty = declarator(&tok, tok, &dummy);
tok = consume_attributes(tok, ((void *)0));
return ty->kind == TY_FUNC;
}
static void scan_globals(void) {
Obj head;
Obj *cur = &head;
for (Obj *var = globals; var; var = var->next) {
if (!var->is_tentative) {
cur = cur->next = var;
continue;
}
Obj *var2 = globals;
for (; var2; var2 = var2->next)
if (var != var2 && var2->is_definition && !strcmp(var->name, var2->name))
break;
if (!var2)
cur = cur->next = var;
}
cur->next = ((void *)0);
globals = head.next;
}
static void declare_builtin_functions(void) {
Type *ty = func_type(pointer_to(ty_void));
ty->params = copy_type(ty_int);
builtin_alloca = new_gvar("alloca", ty);
builtin_alloca->is_definition = 0;
}
static void declare_builtin_types(void) {
declare_abi_builtin_types();
}
static void static_assertion(Token **rest, Token *tok) {
Token *start = tok;
tok = skip(tok->next, "(");
int64_t val = const_expr(&tok, tok);
tok = skip(tok, ",");
if (tok->kind != TK_STR)
error_tok(tok, "expected string literal in _Static_assert");
tok = tok->next;
tok = skip(tok, ")");
*rest = skip(tok, ";");
if (!val)
error_tok(start, "static assertion failed");
}
Obj *parse(Token *tok) {
scope = &(Scope){};
declare_builtin_functions();
declare_builtin_types();
globals = ((void *)0);
while (tok->kind != TK_EOF) {
if (equal(tok, "__extension__") || equal(tok, "__extension")) {
tok = tok->next;
continue;
}
if (equal(tok, "asm") || equal(tok, "__asm__") || equal(tok, "__asm")) {
asm_stmt(&tok, tok);
continue;
}
if (equal(tok, "_Static_assert") || equal(tok, "static_assert")) {
static_assertion(&tok, tok);
continue;
}
VarAttr attr = {};
Type *basety = declspec(&tok, tok, &attr);
if (attr.is_typedef) {
tok = parse_typedef(tok, basety);
continue;
}
if (is_function(tok)) {
tok = function(tok, basety, &attr);
continue;
}
tok = global_variable(tok, basety, &attr);
}
for (Obj *var = globals; var; var = var->next) {
if (var->is_root)
mark_live(var);
if (!var->is_function) {
for (Relocation *rel = var->rel; rel; rel = rel->next) {
if (rel->label && *rel->label && (*rel->label)[0] != '.') {
Obj *fn = find_func(*rel->label);
if (fn)
mark_live(fn);
}
}
}
}
scan_globals();
return globals;
}
