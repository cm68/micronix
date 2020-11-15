/*
 * reg.c 
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/proc.h>

extern long seconds;

extern int arg[];               /* in-line arguments from system() */

/*
 * Handle register passing across system calls
 */

/*
 * First, pass registers across the system interface
 */
r_system()
{
    system();
    if (u.error) {
        u.af |= ERRBIT;
        u.hl = u.error;
    } else
        u.af &= ~ERRBIT;
}

r_alarm()
{
    u.hl = alarm(u.hl);
}

r_close()
{
    close(u.hl);
}

r_creat()
{
    u.hl = creat(arg[0], arg[1]);
}

r_csw()
{
    u.hl = csw();
}

r_ssw()
{
    ssw(u.hl);
}

r_dup()
{
    u.hl = dup(u.hl);
}

r_exit()
{
    exit(u.hl);
}

r_fork()
{
    int id;

    if ((id = fork()) != 0) {   /* no reg changes in child */
        u.pc += 3;              /* old process return */
        u.hl = id;
    }
}

r_fstat()
{
    fstat(u.hl, arg[0]);
}

r_getpid()
{
    u.hl = getpid();
}

r_getuid()
{
    u.hl = getuid();
}

r_gtty()
{
    gtty(u.hl, arg[0]);
}

r_kill()
{
    kill(u.hl, arg[0]);
}

r_nice()
{
    nice(u.hl);
}

r_open()
{
    u.hl = open(arg[0], arg[1]);
}

r_pipe()
{
    int vec[2];

    pipe(vec);
    u.hl = vec[0];
    u.de = vec[1];
}

r_read()
{
    u.hl = read(u.hl, arg[0], arg[1]);
}

r_seek()
{
    seek(u.hl, arg[0], arg[1]);
}

r_setuid()
{
    setuid(u.hl);
}

r_signal()
{
    u.hl = signal(arg[0], arg[1]);
}

r_sleep()
{
    snooze(u.hl);
}

r_stime()
{
    stime(u.hl, u.de);
}

r_stty()
{
    stty(u.hl, arg[0]);
}

r_time()
{
    u.de = seconds;
    u.hl = seconds >> 16;
}

r_wait()
{
    u.hl = wait(&u.de);
}

r_write()
{
    u.hl = write(u.hl, arg[0], arg[1]);
}

/*
 * r_lock () { reclock (u.hl, arg [0]); }
 * 
 * r_unlock () { unlock (u.hl); } 
 */
/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
