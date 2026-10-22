#!/usr/bin/env python3
#
# nettestmail - the mail path across usersim machines
#
#	dest	a -- b, stock a, our netdaemon at the destination b
#	relay	a -- b -- c, stock a and c, our netdaemon relaying at b
#
# The sender is the stock mail(1); a network name makes it exec
# /bin/netmail, which writes an M record to /usr/spool/net/out.  The
# daemon transports it, and at the destination netact execs mail, which
# puts the message in /usr/spool/mail/<user>.
#
#	run:	python3 -I src/usersim/nettestmail.py dest
#		python3 -I src/usersim/nettestmail.py relay
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:
#
import os
import pty
import select
import shutil
import subprocess
import sys
import threading
import time
import tty

HERE = os.path.dirname(os.path.abspath(__file__))
TOP = os.path.abspath(os.path.join(HERE, '..', '..'))
SIM = os.path.join(HERE, 'sim')
CMD = os.path.join(TOP, 'src/micronix/cmd')
STOCK_NC = os.path.join(CMD, 'netcp/netcp.bin')
STOCK_ND = os.path.join(CMD, 'netdaemon/netdaemon.bin')
OUR_ND = os.path.join(CMD, 'netdaemon/netdaemon')
NETACT = os.path.join(CMD, 'netdaemon/netact.bin')
MAIL = os.path.join(CMD, 'mail/mail16.bin')
NETMAIL = os.path.join(CMD, 'netmail/netmail16.bin')

PASSWD = ('root::0:0:Super user:/:/bin/sh\n'
          'network::9:7::/a/network:/bin/sh\n')
GROUP = ('software::1:\nhardware::2:\ndocum::3:\nadmin::4:\n'
         'sales::5:\nsupport::6:\ndaemon::7:\n')

ROOT = '/tmp/mailtest'
A = ROOT + '/a'
B = ROOT + '/b'
C = ROOT + '/c'
WAIT = 100


def machine(root, me, body, ttys, nd):
    shutil.rmtree(root, ignore_errors=True)
    for d in ('bin', 'etc', 'dev', 'tmp', 'a/network', 'usr/spool/mail',
              'usr/spool/net/in', 'usr/spool/net/out'):
        os.makedirs(os.path.join(root, d))
    for src, name in ((STOCK_NC, 'netcp'), (nd, 'netdaemon'),
                      (NETACT, 'netact'), (MAIL, 'mail'),
                      (NETMAIL, 'netmail')):
        dst = os.path.join(root, 'bin', name)
        shutil.copy(src, dst)
        os.chmod(dst, 0o755)
    open(os.path.join(root, 'etc/netmap'), 'w').write(me + '\n' + body + '\n')
    open(os.path.join(root, 'etc/ttys'), 'w').write(ttys)
    open(os.path.join(root, 'etc/passwd'), 'w').write(PASSWD)
    open(os.path.join(root, 'etc/group'), 'w').write(GROUP)
    open(os.path.join(root, 'etc/utmp'), 'w').close()


def line(root, dev='ttyD'):
    m, s = pty.openpty()
    tty.setraw(s)
    os.symlink(os.ttyname(s), os.path.join(root, 'dev', dev))
    return m, s


def pump(m1, m2, left, right, stop):
    while not stop[0]:
        r, _, _ = select.select([m1, m2], [], [], 0.2)
        for fd in r:
            try:
                b = os.read(fd, 4096)
            except OSError:
                continue
            if not b:
                continue
            if fd == m1:
                left.extend(b)
                os.write(m2, b)
            else:
                right.extend(b)
                os.write(m1, b)


def kill_sims():
    os.system("ps -ef | grep '/usersim/sim ' | grep '%s/' | "
              "grep -v grep | awk '{print $2}' | xargs -r kill -9" % ROOT)


def delivered(root, user='root'):
    p = os.path.join(root, 'usr/spool/mail', user)
    return open(p, 'rb').read() if os.path.exists(p) else b''


def main():
    if len(sys.argv) < 2 or sys.argv[1] not in ('dest', 'relay'):
        print('usage: nettestmail.py dest|relay')
        return 1
    mode = sys.argv[1]
    for p in (SIM, STOCK_NC, STOCK_ND, OUR_ND, NETACT, MAIL, NETMAIL):
        if not os.path.exists(p):
            print('missing', p)
            return 1

    text = b'Mail across two hops\n\t-Jill\n'
    if mode == 'dest':
        machine(A, 'a', 'b', 'ttyD 1200 net-b\n', STOCK_ND)
        machine(B, 'b', 'a', 'ttyD 1200 net-a\n', OUR_ND)
        target, dest = 'b%root', B
        byhand = [A, B]
    else:
        machine(A, 'a', 'b c', 'ttyD 1200 net-b\n', STOCK_ND)
        machine(B, 'b', 'a\nc', 'ttyD 1200 net-a\nttyE 1200 net-c\n',
                OUR_ND)
        machine(C, 'c', 'b a', 'ttyD 1200 net-b\n', STOCK_ND)
        target, dest = 'c%root', C
        byhand = [A, B, C]

    a1, s1 = line(A)
    a2, s2 = line(B)
    cons = []
    for root in byhand:
        m, s = pty.openpty()
        tty.setraw(s)
        cons.append((m, s))
        subprocess.Popen([SIM, '-d', root, '/bin/netdaemon'],
                         stdin=s, stdout=s, stderr=s)
    caps = [bytearray() for _ in range((len(byhand) - 1) * 2)]
    stop = [False]
    threads = [threading.Thread(target=pump, args=(a1, a2, caps[0], caps[1],
                                                   stop), daemon=True)]
    if mode == 'relay':
        b1, t1 = line(B, 'ttyE')
        b2, t2 = line(C)
        threads.append(threading.Thread(target=pump,
                                        args=(b1, b2, caps[2], caps[3], stop),
                                        daemon=True))
    for th in threads:
        th.start()
    time.sleep(2)

    msg = os.path.join(ROOT, 'msg')
    open(msg, 'wb').write(text)
    with open(msg, 'rb') as fp:
        p = subprocess.run([SIM, '-d', A, '/bin/mail', target], stdin=fp,
                           stdout=subprocess.DEVNULL,
                           stderr=subprocess.DEVNULL, timeout=20)
    print('mail', p.returncode)
    time.sleep(WAIT)

    got = delivered(dest)
    print('mail at', dest, ':')
    print(' ', repr(got))
    ok = text in got and b'From a%root' in got

    stop[0] = True
    for th in threads:
        th.join(timeout=1)
    if not ok:
        names = ['a -> b', 'b -> a']
        if mode == 'relay':
            names += ['b -> c', 'c -> b']
        for name, cap in zip(names, caps):
            print('%s (%d):' % (name, len(cap)))
            print(repr(bytes(cap)))
    kill_sims()
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
