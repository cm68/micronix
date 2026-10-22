#!/usr/bin/env python3
#
# nettestmix - mixed binaries across three usersim machines
#
#	a -- b -- c, two hops, stock b and c
#	ours-netcp	our netcp on a, stock netdaemon on a
#	ours-daemon	stock netcp on a, our netdaemon on a
#
# The b and c daemons are the recovered 1.6 binaries in both modes, so
# this is the interoperability test: our spool writer or our daemon
# against the stock daemon chain.
#
#	run:	python3 -I src/usersim/nettestmix.py ours-netcp
#		python3 -I src/usersim/nettestmix.py ours-daemon
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
OUR_NC = os.path.join(CMD, 'netcp/netcp')
OUR_ND = os.path.join(CMD, 'netdaemon/netdaemon')
NETACT = os.path.join(CMD, 'netdaemon/netact.bin')

PASSWD = ('root::0:0:Super user:/:/bin/sh\n'
          'network::9:7::/a/network:/bin/sh\n')
GROUP = ('software::1:\nhardware::2:\ndocum::3:\nadmin::4:\n'
         'sales::5:\nsupport::6:\ndaemon::7:\n')

A = '/tmp/mix/a'
B = '/tmp/mix/b'
C = '/tmp/mix/c'
WAIT = 100


def machine(root, me, body, ttys, nc, nd):
    shutil.rmtree(root, ignore_errors=True)
    for d in ('bin', 'etc', 'dev', 'tmp', 'a/network',
              'usr/spool/net/in', 'usr/spool/net/out'):
        os.makedirs(os.path.join(root, d))
    for src, name in ((nc, 'netcp'), (nd, 'netdaemon'), (NETACT, 'netact')):
        dst = os.path.join(root, 'bin', name)
        shutil.copy(src, dst)
        os.chmod(dst, 0o755)
    open(os.path.join(root, 'etc/netmap'), 'w').write(me + '\n' + body + '\n')
    open(os.path.join(root, 'etc/ttys'), 'w').write(ttys)
    open(os.path.join(root, 'etc/passwd'), 'w').write(PASSWD)
    open(os.path.join(root, 'etc/group'), 'w').write(GROUP)


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
    os.system("ps -ef | grep '/usersim/sim ' | grep '/tmp/mix/' | "
              "grep -v grep | awk '{print $2}' | xargs -r kill -9")


def main():
    if len(sys.argv) < 2 or sys.argv[1] not in ('ours-netcp', 'ours-daemon'):
        print('usage: nettestmix.py ours-netcp|ours-daemon')
        return 1
    mode = sys.argv[1]
    for p in (SIM, STOCK_NC, STOCK_ND, OUR_NC, OUR_ND, NETACT):
        if not os.path.exists(p):
            print('missing', p)
            return 1
    if mode == 'ours-netcp':
        anc, and_ = OUR_NC, STOCK_ND
    else:
        anc, and_ = STOCK_NC, OUR_ND
    machine(A, 'a', 'b c', 'ttyD 1200 net-b\n', anc, and_)
    machine(B, 'b', 'a\nc', 'ttyD 1200 net-a\nttyE 1200 net-c\n',
            STOCK_NC, STOCK_ND)
    machine(C, 'c', 'b a', 'ttyD 1200 net-b\n', STOCK_NC, STOCK_ND)
    push = b'hello from a to c\n'
    pull = b'c motd across two hops\n'
    open(os.path.join(A, 'tmp/hello'), 'wb').write(push)
    open(os.path.join(C, 'etc/motd'), 'wb').write(pull)

    a1, s1 = line(A)
    a2, s2 = line(B)
    b1, t1 = line(B, 'ttyE')
    b2, t2 = line(C)
    cons = []
    for root in (A, B, C):
        m, s = pty.openpty()
        tty.setraw(s)
        cons.append((m, s))
        subprocess.Popen([SIM, '-d', root, '/bin/netdaemon'],
                         stdin=s, stdout=s, stderr=s)
    caps = [bytearray() for _ in range(4)]
    stop = [False]
    th1 = threading.Thread(target=pump,
                           args=(a1, a2, caps[0], caps[1], stop), daemon=True)
    th2 = threading.Thread(target=pump,
                           args=(b1, b2, caps[2], caps[3], stop), daemon=True)
    th1.start()
    th2.start()
    time.sleep(2)

    p = subprocess.run([SIM, '-d', A, '/bin/netcp', '/tmp/hello',
                        'c%/tmp/hello'],
                       stdin=cons[0][1], stdout=cons[0][1],
                       stderr=cons[0][1], timeout=20)
    print('push', p.returncode)
    time.sleep(WAIT)
    got = os.path.join(C, 'tmp/hello')
    pushed = open(got, 'rb').read() if os.path.exists(got) else b''
    print('push arrived:', repr(pushed))

    p = subprocess.run([SIM, '-d', A, '/bin/netcp', 'c%/etc/motd',
                        '/tmp/fetched'],
                       stdin=cons[0][1], stdout=cons[0][1],
                       stderr=cons[0][1], timeout=20)
    print('pull', p.returncode)
    time.sleep(WAIT)
    got = os.path.join(A, 'tmp/fetched')
    fetched = open(got, 'rb').read() if os.path.exists(got) else b''
    print('pull arrived:', repr(fetched))

    stop[0] = True
    th1.join(timeout=1)
    th2.join(timeout=1)
    ok = pushed == push and fetched == pull
    if not ok:
        for name, cap in zip(('a -> b', 'b -> a', 'b -> c', 'c -> b'), caps):
            print('%s (%d):' % (name, len(cap)))
            print(repr(bytes(cap)))
    kill_sims()
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
