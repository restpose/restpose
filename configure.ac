dnl Process this file with autoconf to produce a configure script.
dnl Note - large parts of this file are copied from Xapian.

AC_INIT([restpose], [0.7.0dev], [http://github.com/rboulton/restpose])
dnl We require the same version of autoconf as xapian does, for convenience.
AC_PREREQ([2.64])

dnl We require the same version of automake as xapian does, for convenience.
AM_INIT_AUTOMAKE([1.10.2 -Wportability tar-ustar std-options])

AC_CONFIG_SRCDIR([src/jsonxapian/indexing.cc])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

dnl Use the same version of libtool as xapian does, for convenience.
LT_PREREQ([2.2.6])
LT_INIT

dnl Add parameters for aclocal
dnl (This must come after AM_INIT_AUTOMAKE, since it modifies ACLOCAL)
ACLOCAL="$ACLOCAL $ACLOCAL_FLAGS"

dnl -no-undefined can cause problems on some platforms, so only pass it
dnl on platforms where it is required in order to link a shared library at
dnl all (Windows is the main one).
NO_UNDEFINED=
if test unsupported = "$allow_undefined_flag" ; then
  NO_UNDEFINED=-no-undefined
fi
AC_SUBST(NO_UNDEFINED)

dnl disable "maintainer only" rules by default
AM_MAINTAINER_MODE

dnl Checks for programs.
AC_PROG_CXX
AM_PROG_CC_C_O

dnl Run tests using the C++ compiler.
AC_LANG_CPLUSPLUS

dnl Check for xapian-core.
XO_LIB_XAPIAN([], [], [xapian-config])

AC_TYPE_UINT64_T

AM_CXXFLAGS=

dnl configure useful flags for the C++ compiler.
WARNING_CXXFLAGS=
ANSI_CXXFLAGS=
AC_MSG_CHECKING([for $CXX options to enable ANSI C++ mode])
if test yes = "$GXX"; then
  compiler=gcc
  AC_MSG_RESULT([none required])
else
  compiler=unknown
  dnl Some C++ compilers need a special switch to select ANSI C++ mode.  Do
  dnl this early in configure so such settings are used for other tests.
  case $CXX in
    aCC|*/aCC)
      dnl Select ANSI mode for HP's aCC, which gives us the std:: namespace,
      dnl Koenig lookup, and ANSI scoping for the variable i in:
      dnl   for (int i = 1; i < n; ++i) { ... }
      dnl (don't confuse -AA with -Aa which is similar but without the std::
      dnl namespace).
      dnl
      dnl Note that using -AA means that user code must be built with -AA
      dnl (which is the default on Itanium, but not on PARISC:
      dnl  http://h21007.www2.hp.com/dspp/tech/tech_TechDocumentDetailPage_IDX/1,1701,5520,00.html
      dnl )
      dnl Also -ext (which can also be spelled +e):
      dnl "Allow various C++ extensions.  Currently -ext enables the 64 bit
      dnl integer (long long) data type."  This seems to be implied by -AA
      dnl but otherwise is needed to allow "cout << (long long)42;".
      ANSI_CXXFLAGS="-AA"
      dnl +w turns on more warnings.
      dnl +wlint turns on "lint-like" warnings.
      dnl +W<n1>,<n2>,... suppresses warnings n1, n2, ...
      dnl 2340 (remark) "value copied to temporary, reference to temporary
      dnl	use", in: throw Xapian::UnimplementedError("...");
      dnl 2401 "destructor for base class ... is non-virtual" (we don't need a
      dnl	virtual destructor for RefCntBase, since we never delete its
      dnl	subclasses by a RefCntBase *).
      dnl 3348 "declaration hides constant ..." which seems to misfire!
      dnl 4255 (remark) "padding size of struct "..." with ... bytes to
      dnl	alignment boundary".
      dnl 4273 "floating-point equality and inequality comparisons may be
      dnl	inappropriate due to roundoff common in floating-point computation"
      dnl	No obvious workaround for when you really do want == or !=.
      dnl 4285 "operator= does not have a check for the source and destination
      dnl	addresses being non-identical" - fires for AutoPtr which
      dnl	includes such a check indirectly (internaltest's autoptr1 check this).
      dnl 20201 "Memory leak is detected" which triggers for "return new Foo;"!
      WARNING_CXXFLAGS="+w +wlint +W2340,2401,3348,4255,4273,4285,20201" ;;
    cxx|*/cxx)
      dnl Select ANSI mode ('-std strict_ansi' is needed for ANSI iostream
      dnl as '-std ansi' gives a pre-standard AT&T-compatible version).
      dnl If this proves to be a problem, try '-std ansi -D__USE_STD_IOSTREAM'
      ANSI_CXXFLAGS="-std strict_ansi"
      dnl Check that the compiler recognises these flags to avoid problems
      dnl with other compilers named cxx.
      AC_TRY_COMPILE([], [], , ANSI_CXXFLAGS=) ;;
    CC|*/CC)
      dnl Could be Sun's, SGI's, or something else entirely.
      case `$CXX -V 2>&1` in
      *Sun\ C++*)
	dnl Select ANSI conforming STL - for more information see:
	dnl http://developers.sun.com/solaris/articles/cmp_stlport_libCstd.html
	dnl and:
	dnl http://developers.sun.com/sunstudio/documentation/ss12/mr/READMEs/c++_faq.html#Coding12
	dnl (Looks like -features=tmplife was added around 2000 so just pass
	dnl it unconditionally)
	compiler=sun
	ANSI_CXXFLAGS="-library=stlport4 -features=tmplife" ;;
      *)
	case `$CXX -v 2>&1` in
	MIPSpro*)
	  dnl On stderr: MIPSpro Compilers: Version 7.4.2m
	  dnl Select ANSI mode for SGI's CC with "-LANG:std".  Also, we have to
	  dnl specify "-ptused" or we get strange template linking errors.
	  ANSI_CXXFLAGS="-LANG:std -ptused" ;;
	esac
	;;
      esac
      ;;
  esac

  if test -n "$ANSI_CXXFLAGS" ; then
    AC_MSG_RESULT([$ANSI_CXXFLAGS])
  else
    dnl Perhaps we should try to compile some code which uses iostream, for
    dnl scoping, etc, but maintaining such a snippet to cover everything we
    dnl require which any C++ compiler may not support is tricky.
    AC_MSG_RESULT([none known for $CXX])
  fi
fi

dnl Preserve the default CXXFLAGS.
save_CXXFLAGS=$CXXFLAGS

dnl We want XAPIAN_CXXFLAGS to be used for configure tests.
CXXFLAGS="$CXXFLAGS $XAPIAN_CXXFLAGS"

dnl Put any flags for ANSI mode in CXXFLAGS for the duration of configure
dnl since we want to make feature tests using those flags.
if test -n "$ANSI_CXXFLAGS" ; then
  CXXFLAGS="$ANSI_CXXFLAGS $CXXFLAGS"
  AM_CXXFLAGS="$ANSI_CXXFLAGS $AM_CXXFLAGS"
fi

dnl x86 has excess precision issues with 387 FP instructions, which are
dnl avoided by using SSE instructions instead.  This is also faster (~6% in
dnl a CPU bound testcase).
AC_ARG_ENABLE([sse],
[AS_HELP_STRING([--disable-sse],
		[disable use of SSE FP instructions on x86])]
[AS_HELP_STRING([[--enable-sse[=sse|sse2]]],
		[set which SSE FP instructions to use on x86 (default: sse2)])],
  [case ${enableval} in
    sse|sse2|yes|no) ;;
    *) AC_MSG_ERROR([bad value ${enableval} for --enable-sse or --disable-sse]) ;;
  esac],
  [enable_sse=yes])

dnl Some versions of Sun's C++ compiler reportedly need an explicit -lm.
dnl The maths functions we use include: exp log ceil fabs sqrt
AC_MSG_CHECKING([if -lm is required for maths functions])
dnl Don't use constant arguments as the compiler might simply evaluate the
dnl whole expression at compile time, and it might inline certain functions
dnl so test several functions.  Also write results using printf() so that
dnl the compiler can't optimise away the computations.
AC_TRY_LINK([#ifdef __SUNPRO_CC
#error Need -lm for Sun C++ 5.9 under libtool 2.2.10
#endif
#include <cmath>
#include <cstdio>
#include <ctime>
using namespace std;], [
  double a = log(ceil(time(NULL)/7.0));
  printf("%f %f %f\n", a, sqrt(a), exp(fabs(a - 12345.6)));
  ], [AC_MSG_RESULT(no)], [
    LIBS="-lm $LIBS"
    AC_TRY_LINK([#include <cmath>
#include <cstdio>
#include <ctime>
using namespace std;], [
  double a = log(ceil(time(NULL)/7.0));
  printf("%f %f %f\n", a, sqrt(a), exp(fabs(a - 12345.6)));],
	[AC_MSG_RESULT(yes)],
	[AC_MSG_ERROR([Failed to link a C++ program using <cmath>])
    ])
])

dnl Check for time functions.
AC_CHECK_FUNCS([gettimeofday ftime])

dnl Check if we can use setenv (Solaris < 10 only has putenv()):
AC_CHECK_FUNCS([setenv])

dnl Check for a more efficient way of closing fds during daemonisation.
dnl Apparently closefrom() is available on at least "Solaris 9 or later, NetBSD
dnl 3.0 or later, OpenBSD 3.5 or later".  If we don't have closefrom(), then
dnl dirfd() is useful for an efficient implementation on some platforms.
AC_CHECK_FUNCS([closefrom dirfd])

dnl See if ftime returns void (as it does on mingw)
AC_MSG_CHECKING([return type of ftime])
if test $ac_cv_func_ftime = yes ; then
  AC_TRY_COMPILE([#include <sys/timeb.h>],
    [struct timeb tp; int i = ftime(&tp);],
    AC_MSG_RESULT(int),
    AC_MSG_RESULT(void)
    AC_DEFINE(FTIME_RETURNS_VOID, 1, [Define if ftime returns void]))
fi

dnl Check how to find the hostname: uname() in sys/utsname.h, or gethostname()
dnl Don't use default includes as inttypes.h is found by Compaq C but not C++
dnl so it causes all header probes to fail.
AC_CHECK_HEADERS(sys/utsname.h, [], [], [ ])
AC_CHECK_FUNCS(gethostname)

dnl mingw (for instance) lacks ssize_t
AC_TYPE_SSIZE_T
AC_TYPE_PID_T
AC_TYPE_MODE_T
AC_FUNC_STRERROR_R

AC_CHECK_SIZEOF([int])
AC_CHECK_SIZEOF([long])

dnl Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h sys/errno.h sys/select.h], [], [], [ ])

dnl If valgrind is installed and new enough, we use it for leak checking in the
dnl testsuite.  If VALGRIND is set to an empty value, then skip the check and
dnl don't use valgrind.
if test -n "${VALGRIND-unset}" ; then
  AC_PATH_PROG(VALGRIND, valgrind, [])
  if test -n "$VALGRIND" ; then
    dnl Check that the installed valgrind version works, and supports the
    dnl options we use.  This means we won't try to use valgrind < 3.3.0
    dnl (released 7/12/2007) since before that --log-file didn't expand
    dnl %p (and appended the process id).
    dnl
    dnl No need to check for VALGRIND_COUNT_LEAKS now - that was added before
    dnl 2.0.0.
    AC_MSG_CHECKING([if valgrind supports --log-file with %p])
    vglog=config.vglog.%p.tmp
    vglogwild="config.vglog.*.tmp*"
    rm -f $vglogwild
    if $VALGRIND --log-file="$vglog" -q true 2>&AS_MESSAGE_LOG_FD ; then
      for f in $vglogwild ; do
        case $f in
	$vglog*) VALGRIND= ;;
	esac
      done
      if test x"$VALGRIND" = x ; then
	AC_MSG_RESULT([no])
      else
	AC_MSG_RESULT([yes])
	AC_CHECK_HEADERS([valgrind/memcheck.h], [], [VALGRIND=], [ ])
      fi	
    else
      dnl The valgrind detected doesn't seem to work!  Perhaps this is an
      dnl x86_64 box with a 32 bit valgrind.
      AC_MSG_RESULT([$VALGRIND doesn't work])
      VALGRIND=
    fi
    rm -f $vglogwild
  fi
fi

if test -n "$VALGRIND" ; then
  AC_DEFINE(HAVE_VALGRIND, 1, [Define if a suitable valgrind is installed])
fi

dnl Checks for library functions.
AC_FUNC_MEMCMP
AC_CHECK_FUNCS(strerror hstrerror)

dnl Check for lstat() (not available under mingw for example).
AC_CHECK_FUNCS(lstat)

dnl Need a full link check for posix_fadvise.
AC_LINK_IFELSE([
  AC_LANG_PROGRAM(
    [[
#ifdef __linux__
# define _POSIX_C_SOURCE 200112L /* for posix_fadvise from fcntl.h */
#endif
#include <fcntl.h>
    ]], [[
      posix_fadvise(1, 0, 0, POSIX_FADV_NOREUSE);
      posix_fadvise(1, 0, 0, POSIX_FADV_SEQUENTIAL);
      posix_fadvise(1, 0, 0, POSIX_FADV_DONTNEED);
    ]])],
  [AC_DEFINE(HAVE_POSIX_FADVISE, 1, [Define to 1 if you have the 'posix_fadvise' function])]
)

dnl Check that snprintf actually works as it's meant to.
dnl
dnl Linux 'man snprintf' warns:
dnl  Linux libc4.[45] does not have a snprintf, but provides a libbsd that
dnl  contains an snprintf equivalent to sprintf, i.e., one that ignores the
dnl  size argument.  Thus, the use of snprintf with early libc4 leads to
dnl  serious security problems.
dnl
dnl It also warns that glibc < 2.0.6 (and presumably other pre-C90
dnl implementations) return -1 when truncating so check that we get the
dnl ISO C90 semantics for the returned length when truncating.  If we
dnl have a working snprintf but with non-ISO return semantics, handle
dnl that case separately as it may still be useful in many cases.
dnl
dnl mingw has _snprintf so check for that too.
AC_MSG_CHECKING(for working ISO C90 conforming snprintf)
ac_cv_func_snprintf_noniso=no
for func in snprintf _snprintf ; do
  AC_RUN_IFELSE([
    AC_LANG_PROGRAM(
      [[
#include <cstdio>
#include <cstring>
using namespace std;
      ]],
      dnl Return different exit status for each error so we can see which
      dnl check failed by consulting config.log.
      [[
	char buffer[4] = "abc";
	int res1 = $func(buffer, 2, "%s", "XYZ");
	if (memcmp(buffer, "X\0c", 4) != 0) return 2;
	int res2 = $func(buffer, 2, "%x", 0x12);
	if (memcmp(buffer, "1\0c", 4) != 0) return 3;
	if (res1 == -1 && res2 == -1) return 15; /* Pre-ISO semantics. */
	if (res1 != 3) return 4;
	if (res2 != 2) return 5;
      ]]
    )],
    [ac_cv_func_snprintf=$func;break],
    [
    if test 15no = "$?$ac_cv_func_snprintf_noniso" ; then
      ac_cv_func_snprintf_noniso=$func
    fi
    ac_cv_func_snprintf=no
    ],
    [ac_cv_func_snprintf=unknown;break]
  )
done
AC_MSG_RESULT([$ac_cv_func_snprintf])
case $ac_cv_func_snprintf in
  no)
    AC_MSG_CHECKING(for working non-ISO C90 conforming snprintf)
    AC_MSG_RESULT([$ac_cv_func_snprintf_noniso])
    if test no != "$ac_cv_func_snprintf_noniso" ; then
      AC_DEFINE_UNQUOTED(SNPRINTF, [$ac_cv_func_snprintf_noniso],
	[Define to the name of a function implementing snprintf but not caring about ISO C99 return value semantics (if one exists)])
    fi
    ;;
  unknown)
    dnl be conservative when crosscompiling
    ;;
  *)
    AC_DEFINE_UNQUOTED(SNPRINTF_ISO, [$ac_cv_func_snprintf],
       [Define to the name of a function implementing snprintf with ISO C99 semantics (if one exists)])
    AC_DEFINE_UNQUOTED(SNPRINTF, [$ac_cv_func_snprintf],
       [Define to the name of a function implementing snprintf but not caring about ISO C99 return value semantics (if one exists)])
    ;;
esac

dnl ***************************
dnl * Select modules to build *
dnl ***************************

dnl Check which modules should be built.

AC_DEFUN([JX_MODULE_ENABLE],
  [AC_ARG_ENABLE(backend_$1,
    [AS_HELP_STRING([--enable-module-$1], [build the $1 module [default=yes]])],
    [case $enableval in
      yes|no) ;;
      *) AC_MSG_ERROR([Invalid option: '--enable-module-$1=$enableval']) ;;
    esac], [enable_backend_$1=yes])
  ])

dnl Optional compile-time modules.
JX_MODULE_ENABLE(mongodb)

if test yes = "$enable_module_mongodb" ; then
  dnl FIXME - enable build of mongodb interface library.
  :
fi

use_win32_uuid_api=0

dnl We use zlib for decompressing binary items from mongodb.  We could
dnl automatically disable support if zlib isn't found, but zlib is required for
dnl Xapian anyway, so it's easier just to require it.
dnl
dnl Similarly for uuid support.

dnl Check for zlib.h.
AC_CHECK_HEADERS(zlib.h, [], [
  AC_MSG_ERROR([zlib.h not found - required for brass, chert and flint (you may need to install the zlib1g-dev or zlib-devel package)])
  ], [ ])

dnl Check for zlibVersion in -lz.
SAVE_LIBS=$LIBS
LIBS=
dnl mingw build needs -lzlib or -lzdll.
AC_SEARCH_LIBS([zlibVersion], [z zlib zdll], [], [
  AC_MSG_ERROR([zlibVersion() not found in -lz, -lzlib, or -lzdll - required for brass, chert and flint (you may need to install the zlib1g-dev or zlib-devel package)])
  ])
if test x != x"$LIBS" ; then
  RESTPOSE_LDFLAGS="$XAPIAN_LDFLAGS $LIBS"
fi
LIBS=$SAVE_LIBS

dnl Find the UUID library (from e2fsprogs/util-linux-ng, not the OSSP one).

case $host_os in
  *mingw* | *cygwin*) dnl Use built-in API.
    use_win32_uuid_api=1
    ;;
  *)
    dnl Check for uuid/uuid.h (e2fsprogs/util-linux-ng) or uuid.h
    dnl (FreeBSD/NetBSD).
    AC_CHECK_HEADERS([uuid/uuid.h], [
      dnl util-linux-ng's uuid/uuid.h found - check for uuid_generate in
      dnl -luuid.
      SAVE_LIBS=$LIBS
      LIBS=
      AC_SEARCH_LIBS([uuid_generate], [uuid], [], [
	AC_MSG_ERROR([uuid_generate() not found in -luuid - required for brass, chert and flint (you may need to install the uuid-dev, libuuid-devel or e2fsprogs-devel package)])
	])
      if test x != x"$LIBS" ; then
	XAPIAN_LDFLAGS="$XAPIAN_LDFLAGS $LIBS"
      fi
    ], [
      dnl Try uuid.h as found on FreeBSD/NetBSD, with associated code in libc.
      AC_CHECK_HEADERS([uuid.h], [
	dnl Check for uuid_create with no extra libraries required.
	AC_CHECK_FUNC([uuid_create], [], [
	  AC_MSG_ERROR([uuid.h found, but uuid_create() not found.  You probably wants to install libuuid from util-linux-ng or e2fsprogs (you may need to install the uuid-dev, libuuid-devel or e2fsprogs-devel package)])
	])
      ], [
	AC_MSG_ERROR([Neither uuid/uuid.h nor uuid.h found - required for brass, chert and flint (you may need to install the uuid-dev, libuuid-devel or e2fsprogs-devel package)])
      ], [ ])
    ], [ ])
    ;;
esac

dnl Older versions of libuuid (such as that on CentOS 4.7) don't have
dnl uuid_unparse_lower(), only uuid_unparse().
AC_LINK_IFELSE([AC_LANG_CALL([], [uuid_unparse_lower])], [found=true], [found=false])
if $found ; then
  AC_DEFINE([HAVE_UUID_UNPARSE_LOWER], [1],
	    [Define to 1 if you have the 'uuid_unparse_lower' function.])
fi

LIBS=$SAVE_LIBS

AM_CONDITIONAL([USE_WIN32_UUID_API], [test "$use_win32_uuid_api" = 1])

AC_ARG_ENABLE(quiet,
  [AS_HELP_STRING([--enable-quiet], [enable quiet building [default=no]])],
  [case ${enableval} in
    yes|no) ;;
    *) AC_MSG_ERROR([bad value ${enableval} for --enable-quiet]) ;;
  esac])

dnl Pass `--quiet' to libtool if quiet building is requested.  Using an
dnl AC_SUBST-ed value like this allows the choice to be easily overridden
dnl if you want to see exactly what make and libtool are up to - just run
dnl make like this: `make QUIET='
QUIET=
if test yes = "$enable_quiet"; then
  QUIET=--quiet
fi
AC_SUBST(QUIET)

AC_ARG_ENABLE(debuglog,
  [AS_HELP_STRING([--enable-debuglog], [enable debug logging [default=no]])],
  [case ${enableval} in
    yes|no) ;;
    *) AC_MSG_ERROR([bad value ${enableval} for --enable-debuglog]) ;;
  esac])
if test yes = "$enable_debuglog"; then
  AC_DEFINE([RESTPOSE_ENABLE_DEBUGLOG], [1],
	    [Define to 1 to enable debug log messages])
fi

vpath_build=no
if test "`pwd`" != "`cd $srcdir;pwd`" ; then
  vpath_build=yes
fi
AM_CONDITIONAL(VPATH_BUILD, test yes = "$vpath_build")

dnl Turn off compilation of anything that we don't have the requirements for

dnl Set conditionals to specify what we compile

AM_CONDITIONAL(BUILD_MODULE_MONGODB, test yes = "$enable_module_mongodb")

dnl ******************************
dnl * Set special compiler flags *
dnl ******************************

dnl Set flags to control warnings (enable more, or disable annoying ones).
if test yes = "$GXX"; then
  dnl Intel's C++ compiler is identified as "GXX" by autoconf's test - check
  dnl which we actually have.
  AC_EGREP_CPP(yes,
    [#ifdef __INTEL_COMPILER
     yes
     #endif
    ],
    [
      dnl Intel's compiler:
      dnl
      dnl -w1 stops the avalanche of uninteresting "remark" messages.
      dnl -wd... disables warnings which don't have good code workarounds.
      AM_CXXFLAGS="$AM_CXXFLAGS -Wall -w1 -wd177,1572"
      dnl Automatically add -Werror if maintainer mode is enabled.
      if test x$USE_MAINTAINER_MODE = xyes; then
	AM_CXXFLAGS="$AM_CXXFLAGS -Werror"
      fi
    ],
    [
      dnl GCC:
      dnl
      dnl -Wundef was supported by g++ 3.0 and since we now require GCC >= 3.1,
      dnl we can rely on it.
      dnl
      dnl All the other options were supported by g++ 2.95.
      AM_CXXFLAGS="$AM_CXXFLAGS -Wall -W -Wredundant-decls -Wpointer-arith -Wcast-qual -Wcast-align -Wno-long-long -Wformat-security -fno-gnu-keywords -Wundef -Wshadow -Woverloaded-virtual"

      dnl The output of g++ --version seems to change with almost every minor
      dnl release so use the preprocessor macros which should be more robust.
      gxx_major_version=`echo __GNUC__|$CXX -E -|sed '/^#/d;s/ //g'`
      gxx_minor_version=`echo __GNUC_MINOR__|$CXX -E -|sed '/^#/d;s/ //g'`
      gxx_version=$gxx_major_version.$gxx_minor_version

      case $gxx_version in
      [[012]].* | 3.0)
	AC_MSG_ERROR([Xapian requires GCC 3.1 or later (you appear to have $gxx_version)])
	;;
      3.*) ;;
      *) dnl GCC >= 4.0
	case $gxx_version in
	4.0) ;;
	4.1)
	  dnl -Wstrict-null-sentinel was new in 4.0.1, but for simplicity we
	  dnl only enable it for GCC >= 4.1.
	  dnl
	  dnl -Wshadow was supported by g++ 3.0 (though buggy then).  But it's
	  dnl also buggy in 4.0 (at least on Mac OS X) and warns if a parameter
	  dnl in a static method is the same as a member variable, so only
	  dnl enable it for GCC >= 4.1.
	  AM_CXXFLAGS="$AM_CXXFLAGS -Wstrict-null-sentinel -Wshadow" ;;
	4.2)
	  dnl -Wstrict-overflow was new in GCC 4.2.  Higher settings than 1
	  dnl have proved too noisy.
	  dnl
	  dnl Also, xapian-core seems to hit an aliasing bug in GCC 4.2:
	  dnl   http://gcc.gnu.org/bugzilla/show_bug.cgi?id=35643
	  dnl GCC 4.1 and 4.3 produce working code.  This bug can be avoided
	  dnl by disabling aliasing with -fno-strict-aliasing, so we do that.
	  dnl See http://trac.xapian.org/ticket/449 for more details.
	  AM_CXXFLAGS="$AM_CXXFLAGS -Wstrict-null-sentinel -Wshadow -Wstrict-overflow=1 -fno-strict-aliasing" ;;
	*)
	  dnl -Wlogical-op and -Wmissing-declarations (for C++) were added in
	  dnl GCC 4.3.
	  dnl
	  dnl -Winit-self was added in GCC 3.4, but up to at least GCC 4.4 it
	  dnl is ignored by g++ which warns for this case with -Wuninitialized
	  dnl (implied by -W).  But at some point this bug (filed upstream as
	  dnl http://gcc.gnu.org/PR34772) may get fixed so we pass this option
	  dnl anyway - we don't intend to use this idiom, so any instances are
	  dnl bugs we'd like to know about.
	  AM_CXXFLAGS="$AM_CXXFLAGS -Wstrict-null-sentinel -Wshadow -Wstrict-overflow=1 -Winit-self -Wlogical-op -Wmissing-declarations" ;;
	esac

	dnl FIXME:
	dnl -Wconversion in older GCC versions is intended to help migration
	dnl from K&R to ISO C, and isn't useful for us.  In 4.3 it was renamed
	dnl to -Wtraditional-conversion and a new -Wconversion added which
	dnl sounds useful but is a bit buggy currently.  So we should consider
	dnl enabling -Wconversion once it is stabilised (GCC 4.4 or ...)
	dnl
	dnl -Wold-style-cast is interesting, but triggers for macros from
	dnl system headers (e.g. FD_SET) (tested with GCC 4.4).

	dnl Automatically add -Werror if maintainer mode is enabled and we're
	dnl using GCC4 or newer.  We don't do this for older GCCs as GCC 2.95
	dnl and some GCC 3.x compilers issue spurious warnings.
	if test x$USE_MAINTAINER_MODE = xyes; then
	  case $gxx_version in
	  4.0)
	    dnl Apple's GCC 4.0 issues spurious uninitialised warnings.
	    ;;
	  *)
	    AM_CXXFLAGS="$AM_CXXFLAGS -Werror" ;;
	  esac
	fi
	;;
      esac
    ])
else
  dnl WARNING_CXXFLAGS is set above.
  AM_CXXFLAGS="$AM_CXXFLAGS $WARNING_CXXFLAGS"
fi

AC_MSG_CHECKING([whether to use SSE instructions on x86])
case $host_cpu in
  i*86)
    if test "$enable_sse" = no ; then
      AC_MSG_RESULT([no])
    else
      dnl Default to sse2.
      test "$enable_sse" != yes || enable_sse=sse2
      case $compiler in
      gcc)
	AC_MSG_RESULT([yes (configure with --disable-sse to disable)])
	case $gxx_version in
	3.*|4.[[01]]) dnl -mtune=generic added in GCC 4.2.
	  AM_CXXFLAGS="$AM_CXXFLAGS -mfpmath=sse -m$enable_sse" ;;
	*)
	  AM_CXXFLAGS="$AM_CXXFLAGS -mfpmath=sse -m$enable_sse -mtune=generic" ;;
	esac
	;;
      sun)
	AC_MSG_RESULT([yes (configure with --disable-sse to disable)])
	AM_CXXFLAGS="$AM_CXXFLAGS -xarch=$enable_sse"
	;;
      *)
        AC_MSG_RESULT([don't know how to for compiler $CXX])
	;;
      esac
    fi
    ;;
  *)
    AC_MSG_RESULT([non-x86 arch ($host_cpu)]) ;;
esac

AH_BOTTOM(
[/* Disable stupid MSVC "performance" warning for converting int to bool. */
#ifdef _MSC_VER
# pragma warning(disable:4800)
#endif

/* _FORTIFY_SOURCE is only supported by GCC >= 4.1 and glibc >= 2.3.4, but it
 * shouldn't cause a problem to define it where it's not supported and some
 * distros may have backported support, so hardcoding version checks is
 * counter-productive.
 *
 * Check if _FORTIFY_SOURCE is already defined to allow the user to override
 * our choice with "./configure CPPFLAGS=-D_FORTIFY_SOURCE=0" or "...=1".
 */
#if defined __GNUC__ && !defined _FORTIFY_SOURCE
# define _FORTIFY_SOURCE 2
#endif

/* For GCC >= 3.0 (and Intel's C++ compiler, which also defines __GNUC__),
 * we can use __builtin_expect to give the compiler hints about branch
 * prediction.  See HACKING for how to use these.
 */
#if defined __GNUC__
/* The arguments of __builtin_expect() are both long, so use !! to ensure that
 * the first argument is always an integer expression, and always 0 or 1, but
 * still has the same truth value for the if or while it is used in.
 */
# define rare(COND) __builtin_expect(!!(COND), 0)
# define usual(COND) __builtin_expect(!!(COND), 1)
#else
# define rare(COND) (COND)
# define usual(COND) (COND)
#endif
])

AC_SUBST(AM_CXXFLAGS)

dnl Restore CXXFLAGS to those the user specified or autoconf defaulted to.
CXXFLAGS=$save_CXXFLAGS

dnl See if large file support is available
AC_SYS_LARGEFILE

dnl Split up the version number into "MAJOR.MINOR.REVISION".
MAJOR_VERSION=`echo "$VERSION"|sed 's/\..*//'`
[MINOR_VERSION=`echo "$VERSION"|sed 's/[^.]*\.//;s/\..*//'`]
[REVISION=`echo "$VERSION"|sed 's/.*\.//;s/_svn[0-9]*$//'`]

dnl ***************
dnl * Subprojects *
dnl ***************

AC_CONFIG_SUBDIRS([libs/libmicrohttpd])

dnl **************************
dnl * Build the output files *
dnl **************************

AC_CONFIG_FILES([
 Makefile
 ])
AC_OUTPUT

dnl Required for auto regeneration to work in a combined maintainer-mode tree.
: ${AUTOM4TE=autom4te}
AC_SUBST([AUTOM4TE])
