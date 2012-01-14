/* gnome-exec.c - Execute some command.

   Copyright (C) 1998 Tom Tromey
   All rights reserved.

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin Street,
   Fifth Floor, Boston, MA  02110-1301 USA.  */
/*
  @NOTATION@
 */
/*
#include <config.h>

#include "gnome-i18nP.h"
*/

#include "gnome-exec.h"
#include "gnome-util.h"
/*
#include "gnome-i18n.h"
#include "gnome-gconfP.h"
#include "gnome-init.h"
*/
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#ifndef G_OS_WIN32
#include <sys/wait.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/*
#include <gconf/gconf-client.h>
*/

#include <popt.h>

#include <errno.h>
#ifndef errno
extern int errno;
#endif

#ifndef G_OS_WIN32

static void
set_cloexec (gint fd)
{
  fcntl (fd, F_SETFD, FD_CLOEXEC);
}

static ssize_t
safe_read (int fd, void *buf, size_t count)
{
  ssize_t n;

  while ((n = read (fd, buf, count)) < 0 && (errno == EINTR || errno == EAGAIN));

  return n;
}

#endif

typedef struct {
  int pipe_out[2];
  int pipe_err[2];
} out_err_t;

static void 
redirect_fd(int old_fd, int new_fd)
{
  dup2(new_fd, old_fd);
  close(new_fd);
}

static void
close_oe_ends(out_err_t* out_err, gboolean is_in)
{
  close(out_err->pipe_out[is_in ? 0 : 1]);
  close(out_err->pipe_err[is_in ? 0 : 1]);
}

static int
gnome_execute_async_with_env_fds_redirect (const char *dir, int argc, 
					   char * const argv[], int envc, 
					   char * const envv[], 
					   gboolean close_fds,
					   out_err_t* out_err)
{
#ifndef G_OS_WIN32
  int parent_comm_pipes[2], child_comm_pipes[2];
  int child_errno, itmp, i, open_max;
  gssize res;
  char **cpargv;
  pid_t child_pid, immediate_child_pid; /* XXX this routine assumes
					   pid_t is signed */

  if(pipe(parent_comm_pipes))
    return -1;

  child_pid = immediate_child_pid = fork();

  switch(child_pid) {
  case -1:
    close(parent_comm_pipes[0]);
    close(parent_comm_pipes[1]);

    /* Murav'jov */
    if(out_err)
      {
	close_oe_ends(out_err, TRUE);
	close_oe_ends(out_err, FALSE);
      }
    return -1;

  case 0: /* START PROCESS 1: child */
    /* Murav'jov */
    if(out_err)
      {
	close_oe_ends(out_err, TRUE);
	redirect_fd(1, out_err->pipe_out[1]);
	redirect_fd(2, out_err->pipe_err[1]);
      }

    child_pid = -1;
    res = pipe(child_comm_pipes);
    close(parent_comm_pipes[0]);
    if(!res)
      child_pid = fork();

    switch(child_pid) {
    case -1:
      itmp = errno;
      child_pid = -1; /* simplify parent code */
      write(parent_comm_pipes[1], &child_pid, sizeof(child_pid));
      write(parent_comm_pipes[1], &itmp, sizeof(itmp));
      close(child_comm_pipes[0]);
      close(child_comm_pipes[1]);
      _exit(0); break;      /* END PROCESS 1: monkey in the middle dies */

    default:
      {
	char buf[16];
	
	close(child_comm_pipes[1]);
	while((res = safe_read(child_comm_pipes[0], buf, sizeof(buf))) > 0)
	  write(parent_comm_pipes[1], buf, res);
	close(child_comm_pipes[0]);
	_exit(0); /* END PROCESS 1: monkey in the middle dies */
      }
      break;

    case 0:                 /* START PROCESS 2: child of child */
      close(parent_comm_pipes[1]);
      /* pre-exec setup */
      close (child_comm_pipes[0]);
      set_cloexec (child_comm_pipes[1]);
      child_pid = getpid();
      res = write(child_comm_pipes[1], &child_pid, sizeof(child_pid));

      if(envv) {
	for(itmp = 0; itmp < envc; itmp++)
	  putenv(envv[itmp]);
      }

      if(dir) {
        if(chdir(dir))
          _exit(-1);  
      }
      
      cpargv = g_alloca((argc + 1) * sizeof(char *));
      memcpy(cpargv, argv, argc * sizeof(char *));
      cpargv[argc] = NULL;

      if(close_fds)
	{
	  int stdinfd;
	  /* Close all file descriptors but stdin stdout and stderr */
	  open_max = sysconf (_SC_OPEN_MAX);
	  for (i = 3; i < open_max; i++)
	    set_cloexec (i);

	  if(child_comm_pipes[1] != 0) {
	    close(0);
	    /* Open stdin as being nothingness, so that if someone tries to
	       read from this they don't hang up the whole GNOME session. BUGFIX #1548 */
	    stdinfd = open("/dev/null", O_RDONLY);
	    g_assert(stdinfd >= 0);
	    if(stdinfd != 0)
	      {
		dup2(stdinfd, 0);
		close(stdinfd);
	      }
	  }
	}
      setsid ();
      signal (SIGPIPE, SIG_DFL);
      /* doit */
      execvp(cpargv[0], cpargv);

      /* failed */
      itmp = errno;
      write(child_comm_pipes[1], &itmp, sizeof(itmp));
      _exit(1); break;      /* END PROCESS 2 */
    }
    break;

  default: /* parent process */
    /* do nothing */
    break;
  }

  close(parent_comm_pipes[1]);
  /* Murav'jov */
  if(out_err) close_oe_ends(out_err, FALSE);

  res = safe_read (parent_comm_pipes[0], &child_pid, sizeof(child_pid));
  if (res != sizeof(child_pid))
    {
      g_message("res is %ld instead of %d",
		(long)res, (int)sizeof(child_pid));
      child_pid = -1; /* really weird things happened */
    }
  else if (safe_read (parent_comm_pipes[0], &child_errno, sizeof(child_errno))
	  == sizeof(child_errno))
    {
      errno = child_errno;
      child_pid = -1;
    }

  /* do this after the read's in case some OS's handle blocking on pipe writes
     differently */
   while ((waitpid(immediate_child_pid, &itmp, 0)== -1) && (errno == EINTR)); /* eat zombies */

  close(parent_comm_pipes[0]);

  if(child_pid < 0)
    g_message("gnome_execute_async_with_env_fds: returning %d", child_pid);

  return child_pid;
#else
  /* FIXME: Implement if needed */
  g_warning ("gnome_execute_async_with_env_fds: Not implemented");

  return -1;
#endif
}

/**
 * gnome_execute_async_with_env_fds:
 * @dir: Directory in which child should be executed, or %NULL for current
 *       directory
 * @argc: Number of arguments
 * @argv: Argument vector to exec child
 * @envc: Number of environment slots
 * @envv: Environment vector
 * @close_fds: If %TRUE will close all fds but 0,1, and 2
 * 
 * Description:  Like gnome_execute_async_with_env() but has a flag to
 * decide whether or not to close fd's
 * 
 * Returns: the process id, or %-1 on error.
 **/
int
gnome_execute_async_with_env_fds (const char *dir, int argc, 
				  char * const argv[], int envc, 
				  char * const envv[], 
				  gboolean close_fds)
{
  return gnome_execute_async_with_env_fds_redirect (dir, argc, argv, envc, envv, close_fds, NULL);
}

/**
 * gnome_execute_async_with_env:
 * @dir: Directory in which child should be executed, or NULL for current
 *       directory
 * @argc: Number of arguments
 * @argv: Argument vector to exec child
 * @envc: Number of environment slots
 * @envv: Environment vector
 * 
 * Description: This function forks and executes some program in the
 * background.  On error, returns %-1; in this case, #errno should hold a useful
 * value.  Searches the path to find the child.  Environment settings in @envv
 * are added to the existing environment -- they do not completely replace it.
 * This function closes all fds besides 0, 1, and 2 for the child
 * 
 * Returns: the process id, or %-1 on error.
 **/
int
gnome_execute_async_with_env (const char *dir, int argc, char * const argv[], 
			      int envc, char * const envv[])
{
  return gnome_execute_async_with_env_fds (dir, argc, argv, envc, envv, TRUE);
}


/**
 * gnome_execute_async:
 * @dir: Directory in which child should be executesd, or %NULL for current
 *       directory
 * @argc: Number of arguments
 * @argv: Argument vector to exec child
 * 
 * Description: Like gnome_execute_async_with_env(), but doesn't add anything
 * to child's environment.
 * 
 * Returns: process id of child, or %-1 on error.
 **/
int
gnome_execute_async (const char *dir, int argc, char * const argv[])
{
  return gnome_execute_async_with_env (dir, argc, argv, 0, NULL);
}

/**
 * gnome_execute_async_fds:
 * @dir: Directory in which child should be executed, or %NULL for current
 *       directory
 * @argc: Number of arguments
 * @argv: Argument vector to exec child
 * @close_fds: If %TRUE, will close all but file descriptors 0, 1 and 2.
 *
 * Description: Like gnome_execute_async_with_env_fds(), but doesn't add
 * anything to child's environment.
 * 
 * Returns: process id of child, or %-1 on error.
 **/
int
gnome_execute_async_fds (const char *dir, int argc, 
			 char * const argv[], gboolean close_fds)
{
  return gnome_execute_async_with_env_fds (dir, argc, argv, 0, NULL, 
					   close_fds);
}

static int
gnome_execute_shell_fds_redirect (const char *dir, const char *commandline,
				  gboolean close_fds, out_err_t* out_err)
{
  char *user_shell;
  char * argv[4];
  int r;

  g_return_val_if_fail(commandline != NULL, -1);

  user_shell = gnome_util_user_shell ();

  argv[0] = user_shell;
  argv[1] = "-c";
  /* necessary cast, to avoid warning, but safe */
  argv[2] = (char *)commandline;
  argv[3] = NULL;

  r = gnome_execute_async_with_env_fds_redirect (dir, 4, argv, 0, NULL, close_fds, out_err);

  g_free (user_shell);
  return r;
}

/**
 * gnome_execute_shell_fds:
 * @dir: Directory in which child should be executed, or %NULL for current
 *       directory
 * @commandline: Shell command to execute
 * @close_fds: Like close_fds in gnome_execute_async_with_env_fds()
 *
 * Description: Like gnome_execute_async_with_env_fds(), but uses the user's
 * shell to run the desired program.  Note that the pid of the shell is
 * returned, not the pid of the user's program.
 * 
 * Returns: process id of shell, or %-1 on error.
 **/
int
gnome_execute_shell_fds (const char *dir, const char *commandline,
			 gboolean close_fds)
{
  return gnome_execute_shell_fds_redirect(dir, commandline, close_fds, NULL);
}

/**
 * gnome_execute_shell:
 * @dir: Directory in which child should be executed, or %NULL for current
 *       directory
 * @commandline: Shell command to execute
 * 
 * Description: Like gnome_execute_async_with_env(), but uses the user's shell
 * to run the desired program.  Note that the pid of the shell is returned, not
 * the pid of the user's program.
 * 
 * Returns: process id of shell, or %-1 on error.
 **/
int
gnome_execute_shell (const char *dir, const char *commandline)
{
  return gnome_execute_shell_fds(dir, commandline, TRUE);
}

/************************************************************************************/

/**
 * @author Ilya Murav'jov (2/4/2009) 
 *  
 * gnome_execute_shell_redirect: same as gnome_execute_shell(), 
 * but forces output (stdout and stderr) of child go to 
 * out_err[0] and out_err[1]. 
 */
int
gnome_execute_shell_redirect (const char *dir, const char *commandline, int out_err[2])
{
  out_err_t oe;
  if(pipe(oe.pipe_out) || pipe(oe.pipe_err))
    return -1;

  out_err[0] = oe.pipe_out[0];
  out_err[1] = oe.pipe_err[0];
  return gnome_execute_shell_fds_redirect (dir, commandline, TRUE, &oe);
}
