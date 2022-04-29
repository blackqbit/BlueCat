  /*
   * This is the BlueCat installation program.
   *
   */

#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#ifdef __CYGWIN32__
#include <getopt.h>
#include <dirent.h>
#define TEMP1 "/tmp/__install_1"
#define TEMP2 "/tmp/__install_2"
#endif

static char	ins_cbuf[256];
static char	ins_vbuf[256];
static int	ins_verbose = 1;

static int	do_install(char *list, char *path);

#ifdef __CYGWIN32__
static int 	check_empty	(void);
static int	isdotdir	(char *);
static int 	parse_mnttab 	(char *);
static void 	fill_registry	(char *);
static char *	utodos		(char *, int);
static char *	dostodos		(char *, int);

#endif

int	main(
    int		argc,
    char **	argv)
{
    char	c;
    char	binwd[256];
    char *	srcdir;

    if (srcdir = rindex(argv[0], '/'))
    {
	*++srcdir = '\0';
    }
    srcdir = argv[0];

    while ((c = getopt(argc, argv, "v")) != -1)
    {
	switch(c)
	{
	    case 'v':
		ins_verbose = 0;
		break;
	    default:
		goto fail;
	}
    }

    argc -= optind;
    argv += optind;

    if (argc && chdir(*argv) == -1)
    {
	perror("chdir");
	goto fail;
    }

#ifdef __CYGWIN32__
    if (! check_empty())
#else
    if (system("/bin/ls -ald * &>/dev/null") == 0 ||
	system("/bin/ls -ald .??* &>/dev/null") == 0)
#endif
    {
	fprintf(stderr, "Installation directory must be empty\n");
	goto fail;
    }

    printf("Installing BlueCat Linux: [verbosity = %d]\n\n", 
           ins_verbose);

    getcwd(binwd, 200);
#ifdef __CYGWIN32__
    if (binwd[0] != '/' || binwd[1] != '/')
    {
        /* Change mount point in the working directory name to the real directory name
         */

        if (! parse_mnttab(binwd))
          goto fail;
    }
#endif
    setenv("BLUECAT_PREFIX", binwd, 1);

#define DOBUF					\
    if (ins_verbose) printf("%s", ins_vbuf);	\
    if (system(ins_cbuf) != 0) goto fail;

    sprintf(ins_vbuf, "Installing SETUP.sh\n");
    sprintf(ins_cbuf, "/bin/cp %sBlueCat_" TARGET_CPU "/SETUP.sh .",
	    srcdir);
    DOBUF;

    sprintf(ins_vbuf, "Preparing installation tools");
    sprintf(ins_cbuf, "/bin/mkdir -p var/lib/rpm  var/tmp cdt/lib");
    DOBUF;
    sprintf(ins_vbuf, "\n");
    sprintf(ins_cbuf, "/bin/cp -r %sBlueCat_" TARGET_CPU
	    "/tools/rpm cdt/lib",
	    srcdir);
    DOBUF;
#ifdef __CYGWIN32__
    sprintf(ins_cbuf, "/bin/chmod -R u+w cdt/lib/rpm");
    DOBUF;
#endif

      /*
       * Install rmps according to given lists
       */
    chdir(srcdir);
    sprintf(ins_vbuf, "");
    sprintf(ins_cbuf, "./BlueCat_" TARGET_CPU "/tools/bin/rpm --initdb");
    DOBUF;

    if (do_install("BlueCat_" TARGET_CPU "/tools/etc/rpm_cdt.list",
                   "BlueCat_" TARGET_CPU "/cdt/RPMS") == -1 ||
        do_install("BlueCat_" TARGET_CPU "/tools/etc/rpm_target_"
	    TARGET_CPU ".list",
	    "BlueCat_" TARGET_CPU "/target/RPMS") == -1)
    {
fail:
	fprintf(stderr, "install: terminating\n");
	exit(1);
    }

#ifdef __CYGWIN__
    fill_registry(binwd);
#endif

    printf("\nInstallation complete ... Execute . SETUP.sh\n");
    return 0;
}

  /*
   * Install rpms according to given list
   */
static int	do_install(char *list, char * path)
{
    FILE *	f;
    char	buf[256];
    char *	ind;

    if ((f = fopen(list, "r")) == NULL)
    {
	fprintf(stderr, "error opening file %s\n", list);
	perror("fopen");
	return -1;
    }
    strcpy(buf, path);
    ind = buf + strlen(path);
    *ind++ = '/';
    while (fgets(ind, 200, f))
    {
	  /*
	   * Skip empty lines and comments
	   */
	if (*ind == '#' || *ind == '\n') continue;

        sprintf(ins_vbuf, "Installing %s", buf);
	sprintf(ins_cbuf,
	        "./BlueCat_" TARGET_CPU "/tools/bin/rpm -i --nodeps %s",
	        buf);
	if (ins_verbose)
	{
	    printf("%s", ins_vbuf);
	}
	if (system(ins_cbuf) != 0)
	{
	    return -1;
	}
    }

    return 0;
}

#ifdef __CYGWIN32__
  /* 
   * Check if the current directory is empty.
   */
static int check_empty (void)
{
  DIR *dd;
  struct dirent *dp;
  int ret = 1;

  if (! (dd = opendir(".")))
  {
    perror(".");
    return 0;
  }
  while ((dp = readdir(dd)) != NULL) 
  {
    if (dp->d_name && !isdotdir(dp->d_name))
    {
      ret = 0;
      break;
    }
  }
  closedir(dd);
  return ret;
}

static int isdotdir (
  char *	s)
{
  return (*s == '.' && (!s[1] || (s[1] == '.' && !s[2])));
}

  /* 
   * Parse table of mounts.
   */
static int parse_mnttab (
  char *		path)
{
  FILE *		fp = popen("mount", "r");
  char 		str [512];
  char * 		ptr;
  char *		endp;
  char *		mpath = NULL;

  if (fp == NULL)
  {
    fprintf(stderr, "Can't exec mount\n");
    return 0;
  }
  
  while (fgets(str, 512, fp))
  {
    if (!strncmp(str, "Device", 6))
      continue;
    endp = strchr(str, ' ');
    *endp = 0;
    ptr = endp + 1;
    while (*ptr == ' ') ptr++ ;
    endp = strchr(ptr, ' ');
    *endp = 0;
   
      /* If path begins with mount point change it to real directory.
       */

    if (! strncmp(path, ptr, strlen(ptr)))
    {
      int len = strlen(ptr);
      int len1, len2;
      char *p;
      char *dp;

      dp = str;

      mpath = p = alloca(strlen(dp) + 3);
      if ( *(dp + 1) == ':')
      {
         *p = '/';
         *(p + 1) = '/';
         *(p + 2) = *dp;
         p += 3;
         dp += 2;
      }
      while (*dp)
      {
         if (*dp == '\\')
         {
           *p++ = '/';
           dp++;
         } else 
           *p++ = *dp++;
      }
      if (*(p - 1) != '/' && *(path + len) != '/')
        *p++ = '/';
      *p = '\0';
      len1 = strlen(mpath);
      len2 = strlen(path) + 1;

      memmove(path + len1, path + len, len2 - len);
      strncpy(path, mpath, len1);
      break;
    }
  }
  pclose(fp);
  if (*path == '/' && *(path + 1) == '/')
    return 1;
  fprintf(stderr, "Can't get drive letter for %s\n", path);
  return 0;
}

static void fill_registry(char *instdir)
{
  char s[512];
  char *name1 = TEMP1;
  char *name2 = TEMP2;
  char *dospath;

  sprintf(s, "sed -e 's,__VER__,%s,g' ./BlueCat_%s/tools/etc/BlueCat.reg >%s\n", 
          VERSION, TARGET_CPU, name1);
  if (system(s) != 0)
  {
    fprintf(stderr, "Cannot modify registry file\n");
    goto ret;
    return;
  }
  sprintf(s, "sed -e 's,__BLUECAT_PREFIX__,%s,g' %s >%s", instdir, name1, name2);
  if (system(s) != 0)
  {
    fprintf(stderr, "Cannot modify registry file\n");
    goto ret;
  }
  dospath = utodos(instdir, 4);
  sprintf(s, "sed -e 's,__INSTALL_DIR__,%s,g' %s >%s", dospath, name2, name1);
  if (system(s) != 0)
  {
    fprintf(stderr, "Cannot modify registry file\n");
    goto ret;
  }
  sprintf(s, "sed -e 's,__TARGET_CPU__,%s,g' %s >%s", TARGET_CPU, name1, name2);
  if (system(s) != 0)
  {
    fprintf(stderr, "Cannot modify registry file\n");
    goto ret;
  }
  sprintf(s, 
	  "sed -e 's,__TARGET_PLATFORM__,%s,g' %s >%s", 
          strcmp(TARGET_CPU, "i386") ? TARGET_CPU : "x86",
	  name2, 
	  name1);
  if (system(s) != 0)
  {
    fprintf(stderr, "Cannot modify registry file\n");
    goto ret;
  }
  dospath = dostodos(getenv("TARGET_PATH"), 4);
  sprintf(s, "sed -e 's,__CYGWIN_DIR__,%s,g' %s >%s", dospath, name1, name2);
  if (system(s) != 0)
  {
    fprintf(stderr, "Cannot modify registry file\n");
    goto ret;
  }
  sprintf(s, "cp %s %s/cdt/etc/BlueCat.reg", name2, instdir);
  if (system(s) != 0)
  {
    fprintf(stderr, "Cannot save registry file\n");
  }
  dospath = utodos(instdir, 2);
  sprintf(s, "regedit /s %s\\\\cdt\\\\etc\\\\BlueCat.reg", dospath);
  if (system(s) != 0)
  {
    fprintf(stderr, "Cannot save registry file\n");
  }

ret:
  unlink(name1);
  unlink(name2);
}

static char *utodos (
  char *	path,
  int		n)
{
  char *s = malloc(512);
  char *p = s;
  int i;

  if (*path == '/' && *(path + 1) == '/')
  {
    *p++ = *(path + 2);
    *p++ = ':';
    path += 3;
  }
  while(*path)
  {
    if(*path == '/')
    {
      for(i=0; i<n; i++)
        *p++ = '\\';
      path++;
    }
    else
      *p++ = *path++;
  }
  *p = '\0';
  return s;
}

static char *dostodos (
  char *	path,
  int		n)
{
  char *s = malloc(512);
  char *p = s;
  int i;

  while(*path)
  {
    if(*path == '\\')
    {
      for(i=0; i<n; i++)
        *p++ = '\\';
      path++;
    }
    else
      *p++ = *path++;
  }
  *p = '\0';
  return s;
}
#endif

  /*
   * End of File
   */

