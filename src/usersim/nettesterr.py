#!/usr/bin/env python3
#
# nettesterr - inject line errors into a reconstruction transfer
#
# Two usersim machines, our netcp and netdaemon on each, one pty.  The
# bridge corrupts or drops one line in the a->b direction and the test
# checks that the transfer still arrives:
#
#	chk	a D record's checksum is corrupted; the receiver NAKs and
#		the sender retransmits the record
#	P	the proposal's checksum is corrupted; NAK and a new proposal
#	seq	a D record's sequence is changed and its checksum recomputed;
#		the receiver NAKs and the sender resends the record
#	drop	a D record is dropped; the end record's sequence no longer
#		matches, the receiver NAKs and discards the partial file, and
#		the sender retries the whole transfer
#
#	run:	python3 -I src/usersim/nettesterr.py
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
NC = os.path.join(TOP, 'src/micronix/cmd/netcp/netcp')
ND = os.path.join(TOP, 'src/micronix/cmd/netdaemon/netdaemon')
PAYLOAD = b'hello, network\n'


def cksum(b):
    s = 0
    for c in b + b'\n':
        s = (s + c) & 0xffff
        s = ((s << 1) | (s >> 15)) & 0xffff
    return s


def prefix(b):
    s = cksum(b)
    return bytes([((s >> sh) & 0x3f) + 0x20 for sh in (0, 6, 12)])


def machine(root, me, peer):
    shutil.rmtree(root, ignore_errors=True)
    for d in ('bin', 'etc', 'dev', 'tmp', 'usr/spool/net/in',
              'usr/spool/net/out'):
        os.makedirs(os.path.join(root, d))
    for src, name in ((NC, 'netcp'), (ND, 'netdaemon')):
        dst = os.path.join(root, 'bin', name)
        shutil.copy(src, dst)
        os.chmod(dst, 0o755)
    open(os.path.join(root, 'etc/netmap'), 'w').write(me + '\n' + peer + '\n')
    open(os.path.join(root, 'etc/ttys'), 'w').write(
        'ttyD 1200 net-' + peer + '\n')
    open(os.path.join(root, 'etc/passwd'), 'w').write(
        'root::0:0:root:/:/bin/sh\n')


def run(mode):
    base = '/tmp/netterr/' + mode
    a = os.path.join(base, 'a')
    b = os.path.join(base, 'b')
    machine(a, 'a', 'b')
    machine(b, 'b', 'a')
    open(os.path.join(a, 'tmp/hello'), 'wb').write(PAYLOAD)
    m1, s1 = pty.openpty()
    m2, s2 = pty.openpty()
    tty.setraw(s1)
    tty.setraw(s2)
    os.symlink(os.ttyname(s1), os.path.join(a, 'dev/ttyD'))
    os.symlink(os.ttyname(s2), os.path.join(b, 'dev/ttyD'))
    c1m, c1s = pty.openpty()
    tty.setraw(c1s)
    c2m, c2s = pty.openpty()
    tty.setraw(c2s)
    subprocess.Popen([SIM, '-d', a, '/bin/netdaemon'],
                     stdin=c1s, stdout=c1s, stderr=c1s)
    subprocess.Popen([SIM, '-d', b, '/bin/netdaemon'],
                     stdin=c2s, stdout=c2s, stderr=c2s)

    stop = [False]
    to_b = bytearray()
    to_a = bytearray()
    done = [False]
    buf = bytearray()

    def pump():
        while not stop[0]:
            r, _, _ = select.select([m1, m2], [], [], 0.2)
            for fd in r:
                try:
                    x = os.read(fd, 4096)
                except OSError:
                    continue
                if not x:
                    continue
                if fd == m1:
                    buf.extend(x)
                    while b'\n' in buf:
                        i = buf.index(b'\n')
                        line = bytes(buf[:i + 1])
                        del buf[:i + 1]
                        if not done[0]:
                            if mode == 'chk' and line[3:4] == b'D':
                                line = bytes([line[0] ^ 1]) + line[1:]
                                done[0] = True
                            elif mode == 'P' and line[3:5] == b'P!':
                                line = bytes([line[0] ^ 1]) + line[1:]
                                done[0] = True
                            elif mode == 'seq' and line[3:4] == b'D':
                                pay = bytearray(line[3:-1])
                                pay[1] = ((pay[1] - 0x20 + 1) & 0x3f) + 0x20
                                line = prefix(bytes(pay)) + bytes(pay) + b'\n'
                                done[0] = True
                            elif mode == 'drop' and line[3:4] == b'D':
                                done[0] = True
                                continue
                        to_b.extend(line)
                        os.write(m2, line)
                else:
                    to_a.extend(x)
                    os.write(m1, x)

    th = threading.Thread(target=pump, daemon=True)
    th.start()
    time.sleep(1)
    subprocess.run([SIM, '-d', a, '/bin/netcp', '/tmp/hello',
                    'b%/tmp/hello'], stdin=c1s, stdout=c1s,
                   stderr=c1s, timeout=20)
    time.sleep(12)
    stop[0] = True
    th.join(timeout=1)
    got = open(os.path.join(b, 'tmp/hello'), 'rb').read() \
        if os.path.exists(os.path.join(b, 'tmp/hello')) else b''
    print('==', mode, 'arrived:', repr(got))
    print('   a -> b:', repr(bytes(to_b)))
    print('   b -> a:', repr(bytes(to_a)))
    os.system("ps -ef | grep '/usersim/sim ' | grep '/tmp/netterr/" + mode +
              "/' | grep -v grep | awk '{print $2}' | xargs -r kill -9")
    if got != PAYLOAD:
        return 0
    return 1


def main():
    for p in (SIM, NC, ND):
        if not os.path.exists(p):
            print('missing', p)
            return 1
    ok = 1
    for mode in ('chk', 'P', 'seq', 'drop'):
        if not run(mode):
            ok = 0
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
